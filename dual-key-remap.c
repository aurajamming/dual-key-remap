#define VERSION "0.8"
#define AUTHOR "ililim"

#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include "input.h"
#include "keys.c"
#include "remap.c"
#include "mouse.c"

#pragma comment(lib, "winmm.lib") // for timeGetTime()

// Globals
// ----------------

HHOOK g_keyboard_hook;
HHOOK g_mouse_hook;
HANDLE ghEvent;
HANDLE ghTimerQueue = NULL;
struct InputBuffer g_input_buffer;

void debug_print(const char* color, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("%s", color);
    vprintf(format, args);
    printf("%s", RESET);

    va_end(args);
}

void send_input(int scan_code, int virt_code, enum Direction direction, int remap_id, struct InputBuffer * input_buffer) {
    if (virt_code) {
        uint32_t n, tail;
        int index;
        n = input_buffer_move_prod_head(input_buffer, &tail);
        index = tail & INPUT_BUFFER_MASK;
        if (n == 0) return;
        ZeroMemory(&input_buffer->inputs[index], sizeof(INPUT));

        input_buffer->inputs[index].type = INPUT_KEYBOARD;
        input_buffer->inputs[index].ki.time = 0;
        input_buffer->inputs[index].ki.dwExtraInfo = (ULONG_PTR)(INJECTED_KEY_ID | remap_id);

        input_buffer->inputs[index].ki.wScan = scan_code;
        input_buffer->inputs[index].ki.wVk = ((g_scancode && scan_code != 0x00) ? 0 : virt_code);
        // Per MS Docs: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-keybd_even
        // we need to flag whether "the scan code was preceded by a prefix byte having the value 0xE0 (224)"
        int is_extended_key = scan_code>>8 == 0xE0;
        input_buffer->inputs[index].ki.dwFlags = (direction == UP ? KEYEVENTF_KEYUP : 0) |
            (is_extended_key ? KEYEVENTF_EXTENDEDKEY : 0) |
            ((g_scancode && scan_code != 0x00) ? KEYEVENTF_SCANCODE : 0);
        input_buffer_update_tail(&input_buffer->prod, tail, n);
    } else {
        mouse_emulation(scan_code, direction, remap_id, &g_input_buffer);
    }
}

LRESULT CALLBACK mouse_callback(int msg_code, WPARAM w_param, LPARAM l_param) {
    int block_input = 0;

    // Per MS docs we should only act for HC_ACTION's
    if (msg_code == HC_ACTION) {
        MSLLHOOKSTRUCT * data = (MSLLHOOKSTRUCT *)l_param;
        int is_injected = ((LLMHF_INJECTED & data->flags) && data->dwExtraInfo != 0x00) ? 1 : 0;
        switch (w_param) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_MOUSEWHEEL:
            // Since no key corresponds to the mouse inputs; use a dummy input
            block_input = handle_input(
                w_param,
                MOUSE_DUMMY_VK,
                DOWN,
                data->time,
                is_injected,
                data->flags,
                data->dwExtraInfo,
                &g_input_buffer);
        }

        if (block_input == -1) {
            uint32_t n, tail;
            int index;
            n = input_buffer_move_prod_head(&g_input_buffer, &tail);
            index = tail & INPUT_BUFFER_MASK;
            if (n == 0) return 1;
            ZeroMemory(&g_input_buffer.inputs[index], sizeof(INPUT));

            g_input_buffer.inputs[index].type = INPUT_MOUSE;
            g_input_buffer.inputs[index].mi.dwExtraInfo = (ULONG_PTR)INJECTED_KEY_ID;

            switch (w_param) {
            case WM_LBUTTONDOWN:
                g_input_buffer.inputs[index].mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
                break;
            case WM_RBUTTONDOWN:
                g_input_buffer.inputs[index].mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
                break;
            case WM_MBUTTONDOWN:
                g_input_buffer.inputs[index].mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
                break;
            case WM_XBUTTONDOWN:
                g_input_buffer.inputs[index].mi.dwFlags |= MOUSEEVENTF_XDOWN;
                g_input_buffer.inputs[index].mi.mouseData = data->mouseData;
                break;
            case WM_MOUSEWHEEL:
                g_input_buffer.inputs[index].mi.dwFlags |= MOUSEEVENTF_WHEEL;
                g_input_buffer.inputs[index].mi.mouseData = ((int)data->mouseData)>>16;
                break;
            }
            input_buffer_update_tail(&g_input_buffer.prod, tail, n);
        }
    }
    if (!input_buffer_empty(&g_input_buffer)) {
        SetEvent(ghEvent);
    }

    return (block_input) ? 1 : CallNextHookEx(NULL, msg_code, w_param, l_param);
}

LRESULT CALLBACK keyboard_callback(int msg_code, WPARAM w_param, LPARAM l_param) {
    int block_input = 0;
    
    // Per MS docs we should only act for HC_ACTION's
    if (msg_code == HC_ACTION) {
        KBDLLHOOKSTRUCT * data = (KBDLLHOOKSTRUCT *)l_param;
        enum Direction direction = (LLKHF_UP & data->flags) ? UP : DOWN;
        int is_injected = (LLKHF_INJECTED & data->flags) ? 1 : 0;
        block_input = handle_input(
            data->scanCode,
            data->vkCode,
            direction,
            data->time,
            is_injected,
            data->flags,
            data->dwExtraInfo,
            &g_input_buffer
        );

        if (block_input == -1) {
            send_input(data->scanCode, data->vkCode, direction, 0, &g_input_buffer);
        }
        if (!input_buffer_empty(&g_input_buffer)) {
            SetEvent(ghEvent);
        }
    }

    return (block_input) ? 1 : CallNextHookEx(NULL, msg_code, w_param, l_param);
}

void enable_ansi_support() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hConsole, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, mode);
}

void create_console() {
    if (AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        enable_ansi_support();
    }
}

void destroy_console() {
    fclose(stdout);
    fclose(stderr);
    FreeConsole();
}

int load_config_file(wchar_t * path) {
    FILE * file;
    char line[255];

    if (_wfopen_s(&file, path, L"r") > 0) {
        printf("Cannot open configuration file '%ws'. Make sure it is in the same directory as 'dual-key-remap.exe'.\n",
            path);
        return 1;
    }

    int linenum = 1;
    while (fgets(line, 255, file)) {
        if (load_config_line((char *)&line, linenum++)) {
            fclose(file);
            return 1;
        }
    };
    fclose(file);
    return load_config_line(NULL, linenum++);
}

void put_config_path(wchar_t * path) {
    HMODULE module = GetModuleHandleW(NULL);
    GetModuleFileNameW(module, path, MAX_PATH);
    path[wcslen(path) - strlen("dual-key-remap.exe")] = '\0';
    wcscat(path, L"config.txt");
}

void rehook() {
    UnhookWindowsHookEx(g_keyboard_hook);
    UnhookWindowsHookEx(g_mouse_hook);
    g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_callback, NULL, 0);
    g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_callback, NULL, 0);
    DEBUG(1, debug_print(RED, "\nRehooked!"));
}

DWORD WINAPI send_input_thread(LPVOID arg) {
    uint32_t n, tail;
    int index;
    struct InputBuffer * input_buffer = (struct InputBuffer *)arg;
    while (1) {
        WaitForSingleObject(ghEvent, INFINITE);
        ResetEvent(ghEvent);
        while (!input_buffer_empty(input_buffer)) {
            n = input_buffer_move_cons_head(input_buffer, -2, &tail);
            index = tail & INPUT_BUFFER_MASK;
            if (n > 0) {
                SendInput(n, &input_buffer->inputs[index], sizeof(INPUT));
                input_buffer_update_tail(&input_buffer->cons, tail, n);
            }
        }
    }
}

void close_all() {
    UnhookWindowsHookEx(g_keyboard_hook);
    UnhookWindowsHookEx(g_mouse_hook);
    if (g_active) g_active = 0;
    if (ghTimer) DeleteTimerQueueTimer(ghTimerQueue, ghTimer, NULL);
    CloseHandle(ghEvent);
    DeleteTimerQueue(ghTimerQueue);
    unlock_all(&g_input_buffer);
    free_all();
}

int main() {
    HANDLE threadHandle;
    DWORD threadId;

    // Initialization may print errors to stdout, create a console to show that output.
    create_console();
    printf("%s== dual-key-remap (version: %s, author: %s, adapted by ulixxe) ==%s\n\n", GREEN, VERSION, AUTHOR, RESET);

    HANDLE mutex = CreateMutex(NULL, TRUE, "dual-key-remap.single-instance");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        printf("dual-key-remap.exe is already running!\n");
        goto end;
    }

    wchar_t config_path[MAX_PATH];
    put_config_path(config_path);
    int err = load_config_file(config_path);
    if (err) {
        goto end;
    }

    if (g_priority) {
        if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
            printf("Error setting process priority: %d\n", GetLastError());
            goto end;
        }

        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
            printf("Error setting thread priority: %d\n", GetLastError());
            goto end;
        }
    }

    ghEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ghEvent == NULL) {
        printf("CreateEvent error: %d\n", GetLastError());
        goto end;
    }
    input_buffer_init(&g_input_buffer);
    threadHandle = CreateThread(NULL, 0, send_input_thread, &g_input_buffer, 0, &threadId);
    if (threadHandle == NULL) {
        printf("Error creating the thread: %d\n", GetLastError());
        goto end;
    }
    ghTimerQueue = CreateTimerQueue();

    g_debug = g_debug || getenv("DEBUG") != NULL;
    g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_callback, NULL, 0);
    g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_callback, NULL, 0);

    // We're all good if we got this far. Hide the console window unless we're debugging.
    if (g_debug) {
        printf("-- DEBUG MODE --\n");
    } else {
        destroy_console();
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    close_all();
    return 0;

    end:
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
}
