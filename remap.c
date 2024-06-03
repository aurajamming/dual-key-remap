#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "input.h"
#include "keys.c"

#define INJECTED_KEY_ID (0xFFC3CED7 & 0xFFFFFF00)

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

struct Remap
{
    int id;
    KEY_DEF * from;
    struct KeyDefNode * to_when_alone;
    struct KeyDefNode * to_with_other;
    struct KeyDefNode * to_when_doublepress;
    int to_when_alone_is_modifier;
    int to_when_doublepress_is_modifier;

    enum State state;
    DWORD time;

    struct Remap * next;
};

// Globals
// --------------------------------------

int g_debug = 0;
int g_hold_delay = 0;
int g_tap_timeout = 0;
int g_doublepress_timeout = 0;
int g_scancode = 0;
struct Remap * g_remap_list = NULL;
struct Remap * g_remap_parsee = NULL;
struct Remap * g_remap_array[256] = {NULL};

// Debug Logging
// --------------------------------------

char * fmt_dir(enum Direction dir)
{
    return dir ? "DOWN" : "UP";
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

void log_handle_input_start(int scan_code, int virt_code, int dir, DWORD flags, ULONG_PTR dwExtraInfo)
{
    if (!g_debug) return;
    print_log_prefix();
    printf("[%s] %s %s (scan:0x%04X virt:0x%02X flags:0x%02X dwExtraInfo:0x%IX)",
           ((LLKHF_INJECTED & flags) && ((dwExtraInfo & 0xFFFFFF00) == INJECTED_KEY_ID)) ? "output" : "input",
           friendly_virt_code_name(virt_code),
           fmt_dir(dir),
           scan_code, // MapVirtualKeyA(virt_code, MAPVK_VK_TO_VSC_EX)
           virt_code,
           flags,
           dwExtraInfo);
    log_indent_level++;
}

void log_handle_input_end(int scan_code, int virt_code, int dir, int block_input)
{
    if (!g_debug) return;
    log_indent_level--;
    if (block_input) {
        print_log_prefix();
        printf("#blocked-input# %s %s",
            friendly_virt_code_name(virt_code),
            fmt_dir(dir));
    }
}

void log_send_input(char * remap_name, KEY_DEF * key, int dir)
{
    if (!g_debug) return;
    print_log_prefix();
    printf("(sending:%s) %s %s",
        remap_name,
        key ? key->name : "???",
        fmt_dir(dir));
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

struct Remap * new_remap(KEY_DEF * from, struct KeyDefNode * to_when_alone, struct KeyDefNode * to_with_other, struct KeyDefNode * to_when_doublepress)
{
    struct Remap * remap = malloc(sizeof(struct Remap));
    remap->id = 0;
    remap->from = from;
    remap->to_when_alone = to_when_alone;
    remap->to_with_other = to_with_other;
    remap->to_when_doublepress = to_when_doublepress;
    remap->to_when_alone_is_modifier = 0;
    remap->to_when_doublepress_is_modifier = 0;
    remap->state = IDLE;
    remap->time = 0;
    remap->next = NULL;
    return remap;
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
        struct KeyDefNode * cur = head;
        cur->previous->next = NULL;
        while (cur) {
            struct KeyDefNode * key_node = cur;
            cur = key_node->next;
            free(key_node);
        }
    }
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

void add_active_remap(struct Remap * remap)
{
    if (find_remap(remap) == NULL) {
        remap->next = g_remap_list;
        g_remap_list = remap;
    }
    //printf("\nList depth = %d", remap_list_depth());
}

void remove_active_remap(struct Remap * remap)
{
    if (g_remap_list == NULL) {
        //printf("\nList depth = %d", remap_list_depth());
        return;
    }
    if (remap == g_remap_list) {
        g_remap_list = remap->next;
        remap->next = NULL;
        //printf("\nList depth = %d", remap_list_depth());
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
    //printf("\nList depth = %d", remap_list_depth());
}

void send_key_def_input_down(char * input_name, struct KeyDefNode * head, int remap_id)
{
    struct KeyDefNode * cur = head;
    do {
        log_send_input(input_name, cur->key_def, DOWN);
        send_input(cur->key_def->scan_code, cur->key_def->virt_code, DOWN, remap_id);
        cur = cur->next;
    } while(cur != head);
}

void send_key_def_input_up(char * input_name, struct KeyDefNode * head, int remap_id)
{
    struct KeyDefNode * cur = head;
    do {
        cur = cur->previous;
        log_send_input(input_name, cur->key_def, UP);
        send_input(cur->key_def->scan_code, cur->key_def->virt_code, UP, remap_id);
    } while(cur != head);
}

/* @return block_input */
int event_remapped_key_down(struct Remap * remap, DWORD time)
{
    if (remap->state == IDLE) {
        if (remap->to_with_other) {
            remap->time = time;
            remap->state = HELD_DOWN_ALONE;
        } else {
            remap->state = TAP;
            if (remap->to_when_alone) {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id);
            }
        }
        add_active_remap(remap);
    } else if (remap->state == HELD_DOWN_WITH_OTHER) {
        send_key_def_input_down("with_other", remap->to_with_other, remap->id);
    } else if (remap->state == TAP) {
        if (remap->to_when_alone) {
            send_key_def_input_down("when_alone", remap->to_when_alone, remap->id);
        }
    } else if (remap->state == TAPPED) {
        if ((g_doublepress_timeout > 0) && (time - remap->time < g_doublepress_timeout)) {
            remap->state = DOUBLE_TAP;
            if (remap->to_when_doublepress) {
                send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id);
            } else {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id);
            }
        } else {
            if (remap->to_with_other) {
                remap->time = time;
                remap->state = HELD_DOWN_ALONE;
            } else {
                remap->state = TAP;
                if (remap->to_when_alone) {
                    send_key_def_input_down("when_alone", remap->to_when_alone, remap->id);
                }
            }
        }
        add_active_remap(remap);
    } else if (remap->state == DOUBLE_TAP) {
        if (remap->to_when_doublepress) {
            send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id);
        } else {
            send_key_def_input_down("when_alone", remap->to_when_alone, remap->id);
        }
    }
    return 1;
}

/* @return block_input */
int event_remapped_key_up(struct Remap * remap, DWORD time)
{
    if (remap->state == HELD_DOWN_ALONE) {
        if (((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) &&
            (remap->to_when_alone || remap->to_when_doublepress)) {
            remap->time = time;
            remap->state = TAPPED;
            if (remap->to_when_alone) {
                send_key_def_input_down("when_alone", remap->to_when_alone, remap->id);
                send_key_def_input_up("when_alone", remap->to_when_alone, remap->id);
            }
        } else {
            remap->state = IDLE;
        }
    } else if (remap->state == HELD_DOWN_WITH_OTHER) {
        remap->state = IDLE;
        send_key_def_input_up("with_other", remap->to_with_other, remap->id);
    } else if (remap->state == TAP) {
        if (remap->to_when_doublepress) {
            remap->time = time;
            remap->state = TAPPED;
            if (remap->to_when_alone) {
                send_key_def_input_up("when_alone", remap->to_when_alone, remap->id);
            }
        } else {
            remap->state = IDLE;
            send_key_def_input_up("when_alone", remap->to_when_alone, remap->id);
        }
    } else if (remap->state == DOUBLE_TAP) {
        remap->state = IDLE;
        if (remap->to_when_doublepress) {
            send_key_def_input_up("when_doublepress", remap->to_when_doublepress, remap->id);
        } else {
            send_key_def_input_up("when_alone", remap->to_when_alone, remap->id);
        }
    }
    remove_active_remap(remap);
    return 1;
}

/* @return block_input */
int event_other_input(int virt_code, int key_up, DWORD time, int remap_id)
{
    struct Remap * remap = g_remap_list;
    while(remap) {
        if ((remap->id != remap_id) && !key_up) {
            if (remap->state == HELD_DOWN_ALONE) {
                if ((g_hold_delay > 0) && (time - remap->time < g_hold_delay) && remap->to_when_alone) {
                    remap->state = TAP;
                    send_key_def_input_down("when_alone", remap->to_when_alone, remap->id);
                } else {
                    remap->state = HELD_DOWN_WITH_OTHER;
                    send_key_def_input_down("with_other", remap->to_with_other, remap->id);
                }
            } else if (remap->state == HELD_DOWN_WITH_OTHER) {
                send_key_def_input_down("with_other", remap->to_with_other, remap->id);
            } else if (remap->state == TAP) {
                if (remap->to_when_alone && remap->to_when_alone_is_modifier) {
                    send_key_def_input_down("when_alone", remap->to_when_alone, remap->id);
                }
            } else if (remap->state == DOUBLE_TAP) {
                if (remap->to_when_doublepress && remap->to_when_doublepress_is_modifier) {
                    send_key_def_input_down("when_doublepress", remap->to_when_doublepress, remap->id);
                }
            }
        }
        remap = remap->next;
    }
    return 0;
}


/* @return block_input */
int handle_input(int scan_code, int virt_code, int direction, DWORD time, DWORD flags, ULONG_PTR dwExtraInfo)
{
    struct Remap * remap_for_input;
    int block_input;
    int remap_id = 0; // if 0 then no remapped injected key

    log_handle_input_start(scan_code, virt_code, direction, flags, dwExtraInfo);
    if ((LLKHF_INJECTED & flags) && ((dwExtraInfo & 0xFFFFFF00) != INJECTED_KEY_ID)) {
        // Note: passthrough of injected keys from other tools. Dual-key-remap works at a lower level.
        block_input = 0;
    } else {
        if (LLKHF_INJECTED & flags) {
            // Note: injected keys are never remapped to avoid complex nested scenarios
            remap_for_input = NULL;
            remap_id = dwExtraInfo & 0x000000FF;
        } else {
            remap_for_input = g_remap_array[virt_code & 0xFF];
        }
        if (remap_for_input) {
            if (LLKHF_UP & flags) {
                block_input = event_remapped_key_up(remap_for_input, time);
            } else {
                block_input = event_remapped_key_down(remap_for_input, time);
            }
        } else {
            block_input = event_other_input(virt_code, LLKHF_UP & flags, time, remap_id);
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
        (g_remap_parsee->to_when_alone || g_remap_parsee->to_with_other || g_remap_parsee->to_when_doublepress);
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
            g_remap_array[g_remap_list->from->virt_code & 0xFF] = g_remap_list;
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

    if (sscanf(line, "scancode=%d", &g_scancode)) {
        if (g_scancode == 1 || g_scancode == 0)
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
    if (!key_def) {
        printf("Config error (line %d): Invalid key name '%s'.\n", linenum, key_name);
        printf("Key names were changed in the most recent version. Please review review the wiki for the new names!\n");
        return 1;
    }

    if (g_remap_parsee == NULL) {
        g_remap_parsee = new_remap(NULL, NULL, NULL, NULL);
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
            g_remap_parsee = new_remap(NULL, NULL, NULL, NULL);
        }
        g_remap_parsee->from = key_def;
    } else if (strstr(line, "when_alone=")) {
        if (g_remap_parsee->to_when_alone == NULL) {
            g_remap_parsee->to_when_alone = new_key_node(key_def);
        } else {
            append_key_node(g_remap_parsee->to_when_alone, key_def);
        }
    } else if (strstr(line, "with_other=")) {
        if (g_remap_parsee->to_with_other == NULL) {
            g_remap_parsee->to_with_other = new_key_node(key_def);
        } else {
            append_key_node(g_remap_parsee->to_with_other, key_def);
        }
    } else if (strstr(line, "when_doublepress=")) {
        if (g_remap_parsee->to_when_doublepress == NULL) {
            g_remap_parsee->to_when_doublepress = new_key_node(key_def);
        } else {
            append_key_node(g_remap_parsee->to_when_doublepress, key_def);
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
        free(remap);
    }
    g_remap_list = NULL;
}
