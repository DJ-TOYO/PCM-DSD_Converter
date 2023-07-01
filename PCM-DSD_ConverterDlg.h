
// PCM-DSD_ConverterDlg.h : ヘッダー ファイル
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include <string>
#include"ProgressDlg.h"
#include "fftw3.h"
#include <omp.h>
#include <fstream>
#include <complex>
#include "FLAC/stream_decoder.h"
#include "IniFile.h"
#include "DffToDsfConv.h"
#include "AlacDecode.h"

using namespace std;

#define ALBUM_MODE_WAV_TAG_ENABLE	1
#define GAIN_LIMIT_DB_OFFSET		10	// ゲイン制限の開始dB

typedef enum
{
	EN_LIST_COLUMN_FILE_NAME = 0,				//  0:ファイル名
	EN_LIST_COLUMN_TITLE,						//  1:タイトル
	EN_LIST_COLUMN_ARTIST,						//  2:アーティスト
	EN_LIST_COLUMN_ALBUM,						//  3:アルバム
	EN_LIST_COLUMN_TRACK_NO,					//  4:トラックNo
	EN_LIST_COLUMN_DISC_NO,						//  5:Disc No
	EN_LIST_COLUMN_DISC_TOTAL,					//  6:Disc 数
	EN_LIST_COLUMN_ALBUM_ARTIST,				//  7:アルバムアーティスト
	EN_LIST_COLUMN_SAMPLING_RATE,				//  8:サンプリングレート
	EN_LIST_COLUMN_BIT,							//  9:ビット数
	EN_LIST_COLUMN_LENGTH,						// 10:長さ
	EN_LIST_COLUMN_SAMPLING_COUNT,				// 11:サンプル数
	EN_LIST_COLUMN_PATH,						// 12:ファイルパス
	EN_LIST_COLUMN_EXT,							// 13:拡張子

	EN_LIST_COLUMN_MAX,							// 13:最大項目数
}EN_LIST_COLUMN;

// 拡張子種別
typedef enum
{
	EXT_TYPE_UNKNOW = 0,	// 不明
	EXT_TYPE_WAV,			// .wav
	EXT_TYPE_WAVE64,		// .w64 ※SONY WAVE64
	EXT_TYPE_FLAC,			// .flac
	EXT_TYPE_ALAC,			// .m4a ※ALACのみ
	EXT_TYPE_DFF,			// .dff ※DSDIFF
	EXT_TYPE_MAX			// 拡張子種別最大数
}EXT_TYPE;

typedef struct {
	UINT32 nLength;
	BYTE *pData;
}STPICTUREINFO, *PSTPICTUREINFO;

// FLAC Comment Field Recommendations
// https://www.xiph.org/vorbis/doc/v-comment.html
// https://www.legroom.net/2009/05/09/ogg-vorbis-and-flac-comment-field-recommendations
typedef struct {
	CString strAlbum;			// アルバム
	CString strArtist;			// アーティスト
	CString strAlbumArtist;		// アルバムアーティスト
	CString strPublisher;		// 発行元
	CString strCopyright;		// 著作権
	CString strDiscnumber;		// Disc No
	CString strDisctotal;		// Disc Total
	CString strIsrc;			// 国際標準録音(ISRC)コード
	CString strUpn;				// Universal Product Number or EAN
	CString strLabel;			// ラベル
	CString strLabelNo;			// ラベルNo
	CString strLicense;			// ライセンス
	CString strOpus;			// オーパス
	CString strSourcemedia;		// ソースメディア
	CString strTitle;			// 曲タイトル
	CString strTracknumber;		// トラックNo
	CString strTracktotal;		// トラックTotal
	CString strVersion;			// バージョン
	CString strEncodedBy;		// エンコード者
	CString strEncoded;			// エンコード設定
	CString strLyrics;			// 歌詞
	CString strWebsite;			// Web Site
	CString strComposer;		// 作曲家
	CString strArranger;		// 編曲
	CString strLyricist;		// 作詞家
	CString strAuthor;			// 著者
	CString strConductor;		// 指揮者
	CString strPerformer;		// 演奏者
	CString strEnsemble;		// 演奏グループ
	CString strPart;			// トラックのパート
	CString strPartnumber;		// トラックのパートNo
	CString strGenre;			// ジャンル
	CString strDate;			// 日付
	CString strLocation;		// 収録場所
	CString strComment;			// コメント
	CString strOrganization;	// 組織
	CString strDescription;		// 説明
	CString strContact;			// コンタクト
	STPICTUREINFO stPicture[FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED];
} STFLAC_COMMENT, *PSTFLAC_COMMENT;

typedef struct {
	CString strTALB;			// アルバム
	CString strTPE1;			// アーティスト
	CString strTPE2;			// バンド※アーティスト
	CString strTIT2;			// 曲タイトル
	CString strTRCK;			// トラックNo　※ ISO-8859-1
	CString strTPOS;			// セット中の位置 ※CD枚数 2枚組なら「1/2」「2/2」など
	CString strTCON;			// ジャンル
	CString strCOMM;			// コメント
	CString strTCOM;			// 作曲家
	CString strTYER;			// 年　※ ISO-8859-1
	CString strTSSE;			// ソフトウェア
	CString strTCOP;			// 著作権
	CString strTENC;			// エンコードした人
	CString strTEXT;			// 作詞者
	CString strTXXX;			// ユーザー定義
	STPICTUREINFO stPicture[FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED];
} STID3TAGINFO, *PSTID3TAGINFO;

typedef struct {
	char cFlameID[4];			// フレームID
	DWORD dwSize;				// サイズ
	WORD wFlag;					// フラグ
} STID3_TAG_FLAME_HEADER, *PSTID3_TAG_FLAME_HEADER;

typedef struct {
	CString strOrgFile;				// ソースファイル名
	UINT64 iStartPos;				// 64BIT PCM読込みオフセット位置
	UINT64 iOrigDataSize;			// ソースデータサイズ
	UINT nTagMode;					// 0:TAG無し 1:FLAC TAGあり
	STID3TAGINFO stID3tag;			// TAG
}ST_64BIT_DATA_INDEX;

// GUID
typedef enum {
	W64_RIFF = 0,		// "RIFF"
	W64_LIST,			// "LIST"
	W64_WAVE,			// "WAVE"
	W64_FMT,			// "FMT "
	W64_FACT,			// "FACT"
	W64_DATA,			// "DATA"
	W64_LEVL,			// "LEVL"
	W64_JUNK,			// "JUNK"
	W64_BEXT,			// "BEXT"
	W64_MARKER,			// MARKER
	W64_SUMMARYLIST,	// SUMMARYLIST
	W64_GUID_MAX,
}EN_W64_GUID;

#pragma pack(push, 1)
typedef struct {
	GUID Guid;
	ULONGLONG ullSize;
}ST_W64_CHUNK;

typedef struct { 
	GUID Guid;
	ULONGLONG ullSize;
	DWORD dwMarkerCount;
} CUE64_HEADER; 
#pragma pack(pop)

typedef struct {
	ULONGLONG ullSampleLength;
}ST_W64_FACT;

typedef struct {
	ULONGLONG ullSampleLength;
}ST_W64_DATA;

#pragma pack(push, 1)
typedef struct {
	DWORD dwId;
	DWORD dwPadding1;
	LONGLONG llPosition;
	LONGLONG llLength;
	DWORD dwcbName;		// never will more than 32 bits be needed for size
	DWORD dwPadding2;
}CUE64;
#pragma pack(pop)

// 振幅データ
typedef struct {
	double dAmpPeak[2];				// 振幅ピーク値 [0]:Ch L [1]:Ch R
	double dAmpSum[2];				// 振幅積算値 [0]:Ch L [1]:Ch R
	UINT64 ullAmpSumCnt[2];			// 振幅積算数 [0]:Ch L [1]:Ch R
}ST_AMP_POWER,*PST_AMP_POWER;

// CPCMDSD_ConverterDlg ダイアログ
class CPCMDSD_ConverterDlg : public CDialogEx
{
	// コンストラクション
public:
	CPCMDSD_ConverterDlg(CWnd* pParent = NULL);	// 標準コンストラクター
//	string flag_Bottun = "";
	DWORD m_dwConvertProcessFlag;
	// ダイアログ データ
	enum { IDD = IDD_PCMDSD_CONVERTER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート

private:
	ProgressDlg m_dProgress;
	static UINT __cdecl WorkThread(LPVOID pParam);
	// 実装
protected:
	HICON m_hIcon;
	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void ListInit();
	void ListRegistDisp();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	// リストビューカラムデフォルト幅
	int m_nColWidthDef[EN_LIST_COLUMN_MAX] = {
		150,				// [0]:ファイル名
		150,				// [1]:タイトル(トラック名)
		110,				// [2]:アーティスト
		150,				// [3]:アルバム
		 70,				// [4]:トラックNo
		 70,				// [5]:ディスクNo
		 70,				// [6]:ディスク数
		110,				// [7]:アルバムアーティスト
		100,				// [8]:サンプリングレート
		70, 				// [9]:ビット数
		50, 				// [10]:長さ
		75, 				// [11]:サンプル数
		150,				// [12]:ファイルパス
		70					// [13]:拡張子
	};

	CIniFile m_IniFile;					// INIファイル

	CString m_strOutputPath;			// 出力先パス
	int m_nDSDidx;						// DSDサンプリング選択
	int m_nPrecisionIdx;				// 精度選択
	int m_nGainLevelIdx;				// ゲイン調整選択
	int m_nGainLimitIdx;				// ゲイン制限選択
	int m_NormalizeFlag;				// ノーマライズ 0:無効 1:有効
	int m_NormalizeFlagLast;			// 起動時のノーマライズ 0:無効 1:有効
	int m_CrossGainLevel;				// ゲイン調整掛け合わせ 0:無効 1:有効
	int m_CrossGainLevelLast;			// 起動時のゲイン調整掛け合わせ 0:無効 1:有効
	int m_Pcm48KHzEnableDsd3MHzFlag;	// 48KHz系PCMは、DSD3.0MHzとして出力する　0:無効 1:有効
	int m_DsfOverWriteFlag;				// DSF上書きモード 0:しない 1:する
	int m_radioGainMode;				// 起動時のゲインモード ※現在のゲインモードはm_radioGainModeDdv(DDV変数)を参照する事。

	int m_DSDsamplingRateCurSel;		// DSD変換選択位置
	int m_nCompletePowerCrl;			// 変換完了電源制御

	CString m_SrcFileName;				// ソースファイル名
	int m_SrcPcmSamplingrate;			// ソースサンプリング周波数
	int m_SrcPcmBit;					// ソースBIT
	unsigned __int64 m_OrigDataSize;	// ソースデータサイズ
	unsigned int m_DSD_Times;			// DSD倍数(64/128/256/1024/2048)

	int m_Pcm48KHzEnableDsd3MHzEnable;	// 48KHz系PCMは、DSD3.0MHzとして出力する　0:無効 1:有効
	int m_NormalizeEnable;				// ノーマライズ 0:無効 1:有効
	int m_ClipErrNormalizeEnable;		// ノーマライズ 0:無効 1:有効 ※クリップエラーが発生した場合に強制的にノーマライズを行い補正する
	int m_DsfOverWriteEnable;			// DSF上書きモード 0:無効 1:有効
	int m_SequentialProcessEnable;		// 連続データ処理 0:無効 1:有効
	int m_DsdClipOverChkEnable;			// DSD変換クリップオーバーチェック 0:無効 1:有効
	double m_DsdClipOverLimit;			// DSD変換クリップオーバー最大値

	STFLAC_COMMENT m_stFlacComm;		// FLAC COMMENT
	STALACTAG m_stAlacTag;				// ALAC TAG
	STID3TAGINFO m_stID3tag;			// ID3タグ出力用情報
	UINT m_nTagMode;					// 0:TAG無し 1:FLAC TAGあり
	CString m_AppName;					// アプリケーション名
	CString m_strEncodedBy;				// エンコーダー者

	double m_dVolGain;					// 音量調整
	double m_dGain;						// ゲイン

	UINT64 m_nDsdClipOverCnt;			// DSD変換中にクリップオーバーしたトータルカウント 0:エラーなし 1～:クリップオーバー
	BOOL m_bDsdClipOver;				// 現在のファイルがDSD変換中にクリップオーバーした TRUE:エラー FALSE:正常

	// DSD変換マップ
	int m_nDsdSampling44100;			// 44100 or 48000
	int m_nDsdSampling88200;			// 88200  or 96000
	int m_nDsdSampling176400;			// 176400 or 192000
	int m_nDsdSampling352800;			// 352800 or 384000
	int m_nDsdSampling705600;			// 705600 or 768000
	int m_nDsdSampling44100Diff;		// 44100 or 48000 ※変更チェック用
	int m_nDsdSampling88200Diff;		// 88200  or 96000 ※変更チェック用
	int m_nDsdSampling176400Diff;		// 176400 or 192000 ※変更チェック用
	int m_nDsdSampling352800Diff;		// 352800 or 384000 ※変更チェック用
	int m_nDsdSampling705600Diff;		// 705600 or 768000 ※変更チェック用

	DWORD dwCompletionNoticeFlag;		// 完了通知 00B:なし 01B:音を鳴らす 10B:画面を復帰させる

	RECT m_InitPos;
	int m_ShowCmd;
	int m_nWindowGetPosMode;			// 前回ウィンドウ位置の取得方法 0:GetSystemMetricsによる仮想スクリーンサイズ 1:MonitorFromRectによる近いモニタ
	BOOL m_bWindowMonitorBorder;		// 前回ウィンドウ位置がモニタ外の場合　0:何もしない　1:一番近いモニタのモニタ内に補正する

//	double m_amp_peak[2];				// 振幅ピーク値 [0]:Ch L [1]:Ch R
//	double m_amp_sum[2];				// 振幅積算値 [0]:Ch L [1]:Ch R
//	double m_amp_sum_cnt[2];			// 振幅積算数 [0]:Ch L [1]:Ch R
	ST_AMP_POWER stAmp;					// 振幅データ
	ST_AMP_POWER stAmpPeakdB;			// 振幅データピークデシベル用
	double m_dAvgPeakdB[2];				// 平均ピークdB [0]:Ch L [1]:Ch R
	double m_AveragedB[3];				// 平均dB [0]:Ch L [1]:Ch R [2]:Ch L+R
	double m_LimitdB;					// 制限dB
	double m_DiffdB;					// 制限db差分

	UINT64 m_fillsize;
	CArray <ST_64BIT_DATA_INDEX> m_DataIdxArray;

	CString m_strSeqModeTempFile;		// アルバム実行(連続変換)時のテンポラリファイル
	CString m_strSrcAlbumPath;			// アルバム実行(連続変換)時のソースディレクトリ

	CDffToDsfConv m_DffToDsfObj;		// DFF to DSF変換
	CAlacDecode m_AlacDecodeObj;		// ALACデコード

	int m_nListRegistCnt;				// リストコントロール登録件数

public:
	virtual void OnCancel();
	CMFCListCtrl m_lFileList;
	CButton m_bAllRun;
	CButton m_bAllListDelete;
	CButton m_bRun;
	CButton m_bListDelete;
	CStatic m_scPrecision;
	CComboBox m_ccPrecision;
	CStatic m_scPath;
	CEdit m_ecPath;
	CButton m_bcPath;
	CComboBox m_cSamplingRate;
	CStatic m_sSamplingRate;
	CString m_evPath;
	CString m_strAlbumOutDir;
	CString m_strAlbumArtistOutDir;
	CString m_strDiscNoOutDir;
	CComboBox m_combVol;
	CToolTipCtrl m_tooltipSamplingRate;

	CStringArray m_strCmdLineArray;		// コマンドライン

	// Version取得
	VS_FIXEDFILEINFO GetVersionInfo();
	//全て実行
	afx_msg void OnBnClickedAllrun();
	//全て削除
	afx_msg void OnBnClickedAlllistdelete();
	//実行
	afx_msg void OnBnClickedRun();
	//削除
	afx_msg void OnBnClickedListdelete();
	//参照
	afx_msg void OnBnClickedPathcheck();
	//ファイル/ディレクトリがドロップ&ドラッグ
	afx_msg void OnDropFiles(HDROP hDropInfo);
	//閉じる
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	// INIファイル保存
	void SaveIniFilee();
	//サイズ変更
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//Wavファイルがドロップされた時の初動
	afx_msg void WAV_FileRead(TCHAR *FileName, BOOL bErrMsgEnable = TRUE);
	//ディレクトリの再帰的検索
	afx_msg void DirectoryFind(TCHAR *DirectoryPath);
	// 有効サンプリングレートチェック
	BOOL IsSamplingRate(DWORD dwSamplingRate);
	// 有効BIT深度チェック
	BOOL IsBitDepth(DWORD dwBit);
	// WAVサンプリングレート取得
	bool GeyWavSamplePerSec(TCHAR *filepath, int *pnSamplePerSec);
	//WAVファイルチェック及びメタデータ読み取り
	afx_msg bool WAV_Metadata(TCHAR *filepath, CString *metadata);
	afx_msg bool WAV_Metadata(TCHAR *filepath, CString *metadata, int *pnSamplePerSec);
	// SONY WAVE64サンプリングレート取得
	bool CPCMDSD_ConverterDlg::GeyWave64SamplePerSec(TCHAR *filepath, int *pnSamplePerSec);
	//SONT WAVE64(W64)ファイルチェック及びメタデータ読み取り
	afx_msg bool Wave64_Metadata(TCHAR *filepath, CString *metadata);
	afx_msg bool Wave64_Metadata(TCHAR *filepath, CString *metadata, int *pnSamplePerSec);
	//Flacファイルチェック及びメタデータ読み取り
	afx_msg bool FLAC_Metadata(TCHAR *filepath, CString *metadata);
	// ALAC(.m4a)ファイルチェック及びメタデータ読み取り
	bool ALAC_Metadata(TCHAR *filepath, CString *metadata);
	// DFFファイルチェック及びメタデータ読み取り
	bool DFF_Metadata(TCHAR *filepath, CString *metadata);
	//Flacタグデータ読み取り
	afx_msg bool FLAC_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag, BOOL *pbEnable);
	//  ALACファイルタグデータ読み取り
	bool ALAC_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag);
	//  WAVファイルタグ(RIFF)データ読み取り
	bool WAV_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag, BOOL *pbEnable);
	// WAVファイル用タグデータ設定
	afx_msg UINT SetTagdataForWav(TCHAR *filepath, int nTrackNo, int nTrackTotal, PSTID3TAGINFO pstID3v2Tag);
	// FLAC COMMENTタグ初期化
	void FlacCommentInit(PSTFLAC_COMMENT pstTag, int mode);
	// ID3v2タグ情報初期化
	void ID3v2TagInfoInit();
	void ID3v2TagInfoClear(STID3TAGINFO *pstID3Tag, int mode = 0);
	void ID3v2TagInfoDataIdxArrayClear();
	// ID3v2タグソフトウェア設定
	void SetID3v2SoftwareTag(CString strSoftware, PSTID3TAGINFO pstID3v2Tag);
	// ID3v2タグエンコーダー者設定
	void SetID3v2EncodedByTag(CString strEncodedBy, PSTID3TAGINFO pstID3v2Tag);
	// ID3v2タグユーザー定義設定
	void SetID3v2UserTag(CString strUser, PSTID3TAGINFO pstID3v2Tag);
	// ID3v2タグユーザー定義にオリジナルフォーマット設定
	void SetID3v2UserTagOriginalFormat(int nMode, CString strFile, DWORD dwSamplingRate, DWORD dwBitDepth, PSTID3TAGINFO pstID3v2Tag);
	// ログインユーザー取得
	CString GetUserName();
	// Flacタグ情報 to ID3v2タグ情報コピー
	UINT FlacTagToID3v2Tag(PSTFLAC_COMMENT pstFlacTag, PSTID3TAGINFO pstID3v2Tag);
	// ID3v2タグサイズ
	afx_msg DWORD GetID3V2Length();
	// TTAG作成
	DWORD MakeTtag(char *strID, int nStrCode, CString strTtag, BYTE *pBuff);
	// CTAG作成
	DWORD MakeCtag(char *strID, CString strCtag, BYTE *pBuff);
	// APIC TAG作成
	DWORD MakeAPICtag(BYTE *pBuff);
	// Syncsafe Integerサイズ作成
	afx_msg void MakeSynchsafeIntegerSize(DWORD dwSize, unsigned char size[4]);
	// フレームサイズ取得
	void GetFrameSize(DWORD dwSize, BOOL bSynchsafeIntegerSize, unsigned char size[4]);
	//PCM-DSD変換の管理
	afx_msg bool WAV_ConvertProc(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile, bool bLastDataFlag = true, int mode = 0);
	// PCM-DSD変換
	afx_msg bool WAV_Convert(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile);
	// PCM-DSD連続データ変換
	afx_msg bool WAV_ConvertSequential(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile, bool bLastDataFlag);
	// DFF-DSF変換
	afx_msg bool DFFtoDSFconvert(TCHAR *filepath);
	// DFF-DSF変換進捗コールバック関数
	static BOOL CALLBACK DFFtoDSFconvertProgress(UINT nProgress, void *pPrm);
	//DSDIFF形式で書き込み
	afx_msg bool DSD_Write(FILE *LData, FILE *RData, FILE *WriteData, int number);
	//DSF形式で書き込み
	afx_msg bool DSD_WriteDSF(FILE *LData, FILE *RData, INT64 iSeekOffset, FILE *WriteData, int number);
	// DSFファイルID3v2タグ書き込み
	afx_msg bool DSFID3V2Write(FILE *WriteData);
	//読み書きデータ準備
	bool RequireWriteData(TCHAR *filepath, CString flag, TCHAR *WriteFilepath);
	bool RequireWriteData(TCHAR *filepath, CString flag, wchar_t *FileMode, FILE **WriteDatadsd, TCHAR *WriteFilepath = NULL, BOOL bOverWrite = TRUE);
	//一時ファイル削除
	afx_msg bool TrushFile(TCHAR *filepath, CString flag);
	//Wavファイルを64bit Float(double)化し、LR分離して一時ファイルとして書き出し
//	afx_msg bool TmpWriteData(TCHAR *filepath, FILE *tmpl, FILE *tmpr, int Times);
	afx_msg bool TmpWriteData(EXT_TYPE etExtType, TCHAR *filepath, FILE *tmpl, FILE *tmpr, int DSD_Times, unsigned int *Times, int number);
	// 振幅を解析　戻り値：デシベル変換値
	void PcmAnalysis(double amp_value, int ch, int nPcmSamplingRate);
	// 振幅ピーク値取得 ※左右Chの大きい方
	double GetPeakAmp();
	// 平均デシベルピーク取得 ※左右Chの大きい方
	double GetAvgPeakdB();
	// 平均ピークデシベル取得
	void CalcPeakdB();
	void CalcPeakdB(int ch);
	// 平均デシベル取得
	double GetAveragedB(double *dbL, double *dbR);
	// デシベルから倍率取得
	double GetdBtoPower(double ddB);
	// 振幅からデシベル取得
	double GetPowertodB(double dAmpPower);
	//PCM-DSD変換
	afx_msg bool WAV_Filter(FILE *UpSampleData, FILE *OrigData, unsigned int Times, omp_lock_t *myLock, int *pErr, int mode = 0);
	afx_msg bool WAV_FilterLight(FILE *UpSampleData, FILE *OrigData, unsigned int Times, int *pErr, int mode = 0);
	// オーバーサンプリング
	bool WAV_Oversampling(FILE *UpSampleData, FILE *OrigData, unsigned int Times, omp_lock_t *myLock);
	// 最終フォルダ名名取得
	CString GetLastPath(TCHAR *filepath);
	// ファイル名取得
	CString GetFileName(TCHAR *filepath);
	//フリーズ防止のためにスレッド作成
	void WorkThread();
	// 拡張子種別取得 
	EXT_TYPE GetExtType(CString strExt);
	// 出力ディレクトリ用アルバム＆アルバムアーティスト＆Disc NO設定
	void SetAlbumAndAlbumArtistOutputDir(CString strAlbum, CString strAlbumArtist, CString strDiscNo, CString strDiscTotal);
	// ファイル、ディレクトリに使えない文字を全角変換
	CString ConvertInvalidFileName(CString strText);
	// 完了通知処理
	void CompletionNotice();
	// 変換完了時の電源制御
	BOOL CompletePowerControl(DWORD dwMode);
	// フルパスからディレクトリパスだけ取得
	CString GetDirectoryPath(TCHAR *pstrText);
	// アルバム実行アルバムファイル数取得
	int GetAlbumCount();
	// DSDサンプリングレート選択から変換DSD取得
	unsigned int GetDsdSamplingrateComboBoxToDsdTimes(int nSamplePerSec);
	//処理中はボタンなど無効に
	void Disable();
	//処理が終わったらボタンなど有効化
	void Enable();
	//F1ヘルプ無効化
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//FFTプラン初期化
	void FFTInit(fftw_plan *plan, unsigned int fftsize, int Times, double *fftin, fftw_complex *ifftout);
	void iFFTInit(fftw_plan *plan, unsigned int fftsize, int Times, fftw_complex *ifftin, double *fftout);
	// FLACデコード
	bool FlacDecode(TCHAR *psrcfile, TCHAR *pdecodefile, int decodefilebuffsize);

	static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
	static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
	static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

	// ALACデコード
	bool AlacDecode(TCHAR *psrcfile, TCHAR *pdecodefile);

	bool GetTargetSample(unsigned int *pnDstSamplePerSec, unsigned int *pnDstBaseSamplePerSec, unsigned int nSrcSamplePerSec, int SearchMode);
	int GeDsdTimesWithtSamplePerSec(unsigned int nSrcSamplePerSec);

	void WakeupDisplay();
	void LoadSafeWindowPos(RECT *prcWnd, INT *pnShowCmd);
	void DesktopCenterWindow(RECT *prcWnd);

//	CStatic m_scVolume;
	CStatic m_staticAlbumTagSuffix;
	CEdit m_editAlbumTagSuffix;
	CString m_evAlbumTagSuffix;
	CString m_strAlbumTagSuffixCmp;
	afx_msg void OnLvnKeydownFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	CButton m_chkboxNormalize;
	afx_msg void OnBnClickedCheckNormalize();
	CButton m_bcPathClear;
	afx_msg void OnBnClickedButtonPathClear();
	CButton m_chkboxFileOverWrite;
	afx_msg void OnBnClickedCheckFileoverwrite();
	CButton m_btnAlbumRun;
	afx_msg void OnBnClickedAlbumrun();
	BOOL ChkListAlbumMode(int *pnErrRow);
	BOOL ChkListAlbumSeq(int nStartPos, int *pnErrRow);
	afx_msg void OnEnChangeEditalbumTagSuffix();
	afx_msg void OnHdnDividerdblclickFilelist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnKillfocusEditalbumTagSuffix();
	CStatic m_staticCompleteOption;
	CComboBox m_combCompleteOption;
	afx_msg void OnCbnSelchangeComboCompleteOption();
	BOOL m_radioGainModeDdv;
	CStatic m_groupGain;
	CButton m_radioGainMode1;
	CButton m_radioGainMode2;
	CComboBox m_combLimitVol;
	void GainModeGroupDisp();
	afx_msg void OnBnClickedRadioGainMode1();
	afx_msg void OnBnClickedRadioGainMode2();
	CButton m_chkboxCrossGainLevel;
	afx_msg void OnBnClickedCheckCrossGainlevel();
	afx_msg void OnBnClickedButtonSetting();
	CButton m_btnAutSetting;
	void AutoSettingBtnEnableProc();
	afx_msg void OnCbnSelchangeSamplingrate();
	CEdit m_editEncoderPerson;
	CStatic m_staEncoderPerson;
	CString m_evEncoderPerson;
	CStatic m_scRegistCnt;
	afx_msg void OnLvnInsertitemFilelist(NMHDR* pNMHDR, LRESULT* pResult);
};

static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
