#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <imm.h>

#define ASCII_BUFFER_SIZE 64
#define INPUT_WAIT_MS 1
#define STARTUP_WAIT_MS 100
#define ALT_TAB_WAIT_MS 50
#define POST_ALT_TAB_WAIT_MS 350

static const char* wday_ascii_lower[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat"};

typedef struct {
    HWND ime_target_window;
    HKL keyboard_layout;
    BOOL has_ime_context;
    BOOL has_conversion_state;
    BOOL previous_ime_open;
    DWORD previous_conversion_mode;
    DWORD previous_sentence_mode;
} ASCII_INPUT_STATE;

void send_alt_tab(void);
void show_error_message(const char* message);
void send_ascii_string_with_layout(const char* str, HKL keyboard_layout);
void send_vk_key(WORD vk, DWORD key_up);
void send_vk_combo(WORD vk, BYTE shift_state);
BOOL prepare_ascii_input(HWND foreground_window, ASCII_INPUT_STATE* input_state);
void restore_ascii_input(const ASCII_INPUT_STATE* input_state);
HWND find_ime_target_window(HWND foreground_window, DWORD* target_thread_id);
DWORD build_halfwidth_alnum_conversion_mode(DWORD current_conversion_mode);

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
    HWND foreground_window;
    ASCII_INPUT_STATE input_state;

    ZeroMemory(&input_state, sizeof(input_state));
    foreground_window = GetForegroundWindow();

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

    if (!prepare_ascii_input(foreground_window, &input_state)) {
        show_error_message("Failed to prepare ASCII input state.");
        return 1;
    }

    send_ascii_string_with_layout(time_string, input_state.keyboard_layout);
    restore_ascii_input(&input_state);

    return 0;
}

BOOL prepare_ascii_input(HWND foreground_window, ASCII_INPUT_STATE* input_state) {
    HIMC himc;
    DWORD target_thread_id = 0;

    if (input_state == NULL) {
        return FALSE;
    }

    ZeroMemory(input_state, sizeof(*input_state));
    input_state->ime_target_window = find_ime_target_window(foreground_window, &target_thread_id);

    if (target_thread_id != 0) {
        input_state->keyboard_layout = GetKeyboardLayout(target_thread_id);
    }
    if (input_state->keyboard_layout == NULL) {
        input_state->keyboard_layout = GetKeyboardLayout(0);
    }

    if (input_state->ime_target_window == NULL) {
        return TRUE;
    }

    himc = ImmGetContext(input_state->ime_target_window);
    if (himc == NULL) {
        return TRUE;
    }

    input_state->has_ime_context = TRUE;
    input_state->previous_ime_open = ImmGetOpenStatus(himc);
    input_state->has_conversion_state = ImmGetConversionStatus(
        himc,
        &input_state->previous_conversion_mode,
        &input_state->previous_sentence_mode
    );

    if (input_state->has_conversion_state) {
        DWORD halfwidth_alnum_mode = build_halfwidth_alnum_conversion_mode(
            input_state->previous_conversion_mode
        );
        ImmSetConversionStatus(himc, halfwidth_alnum_mode, input_state->previous_sentence_mode);
    }

    ImmSetOpenStatus(himc, FALSE);
    ImmReleaseContext(input_state->ime_target_window, himc);

    return TRUE;
}

void restore_ascii_input(const ASCII_INPUT_STATE* input_state) {
    HIMC himc;

    if (input_state == NULL || !input_state->has_ime_context || input_state->ime_target_window == NULL) {
        return;
    }

    himc = ImmGetContext(input_state->ime_target_window);
    if (himc == NULL) {
        return;
    }

    if (input_state->has_conversion_state) {
        ImmSetConversionStatus(
            himc,
            input_state->previous_conversion_mode,
            input_state->previous_sentence_mode
        );
    }

    ImmSetOpenStatus(himc, input_state->previous_ime_open);
    ImmReleaseContext(input_state->ime_target_window, himc);
}

HWND find_ime_target_window(HWND foreground_window, DWORD* target_thread_id) {
    DWORD thread_id = 0;

    if (foreground_window != NULL) {
        GUITHREADINFO gui_thread_info;

        thread_id = GetWindowThreadProcessId(foreground_window, NULL);
        ZeroMemory(&gui_thread_info, sizeof(gui_thread_info));
        gui_thread_info.cbSize = sizeof(gui_thread_info);

        if (thread_id != 0 && GetGUIThreadInfo(thread_id, &gui_thread_info) && gui_thread_info.hwndFocus != NULL) {
            if (target_thread_id != NULL) {
                *target_thread_id = thread_id;
            }
            return gui_thread_info.hwndFocus;
        }
    }

    if (target_thread_id != NULL) {
        *target_thread_id = thread_id;
    }
    return foreground_window;
}

DWORD build_halfwidth_alnum_conversion_mode(DWORD current_conversion_mode) {
    DWORD halfwidth_alnum_mode = current_conversion_mode;

    halfwidth_alnum_mode &= ~(IME_CMODE_NATIVE |
                              IME_CMODE_KATAKANA |
                              IME_CMODE_FULLSHAPE |
                              IME_CMODE_ROMAN |
                              IME_CMODE_CHARCODE |
                              IME_CMODE_HANJACONVERT |
                              IME_CMODE_SOFTKBD |
                              IME_CMODE_NOCONVERSION |
                              IME_CMODE_EUDC |
                              IME_CMODE_SYMBOL |
                              IME_CMODE_FIXED);

    return halfwidth_alnum_mode;
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
