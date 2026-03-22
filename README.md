# sendinput-timestamp-output
タイムスタンプを取得してテキスト入力領域に直接出力するWindows用アプリケーション

## ビルド用バッチファイル or 実行ファイル（.exe） 置き場
https://github.com/imagawatatsuya/sendinput-timestamp-output/releases

## 収録バージョン
### 日本語曜日版（従来版）
- ソース: `get_time_sendinput_win_wchar.c`
- ビルド: `compile_get_time_sendinput_win_wchar.bat`
- 出力例: `2026/03/22(日) 14:35:41`
- 特徴: 日本語曜日あり。従来仕様を維持。

### ASCII高速版（IME対策を優先）
- ソース: `get_time_sendinput_ascii_fast.c`
- ビルド: `compile_get_time_sendinput_ascii_fast.bat`
- 出力例: `2026/03/22(sun) 14:35:41`
- 特徴: 英数字と記号だけに限定し、標準 `Edit` / `RichEdit` 系コントロールには `EM_REPLACESEL` で直接挿入し、それ以外ではIME状態を半角英数寄りにしつつASCII文字そのものをUnicode入力で送る高速版。

## 使い方
### 日本語曜日版
`get_time_sendinput_win_wchar.exe`をダブルクリックすると、テキスト入力領域にタイムスタンプ（半角英数字）が自動的に出力される。
特に、低スペック（Core i5-8350U メモリ8GB以下）のWindowsパソコンにおいて効果を発揮する。

### ASCII高速版
`get_time_sendinput_ascii_fast.exe`をダブルクリックすると、テキスト入力領域にASCII形式のタイムスタンプが自動的に出力される。
ブラウザのURL欄、日本語テキストエディタなど、IMEの影響を受けやすい入力先を意識した派生版。標準 `Edit` / `RichEdit` 系では直接挿入を優先し、従来の1文字ずつの送信はフォールバックとして残している。

初回の実行時にはダイアログが表示されるので **「詳細情報」** という部分をクリックする

![image](https://github.com/user-attachments/assets/5f6bd6ce-de10-4632-b9ed-18a5cdb5e393)

初回のみ、許可を求めるダイアログが表示されます。Windowsの仕様です。

**「実行」** ボタンをクリックする

![image](https://github.com/user-attachments/assets/e4ba3b29-0fce-46d4-aaee-6015ae06f63f)

**2度目の実行からはダイアログは表示されません。**

## 前提
テキストエディタやMicrosoft Word等における利用は想定していない。マクロによるタイムスタンプ生成を推奨する。
ただしASCII高速版は、日本語曜日版よりもIME設定の影響を受けにくく、対応できる入力先では半角英数寄りのIME状態へ一時的に切り替えたうえでASCII文字そのものを送信する。
また、標準 `Edit` / `RichEdit` 系コントロールではIME制御や `SendInput` を使わずに一括挿入する。

## 動作確認済み
- Windwos標準のメモ帳
- Google Chrome
- Micorosoft Edge

### 非推奨 - マクロが順当
- 主要テキストエディタ（秀丸エディタ、Meryなど）
- Microsoft Word
- Micorosoft Excel
- Mozila Firefox
- etc……

## タイムスタンプをクリップボードにコピーするだけのアプリ
https://github.com/imagawatatsuya/get-timestamp-clipboard-exe
- 実行後にペーストするためIME設定は関係なし。
- 「貼り付け」というひと手間が必要だが、汎用性は高い。
