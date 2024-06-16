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

struct KeyDefNode
{
    KEY_DEF * key_def;

    struct KeyDefNode * next;
    struct KeyDefNode * previous;
};

struct Layer {
    char * name;
    int state;
    int lock;

    struct Layer * next;
};

struct Remap
{
    int id;
    KEY_DEF * from;
    struct Layer * layer;
    struct Layer * to_when_alone_layer;
    struct Layer * to_with_other_layer;
    struct Layer * to_when_doublepress_layer;
    struct Layer * to_when_tap_lock_layer;
    struct Layer * to_when_double_tap_lock_layer;
    struct KeyDefNode * to_when_alone;
    struct KeyDefNode * to_with_other;
    struct KeyDefNode * to_when_doublepress;
    struct KeyDefNode * to_when_tap_lock;
    struct KeyDefNode * to_when_double_tap_lock;
    int to_when_alone_is_modifier;
    int to_when_doublepress_is_modifier;
    int tap_lock;
    int double_tap_lock;

    enum State state;
    DWORD time;

    struct Remap * next;
};

struct RemapNode
{
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
struct RemapNode * g_remap_array[256] = {NULL};
struct Layer * g_layer_list = NULL;

// Debug Logging
// --------------------------------------

char * fmt_dir(enum Direction direction)
{
    return (direction == DOWN) ? "DOWN" : "UP";
}

int log_indent_level = 0;
int log_counter = 1;
void print_log_prefix()
{
    printf("\n%03d. ", log_counter++);
    for (int i = 0; i < log_indent_level; i++)
    {
        printf("\t");
    }
}

void log_handle_input_start(int scan_code, int virt_code, enum Direction direction, int is_injected, DWORD flags, ULONG_PTR dwExtraInfo)
{
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

void log_handle_input_end(int scan_code, int virt_code, enum Direction direction, int block_input)
{
    if (!g_debug) return;
    log_indent_level--;
    if (block_input) {
        print_log_prefix();
        printf("#blocked-input# %s %s",
            friendly_virt_code_name(virt_code),
            fmt_dir(direction));
    }
}

void log_send_input(char * remap_name, KEY_DEF * key, enum Direction direction)
{
    if (!g_debug) return;
    print_log_prefix();
    printf("(sending:%s) %s %s",
        remap_name,
        key ? key->name : "???",
        fmt_dir(direction));
}

// Remapping
// -------------------------------------

struct KeyDefNode * new_key_node(KEY_DEF * key_def)
{
    struct KeyDefNode * key_node = malloc(sizeof(struct KeyDefNode));
    key_node->key_def = key_def;
    key_node->next = key_node;
    key_node->previous = key_node;
    return key_node;
}

struct Layer * new_layer(char * name)
{
    struct Layer * layer = malloc(sizeof(struct Layer));
    layer->name = strdup(name);
    layer->state = 0;
    layer->lock = 0;
    layer->next = NULL;
    return layer;
}

struct Remap * new_remap(KEY_DEF * from,
                         struct Layer * layer,
                         struct KeyDefNode * to_when_alone,
                         struct KeyDefNode * to_with_other,
                         struct KeyDefNode * to_when_doublepress,
                         struct KeyDefNode * to_when_tap_lock,
                         struct KeyDefNode * to_when_double_tap_lock)
{
    struct Remap * remap = malloc(sizeof(struct Remap));
    remap->id = 0;
    remap->from = from;
    remap->layer = layer;
    remap->to_when_alone_layer = NULL;
    remap->to_with_other_layer = NULL;
    remap->to_when_doublepress_layer = NULL;
    remap->to_when_tap_lock_layer = NULL;
    remap->to_when_double_tap_lock_layer = NULL;
    remap->to_when_alone = to_when_alone;
    remap->to_with_other = to_with_other;
    remap->to_when_doublepress = to_when_doublepress;
    remap->to_when_tap_lock = to_when_tap_lock;
    remap->to_when_double_tap_lock = to_when_double_tap_lock;
    remap->to_when_alone_is_modifier = 0;
    remap->to_when_doublepress_is_modifier = 0;
    remap->tap_lock = 0;
    remap->double_tap_lock = 0;
    remap->state = IDLE;
    remap->time = 0;
    remap->next = NULL;
    return remap;
}

struct RemapNode * new_remap_node(struct Remap * remap)
{
    struct RemapNode * remap_node = malloc(sizeof(struct RemapNode));
    remap_node->remap = remap;
    remap_node->next = NULL;
    return remap_node;
}

void append_key_node(struct KeyDefNode * head, KEY_DEF * key_def)
{
    head->previous->next = new_key_node(key_def);
    head->previous->next->previous = head->previous;
    head->previous = head->previous->next;
    head->previous->next = head;
}

int key_eq(struct KeyDefNode * head_a, struct KeyDefNode * head_b)
{
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

int is_modifier(struct KeyDefNode * head)
{
    struct KeyDefNode * cur = head;
    int modifier = 1;
    do {
        modifier *= cur->key_def->is_modifier;
        cur = cur->next;
    } while(cur != head);
    return modifier;
}

void free_key_nodes(struct KeyDefNode * head)
{
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

void free_layers(struct Layer * head)
{
    struct Layer * cur = head;
    while (cur) {
        struct Layer * layer = cur;
        cur = cur->next;
        free(layer->name);
        free(layer);
    }
}

void free_remap_nodes(struct RemapNode * head)
{
    struct RemapNode * cur = head;
    while (cur) {
        struct RemapNode * remap_node = cur;
        cur = cur->next;
        free(remap_node);
    }
}

struct Layer * find_layer(char * name)
{
    struct Layer * layer_iter = g_layer_list;
    while (layer_iter) {
        if (strcmp(layer_iter->name, name) == 0) {
            return layer_iter;
        }
        layer_iter = layer_iter->next;
    }
    return NULL;
}

struct Layer * append_layer(char * name)
{
    if (g_layer_list) {
        struct Layer * tail = g_layer_list;
        while (tail->next) tail = tail->next;
        tail->next = new_layer(name);
        return tail->next;
    }
    g_layer_list = new_layer(name);
    return g_layer_list;
}

int register_remap(struct Remap * remap)
{
    int id = 0;
    if (g_remap_list) {
        struct Remap * tail = g_remap_list;
        while (tail->next) tail = tail->next;
        tail->next = remap;
        id = tail->id;
        if (id == 255) return 1;
    } else {
        g_remap_list = remap;
    }
    remap->id = id + 1;
    if (key_eq(remap->to_when_alone, remap->to_with_other)) {
        free_key_nodes(remap->to_with_other);
        remap->to_with_other = NULL;
    }
    if (key_eq(remap->to_when_alone, remap->to_when_doublepress)) {
        free_key_nodes(remap->to_when_doublepress);
        remap->to_when_doublepress = NULL;
    }
    if (remap->to_when_alone) {
        remap->to_when_alone_is_modifier = is_modifier(remap->to_when_alone);
    }
    if (remap->to_with_other && !is_modifier(remap->to_with_other)) {
        free_key_nodes(remap->to_with_other);
        remap->to_with_other = NULL;
    }
    if (remap->to_when_doublepress) {
        remap->to_when_doublepress_is_modifier = is_modifier(remap->to_when_doublepress);
    }
    return 0;
}

struct Remap * find_remap_for_virt_code(int virt_code)
{
    struct Remap * remap = g_remap_list;
    while(remap) {
        if (remap->from->virt_code == virt_code) {
            return remap;
        }
        remap = remap->next;
    }
    return NULL;
}

int remap_list_depth()
{
    int depth = 0;
    struct Remap * remap_iter = g_remap_list;
    while (remap_iter) {
        depth += 1;
        remap_iter = remap_iter->next;
    }
    return depth;
}

struct Remap * find_remap(struct Remap * remap)
{
    struct Remap * remap_iter = g_remap_list;
    while (remap_iter) {
        if (remap_iter == remap) {
            return remap_iter;
        }
        remap_iter = remap_iter->next;
    }
    return NULL;
}

void prepend_active_remap(struct Remap * remap)
{
    if (find_remap(remap) == NULL) {
        remap->next = g_remap_list;
        g_remap_list = remap;
    }
    DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
}

void append_active_remap(struct Remap * remap)
{
    if (g_remap_list) {
        struct Remap * remap_iter = g_remap_list;
        do {
            if (remap_iter == remap) {
                DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
                return;
            }
            if (remap_iter->next == NULL) break;
            remap_iter = remap_iter->next;
        } while (1);
        remap_iter->next = remap;
        remap->next = NULL;
    } else {
        g_remap_list = remap;
        remap->next = NULL;
    }
    DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
}

void remove_active_remap(struct Remap * remap)
{
    if (g_remap_list == NULL) {
        DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
        return;
    }
    if (remap == g_remap_list) {
        g_remap_list = remap->next;
        remap->next = NULL;
        DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
        return;
    }
    struct Remap * remap_iter = g_remap_list;
    while (remap_iter->next && remap_iter->next != remap) {
        remap_iter = remap_iter->next;
    }
    if (remap_iter->next) {
        remap_iter->next = remap->next;
        remap->next = NULL;
    }
    DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
}

void send_key_def_input_down(char * input_name, struct KeyDefNode * head, int remap_id, struct InputBuffer * input_buffer)
{
    struct KeyDefNode * cur = head;
    do {
        log_send_input(input_name, cur->key_def, DOWN);
        send_input(cur->key_def->scan_code, cur->key_def->virt_code, DOWN, remap_id, input_buffer);
        cur = cur->next;
    } while(cur != head);
}

void send_key_def_input_up(char * input_name, struct KeyDefNode * head, int remap_id, struct InputBuffer * input_buffer)
{
    struct KeyDefNode * cur = head;
    do {
        cur = cur->previous;
        log_send_input(input_name, cur->key_def, UP);
        send_input(cur->key_def->scan_code, cur->key_def->virt_code, UP, remap_id, input_buffer);
    } while(cur != head);
}

void unlock_all(struct InputBuffer * input_buffer)
{
    struct Layer * layer_iter = g_layer_list;
    while (layer_iter) {
        layer_iter->state = 0;
        layer_iter->lock = 0;
        layer_iter = layer_iter->next;
    }
    struct Remap * remap_iter = g_remap_list;
    while(remap_iter) {
        g_remap_list = remap_iter->next;
        if (remap_iter->state == HELD_DOWN_ALONE) {
        } else if (remap_iter->state == HELD_DOWN_WITH_OTHER) {
            if (remap_iter->to_with_other) {
                send_key_def_input_up("unlock_with_other", remap_iter->to_with_other, remap_iter->id, input_buffer);
            }
        } else if (remap_iter->state == TAP) {
            if (remap_iter->to_when_alone) {
                send_key_def_input_up("unlock_when_alone", remap_iter->to_when_alone, remap_iter->id, input_buffer);
            }
        } else if (remap_iter->state == DOUBLE_TAP) {
            if (remap_iter->to_when_doublepress) {
                send_key_def_input_up("unlock_when_doublepress", remap_iter->to_when_doublepress, remap_iter->id, input_buffer);
            }
        }
        if (remap_iter->double_tap_lock) {
            send_key_def_input_up("unlock_when_double_tap_lock", remap_iter->to_when_double_tap_lock, remap_iter->id, input_buffer);
            remap_iter->double_tap_lock = 0;
        }
        if (remap_iter->tap_lock) {
            send_key_def_input_up("unlock_when_tap_lock", remap_iter->to_when_tap_lock, remap_iter->id, input_buffer);
            remap_iter->tap_lock = 0;
        }
        remap_iter->state = IDLE;
        remap_iter->next = NULL;
        remap_iter = g_remap_list;
        DEBUG(1, debug_print(RED, "\nRemap list depth = %d", remap_list_depth()));
    }
}

/* @return block_input */
int event_remapped_key_down(struct Remap * remap, DWORD time, struct InputBuffer * input_buffer)
{
    if (remap->state == IDLE) {
        if (remap->to_with_other || remap->to_with_other_layer) {
            remap->time = time;
            remap->state = HELD_DOWN_ALONE;
            if (remap->to_with_other_layer) {
                remap->to_with_other_layer->state = 1;
            }
        } else {
            remap->time = time;
            remap->state = TAP;
            if (remap->to_when_alone) {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, input_buffer);
            }
            if (remap->to_when_alone_layer) {
                remap->to_when_alone_layer->state = 1;
            }
        }
        append_active_remap(remap);
    } else if (remap->state == HELD_DOWN_WITH_OTHER) {
        if (remap->to_with_other) {
            send_key_def_input_down("with_other", remap->to_with_other, remap->id, input_buffer);
        }
    } else if (remap->state == TAP) {
        if (remap->to_when_alone) {
            send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, input_buffer);
        }
    } else if (remap->state == TAPPED) {
        if ((g_doublepress_timeout > 0) && (time - remap->time < g_doublepress_timeout)) {
            remap->time = time;
            remap->state = DOUBLE_TAP;
            if (remap->to_when_tap_lock) {
                remap->tap_lock = 1 - remap->tap_lock;
                if (remap->tap_lock == 0) {
                    send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, input_buffer);
                }
            }
            if (remap->to_when_tap_lock_layer) {
                remap->to_when_tap_lock_layer->lock = 1 - remap->to_when_tap_lock_layer->lock;
                remap->to_when_tap_lock_layer->state = remap->to_when_tap_lock_layer->lock;
            }
            if (remap->to_when_doublepress_layer) {
                remap->to_when_doublepress_layer->state = 1;
            }
            if (remap->to_when_doublepress) {
                send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, input_buffer);
            } else if (remap->to_when_doublepress_layer) {
            } else if (remap->to_when_alone) {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, input_buffer);
            }
        } else {
            if (remap->to_with_other || remap->to_with_other_layer) {
                remap->time = time;
                remap->state = HELD_DOWN_ALONE;
                if (remap->to_with_other_layer) {
                    remap->to_with_other_layer->state = 1;
                }
            } else {
                remap->time = time;
                remap->state = TAP;
                if (remap->to_when_alone) {
                    send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, input_buffer);
                }
                if (remap->to_when_alone_layer) {
                    remap->to_when_alone_layer->state = 1;
                }
            }
        }
        append_active_remap(remap);
    } else if (remap->state == DOUBLE_TAP) {
        if (remap->to_when_doublepress) {
            send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, input_buffer);
        } else if (remap->to_when_doublepress_layer) {
        } else if (remap->to_when_alone) {
            send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, input_buffer);
        }
    }
    return 1;
}

/* @return block_input */
int event_remapped_key_up(struct Remap * remap, DWORD time, struct InputBuffer * input_buffer)
{
    if (remap->state == HELD_DOWN_ALONE) {
        if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
            remap->time = time;
            remap->state = TAPPED;
            if (remap->to_when_alone) {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, input_buffer);
                send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, input_buffer);
            }
            if (remap->to_when_tap_lock) {
                remap->tap_lock = 1 - remap->tap_lock;
                if (remap->tap_lock) {
                    send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, input_buffer);
                } else {
                    send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, input_buffer);
                }
            }
            if (remap->to_when_tap_lock_layer) {
                remap->to_when_tap_lock_layer->lock = 1 - remap->to_when_tap_lock_layer->lock;
                remap->to_when_tap_lock_layer->state = remap->to_when_tap_lock_layer->lock;
            }
        } else {
            remap->state = IDLE;
        }
        if (remap->to_with_other_layer) {
            remap->to_with_other_layer->state = remap->to_with_other_layer->lock;
        }
    } else if (remap->state == HELD_DOWN_WITH_OTHER) {
        remap->state = IDLE;
        if (remap->to_with_other) {
            send_key_def_input_up("with_other", remap->to_with_other, remap->id, input_buffer);
        }
        if (remap->to_with_other_layer) {
            remap->to_with_other_layer->state = remap->to_with_other_layer->lock;
        }
    } else if (remap->state == TAP) {
        if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
            remap->time = time;
            remap->state = TAPPED;
            if (remap->to_when_alone) {
                send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, input_buffer);
            }
            if (remap->to_when_tap_lock) {
                remap->tap_lock = 1 - remap->tap_lock;
                if (remap->tap_lock) {
                    send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, input_buffer);
                } else {
                    send_key_def_input_up("when_tap_lock", remap->to_when_tap_lock, remap->id, input_buffer);
                }
            }
            if (remap->to_when_tap_lock_layer) {
                remap->to_when_tap_lock_layer->lock = 1 - remap->to_when_tap_lock_layer->lock;
                remap->to_when_tap_lock_layer->state = remap->to_when_tap_lock_layer->lock;
            }
        } else {
            remap->state = IDLE;
            if (remap->to_when_alone) {
                send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, input_buffer);
            }
        }
        if (remap->to_when_alone_layer) {
            remap->to_when_alone_layer->state = remap->to_when_alone_layer->lock;
        }
    } else if (remap->state == DOUBLE_TAP) {
        remap->state = IDLE;
        if (remap->to_when_doublepress) {
            send_key_def_input_up("when_doublepress", remap->to_when_doublepress, remap->id, input_buffer);
        } else if (remap->to_when_doublepress_layer) {
        } else if (remap->to_when_alone) {
            send_key_def_input_up("when_alone", remap->to_when_alone, remap->id, input_buffer);
        }
        if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
            if (remap->to_when_double_tap_lock) {
                remap->double_tap_lock = 1 - remap->double_tap_lock;
                if (remap->double_tap_lock) {
                    send_key_def_input_down("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, input_buffer);
                } else {
                    send_key_def_input_up("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, input_buffer);
                }
            }
            if (remap->to_when_double_tap_lock_layer) {
                remap->to_when_double_tap_lock_layer->lock = 1 - remap->to_when_double_tap_lock_layer->lock;
                remap->to_when_double_tap_lock_layer->state = remap->to_when_double_tap_lock_layer->lock;
            }
        }
        if (remap->to_when_doublepress_layer) {
            remap->to_when_doublepress_layer->state = remap->to_when_doublepress_layer->lock;
        }
    }
    if (remap->tap_lock == 0 && remap->double_tap_lock == 0) {
        remove_active_remap(remap);
    }
    return 1;
}

/* @return block_input */
int event_other_input(int virt_code, enum Direction direction, DWORD time, int remap_id, struct InputBuffer * input_buffer)
{
    int block_input = 0;
    if (direction == DOWN && !vk_is_modifier(virt_code)) {
        struct Remap * remap = g_remap_list;
        while(remap) {
            if (remap->id != remap_id) {
                if (remap->state == HELD_DOWN_ALONE) {
                    if ((g_hold_delay > 0) && (time - remap->time < g_hold_delay) && remap->to_when_alone) {
                        remap->state = TAP;
                        send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, input_buffer);
                        block_input = -1;
                    } else {
                        remap->state = HELD_DOWN_WITH_OTHER;
                        if (remap->to_with_other) {
                            send_key_def_input_down("with_other", remap->to_with_other, remap->id, input_buffer);
                            block_input = -1;
                        }
                    }
                } else if (remap->state == HELD_DOWN_WITH_OTHER) {
                    if (remap->to_with_other) {
                        send_key_def_input_down("with_other", remap->to_with_other, remap->id, input_buffer);
                        block_input = -1;
                    }
                } else if (remap->state == TAP) {
                    if (remap->to_when_alone && remap->to_when_alone_is_modifier) {
                        send_key_def_input_down("when_alone", remap->to_when_alone, remap->id, input_buffer);
                        block_input = -1;
                    }
                } else if (remap->state == DOUBLE_TAP) {
                    if (remap->to_when_doublepress && remap->to_when_doublepress_is_modifier) {
                        send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id, input_buffer);
                        block_input = -1;
                    }
                } else {
                    if (remap->double_tap_lock) {
                        send_key_def_input_down("when_double_tap_lock", remap->to_when_double_tap_lock, remap->id, input_buffer);
                        block_input = -1;
                    }
                    if (remap->tap_lock) {
                        send_key_def_input_down("when_tap_lock", remap->to_when_tap_lock, remap->id, input_buffer);
                        block_input = -1;
                    }
                }
                remap->time = 0; // disable tap and double_tap
            }
            remap = remap->next;
        }
    }
    return block_input;
}


/* @return block_input */
int handle_input(int scan_code, int virt_code, enum Direction direction, DWORD time, int is_injected, DWORD flags, ULONG_PTR dwExtraInfo, struct InputBuffer * input_buffer)
{
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
            remap_for_input = find_remap_for_virt_code(virt_code);
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

void trim_newline(char * str)
{
    str[strcspn(str, "\r\n")] = 0;
}

int parsee_is_valid()
{
    return g_remap_parsee &&
        g_remap_parsee->from &&
        (g_remap_parsee->to_when_alone || g_remap_parsee->to_with_other ||
         g_remap_parsee->to_when_doublepress || g_remap_parsee->to_when_tap_lock ||
         g_remap_parsee->to_when_double_tap_lock ||
         g_remap_parsee->to_when_alone_layer || g_remap_parsee->to_with_other_layer ||
         g_remap_parsee->to_when_doublepress_layer || g_remap_parsee->to_when_tap_lock_layer ||
         g_remap_parsee->to_when_double_tap_lock_layer);
}

/* @return error */
int load_config_line(char * line, int linenum)
{
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
    if (!key_def && strncmp(key_name, "layer", 5)) {
        printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
        printf("Key names were changed in the most recent version. Please review review the wiki for the new names!\n");
        return 1;
    }

    if (g_remap_parsee == NULL) {
        g_remap_parsee = new_remap(NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    if (strstr(line, "remap_key=")) {
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
    } else if (strstr(line, "layer=")) {
        g_remap_parsee->layer = find_layer(key_name);
        if (g_remap_parsee->layer == NULL) {
            g_remap_parsee->layer = append_layer(key_name);
        }
    } else if (strstr(line, "when_alone=")) {
        if (strncmp(key_name, "layer", 5) == 0) {
            g_remap_parsee->to_when_alone_layer = find_layer(key_name);
            if (g_remap_parsee->to_when_alone_layer == NULL) {
                g_remap_parsee->to_when_alone_layer = append_layer(key_name);
            }
        } else {
            if (g_remap_parsee->to_when_alone == NULL) {
                g_remap_parsee->to_when_alone = new_key_node(key_def);
            } else {
                append_key_node(g_remap_parsee->to_when_alone, key_def);
            }
        }
    } else if (strstr(line, "with_other=")) {
        if (strncmp(key_name, "layer", 5) == 0) {
            g_remap_parsee->to_with_other_layer = find_layer(key_name);
            if (g_remap_parsee->to_with_other_layer == NULL) {
                g_remap_parsee->to_with_other_layer = append_layer(key_name);
            }
        } else {
            if (g_remap_parsee->to_with_other == NULL) {
                g_remap_parsee->to_with_other = new_key_node(key_def);
            } else {
                append_key_node(g_remap_parsee->to_with_other, key_def);
            }
        }
    } else if (strstr(line, "when_doublepress=")) {
        if (strncmp(key_name, "layer", 5) == 0) {
            g_remap_parsee->to_when_doublepress_layer = find_layer(key_name);
            if (g_remap_parsee->to_when_doublepress_layer == NULL) {
                g_remap_parsee->to_when_doublepress_layer = append_layer(key_name);
            }
        } else {
            if (g_remap_parsee->to_when_doublepress == NULL) {
                g_remap_parsee->to_when_doublepress = new_key_node(key_def);
            } else {
                append_key_node(g_remap_parsee->to_when_doublepress, key_def);
            }
        }
    } else if (strstr(line, "when_tap_lock=")) {
        if (strncmp(key_name, "layer", 5) == 0) {
            g_remap_parsee->to_when_tap_lock_layer = find_layer(key_name);
            if (g_remap_parsee->to_when_tap_lock_layer == NULL) {
                g_remap_parsee->to_when_tap_lock_layer = append_layer(key_name);
            }
        } else {
            if (g_remap_parsee->to_when_tap_lock == NULL) {
                g_remap_parsee->to_when_tap_lock = new_key_node(key_def);
            } else {
                append_key_node(g_remap_parsee->to_when_tap_lock, key_def);
            }
        }
    } else if (strstr(line, "when_double_tap_lock=")) {
        if (strncmp(key_name, "layer", 5) == 0) {
            g_remap_parsee->to_when_double_tap_lock_layer = find_layer(key_name);
            if (g_remap_parsee->to_when_double_tap_lock_layer == NULL) {
                g_remap_parsee->to_when_double_tap_lock_layer = append_layer(key_name);
            }
        } else {
            if (g_remap_parsee->to_when_double_tap_lock == NULL) {
                g_remap_parsee->to_when_double_tap_lock = new_key_node(key_def);
            } else {
                append_key_node(g_remap_parsee->to_when_double_tap_lock, key_def);
            }
        }
    } else {
        after_eq[0] = 0;
        printf("Config error (line %d): Invalid setting '%s'.\n", linenum, line);
        return 1;
    }

    return 0;
}

void reset_config()
{
    free(g_remap_parsee);
    g_remap_parsee = NULL;
    while (g_remap_list) {
        struct Remap * remap = g_remap_list;
        g_remap_list = remap->next;
        free_key_nodes(remap->to_when_alone);
        free_key_nodes(remap->to_with_other);
        free_key_nodes(remap->to_when_doublepress);
        free_key_nodes(remap->to_when_tap_lock);
        free_key_nodes(remap->to_when_double_tap_lock);
        free(remap);
    }
    g_remap_list = NULL;
    free_layers(g_layer_list);
    g_layer_list = NULL;
    for (int i = 0; i < 256; i++) {
        free_remap_nodes(g_remap_array[i]);
        g_remap_array[i] = NULL;
    }
}
