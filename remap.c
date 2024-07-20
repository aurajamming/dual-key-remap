#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "input.h"
#include "keys.c"

// Types
// --------------------------------------

enum State {
    IDLE,
    HELD_DOWN_ALONE,
    HELD_DOWN_WITH_OTHER,
    TAP,
    TAPPED,
    DOUBLE_TAP,
};

struct KeyDefNode {
    KEY_DEF * key_def;

    struct KeyDefNode * next;
    struct KeyDefNode * previous;
};

struct LayerNode {
    struct Layer * layer;

    struct LayerNode * next;
};

struct Layer {
    char * name;
    int state;
    int lock, prev_lock;
    struct LayerNode * and_master_layers;
    struct LayerNode * and_not_master_layers;
    struct LayerNode * and_slave_layers;

    struct Layer * next;
};

void toggle_layer_lock(struct Layer * layer) {
    layer->prev_lock = layer->lock;
    layer->lock = 1 - layer->lock;
}

void set_layer_lock(struct Layer * layer) {
    layer->prev_lock = layer->lock;
    layer->lock = 1;
}

void reset_layer_lock(struct Layer * layer) {
    layer->prev_lock = layer->lock;
    layer->lock = 0;
}

int check_layer_states(struct LayerNode * layer_list, int expected_state) {
    while (layer_list) {
        if (layer_list->layer->state != expected_state) {
            return 0;
        }
        layer_list = layer_list->next;
    }
    return 1;
}

void set_layer_state(struct Layer * layer, int state) {
    //if (!layer) return;
    layer->state = state;
    struct LayerNode * slave_iter = layer->and_slave_layers;
    while (slave_iter) {
        struct Layer * slave_layer = slave_iter->layer;
        int all_master_active = check_layer_states(slave_layer->and_master_layers, 1) &&
            check_layer_states(slave_layer->and_not_master_layers, 0);
        set_layer_state(slave_layer, all_master_active ? 1 : slave_layer->lock);
        slave_iter = slave_iter->next;
    }
}

struct LayerConf {
    struct Layer * layer;
    void (*conf)(struct Layer * layer);

    struct LayerConf * next;
};

struct Remap {
    int id;
    KEY_DEF * from;
    struct Layer * layer;
    struct Layer * to_when_press_layer;
    struct Layer * to_when_doublepress_layer;
    struct LayerConf * to_when_tap_lock_layer;
    struct LayerConf * to_when_double_tap_lock_layer;
    struct KeyDefNode * to_when_alone;
    struct KeyDefNode * to_with_other;
    struct KeyDefNode * to_when_doublepress;
    struct KeyDefNode * to_when_tap_lock;
    struct KeyDefNode * to_when_double_tap_lock;
    int to_when_alone_modifiers;
    int to_with_other_modifiers;
    int to_when_doublepress_modifiers;
    int to_when_tap_lock_modifiers;
    int to_when_double_tap_lock_modifiers;
    int tap_lock;
    int double_tap_lock;

    enum State state;
    DWORD time;
    int active_modifiers;

    struct Remap * next;
};

struct RemapNode {
    struct Remap * remap;

    struct RemapNode * next;
};

// Globals
// --------------------------------------

int g_debug = 0;
int g_hold_delay = 0;
int g_tap_timeout = 0;
int g_doublepress_timeout = 0;
int g_rehook_timeout = 1000;
int g_unlock_timeout = 60000;
int g_scancode = 0;
int g_priority = 1;
int g_last_input = 0;
struct Remap * g_remap_list = NULL;
struct Remap * g_remap_parsee = NULL;
struct Remap * g_remap_by_id[256] = {NULL};
struct RemapNode * g_remap_array[256] = {NULL};
struct Layer * g_layer_list = NULL;
struct Layer * g_layer_parsee = NULL;

// Debug Logging
// --------------------------------------

char * fmt_dir(enum Direction direction) {
    return (direction == DOWN) ? "DOWN" : "UP";
}

int log_indent_level = 0;
int log_counter = 1;
void print_log_prefix() {
    printf("\n%03d. ", log_counter++);
    for (int i = 0; i < log_indent_level; i++)
    {
        printf("\t");
    }
}

void log_handle_input_start(int scan_code, int virt_code, enum Direction direction, int is_injected, DWORD flags, ULONG_PTR dwExtraInfo) {
    if (!g_debug) return;
    print_log_prefix();
    printf("[%s] %s %s (scan:0x%04X virt:0x%02X flags:0x%02X dwExtraInfo:0x%IX)",
           (is_injected && ((dwExtraInfo & 0xFFFFFF00) == INJECTED_KEY_ID)) ? "output" : "input",
           friendly_virt_code_name(virt_code),
           fmt_dir(direction),
           scan_code, // MapVirtualKeyA(virt_code, MAPVK_VK_TO_VSC_EX)
           virt_code,
           flags,
           dwExtraInfo);
    log_indent_level++;
}

void log_handle_input_end(int scan_code, int virt_code, enum Direction direction, int block_input) {
    if (!g_debug) return;
    log_indent_level--;
    if (block_input) {
        print_log_prefix();
        printf("#blocked-input# %s %s",
            friendly_virt_code_name(virt_code),
            fmt_dir(direction));
    }
}

void log_send_input(char * remap_name, KEY_DEF * key, enum Direction direction) {
    if (!g_debug) return;
    print_log_prefix();
    printf("(sending:%s) %s %s",
        remap_name,
        key ? key->name : "???",
        fmt_dir(direction));
}

// Remapping
// -------------------------------------

struct KeyDefNode * new_key_node(KEY_DEF * key_def) {
    struct KeyDefNode * key_node = malloc(sizeof(struct KeyDefNode));
    key_node->key_def = key_def;
    key_node->next = key_node;
    key_node->previous = key_node;
    return key_node;
}

struct LayerNode * new_layer_node(struct Layer * layer) {
    struct LayerNode * layer_node = malloc(sizeof(struct LayerNode));
    layer_node->layer = layer;
    layer_node->next = NULL;
    return layer_node;
}

struct Layer * new_layer(char * name) {
    struct Layer * layer = malloc(sizeof(struct Layer));
    layer->name = strdup(name);
    layer->state = 0;
    layer->lock = 0;
    layer->prev_lock = 0;
    layer->and_master_layers = NULL;
    layer->and_not_master_layers = NULL;
    layer->and_slave_layers = NULL;
    layer->next = NULL;
    return layer;
}

struct LayerConf * new_layer_conf(struct Layer * layer, void (*conf)(struct Layer * layer)) {
    struct LayerConf * layer_conf = malloc(sizeof(struct LayerConf));
    layer_conf->layer = layer;
    layer_conf->conf = conf;
    layer_conf->next = NULL;
    return layer_conf;
}

struct Remap * new_remap(KEY_DEF * from,
                         struct Layer * layer,
                         struct KeyDefNode * to_when_alone,
                         struct KeyDefNode * to_with_other,
                         struct KeyDefNode * to_when_doublepress,
                         struct KeyDefNode * to_when_tap_lock,
                         struct KeyDefNode * to_when_double_tap_lock) {
    struct Remap * remap = malloc(sizeof(struct Remap));
    remap->id = 0;
    remap->from = from;
    remap->layer = layer;
    remap->to_when_press_layer = NULL;
    remap->to_when_doublepress_layer = NULL;
    remap->to_when_tap_lock_layer = NULL;
    remap->to_when_double_tap_lock_layer = NULL;
    remap->to_when_alone = to_when_alone;
    remap->to_with_other = to_with_other;
    remap->to_when_doublepress = to_when_doublepress;
    remap->to_when_tap_lock = to_when_tap_lock;
    remap->to_when_double_tap_lock = to_when_double_tap_lock;
    remap->to_when_alone_modifiers = 0;
    remap->to_with_other_modifiers = 0;
    remap->to_when_doublepress_modifiers = 0;
    remap->to_when_tap_lock_modifiers = 0;
    remap->to_when_double_tap_lock_modifiers = 0;
    remap->tap_lock = 0;
    remap->double_tap_lock = 0;
    remap->state = IDLE;
    remap->time = 0;
    remap->active_modifiers = 0;
    remap->next = NULL;
    return remap;
}

struct RemapNode * new_remap_node(struct Remap * remap) {
    struct RemapNode * remap_node = malloc(sizeof(struct RemapNode));
    remap_node->remap = remap;
    remap_node->next = NULL;
    return remap_node;
}

void append_key_node(struct KeyDefNode * head, KEY_DEF * key_def) {
    head->previous->next = new_key_node(key_def);
    head->previous->next->previous = head->previous;
    head->previous = head->previous->next;
    head->previous->next = head;
}

void append_layer_node(struct LayerNode ** list, struct LayerNode * elem) {
    while (*list) list = &(*list)->next;
    *list = elem;
}

int is_master_layer(struct Layer * master_layer, struct Layer * slave_layer) {
    struct LayerNode * master_iter = slave_layer->and_master_layers;
    while (master_iter) {
        if (master_iter->layer == master_layer || is_master_layer(master_layer, master_iter->layer)) {
            if (check_layer_states(slave_layer->and_master_layers, 1) &&
                check_layer_states(slave_layer->and_not_master_layers, 0)) {
                return 1;
            }
            return 0;
        }
        master_iter = master_iter->next;
    }
    return 0;
}

int has_to_block_modifiers(struct Remap * remap, struct Layer * layer) {
    return remap && remap->layer &&
        (remap->layer == layer || is_master_layer(layer, remap->layer));
}

int key_eq(struct KeyDefNode * head_a, struct KeyDefNode * head_b) {
    struct KeyDefNode * cur_a = head_a;
    struct KeyDefNode * cur_b = head_b;
    while (cur_a && cur_b) {
        if (cur_a->key_def != cur_b->key_def) return 0;
        cur_a = cur_a->next;
        cur_b = cur_b->next;
        if (cur_a == head_a && cur_b == head_b) return 1;
    }
    return 0;
}

int modifiers(struct KeyDefNode * head) {
    struct KeyDefNode * cur = head;
    int modifiers = 0;
    do {
        modifiers |= cur->key_def->modifier;
        cur = cur->next;
    } while (cur != head);
    return modifiers;
}

void free_key_nodes(struct KeyDefNode * head) {
    if (head) {
        head->previous->next = NULL;
        struct KeyDefNode * cur = head;
        while (cur) {
            struct KeyDefNode * key_node = cur;
            cur = cur->next;
            free(key_node);
        }
    }
}

void free_layer_nodes(struct LayerNode * head) {
    struct LayerNode * cur = head;
    while (cur) {
        struct LayerNode * layer_node = cur;
        cur = cur->next;
        free(layer_node);
    }
}

void free_layers(struct Layer * head) {
    struct Layer * cur = head;
    while (cur) {
        struct Layer * layer = cur;
        cur = cur->next;
        free(layer->name);
        free_layer_nodes(layer->and_master_layers);
        free_layer_nodes(layer->and_not_master_layers);
        free_layer_nodes(layer->and_slave_layers);
        free(layer);
    }
}

void free_layer_confs(struct LayerConf * head) {
    struct LayerConf * cur = head;
    while (cur) {
        struct LayerConf * layer_conf = cur;
        cur = cur->next;
        free(layer_conf);
    }
}

void free_remap(struct Remap * remap) {
    free_layer_confs(remap->to_when_tap_lock_layer);
    free_layer_confs(remap->to_when_double_tap_lock_layer);
    free_key_nodes(remap->to_when_alone);
    free_key_nodes(remap->to_with_other);
    free_key_nodes(remap->to_when_doublepress);
    free_key_nodes(remap->to_when_tap_lock);
    free_key_nodes(remap->to_when_double_tap_lock);
    free(remap);
}

void free_remap_nodes(struct RemapNode * head) {
    struct RemapNode * cur = head;
    while (cur) {
        struct RemapNode * remap_node = cur;
        cur = cur->next;
        free_remap(remap_node->remap);
        free(remap_node);
    }
}

void free_all() {
    free(g_remap_parsee);
    g_remap_parsee = NULL;
    g_layer_parsee = NULL;
    g_remap_list = NULL;
    free_layers(g_layer_list);
    g_layer_list = NULL;
    for (int i = 0; i < 256; i++) {
        free_remap_nodes(g_remap_array[i]);
        g_remap_array[i] = NULL;
        g_remap_by_id[i] = NULL;
    }
}

struct Layer * find_layer(struct Layer * list, char * name) {
    while (list) {
        if (strcmp(list->name, name) == 0) {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

struct Layer * append_layer(struct Layer ** list, struct Layer * elem) {
    while (*list) list = &(*list)->next;
    *list = elem;
    return *list;
}

void append_layer_conf(struct LayerConf ** list, struct LayerConf * elem) {
    while (*list) list = &(*list)->next;
    *list = elem;
}

int register_remap(struct Remap * remap) {
    if (g_remap_list) {
        struct Remap * tail = g_remap_list;
        while (tail->next) tail = tail->next;
        if (tail->id == 255) return 1;
        tail->next = remap;
        remap->id = tail->id + 1;
    } else {
        g_remap_list = remap;
        remap->id = 1;
    }
    g_remap_by_id[remap->id] = remap;
    if (key_eq(remap->to_when_alone, remap->to_with_other)) {
        free_key_nodes(remap->to_with_other);
        remap->to_with_other = NULL;
    }
    if (key_eq(remap->to_when_alone, remap->to_when_doublepress)) {
        free_key_nodes(remap->to_when_doublepress);
        remap->to_when_doublepress = NULL;
    }
    if (remap->to_when_alone) {
        remap->to_when_alone_modifiers = modifiers(remap->to_when_alone);
    }
    if (remap->to_with_other) {
        remap->to_with_other_modifiers = modifiers(remap->to_with_other);
        if (!remap->to_with_other_modifiers) {
            free_key_nodes(remap->to_with_other);
            remap->to_with_other = NULL;
            remap->to_with_other_modifiers = 0;
        }
    }
    if (remap->to_when_doublepress) {
        remap->to_when_doublepress_modifiers = modifiers(remap->to_when_doublepress);
    }
    if (remap->to_when_tap_lock) {
        remap->to_when_tap_lock_modifiers = modifiers(remap->to_when_tap_lock);
    }
    if (remap->to_when_double_tap_lock) {
        remap->to_when_double_tap_lock_modifiers = modifiers(remap->to_when_double_tap_lock);
    }
    return 0;
}

struct Remap * find_remap_for_virt_code(struct Remap * list, int virt_code) {
    while (list) {
        if (list->from->virt_code == virt_code) {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

int remap_list_depth() {
    int depth = 0;
    struct Remap * remap_iter = g_remap_list;
    while (remap_iter) {
        depth++;
        remap_iter = remap_iter->next;
    }
    return depth;
}

void append_active_remap(struct Remap ** list, struct Remap * elem) {
    while (*list) {
        if (*list == elem) {
            DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
            return;
        }
        list = &(*list)->next;
    }
    *list = elem;
    elem->next = NULL;
    DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
}

void remove_active_remap(struct Remap ** list, struct Remap * elem) {
    while (*list && *list != elem) {
        list = &(*list)->next;
    }
    if (*list) {
        *list = (*list)->next;
        elem->next = NULL;
    }
    DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
}

int send_key_def_input_down(char * input_name, struct KeyDefNode * head, int remap_id, int modifiers_mask, struct InputBuffer * input_buffer) {
    int key_sent = 0;
    struct KeyDefNode * cur = head;
    do {
        if (!(modifiers_mask & KEY_ARRAY[cur->key_def->virt_code & 0xFF].modifier)) {
            log_send_input(input_name, cur->key_def, DOWN);
            send_input(cur->key_def->scan_code, cur->key_def->virt_code, DOWN, remap_id, input_buffer);
            key_sent = 1;
        }
        cur = cur->next;
    } while (cur != head);
    return key_sent;
}

int send_key_def_input_up(char * input_name, struct KeyDefNode * head, int remap_id, int modifiers_mask, struct InputBuffer * input_buffer) {
    int key_sent = 0;
    struct KeyDefNode * cur = head;
    do {
        cur = cur->previous;
        if (!(modifiers_mask & KEY_ARRAY[cur->key_def->virt_code & 0xFF].modifier)) {
            log_send_input(input_name, cur->key_def, UP);
            send_input(cur->key_def->scan_code, cur->key_def->virt_code, UP, remap_id, input_buffer);
            key_sent = 1;
        }
    } while (cur != head);
    return key_sent;
}

void unlock_all(struct InputBuffer * input_buffer) {
    struct Layer * layer_iter = g_layer_list;
    while (layer_iter) {
        layer_iter->state = 0;
        layer_iter->lock = 0;
        layer_iter->prev_lock = 0;
        layer_iter = layer_iter->next;
    }
    struct Remap * remap_iter = g_remap_list;
    while (remap_iter) {
        g_remap_list = remap_iter->next;
        if (remap_iter->state == HELD_DOWN_ALONE) {
        } else if (remap_iter->state == HELD_DOWN_WITH_OTHER) {
            if (remap_iter->to_with_other) {
                send_key_def_input_up("unlock_with_other", remap_iter->to_with_other, remap_iter->id, 0, input_buffer);
            }
        } else if (remap_iter->state == TAP) {
            if (remap_iter->to_when_alone) {
                send_key_def_input_up("unlock_when_alone", remap_iter->to_when_alone, remap_iter->id, 0, input_buffer);
            }
        } else if (remap_iter->state == DOUBLE_TAP) {
            if (remap_iter->to_when_doublepress) {
                send_key_def_input_up("unlock_when_doublepress", remap_iter->to_when_doublepress, remap_iter->id, 0, input_buffer);
            }
        }
        if (remap_iter->double_tap_lock) {
            send_key_def_input_up("unlock_when_double_tap_lock", remap_iter->to_when_double_tap_lock, remap_iter->id, 0, input_buffer);
            remap_iter->double_tap_lock = 0;
        }
        if (remap_iter->tap_lock) {
            send_key_def_input_up("unlock_when_tap_lock", remap_iter->to_when_tap_lock, remap_iter->id, 0, input_buffer);
            remap_iter->tap_lock = 0;
        }
        remap_iter->state = IDLE;
        remap_iter->active_modifiers = 0;
        remap_iter->next = NULL;
        remap_iter = g_remap_list;
        DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
    }
}

/* @return block_input */
int event_remapped_key_down(struct Remap * remap, DWORD time, struct InputBuffer * input_buffer) {
    if (remap->state == IDLE) {
        if (remap->to_with_other) {
            remap->time = time;
            remap->state = HELD_DOWN_ALONE;
        } else {
            remap->time = time;
            remap->state = TAP;
            if (remap->to_when_alone) {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
                remap->active_modifiers = remap->to_when_alone_modifiers;
            }
        }
        if (remap->to_when_press_layer) {
            set_layer_state(remap->to_when_press_layer, 1);
        }
        append_active_remap(&g_remap_list, remap);
    } else if (remap->state == HELD_DOWN_WITH_OTHER) {
        if (remap->to_with_other) {
            send_key_def_input_down("with_other", remap->to_with_other, remap->id, 0, input_buffer);
        }
    } else if (remap->state == TAP) {
        if (remap->to_when_alone) {
            send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
        }
    } else if (remap->state == TAPPED) {
        if ((g_doublepress_timeout > 0) && (time - remap->time < g_doublepress_timeout)) {
            remap->time = time;
            remap->state = DOUBLE_TAP;
            if (remap->to_when_tap_lock) {
                remap->tap_lock = 1 - remap->tap_lock;
                if (remap->tap_lock == 0) {
                    send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
                    remap->active_modifiers = 0;
                }
            }
            struct LayerConf * layer_conf = remap->to_when_tap_lock_layer;
            while (layer_conf) {
                layer_conf->layer->lock = layer_conf->layer->prev_lock;
                set_layer_state(layer_conf->layer, layer_conf->layer->lock);
                layer_conf = layer_conf->next;
            }
            if (remap->to_when_doublepress_layer) {
                set_layer_state(remap->to_when_doublepress_layer, 1);
            }
            if (remap->to_when_doublepress) {
                send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, 0, input_buffer);
                remap->active_modifiers = remap->to_when_doublepress_modifiers;
            } else if (remap->to_when_doublepress_layer) {
            } else if (remap->to_when_alone) {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
                remap->active_modifiers = remap->to_when_alone_modifiers;
            }
        } else {
            if (remap->to_with_other) {
                remap->time = time;
                remap->state = HELD_DOWN_ALONE;
            } else {
                remap->time = time;
                remap->state = TAP;
                if (remap->to_when_alone) {
                    send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
                    remap->active_modifiers = remap->to_when_alone_modifiers;
                }
            }
            if (remap->to_when_press_layer) {
                set_layer_state(remap->to_when_press_layer, 1);
            }
        }
        append_active_remap(&g_remap_list, remap);
    } else if (remap->state == DOUBLE_TAP) {
        if (remap->to_when_doublepress) {
            send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, 0, input_buffer);
        } else if (remap->to_when_doublepress_layer) {
        } else if (remap->to_when_alone) {
            send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
        }
    }
    return 1;
}

/* @return block_input */
int event_remapped_key_up(struct Remap * remap, DWORD time, struct InputBuffer * input_buffer) {
    if (remap->state == HELD_DOWN_ALONE) {
        if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
            remap->time = time;
            remap->state = TAPPED;
            if (remap->to_when_alone) {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
                send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
            }
            if (remap->to_when_tap_lock) {
                remap->tap_lock = 1 - remap->tap_lock;
                if (remap->tap_lock) {
                    send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
                    remap->active_modifiers = remap->to_when_tap_lock_modifiers;
                } else {
                    send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
                    remap->active_modifiers = 0;
                }
            }
            struct LayerConf * layer_conf = remap->to_when_tap_lock_layer;
            while (layer_conf) {
                layer_conf->conf(layer_conf->layer);
                set_layer_state(layer_conf->layer, layer_conf->layer->lock);
                layer_conf = layer_conf->next;
            }
        } else {
            remap->state = IDLE;
        }
        if (remap->to_when_press_layer) {
            set_layer_state(remap->to_when_press_layer, remap->to_when_press_layer->lock);
        }
    } else if (remap->state == HELD_DOWN_WITH_OTHER) {
        remap->state = IDLE;
        if (remap->to_with_other) {
            send_key_def_input_up("with_other", remap->to_with_other, remap->id, 0, input_buffer);
            remap->active_modifiers = 0;
        }
        if (remap->to_when_press_layer) {
            set_layer_state(remap->to_when_press_layer, remap->to_when_press_layer->lock);
        }
    } else if (remap->state == TAP) {
        if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
            remap->time = time;
            remap->state = TAPPED;
            if (remap->to_when_alone) {
                send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
                remap->active_modifiers = 0;
            }
            if (remap->to_when_tap_lock) {
                remap->tap_lock = 1 - remap->tap_lock;
                if (remap->tap_lock) {
                    send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
                    remap->active_modifiers = remap->to_when_tap_lock_modifiers;
                } else {
                    send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
                    remap->active_modifiers = 0;
                }
            }
            struct LayerConf * layer_conf = remap->to_when_tap_lock_layer;
            while (layer_conf) {
                layer_conf->conf(layer_conf->layer);
                set_layer_state(layer_conf->layer, layer_conf->layer->lock);
                layer_conf = layer_conf->next;
            }
        } else {
            remap->state = IDLE;
            if (remap->to_when_alone) {
                send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
                remap->active_modifiers = 0;
            }
        }
        if (remap->to_when_press_layer) {
            set_layer_state(remap->to_when_press_layer, remap->to_when_press_layer->lock);
        }
    } else if (remap->state == DOUBLE_TAP) {
        remap->state = IDLE;
        if (remap->to_when_doublepress) {
            send_key_def_input_up("when_doublepress", remap->to_when_doublepress, remap->id, 0, input_buffer);
            remap->active_modifiers = 0;
        } else if (remap->to_when_doublepress_layer) {
        } else if (remap->to_when_alone) {
            send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
            remap->active_modifiers = 0;
        }
        if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
            if (remap->to_when_double_tap_lock) {
                remap->double_tap_lock = 1 - remap->double_tap_lock;
                if (remap->double_tap_lock) {
                    send_key_def_input_down("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, 0, input_buffer);
                    remap->active_modifiers = remap->to_when_double_tap_lock_modifiers;
                } else {
                    send_key_def_input_up("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, 0, input_buffer);
                    remap->active_modifiers = 0;
                }
            }
            struct LayerConf * layer_conf = remap->to_when_double_tap_lock_layer;
            while (layer_conf) {
                layer_conf->conf(layer_conf->layer);
                set_layer_state(layer_conf->layer, layer_conf->layer->lock);
                layer_conf = layer_conf->next;
            }
        }
        if (remap->to_when_doublepress_layer) {
            set_layer_state(remap->to_when_doublepress_layer, remap->to_when_doublepress_layer->lock);
        }
    }
    if (remap->tap_lock == 0 && remap->double_tap_lock == 0) {
        remove_active_remap(&g_remap_list, remap);
    }
    return 1;
}

/* @return block_input */
int event_other_input(int virt_code, enum Direction direction, DWORD time, int remap_id, struct InputBuffer * input_buffer) {
    int block_input = 0;
    if (direction == DOWN && !KEY_ARRAY[virt_code & 0xFF].modifier) {
        struct Remap * remap = g_remap_list;
        while (remap) {
            if (remap->id != remap_id) {
                if (remap->state == HELD_DOWN_ALONE) {
                    if ((g_hold_delay > 0) && (time - remap->time < g_hold_delay) && remap->to_when_alone) {
                        remap->state = TAP;
                        block_input |= send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
                        remap->active_modifiers = remap->to_when_alone_modifiers;
                    } else {
                        if (!has_to_block_modifiers(g_remap_by_id[remap_id], remap->to_when_press_layer)) {
                            remap->state = HELD_DOWN_WITH_OTHER;
                            if (remap->to_with_other) {
                                block_input |= send_key_def_input_down("with_other", remap->to_with_other, remap->id, 0, input_buffer);
                                remap->active_modifiers = remap->to_with_other_modifiers;
                            }
                        }
                    }
                } else if (remap->state == HELD_DOWN_WITH_OTHER) {
                    if (remap->to_with_other) {
                        if (!has_to_block_modifiers(g_remap_by_id[remap_id], remap->to_when_press_layer)) {
                            block_input |= send_key_def_input_down("with_other", remap->to_with_other, remap->id, 0, input_buffer);
                        } else {
                            block_input |= send_key_def_input_up("with_other", remap->to_with_other, remap->id, g_remap_by_id[remap_id]->active_modifiers, input_buffer);
                        }
                    }
                } else if (remap->state == TAP) {
                    if (remap->to_when_alone && remap->to_when_alone_modifiers) {
                        if (!has_to_block_modifiers(g_remap_by_id[remap_id], remap->to_when_press_layer)) {
                            block_input |= send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, 0, input_buffer);
                        } else {
                            block_input |= send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, g_remap_by_id[remap_id]->active_modifiers, input_buffer);
                        }
                    }
                } else if (remap->state == DOUBLE_TAP) {
                    if (remap->to_when_doublepress && remap->to_when_doublepress_modifiers) {
                        if (!has_to_block_modifiers(g_remap_by_id[remap_id], remap->to_when_doublepress_layer)) {
                            block_input |= send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, 0, input_buffer);
                        } else {
                            block_input |= send_key_def_input_up("when_doublepress", remap->to_when_doublepress, remap->id, g_remap_by_id[remap_id]->active_modifiers, input_buffer);
                        }
                    }
                } else {
                    if (remap->double_tap_lock) {
                        block_input |= send_key_def_input_down("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, 0, input_buffer);
                    }
                    if (remap->tap_lock) {
                        block_input |= send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, 0, input_buffer);
                    }
                }
                remap->time = 0; // disable tap and double_tap
            }
            remap = remap->next;
        }
    }
    return -block_input;
}


/* @return block_input */
int handle_input(int scan_code, int virt_code, enum Direction direction, DWORD time, int is_injected, DWORD flags, ULONG_PTR dwExtraInfo, struct InputBuffer * input_buffer) {
    struct Remap * remap_for_input;
    int block_input;
    int remap_id = 0; // if 0 then no remapped injected key

    log_handle_input_start(scan_code, virt_code, direction, is_injected, flags, dwExtraInfo);
    if ((g_unlock_timeout > 0) && (time - g_last_input > g_unlock_timeout)) {
        unlock_all(input_buffer);
    }
    if (is_injected && ((dwExtraInfo & 0xFFFFFF00) != INJECTED_KEY_ID || dwExtraInfo == INJECTED_KEY_ID)) {
        // Note: passthrough of injected keys from other tools or
        //   from Dual-key-remap self when passthrough is requested (remap_id = 0).
        block_input = 0;
        if ((g_rehook_timeout > 0) && (time - g_last_input > g_rehook_timeout)) {
            rehook();
            g_last_input = time;
        }
    } else {
        g_last_input = time;
        if (is_injected) {
            // Note: injected keys are never remapped to avoid complex nested scenarios
            remap_for_input = NULL;
            remap_id = dwExtraInfo & 0x000000FF;
        } else {
            remap_for_input = find_remap_for_virt_code(g_remap_list, virt_code);
            if (remap_for_input == NULL) {
                struct RemapNode * remap_node_iter = g_remap_array[virt_code & 0xFF];
                while (remap_node_iter) {
                    if (remap_node_iter->remap->layer == NULL) {
                        break;
                    } else if (remap_node_iter->remap->layer->state) {
                        break;
                    }
                    remap_node_iter = remap_node_iter->next;
                }
                if (remap_node_iter) {
                    remap_for_input = remap_node_iter->remap;
                } else {
                    // auto_unlock if not modifier
                }
            }
        }
        if (remap_for_input) {
            if (direction == UP) {
                block_input = event_remapped_key_up(remap_for_input, time, input_buffer);
            } else {
                block_input = event_remapped_key_down(remap_for_input, time, input_buffer);
            }
        } else {
            block_input = event_other_input(virt_code, direction, time, remap_id, input_buffer);
        }
    }
    log_handle_input_end(scan_code, virt_code, direction, block_input);
    return block_input;
}

// Config
// ---------------------------------

void trim_newline(char * str) {
    str[strcspn(str, "\r\n")] = 0;
    int length = strlen(str);
    while (length > 0 && isspace((unsigned char)str[length - 1])) {
        str[length - 1] = 0;
        length--;
    }
}

int parsee_is_valid() {
    return g_remap_parsee &&
        g_remap_parsee->from &&
        (g_remap_parsee->to_when_alone || g_remap_parsee->to_with_other ||
         g_remap_parsee->to_when_doublepress ||
         g_remap_parsee->to_when_tap_lock || g_remap_parsee->to_when_double_tap_lock ||
         g_remap_parsee->to_when_press_layer || g_remap_parsee->to_when_doublepress_layer ||
         g_remap_parsee->to_when_tap_lock_layer || g_remap_parsee->to_when_double_tap_lock_layer);
}

/* @return error */
int load_config_line(char * line, int linenum) {
    if (line == NULL) {
        if (parsee_is_valid()) {
            if (register_remap(g_remap_parsee)) {
                g_remap_parsee = NULL;
                printf("Config error (line %d): Exceeded the maximum limit of 255 remappings.\n", linenum);
                return 1;
            }
            g_remap_parsee = NULL;
        }
        while (g_remap_list) {
            struct RemapNode * remap_node = new_remap_node(g_remap_list);
            int index = g_remap_list->from->virt_code & 0xFF;

            if (g_remap_list->layer || (g_remap_array[index] && !g_remap_array[index]->remap->layer)) {
                remap_node->next = g_remap_array[index];
                g_remap_array[index] = remap_node;
            } else {
                if (g_remap_array[index]) {
                    struct RemapNode * tail = g_remap_array[index];
                    while (tail->next && tail->next->remap->layer) tail = tail->next;
                    remap_node->next = tail->next;
                    tail->next = remap_node;
                } else {
                    g_remap_array[index] = remap_node;
                }
            }
            g_remap_list = g_remap_list->next;
        }
        return 0;
    }

    trim_newline(line);

    // Ignore comments and empty lines
    if (line[0] == '#' || line[0] == '\0') {
        return 0;
    }

    // Handle config declaration
    if (sscanf(line, "debug=%d", &g_debug)) {
        if (g_debug == 1 || g_debug == 0)
            return 0;
    }

    if (sscanf(line, "hold_delay=%d", &g_hold_delay)) {
        return 0;
    }

    if (sscanf(line, "tap_timeout=%d", &g_tap_timeout)) {
        return 0;
    }

    if (sscanf(line, "doublepress_timeout=%d", &g_doublepress_timeout)) {
        return 0;
    }

    if (sscanf(line, "rehook_timeout=%d", &g_rehook_timeout)) {
        return 0;
    }

    if (sscanf(line, "unlock_timeout=%d", &g_unlock_timeout)) {
        return 0;
    }

    if (sscanf(line, "scancode=%d", &g_scancode)) {
        if (g_scancode == 1 || g_scancode == 0)
            return 0;
    }

    if (sscanf(line, "priority=%d", &g_priority)) {
        if (g_priority == 1 || g_priority == 0)
            return 0;
    }

    // Handle key remappings
    char * after_eq = (char *)strchr(line, '=');
    if (!after_eq) {
        printf("Config error (line %d): Couldn't understand '%s'.\n", linenum, line);
        return 1;
    }
    char * key_name = after_eq + 1;
    KEY_DEF * key_def = find_key_def_by_name(key_name);
    if (!key_def && strncmp(key_name, "layer", strlen("layer")) &&
        strncmp(key_name, "toggle_layer", strlen("toggle_layer")) &&
        strncmp(key_name, "set_layer", strlen("set_layer")) &&
        strncmp(key_name, "reset_layer", strlen("reset_layer"))) {
        printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
        printf("Key names were changed in the most recent version. Please review review the wiki for the new names!\n");
        return 1;
    }

    if (g_remap_parsee == NULL) {
        g_remap_parsee = new_remap(NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    if (strncmp(line, "remap_key=", strlen("remap_key=")) == 0) {
        if (g_remap_parsee->from && !parsee_is_valid()) {
            printf("Config error (line %d): Incomplete remapping.\n"
                   "Each remapping must have a 'remap_key', 'when_alone', and 'with_other'.\n",
                   linenum);
            return 1;
        }
        if (g_remap_parsee->from && parsee_is_valid()) {
            if (register_remap(g_remap_parsee)) {
                g_remap_parsee = NULL;
                printf("Config error (line %d): Exceeded the maximum limit of 255 remappings.\n", linenum);
                return 1;
            }
            g_remap_parsee = new_remap(NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        }
        g_remap_parsee->from = key_def;
    } else if (strncmp(line, "layer=", strlen("layer=")) == 0) {
        if (strncmp(key_name, "layer", strlen("layer")) == 0) {
            g_remap_parsee->layer = find_layer(g_layer_list, key_name);
            if (g_remap_parsee->layer == NULL) {
                g_remap_parsee->layer = append_layer(&g_layer_list, new_layer(key_name));
            }
        } else {
            printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
            return 1;
        }
    } else if (strncmp(line, "when_alone=", strlen("when_alone=")) == 0) {
        if (!key_def) {
            printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
            return 1;
        }
        if (g_remap_parsee->to_when_alone == NULL) {
            g_remap_parsee->to_when_alone = new_key_node(key_def);
        } else {
            append_key_node(g_remap_parsee->to_when_alone, key_def);
        }
    } else if (strncmp(line, "with_other=", strlen("with_other=")) == 0) {
        if (!key_def) {
            printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
            return 1;
        }
        if (g_remap_parsee->to_with_other == NULL) {
            g_remap_parsee->to_with_other = new_key_node(key_def);
        } else {
            append_key_node(g_remap_parsee->to_with_other, key_def);
        }
    } else if (strncmp(line, "when_press=", strlen("when_press=")) == 0) {
        if (strncmp(key_name, "layer", strlen("layer")) == 0) {
            g_remap_parsee->to_when_press_layer = find_layer(g_layer_list, key_name);
            if (g_remap_parsee->to_when_press_layer == NULL) {
                g_remap_parsee->to_when_press_layer = append_layer(&g_layer_list, new_layer(key_name));
            }
        } else {
            printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
            return 1;
        }
    } else if (strncmp(line, "when_doublepress=", strlen("when_doublepress=")) == 0) {
        if (strncmp(key_name, "layer", strlen("layer")) == 0) {
            g_remap_parsee->to_when_doublepress_layer = find_layer(g_layer_list, key_name);
            if (g_remap_parsee->to_when_doublepress_layer == NULL) {
                g_remap_parsee->to_when_doublepress_layer = append_layer(&g_layer_list, new_layer(key_name));
            }
        } else {
            if (!key_def) {
                printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
                return 1;
            }
            if (g_remap_parsee->to_when_doublepress == NULL) {
                g_remap_parsee->to_when_doublepress = new_key_node(key_def);
            } else {
                append_key_node(g_remap_parsee->to_when_doublepress, key_def);
            }
        }
    } else if (strncmp(line, "when_tap_lock=", strlen("when_tap_lock=")) == 0) {
        if (strncmp(key_name, "toggle_layer", strlen("toggle_layer")) == 0) {
            struct Layer * layer = find_layer(g_layer_list, key_name + strlen("toggle_"));
            if (layer == NULL) {
                layer = append_layer(&g_layer_list, new_layer(key_name + strlen("toggle_")));
            }
            append_layer_conf(&g_remap_parsee->to_when_tap_lock_layer, new_layer_conf(layer, toggle_layer_lock));
        } else if (strncmp(key_name, "set_layer", strlen("set_layer")) == 0) {
            struct Layer * layer = find_layer(g_layer_list, key_name + strlen("set_"));
            if (layer == NULL) {
                layer = append_layer(&g_layer_list, new_layer(key_name + strlen("set_")));
            }
            append_layer_conf(&g_remap_parsee->to_when_tap_lock_layer, new_layer_conf(layer, set_layer_lock));
        } else if (strncmp(key_name, "reset_layer", strlen("reset_layer")) == 0) {
            struct Layer * layer = find_layer(g_layer_list, key_name + strlen("reset_"));
            if (layer == NULL) {
                layer = append_layer(&g_layer_list, new_layer(key_name + strlen("reset_")));
            }
            append_layer_conf(&g_remap_parsee->to_when_tap_lock_layer, new_layer_conf(layer, reset_layer_lock));
        } else {
            if (!key_def) {
                printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
                return 1;
            }
            if (g_remap_parsee->to_when_tap_lock == NULL) {
                g_remap_parsee->to_when_tap_lock = new_key_node(key_def);
            } else {
                append_key_node(g_remap_parsee->to_when_tap_lock, key_def);
            }
        }
    } else if (strncmp(line, "when_double_tap_lock=", strlen("when_double_tap_lock=")) == 0) {
        if (strncmp(key_name, "toggle_layer", strlen("toggle_layer")) == 0) {
            struct Layer * layer = find_layer(g_layer_list, key_name + strlen("toggle_"));
            if (layer == NULL) {
                layer = append_layer(&g_layer_list, new_layer(key_name + strlen("toggle_")));
            }
            append_layer_conf(&g_remap_parsee->to_when_double_tap_lock_layer, new_layer_conf(layer, toggle_layer_lock));
        } else if (strncmp(key_name, "set_layer", strlen("set_layer")) == 0) {
            struct Layer * layer = find_layer(g_layer_list, key_name + strlen("set_"));
            if (layer == NULL) {
                layer = append_layer(&g_layer_list, new_layer(key_name + strlen("set_")));
            }
            append_layer_conf(&g_remap_parsee->to_when_double_tap_lock_layer, new_layer_conf(layer, set_layer_lock));
        } else if (strncmp(key_name, "reset_layer", strlen("reset_layer")) == 0) {
            struct Layer * layer = find_layer(g_layer_list, key_name + strlen("reset_"));
            if (layer == NULL) {
                layer = append_layer(&g_layer_list, new_layer(key_name + strlen("reset_")));
            }
            append_layer_conf(&g_remap_parsee->to_when_double_tap_lock_layer, new_layer_conf(layer, reset_layer_lock));
        } else {
            if (!key_def) {
                printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
                return 1;
            }
            if (g_remap_parsee->to_when_double_tap_lock == NULL) {
                g_remap_parsee->to_when_double_tap_lock = new_key_node(key_def);
            } else {
                append_key_node(g_remap_parsee->to_when_double_tap_lock, key_def);
            }
        }
    } else if (strncmp(line, "define_layer=", strlen("define_layer=")) == 0) {
        if (strncmp(key_name, "layer", strlen("layer")) == 0) {
            g_layer_parsee = find_layer(g_layer_list, key_name);
            if (g_layer_parsee == NULL) {
                g_layer_parsee = append_layer(&g_layer_list, new_layer(key_name));
            }
        } else {
            printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
            return 1;
        }
    } else if (strncmp(line, "and_layer=", strlen("and_layer=")) == 0) {
        if (strncmp(key_name, "layer", strlen("layer")) == 0) {
            if (g_layer_parsee == NULL) {
                printf("Config error (line %d): Incomplete layer definition.\n"
                       "Each layer definition must start with a 'define_layer'.\n",
                       linenum);
                return 1;
            } else {
                struct Layer * master_layer = find_layer(g_layer_list, key_name);
                if (master_layer == NULL) {
                    master_layer = append_layer(&g_layer_list, new_layer(key_name));
                }
                append_layer_node(&g_layer_parsee->and_master_layers, new_layer_node(master_layer));
                append_layer_node(&master_layer->and_slave_layers, new_layer_node(g_layer_parsee));
            }
        } else {
            printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
            return 1;
        }
    } else if (strncmp(line, "and_not_layer=", strlen("and_not_layer=")) == 0) {
        if (strncmp(key_name, "layer", strlen("layer")) == 0) {
            if (g_layer_parsee == NULL) {
                printf("Config error (line %d): Incomplete layer definition.\n"
                       "Each layer definition must start with a 'define_layer'.\n",
                       linenum);
                return 1;
            } else {
                struct Layer * master_layer = find_layer(g_layer_list, key_name);
                if (master_layer == NULL) {
                    master_layer = append_layer(&g_layer_list, new_layer(key_name));
                }
                append_layer_node(&g_layer_parsee->and_not_master_layers, new_layer_node(master_layer));
                append_layer_node(&master_layer->and_slave_layers, new_layer_node(g_layer_parsee));
            }
        } else {
            printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
            return 1;
        }
    } else {
        after_eq[0] = 0;
        printf("Config error (line %d): Invalid setting '%s'.\n", linenum, line);
        return 1;
    }

    return 0;
}
