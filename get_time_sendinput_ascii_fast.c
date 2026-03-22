#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <imm.h>

#define ASCII_BUFFER_SIZE 64
#define INPUT_WAIT_MS 1
#define STARTUP_WAIT_MS 100
#define ALT_TAB_WAIT_MS 50
#define POST_ALT_TAB_WAIT_MS 350
#define WINDOW_CLASS_NAME_SIZE 64

static const char* wday_ascii_lower[] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat"};

typedef struct {
    HWND ime_target_window;
    BOOL has_ime_context;
    BOOL has_conversion_state;
    BOOL previous_ime_open;
    DWORD previous_conversion_mode;
    DWORD previous_sentence_mode;
} ASCII_INPUT_STATE;

void send_alt_tab(void);
void show_error_message(const char* message);
void send_ascii_string_as_unicode_input(const char* str);
BOOL prepare_ascii_input(HWND foreground_window, ASCII_INPUT_STATE* input_state);
void restore_ascii_input(const ASCII_INPUT_STATE* input_state);
HWND find_ime_target_window(HWND foreground_window);
DWORD build_halfwidth_alnum_conversion_mode(DWORD current_conversion_mode);
BOOL try_insert_ascii_string_directly(HWND target_window, const char* str);
BOOL is_direct_insert_target_class(HWND target_window);
BOOL convert_ascii_to_wide_string(const char* str, WCHAR* wide_str, size_t wide_str_count);

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

    if (try_insert_ascii_string_directly(find_ime_target_window(foreground_window), time_string)) {
        return 0;
    }

    if (!prepare_ascii_input(foreground_window, &input_state)) {
        show_error_message("Failed to prepare ASCII input state.");
        return 1;
    }

    send_ascii_string_as_unicode_input(time_string);
    restore_ascii_input(&input_state);

    return 0;
}

BOOL prepare_ascii_input(HWND foreground_window, ASCII_INPUT_STATE* input_state) {
    HIMC himc;

    if (input_state == NULL) {
        return FALSE;
    }

    ZeroMemory(input_state, sizeof(*input_state));
    input_state->ime_target_window = find_ime_target_window(foreground_window);

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

HWND find_ime_target_window(HWND foreground_window) {
    if (foreground_window != NULL) {
        GUITHREADINFO gui_thread_info;
        DWORD thread_id = GetWindowThreadProcessId(foreground_window, NULL);

        ZeroMemory(&gui_thread_info, sizeof(gui_thread_info));
        gui_thread_info.cbSize = sizeof(gui_thread_info);

        if (thread_id != 0 && GetGUIThreadInfo(thread_id, &gui_thread_info) && gui_thread_info.hwndFocus != NULL) {
            return gui_thread_info.hwndFocus;
        }
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

BOOL try_insert_ascii_string_directly(HWND target_window, const char* str) {
    WCHAR wide_str[ASCII_BUFFER_SIZE];

    if (target_window == NULL || str == NULL || !is_direct_insert_target_class(target_window)) {
        return FALSE;
    }

    if (!convert_ascii_to_wide_string(str, wide_str, sizeof(wide_str) / sizeof(wide_str[0]))) {
        return FALSE;
    }

    SendMessageW(target_window, EM_REPLACESEL, TRUE, (LPARAM)wide_str);
    return TRUE;
}

BOOL is_direct_insert_target_class(HWND target_window) {
    WCHAR class_name[WINDOW_CLASS_NAME_SIZE];

    if (target_window == NULL) {
        return FALSE;
    }

    if (GetClassNameW(target_window, class_name, sizeof(class_name) / sizeof(class_name[0])) == 0) {
        return FALSE;
    }

    if (lstrcmpW(class_name, L"Edit") == 0 ||
        lstrcmpW(class_name, L"RichEdit") == 0 ||
        lstrcmpW(class_name, L"RICHEDIT50W") == 0) {
        return TRUE;
    }

    return FALSE;
}

BOOL convert_ascii_to_wide_string(const char* str, WCHAR* wide_str, size_t wide_str_count) {
    size_t i;

    if (str == NULL || wide_str == NULL || wide_str_count == 0) {
        return FALSE;
    }

    for (i = 0; str[i] != '\0'; ++i) {
        unsigned char ascii_char = (unsigned char)str[i];
        char error_buf[128];

        if (i + 1 >= wide_str_count) {
            show_error_message("ASCII string is too long for direct insertion.");
            return FALSE;
        }

        if (ascii_char > 0x7F) {
            snprintf(error_buf, sizeof(error_buf),
                     "Non-ASCII character 0x%02X found in ASCII direct insert path.",
                     ascii_char);
            show_error_message(error_buf);
            return FALSE;
        }

        wide_str[i] = (WCHAR)ascii_char;
    }

    wide_str[i] = L'\0';
    return TRUE;
}

void send_ascii_string_as_unicode_input(const char* str) {
    INPUT input[2];
    size_t i;

    if (str == NULL) {
        return;
    }

    ZeroMemory(input, sizeof(input));
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    for (i = 0; str[i] != '\0'; ++i) {
        unsigned char ascii_char = (unsigned char)str[i];
        char error_buf[128];

        if (ascii_char > 0x7F) {
            snprintf(error_buf, sizeof(error_buf),
                     "Non-ASCII character 0x%02X found in ASCII fast path.",
                     ascii_char);
            show_error_message(error_buf);
            return;
        }

        input[0].ki.wScan = ascii_char;
        input[1].ki.wScan = ascii_char;

        if (SendInput(2, input, sizeof(INPUT)) == 0) {
            snprintf(error_buf, sizeof(error_buf),
                     "SendInput failed for ASCII char 0x%02X (Error: %lu)",
                     ascii_char, GetLastError());
            show_error_message(error_buf);
            return;
        }

#if INPUT_WAIT_MS > 0
        Sleep(INPUT_WAIT_MS);
#endif
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
