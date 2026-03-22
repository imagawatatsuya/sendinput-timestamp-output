#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <imm.h>

#define ASCII_BUFFER_SIZE 64
#define INPUT_WAIT_MS 1
#define STARTUP_WAIT_MS 100
#define ALT_TAB_WAIT_MS 50
#define POST_ALT_TAB_WAIT_MS 350

static const char* wday_ascii_lower[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat"};

void send_alt_tab(void);
void show_error_message(const char* message);
void send_ascii_string_with_layout(const char* str, HKL keyboard_layout);
void send_vk_key(WORD vk, DWORD key_up);
void send_vk_combo(WORD vk, BYTE shift_state);
void restore_ime_open_status(HWND hwnd, BOOL has_saved_ime_state, BOOL previous_ime_open);
BOOL prepare_ascii_input(HWND hwnd, BOOL* has_saved_ime_state, BOOL* previous_ime_open, HKL* keyboard_layout);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#if STARTUP_WAIT_MS > 0
    Sleep(STARTUP_WAIT_MS);
#endif

    send_alt_tab();

#if POST_ALT_TAB_WAIT_MS > 0
    Sleep(POST_ALT_TAB_WAIT_MS);
#endif

    time_t now;
    struct tm local_time;
    char time_string[ASCII_BUFFER_SIZE];
    int written;
    HWND target_window;
    BOOL has_saved_ime_state = FALSE;
    BOOL previous_ime_open = FALSE;
    HKL keyboard_layout = NULL;

    target_window = GetForegroundWindow();

    time(&now);
    if (localtime_s(&local_time, &now) != 0) {
        show_error_message("Failed to get local time after Alt+Tab.");
        return 1;
    }

    written = snprintf(time_string, sizeof(time_string),
                       "%04d/%02d/%02d(%s) %02d:%02d:%02d",
                       local_time.tm_year + 1900,
                       local_time.tm_mon + 1,
                       local_time.tm_mday,
                       wday_ascii_lower[local_time.tm_wday],
                       local_time.tm_hour,
                       local_time.tm_min,
                       local_time.tm_sec);
    if (written < 0 || written >= (int)sizeof(time_string)) {
        show_error_message("Failed to format ASCII time string after Alt+Tab.");
        return 1;
    }

    if (!prepare_ascii_input(target_window, &has_saved_ime_state, &previous_ime_open, &keyboard_layout)) {
        show_error_message("Failed to prepare ASCII input state.");
        return 1;
    }

    send_ascii_string_with_layout(time_string, keyboard_layout);
    restore_ime_open_status(target_window, has_saved_ime_state, previous_ime_open);

    return 0;
}

BOOL prepare_ascii_input(HWND hwnd, BOOL* has_saved_ime_state, BOOL* previous_ime_open, HKL* keyboard_layout) {
    DWORD target_thread_id;
    HIMC himc;

    if (has_saved_ime_state == NULL || previous_ime_open == NULL || keyboard_layout == NULL) {
        return FALSE;
    }

    *has_saved_ime_state = FALSE;
    *previous_ime_open = FALSE;

    target_thread_id = 0;
    if (hwnd != NULL) {
        target_thread_id = GetWindowThreadProcessId(hwnd, NULL);
    }
    if (target_thread_id == 0) {
        target_thread_id = GetCurrentThreadId();
    }

    *keyboard_layout = GetKeyboardLayout(target_thread_id);
    if (*keyboard_layout == NULL) {
        *keyboard_layout = GetKeyboardLayout(0);
    }

    if (hwnd == NULL) {
        return TRUE;
    }

    himc = ImmGetContext(hwnd);
    if (himc == NULL) {
        return TRUE;
    }

    *previous_ime_open = ImmGetOpenStatus(himc);
    *has_saved_ime_state = TRUE;
    ImmSetOpenStatus(himc, FALSE);
    ImmReleaseContext(hwnd, himc);

    return TRUE;
}

void restore_ime_open_status(HWND hwnd, BOOL has_saved_ime_state, BOOL previous_ime_open) {
    HIMC himc;

    if (!has_saved_ime_state || hwnd == NULL) {
        return;
    }

    himc = ImmGetContext(hwnd);
    if (himc == NULL) {
        return;
    }

    ImmSetOpenStatus(himc, previous_ime_open);
    ImmReleaseContext(hwnd, himc);
}

void send_ascii_string_with_layout(const char* str, HKL keyboard_layout) {
    size_t i;

    if (str == NULL) {
        return;
    }

    for (i = 0; str[i] != '\0'; ++i) {
        SHORT vk_info = VkKeyScanExA(str[i], keyboard_layout);
        WORD vk;
        BYTE shift_state;
        char error_buf[128];

        if (vk_info == -1) {
            snprintf(error_buf, sizeof(error_buf),
                     "VkKeyScanExA failed for char 0x%02X.",
                     (unsigned char)str[i]);
            show_error_message(error_buf);
            return;
        }

        vk = (WORD)(vk_info & 0xFF);
        shift_state = (BYTE)((vk_info >> 8) & 0xFF);
        send_vk_combo(vk, shift_state);

#if INPUT_WAIT_MS > 0
        Sleep(INPUT_WAIT_MS);
#endif
    }
}

void send_vk_combo(WORD vk, BYTE shift_state) {
    if (shift_state & 1) {
        send_vk_key(VK_SHIFT, 0);
    }
    if (shift_state & 2) {
        send_vk_key(VK_CONTROL, 0);
    }
    if (shift_state & 4) {
        send_vk_key(VK_MENU, 0);
    }

    send_vk_key(vk, 0);
    send_vk_key(vk, KEYEVENTF_KEYUP);

    if (shift_state & 4) {
        send_vk_key(VK_MENU, KEYEVENTF_KEYUP);
    }
    if (shift_state & 2) {
        send_vk_key(VK_CONTROL, KEYEVENTF_KEYUP);
    }
    if (shift_state & 1) {
        send_vk_key(VK_SHIFT, KEYEVENTF_KEYUP);
    }
}

void send_vk_key(WORD vk, DWORD key_up) {
    INPUT input;

    ZeroMemory(&input, sizeof(input));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = key_up;

    if (SendInput(1, &input, sizeof(input)) == 0) {
        char error_buf[128];
        snprintf(error_buf, sizeof(error_buf),
                 "SendInput failed for virtual key 0x%02X (Error: %lu)",
                 (unsigned int)vk, GetLastError());
        show_error_message(error_buf);
    }
}

void show_error_message(const char* message) {
    MessageBoxA(NULL, message, "Get Time SendInput ASCII Fast Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
}

void send_alt_tab(void) {
    INPUT inputs[4] = {0};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_MENU;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_TAB;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_TAB;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_MENU;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(1, &inputs[0], sizeof(INPUT));
    Sleep(ALT_TAB_WAIT_MS);
    SendInput(1, &inputs[1], sizeof(INPUT));
    Sleep(ALT_TAB_WAIT_MS);
    SendInput(1, &inputs[2], sizeof(INPUT));
    Sleep(ALT_TAB_WAIT_MS);
    SendInput(1, &inputs[3], sizeof(INPUT));
}
