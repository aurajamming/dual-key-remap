#include <stddef.h>

#ifndef KEYS_C
#define KEYS_C

struct KeyDef {
    char * name;
    int scan_code;
    int virt_code;
    int is_modifier;
};
typedef const struct KeyDef KEY_DEF;

#define VK_LEFT_CTRL 0xA2
#define VK_RIGHT_CTRL 0xA3
#define VK_LEFT_ALT 0xA4
#define VK_RIGHT_ALT 0xA5
#define VK_LEFT_SHIFT 0xA0
#define VK_RIGHT_SHIFT 0xA1
#define VK_LEFT_WIN 0x5B
#define VK_RIGHT_WIN 0x5C

#define VK_BACKSPACE 0x08
#define VK_CAPSLOCK 0x14
#define VK_ENTER 0x0D
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_TAB 0x09

#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28

#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_F13 0x7C
#define VK_F14 0x7D
#define VK_F15 0x7E
#define VK_F16 0x7F
#define VK_F17 0x80
#define VK_F18 0x81
#define VK_F19 0x82
#define VK_F20 0x83
#define VK_F21 0x84
#define VK_F22 0x85
#define VK_F23 0x86
#define VK_F24 0x87

#define VK_KEY_0 0x30
#define VK_KEY_1 0x31
#define VK_KEY_2 0x32
#define VK_KEY_3 0x33
#define VK_KEY_4 0x34
#define VK_KEY_5 0x35
#define VK_KEY_6 0x36
#define VK_KEY_7 0x37
#define VK_KEY_8 0x38
#define VK_KEY_9 0x39

#define VK_KEY_A 0x41
#define VK_KEY_B 0x42
#define VK_KEY_C 0x43
#define VK_KEY_D 0x44
#define VK_KEY_E 0x45
#define VK_KEY_F 0x46
#define VK_KEY_G 0x47
#define VK_KEY_H 0x48
#define VK_KEY_I 0x49
#define VK_KEY_J 0x4A
#define VK_KEY_K 0x4B
#define VK_KEY_L 0x4C
#define VK_KEY_M 0x4D
#define VK_KEY_N 0x4E
#define VK_KEY_O 0x4F
#define VK_KEY_P 0x50
#define VK_KEY_Q 0x51
#define VK_KEY_R 0x52
#define VK_KEY_S 0x53
#define VK_KEY_T 0x54
#define VK_KEY_U 0x55
#define VK_KEY_V 0x56
#define VK_KEY_W 0x57
#define VK_KEY_X 0x58
#define VK_KEY_Y 0x59
#define VK_KEY_Z 0x5A

#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HOME 0x24
#define VK_END 0x23
#define VK_PAGE_UP 0x21
#define VK_PAGE_DOWN 0x22

#define VK_PRINT_SCREEN 0x2C
#define VK_NUMLOCK 0x90
#define VK_SCROLLLOCK 0x91
#define VK_PAUSE 0x13

#define VK_PLUS 0xBB
#define VK_COMMA 0xBC
#define VK_MINUS 0xBD
#define VK_PERIOD 0xBE

#define VK_US_SEMI 0xBA // ;: key on US keyboards
#define VK_US_SLASH 0xBF // /? key on US keyboards
#define VK_US_TILDE 0xC0 // `~ key on US keyboards

#define MOUSE_DUMMY_VK 0xFF

#define SK_LEFT_CTRL 0x001D
#define SK_RIGHT_CTRL 0xE01D
#define SK_LEFT_ALT 0x0038
#define SK_RIGHT_ALT 0xE038
#define SK_LEFT_SHIFT 0x002A
#define SK_RIGHT_SHIFT 0x0036
#define SK_LEFT_WIN 0xE05B
#define SK_RIGHT_WIN 0xE05C

#define SK_BACKSPACE 0x0E
#define SK_CAPSLOCK 0x3A
#define SK_ENTER 0x1C
#define SK_ESCAPE 0x01
#define SK_SPACE 0x39
#define SK_TAB 0x0F

#define SK_UP 0X48
#define SK_LEFT 0X4B
#define SK_RIGHT 0X4D
#define SK_DOWN 0X50

#define SK_F1 0x3B
#define SK_F2 0x3C
#define SK_F3 0x3D
#define SK_F4 0x3E
#define SK_F5 0x3F
#define SK_F6 0x40
#define SK_F7 0x41
#define SK_F8 0x42
#define SK_F9 0x43
#define SK_F10 0x44
#define SK_F11 0x57
#define SK_F12 0x58
#define SK_F13 0x64
#define SK_F14 0x65
#define SK_F15 0x66
#define SK_F16 0x67
#define SK_F17 0x68
#define SK_F18 0x69
#define SK_F19 0x6A
#define SK_F20 0x6B
#define SK_F21 0x6C
#define SK_F22 0x6D
#define SK_F23 0x6E
#define SK_F24 0x76

#define SK_KEY_0 0x0B
#define SK_KEY_1 0x02
#define SK_KEY_2 0x03
#define SK_KEY_3 0x04
#define SK_KEY_4 0x05
#define SK_KEY_5 0x06
#define SK_KEY_6 0x07
#define SK_KEY_7 0x08
#define SK_KEY_8 0x09
#define SK_KEY_9 0x0A

#define SK_KEY_A 0x1E
#define SK_KEY_B 0x30
#define SK_KEY_C 0x2E
#define SK_KEY_D 0x20
#define SK_KEY_E 0x12
#define SK_KEY_F 0x21
#define SK_KEY_G 0x22
#define SK_KEY_H 0x23
#define SK_KEY_I 0x17
#define SK_KEY_J 0x24
#define SK_KEY_K 0x25
#define SK_KEY_L 0x26
#define SK_KEY_M 0x32
#define SK_KEY_N 0x31
#define SK_KEY_O 0x18
#define SK_KEY_P 0x19
#define SK_KEY_Q 0x10
#define SK_KEY_R 0x13
#define SK_KEY_S 0x1F
#define SK_KEY_T 0x14
#define SK_KEY_U 0x16
#define SK_KEY_V 0x2F
#define SK_KEY_W 0x11
#define SK_KEY_X 0x2D
#define SK_KEY_Y 0x15
#define SK_KEY_Z 0x2C

#define SK_INSERT 0x52
#define SK_DELETE 0x53
#define SK_HOME 0x47
#define SK_END 0x4f
#define SK_PAGE_UP 0x49
#define SK_PAGE_DOWN 0x51

#define SK_PLUS 0x0d
#define SK_COMMA 0x33
#define SK_MINUS 0x0c
#define SK_PERIOD 0x34

#define SK_US_SEMI 0x27  // ;: key on US keyboards
#define SK_US_SLASH 0x35 // /? key on US keyboards
#define SK_US_TILDE 0x29 // `~ key on US keyboards

enum {
  /** Move up. */
  MS_U = 1,
  /** Move down. */
  MS_D = 2,
  /** Move left. */
  MS_L = 3,
  /** Move right. */
  MS_R = 4,
  /** Move forward. */
  MS_F = 5,
  /** Move backward. */
  MS_B = 6,
  /** Steer left (counter-clockwise). */
  MS_S_L = 7,
  /** Steer right (clockwise). */
  MS_S_R = 8,
  /** Mouse wheel up. */
  MS_W_U = 9,
  /** Mouse wheel down. */
  MS_W_D = 10,
  /** Mouse wheel left. */
  MS_W_L = 11,
  /** Mouse wheel right. */
  MS_W_R = 12,
  /** Press mouse button 1. */
  MS_BTN1 = 13,
  /** Press mouse button 2. */
  MS_BTN2 = 14,
  /** Press mouse button 3. */
  MS_BTN3 = 15,
  /** Press mouse button 4. */
  MS_BTN4 = 16,
  /** Press mouse button 5. */
  MS_BTN5 = 17,
  /** Press the selected mouse button. */
  MS_BTNS = 18,
  /** Hold the selected mouse button. */
  MS_HLDS = 19,
  /** Release the selected mouse button. */
  MS_RELS = 20,
  /** Select mouse button 1. */
  MS_SEL1 = 21,
  /** Select mouse button 2. */
  MS_SEL2 = 22,
  /** Select mouse button 3. */
  MS_SEL3 = 23,
  /** Select mouse button 4. */
  MS_SEL4 = 24,
  /** Select mouse button 5. */
  MS_SEL5 = 25,
};

// The table of configurable key names and their respective codes.
//
// We identify the virtual codes with a binary flag.
//
// When sending output using hardware scan codes is recommended as these do not
// get intercepted by DirectX and thus will work in more applications.
//
// When reading input the virtual codes will work accross more locales and envs.
//
// For backwards compatibility modifiers refer to the left key by default.
KEY_DEF key_table[] = {
    {"CTRL", SK_LEFT_CTRL, VK_LEFT_CTRL, 1},
    {"LEFT_CTRL", SK_LEFT_CTRL, VK_LEFT_CTRL, 1},
    {"RIGHT_CTRL", SK_RIGHT_CTRL, VK_RIGHT_CTRL, 1},

    {"SHIFT", SK_LEFT_SHIFT, VK_LEFT_SHIFT, 1},
    {"LEFT_SHIFT", SK_LEFT_SHIFT, VK_LEFT_SHIFT, 1},
    {"RIGHT_SHIFT", SK_RIGHT_SHIFT, VK_RIGHT_SHIFT, 1},

    {"ALT", SK_LEFT_ALT, VK_LEFT_ALT, 1},
    {"LEFT_ALT", SK_LEFT_ALT, VK_LEFT_ALT, 1},
    {"RIGHT_ALT", SK_RIGHT_ALT, VK_RIGHT_ALT, 1},

    {"LEFT_WIN", SK_LEFT_WIN, VK_LEFT_WIN, 1},
    {"RIGHT_WIN", SK_RIGHT_WIN, VK_RIGHT_WIN, 1},

    {"BACKSPACE", SK_BACKSPACE, VK_BACKSPACE, 0},
    {"CAPSLOCK", SK_CAPSLOCK, VK_CAPSLOCK, 1},
    {"ENTER", SK_ENTER, VK_ENTER, 0},
    {"ESCAPE", SK_ESCAPE, VK_ESCAPE, 0},
    {"SPACE", SK_SPACE, VK_SPACE, 0},
    {"TAB", SK_TAB, VK_TAB, 0},

    {"UP", SK_UP, VK_UP, 0},
    {"LEFT", SK_LEFT, VK_LEFT, 0},
    {"RIGHT", SK_RIGHT, VK_RIGHT, 0},
    {"DOWN", SK_DOWN, VK_DOWN, 0},

    {"F1", SK_F1, VK_F1, 0},
    {"F2", SK_F2, VK_F2, 0},
    {"F3", SK_F3, VK_F3, 0},
    {"F4", SK_F4, VK_F4, 0},
    {"F5", SK_F5, VK_F5, 0},
    {"F6", SK_F6, VK_F6, 0},
    {"F7", SK_F7, VK_F7, 0},
    {"F8", SK_F8, VK_F8, 0},
    {"F9", SK_F9, VK_F9, 0},
    {"F10", SK_F10, VK_F10, 0},
    {"F11", SK_F11, VK_F11, 0},
    {"F12", SK_F12, VK_F12, 0},
    {"F13", SK_F13, VK_F13, 0},
    {"F14", SK_F14, VK_F14, 0},
    {"F15", SK_F15, VK_F15, 0},
    {"F16", SK_F16, VK_F16, 0},
    {"F17", SK_F17, VK_F17, 0},
    {"F18", SK_F18, VK_F18, 0},
    {"F19", SK_F19, VK_F19, 0},
    {"F20", SK_F20, VK_F20, 0},
    {"F21", SK_F21, VK_F21, 0},
    {"F22", SK_F22, VK_F22, 0},
    {"F23", SK_F23, VK_F23, 0},
    {"F24", SK_F24, VK_F24, 0},

    {"KEY_0", SK_KEY_0, VK_KEY_0, 0},
    {"KEY_1", SK_KEY_1, VK_KEY_1, 0},
    {"KEY_2", SK_KEY_2, VK_KEY_2, 0},
    {"KEY_3", SK_KEY_3, VK_KEY_3, 0},
    {"KEY_4", SK_KEY_4, VK_KEY_4, 0},
    {"KEY_5", SK_KEY_5, VK_KEY_5, 0},
    {"KEY_6", SK_KEY_6, VK_KEY_6, 0},
    {"KEY_7", SK_KEY_7, VK_KEY_7, 0},
    {"KEY_8", SK_KEY_8, VK_KEY_8, 0},
    {"KEY_9", SK_KEY_9, VK_KEY_9, 0},

    {"KEY_A", SK_KEY_A, VK_KEY_A, 0},
    {"KEY_B", SK_KEY_B, VK_KEY_B, 0},
    {"KEY_C", SK_KEY_C, VK_KEY_C, 0},
    {"KEY_D", SK_KEY_D, VK_KEY_D, 0},
    {"KEY_E", SK_KEY_E, VK_KEY_E, 0},
    {"KEY_F", SK_KEY_F, VK_KEY_F, 0},
    {"KEY_G", SK_KEY_G, VK_KEY_G, 0},
    {"KEY_H", SK_KEY_H, VK_KEY_H, 0},
    {"KEY_I", SK_KEY_I, VK_KEY_I, 0},
    {"KEY_J", SK_KEY_J, VK_KEY_J, 0},
    {"KEY_K", SK_KEY_K, VK_KEY_K, 0},
    {"KEY_L", SK_KEY_L, VK_KEY_L, 0},
    {"KEY_M", SK_KEY_M, VK_KEY_M, 0},
    {"KEY_N", SK_KEY_N, VK_KEY_N, 0},
    {"KEY_O", SK_KEY_O, VK_KEY_O, 0},
    {"KEY_P", SK_KEY_P, VK_KEY_P, 0},
    {"KEY_Q", SK_KEY_Q, VK_KEY_Q, 0},
    {"KEY_R", SK_KEY_R, VK_KEY_R, 0},
    {"KEY_S", SK_KEY_S, VK_KEY_S, 0},
    {"KEY_T", SK_KEY_T, VK_KEY_T, 0},
    {"KEY_U", SK_KEY_U, VK_KEY_U, 0},
    {"KEY_V", SK_KEY_V, VK_KEY_V, 0},
    {"KEY_W", SK_KEY_W, VK_KEY_W, 0},
    {"KEY_X", SK_KEY_X, VK_KEY_X, 0},
    {"KEY_Y", SK_KEY_Y, VK_KEY_Y, 0},
    {"KEY_Z", SK_KEY_Z, VK_KEY_Z, 0},

    {"INSERT", SK_INSERT, VK_INSERT, 0},
    {"DELETE", SK_DELETE, VK_DELETE, 0},
    {"HOME", SK_HOME, VK_HOME, 0},
    {"END", SK_END, VK_END, 0},
    {"PAGE_UP", SK_PAGE_UP, VK_PAGE_UP, 0},
    {"PAGE_DOWN", SK_PAGE_DOWN, VK_PAGE_DOWN, 0},

    {"PRINT_SCREEN", 0, VK_PRINT_SCREEN, 0},
    {"NUMLOCK", 0, VK_NUMLOCK, 0},
    {"SCROLLLOCK", 0, VK_SCROLLLOCK, 0},
    {"PAUSE", 0, VK_PAUSE, 0},

    {"PLUS", SK_PLUS, VK_PLUS, 0},
    {"COMMA", SK_COMMA, VK_COMMA, 0},
    {"MINUS", SK_MINUS, VK_MINUS, 0},
    {"PERIOD", SK_PERIOD, VK_PERIOD, 0},

    {"US_SEMI", SK_US_SEMI, VK_US_SEMI, 0}, // ;: key on US keyboards
    {"US_SLASH", SK_US_SLASH, VK_US_SLASH, 0}, // /? key on US keyboards
    {"US_TILDE", SK_US_TILDE, VK_US_TILDE, 0}, // `~ key on US keyboards

    {"MOUSE_UP", MS_U, 0, 0}, // Move up
    {"MOUSE_DOWN", MS_D, 0, 0}, // Move down
    {"MOUSE_LEFT", MS_L, 0, 0}, // Move left
    {"MOUSE_RIGHT", MS_R, 0, 0}, // Move right
    {"MOUSE_FORWARD", MS_F, 0, 0}, // Move forward
    {"MOUSE_BACKWARD", MS_B, 0, 0}, // Move backward
    {"MOUSE_STEER_LEFT", MS_S_L, 0, 0}, // Steer left (counter-clockwise)
    {"MOUSE_STEER_RIGHT", MS_S_R, 0, 0}, // Steer right (clockwise)
    {"MOUSE_WHEEL_UP", MS_W_U, 0, 0}, // Mouse wheel up
    {"MOUSE_WHEEL_DOWN", MS_W_D, 0, 0}, // Mouse wheel down
    {"MOUSE_WHEEL_LEFT", MS_W_L, 0, 0}, // Mouse wheel left
    {"MOUSE_WHEEL_RIGHT", MS_W_R, 0, 0}, // Mouse wheel right
    {"MOUSE_LBUTTON", MS_BTN1, 0, 0}, // Press mouse button 1
    {"MOUSE_RBUTTON", MS_BTN2, 0, 0}, // Press mouse button 2
    {"MOUSE_MBUTTON", MS_BTN3, 0, 0}, // Press mouse button 3
    {"MOUSE_XBUTTON1", MS_BTN4, 0, 0}, // Press mouse button 4
    {"MOUSE_XBUTTON2", MS_BTN5, 0, 0}, // Press mouse button 5
    {"MOUSE_SBUTTON", MS_BTNS, 0, 0}, // Press the selected mouse button
    {"MOUSE_SHOLD", MS_HLDS, 0, 0}, // Hold the selected mouse button
    {"MOUSE_SRELEASE", MS_RELS, 0, 0}, // Release the selected mouse button
    {"MOUSE_LBUTTON_SEL", MS_SEL1, 0, 0}, // Select mouse button 1
    {"MOUSE_RBUTTON_SEL", MS_SEL2, 0, 0}, // Select mouse button 2
    {"MOUSE_MBUTTON_SEL", MS_SEL3, 0, 0}, // Select mouse button 3
    {"MOUSE_XBUTTON1_SEL", MS_SEL4, 0, 0}, // Select mouse button 4
    {"MOUSE_XBUTTON2_SEL", MS_SEL5, 0, 0}, // Select mouse button 5
};

KEY_DEF KEY_ARRAY[256] = {
    [VK_LEFT_CTRL] = {"LEFT_CTRL", SK_LEFT_CTRL, VK_LEFT_CTRL, 1},
    [VK_RIGHT_CTRL] = {"RIGHT_CTRL", SK_RIGHT_CTRL, VK_RIGHT_CTRL, 1},
    [VK_LEFT_SHIFT] = {"LEFT_SHIFT", SK_LEFT_SHIFT, VK_LEFT_SHIFT, 1},
    [VK_RIGHT_SHIFT] = {"RIGHT_SHIFT", SK_RIGHT_SHIFT, VK_RIGHT_SHIFT, 1},
    [VK_LEFT_ALT] = {"LEFT_ALT", SK_LEFT_ALT, VK_LEFT_ALT, 1},
    [VK_RIGHT_ALT] = {"RIGHT_ALT", SK_RIGHT_ALT, VK_RIGHT_ALT, 1},
    [VK_LEFT_WIN] = {"LEFT_WIN", SK_LEFT_WIN, VK_LEFT_WIN, 1},
    [VK_RIGHT_WIN] = {"RIGHT_WIN", SK_RIGHT_WIN, VK_RIGHT_WIN, 1},
    [VK_BACKSPACE] = {"BACKSPACE", SK_BACKSPACE, VK_BACKSPACE, 0},
    [VK_CAPSLOCK] = {"CAPSLOCK", SK_CAPSLOCK, VK_CAPSLOCK, 1},
    [VK_ENTER] = {"ENTER", SK_ENTER, VK_ENTER, 0},
    [VK_ESCAPE] = {"ESCAPE", SK_ESCAPE, VK_ESCAPE, 0},
    [VK_SPACE] = {"SPACE", SK_SPACE, VK_SPACE, 0},
    [VK_TAB] = {"TAB", SK_TAB, VK_TAB, 0},
    [VK_UP] = {"UP", SK_UP, VK_UP, 0},
    [VK_LEFT] = {"LEFT", SK_LEFT, VK_LEFT, 0},
    [VK_RIGHT] = {"RIGHT", SK_RIGHT, VK_RIGHT, 0},
    [VK_DOWN] = {"DOWN", SK_DOWN, VK_DOWN, 0},
    [VK_F1] = {"F1", SK_F1, VK_F1, 0},
    [VK_F2] = {"F2", SK_F2, VK_F2, 0},
    [VK_F3] = {"F3", SK_F3, VK_F3, 0},
    [VK_F4] = {"F4", SK_F4, VK_F4, 0},
    [VK_F5] = {"F5", SK_F5, VK_F5, 0},
    [VK_F6] = {"F6", SK_F6, VK_F6, 0},
    [VK_F7] = {"F7", SK_F7, VK_F7, 0},
    [VK_F8] = {"F8", SK_F8, VK_F8, 0},
    [VK_F9] = {"F9", SK_F9, VK_F9, 0},
    [VK_F10] = {"F10", SK_F10, VK_F10, 0},
    [VK_F11] = {"F11", SK_F11, VK_F11, 0},
    [VK_F12] = {"F12", SK_F12, VK_F12, 0},
    [VK_F13] = {"F13", SK_F13, VK_F13, 0},
    [VK_F14] = {"F14", SK_F14, VK_F14, 0},
    [VK_F15] = {"F15", SK_F15, VK_F15, 0},
    [VK_F16] = {"F16", SK_F16, VK_F16, 0},
    [VK_F17] = {"F17", SK_F17, VK_F17, 0},
    [VK_F18] = {"F18", SK_F18, VK_F18, 0},
    [VK_F19] = {"F19", SK_F19, VK_F19, 0},
    [VK_F20] = {"F20", SK_F20, VK_F20, 0},
    [VK_F21] = {"F21", SK_F21, VK_F21, 0},
    [VK_F22] = {"F22", SK_F22, VK_F22, 0},
    [VK_F23] = {"F23", SK_F23, VK_F23, 0},
    [VK_F24] = {"F24", SK_F24, VK_F24, 0},
    [VK_KEY_0] = {"KEY_0", SK_KEY_0, VK_KEY_0, 0},
    [VK_KEY_1] = {"KEY_1", SK_KEY_1, VK_KEY_1, 0},
    [VK_KEY_2] = {"KEY_2", SK_KEY_2, VK_KEY_2, 0},
    [VK_KEY_3] = {"KEY_3", SK_KEY_3, VK_KEY_3, 0},
    [VK_KEY_4] = {"KEY_4", SK_KEY_4, VK_KEY_4, 0},
    [VK_KEY_5] = {"KEY_5", SK_KEY_5, VK_KEY_5, 0},
    [VK_KEY_6] = {"KEY_6", SK_KEY_6, VK_KEY_6, 0},
    [VK_KEY_7] = {"KEY_7", SK_KEY_7, VK_KEY_7, 0},
    [VK_KEY_8] = {"KEY_8", SK_KEY_8, VK_KEY_8, 0},
    [VK_KEY_9] = {"KEY_9", SK_KEY_9, VK_KEY_9, 0},
    [VK_KEY_A] = {"KEY_A", SK_KEY_A, VK_KEY_A, 0},
    [VK_KEY_B] = {"KEY_B", SK_KEY_B, VK_KEY_B, 0},
    [VK_KEY_C] = {"KEY_C", SK_KEY_C, VK_KEY_C, 0},
    [VK_KEY_D] = {"KEY_D", SK_KEY_D, VK_KEY_D, 0},
    [VK_KEY_E] = {"KEY_E", SK_KEY_E, VK_KEY_E, 0},
    [VK_KEY_F] = {"KEY_F", SK_KEY_F, VK_KEY_F, 0},
    [VK_KEY_G] = {"KEY_G", SK_KEY_G, VK_KEY_G, 0},
    [VK_KEY_H] = {"KEY_H", SK_KEY_H, VK_KEY_H, 0},
    [VK_KEY_I] = {"KEY_I", SK_KEY_I, VK_KEY_I, 0},
    [VK_KEY_J] = {"KEY_J", SK_KEY_J, VK_KEY_J, 0},
    [VK_KEY_K] = {"KEY_K", SK_KEY_K, VK_KEY_K, 0},
    [VK_KEY_L] = {"KEY_L", SK_KEY_L, VK_KEY_L, 0},
    [VK_KEY_M] = {"KEY_M", SK_KEY_M, VK_KEY_M, 0},
    [VK_KEY_N] = {"KEY_N", SK_KEY_N, VK_KEY_N, 0},
    [VK_KEY_O] = {"KEY_O", SK_KEY_O, VK_KEY_O, 0},
    [VK_KEY_P] = {"KEY_P", SK_KEY_P, VK_KEY_P, 0},
    [VK_KEY_Q] = {"KEY_Q", SK_KEY_Q, VK_KEY_Q, 0},
    [VK_KEY_R] = {"KEY_R", SK_KEY_R, VK_KEY_R, 0},
    [VK_KEY_S] = {"KEY_S", SK_KEY_S, VK_KEY_S, 0},
    [VK_KEY_T] = {"KEY_T", SK_KEY_T, VK_KEY_T, 0},
    [VK_KEY_U] = {"KEY_U", SK_KEY_U, VK_KEY_U, 0},
    [VK_KEY_V] = {"KEY_V", SK_KEY_V, VK_KEY_V, 0},
    [VK_KEY_W] = {"KEY_W", SK_KEY_W, VK_KEY_W, 0},
    [VK_KEY_X] = {"KEY_X", SK_KEY_X, VK_KEY_X, 0},
    [VK_KEY_Y] = {"KEY_Y", SK_KEY_Y, VK_KEY_Y, 0},
    [VK_KEY_Z] = {"KEY_Z", SK_KEY_Z, VK_KEY_Z, 0},
    [VK_INSERT] = {"INSERT", SK_INSERT, VK_INSERT, 0},
    [VK_DELETE] = {"DELETE", SK_DELETE, VK_DELETE, 0},
    [VK_HOME] = {"HOME", SK_HOME, VK_HOME, 0},
    [VK_END] = {"END", SK_END, VK_END, 0},
    [VK_PAGE_UP] = {"PAGE_UP", SK_PAGE_UP, VK_PAGE_UP, 0},
    [VK_PAGE_DOWN] = {"PAGE_DOWN", SK_PAGE_DOWN, VK_PAGE_DOWN, 0},
    [VK_PRINT_SCREEN] = {"PRINT_SCREEN", 0, VK_PRINT_SCREEN, 0},
    [VK_NUMLOCK] = {"NUMLOCK", 0, VK_NUMLOCK, 0},
    [VK_SCROLLLOCK] = {"SCROLLLOCK", 0, VK_SCROLLLOCK, 0},
    [VK_PAUSE] = {"PAUSE", 0, VK_PAUSE, 0},
    [VK_PLUS] = {"PLUS", SK_PLUS, VK_PLUS, 0},
    [VK_COMMA] = {"COMMA", SK_COMMA, VK_COMMA, 0},
    [VK_MINUS] = {"MINUS", SK_MINUS, VK_MINUS, 0},
    [VK_PERIOD] = {"PERIOD", SK_PERIOD, VK_PERIOD, 0},
    [VK_US_SEMI] = {"US_SEMI", SK_US_SEMI, VK_US_SEMI, 0}, // ;: key on US keyboards
    [VK_US_SLASH] = {"US_SLASH", SK_US_SLASH, VK_US_SLASH, 0}, // /? key on US keyboards
    [VK_US_TILDE] = {"US_TILDE", SK_US_TILDE, VK_US_TILDE, 0}, // `~ key on US keyboards
};

#define KEY_TABLE_LEN (sizeof(key_table) / sizeof(struct KeyDef))

// Shortcuts to common keys (useful for debugging/testing)
KEY_DEF * CTRL  = &key_table[0];
KEY_DEF * SHIFT = &key_table[3];
KEY_DEF * LSHIFT = &key_table[4];
KEY_DEF * RSHIFT = &key_table[5];
KEY_DEF * ALT   = &key_table[6];
KEY_DEF * CAPS  = &key_table[12];
KEY_DEF * ENTER = &key_table[13];
KEY_DEF * ESC   = &key_table[14];
KEY_DEF * SPACE = &key_table[15];
KEY_DEF * TAB   = &key_table[16];

KEY_DEF * find_key_def_by_name(char * name)
{
    if (name) {
        for (int i = 0; i < KEY_TABLE_LEN; ++i) {
            KEY_DEF * key = key_table + i;
            if (strcmp(key->name, name) == 0) {
                return key;
            }
        }
    }
    return NULL;
}

KEY_DEF * find_key_def_by_scan_code(int code)
{
    for (int i = 0; i < KEY_TABLE_LEN; ++i) {
        KEY_DEF * key = key_table + i;
        if (key->scan_code == code) {
            return key;
        }
    }
    return NULL;
}

KEY_DEF * find_key_def_by_virt_code(int code)
{
    for (int i = 0; i < KEY_TABLE_LEN; ++i) {
        KEY_DEF * key = key_table + i;
        if (key->virt_code == code) {
            return key;
        }
    }
    return NULL;
}

int vk_is_modifier(int code)
{
    return (KEY_ARRAY[code & 0xFF]).is_modifier;
}

// Defaults to the remappable key names per our definitions, but also
// handles more obscure codes that aren't available for remapping (yet).
// These fallback names are wrapped in angle brackets for log clarity.
// See: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
char * friendly_virt_code_name(int code)
{
    KEY_DEF * key = find_key_def_by_virt_code(code);
    if (key) return key->name;

    switch (code)
    {
    case MOUSE_DUMMY_VK:
        return "<MOUSE INPUT>";
    case 0:
        return "<ZERO_CODE>";
    case 0x01:
        return "<MOUSE_LEFT>";
    case 0x02:
        return "<MOUSE_RIGHT>";
    case 0x03:
        return "<CANCEL>";
    case 0x04:
        return "<MOUSE_MIDDLE>";
    case 0x05:
        return "<MOUSE_X1>";
    case 0x06:
        return "<MOUSE_X2>";
    case 0x0C:
        return "<CLEAR>";
    case 0x10:
        return "<SHIFT_NO_DIR>";
    case 0x11:
        return "<CTRL_NO_DIR>";
    case 0x12:
        return "<ALT_NO_DIR>";
    case 0x15:
        return "<IME_KANA_OR_HANGUL>";
    case 0x16:
        return "<IME_ON>";
    case 0x17:
        return "<IME_JUNJA>";
    case 0x18:
        return "<IME_FINAL>";
    case 0x19:
        return "<IME_HANJA_OR_KANJI>";
    case 0x1A:
        return "<IME_OFF>";
    case 0x1C:
        return "<IME_CONVERT>";
    case 0x1D:
        return "<IME_NONCONVERT>";
    case 0x1E:
        return "<IME_ACCEPT>";
    case 0x1F:
        return "<IME_MODE_CHANGE>";
    case 0x29:
        return "<SELECT>";
    case 0x2A:
        return "<PRINT>";
    case 0x2B:
        return "<EXECUTE>";
    case 0x2F:
        return "<HELP>";
    case 0x5D:
        return "<APPS>";
    case 0x5F:
        return "<SLEEP>";
    case 0x60:
        return "<NUMPAD_0>";
    case 0x61:
        return "<NUMPAD_1>";
    case 0x62:
        return "<NUMPAD_2>";
    case 0x63:
        return "<NUMPAD_3>";
    case 0x64:
        return "<NUMPAD_4>";
    case 0x65:
        return "<NUMPAD_5>";
    case 0x66:
        return "<NUMPAD_6>";
    case 0x67:
        return "<NUMPAD_7>";
    case 0x68:
        return "<NUMPAD_8>";
    case 0x69:
        return "<NUMPAD_9>";
    case 0x6A:
        return "<MULTIPLY>";
    case 0x6B:
        return "<ADD>";
    case 0x6C:
        return "<SEPARATOR>";
    case 0x6D:
        return "<SUBTRACT>";
    case 0x6E:
        return "<DECIMAL>";
    case 0x6F:
        return "<DIVIDE>";
    case 0xA6:
        return "<BROWSER_BACK>";
    case 0xA7:
        return "<BROWSER_FORWARD>";
    case 0xA8:
        return "<BROWSER_REFRESH>";
    case 0xA9:
        return "<BROWSER_STOP>";
    case 0xAA:
        return "<BROWSER_SEARCH>";
    case 0xAB:
        return "<BROWSER_FAVORITES>";
    case 0xAC:
        return "<BROWSER_HOME>";
    case 0xAD:
        return "<VOLUME_MUTE>";
    case 0xAE:
        return "<VOLUME_DOWN>";
    case 0xAF:
        return "<VOLUME_UP>";
    case 0xB0:
        return "<MEDIA_NEXT_TRACK>";
    case 0xB1:
        return "<MEDIA_PREV_TRACK>";
    case 0xB2:
        return "<MEDIA_STOP>";
    case 0xB3:
        return "<MEDIA_PLAY_PAUSE>";
    case 0xB4:
        return "<LAUNCH_MAIL>";
    case 0xB5:
        return "<LAUNCH_MEDIA_SELECT>";
    case 0xB6:
        return "<LAUNCH_APP1>";
    case 0xB7:
        return "<LAUNCH_APP2>";
    case 0xBA:
        return "<OEM_1>";
    case 0xBF:
        return "<OEM_2>";
    case 0xC0:
        return "<OEM_3>";
    case 0xDB:
        return "<OEM_4>";
    case 0xDC:
        return "<OEM_5>";
    case 0xDD:
        return "<OEM_6>";
    case 0xDE:
        return "<OEM_7>";
    case 0xDF:
        return "<OEM_8>";
    case 0xE2:
        return "<OEM_102>";
    case 0xE5:
        return "<IME_PROCESS>";
    case 0xE6:
        return "<OEM_SPECIFIC>";
    case 0xE7:
        return "<PACKET>";
    case 0xF6:
        return "<ATTN>";
    case 0xF7:
        return "<CRSEL>";
    case 0xF8:
        return "<EXSEL>";
    case 0xF9:
        return "<EREOF>";
    case 0xFA:
        return "<PLAY>";
    case 0xFB:
        return "<ZOOM>";
    case 0xFC:
        return "<NONAME>";
    case 0xFD:
        return "<PA1>";
    case 0xFE:
        return "<OEM_CLEA>";
    default:
        return "<UNKNOWN>";
    }
}

#endif
