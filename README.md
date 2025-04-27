# sendinput-timestamp-output
タイムスタンプを取得してテキスト入力領域に直接出力するWindows用アプリケーション

## ビルド用バッチファイル or 実行ファイル（.exe） 置き場
https://github.com/imagawatatsuya/sendinput-timestamp-output/releases

## 使い方
`get_time_sendinput_win_wchar.exe`をダブルクリックすると、テキスト入力領域にタイムスタンプ（半角英数字）が自動的に出力される。  
特に、低スペック（Core i5-8350U メモリ8GB以下）のWindowsパソコンにおいて効果を発揮する。

初回の実行時にはダイアログが表示されるので **「詳細情報」** という部分をクリックする  
![image](https://github.com/user-attachments/assets/5f6bd6ce-de10-4632-b9ed-18a5cdb5e393)  
初回のみ、許可を求めるダイアログが表示されます。Windowsの仕様です。

**「実行」** ボタンをクリックする  
![image](https://github.com/user-attachments/assets/e4ba3b29-0fce-46d4-aaee-6015ae06f63f)  
**2度目の実行からはダイアログは表示されません。**

## 前提
テキストエディタやMicrosoft Word等における利用は想定していない。マクロによるタイムスタンプ生成を推奨する。  
**IME設定の影響を受けてしまい、タイムスタンプが半角英数字としてが出力されないため。**

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
実行後にペーストするためIME設定は関係なし。  
「貼り付け」というひと手間が必要だが、汎用性は高い。
