// get_time_sendinput_win_wchar.c (ワイド文字曜日・フォーマット版, 先にAlt+Tab)
// ★ソースファイルは UTF-8 で保存しても Shift_JIS で保存しても動作するはずです★

#include <stdio.h>  // snprintf (エラーメッセージ用), swprintf_s (時刻フォーマット用)
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <wchar.h>  // wchar_t, swprintf_s など

// --- 定数 ---
#define WIDE_BUFFER_SIZE 256 // ワイド文字バッファサイズ
#define INPUT_WAIT_MS 1
#define STARTUP_WAIT_MS 100
#define ALT_TAB_WAIT_MS 50
#define POST_ALT_TAB_WAIT_MS 500

// --- グローバル変数 ---
// ★ ワイド文字列で曜日を定義 ★
const wchar_t* wday_jp_w[] = {L"日", L"月", L"火", L"水", L"木", L"金", L"土"};

// --- プロトタイプ宣言 ---
void send_wide_string_as_unicode_input(const wchar_t* wstr); // ワイド文字列を受け取る関数
void show_error_message(const char* message); // エラーメッセージは char* のまま
void send_alt_tab();

// --- エントリーポイント ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#if STARTUP_WAIT_MS > 0
    Sleep(STARTUP_WAIT_MS);
#endif

    // --- 先に Alt+Tab を送信 ---
    send_alt_tab();

    // --- Alt+Tab によるウィンドウ切り替えとフォーカス安定を待つ ---
#if POST_ALT_TAB_WAIT_MS > 0
    Sleep(POST_ALT_TAB_WAIT_MS);
#endif

    // --- Alt+Tab後のアクティブウィンドウに対して処理を開始 ---
    time_t now;
    struct tm local_time;
    wchar_t time_string_w[WIDE_BUFFER_SIZE]; // ★ ワイド文字列バッファ
    int written;

    // --- 現在時刻取得 ---
    time(&now);
    if (localtime_s(&local_time, &now) != 0) {
        show_error_message("Failed to get local time after Alt+Tab.");
        return 1;
    }

    // --- ★ ワイド文字列として時刻文字列をフォーマット ---
    // swprintf_s (セキュリティ強化版) を使用
    written = swprintf_s(time_string_w, WIDE_BUFFER_SIZE,
                         L"%04d/%02d/%02d(%ls) %02d:%02d:%02d", // %s を %ls に変更
                         local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,
                         wday_jp_w[local_time.tm_wday], // ワイド文字の曜日配列を使用
                         local_time.tm_hour, local_time.tm_min, local_time.tm_sec);

    // Microsoft 系以外のコンパイラで swprintf_s がない場合は swprintf を使用
    // written = swprintf(time_string_w, WIDE_BUFFER_SIZE,
    //                    L"%04d/%02d/%02d(%ls) %02d:%02d:%02d",
    //                    local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,
    //                    wday_jp_w[local_time.tm_wday],
    //                    local_time.tm_hour, local_time.tm_min, local_time.tm_sec);

    if (written < 0) {
        show_error_message("Failed to format wide time string after Alt+Tab.");
        return 1;
    }

    // --- ★ SendInput でワイド文字列を直接送信 ---
    send_wide_string_as_unicode_input(time_string_w);

    return 0;
}

// --- ワイド文字列を Unicode キー入力として送信する関数 ---
void send_wide_string_as_unicode_input(const wchar_t* wstr) {
    // ★ MultiByteToWideChar は不要になった ★
    INPUT input[2];
    memset(input, 0, sizeof(input));

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    for (int i = 0; wstr[i] != L'\0'; ++i) {
        // サロゲートペアは考慮しないシンプルな実装 (タイムスタンプでは通常不要)
        input[0].ki.wScan = wstr[i];
        input[1].ki.wScan = wstr[i];

        // KeyDownとKeyUpを送信
        if (SendInput(2, input, sizeof(INPUT)) == 0) {
            char error_buf[128]; // エラーメッセージは char で作成
            snprintf(error_buf, sizeof(error_buf),
                     "SendInput failed for char U+%04X (Error: %lu)",
                     (unsigned int)wstr[i], GetLastError());
            show_error_message(error_buf);
            // ここで return する場合、呼び出し元での後続処理（Alt+Tabなど）は実行されない
            return;
        }

    #if INPUT_WAIT_MS > 0
        Sleep(INPUT_WAIT_MS);
    #endif
    }
    // 正常終了
}

// --- エラーメッセージをメッセージボックスで表示する関数 ---
// (変更なし - MessageBoxA を使うので char* のまま)
void show_error_message(const char* message) {
    MessageBoxA(NULL, message, "Get Time SendInput Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
}

// --- Alt+Tab を送信する関数 ---
// (変更なし)
void send_alt_tab() {
    INPUT inputs[4] = {0};
    inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = VK_MENU;
    inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = VK_TAB;
    inputs[2].type = INPUT_KEYBOARD; inputs[2].ki.wVk = VK_TAB; inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD; inputs[3].ki.wVk = VK_MENU; inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &inputs[0], sizeof(INPUT)); Sleep(ALT_TAB_WAIT_MS);
    SendInput(1, &inputs[1], sizeof(INPUT)); Sleep(ALT_TAB_WAIT_MS);
    SendInput(1, &inputs[2], sizeof(INPUT)); Sleep(ALT_TAB_WAIT_MS);
    SendInput(1, &inputs[3], sizeof(INPUT));
    // エラーチェックは省略
}