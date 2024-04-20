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
    TAPPED,
    TAP,
};

struct Remap
{
    int id;
    KEY_DEF * from;
    KEY_DEF * to_when_alone;
    KEY_DEF * to_with_other;

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
struct Remap * g_remap_list;
struct Remap * g_remap_parsee = NULL;

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
    printf("[%s] %s %s (scan:0x%02x virt:0x%02x flags:0x%01x dwExtraInfo:0x%Ix)",
           ((LLKHF_INJECTED & flags) && ((dwExtraInfo & 0xFFFFFF00) == INJECTED_KEY_ID)) ? "output" : "input",
        friendly_virt_code_name(virt_code),
        fmt_dir(dir),
        scan_code,
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

struct Remap * new_remap(KEY_DEF * from, KEY_DEF * to_when_alone, KEY_DEF * to_with_other)
{
    struct Remap * remap = malloc(sizeof(struct Remap));
    remap->id = 0;
    remap->from = from;
    remap->to_when_alone = to_when_alone;
    remap->to_with_other = to_with_other;
    remap->state = IDLE;
    remap->time = 0;
    remap->next = NULL;
    return remap;
}

void register_remap(struct Remap * remap)
{
    int id = 0;
    if (g_remap_list) {
        struct Remap * tail = g_remap_list;
        while (tail->next) tail = tail->next;
        tail->next = remap;
        id = tail->id;
    } else {
        g_remap_list = remap;
    }
    remap->id = id + 1;
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

void send_key_def_input(char * input_name, KEY_DEF * key_def, enum Direction dir, int remap_id)
{
    log_send_input(input_name, key_def, dir);
    send_input(key_def->scan_code, key_def->virt_code, dir, remap_id);
}

/* @return block_input */
int event_remapped_key_down(struct Remap * remap, DWORD time)
{
    if (remap->state == IDLE) {
        remap->time = time;
        remap->state = HELD_DOWN_ALONE;
    } else if (remap->state == HELD_DOWN_WITH_OTHER) {
        send_key_def_input("with_other", remap->to_with_other, DOWN, remap->id);
    } else if (remap->state == TAPPED) {
        if ((g_doublepress_timeout > 0) && (time - remap->time < g_doublepress_timeout)) {
            remap->state = TAP;
            send_key_def_input("when_alone", remap->to_when_alone, DOWN, remap->id);
        } else {
            remap->time = time;
            remap->state = HELD_DOWN_ALONE;
        }
    } else if (remap->state == TAP) {
        send_key_def_input("when_alone", remap->to_when_alone, DOWN, remap->id);
    }
    return 1;
}

/* @return block_input */
int event_remapped_key_up(struct Remap * remap, DWORD time)
{
    if (remap->state == HELD_DOWN_ALONE) {
        if ((g_tap_timeout == 0) || (time - remap->time < g_tap_timeout)) {
            remap->time = time;
            remap->state = TAPPED;
            send_key_def_input("when_alone", remap->to_when_alone, DOWN, remap->id);
            send_key_def_input("when_alone", remap->to_when_alone, UP, remap->id);
        } else {
            remap->state = IDLE;
        }
    } else if (remap->state == HELD_DOWN_WITH_OTHER) {
        remap->state = IDLE;
        send_key_def_input("with_other", remap->to_with_other, UP, remap->id);
    } else if (remap->state == TAP) {
        remap->time = time;
        remap->state = TAPPED;
        send_key_def_input("when_alone", remap->to_when_alone, UP, remap->id);
    }
    return 1;
}

/* @return block_input */
int event_other_input(int virt_code, boolean key_up, DWORD time, int remap_id)
{
    struct Remap * remap = g_remap_list;
    while(remap) {
        if (remap->id != remap_id) {
            if (remap->state == HELD_DOWN_ALONE) {
                if (!key_up && (g_hold_delay > 0) && (time - remap->time < g_hold_delay)) {
                    remap->state = TAP;
                    send_key_def_input("when_alone", remap->to_when_alone, DOWN, remap->id);
                } else {
                    remap->state = HELD_DOWN_WITH_OTHER;
                    send_key_def_input("with_other", remap->to_with_other, DOWN, remap->id);
                }
            } else if (remap->state == HELD_DOWN_WITH_OTHER) {
                send_key_def_input("with_other", remap->to_with_other, DOWN, remap->id);
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
            remap_for_input = find_remap_for_virt_code(virt_code);
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
        g_remap_parsee->to_when_alone &&
        g_remap_parsee->to_with_other;
}

/* @return error */
int load_config_line(char * line, int linenum)
{
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
        g_remap_parsee = new_remap(NULL, NULL, NULL);
    }

    if (strstr(line, "remap_key=")) {
        if (g_remap_parsee->from && !parsee_is_valid()) {
            printf("Config error (line %d): Incomplete remapping.\n"
                   "Each remapping must have a 'remap_key', 'when_alone', and 'with_other'.\n",
                   linenum);
            return 1;
        }
        g_remap_parsee->from = key_def;
    } else if (strstr(line, "when_alone=")) {
        g_remap_parsee->to_when_alone = key_def;
    } else if (strstr(line, "with_other=")) {
        g_remap_parsee->to_with_other = key_def;
    } else {
        after_eq[0] = 0;
        printf("Config error (line %d): Invalid setting '%s'.\n", linenum, line);
        return 1;
    }

    if (parsee_is_valid()) {
        register_remap(g_remap_parsee);
        g_remap_parsee = NULL;
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
        free(remap);
    }
    g_remap_list = NULL;
}
