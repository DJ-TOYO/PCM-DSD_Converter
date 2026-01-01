「PCM-DSD_Converter改」はserieril氏作DSD変換ソフト「PCM-DSD_Converter V1.0.0.11」をベースに
内部64Bit Float演算、FIR(高精度)/IIR(低精度)フィルタによるアップサンプリング、
ノイズシェーピング（量子化ノイズ高周波移動）による1BIT変換、フロントエンド(GUI)などの
基本性能・機能はそのままに機能拡張、仕様変更を行った改良版となります。
生成された音の特性、性能についてはserieril氏のHPを参照願います。


主な変更内容について
・WAVだけでなくFLAC/ALAC/RF64/SONY WAVE64に対応
・ID3V2.3形式タグ出力に対応 ※WAV、SONY WAVE64(.W64)、RF64は下記「・タグ」を参照
・DSD出力フォーマットを.DFFから.DSFに変更、これによりID3V2タグの使用が可能
・オーディオ機器との再生互換性向上の為、48KHz系PCMをDSD2.8MHzの倍数に準拠したサンプリングレートで出力可能
・PCMサンプリングレートからDSDサンプリングレート自動選択機能追加
・ギャップレス再生のDSF出力に対応
・RF64/SONY WAVE64ファイルを使用することで4GByteサイズ以上のDSDも作成可能
・発振（クリップオーバー）対策の為、ゲインを下げるゲイン調整、制限機能を追加
・音量を正規化するノーマライズ機能を追加
・最大音量を均一にするためのゲイン制限機能を追加 ※ゲイン調整との掛け合わせも可能
・オプション設定、ウィンドウ位置を保持する機能を追加
・ショートカットへのドラッグ＆ドロップに対応
・DFFをDSF形式にネイティブ変換する機能追加
・その他機能追加、一部仕様変更など


【動作環境】
・Windows 10/11 64BIT版
  ※32BITは対応しておりません
  ※Windows7でもVisualStudio2017/2019ランタイムをインストール擦る事で動作する可能性がありますが、環境がない為未確認です。
    https://support.microsoft.com/ja-jp/help/2977003/the-latest-supported-visual-c-downloads


【仕様】
■入力フォーマット
・ファイル
  WAVE(.wav)
  RF64(.rf64)
  SONY WAVE64(.w64)
  FLAC(.flac)
  ALAC(.m4A)
  DSDIFF(.dff)
・サンプリングレート
  44.1/48/88.2/96/176.4/192/352.8/384/705.6/768KHz
  ※DSDIFFは制限がありません。
・ビットレート
  WAV        :16/20/24/32Bit Integer
              32/64Bit Float
  RF64       :16/20/24/32Bit Integer
              32/64Bit Float
  SONY WAVE64:16/20/24/32Bit Integer
              32/64Bit Float
  FLAC       :16/24/32Bit Integer
  ALAC       :16/20/24/32Bit Integer
  DSDIFF     :1Bit
・チャンネル
  ステレオ(2Ch)のみ
・タグ
  WAV
  FLAC
  ALAC ※カバー画像は、JPEG/PNGのみサポート
  ※RF64/SONY WAVE64ファイルはタグ未対応

■出力DSDフォーマット
・ファイル
  DSFファイル
・サンプリングレート
  DSD64(2.8/3.0MHz)
  DSD128(5.6/6.1MHz)
  DSD256(11.2/12.2MHz)
  DSD512(22.5/24.5MHz)
  DSD1024(45.1/49.1MHz)
  DSD2048(90.3/98.3MHz)
  ※DSDIFFは、元のサンプリングレートが引き継がれます。
