// PCM-DSD_ConverterDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "ks.h"
#include "ksmedia.h"
#include "mmreg.h"
#include "PCM-DSD_Converter.h"
#include "PCM-DSD_ConverterDlg.h"
#include "afxdialogex.h"
#include "FLAC/all.h"
#include "share/compat.h"
#include "mp3infp\RiffSIF.h"
#include "IniFile.h"
#include "strconverter.h"
#include "AutoSettingDlg.h"
#include <stdio.h>
#include <string.h>
#include <mmsystem.h>

#pragma comment(lib, "version.lib")
//#ifdef _DEBUG
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
//#endif

using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#define ENABLE_OUTPUT_DFF	// DFF出力
#define ENABLE_OUTPUT_DSF	// DSF出力

#define GAIN_CONTROL_MODE	0		// ゲイン調整 0:DSD 1:PCM

#define DISPLAY_ON -1
#define ABSOLUTE_PATH_MAX	MAX_PATH		// 絶対パスの最大サイズ

#define AMP_LIMIT_MAX	1.0						// -1.0～1.0
//#define AMP_LIMIT_MAX	(1.0 - DBL_EPSILON)		// -(1.0 - DBL_EPSILON) ～ (1.0 - DBL_EPSILON)

GUID s_GUID[W64_GUID_MAX] = {
	{0x66666972,0x912E,0x11CF,0xA5,0xD6,0x28,0xDB,0x04,0xC1,0x00,0x00},		// W64_RIFF       :"RIFF"      { 66666972-912E-11CF-A5D6-28DB04C10000 }
	{0x7473696C,0x912F,0x11CF,0xA5,0xD6,0x28,0xDB,0x04,0xC1,0x00,0x00},		// W64_LIST       :"LIST"      { 7473696C-912F-11CF-A5D6-28DB04C10000 }
	{0x65766177,0xACF3,0x11D3,0x8C,0xD1,0x00,0xC0,0x4F,0x8E,0xDB,0x8A},		// W64_WAVE       :"WAVE"      { 65766177-ACF3-11D3-8CD1-00C04F8EDB8A }
	{0x20746D66,0xACF3,0x11D3,0x8C,0xD1,0x00,0xC0,0x4F,0x8E,0xDB,0x8A},		// W64_FMT        :"FMT "      { 20746D66-ACF3-11D3-8CD1-00C04F8EDB8A }
	{0x74636166,0xACF3,0x11D3,0x8C,0xD1,0x00,0xC0,0x4F,0x8E,0xDB,0x8A},		// W64_FACT       :"FACT"      { 74636166-ACF3-11D3-8CD1-00C04F8EDB8A }
	{0x61746164,0xACF3,0x11D3,0x8C,0xD1,0x00,0xC0,0x4F,0x8E,0xDB,0x8A},		// W64_DATA       :"DATA"      { 61746164-ACF3-11D3-8CD1-00C04F8EDB8A }
	{0x6C76656C,0xACF3,0x11D3,0x8C,0xD1,0x00,0xC0,0x4F,0x8E,0xDB,0x8A},		// W64_LEVL       :"LEVL"      { 6C76656C-ACF3-11D3-8CD1-00C04F8EDB8A }
	{0x6b6E756A,0xACF3,0x11D3,0x8C,0xD1,0x00,0xC0,0x4f,0x8E,0xDB,0x8A},		// W64_JUNK       :"JUNK"      { 6b6E756A-ACF3-11D3-8CD1-00C04f8EDB8A }
	{0x74786562,0xACF3,0x11D3,0x8C,0xD1,0x00,0xC0,0x4F,0x8E,0xDB,0x8A},		// W64_BEXT       :"BEXT"      { 74786562-ACF3-11D3-8CD1-00C04F8EDB8A }
	{0xABF76256,0x392D,0x11D2,0x86,0xC7,0x00,0xC0,0x4F,0x8E,0xDB,0x8A},		// W64_MARKER     :MARKER      { ABF76256-392D-11D2-86C7-00C04F8EDB8A }
	{0x925F94BC,0x525A,0x11D2,0x86,0xDC,0x00,0xC0,0x4F,0x8E,0xDB,0x8A},		// W64_SUMMARYLIST:SUMMARYLIST { 925F94BC-525A-11D2-86DC-00C04F8EDB8A }
};

template<class T>
T reverse_endian(T value)
{
	char * first = reinterpret_cast<char*>(&value);
	char* last = first + sizeof(T);
	std::reverse(first, last);
	return value;
}

// CPCMDSD_ConverterDlg ダイアログ
CPCMDSD_ConverterDlg::CPCMDSD_ConverterDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPCMDSD_ConverterDlg::IDD, pParent)
	, m_evPath(_T(""))
	, m_evAlbumTagSuffix(_T(""))
	, m_radioGainModeDdv(FALSE)
	, m_evEncoderPerson(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_strCmdLineArray.RemoveAll();

	memset(m_stFlacComm.stPicture, NULL, sizeof(m_stFlacComm.stPicture));
	memset(m_stID3tag.stPicture, NULL, sizeof(m_stID3tag.stPicture));

	m_bWindowMonitorBorder = FALSE;

	// FLAC のファイルをutf8モードにする
	flac_set_utf8_filenames(true);

	CString strIniFile;
	TCHAR Path[MAX_PATH + 1];
	TCHAR drive[MAX_PATH + 1];
	TCHAR dir[MAX_PATH + 1];
	TCHAR fname[MAX_PATH + 1];
	TCHAR ext[MAX_PATH + 1];

	// 実行ファイルの完全パスを取得
	if (0 != GetModuleFileName(NULL, Path, MAX_PATH)) {
		_tsplitpath_s(Path, drive, dir, fname, ext);

		strIniFile += drive;
		strIniFile += dir;
		strIniFile += fname;
		strIniFile += ".ini";

		m_IniFile.SetIniFile(strIniFile);

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("OutputPath"), _T(""), &m_strOutputPath) == 0) {
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("OutputPath"), _T(""));
		}
		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("Dsd"), -1, &m_nDSDidx) == -1) {
			m_nDSDidx = 0;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("Dsd"), m_nDSDidx);
		}
		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("Precision"), -1, &m_nPrecisionIdx) == -1) {
			m_nPrecisionIdx = 0;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("Precision"), m_nPrecisionIdx);
		}

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("GainMode"), -1, &m_radioGainMode) == -1) {
			m_radioGainMode = 0;		// ゲイン調整
//			m_radioGainMode = 1;		// ゲイン制限
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("GainMode"), m_radioGainMode);
		}
//		m_radioGainModeDdv = m_radioGainMode;

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("GainLevel"), -1, &m_nGainLevelIdx) == -1) {
//			m_nGainLevelIdx = 1;	// -0.1dB
//			m_nGainLevelIdx = 5;	// -0.5dB
			m_nGainLevelIdx = 10;	// -1,0dB
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("GainLevel"), m_nGainLevelIdx);
		}

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("GainLimit"), -1, &m_nGainLimitIdx) == -1) {
			m_nGainLimitIdx = 3;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("GainLimit"), m_nGainLimitIdx);
		}

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("AlbumTagSuffix"), _T(""), &m_evAlbumTagSuffix) == 0) {
			m_evAlbumTagSuffix = _T("(DSD)");
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("AlbumTagSuffix"), m_evAlbumTagSuffix);
		}
		m_strAlbumTagSuffixCmp = m_evAlbumTagSuffix;
		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("CompletionNotice"), -1, (int*)&dwCompletionNoticeFlag) == -1) {
			dwCompletionNoticeFlag = 0x01;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("CompletionNotice"), (int)dwCompletionNoticeFlag);
		}

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("Pcm48KHzDsd3MHzEnable"), -1, (int*)&m_Pcm48KHzEnableDsd3MHzFlag) == -1) {
			m_Pcm48KHzEnableDsd3MHzFlag = 0;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("Pcm48KHzDsd3MHzEnable"), (int)m_Pcm48KHzEnableDsd3MHzFlag);
		}
		m_Pcm48KHzEnableDsd3MHzEnable = m_Pcm48KHzEnableDsd3MHzFlag;

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("NormalizeEnable"), -1, (int*)&m_NormalizeFlagLast) == -1) {
			m_NormalizeFlagLast = 0;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("NormalizeEnable"), (int)m_NormalizeFlagLast);
		}
		m_NormalizeFlag = m_NormalizeFlagLast;
//		m_NormalizeEnable = m_NormalizeFlag;

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("CrossGainLevelEnable"), -1, (int*)&m_CrossGainLevelLast) == -1) {
			m_CrossGainLevelLast = 0;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("CrossGainLevelEnable"), (int)m_CrossGainLevelLast);
		}
		m_CrossGainLevel = m_CrossGainLevelLast;

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("OutFileOverWrite"), -1, (int*)&m_DsfOverWriteFlag) == -1) {
			m_DsfOverWriteFlag = 0;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("OutFileOverWrite"), (int)m_DsfOverWriteFlag);
		}
		m_DsfOverWriteEnable = m_DsfOverWriteFlag;

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("DsdClipOverChk"), -1, (int*)&m_DsdClipOverChkEnable) == -1) {
			m_DsdClipOverChkEnable = 1;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("DsdClipOverChk"), (int)m_DsdClipOverChkEnable);
		}

		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("CompletePowerControl"), -1, &m_nCompletePowerCrl) == -1) {
			m_nCompletePowerCrl = 0;
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("CompletePowerControl"), m_nCompletePowerCrl);
		}

		if (m_IniFile.GetPrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling44100"), -1, &m_nDsdSampling44100) == -1) {
			m_nDsdSampling44100 = 64;
			m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling44100"), m_nDsdSampling44100);
		}
		m_nDsdSampling44100Diff = m_nDsdSampling44100;

		if (m_IniFile.GetPrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling88200"), -1, &m_nDsdSampling88200) == -1) {
			m_nDsdSampling88200 = 128;
			m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling88200"), m_nDsdSampling88200);
		}
		m_nDsdSampling88200Diff = m_nDsdSampling88200;

		if (m_IniFile.GetPrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling176400"), -1, &m_nDsdSampling176400) == -1) {
			m_nDsdSampling176400 = 256;
			m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling176400"), m_nDsdSampling176400);
		}
		m_nDsdSampling176400Diff = m_nDsdSampling176400;

		if (m_IniFile.GetPrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling352800"), -1, &m_nDsdSampling352800) == -1) {
			m_nDsdSampling352800 = 256;
			m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling352800"), m_nDsdSampling352800);
		}
		m_nDsdSampling352800Diff = m_nDsdSampling352800;

		if (m_IniFile.GetPrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling705600"), -1, &m_nDsdSampling705600) == -1) {
			m_nDsdSampling705600 = 256;
			m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling705600"), m_nDsdSampling705600);
		}
		m_nDsdSampling705600Diff = m_nDsdSampling705600;
	}
}

void CPCMDSD_ConverterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDL_FileList, m_lFileList);
	DDX_Control(pDX, IDB_AllRun, m_bAllRun);
	DDX_Control(pDX, IDB_AllListDelete, m_bAllListDelete);
	DDX_Control(pDX, IDB_Run, m_bRun);
	DDX_Control(pDX, IDB_ListDelete, m_bListDelete);
	DDX_Control(pDX, IDC_SamplingRate, m_cSamplingRate);
	DDX_Control(pDX, IDS_STATIC_SAMPLING_RATE, m_sSamplingRate);
	DDX_Control(pDX, IDS_STATIC_PRECISION, m_scPrecision);
	DDX_Control(pDX, IDC_Precision, m_ccPrecision);
	DDX_Control(pDX, IDS_STATIC_EDITPATH, m_scPath);
	DDX_Control(pDX, IDC_EditPath, m_ecPath);
	DDX_Control(pDX, IDC_PathCheck, m_bcPath);
	DDX_Text(pDX, IDC_EditPath, m_evPath);
	DDX_Control(pDX, IDC_GAINLEVEL, m_combVol);
	//	DDX_Control(pDX, IDS_STATIC_GAINLEVEL, m_scVolume);
	DDX_Control(pDX, IDS_STATIC_ALBUM_TAG_SUFFIX, m_staticAlbumTagSuffix);
	DDX_Control(pDX, IDC_EDITALBUM_TAG_SUFFIX, m_editAlbumTagSuffix);
	DDX_Text(pDX, IDC_EDITALBUM_TAG_SUFFIX, m_evAlbumTagSuffix);
	DDX_Control(pDX, IDC_CHECK_NORMALIZE, m_chkboxNormalize);
	DDX_Control(pDX, IDC_BUTTON_PATH_CLEAR, m_bcPathClear);
	DDX_Control(pDX, IDC_CHECK_FILEOVERWRITE, m_chkboxFileOverWrite);
	DDX_Control(pDX, IDB_AlbumRun, m_btnAlbumRun);
	DDX_Control(pDX, IDS_STATIC_COMPLETE_OPTION, m_staticCompleteOption);
	DDX_Control(pDX, IDC_COMBO_COMPLETE_OPTION, m_combCompleteOption);
	DDX_Radio(pDX, IDC_RADIO_GAIN_MODE1, m_radioGainModeDdv);
	DDX_Control(pDX, IDC_STATIC_GAIN, m_groupGain);
	DDX_Control(pDX, IDC_RADIO_GAIN_MODE1, m_radioGainMode1);
	DDX_Control(pDX, IDC_RADIO_GAIN_MODE2, m_radioGainMode2);
	DDX_Control(pDX, IDC_GAINLIMITLEVEL, m_combLimitVol);
	DDX_Control(pDX, IDC_CHECK_CROSS_GAINLEVEL, m_chkboxCrossGainLevel);
	DDX_Control(pDX, IDC_BUTTON_SETTING, m_btnAutSetting);
	DDX_Control(pDX, IDC_EDIT_ENCODER_PERSON, m_editEncoderPerson);
	DDX_Control(pDX, IDS_STATIC_ENCODER_PERSON, m_staEncoderPerson);
	DDX_Text(pDX, IDC_EDIT_ENCODER_PERSON, m_evEncoderPerson);
	DDX_Control(pDX, IDC_STATIC_REGISTCNT, m_scRegistCnt);
}

BEGIN_MESSAGE_MAP(CPCMDSD_ConverterDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDB_AllRun, &CPCMDSD_ConverterDlg::OnBnClickedAllrun)
	ON_BN_CLICKED(IDB_AllListDelete, &CPCMDSD_ConverterDlg::OnBnClickedAlllistdelete)
	ON_BN_CLICKED(IDB_Run, &CPCMDSD_ConverterDlg::OnBnClickedRun)
	ON_BN_CLICKED(IDB_ListDelete, &CPCMDSD_ConverterDlg::OnBnClickedListdelete)
	ON_WM_DROPFILES()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_PathCheck, &CPCMDSD_ConverterDlg::OnBnClickedPathcheck)
	ON_NOTIFY(LVN_KEYDOWN, IDL_FileList, &CPCMDSD_ConverterDlg::OnLvnKeydownFilelist)
	ON_BN_CLICKED(IDC_CHECK_NORMALIZE, &CPCMDSD_ConverterDlg::OnBnClickedCheckNormalize)
	ON_BN_CLICKED(IDC_BUTTON_PATH_CLEAR, &CPCMDSD_ConverterDlg::OnBnClickedButtonPathClear)
	ON_BN_CLICKED(IDC_CHECK_FILEOVERWRITE, &CPCMDSD_ConverterDlg::OnBnClickedCheckFileoverwrite)
	ON_BN_CLICKED(IDB_AlbumRun, &CPCMDSD_ConverterDlg::OnBnClickedAlbumrun)
	ON_EN_CHANGE(IDC_EDITALBUM_TAG_SUFFIX, &CPCMDSD_ConverterDlg::OnEnChangeEditalbumTagSuffix)
	ON_NOTIFY(HDN_DIVIDERDBLCLICK, 0, &CPCMDSD_ConverterDlg::OnHdnDividerdblclickFilelist)
	ON_EN_KILLFOCUS(IDC_EDITALBUM_TAG_SUFFIX, &CPCMDSD_ConverterDlg::OnEnKillfocusEditalbumTagSuffix)
	ON_CBN_SELCHANGE(IDC_COMBO_COMPLETE_OPTION, &CPCMDSD_ConverterDlg::OnCbnSelchangeComboCompleteOption)
	ON_BN_CLICKED(IDC_RADIO_GAIN_MODE1, &CPCMDSD_ConverterDlg::OnBnClickedRadioGainMode1)
	ON_BN_CLICKED(IDC_RADIO_GAIN_MODE2, &CPCMDSD_ConverterDlg::OnBnClickedRadioGainMode2)
	ON_BN_CLICKED(IDC_CHECK_CROSS_GAINLEVEL, &CPCMDSD_ConverterDlg::OnBnClickedCheckCrossGainlevel)
	ON_BN_CLICKED(IDC_BUTTON_SETTING, &CPCMDSD_ConverterDlg::OnBnClickedButtonSetting)
	ON_CBN_SELCHANGE(IDC_SamplingRate, &CPCMDSD_ConverterDlg::OnCbnSelchangeSamplingrate)
	ON_NOTIFY(LVN_INSERTITEM, IDL_FileList, &CPCMDSD_ConverterDlg::OnLvnInsertitemFilelist)
END_MESSAGE_MAP()


// CPCMDSD_ConverterDlg メッセージ ハンドラー
BOOL CPCMDSD_ConverterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	{
		VS_FIXEDFILEINFO stVerInfo;
		CString strTitle;
		CString strTitleIni;
		CString strVer;

		stVerInfo = GetVersionInfo();
		DWORD ProductVer1 = HIWORD(stVerInfo.dwProductVersionMS);	// Major Ver
		DWORD ProductVer2 = LOWORD(stVerInfo.dwProductVersionMS);	// Minar1 Ver
		DWORD ProductVer3 = HIWORD(stVerInfo.dwProductVersionLS);	// Minar2 Ver
		DWORD ProductVer4 = LOWORD(stVerInfo.dwProductVersionLS);	// β Ver
		if (ProductVer4 == 0) {
			// 正式Ver「9.99」
			strVer.Format(_T("Ver %d.%d%d"), ProductVer1, ProductVer2, ProductVer3);
		} else {
			// βVer「9.99β9」
			strVer.Format(_T("Ver %d.%d%dβ%d"), ProductVer1, ProductVer2, ProductVer3, ProductVer4);
		}

		// ダイアログタイトル設定
		this->GetWindowText(strTitle);
		strTitleIni = strTitle;
		strTitle += _T(" ");
		strTitle += strVer;
		this->SetWindowText(strTitle);
		m_AppName = strTitle;

		// 初期設定
		if (m_IniFile.GetPrivateProfile(_T("OPTION"), _T("EncodedBy"), _T(""), &m_strEncodedBy) == 0) {
			CString msg;
			CString strUser;

			strUser = GetUserName();
			msg = _T("変換ファイルに作成者(エンコーダした人)タグの設定をユーザー名「") + strUser + _T("」にしますか？\n\nキャンセルした場合は「")  + m_AppName + _T("」が設定されます。\n後でも変更出来ます。");
			if(MessageBox(msg, m_AppName + L" 【初期設定】", MB_OKCANCEL) == IDOK){
				// ログインユーザー取得
				m_strEncodedBy = strUser;
			} else {
				m_strEncodedBy = m_AppName;
			}
			m_IniFile.WritePrivateProfile(_T("OPTION"), _T("EncodedBy"), m_strEncodedBy);

			msg = _T("");
			msg += _T("1.オーディオファイルを追加します。\n");
			msg += _T("  WAV/FLAC/ALAC(.m4a)/SONY WAVE64(.w64)/DSDIFF(.dff)\n\n");
			msg += _T("2.実行ボタンでDSD変換が開始されます。\n");
			msg += _T("  [全て実行]    ：リストの曲を全て変換します。\n");
			msg += _T("  [実行]          ：リストの選択した曲を変換します。\n");
			msg += _T("  [アルバム実行]：完全なギャップレスDSDを作成します。\n");
			msg += _T("                        ライブなどのノンストップアルバムに適します。\n");
			msg += _T("                        HDDに十分な空き容量が必要になります。\n");
			msg += _T("                        普通のアルバムは[全て実行]で行って下さい。\n");
			msg += _T("\n");
			msg += _T("※詳細な使い方は付属のドキュメントを参照下さい。\n");
			MessageBox(msg, m_AppName + L" 【使い方】", MB_OK);
		}
		m_strEncodedBy.Trim();

		//*** エンコーダー者の設定が「PCM-DSD_Converter 改」が含まれるならVer不一致するなら更新する ※そんな名前の人いないだろうから***
		int idx;
		idx = m_strEncodedBy.Find(strTitleIni, 0);
		// ダイアログタイトルが含まれる？
		if (idx >= 0) {
			// 比較して不一致なら更新する
			if (m_strEncodedBy.Compare(m_AppName) != 0) {
				m_IniFile.WritePrivateProfile(_T("OPTION"), _T("EncodedBy"), m_AppName);
				m_strEncodedBy = m_AppName;
			}
		}
		m_evEncoderPerson = m_strEncodedBy;

		setlocale(LC_CTYPE, "jpn");

		m_evPath = m_strOutputPath;
//		m_ecPath.SetWindowText(m_strOutputPath);
		m_cSamplingRate.ResetContent();
		m_cSamplingRate.AddString(_T("AUTO"));
		m_cSamplingRate.AddString(_T("DSD64"));
		m_cSamplingRate.AddString(_T("DSD128"));
		m_cSamplingRate.AddString(_T("DSD256"));
		m_cSamplingRate.AddString(_T("DSD512"));
		m_cSamplingRate.AddString(_T("DSD1024"));
		m_cSamplingRate.AddString(_T("DSD2048"));
		m_cSamplingRate.SetCurSel(m_nDSDidx);
		AutoSettingBtnEnableProc();
		m_ccPrecision.SetCurSel(m_nPrecisionIdx);
		m_chkboxNormalize.SetCheck(m_NormalizeFlag);
		m_chkboxCrossGainLevel.SetCheck(m_CrossGainLevel);
		m_chkboxFileOverWrite.SetCheck(m_DsfOverWriteFlag);

		CString strToolTip;
		
		strToolTip += _T("[DSDサンプリングレート]\n");
		strToolTip += _T("DSD  64： 2.8/ 3.0MHz\n");
		strToolTip += _T("DSD 128： 5.6/ 6.1MHz\n");
		strToolTip += _T("DSD 256：11.2/12.2MHz\n");
		strToolTip += _T("DSD 512：22.5/24.5MHz\n");
		strToolTip += _T("DSD1024：45.1/49.1MHz\n");
		strToolTip += _T("DSD2048：90.3/98.3MHz\n");
		strToolTip += _T("※AUTOは設定ボタンで設定した内容で変換されます。\n");
		m_tooltipSamplingRate.Create(this);
		m_tooltipSamplingRate.SetMaxTipWidth(300);
		m_tooltipSamplingRate.AddTool(GetDlgItem(IDC_SamplingRate), strToolTip);

		int i;
		CString strText;
		m_combVol.ResetContent();
		for (i = 0; i < 21; i++) {
			strText.Format(_T("%2.1lf"), (double)i * -0.1);
			m_combVol.AddString(strText);
		}
		m_combVol.SetCurSel(m_nGainLevelIdx);

		m_combLimitVol.ResetContent();
		for (i = 0; i < 15; i++) {
			// -10dB～-24dB
			strText.Format(_T("%2d"), (GAIN_LIMIT_DB_OFFSET + i) * -1);
			m_combLimitVol.AddString(strText);
		}
		m_combLimitVol.SetCurSel(m_nGainLimitIdx);

		m_radioGainModeDdv = m_radioGainMode;

		m_combCompleteOption.ResetContent();
		m_combCompleteOption.AddString(_T("何もしない"));
		m_combCompleteOption.AddString(_T("シャットダウン"));
		m_combCompleteOption.AddString(_T("スタンバイ"));
		m_combCompleteOption.AddString(_T("休止"));
		m_combCompleteOption.AddString(_T("終了する"));
		m_combCompleteOption.SetCurSel(m_nCompletePowerCrl);

//		m_cSamplingRate.SetCurSel(0);
//		m_ccPrecision.SetCurSel(0);
//		m_combVol.SetCurSel(1);

		UpdateData(FALSE);		// DDVを先に更新して
		GainModeGroupDisp();	// ゲインモードを表示

		DragAcceptFiles();
		ListInit();
		fftw_init_threads();
		m_dProgress.Create(ProgressDlg::IDD, this);

		// ウィンドウ位置の変更
		// https://www.ruche-home.net/program/tips/window-placeここを参考にマルチモニタ環境を考慮しよう
		LoadSafeWindowPos(&m_InitPos, &m_ShowCmd);

		if (!::IsRectEmpty(&m_InitPos)){
			// ウィンドウ位置復元
#if 1		// どっちでやっても大差ないと思う。
			MoveWindow(&m_InitPos, FALSE);
#else
			SetWindowPos(NULL, m_InitPos.left, m_InitPos.top, m_InitPos.right - m_InitPos.left, m_InitPos.bottom - m_InitPos.top, SWP_NOZORDER);
#endif
		}

		ShowWindow(m_ShowCmd);
	}

	// コマンドラインのファイルをリストに登録
	INT_PTR max = m_strCmdLineArray.GetCount();
	INT_PTR i;
	CString strPrm;
	for(i = 0; i < max;i ++){
		strPrm = m_strCmdLineArray.GetAt(i);
		if (PathIsDirectory(strPrm)) {
			DirectoryFind(strPrm.GetBuffer());
		} else {
			WAV_FileRead(strPrm.GetBuffer(), FALSE);
		}
	}
	// 1件以上あり？
	if (m_lFileList.GetItemCount() > 0) {
		// 先頭をフォーカス
		m_lFileList.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}

	// リストコントロール登録件数表示
	ListRegistDisp();

	UpdateData(TRUE);

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

BOOL CPCMDSD_ConverterDlg::PreTranslateMessage(MSG* pMsg)
{
	 m_tooltipSamplingRate.RelayEvent(pMsg);

	return CDialog::PreTranslateMessage(pMsg);
}

// Version取得
VS_FIXEDFILEINFO CPCMDSD_ConverterDlg::GetVersionInfo()
{
	VS_FIXEDFILEINFO stVerInfo;
	TCHAR path[MAX_PATH];
	path[::GetModuleFileName(AfxGetInstanceHandle(), path, sizeof path / sizeof path[0])] = 0;

	DWORD dummy;
	DWORD size = GetFileVersionInfoSize(path, &dummy);
	if (size == 0) {
		// Error...
	}
	LPVOID lpData = NULL;
	__try {
		lpData = alloca(size);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		// Error...
	}
	if (!GetFileVersionInfo(path, 0, size, lpData)) {
		// Error...
	}
	LPVOID lpBuffer;
	UINT vSize;
	if (!VerQueryValue(lpData, _T("\\"), &lpBuffer, &vSize)) {
		// Error...
	}
	VS_FIXEDFILEINFO* info = (VS_FIXEDFILEINFO*)lpBuffer;
	stVerInfo = *info;

	return stVerInfo;
}

//ファイルリスト設定
void CPCMDSD_ConverterDlg::ListInit()
{
	m_lFileList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"ファイル名", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_FILE_NAME]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"タイトル", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_TITLE]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"アーティスト", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_ARTIST]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"アルバム", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_ALBUM]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"トラックNo", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_TRACK_NO]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"ディスクNo", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_DISC_NO]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"ディスク数", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_DISC_TOTAL]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"アルバムアーティスト", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_ALBUM_ARTIST]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"サンプリングレート", LVCFMT_RIGHT, m_nColWidthDef[EN_LIST_COLUMN_SAMPLING_RATE]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"ビット数", LVCFMT_RIGHT, m_nColWidthDef[EN_LIST_COLUMN_BIT]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"長さ", LVCFMT_RIGHT, m_nColWidthDef[EN_LIST_COLUMN_LENGTH]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"サンプル数", LVCFMT_RIGHT, m_nColWidthDef[EN_LIST_COLUMN_SAMPLING_COUNT]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"ファイルパス", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_PATH]);
	m_lFileList.InsertColumn(m_lFileList.GetHeaderCtrl().GetItemCount(), L"拡張子", LVCFMT_LEFT, m_nColWidthDef[EN_LIST_COLUMN_EXT]);

	int i;
	int nWidth;
	CString strKey;
	for (i = 0; i < EN_LIST_COLUMN_MAX; i++) {
		strKey.Format(_T("ListWidth%d"), i);
		m_IniFile.GetPrivateProfile(_T("WINDOWINFO"), strKey, m_nColWidthDef[i], &nWidth);
#if 0
		if(nWidth < m_nColWidthDef[i]){
			nWidth = m_nColWidthDef[i];
		}
#endif
		m_lFileList.SetColumnWidth(i, nWidth);
	}
}

// リストコントロール登録件数表示
void CPCMDSD_ConverterDlg::ListRegistDisp()
{
	CString strTmp;
	int nVal;
	
	nVal = m_lFileList.GetItemCount();
	m_nListRegistCnt = nVal;
	strTmp.Format(_T("%d 件"), nVal);

	m_scRegistCnt.SetWindowText(strTmp);
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

//ウィンドウサイズ変更時のアイテム追従
void CPCMDSD_ConverterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		//②ダイアログ全体の大きさ取得
		CRect rect;
		GetClientRect(&rect);

		//③サイズ調整
		m_lFileList.MoveWindow(0, 0, rect.Width(), rect.Height() - 180);

		m_chkboxFileOverWrite.MoveWindow(rect.Width() - 595, rect.Height() - 175, 110, 20);

		m_btnAlbumRun.MoveWindow(rect.Width() - 485, rect.Height() - 175, 80, 20);
		m_bAllRun.MoveWindow(rect.Width() - 400, rect.Height() - 175, 80, 20);
		m_bAllListDelete.MoveWindow(rect.Width() - 315, rect.Height() - 175, 80, 20);
		m_bRun.MoveWindow(rect.Width() - 230, rect.Height() - 175, 80, 20);
		m_bListDelete.MoveWindow(rect.Width() - 145, rect.Height() - 175, 80, 20);

		m_scRegistCnt.MoveWindow(rect.Width() - 60, rect.Height() - 171, 55, 20);

		m_cSamplingRate.MoveWindow(rect.Width() - 490, rect.Height() - 150, 80, 20);
		m_sSamplingRate.MoveWindow(rect.Width() - 660, rect.Height() - 146, 160, 20);

		m_btnAutSetting.MoveWindow(rect.Width() - 405, rect.Height() - 150, 60, 20);

		m_ccPrecision.MoveWindow(rect.Width() - 490, rect.Height() - 125, 80, 20);
		m_scPrecision.MoveWindow(rect.Width() - 660, rect.Height() - 121, 160, 20);

		m_groupGain.MoveWindow(rect.Width() - 340, rect.Height() - 150, 335, 60);
		CRect rectGropGain;
		CRect rectDialog = rect;
		ClientToScreen(rectDialog);
		m_groupGain.GetClientRect(&rectGropGain);
		m_groupGain.ClientToScreen(&rectGropGain);
		rectGropGain.left -= rectDialog.left;
		rectGropGain.right -= rectDialog.left;
		rectGropGain.top -= rectDialog.top;
		rectGropGain.bottom -= rectDialog.top;

		m_radioGainMode1.MoveWindow(rectGropGain.left + 10, rectGropGain.top + 14, 39, 19);
		m_combVol.MoveWindow(rectGropGain.left + 60, rectGropGain.top + 14, 50, 12);
		m_chkboxNormalize.MoveWindow(rectGropGain.left + 115, rectGropGain.top + 14, 215, 19);
		m_radioGainMode2.MoveWindow(rectGropGain.left + 10, rectGropGain.top + 36, 39, 19);
		m_combLimitVol.MoveWindow(rectGropGain.left + 60, rectGropGain.top + 36, 50, 12);
		m_chkboxCrossGainLevel.MoveWindow(rectGropGain.left + 115, rectGropGain.top + 36, 215, 19);

		m_editAlbumTagSuffix.MoveWindow(rect.Width() - 490, rect.Height() - 100, 140, 20);
		m_staticAlbumTagSuffix.MoveWindow(rect.Width() - 660, rect.Height() - 96, 160, 20);

		m_editEncoderPerson.MoveWindow(rect.Width() - 490, rect.Height() - 75, 210, 20);
		m_staEncoderPerson.MoveWindow(rect.Width() - 660, rect.Height() - 71, 160, 20);

		m_scPath.MoveWindow(rect.Width() - 660, rect.Height() - 45, 160, 20);
		m_ecPath.MoveWindow(rect.Width() - 490, rect.Height() - 50, 355, 20);
		m_bcPath.MoveWindow(rect.Width() - 130, rect.Height() - 50, 60, 20);
		m_bcPathClear.MoveWindow(rect.Width() - 65, rect.Height() - 50, 60, 20);

		m_staticCompleteOption.MoveWindow(rect.Width() - 660, rect.Height() - 20, 160, 20);
		m_combCompleteOption.MoveWindow(rect.Width() - 490, rect.Height() - 25, 100, 20);

		CDialogEx::OnPaint();
	}
}


void CPCMDSD_ConverterDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	// TODO: ここにメッセージ ハンドラー コードを追加します。
	Invalidate(TRUE);
}

//閉じる操作等無効化
void CPCMDSD_ConverterDlg::OnClose()
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	// INIファイル保存
	SaveIniFilee();

	CDialogEx::OnClose();
}

void CPCMDSD_ConverterDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: ここにメッセージ ハンドラー コードを追加します。
}

// INIファイル保存
void CPCMDSD_ConverterDlg::SaveIniFilee()
{
	CString strValue;
	int nVal;
	WINDOWPLACEMENT wpl;

	UpdateData();

	m_ecPath.GetWindowText(strValue);

	if(strValue != m_strOutputPath){
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("OutputPath"), strValue);
	}

	nVal = m_cSamplingRate.GetCurSel();
	if (nVal != m_nDSDidx) {
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("Dsd"), nVal);
	}

	nVal = m_ccPrecision.GetCurSel();
	if (nVal != m_nPrecisionIdx) {
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("Precision"), nVal);
	}

	nVal = m_radioGainModeDdv;
	if (nVal != m_radioGainMode) {
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("GainMode"), nVal);
	}

	nVal = m_combVol.GetCurSel();
	if (nVal != m_nGainLevelIdx) {
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("GainLevel"), nVal);
	}

	nVal = m_combLimitVol.GetCurSel();
	if (nVal != m_nGainLimitIdx) {
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("GainLimit"), nVal);
	}

	m_editAlbumTagSuffix.GetWindowText(strValue);
	if(strValue != m_strAlbumTagSuffixCmp){
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("AlbumTagSuffix"), strValue);
	}

	if(m_Pcm48KHzEnableDsd3MHzFlag !=  m_Pcm48KHzEnableDsd3MHzEnable){
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("Pcm48KHzDsd3MHzEnable"), (int)m_Pcm48KHzEnableDsd3MHzEnable);
	}

//	if(m_NormalizeFlag != m_NormalizeEnable){
	if(m_NormalizeFlag != m_NormalizeFlagLast){
//		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("NormalizeEnable"), m_NormalizeEnable);
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("NormalizeEnable"), m_NormalizeFlag);
	}

	if(m_CrossGainLevel != m_CrossGainLevelLast){
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("CrossGainLevelEnable"), m_CrossGainLevel);
	}

	if(m_DsfOverWriteFlag != m_DsfOverWriteEnable){
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("OutFileOverWrite"), m_DsfOverWriteEnable);
	}

	nVal = m_combCompleteOption.GetCurSel();
	if (nVal != m_nCompletePowerCrl) {
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("CompletePowerControl"), nVal);
	}

	if(m_evEncoderPerson != m_strEncodedBy){
		m_IniFile.WritePrivateProfile(_T("OPTION"), _T("EncodedBy"), m_evEncoderPerson);
	}

	if(m_nDsdSampling44100Diff != m_nDsdSampling44100){
		m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling44100"), m_nDsdSampling44100);
	}

	if(m_nDsdSampling88200Diff = m_nDsdSampling88200){
		m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling88200"), m_nDsdSampling88200);
	}

	if(m_nDsdSampling176400Diff = m_nDsdSampling176400){
		m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling176400"), m_nDsdSampling176400);
	}

	if(m_nDsdSampling352800Diff = m_nDsdSampling352800){
		m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling352800"), m_nDsdSampling352800);
	}

	if(m_nDsdSampling705600Diff = m_nDsdSampling705600){
		m_IniFile.WritePrivateProfile(_T("DSDCONVERTMAP"), _T("Sampling705600"), m_nDsdSampling705600);
	}

	GetWindowPlacement(&wpl);

	m_IniFile.WritePrivateProfile(_T("WINDOWINFO"), _T("Left"), wpl.rcNormalPosition.left);
	m_IniFile.WritePrivateProfile(_T("WINDOWINFO"), _T("Top"), wpl.rcNormalPosition.top);
	m_IniFile.WritePrivateProfile(_T("WINDOWINFO"), _T("Right"), wpl.rcNormalPosition.right);
	m_IniFile.WritePrivateProfile(_T("WINDOWINFO"), _T("Bottom"), wpl.rcNormalPosition.bottom);
	m_IniFile.WritePrivateProfile(_T("WINDOWINFO"), _T("Show"), wpl.showCmd);
	m_IniFile.WritePrivateProfile(_T("WINDOWINFO"), _T("MonitorBorder"), m_bWindowMonitorBorder);

	int i;
	int nWidth;
	CString strKey;
	for (i = 0; i < EN_LIST_COLUMN_MAX; i++) {
		strKey.Format(_T("ListWidth%d"), i);
		nWidth = m_lFileList.GetColumnWidth(i);
		m_IniFile.WritePrivateProfile(_T("WINDOWINFO"), strKey, nWidth);
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CPCMDSD_ConverterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 有効サンプリングレートチェック
BOOL CPCMDSD_ConverterDlg::IsSamplingRate(DWORD dwSamplingRate)
{
	BOOL bRet;

	switch(dwSamplingRate){
		case 44100:			//  44.1KHz
		case 44100 * 2:		//  88.2KHz
		case 44100 * 4:		// 176.4KHz
		case 44100 * 8:		// 352.8KHz
		case 44100 * 16:	// 705.6KHz
		case 48000:			//  48.0KHz
		case 48000 * 2:		//  96.0KHz
		case 48000 * 4:		// 192.0KHz
		case 48000 * 8:		// 384.0KHz
		case 48000 * 16:	// 768.0KHz
			bRet = TRUE;
			break;
		default:
			bRet = FALSE;
			break;
	}

	return bRet;
}

// 有効BIT深度チェック
BOOL CPCMDSD_ConverterDlg::IsBitDepth(DWORD dwBit)
{
	BOOL bRet;

	switch(dwBit){
		case 16:
		case 20:
		case 24:
		case 32:
		case 64:
			bRet = TRUE;
			break;
		default:
			bRet = FALSE;
			break;
	}

	return bRet;
}

// WAVサンプリングレート取得
bool CPCMDSD_ConverterDlg::GeyWavSamplePerSec(TCHAR *filepath, int *pnSamplePerSec)
{
	bool bRet;

	CString *metadata = new CString[EN_LIST_COLUMN_MAX];
	bRet = WAV_Metadata(filepath, metadata, pnSamplePerSec);
	delete[] metadata;

	return bRet;
}

//Wavファイルチェック及びメタデータ読み取り
bool CPCMDSD_ConverterDlg::WAV_Metadata(TCHAR *filepath, CString *metadata)
{
	return WAV_Metadata(filepath, metadata, NULL);
}

bool CPCMDSD_ConverterDlg::WAV_Metadata(TCHAR *filepath, CString *metadata, int *pnSamplePerSec)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	metadata[EN_LIST_COLUMN_PATH] = filepath;
	metadata[EN_LIST_COLUMN_FILE_NAME] = filename;
	BOOL flag = true;
//	string wav;
	CString wav;
	wav = fileext;
	CString strExt(fileext);
	strExt = strExt.MakeUpper();
//	if (strExt != ".WAV") {
	if (strExt != _T(".WAV")) {
		return false;
	}
#if 0
	if (wav != ".wav" && wav != ".WAV"){
		return false;
	}
#endif

	FILE *fprwav;
	errno_t error;

//	if ((error = fopen_s(&fprwav, filepath_tmp, "rb")) != 0) {
//		return false;
//	}
	error = _tfopen_s(&fprwav, filepath, _T("rb"));
	if(error != 0) {
		return false;
	}

	unsigned __int32 samplingrate;
	unsigned short fmtID;
	unsigned short bitdepth;
	char tmp4[6];
	CString strBit;

	while (flag){
		if (feof(fprwav)){
			fclose(fprwav);
			return false;
		}
		fread(tmp4, 1, 1, fprwav);
		tmp4[1] = '\0';
		wav = tmp4;
		if (wav == "f"){
			fread(tmp4, 1, 3, fprwav);
			tmp4[3] = '\0';
			wav = tmp4;
			if (wav == "mt "){
				flag = false;
			}
		}
	}

	//fmtIDでFloatInt判別
	_fseeki64(fprwav, 4, SEEK_CUR);
	fread(&fmtID, 2, 1, fprwav);
	if (fmtID == WAVE_FORMAT_IEEE_FLOAT){
		strBit = "bit Float";
	}
	else if (fmtID == WAVE_FORMAT_PCM){
		strBit = "bit Int";
	}
	else{
		fclose(fprwav);
		return false;
	}

	unsigned short chnum;
	fread(&chnum, 2, 1, fprwav);
	if (chnum != 2){
		fclose(fprwav);
		return false;
	}

	fread(&samplingrate, 4, 1, fprwav);
	if (pnSamplePerSec != NULL){
		*pnSamplePerSec = (int)samplingrate;
	}
//	metadata[EN_LIST_COLUMN_SAMPLING_RATE] = to_string(samplingrate);
	CString strTmp;
	strTmp.Format(_T("%d"), samplingrate);
	metadata[EN_LIST_COLUMN_SAMPLING_RATE] = strTmp;
	// 有効サンプリングレートチェック
	if(IsSamplingRate(samplingrate) == TRUE){
		flag = true;
	} else {
		fclose(fprwav);
		return false;
	}

	_fseeki64(fprwav, 6, SEEK_CUR);
	fread(&bitdepth, 2, 1, fprwav);
//	metadata[EN_LIST_COLUMN_BIT] = to_string(bitdepth) + metadata[EN_LIST_COLUMN_BIT];
	strTmp.Format(_T("%d"), bitdepth);
	metadata[EN_LIST_COLUMN_BIT] = strTmp + strBit;
	// 有効BIT深度チェック
	if (IsBitDepth(bitdepth) == TRUE){
		flag = true;
	} else{
		fclose(fprwav);
		return false;
	}

	while (flag){
		if (feof(fprwav)){
			fclose(fprwav);
			return false;
		}
		fread(tmp4, 1, 1, fprwav);
		tmp4[1] = '\0';
		wav = tmp4;
		if (wav == "d"){
			fread(tmp4, 1, 3, fprwav);
			tmp4[3] = '\0';
			wav = tmp4;
			if (wav == "ata"){
				flag = false;
			}
		}
	}
	long samplesize;
	fread(&samplesize, 4, 1, fprwav);


	INT64 nPlayTimeSec;		// 再生時間(sec)
	CString strPlayTime;

	nPlayTimeSec = samplesize / ((bitdepth + (bitdepth % 8)) / 8) / chnum / samplingrate;
	strPlayTime.Format(_T("%3I64d:%02I64d"), nPlayTimeSec / 60, nPlayTimeSec % 60);
	metadata[EN_LIST_COLUMN_LENGTH] = strPlayTime;

//	metadata[EN_LIST_COLUMN_SAMPLING_COUNT] = to_string(samplesize);
//	strTmp.Format(_T("%d"), samplesize);
	strTmp.Format(_T("%d"), samplesize / ((bitdepth + (bitdepth % 8)) / 8) / chnum);
	metadata[EN_LIST_COLUMN_SAMPLING_COUNT] = strTmp;

	metadata[EN_LIST_COLUMN_EXT] = _T("WAV");

	fclose(fprwav);

	STFLAC_COMMENT stFlacComm;		// FLAC COMMENT
	BOOL bTagEnable = FALSE;

	FlacCommentInit(&stFlacComm, 1);
	WAV_Tagdata(filepath, &stFlacComm, &bTagEnable);

	metadata[EN_LIST_COLUMN_TRACK_NO] = stFlacComm.strTracknumber;
	metadata[EN_LIST_COLUMN_TITLE] = stFlacComm.strTitle;
	metadata[EN_LIST_COLUMN_ARTIST] = stFlacComm.strArtist;
	metadata[EN_LIST_COLUMN_ALBUM] = stFlacComm.strAlbum;
	metadata[EN_LIST_COLUMN_DISC_NO] = stFlacComm.strDiscnumber;
	metadata[EN_LIST_COLUMN_DISC_TOTAL] = stFlacComm.strDisctotal;
	metadata[EN_LIST_COLUMN_ALBUM_ARTIST] = stFlacComm.strAlbumArtist;

	FlacCommentInit(&stFlacComm, 0);
	
	return true;
}

// SONY WAVE64サンプリングレート取得
bool CPCMDSD_ConverterDlg::GeyWave64SamplePerSec(TCHAR *filepath, int *pnSamplePerSec)
{
	bool bRet;

	CString *metadata = new CString[EN_LIST_COLUMN_MAX];
	bRet = Wave64_Metadata(filepath, metadata, pnSamplePerSec);
	delete[] metadata;

	return bRet;
}

//SONT WAVE64(W64)ファイルチェック及びメタデータ読み取り
bool CPCMDSD_ConverterDlg::Wave64_Metadata(TCHAR *filepath, CString *metadata)
{
	int nSamplePerSec = 0;

	return Wave64_Metadata(filepath, metadata, &nSamplePerSec);
}

bool CPCMDSD_ConverterDlg::Wave64_Metadata(TCHAR *filepath, CString *metadata, int *pnSamplePerSec)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	metadata[EN_LIST_COLUMN_PATH] = filepath;
	metadata[EN_LIST_COLUMN_FILE_NAME] = filename;
	CString wav;
	wav = fileext;
	CString strExt(fileext);
	strExt = strExt.MakeUpper();
	if (strExt != _T(".W64")) {
		return false;
	}

	CFile fileObj;
	CFileException exp;
	BOOL bRet;

	ST_W64_CHUNK stW64ChunkRiff;
	ST_W64_CHUNK stW64Chunkfmt;
	ST_W64_CHUNK stW64Chunk;
	ST_W64_FACT stFact;
	ST_W64_DATA stData;
	GUID guidWave;
	PCMWAVEFORMAT w64fmt;

	DWORD samplingrate = 0;
	WORD bitdepth = 0;
	CString strTmp;
	CString strBit;

	bRet = fileObj.Open(filepath, CFile::modeRead | CFile::typeBinary, &exp);
	if (bRet == FALSE) {
		return false;
	}

	// RIFFチャンク
	fileObj.Read(&stW64ChunkRiff, sizeof(stW64ChunkRiff));
	if (stW64ChunkRiff.Guid != s_GUID[W64_RIFF]) {
		fileObj.Close();
		return false;
	}

	// WAVEチャンク
	fileObj.Read(&guidWave, sizeof(guidWave));
	if (guidWave != s_GUID[W64_WAVE]) {
		fileObj.Close();
		return false;
	}

	// fmtチャンク
	fileObj.Read(&stW64Chunkfmt, sizeof(stW64Chunkfmt));
	if (stW64Chunkfmt.Guid != s_GUID[W64_FMT]) {
		fileObj.Close();
		return false;
	}
	fileObj.Read(&w64fmt, sizeof(w64fmt));

	WAVEFORMATEXTENSIBLE stWaveFormatExt;
	GUID SubFormatGUID;

	LONGLONG llOff;
	unsigned short chnum = 0;

	switch(w64fmt.wf.wFormatTag){
		case WAVE_FORMAT_PCM:
			samplingrate = w64fmt.wf.nSamplesPerSec;
			bitdepth = w64fmt.wBitsPerSample;
			chnum = w64fmt.wf.nChannels;
			strBit = "bit Int";
			break;
		case WAVE_FORMAT_IEEE_FLOAT:
			samplingrate = w64fmt.wf.nSamplesPerSec;
			bitdepth = w64fmt.wBitsPerSample;
			chnum = w64fmt.wf.nChannels;
			strBit = "bit Float";
			break;
		case WAVE_FORMAT_EXTENSIBLE:
			llOff = sizeof(w64fmt) * -1;
			fileObj.Seek(llOff, CFile::current);
			fileObj.Read(&stWaveFormatExt, sizeof(stWaveFormatExt));
			
			SubFormatGUID = (GUID)stWaveFormatExt.SubFormat;
			if (SubFormatGUID == KSDATAFORMAT_SUBTYPE_PCM){
				strBit = "bit Int";
			}
			if (SubFormatGUID == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
				strBit = "bit Float";
			}
			samplingrate = stWaveFormatExt.Format.nSamplesPerSec;
			bitdepth = stWaveFormatExt.Samples.wValidBitsPerSample;
			chnum = stWaveFormatExt.Format.nChannels;
			break;
		default:
			TRACE("fmt Chunk FormatTag is Unknown:0x%04X", w64fmt.wf.wFormatTag);
			break;
	}

	if (pnSamplePerSec != NULL){
		*pnSamplePerSec = (int)samplingrate;
	}

	strTmp.Format(_T("%d"), samplingrate);
	metadata[EN_LIST_COLUMN_SAMPLING_RATE] = strTmp;
	strTmp.Format(_T("%d"), bitdepth);
	metadata[EN_LIST_COLUMN_BIT] = strTmp + strBit;

	int i;
	stData.ullSampleLength = 0;
	do{
		fileObj.Read(&stW64Chunk, sizeof(stW64Chunk));
		for(i = 0;i < W64_GUID_MAX;i ++){
			if (stW64Chunk.Guid == s_GUID[i]) {
				switch(i){
					case W64_FACT:
						fileObj.Read(&stFact, sizeof(stFact));
						break;
					case W64_DATA:
						stData.ullSampleLength = stW64Chunk.ullSize;
						break;
					default:
						fileObj.Seek(stW64Chunk.ullSize, CFile::current);
						break;
				}
				break;
			}
		}
	} while (stW64Chunk.Guid != s_GUID[W64_DATA]);

	fileObj.Close();

	if (chnum != 2) {
		fileObj.Close();
		return false;
	}

	// 有効サンプリングレートチェック
	if(IsSamplingRate(samplingrate) == FALSE){
		return false;
	}

	// 有効BIT深度チェック
	if (IsBitDepth(bitdepth) == FALSE){
		return false;
	}

	UINT64 nPlayTimeSec;		// 再生時間(sec)
	CString strPlayTime;
	UINT64 samplesize;
	samplesize = stData.ullSampleLength - sizeof(stW64Chunk);

	nPlayTimeSec = samplesize / ((bitdepth + (bitdepth % 8)) / 8) / chnum / samplingrate;
	strPlayTime.Format(_T("%3I64d:%02I64d"), nPlayTimeSec / 60, nPlayTimeSec % 60);
	metadata[EN_LIST_COLUMN_LENGTH] = strPlayTime;

	strTmp.Format(_T("%lld"), samplesize / ((bitdepth + (bitdepth % 8)) / 8) / chnum);
	metadata[EN_LIST_COLUMN_SAMPLING_COUNT] = strTmp;

	metadata[EN_LIST_COLUMN_EXT] = _T("W64");

	return true;
}

//FLACファイルチェック及びメタデータ読み取り
bool CPCMDSD_ConverterDlg::FLAC_Metadata(TCHAR *filepath, CString *metadata)
{
	FLAC__bool  bFlacRet;
	FLAC__StreamMetadata stStreaminfo;
	FLAC__StreamMetadata *pstTaginfo = NULL;
	bool bRet = false;
	CString strExt;
	unsigned __int32 samplingrate;
	INT64 nPlayTimeSec;		// 再生時間(sec)
	CString strPlayTime;
	CString strTmp;

	STFLAC_COMMENT stFlacComm;		// FLAC COMMENT
	BOOL bTagEnable = FALSE;

	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	metadata[EN_LIST_COLUMN_PATH] = filepath;
	metadata[EN_LIST_COLUMN_FILE_NAME] = filename;

	strExt = fileext;
	strExt = strExt.MakeUpper();
//	if (strExt != ".FLAC"){
	if (strExt != _T(".FLAC")){
		return false;
	}

	memset(&stStreaminfo, 0, sizeof(stStreaminfo));

	char *filename_utf8 = convert_to_utf8(filepath);

//	bFlacRet = FLAC__metadata_get_streaminfo(filepath_tmp, &stStreaminfo);
	bFlacRet = FLAC__metadata_get_streaminfo(filename_utf8, &stStreaminfo);
	if(bFlacRet == TRUE){
		samplingrate = stStreaminfo.data.stream_info.sample_rate;
		if(IsSamplingRate(samplingrate) == TRUE){
			FlacCommentInit(&stFlacComm, 1);
			FLAC_Tagdata(filepath, &stFlacComm, &bTagEnable);

			metadata[EN_LIST_COLUMN_TRACK_NO] = stFlacComm.strTracknumber;
			metadata[EN_LIST_COLUMN_TITLE] = stFlacComm.strTitle;
			metadata[EN_LIST_COLUMN_ARTIST] = stFlacComm.strArtist;
			metadata[EN_LIST_COLUMN_ALBUM] = stFlacComm.strAlbum;
			metadata[EN_LIST_COLUMN_DISC_NO] = stFlacComm.strDiscnumber;
			metadata[EN_LIST_COLUMN_DISC_TOTAL] = stFlacComm.strDisctotal;
			metadata[EN_LIST_COLUMN_ALBUM_ARTIST] = stFlacComm.strAlbumArtist;
//			metadata[EN_LIST_COLUMN_SAMPLING_RATE] = to_string(stStreaminfo.data.stream_info.sample_rate);
			strTmp.Format(_T("%d"), stStreaminfo.data.stream_info.sample_rate);
			metadata[EN_LIST_COLUMN_SAMPLING_RATE] = strTmp;
//			metadata[EN_LIST_COLUMN_BIT] = to_string(stStreaminfo.data.stream_info.bits_per_sample) + "bit Int";
			strTmp.Format(_T("%dbit Int"), stStreaminfo.data.stream_info.bits_per_sample);
			metadata[EN_LIST_COLUMN_BIT] = strTmp;
			nPlayTimeSec = stStreaminfo.data.stream_info.total_samples / samplingrate;
			strPlayTime.Format(_T("%3I64d:%02I64d"), nPlayTimeSec / 60, nPlayTimeSec % 60);
			metadata[EN_LIST_COLUMN_LENGTH] = strPlayTime;
//			metadata[EN_LIST_COLUMN_SAMPLING_COUNT] = to_string(stStreaminfo.data.stream_info.total_samples);
			strTmp.Format(_T("%I64d"), stStreaminfo.data.stream_info.total_samples);
			metadata[EN_LIST_COLUMN_SAMPLING_COUNT] = strTmp;
			metadata[EN_LIST_COLUMN_EXT] = _T("FLAC");

			FlacCommentInit(&stFlacComm, 0);

			bRet = true;
		}
	}
//	BOOL bEnable;									// DEBUG
//	FLAC_Tagdata(filepath, &m_stFlacComm, &bEnable); // DEBUG
//	FLAC_PictureCover(filepath, NULL, &bEnable); // DEBUG
//	FlacTagToID3v2Tag(&m_stFlacComm, &m_stID3tag);	// DEBUG

	delete filename_utf8;

	return bRet;
}

// ALAC(.m4a)ファイルチェック及びメタデータ読み取り
bool CPCMDSD_ConverterDlg::ALAC_Metadata(TCHAR *filepath, CString *metadata)
{
	CAlacDecode AlacDecodeObj;
//	STALACTAG stAlacTag;
	STALACFORMAT stAlacFormat;
	BOOL bRet;

	CString strExt;
	CString strTmp;
	INT64 nPlayTimeSec;		// 再生時間(sec)
	CString strPlayTime;

	STFLAC_COMMENT stFlacComm;		// FLAC COMMENT

	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	metadata[EN_LIST_COLUMN_PATH] = filepath;
	metadata[EN_LIST_COLUMN_FILE_NAME] = filename;

	strExt = fileext;
	strExt = strExt.MakeUpper();
	if (strExt != _T(".M4A")) {
		return false;
	}

	bRet = AlacDecodeObj.Open(filepath);
	if (bRet == FALSE) {
		return false;
	}
//	AlacDecodeObj.GetTagInfo(&stAlacTag);
	AlacDecodeObj.GetFormatInfo(&stAlacFormat);
	AlacDecodeObj.Close();

	strTmp.Format(_T("%d"), stAlacFormat.nSampleRate);
	metadata[EN_LIST_COLUMN_SAMPLING_RATE] = strTmp;

	strTmp.Format(_T("%dbit Int"), stAlacFormat.nBitDepth);
	metadata[EN_LIST_COLUMN_BIT] = strTmp;

	nPlayTimeSec = stAlacFormat.nPlayTimeSec;
	strPlayTime.Format(_T("%3I64d:%02I64d"), nPlayTimeSec / 60, nPlayTimeSec % 60);
	metadata[EN_LIST_COLUMN_LENGTH] = strPlayTime;

	strTmp.Format(_T("%I64d"), stAlacFormat.qwTotalSample);
	metadata[EN_LIST_COLUMN_SAMPLING_COUNT] = strTmp;
	metadata[EN_LIST_COLUMN_EXT] = _T("ALAC");

	FlacCommentInit(&stFlacComm, 1);

	ALAC_Tagdata(filepath, &stFlacComm);

	metadata[EN_LIST_COLUMN_TRACK_NO] = stFlacComm.strTracknumber;
	metadata[EN_LIST_COLUMN_TITLE] = stFlacComm.strTitle;
	metadata[EN_LIST_COLUMN_ARTIST] = stFlacComm.strArtist;
	metadata[EN_LIST_COLUMN_ALBUM] = stFlacComm.strAlbum;
	metadata[EN_LIST_COLUMN_DISC_NO] = stFlacComm.strDiscnumber;
	metadata[EN_LIST_COLUMN_DISC_TOTAL] = stFlacComm.strDisctotal;
	metadata[EN_LIST_COLUMN_ALBUM_ARTIST] = stFlacComm.strAlbumArtist;

	FlacCommentInit(&stFlacComm, 0);

	return true;
}

// DFFファイルチェック及びメタデータ読み取り
bool CPCMDSD_ConverterDlg::DFF_Metadata(TCHAR *filepath, CString *metadata)
{
	STDFFHEAD stDffHead;
	BOOL bRet;

	CString strExt;
	CString strTmp;
	INT64 nPlayTimeSec;		// 再生時間(sec)
	CString strPlayTime;

	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	metadata[EN_LIST_COLUMN_PATH] = filepath;
	metadata[EN_LIST_COLUMN_FILE_NAME] = filename;

	strExt = fileext;
	strExt = strExt.MakeUpper();
	if (strExt != _T(".DFF")){
		return false;
	}

	bRet = m_DffToDsfObj.GetHead(filepath, &stDffHead);
	if(bRet == FALSE){
		return false;
	}

	strTmp.Format(_T("%d"), stDffHead.stSampChunk.sampleRate);
	metadata[EN_LIST_COLUMN_SAMPLING_RATE] = strTmp;

	strTmp = _T("1bit Int");
	metadata[EN_LIST_COLUMN_BIT] = strTmp;

	// 再生秒数 = (Dsd Data Samples × 8Bit ÷ 2ch) ÷ サンプリングレート
	nPlayTimeSec = (stDffHead.stDsdDataChunk.stChunk.ckDataSize * 4) / stDffHead.stSampChunk.sampleRate;
	strPlayTime.Format(_T("%3I64d:%02I64d"), nPlayTimeSec / 60, nPlayTimeSec % 60);
	metadata[EN_LIST_COLUMN_LENGTH] = strPlayTime;

//	strTmp.Format(_T("%I64d"), stDffHead.stDsdDataChunk.stChunk.ckDataSize);
	strTmp.Format(_T("%I64d"), stDffHead.stDsdDataChunk.stChunk.ckDataSize * 4);
	metadata[EN_LIST_COLUMN_SAMPLING_COUNT] = strTmp;
	metadata[EN_LIST_COLUMN_EXT] = _T("DFF");

	return true;
}

// FLACファイルタグデータ読み取り メタデータレベル0のインターフェース ※レベル0ではSony Music Center for PC(2019/5月時点)で作成したタグが正常に読めない不具合があるので廃止、レベル1か2好き方を使う。
#if 0
bool CPCMDSD_ConverterDlg::FLAC_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag, BOOL *pbEnable)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	FLAC__bool  bFlacRet;
	FLAC__StreamMetadata *pstMetaData = NULL;
	FLAC__StreamMetadata_VorbisComment *pstVorbisComment;
	FLAC__StreamMetadata_VorbisComment_Entry *pstVorbisCommentEntry;
	bool bRet = false;

	CString strExt;
	unsigned int i;
	
	int		nLen;
	TCHAR*	pszWchar;
	LPCSTR	pszSrcChar;
	int ret;
	CString strFieldID;
	CString strField;
	CString strCommnet;
	CString strRet;
	int iStart;

	*pbEnable = FALSE;

	strExt = fileext;
	strExt = strExt.MakeUpper();
//	if (strExt != ".FLAC"){
	if (strExt != _T(".FLAC")){
		return false;
	}

	// FLAC COMMENTクリア
	FlacCommentInit(pstTag, 1);

	char *filename_utf8 = convert_to_utf8(filepath);

//	bFlacRet = FLAC__metadata_get_tags(filepath_tmp, &pstMetaData);
	bFlacRet = FLAC__metadata_get_tags(filename_utf8, &pstMetaData);
	if (bFlacRet != NULL) {
		bRet = true;
		pstVorbisComment = (FLAC__StreamMetadata_VorbisComment *)&pstMetaData->data;
		for (i = 0;i < pstVorbisComment->num_comments;i ++) {
			pstVorbisCommentEntry = (FLAC__StreamMetadata_VorbisComment_Entry*)&pstVorbisComment->comments[i];
			pstVorbisCommentEntry->length;
			pszSrcChar = (LPCSTR)pstVorbisCommentEntry->entry;

			//Unicodeに必要な文字数の取得
			nLen = ::MultiByteToWideChar(CP_UTF8, 0, pszSrcChar, -1, NULL, 0);
			pszWchar = new TCHAR[nLen];
			if (pszWchar)
			{
				// UTF8→UTF16変換
				nLen = ::MultiByteToWideChar(CP_UTF8, 0, pszSrcChar, (int)pstVorbisCommentEntry->length + 1, pszWchar, nLen);
				strCommnet = pszWchar;
				delete	pszWchar;

				// フィールドID取得
				iStart = 0;
				strFieldID = strCommnet.Tokenize(_T("="), iStart);
				strFieldID = strFieldID.Trim();
				strFieldID = strFieldID.MakeUpper();

				// フィールド取得
				strField = strCommnet.Mid(iStart, strCommnet.GetLength());
				if(strField.GetLength() > 0){
					ret = strFieldID.Compare(_T("ALBUM"));
					if (ret == 0) {
						pstTag->strAlbum = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("ARTIST"));
					if (ret == 0) {
						pstTag->strArtist = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("ALBUMARTIST"));
					if (ret == 0) {
						pstTag->strAlbumArtist = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("PUBLISHER"));
					if (ret == 0) {
						pstTag->strTitle = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("COPYRIGHT"));
					if (ret == 0) {
						pstTag->strCopyright = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("DISCNUMBER"));
					if (ret == 0) {
						pstTag->strDiscnumber = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("DISCTOTAL"));
					if (ret == 0) {
						pstTag->strDisctotal = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("ISRC"));
					if (ret == 0) {
						pstTag->strIsrc = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("UPN"));	// EANは読み捨てとします。
					if (ret == 0) {
						pstTag->strUpn = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("LABEL"));
					if (ret == 0) {
						pstTag->strLabel = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("LABELNO"));
					if (ret == 0) {
						pstTag->strLabelNo = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("LICENSE"));
					if (ret == 0) {
						pstTag->strLicense = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("OPUS"));
					if (ret == 0) {
						pstTag->strOpus = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("SOURCEMEDIA"));
					if (ret == 0) {
						pstTag->strSourcemedia = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("TITLE"));
					if (ret == 0) {
						pstTag->strTitle = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("TRACKNUMBER"));
					if (ret == 0) {
						pstTag->strTracknumber = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("TRACKTOTAL"));
					if (ret == 0) {
						pstTag->strTracktotal = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("VERSION"));
					if (ret == 0) {
						pstTag->strVersion = strField;
						*pbEnable = TRUE;
					}
#if 0				// ファイルを変換するのだからエンコーダー者は引き継がないのがID3v2の仕様と思われる。
					ret = strFieldID.Compare(_T("ENCODED-BY"));
					if (ret == 0) {
						pstTag->strEncodedBy = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("ENCODING"));
					if (ret == 0) {
						pstTag->strEncoded = strField;
						*pbEnable = TRUE;
					}
#endif
					ret = strFieldID.Compare(_T("LYRICS"));
					if (ret == 0) {
						pstTag->strLyrics = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("WEBSITE"));
					if (ret == 0) {
						pstTag->strWebsite = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("COMPOSER"));
					if (ret == 0) {
						pstTag->strComposer = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("ARRANGER"));
					if (ret == 0) {
						pstTag->strArranger = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("LYRICIST"));
					if (ret == 0) {
						pstTag->strLyricist = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("AUTHOR"));
					if (ret == 0) {
						pstTag->strAuthor = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("CONDUCTOR"));
					if (ret == 0) {
						pstTag->strConductor = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("PERFORMER"));
					if (ret == 0) {
						pstTag->strPerformer = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("ENSEMBLE"));
					if (ret == 0) {
						pstTag->strPerformer = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("PART"));
					if (ret == 0) {
						pstTag->strPart = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("PARTNUMBER"));
					if (ret == 0) {
						pstTag->strPartnumber = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("GENRE"));
					if (ret == 0) {
						pstTag->strGenre = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("DATE"));
					if (ret == 0) {
						pstTag->strDate = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("LOCATION"));
					if (ret == 0) {
						pstTag->strLocation = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("COMMENT"));
					if (ret == 0) {
						pstTag->strComment = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("ORGANIZATION"));
					if (ret == 0) {
						pstTag->strOrganization = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("DESCRIPTION"));
					if (ret == 0) {
						pstTag->strDescription = strField;
						*pbEnable = TRUE;
					}
					ret = strFieldID.Compare(_T("CONTACT"));
					if (ret == 0) {
						pstTag->strContact = strField;
						*pbEnable = TRUE;
					}
				}
			}
		}
		FLAC__metadata_object_delete(pstMetaData);
	}

	// Flacカバー画像読み取り
	FLAC__StreamMetadata_Picture *pstPicture = NULL;
	BYTE *pBuff;
	DWORD idx;
	DWORD dwSize;
	DWORD dwMineSize;
	DWORD dwDescriptionSize;

	for(i = 0;i < FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED;i ++){
		switch(i){
			case FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_BACK_COVER:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_LEAFLET_PAGE:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_MEDIA:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_LEAD_ARTIST:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_ARTIST:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_CONDUCTOR:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_BAND:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_COMPOSER:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_LYRICIST:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_RECORDING_LOCATION:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_RECORDING:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_DURING_PERFORMANCE:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_VIDEO_SCREEN_CAPTURE:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_FISH:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_ILLUSTRATION:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_BAND_LOGOTYPE:
			case FLAC__STREAM_METADATA_PICTURE_TYPE_PUBLISHER_LOGOTYPE:
				break;
			default:															// 上記以外は破棄する。
				continue;
		}

//		bFlacRet = FLAC__metadata_get_picture(filepath_tmp, &pstMetaData, (FLAC__StreamMetadata_Picture_Type)i, NULL, NULL, -1, -1, -1, -1);
		bFlacRet = FLAC__metadata_get_picture(filename_utf8, &pstMetaData, (FLAC__StreamMetadata_Picture_Type)i, NULL, NULL, -1, -1, -1, -1);
		if (bFlacRet != NULL) {
			bRet = true;
			*pbEnable = TRUE;

			pstPicture = (FLAC__StreamMetadata_Picture *)&pstMetaData->data;
			dwMineSize = (DWORD)strlen(pstPicture->mime_type) + 1;
			dwDescriptionSize = (DWORD)strlen((char*)pstPicture->description) + 1;

			dwSize = 1;
			dwSize += dwMineSize;
			dwSize += 1;
			dwSize += dwDescriptionSize;
			dwSize += pstPicture->data_length;

			pstTag->stPicture[i].nLength = dwSize;
			pstTag->stPicture[i].pData = new BYTE[dwSize];
			pBuff = pstTag->stPicture[i].pData;
			idx = 0;
			pBuff[idx++] = 0x00;
			memcpy(&pBuff[idx], pstPicture->mime_type, dwMineSize);
			idx += dwMineSize;
			pBuff[idx++] = pstPicture->type;
			memcpy(&pBuff[idx], pstPicture->description, dwDescriptionSize);
			idx += dwDescriptionSize;
			memcpy(&pBuff[idx], pstPicture->data, pstPicture->data_length);
			idx += pstPicture->data_length;

			FLAC__metadata_object_delete(pstMetaData);
		}
	}

	delete filename_utf8;

	return bRet;
}
#endif
//  FLACファイルタグデータ読み取り メタデータレベル1のインターフェース
#if 0
bool CPCMDSD_ConverterDlg::FLAC_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag, BOOL *pbEnable)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

//	FLAC__bool  bFlacRet;
	FLAC__Metadata_SimpleIterator *iterator;
	FLAC__StreamMetadata *pstMetaData = NULL;
	FLAC__StreamMetadata_VorbisComment *pstVorbisComment;
	FLAC__StreamMetadata_VorbisComment_Entry *pstVorbisCommentEntry;
	FLAC__StreamMetadata_Picture *pstPicture = NULL;
	bool bRet = false;

	CString strExt;
	unsigned int i;
	
	int		nLen;
	TCHAR*	pszWchar;
	LPCSTR	pszSrcChar;
	int ret;
	CString strFieldID;
	CString strField;
	CString strCommnet;
	CString strRet;
	int iStart;

	BYTE *pBuff;
	DWORD idx;
	DWORD dwSize;
	DWORD dwMineSize;
	DWORD dwDescriptionSize;

	*pbEnable = FALSE;

	strExt = fileext;
	strExt = strExt.MakeUpper();
//	if (strExt != ".FLAC"){
	if (strExt != _T(".FLAC")){
		return false;
	}

	// FLAC COMMENTクリア
	FlacCommentInit(pstTag, 1);

	char *filename_utf8 = convert_to_utf8(filepath);

	const char *flac_error_msg;

	flac_error_msg = NULL;
	iterator = FLAC__metadata_simple_iterator_new();
//	if ( iterator == NULL || !FLAC__metadata_simple_iterator_init(iterator, filepath_tmp, true, false) )
	if ( iterator == NULL || !FLAC__metadata_simple_iterator_init(iterator, filename_utf8, true, false) )
	{
		if (iterator == NULL )
		{
			flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR];
		}else
		{
			flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(iterator)];
			FLAC__metadata_simple_iterator_delete(iterator);
		}
		delete filename_utf8;
		return FALSE;
	}

	do {
		FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iterator);
		if (block == NULL) {
			break;
		}

		// タグ読み取り
		if(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT){
			bRet = true;
			pstVorbisComment = (FLAC__StreamMetadata_VorbisComment *)&block->data;
			for (i = 0;i < pstVorbisComment->num_comments;i ++) {
				pstVorbisCommentEntry = (FLAC__StreamMetadata_VorbisComment_Entry*)&pstVorbisComment->comments[i];
				pstVorbisCommentEntry->length;
				pszSrcChar = (LPCSTR)pstVorbisCommentEntry->entry;

				//Unicodeに必要な文字数の取得
				nLen = ::MultiByteToWideChar(CP_UTF8, 0, pszSrcChar, -1, NULL, 0);
				pszWchar = new TCHAR[nLen];
				if (pszWchar)
				{
					// UTF8→UTF16変換
					nLen = ::MultiByteToWideChar(CP_UTF8, 0, pszSrcChar, (int)pstVorbisCommentEntry->length + 1, pszWchar, nLen);
					strCommnet = pszWchar;
					delete	pszWchar;

					// フィールドID取得
					iStart = 0;
					strFieldID = strCommnet.Tokenize(_T("="), iStart);
					strFieldID = strFieldID.Trim();
					strFieldID = strFieldID.MakeUpper();

					// フィールド取得
					strField = strCommnet.Mid(iStart, strCommnet.GetLength());
					if(strField.GetLength() > 0){
						ret = strFieldID.Compare(_T("ALBUM"));
						if (ret == 0) {
							pstTag->strAlbum = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ARTIST"));
						if (ret == 0) {
							pstTag->strArtist = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ALBUMARTIST"));
						if (ret == 0) {
							pstTag->strAlbumArtist = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("PUBLISHER"));
						if (ret == 0) {
							pstTag->strTitle = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("COPYRIGHT"));
						if (ret == 0) {
							pstTag->strCopyright = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("DISCNUMBER"));
						if (ret == 0) {
							pstTag->strDiscnumber = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("DISCTOTAL"));
						if (ret == 0) {
							pstTag->strDisctotal = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ISRC"));
						if (ret == 0) {
							pstTag->strIsrc = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("UPN"));	// EANは読み捨てとします。
						if (ret == 0) {
							pstTag->strUpn = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LABEL"));
						if (ret == 0) {
							pstTag->strLabel = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LABELNO"));
						if (ret == 0) {
							pstTag->strLabelNo = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LICENSE"));
						if (ret == 0) {
							pstTag->strLicense = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("OPUS"));
						if (ret == 0) {
							pstTag->strOpus = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("SOURCEMEDIA"));
						if (ret == 0) {
							pstTag->strSourcemedia = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("TITLE"));
						if (ret == 0) {
							pstTag->strTitle = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("TRACKNUMBER"));
						if (ret == 0) {
							pstTag->strTracknumber = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("TRACKTOTAL"));
						if (ret == 0) {
							pstTag->strTracktotal = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("VERSION"));
						if (ret == 0) {
							pstTag->strVersion = strField;
							*pbEnable = TRUE;
						}
#if 0					// ファイルを変換するのだからエンコーダー者は引き継がないのがID3v2の仕様と思われる。
						ret = strFieldID.Compare(_T("ENCODED-BY"));
						if (ret == 0) {
							pstTag->strEncodedBy = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ENCODING"));
						if (ret == 0) {
							pstTag->strEncoded = strField;
							*pbEnable = TRUE;
						}
#endif
						ret = strFieldID.Compare(_T("LYRICS"));
						if (ret == 0) {
							pstTag->strLyrics = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("WEBSITE"));
						if (ret == 0) {
							pstTag->strWebsite = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("COMPOSER"));
						if (ret == 0) {
							pstTag->strComposer = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ARRANGER"));
						if (ret == 0) {
							pstTag->strArranger = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LYRICIST"));
						if (ret == 0) {
							pstTag->strLyricist = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("AUTHOR"));
						if (ret == 0) {
							pstTag->strAuthor = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("CONDUCTOR"));
						if (ret == 0) {
							pstTag->strConductor = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("PERFORMER"));
						if (ret == 0) {
							pstTag->strPerformer = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ENSEMBLE"));
						if (ret == 0) {
							pstTag->strPerformer = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("PART"));
						if (ret == 0) {
							pstTag->strPart = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("PARTNUMBER"));
						if (ret == 0) {
							pstTag->strPartnumber = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("GENRE"));
						if (ret == 0) {
							pstTag->strGenre = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("DATE"));
						if (ret == 0) {
							pstTag->strDate = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LOCATION"));
						if (ret == 0) {
							pstTag->strLocation = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("COMMENT"));
						if (ret == 0) {
							pstTag->strComment = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ORGANIZATION"));
						if (ret == 0) {
							pstTag->strOrganization = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("DESCRIPTION"));
						if (ret == 0) {
							pstTag->strDescription = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("CONTACT"));
						if (ret == 0) {
							pstTag->strContact = strField;
							*pbEnable = TRUE;
						}
					}
				}
			}
		}

		// Flacカバー画像読み取り
		if(block->type == FLAC__METADATA_TYPE_PICTURE){
			bRet = true;
			*pbEnable = TRUE;

			pstPicture = (FLAC__StreamMetadata_Picture *)&block->data;
			i = pstPicture->type;

			dwMineSize = (DWORD)strlen(pstPicture->mime_type) + 1;
			dwDescriptionSize = (DWORD)strlen((char*)pstPicture->description) + 1;

			dwSize = 1;
			dwSize += dwMineSize;
			dwSize += 1;
			dwSize += dwDescriptionSize;
			dwSize += pstPicture->data_length;

			pstTag->stPicture[i].nLength = dwSize;
			pstTag->stPicture[i].pData = new BYTE[dwSize];
			pBuff = pstTag->stPicture[i].pData;
			idx = 0;
			pBuff[idx++] = 0x00;
			memcpy(&pBuff[idx], pstPicture->mime_type, dwMineSize);
			idx += dwMineSize;
			pBuff[idx++] = pstPicture->type;
			memcpy(&pBuff[idx], pstPicture->description, dwDescriptionSize);
			idx += dwDescriptionSize;
			memcpy(&pBuff[idx], pstPicture->data, pstPicture->data_length);
			idx += pstPicture->data_length;
		}

		FLAC__metadata_object_delete(block);
	} while (FLAC__metadata_simple_iterator_next(iterator));

	delete filename_utf8;
	FLAC__metadata_simple_iterator_delete(iterator);

	return bRet;
}
#endif
#if 1
//  FLACファイルタグデータ読み取り メタデータレベル2のインターフェース
bool CPCMDSD_ConverterDlg::FLAC_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag, BOOL *pbEnable)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	FLAC__bool  bFlacRet;
	FLAC__StreamMetadata *pstMetaData = NULL;
	FLAC__StreamMetadata_VorbisComment *pstVorbisComment;
	FLAC__StreamMetadata_VorbisComment_Entry *pstVorbisCommentEntry;
	FLAC__StreamMetadata_Picture *pstPicture = NULL;
	bool bRet = false;

	CString strExt;
	unsigned int i;
	
	int		nLen;
	TCHAR*	pszWchar;
	LPCSTR	pszSrcChar;
	int ret;
	CString strFieldID;
	CString strField;
	CString strCommnet;
	CString strRet;
	int iStart;

	BYTE *pBuff;
	DWORD idx;
	DWORD dwSize;
	DWORD dwMineSize;
	DWORD dwDescriptionSize;

	*pbEnable = FALSE;

	strExt = fileext;
	strExt = strExt.MakeUpper();
//	if (strExt != ".FLAC"){
	if (strExt != _T(".FLAC")){
		return false;
	}

	// FLAC COMMENTクリア
	FlacCommentInit(pstTag, 1);

	FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
	if(chain == 0x00){
		return bRet;
	}

	char *filename_utf8 = convert_to_utf8(filepath);

//	bFlacRet = FLAC__metadata_chain_read(chain, filepath_tmp);
	bFlacRet = FLAC__metadata_chain_read(chain, filename_utf8);
	if (bFlacRet == NULL) {
		delete filename_utf8;
		FLAC__metadata_chain_delete(chain);
		return bRet;
	}

	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
	if(iterator == 0x00){
		delete filename_utf8;
		FLAC__metadata_chain_delete(chain);
		return bRet;
	}

	FLAC__metadata_iterator_init(iterator, chain);

	do {
		FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
		if(block == NULL){
			break;
		}

		// タグ読み取り
		if(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT){
			bRet = true;
			pstVorbisComment = (FLAC__StreamMetadata_VorbisComment *)&block->data;
			for (i = 0;i < pstVorbisComment->num_comments;i ++) {
				pstVorbisCommentEntry = (FLAC__StreamMetadata_VorbisComment_Entry*)&pstVorbisComment->comments[i];
				pstVorbisCommentEntry->length;
				pszSrcChar = (LPCSTR)pstVorbisCommentEntry->entry;

				//Unicodeに必要な文字数の取得
				nLen = ::MultiByteToWideChar(CP_UTF8, 0, pszSrcChar, -1, NULL, 0);
				pszWchar = new TCHAR[nLen];
				if (pszWchar)
				{
					// UTF8→UTF16変換
					nLen = ::MultiByteToWideChar(CP_UTF8, 0, pszSrcChar, (int)pstVorbisCommentEntry->length + 1, pszWchar, nLen);
					strCommnet = pszWchar;
					delete[]	pszWchar;

					// フィールドID取得
					iStart = 0;
					strFieldID = strCommnet.Tokenize(_T("="), iStart);
					strFieldID = strFieldID.Trim();
					strFieldID = strFieldID.MakeUpper();

					// フィールド取得
					strField = strCommnet.Mid(iStart, strCommnet.GetLength());
					TRACE(_T("Flac Field:[%s][%s]\n"), strFieldID.GetBuffer(), strField.GetBuffer());
					if(strField.GetLength() > 0){
						ret = strFieldID.Compare(_T("ALBUM"));
						if (ret == 0) {
							pstTag->strAlbum = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ARTIST"));
						if (ret == 0) {
							pstTag->strArtist = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ALBUMARTIST"));
						if (ret == 0) {
							pstTag->strAlbumArtist = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("PUBLISHER"));
						if (ret == 0) {
							pstTag->strTitle = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("COPYRIGHT"));
						if (ret == 0) {
							pstTag->strCopyright = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("DISCNUMBER"));
						if (ret == 0) {
							pstTag->strDiscnumber = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("DISCTOTAL"));
						if (ret == 0) {
							pstTag->strDisctotal = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ISRC"));
						if (ret == 0) {
							pstTag->strIsrc = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("UPN"));	// EANは読み捨てとします。
						if (ret == 0) {
							pstTag->strUpn = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LABEL"));
						if (ret == 0) {
							pstTag->strLabel = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LABELNO"));
						if (ret == 0) {
							pstTag->strLabelNo = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LICENSE"));
						if (ret == 0) {
							pstTag->strLicense = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("OPUS"));
						if (ret == 0) {
							pstTag->strOpus = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("SOURCEMEDIA"));
						if (ret == 0) {
							pstTag->strSourcemedia = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("TITLE"));
						if (ret == 0) {
							pstTag->strTitle = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("TRACKNUMBER"));
						if (ret == 0) {
							pstTag->strTracknumber = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("TRACKTOTAL"));
						if (ret == 0) {
							pstTag->strTracktotal = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("VERSION"));
						if (ret == 0) {
							pstTag->strVersion = strField;
							*pbEnable = TRUE;
						}
#if 0					// ファイルを変換するのだからエンコーダー者は引き継がないのがID3v2の仕様と思われる。
						ret = strFieldID.Compare(_T("ENCODED-BY"));
						if (ret == 0) {
							pstTag->strEncodedBy = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ENCODING"));
						if (ret == 0) {
							pstTag->strEncoded = strField;
							*pbEnable = TRUE;
						}
#endif
						ret = strFieldID.Compare(_T("LYRICS"));
						if (ret == 0) {
							pstTag->strLyrics = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("WEBSITE"));
						if (ret == 0) {
							pstTag->strWebsite = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("COMPOSER"));
						if (ret == 0) {
							pstTag->strComposer = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ARRANGER"));
						if (ret == 0) {
							pstTag->strArranger = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LYRICIST"));
						if (ret == 0) {
							pstTag->strLyricist = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("AUTHOR"));
						if (ret == 0) {
							pstTag->strAuthor = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("CONDUCTOR"));
						if (ret == 0) {
							pstTag->strConductor = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("PERFORMER"));
						if (ret == 0) {
							pstTag->strPerformer = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ENSEMBLE"));
						if (ret == 0) {
							pstTag->strPerformer = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("PART"));
						if (ret == 0) {
							pstTag->strPart = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("PARTNUMBER"));
						if (ret == 0) {
							pstTag->strPartnumber = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("GENRE"));
						if (ret == 0) {
							pstTag->strGenre = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("DATE"));
						if (ret == 0) {
							pstTag->strDate = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("LOCATION"));
						if (ret == 0) {
							pstTag->strLocation = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("COMMENT"));
						if (ret == 0) {
							pstTag->strComment = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("ORGANIZATION"));
						if (ret == 0) {
							pstTag->strOrganization = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("DESCRIPTION"));
						if (ret == 0) {
							pstTag->strDescription = strField;
							*pbEnable = TRUE;
						}
						ret = strFieldID.Compare(_T("CONTACT"));
						if (ret == 0) {
							pstTag->strContact = strField;
							*pbEnable = TRUE;
						}
					}
				}
			}
		}

		// Flacカバー画像読み取り
		if(block->type == FLAC__METADATA_TYPE_PICTURE){
			bRet = true;
			*pbEnable = TRUE;

			pstPicture = (FLAC__StreamMetadata_Picture *)&block->data;
			i = pstPicture->type;

			dwMineSize = (DWORD)strlen(pstPicture->mime_type) + 1;
			dwDescriptionSize = (DWORD)strlen((char*)pstPicture->description) + 1;

			dwSize = 1;
			dwSize += dwMineSize;
			dwSize += 1;
			dwSize += dwDescriptionSize;
			dwSize += pstPicture->data_length;

			pstTag->stPicture[i].nLength = dwSize;
			pstTag->stPicture[i].pData = new BYTE[dwSize];
			pBuff = pstTag->stPicture[i].pData;
			idx = 0;
			pBuff[idx++] = 0x00;
			memcpy(&pBuff[idx], pstPicture->mime_type, dwMineSize);
			idx += dwMineSize;
			pBuff[idx++] = pstPicture->type;
			memcpy(&pBuff[idx], pstPicture->description, dwDescriptionSize);
			idx += dwDescriptionSize;
			memcpy(&pBuff[idx], pstPicture->data, pstPicture->data_length);
			idx += pstPicture->data_length;
		}
	} while(FLAC__metadata_iterator_next(iterator));

	delete filename_utf8;
	FLAC__metadata_iterator_delete(iterator);
	FLAC__metadata_chain_delete(chain);

	return bRet;
}
#endif

//  ALACファイルタグデータ読み取り
bool CPCMDSD_ConverterDlg::ALAC_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag)
{
	STALACTAG stAlacTag;
	int i;
	BOOL bRet;

	// FLAC COMMENTクリア
	FlacCommentInit(pstTag, 1);

	// ALACオープン
	bRet = m_AlacDecodeObj.Open(filepath);
	if (bRet == FALSE) {
		return false;
	}
	// タグ取得
	m_AlacDecodeObj.GetTagInfo(&stAlacTag);

	// アルバム
	pstTag->strAlbum = stAlacTag.strAlbum;
	// アーティスト
	pstTag->strArtist = stAlacTag.strArtist;
	// アルバムアーティスト
	pstTag->strAlbumArtist = stAlacTag.strAlbumArtist;
	// 曲タイトル
	pstTag->strTitle = stAlacTag.strTitle;
	// トラックNo
	pstTag->strTracknumber = stAlacTag.strTrack;
	// トラックTotal
	pstTag->strTracktotal = stAlacTag.strTotalTracks;
	// Disc No
	pstTag->strDiscnumber = stAlacTag.strDisc;
	// Disc Total
	pstTag->strDisctotal = stAlacTag.strTotalDiscs;
	// ジャンル
	pstTag->strGenre = stAlacTag.strGenre;
	// コメント
	pstTag->strComment = stAlacTag.strComment;
	// 作曲家
	pstTag->strComposer = stAlacTag.strWriter;
	// 日付
	pstTag->strDate = stAlacTag.strDate;
	// 著作権
	pstTag->strCopyright = stAlacTag.strCopyright;
#if 1
	int idxlist[] = { 3, 0 };	// 0:[3]FRONT COVER 1:[0]OTHER
	int idx;

	DWORD dwPictureSize;
	BYTE *pPictureData;
	DWORD dwSize;
	BYTE *pBuff;
	BYTE JPEGHEAD[] = {0xFF,0xD8};
	BYTE PNGHEAD[] = {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A};

	// カバー 最初のFRONTカバーと2枚目のOTHERカバーのみとしそれ以外は読み捨て ※JPEG/PNGをサポート　それ以外は読み捨て
	// ※3枚目以降は全部OTHERカバー扱いになっているようなので。
	for (i = 0; i < 2; i++) {
		dwPictureSize = stAlacTag.stCover[i].dwPictureSize;
		if (dwPictureSize > 0) {
			pPictureData = stAlacTag.stCover[i].pPictureData;
			if (memcmp(pPictureData, JPEGHEAD, 2) == 0) {
				// JPEG
				idx = idxlist[i];

				dwSize = dwPictureSize + 14;
				pstTag->stPicture[idx].nLength = dwSize;
				pstTag->stPicture[idx].pData = new BYTE[dwSize];
				pBuff = pstTag->stPicture[idx].pData;

				pBuff[0] = 0x00;
				memcpy(&pBuff[1], "image/jpeg", 10);
				pBuff[11] = 0x00;
				pBuff[12] = idx;
				pBuff[13] = 0x00;
				memcpy(&pBuff[14], pPictureData, dwPictureSize);
			} else if (memcmp(pPictureData, PNGHEAD, 8) == 0) {
				// PNG
				idx = idxlist[i];

				dwSize = dwPictureSize + 13;
				pstTag->stPicture[idx].nLength = dwSize;
				pstTag->stPicture[idx].pData = new BYTE[dwSize];
				pBuff = pstTag->stPicture[idx].pData;

				pBuff[0] = 0x00;
				memcpy(&pBuff[1], "image/png", 9);
				pBuff[10] = 0x00;
				pBuff[11] = idx;
				pBuff[12] = 0x00;
				memcpy(&pBuff[13], pPictureData, dwPictureSize);
			}
		}
	}
#endif
	// クローズ
	m_AlacDecodeObj.Close();

	return true;
}

//  WAVファイルタグ(RIFF)データ読み取り
bool CPCMDSD_ConverterDlg::WAV_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag, BOOL *pbEnable)
{
	CRiffSIF riff;
	CString strTag;

	*pbEnable = FALSE;

	// FLAC COMMENTクリア
	FlacCommentInit(pstTag, 1);

	if(riff.Load(filepath,'W','A','V','E') != ERROR_SUCCESS){
		return false;
	}

    //INAM/ISBJ タイトル
	strTag = riff.GetField('I', 'N', 'A', 'M');
	if(strTag.GetLength() == 0){
		strTag = riff.GetField('I','S','B','J');
	}
	if(strTag.GetLength() > 0){
		pstTag->strTitle = strTag;
		*pbEnable = TRUE;
	}
	//IART アーティスト名
	strTag = riff.GetField('I', 'A', 'R', 'T');
	if(strTag.GetLength() > 0){
		pstTag->strArtist = strTag;
#if ALBUM_MODE_WAV_TAG_ENABLE
		// WAVはアルバムアーティストがないのでアーティストをアルバムアーティストタグとする
		pstTag->strAlbumArtist = strTag;
#endif
		*pbEnable = TRUE;
	}
	//IPRD アルバム名
	strTag = riff.GetField('I', 'P', 'R', 'D');
	if(strTag.GetLength() > 0){
		pstTag->strAlbum = strTag;
		*pbEnable = TRUE;
	}
	//ICMT コメント
	strTag = riff.GetField('I', 'C', 'M', 'T');
	if(strTag.GetLength() > 0){
		pstTag->strComment = strTag;
		*pbEnable = TRUE;
	}
	//ICRD 日付
	strTag = riff.GetField('I', 'C', 'R', 'D');
	if(strTag.GetLength() > 0){
		pstTag->strDate = strTag;
		*pbEnable = TRUE;
	}
	//IGNR ジャンル
	strTag = riff.GetField('I', 'G', 'N', 'R');
	if(strTag.GetLength() > 0){
		pstTag->strGenre = strTag;
		*pbEnable = TRUE;
	}
	//ICOP 著作権
	strTag = riff.GetField('I', 'C', 'O', 'P');
	if(strTag.GetLength() > 0){
		pstTag->strCopyright = strTag;
		*pbEnable = TRUE;
	}

	//ITRK トラック番号
	strTag = riff.GetField('I', 'T', 'R', 'K');
	if (strTag.GetLength() > 0) {
		pstTag->strTracknumber = strTag;
		*pbEnable = TRUE;
	}

	//IENG エンジニア ※ID3V2タグには保持出来ない
//	strTag = riff.GetField('I', 'E', 'N', 'G');
	//ISRC ソース ※PCM-DSD Converterの情報を格納するので破棄
//	strTag = riff.GetField('I', 'S', 'R', 'C');
	//ISFT ソフトウェア ※ID3V2タグには保持出来ない
//	strTag = riff.GetField('I', 'S', 'F', 'T');

	return true;
}

// WAV/WAVE64ファイル用タグデータ設定
UINT CPCMDSD_ConverterDlg::SetTagdataForWav(TCHAR *filepath, int nTrackNo, int nTrackTotal, PSTID3TAGINFO pstID3v2Tag)
{
	CString strTemp;

	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	// アルバム ※最後のディレクト名をアルバムに設定
	PathRemoveFileSpec(dir);
	strTemp = ::PathFindFileName(dir);
	strTemp += m_evAlbumTagSuffix;
	pstID3v2Tag->strTALB = strTemp;

	// タイトル ※ファイル名をタイトルに設定
	pstID3v2Tag->strTIT2 = filename;

	// トラックNo
	strTemp.Format(_T("%d/%d"), nTrackNo, nTrackTotal);
	pstID3v2Tag->strTRCK = strTemp;

	m_nTagMode = 1;

	return 1;
}


// FLAC COMMENTタグ初期化
// mode 0:初期化
//      1:破棄 ※画像データがある場合はメモリリークするので注意すること
void CPCMDSD_ConverterDlg::FlacCommentInit(PSTFLAC_COMMENT pstTag, int mode)
{
	int i;

	// FLAC COMMENTクリア
	pstTag->strAlbum = _T("");
	pstTag->strArtist = _T("");
	pstTag->strAlbumArtist = _T("");
	pstTag->strPublisher = _T("");
	pstTag->strCopyright = _T("");
	pstTag->strDiscnumber = _T("");
	pstTag->strDisctotal = _T("");
	pstTag->strIsrc = _T("");
	pstTag->strUpn = _T("");
	pstTag->strLabel = _T("");
	pstTag->strLabelNo = _T("");
	pstTag->strLicense = _T("");
	pstTag->strOpus = _T("");
	pstTag->strSourcemedia = _T("");
	pstTag->strTitle = _T("");
	pstTag->strTracknumber = _T("");
	pstTag->strTracktotal = _T("");
	pstTag->strVersion = _T("");
	pstTag->strEncodedBy = _T("");
	pstTag->strEncoded = _T("");
	pstTag->strLyrics = _T("");
	pstTag->strWebsite = _T("");
	pstTag->strComposer = _T("");
	pstTag->strArranger = _T("");
	pstTag->strLyricist = _T("");
	pstTag->strAuthor = _T("");
	pstTag->strConductor = _T("");
	pstTag->strPerformer = _T("");
	pstTag->strEnsemble = _T("");
	pstTag->strPart = _T("");
	pstTag->strPartnumber = _T("");
	pstTag->strGenre = _T("");
	pstTag->strDate = _T("");
	pstTag->strLocation = _T("");
	pstTag->strComment = _T("");
	pstTag->strOrganization = _T("");
	pstTag->strDescription = _T("");
	pstTag->strContact = _T("");
	if(mode == 1){
		memset(pstTag->stPicture, NULL, sizeof(pstTag->stPicture));
	} else {
		for(i = 0;i < FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED;i ++){
			if(pstTag->stPicture[i].pData != NULL){
				delete pstTag->stPicture[i].pData;
				pstTag->stPicture[i].pData = NULL;
				pstTag->stPicture[i].nLength = 0;
			}
		}
	}
}

// ID3v2タグ情報初期化
void CPCMDSD_ConverterDlg::ID3v2TagInfoInit()
{
	int i;

	m_stID3tag.strTALB = _T("");			// アルバム
	m_stID3tag.strTPE1 = _T("");			// アーティスト
	m_stID3tag.strTPE2 = _T("");			// バンド※アーティスト
	m_stID3tag.strTIT2 = _T("");			// 曲タイトル
	m_stID3tag.strTRCK = _T("");			// トラックNo　※ ISO-8859-1
	m_stID3tag.strTPOS = _T("");			// セット中の位置 ※CD枚数 2枚組なら「1/2」「2/2」など
	m_stID3tag.strTCON = _T("");			// ジャンル
	m_stID3tag.strCOMM = _T("");			// コメント
	m_stID3tag.strTCOM = _T("");			// 作曲家
	m_stID3tag.strTYER = _T("");			// 年　※ ISO-8859-1
	m_stID3tag.strTSSE = _T("");			// ソフトウェア
	m_stID3tag.strTCOP = _T("");			// 著作権
	m_stID3tag.strTENC = _T("");			// エンコードした人
	m_stID3tag.strTEXT = _T("");			// 作詞者
	m_stID3tag.strTXXX = _T("");			// ユーザー定義

	for(i = 0;i < FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED;i ++){
		if(m_stID3tag.stPicture[i].pData != NULL){
			delete m_stID3tag.stPicture[i].pData;
			m_stID3tag.stPicture[i].pData = NULL;
			m_stID3tag.stPicture[i].nLength = 0;
		}
	}

	m_nTagMode = 0;
}

// ID3v2タグ情報初期化
// mode 0:初期化 1:破棄 ※画像データがメモリリークするので別の所で破棄する事。
void CPCMDSD_ConverterDlg::ID3v2TagInfoClear(STID3TAGINFO *pstID3Tag, int mode)
{
	int i;

	pstID3Tag->strTALB = _T("");			// アルバム
	pstID3Tag->strTPE1 = _T("");			// アーティスト
	pstID3Tag->strTPE2 = _T("");			// バンド※アーティスト
	pstID3Tag->strTIT2 = _T("");			// 曲タイトル
	pstID3Tag->strTRCK = _T("");			// トラックNo　※ ISO-8859-1
	pstID3Tag->strTPOS = _T("");			// セット中の位置 ※CD枚数 2枚組なら「1/2」「2/2」など
	pstID3Tag->strTCON = _T("");			// ジャンル
	pstID3Tag->strCOMM = _T("");			// コメント
	pstID3Tag->strTCOM = _T("");			// 作曲家
	pstID3Tag->strTYER = _T("");			// 年　※ ISO-8859-1
	pstID3Tag->strTSSE = _T("");			// ソフトウェア
	pstID3Tag->strTCOP = _T("");			// 著作権
	pstID3Tag->strTENC = _T("");			// エンコードした人
	pstID3Tag->strTEXT = _T("");			// 作詞者
	m_stID3tag.strTXXX = _T("");			// ユーザー定義

	for(i = 0;i < FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED;i ++){
		if(mode == 0){
			if(pstID3Tag->stPicture[i].pData != NULL){
				delete pstID3Tag->stPicture[i].pData;
				pstID3Tag->stPicture[i].pData = NULL;
				pstID3Tag->stPicture[i].nLength = 0;
			}
		} else {
			pstID3Tag->stPicture[i].pData = NULL;
			pstID3Tag->stPicture[i].nLength = 0;
		}
	}

	m_nTagMode = 0;
}

// ID3v2タグ情報初期化 ※アルバムモード用
void CPCMDSD_ConverterDlg::ID3v2TagInfoDataIdxArrayClear()
{
	ST_64BIT_DATA_INDEX stDataIndex;
	int cnt = (int)m_DataIdxArray.GetCount();
	int i;

	for(i = 0;i < cnt;i ++){
		stDataIndex = m_DataIdxArray.GetAt(i);
		if(m_stID3tag.stPicture->pData == stDataIndex.stID3tag.stPicture->pData){
			ID3v2TagInfoClear(&m_stID3tag, 1);
		}
		ID3v2TagInfoClear(&stDataIndex.stID3tag, 0);
	}

	m_DataIdxArray.RemoveAll();
}

// ID3v2タグソフトウェア設定
void CPCMDSD_ConverterDlg::SetID3v2SoftwareTag(CString strSoftware, PSTID3TAGINFO pstID3v2Tag)
{
	if (strSoftware.GetLength() == 0) {
		pstID3v2Tag->strTSSE = _T("");
	} else {
		pstID3v2Tag->strTSSE = strSoftware;
		// タグあり設定
		m_nTagMode = 1;
	}
}

// ID3v2タグエンコーダー者設定
void CPCMDSD_ConverterDlg::SetID3v2EncodedByTag(CString strEncodedBy, PSTID3TAGINFO pstID3v2Tag)
{
	if (strEncodedBy.GetLength() == 0) {
		pstID3v2Tag->strTENC = _T("");
	} else {
		pstID3v2Tag->strTENC = strEncodedBy;
		// タグあり設定
		m_nTagMode = 1;
	}
}

// ID3v2タグユーザー定義設定
void CPCMDSD_ConverterDlg::SetID3v2UserTag(CString strUser, PSTID3TAGINFO pstID3v2Tag)
{
	if (strUser.GetLength() == 0) {
		pstID3v2Tag->strTXXX = _T("");
	} else {
		pstID3v2Tag->strTXXX = strUser;
		// タグあり設定
		m_nTagMode = 1;
	}
}

// ID3v2タグユーザー定義にオリジナルフォーマット、オプション設定
void CPCMDSD_ConverterDlg::SetID3v2UserTagOriginalFormat(int nMode, CString strFile, DWORD dwSamplingRate, DWORD dwBitDepth, PSTID3TAGINFO pstID3v2Tag)
{
	CString strTXXX;
	CString strTmp;
	double dSamplingRate;
	int Precision;

	if (strFile.GetLength() == 0) {
		return;
	}

	//*** オリジナルフォーマット ***
	dSamplingRate = dwSamplingRate;

	if(nMode == 0){
		// PCM Format(WAV/FALC/ALAC)
		strTmp.Format(_T("(%.1fKHz/%dBit)"), dSamplingRate / 1000, dwBitDepth);
	} else {
		// DSD Format(DFF)
		strTmp.Format(_T("(%.1fMHz/%dBit)"), dSamplingRate / 1000000, dwBitDepth);
	}

	strTXXX = _T("Original File:");
	strTXXX += strFile;
	strTXXX += strTmp;

	// PCM Format(WAV/FALC/ALAC)？
	if(nMode == 0){
		//*** オプション ***
		// 精度(フィルター)
		strTXXX += _T(", ");
		strTXXX += _T("Filter Option:");
		Precision = m_ccPrecision.GetCurSel();
		if (Precision == 0) {
			strTXXX += _T("FIR(High Quality)");
		} else {
			strTXXX += _T("IIR(Low Quality)");
		}
		// ゲイン
		strTXXX += _T(", ");
		strTXXX += _T("Gain Option:");
		if(m_radioGainModeDdv == 0){
			m_combVol.GetWindowText(strTmp);
			strTXXX += strTmp;
			strTXXX += _T("dB Lower");
			// ノーマライズ
			strTXXX += _T(", ");
			strTXXX += _T("Normalize Option:");
			if(m_NormalizeEnable == TRUE){
				strTXXX += _T("ON");
			} else {
				strTXXX += _T("OFF");
			}
		} else {
			m_combLimitVol.GetWindowText(strTmp);
			strTXXX += strTmp;
			strTXXX += _T("dB Limit");
			if(m_CrossGainLevel == 1){
				strTXXX += _T(", ");
				m_combVol.GetWindowText(strTmp);
				strTXXX += strTmp;
				strTXXX += _T("dB Lower");
			}
		}
	}

	// ID3v2タグユーザー定義設定
	SetID3v2UserTag(strTXXX, &m_stID3tag);
}

// ログインユーザー取得
CString CPCMDSD_ConverterDlg::GetUserName()
{
	TCHAR cUser[1024];
	DWORD dwUserSize;
	CString strUser;

	dwUserSize = sizeof(cUser) / sizeof(cUser[0]);

	strUser = "";

	if ( ::GetUserName(cUser, &dwUserSize) ){
		strUser = cUser;
	}

	return strUser;
}

// Flacタグ情報 to ID3v2タグ情報コピー
UINT CPCMDSD_ConverterDlg::FlacTagToID3v2Tag(PSTFLAC_COMMENT pstFlacTag, PSTID3TAGINFO pstID3v2Tag)
{
	int i;
	int nTagMode = 0;
	CString strTmp;
	CString strCmp;

	// アルバム
	if (pstFlacTag->strAlbum.GetLength() > 0){
		pstID3v2Tag->strTALB = pstFlacTag->strAlbum;
		pstID3v2Tag->strTALB += m_evAlbumTagSuffix;	// アルバムタグサフィックス付加
		nTagMode = 1;
	}
	// アーティスト
	if (pstFlacTag->strArtist.GetLength() > 0) {
		pstID3v2Tag->strTPE1 = pstFlacTag->strArtist;
		nTagMode = 1;
	}
	// アルバムアーティスト
	if (pstFlacTag->strAlbumArtist.GetLength() > 0) {
		pstID3v2Tag->strTPE2 = pstFlacTag->strAlbumArtist;
		nTagMode = 1;
	}
	// タイトル
	if (pstFlacTag->strTitle.GetLength() > 0) {
		pstID3v2Tag->strTIT2 = pstFlacTag->strTitle;
		nTagMode = 1;
	}
	// トラックNo
	if (pstFlacTag->strTracknumber.GetLength() > 0) {
		pstID3v2Tag->strTRCK = pstFlacTag->strTracknumber;
		// トラックトータル
		if (pstFlacTag->strTracktotal.GetLength() > 0) {
			pstID3v2Tag->strTRCK += _T("/");
			pstID3v2Tag->strTRCK += pstFlacTag->strTracktotal;
		}
		nTagMode = 1;
	}
	// ディスクNo
	if (pstFlacTag->strDiscnumber.GetLength() > 0) {
		pstID3v2Tag->strTPOS = pstFlacTag->strDiscnumber;
		// ディスクトータル
		if (pstFlacTag->strDisctotal.GetLength() > 0) {
			pstID3v2Tag->strTPOS += _T("/");
			pstID3v2Tag->strTPOS += pstFlacTag->strDisctotal;
		}
		nTagMode = 1;
	}
	// ジャンル
	if (pstFlacTag->strGenre.GetLength() > 0) {
		pstID3v2Tag->strTCON = pstFlacTag->strGenre;
		nTagMode = 1;
	}
	// コメント
	if (pstFlacTag->strComment.GetLength() > 0) {
		pstID3v2Tag->strCOMM = pstFlacTag->strComment;
		nTagMode = 1;
	}
	// 作曲者
	if (pstFlacTag->strComposer.GetLength() > 0) {
		pstID3v2Tag->strTCOM = pstFlacTag->strComposer;
		nTagMode = 1;
	}
	// 年 ※'YYYY-MM-DD'だった場合→'YYYY'として取り出す
	if (pstFlacTag->strDate.GetLength() > 0) {
		strTmp = pstFlacTag->strDate.Left(4); // 先頭4文字取り出してみる
		strCmp = strTmp.SpanIncluding(_T("0123456789"));	// 数字のみ取り出してみる
		if (strTmp == strCmp) {
			// 全部数字なら'YYYY'とみなしてそれだけコピーする
			pstID3v2Tag->strTYER = strTmp;
		} else {
			// 数字以外が入ってるなら全部コピーする
			pstID3v2Tag->strTYER = pstFlacTag->strDate;
		}
		nTagMode = 1;
	}
	// 著作権
	if (pstFlacTag->strCopyright.GetLength() > 0) {
		pstID3v2Tag->strTCOP = pstFlacTag->strCopyright;
		nTagMode = 1;
	}
	// 作詞者 ※作詞家
	if (pstFlacTag->strLyricist.GetLength() > 0) {
		pstID3v2Tag->strTEXT = pstFlacTag->strLyricist;
		nTagMode = 1;
	}


	// ピクチャー
	for (i = 0;i < FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED; i++) {
		if (pstFlacTag->stPicture[i].nLength > 0) {
//			pstID3v2Tag->stPicture[i].nLength = pstFlacTag->stPicture[i].nLength;
//			pstID3v2Tag->stPicture[i].pData = pstFlacTag->stPicture[i].pData;
			pstID3v2Tag->stPicture[i] = pstFlacTag->stPicture[i];
			nTagMode = 1;
		}
	}

	return nTagMode;
}

// DSDIFF形式で書き込み
bool CPCMDSD_ConverterDlg::DSD_Write(FILE *LData, FILE *RData, FILE *WriteData, int number){
//	int OrigSamplingRate = _ttoi(m_lFileList.GetItemText(number, EN_LIST_COLUMN_SAMPLING_RATE));
	int OrigSamplingRate = m_SrcPcmSamplingrate;

	int BaseSamplingRate;
	if (OrigSamplingRate % 44100 == 0){
		BaseSamplingRate = 44100;
	}
	else{
		BaseSamplingRate = 48000;
	}

	unsigned __int64 OrigDataSize = m_OrigDataSize  / ((m_SrcPcmBit / 8) * 2);

	int DSD_SamplingRate;
#if 0
	switch (m_cSamplingRate.GetCurSel()) {
	case 0:
		DSD_SamplingRate = BaseSamplingRate * 64;
		break;
	case 1:
		DSD_SamplingRate = BaseSamplingRate * 128;
		break;
	case 2:
		DSD_SamplingRate = BaseSamplingRate * 256;
		break;
	case 3:
		DSD_SamplingRate = BaseSamplingRate * 512;
		break;
	case 4:
		DSD_SamplingRate = BaseSamplingRate * 1024;
		break;
	case 5:
		DSD_SamplingRate = BaseSamplingRate * 2048;
		break;
	}
#else
	// DSDサンプリングレート
	// 64(2.8MHz)
	// 128(5.6MHz)
	// 256(11.2MHz)
	// 512(22.5MHz)
	// 1024(45.1MHz)
	// 2048(90.3MHz)
	DSD_SamplingRate = BaseSamplingRate * m_DSD_Times;
#endif
	unsigned __int64 DSD_SampleSize = OrigDataSize*(DSD_SamplingRate / OrigSamplingRate);
	unsigned __int64 DSD_DataSize = DSD_SampleSize / 4;
	_fseeki64(LData, 0, SEEK_END);
	_fseeki64(RData, 0, SEEK_END);
	_fseeki64(LData, _ftelli64(LData) - DSD_SampleSize, SEEK_SET);
	_fseeki64(RData, _ftelli64(RData) - DSD_SampleSize, SEEK_SET);


	fwrite("FRM8", 4, 1, WriteData);//FRM8
	unsigned __int64 binary = 0;
	binary = reverse_endian(DSD_DataSize - 210);
	fwrite(&binary, 8, 1, WriteData);//chunksize
	fwrite("DSD ", 4, 1, WriteData);//DSD

	fwrite("FVER", 4, 1, WriteData);//FVER
	binary = 0;
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	binary = reverse_endian(4);
	fwrite(&binary, 4, 1, WriteData);//Chunksize

	//Version
	binary = 1;
	fwrite(&binary, 1, 1, WriteData);
	binary = 5;
	fwrite(&binary, 1, 1, WriteData);
	binary = 0;
	fwrite(&binary, 1, 1, WriteData);
	binary = 0;
	fwrite(&binary, 1, 1, WriteData);

	fwrite("PROP", 4, 1, WriteData);//PROP
	binary = 0;
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	binary = reverse_endian(74);
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	fwrite("SND ", 4, 1, WriteData);//SND

	fwrite("FS  ", 4, 1, WriteData);//FS
	binary = 0;
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	binary = reverse_endian(4);
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	unsigned __int32 binary1;
	binary1 = reverse_endian(DSD_SamplingRate);
	fwrite(&binary1, 4, 1, WriteData);//SamplingRate

	fwrite("CHNL", 4, 1, WriteData);//CHNL
	binary = 0;
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	binary = reverse_endian(10);
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	binary = 0;//number of channel
	fwrite(&binary, 1, 1, WriteData);
	binary = 2;
	fwrite(&binary, 1, 1, WriteData);
	fwrite("SLFT", 4, 1, WriteData);//SLFT
	fwrite("SRGT", 4, 1, WriteData);//SRGT

	fwrite("CMPR", 4, 1, WriteData);//CMPR
	binary = 0;
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	binary = reverse_endian(20);
	fwrite(&binary, 4, 1, WriteData);//Chunksize
	fwrite("DSD ", 4, 1, WriteData);//DSD
	binary = 14;
	fwrite(&binary, 1, 1, WriteData);
	fwrite("not compressed ", 15, 1, WriteData);//not compressed

	fwrite("DSD ", 4, 1, WriteData);//DSD
	binary = reverse_endian(DSD_DataSize);
	fwrite(&binary, 8, 1, WriteData);//Chunksize
	unsigned __int64 i = 0;
	int buffersize = 16384 * 2 * 8;
	unsigned char *onebit = new unsigned char[buffersize / 4];
	unsigned char *tmpdataL = new unsigned char[buffersize];
	unsigned char *tmpdataR = new unsigned char[buffersize];
	unsigned char tmpL = 0; unsigned char tmpR = 0;
	int n = 0;
	int p = 0;
	int t = 0;
	int k = 0;
	//WAV_FilterはLRごとUnsignedCharで書き出しているので、それを8サンプル1バイトにまとめてから
	//データ領域として書き出す
	for (i = 0; i < DSD_SampleSize / buffersize; i++){
		p = 0;
		fread(tmpdataL, 1, buffersize, LData);
		fread(tmpdataR, 1, buffersize, RData);
		for (k = 0; k < buffersize / 4; k++){
			onebit[k] = tmpdataL[p] << 7;
			onebit[k] += tmpdataL[p + 1] << 6;
			onebit[k] += tmpdataL[p + 2] << 5;
			onebit[k] += tmpdataL[p + 3] << 4;
			onebit[k] += tmpdataL[p + 4] << 3;
			onebit[k] += tmpdataL[p + 5] << 2;
			onebit[k] += tmpdataL[p + 6] << 1;
			onebit[k] += tmpdataL[p + 7] << 0;
			k++;
			onebit[k] = tmpdataR[p] << 7;
			onebit[k] += tmpdataR[p + 1] << 6;
			onebit[k] += tmpdataR[p + 2] << 5;
			onebit[k] += tmpdataR[p + 3] << 4;
			onebit[k] += tmpdataR[p + 4] << 3;
			onebit[k] += tmpdataR[p + 5] << 2;
			onebit[k] += tmpdataR[p + 6] << 1;
			onebit[k] += tmpdataR[p + 7] << 0;
			p += 8;
		}
		fwrite(onebit, 1, buffersize / 4, WriteData);//DSD_Data
	}
	while (!feof(LData)){
		fread(tmpdataL, 1, 8, LData);
		fread(tmpdataR, 1, 8, RData);
		for (n = 0; n < 8; n++){
			if (tmpdataL[n] == 1){
				tmpL += unsigned char(pow(2, 7 - n));
			}
			if (tmpdataR[n] == 1){
				tmpR += unsigned char(pow(2, 7 - n));
			}
		}
		fwrite(&tmpL, 1, 1, WriteData);
		fwrite(&tmpR, 1, 1, WriteData);
		tmpL = 0;
		tmpR = 0;
	}
	delete[] onebit;
	delete[] tmpdataL;
	delete[] tmpdataR;
	return true;
}


//DSF形式で書き込み
#define CHUNK_SIZE_DSD				28
#define CHUNK_SIZE_FMT				52
#define CHUNK_DATA_HEADER_SIZE		12
#define BLOCK_SIZE_PER_CHANNEL		4096

bool CPCMDSD_ConverterDlg::DSD_WriteDSF(FILE *LData, FILE *RData, INT64 iSeekOffset, FILE *WriteData, int number)
{
	size_t ullFwRet;
	bool bRet = true;
	unsigned __int64 binary = 0;

	int OrigSamplingRate = m_SrcPcmSamplingrate;

	int BaseSamplingRate;
	if (OrigSamplingRate % 44100 == 0){
		BaseSamplingRate = 44100;
	}
	else{
		BaseSamplingRate = 48000;
	}

	unsigned __int64 OrigDataSize = m_OrigDataSize  / ((m_SrcPcmBit / 8) * 2);
	int DSD_SamplingRate;

#if 0
	switch (m_cSamplingRate.GetCurSel()) {
	case 0:											// DSD 64(2.8MHz)
		DSD_SamplingRate = BaseSamplingRate * 64;
		break;
	case 1:											// DSD128(5.6MHz)
		DSD_SamplingRate = BaseSamplingRate * 128;
		break;
	case 2:											// DSD256(11.2MHz)
		DSD_SamplingRate = BaseSamplingRate * 256;
		break;
	case 3:											// DSD512(22.5MHz)
		DSD_SamplingRate = BaseSamplingRate * 512;
		break;
	case 4:											// DSD1024(45.1MHz)
		DSD_SamplingRate = BaseSamplingRate * 1024;
		break;
	case 5:											// DSD2048(90.3MHz)
		DSD_SamplingRate = BaseSamplingRate * 2048;
		break;
	}
#else
	// DSDサンプリングレート
	// 64(2.8MHz)
	// 128(5.6MHz)
	// 256(11.2MHz)
	// 512(22.5MHz)
	// 1024(45.1MHz)
	// 2048(90.3MHz)
	DSD_SamplingRate = BaseSamplingRate * m_DSD_Times;
#endif

	unsigned __int64 DSD_SampleSize;
	unsigned __int64 DSD_DataSize;
	unsigned __int64 size_of_data_chunk;
	unsigned __int64 block_size_per_ch;
	unsigned __int64 block_cnt;

	DSD_SampleSize = OrigDataSize*(DSD_SamplingRate / OrigSamplingRate);
	// Block Cnt = Sample ÷ 8byte ÷ 4096Byte 切り上げ  
	block_cnt = (DSD_SampleSize / 32768) + ((DSD_SampleSize % 32768) != 0);
	// Data Size = Block × 4096 × ch
	DSD_DataSize = block_cnt * BLOCK_SIZE_PER_CHANNEL * 2;
	size_of_data_chunk = CHUNK_DATA_HEADER_SIZE + DSD_DataSize;
	block_size_per_ch = BLOCK_SIZE_PER_CHANNEL;

//	_fseeki64(LData, 0, SEEK_END);
//	_fseeki64(RData, 0, SEEK_END);
//	_fseeki64(LData, _ftelli64(LData) - DSD_SampleSize, SEEK_SET);
//	_fseeki64(RData, _ftelli64(RData) - DSD_SampleSize, SEEK_SET);
#if 1
//	_fseeki64(LData, m_fillsize, SEEK_SET);
//	_fseeki64(RData, m_fillsize, SEEK_SET);
	_fseeki64(LData, iSeekOffset, SEEK_SET);
	_fseeki64(RData, iSeekOffset, SEEK_SET);
#else
	INT64 lsize;
	INT64 rsize;
	lsize = _filelengthi64(_fileno(LData));
	rsize = _filelengthi64(_fileno(RData));

	_fseeki64(LData, lsize - DSD_SampleSize, SEEK_SET);
	_fseeki64(RData, rsize - DSD_SampleSize, SEEK_SET);
#endif
	//*** DSD chunk ***
	fwrite("DSD ", 4, 1, WriteData);	// header
	binary = CHUNK_SIZE_DSD;
	fwrite(&binary, 8, 1, WriteData);	// Size of this chunk
	binary = CHUNK_SIZE_DSD + CHUNK_SIZE_FMT + CHUNK_DATA_HEADER_SIZE + DSD_DataSize + GetID3V2Length();
	binary = CHUNK_SIZE_DSD + CHUNK_SIZE_FMT + CHUNK_DATA_HEADER_SIZE + DSD_DataSize;
	fwrite(&binary, 8, 1, WriteData);	// Total file size
	binary = 0;
	fwrite(&binary, 8, 1, WriteData);	// Pointer to Metadata chunk ※ID3V2 tag is None

	//*** FMT chunk ***
	fwrite("fmt ", 4, 1, WriteData);	// header
	binary = CHUNK_SIZE_FMT;
	fwrite(&binary, 8, 1, WriteData);	// Size of this chunk
	binary = 1;
	fwrite(&binary, 4, 1, WriteData);	// Format version 1
	binary = 0;
	fwrite(&binary, 4, 1, WriteData);	// Format ID  0:DSD raw
	binary = 2;
	fwrite(&binary, 4, 1, WriteData);	// Channel Type  2:stereo
	binary = 2;
	fwrite(&binary, 4, 1, WriteData);	// Channel num  2:stereo
	binary = DSD_SamplingRate;
	fwrite(&binary, 4, 1, WriteData);	// Sampling frequency(Hz) 2822400, 5644800,･･･
	binary = 1;
	fwrite(&binary, 4, 1, WriteData);	// Bits per sample  1
	binary = DSD_SampleSize;
	fwrite(&binary, 8, 1, WriteData);	// Sample count ※1chのサンプリング周波数 × n秒
	binary = block_size_per_ch;
	fwrite(&binary, 4, 1, WriteData);	// Block size per channel  4096
	binary = 0;
	fwrite(&binary, 4, 1, WriteData);	// Reserved
	//*** DATA chunk ***
	fwrite("data", 4, 1, WriteData);	// header
	binary = size_of_data_chunk;
	fwrite(&binary, 8, 1, WriteData);	// Size of this chunk

	unsigned __int64 i = 0;
//	int buffersize = 16384 * 2 * 8;
	int buffersize = (BLOCK_SIZE_PER_CHANNEL * 8) * 8;
//	int buffersize = BLOCK_SIZE_PER_CHANNEL * 8;
	int readsize = 0;
	unsigned char *onebit = new unsigned char[buffersize];
	unsigned char *tmpdataL = new unsigned char[buffersize];
	unsigned char *tmpdataR = new unsigned char[buffersize];
	unsigned char tmpL = 0; unsigned char tmpR = 0;
	int n = 0;
	int p_l = 0;
	int p_r = 0;
	int t = 0;
	int j = 0;
	int k = 0;
	int loopMax;
	int blockMax;
	int p_lMax;	// もしかしたらマルチコア化出来そうなのでLとRで分けておく。あまり速度アップしそうにないけど・・・
	int p_rMax;
	int blockcnt = 0;		// DEBUG用
	INT64 blocksize = 0;	// DEBUG用
	int DebugFileSize = 80+12;

	TRACE("FileSize:%10d\n", DebugFileSize);
	//WAV_FilterはLRごとUnsignedCharで書き出しているので、それを8サンプル1バイトにまとめてから
	//データ領域として書き出す
	loopMax = (int)((DSD_SampleSize / buffersize) + (DSD_SampleSize % buffersize != 0));
	for (i = 0; i < loopMax; i++){
		p_l = 0;
		p_r = 0;
		if(i < loopMax -1){
			readsize = buffersize;
		} else {
			readsize = (DSD_SampleSize % buffersize);
			if (readsize == 0) {
				readsize = buffersize;
			}
		}

		fread(tmpdataL, 1, readsize, LData);
		fread(tmpdataR, 1, readsize, RData);
		blockMax = (int)(readsize / (block_size_per_ch * 8) + (readsize % (block_size_per_ch * 8) != 0));
		for (j = 0; j < blockMax; j++){
			blockcnt ++;
			if (i < loopMax - 1) {
				p_lMax = (int)BLOCK_SIZE_PER_CHANNEL;
				p_rMax = (int)BLOCK_SIZE_PER_CHANNEL;
			} else {
				if (j < blockMax - 1) {
					p_lMax = (int)BLOCK_SIZE_PER_CHANNEL;
					p_rMax = (int)BLOCK_SIZE_PER_CHANNEL;
				} else {
					p_lMax = (readsize / 8) % BLOCK_SIZE_PER_CHANNEL;
					p_rMax = (readsize / 8) % BLOCK_SIZE_PER_CHANNEL;
					if (p_lMax == 0) {
						p_lMax = (int)BLOCK_SIZE_PER_CHANNEL;
					}
					if (p_rMax == 0) {
						p_rMax = (int)BLOCK_SIZE_PER_CHANNEL;
					}
				}
			}

			memset(onebit, 0, buffersize);
//			memset(onebit, 0x96, buffersize);
			for (k = 0; k < p_lMax; k++){
				onebit[k] = tmpdataL[p_l] << 0;
				onebit[k] += tmpdataL[p_l + 1] << 1;
				onebit[k] += tmpdataL[p_l + 2] << 2;
				onebit[k] += tmpdataL[p_l + 3] << 3;
				onebit[k] += tmpdataL[p_l + 4] << 4;
				onebit[k] += tmpdataL[p_l + 5] << 5;
				onebit[k] += tmpdataL[p_l + 6] << 6;
				onebit[k] += tmpdataL[p_l + 7] << 7;
				p_l += 8;
			}
			ullFwRet = fwrite(onebit, 1, block_size_per_ch, WriteData);//DSD_Data
			if(ullFwRet != block_size_per_ch){
				bRet = false;
			}
			blocksize += (int)block_size_per_ch;
			memset(onebit, 0, buffersize);
//			memset(onebit, 0x96, buffersize);
			for (k = 0; k < p_rMax; k++){
				onebit[k] = tmpdataR[p_r] << 0;
				onebit[k] += tmpdataR[p_r + 1] << 1;
				onebit[k] += tmpdataR[p_r + 2] << 2;
				onebit[k] += tmpdataR[p_r + 3] << 3;
				onebit[k] += tmpdataR[p_r + 4] << 4;
				onebit[k] += tmpdataR[p_r + 5] << 5;
				onebit[k] += tmpdataR[p_r + 6] << 6;
				onebit[k] += tmpdataR[p_r + 7] << 7;
				p_r += 8;
			}
			ullFwRet = fwrite(onebit, 1, block_size_per_ch, WriteData);//DSD_Data
			if(ullFwRet != block_size_per_ch){
				bRet = false;
			}
			blocksize += (int)block_size_per_ch;
			DebugFileSize += (4096 * 2);
			TRACE("FileSize:%10d\n", DebugFileSize);

			if(bRet == false){
				break;
			}
		}
		if(bRet == false){
			break;
		}
	}

	delete[] onebit;
	delete[] tmpdataL;
	delete[] tmpdataR;
#if 1	// DEBUGの為無効にする
	// Tagあり?
	if(m_nTagMode == 1 && bRet == true){
		bRet = DSFID3V2Write(WriteData);
	}
#endif
	return bRet;
}

// ID3v2タグサイズ
DWORD CPCMDSD_ConverterDlg::GetID3V2Length()
{
	DWORD dwLength;
	DWORD dwFlameSize;
	int i;

	// Tagなし？
	if(m_nTagMode == 0){
		return 0;
	}

	//*** ID3V2タグサイズ計算 ***
	// ID3v2ヘッダ(10Byte)
	dwLength = 10;
	TRACE("ID3v2 Head:%d\n", 10);

	//*** エンコーディング:UTF-16 / BOMあり ***
	// TALB
	dwFlameSize = m_stID3tag.strTALB.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TALB:%d\n", dwFlameSize + 15);
	}
	// TPE1
	dwFlameSize = m_stID3tag.strTPE1.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TPE1:%d\n", dwFlameSize + 15);
	}
	// TPE2
	dwFlameSize = m_stID3tag.strTPE2.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TPE2:%d\n", dwFlameSize + 15);
	}
	// TIT2
	dwFlameSize = m_stID3tag.strTIT2.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TIT2:%d\n", dwFlameSize + 15);
	}
	// TPOS
	dwFlameSize = m_stID3tag.strTPOS.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TPOS:%d\n", dwFlameSize + 15);
	}
	// TCON
	dwFlameSize = m_stID3tag.strTCON.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TCON:%d\n", dwFlameSize + 15);
	}
	// COMM
	dwFlameSize = m_stID3tag.strCOMM.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 22;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + LANG(7Byte) + フィールド + NULL(2Byte)
		TRACE("COMM:%d\n", dwFlameSize + 22);
	}
	// TCOM
	dwFlameSize = m_stID3tag.strTCOM.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TCOM:%d\n", dwFlameSize + 15);
	}
	// TRCK
	dwFlameSize = m_stID3tag.strTRCK.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TRCK:%d\n", dwFlameSize + 15);
	}
	// TYER
	dwFlameSize = m_stID3tag.strTYER.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TYER:%d\n", dwFlameSize + 15);
	}
	// TSSE
	dwFlameSize = m_stID3tag.strTSSE.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TSSE:%d\n", dwFlameSize + 15);
	}
	// TCOP
	dwFlameSize = m_stID3tag.strTCOP.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TCOP:%d\n", dwFlameSize + 15);
	}
	// TENC
	dwFlameSize = m_stID3tag.strTENC.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TENC:%d\n", dwFlameSize + 15);
	}
	// TEXT
	dwFlameSize = m_stID3tag.strTEXT.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TEXT:%d\n", dwFlameSize + 15);
	}
	// TXXX
	dwFlameSize = m_stID3tag.strTXXX.GetLength() * sizeof(TCHAR);
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 15;		// フレームヘッダ(10Byte) + エンコーディング(1Byte) + BOM(2Byte) + フィールド + NULL(2Byte)
		TRACE("TXXX:%d\n", dwFlameSize + 15);
	}

	//*** エンコーディング:ISO-8859-1 ***
#if 0
	// TPOS
	dwFlameSize = m_stID3tag.strTPOS.GetLength();
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 12;			// フレームヘッダ(10Byte) + エンコーディング(1Byte) + フィールド + NULL(1Byte)
		TRACE("TPOS:%d\n", dwFlameSize + 12);
	}
	// TRCK
	dwFlameSize = m_stID3tag.strTRCK.GetLength();
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 12;			// フレームヘッダ(10Byte) + エンコーディング(1Byte) + フィールド + NULL(1Byte)
		TRACE("TRCK:%d\n", dwFlameSize + 12);
	}
	// TYER
	dwFlameSize = m_stID3tag.strTYER.GetLength();
	if (dwFlameSize > 0) {
		dwLength += dwFlameSize + 12;			// フレームヘッダ(10Byte) + エンコーディング(1Byte) + フィールド + NULL(1Byte)
		TRACE("TYER:%d\n", dwFlameSize + 12);
	}
#endif
	// APIC
	for (i = 0;i < FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED; i++) {
		if (dwFlameSize = m_stID3tag.stPicture[i].nLength > 0) {
			dwFlameSize = m_stID3tag.stPicture[i].nLength;
			dwLength += dwFlameSize + 10;				// フレームヘッダ(10Byte) + フィールド
			TRACE("APIC[%2d]:%d\n", dwFlameSize + 10);
		}
	}

	return dwLength;
}

// DSFファイルID3v2タグ書き込み
bool CPCMDSD_ConverterDlg::DSFID3V2Write(FILE *WriteData)
{
	bool bRet = false;
	BYTE *pBuff;			// ID3V2編集バッファ
	DWORD idx;				// ID3V2編集バッファインデックス
	DWORD dwLength;			// ID3V2バッファサイズ
	UINT64 ullTotalFileSize;
	UINT64 ullMetaPos;
	size_t ullFwRet;
	// ID3v2サイズ算出
	dwLength = GetID3V2Length();
	if(dwLength == 0){
		return true;
	}

	fflush(WriteData);
	ullMetaPos = _filelengthi64(_fileno(WriteData));
#if 0
	// ヘッダー情報更新
	_fseeki64(WriteData, 12, SEEK_SET);
	ullTotalFileSize = ullMetaPos + GetID3V2Length();
	fwrite(&ullTotalFileSize, 8, 1, WriteData);	// Total file size
	fwrite(&ullMetaPos, 8, 1, WriteData);	// Pointer to Metadata chunk ※ID3V2 tag is None
#endif
	// 最後尾にID3V2を書き込む
	_fseeki64(WriteData, 0, SEEK_END);

	// バッファ取得
	pBuff = new BYTE[dwLength];
	memset(pBuff, 0xff, dwLength);	// DEBUG用に初期化

	//*** ID3V2編集 ***
	idx = 0;
	//*** Head ***
	pBuff[idx++] = 0x49;		// 'I'
	pBuff[idx++] = 0x44;		// 'D'
	pBuff[idx++] = 0x33;		// '3'

	pBuff[idx++] = 0x03;		// ID3v2.3.0
	pBuff[idx++] = 0x00;

	pBuff[idx++] = 0x00;		// フラグ

	MakeSynchsafeIntegerSize(dwLength - 10, &pBuff[6]);	// サイズ(Synchsafe Integer)
	idx += 4;

	//*** Frame ***
	// TALB
	idx += MakeTtag("TALB", 0x01, m_stID3tag.strTALB, &pBuff[idx]);
	// TPE1
	idx += MakeTtag("TPE1", 0x01, m_stID3tag.strTPE1, &pBuff[idx]);
	// TPE2
	idx += MakeTtag("TPE2", 0x01, m_stID3tag.strTPE2, &pBuff[idx]);
	// COMM
	idx += MakeCtag("COMM", m_stID3tag.strCOMM, &pBuff[idx]);
	// TCOM
	idx += MakeTtag("TCOM", 0x01, m_stID3tag.strTCOM, &pBuff[idx]);
	// TPOS
//	idx += MakeTtag("TPOS", 0x00, m_stID3tag.strTPOS, &pBuff[idx]);
	idx += MakeTtag("TPOS", 0x01, m_stID3tag.strTPOS, &pBuff[idx]);
	// TCON
	idx += MakeTtag("TCON", 0x01, m_stID3tag.strTCON, &pBuff[idx]);
	// TIT2
	idx += MakeTtag("TIT2", 0x01, m_stID3tag.strTIT2, &pBuff[idx]);
	// TRCK
//	idx += MakeTtag("TRCK", 0x00, m_stID3tag.strTRCK, &pBuff[idx]);
	idx += MakeTtag("TRCK", 0x01, m_stID3tag.strTRCK, &pBuff[idx]);
	// TYER
//	idx += MakeTtag("TYER", 0x00, m_stID3tag.strTYER, &pBuff[idx]);
	idx += MakeTtag("TYER", 0x01, m_stID3tag.strTYER, &pBuff[idx]);
	// TSSE
	idx += MakeTtag("TSSE", 0x01, m_stID3tag.strTSSE, &pBuff[idx]);
	// TCOP
	idx += MakeTtag("TCOP", 0x01, m_stID3tag.strTCOP, &pBuff[idx]);
	// TENC
	idx += MakeTtag("TENC", 0x01, m_stID3tag.strTENC, &pBuff[idx]);
	// TEXT
	idx += MakeTtag("TEXT", 0x01, m_stID3tag.strTEXT, &pBuff[idx]);
	// TXXX
	idx += MakeTtag("TXXX", 0x01, m_stID3tag.strTXXX, &pBuff[idx]);
	// APIC
	idx += MakeAPICtag(&pBuff[idx]);

	// ファイル書き込み
	ullFwRet = fwrite(pBuff, 1, idx, WriteData);

	delete[] pBuff;

	if(ullFwRet == idx){
		// ヘッダー情報更新
		_fseeki64(WriteData, 12, SEEK_SET);
		ullTotalFileSize = ullMetaPos + GetID3V2Length();
		fwrite(&ullTotalFileSize, 8, 1, WriteData);	// Total file size
		fwrite(&ullMetaPos, 8, 1, WriteData);	// Pointer to Metadata chunk ※ID3V2 tag is None
		bRet = true;
	}

	return bRet;
}

// TTAG作成
DWORD CPCMDSD_ConverterDlg::MakeTtag(char *strID, int nStrCode, CString strTtag, BYTE *pBuff)
{
	DWORD dwFrameLength;
	DWORD dwFlameTLength;
	DWORD dwFlameSize;
	TCHAR *pField;			// フィールドバッファ
	DWORD idx;

	idx = 0;
	dwFrameLength = strTtag.GetLength();
	if (dwFrameLength > 0) {
		// 00h: ISO-8859-1
		if (nStrCode == 0x00) {
			dwFlameTLength = dwFrameLength + 1;	// NULL(0x00,0x00)文字含めてTCHAR確保
			pField = new TCHAR[dwFlameTLength];
			_tcscpy_s(pField, dwFlameTLength, strTtag);
			// ID
			pBuff[idx++] = strID[0];
			pBuff[idx++] = strID[1];
			pBuff[idx++] = strID[2];
			pBuff[idx++] = strID[3];
			// Frame Size(Synchsafe Integer)  ※Frame Size = Head(3Byte) + Body
			dwFlameSize = dwFlameTLength;		// ISO-8859-1は1文字1BYTE
			GetFrameSize(1 + dwFlameSize, FALSE, &pBuff[idx]);
			idx += 4;
			// Flag
			pBuff[idx++] = 0x00;
			pBuff[idx++] = 0x00;
			// Frame
			pBuff[idx++] = 0x00;	// 00h: ISO-8859-1
			for (DWORD i = 0;i < dwFlameTLength;i ++) {	// 漢字コードが来るならちゃんとやらなくちゃ駄目ｗ
				pBuff[idx++] = (BYTE)pField[i];
			}

			delete[] pField;
		}
		// 01h: UTF-16 / BOMあり
		if (nStrCode == 0x01) {
			dwFlameTLength = dwFrameLength + 1;	// NULL(0x00,0x00)文字含めてTCHAR確保
			pField = new TCHAR[dwFlameTLength];
			_tcscpy_s(pField, dwFlameTLength, strTtag);
			// ID
			pBuff[idx++] = strID[0];
			pBuff[idx++] = strID[1];
			pBuff[idx++] = strID[2];
			pBuff[idx++] = strID[3];
			// Frame Size(Syncsafe Integer)  ※Frame Size = Head(3Byte) + Body
			dwFlameSize = dwFlameTLength * 2;	// UTF-16は1文字2BYTE
			GetFrameSize(3 + dwFlameSize, FALSE, &pBuff[idx]);
			idx += 4;
			// Flag
			pBuff[idx++] = 0x00;
			pBuff[idx++] = 0x00;
			// Frame
			pBuff[idx++] = 0x01;	// 01h: UTF-16 / BOMあり
			pBuff[idx++] = 0xFF;	// BOM
			pBuff[idx++] = 0xFE;
			memcpy(&pBuff[idx], pField, dwFlameSize);
			idx += dwFlameSize;

			delete[] pField;
		}
	}

	return idx;
}

// CTAG作成 ※COMM専用
DWORD CPCMDSD_ConverterDlg::MakeCtag(char *strID, CString strCtag, BYTE *pBuff)
{
	DWORD dwFrameLength;
	DWORD dwFrameTLength;
	DWORD dwFrameSize;
	TCHAR *pField;			// フィールドバッファ
	DWORD idx;

	idx = 0;
	dwFrameLength = strCtag.GetLength();
	if (dwFrameLength > 0) {
		dwFrameTLength = dwFrameLength + 1;	// NULL(0x00,0x00)文字含めてTCHAR確保
		pField = new TCHAR[dwFrameTLength];
		_tcscpy_s(pField, dwFrameTLength, strCtag);
		// ID
		pBuff[idx++] = strID[0];
		pBuff[idx++] = strID[1];
		pBuff[idx++] = strID[2];
		pBuff[idx++] = strID[3];
		// Frame Size(Syncsafe Integer)  ※Frame Size = Head(3Byte) + Body
		dwFrameSize = dwFrameTLength * 2;	// UTF-16は1文字2BYTE
		GetFrameSize(10 + dwFrameSize, FALSE, &pBuff[idx]);
		idx += 4;
		// Flag
		pBuff[idx++] = 0x00;
		pBuff[idx++] = 0x00;
		// Frame
		pBuff[idx++] = 0x01;	// 01h: UTF-16 / BOMあり
		pBuff[idx++] = 'e';		// Language
		pBuff[idx++] = 'n';
		pBuff[idx++] = 'g';
		pBuff[idx++] = 0xFF;	// BOM
		pBuff[idx++] = 0xFE;
		pBuff[idx++] = 0x00;	// 説明文(省略)
		pBuff[idx++] = 0x00;
		pBuff[idx++] = 0xFF;	// BOM
		pBuff[idx++] = 0xFE;
		memcpy(&pBuff[idx], pField, dwFrameSize);
		idx += dwFrameSize;

		delete[] pField;
	}

	return idx;
}

// APIC TAG作成
DWORD CPCMDSD_ConverterDlg::MakeAPICtag(BYTE *pBuff)
{
	DWORD dwFrameLength;
	DWORD idx;
	DWORD i;

	idx = 0;
	for (i = 0;i < FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED; i++) {
		dwFrameLength = m_stID3tag.stPicture[i].nLength;
		if (dwFrameLength > 0) {
			// ID
			pBuff[idx++] = 'A';
			pBuff[idx++] = 'P';
			pBuff[idx++] = 'I';
			pBuff[idx++] = 'C';
			// Frame Size
			GetFrameSize(dwFrameLength, FALSE, &pBuff[idx]);
			idx += 4;
			// Flag
			pBuff[idx++] = 0x00;
			pBuff[idx++] = 0x00;
			// Frame
			memcpy(&pBuff[idx],  m_stID3tag.stPicture[i].pData, dwFrameLength);
			idx += dwFrameLength;
		}
	}

	return idx;
}

// Syncsafe Integerサイズ作成 ※各BYTE 下位7BIT有効の28BIT整数で表す
void CPCMDSD_ConverterDlg::MakeSynchsafeIntegerSize(DWORD dwSize, unsigned char size[4])
{
	size[3] = dwSize & 0x7f;
	size[2] = (dwSize >> 7) & 0x7f;
	size[1] = (dwSize >> 14) & 0x7f;
	size[0] = (dwSize >> 21) & 0x7f;
}

// フレームサイズ取得
void CPCMDSD_ConverterDlg::GetFrameSize(DWORD dwSize, BOOL bSynchsafeIntegerSize, unsigned char size[4])
{
	DWORD idx;

	if(bSynchsafeIntegerSize == TRUE){
		MakeSynchsafeIntegerSize(dwSize, size);
	} else {
		idx = 0;
		size[idx++] = dwSize >> 24 & 0xff;
		size[idx++] = dwSize >> 16 & 0xff;
		size[idx++] = dwSize >> 8 & 0xff;
		size[idx++] = dwSize & 0xff;
	}
}

//一時ファイル削除
bool CPCMDSD_ConverterDlg::TrushFile(TCHAR *filepath, CString flag){
	TCHAR FileName[_MAX_FNAME];
	TCHAR FileExt[_MAX_EXT];
	TCHAR Drive[_MAX_DRIVE];
	TCHAR Directory[_MAX_DIR];
	TCHAR DirectoryName[_MAX_FNAME];

	_tsplitpath_s(filepath, Drive, Directory, FileName, FileExt);

	_tcscpy_s(DirectoryName, sizeof(DirectoryName)/sizeof(DirectoryName[0]), ::PathFindFileName(Directory));

	CString DeletePath;
	if (m_evPath == L""){
		DeletePath = Drive;
		DeletePath += Directory;
	}
	else{
//		DeletePath = m_evPath;
		DeletePath = m_evPath;
		DeletePath += _T("\\");
//		DeletePath += DirectoryName;
		if (m_strAlbumArtistOutDir.GetLength() > 0 && m_strAlbumOutDir.GetLength() > 0) {
			DeletePath += m_strAlbumArtistOutDir;
			DeletePath += _T("\\");
			DeletePath += m_strAlbumOutDir;
			DeletePath += _T("\\");
			// 出力ディレクトリ用Disc NoがあるならDisc Noディレクトリも作成
			if(m_strDiscNoOutDir.GetLength() > 0){
				DeletePath += m_strDiscNoOutDir;
				DeletePath += _T("\\");
			}
		} else {
			DeletePath += DirectoryName;
		}
	}
	DeletePath += _T("\\"); DeletePath += FileName + flag;
	if (!DeleteFile(DeletePath)){
		return false;
	}
	return true;
}

//読み書きデータ準備
bool CPCMDSD_ConverterDlg::RequireWriteData(TCHAR *filepath, CString flag, TCHAR *WriteFilepath)
{
	return RequireWriteData(filepath, flag, NULL, NULL, WriteFilepath, FALSE);
}

bool CPCMDSD_ConverterDlg::RequireWriteData(TCHAR *filepath, CString flag, wchar_t *FileMode, FILE **WriteDatadsd, TCHAR *WriteFilepath, BOOL bOverWrite)
{
	TCHAR FileName[_MAX_FNAME];
	TCHAR FileExt[_MAX_EXT];
	TCHAR Drive[_MAX_DRIVE];
	TCHAR Directory[_MAX_DIR];
	TCHAR DirectoryName[_MAX_FNAME];

	_tsplitpath_s(filepath, Drive, Directory, FileName, FileExt);

	_tcscpy_s(DirectoryName, sizeof(DirectoryName) / sizeof(DirectoryName[0]), ::PathFindFileName(Directory));

	CString WritePath;
	if (m_evPath == L""){
		WritePath = Drive;
		WritePath += Directory;
	}
	else{
//		WritePath = m_evPath;
		WritePath = m_evPath;
		WritePath += _T("\\");
//		WritePath += DirectoryName;
		// 出力ディレクトリ用アルバムアーティスト/出力ディレクトリ用アルバム＋サフィックスのディレクトリ
		if (m_strAlbumArtistOutDir.GetLength() > 0 && m_strAlbumOutDir.GetLength() > 0){
			WritePath += m_strAlbumArtistOutDir;
			WritePath += _T("\\");
			WritePath += m_strAlbumOutDir;
			WritePath += _T("\\");
			// 出力ディレクトリ用Disc NoがあるならDisc Noディレクトリも作成
			if(m_strDiscNoOutDir.GetLength() > 0){
				WritePath += m_strDiscNoOutDir;
				WritePath += _T("\\");
			}

		} else {
			WritePath += DirectoryName;
		}
	}

	CFileFind find;
	if (!PathFileExists(WritePath)){
//		CreateDirectory(WritePath, NULL);
		SHCreateDirectoryEx(NULL, WritePath, NULL);
	}
//	WritePath += _T("\\"); WritePath += FileName; WritePath += flag;
	WritePath += FileName;
	WritePath += flag;

	if(WriteFilepath != NULL){
		_tcscpy_s(WriteFilepath, ABSOLUTE_PATH_MAX, WritePath);
	}

	if(WriteDatadsd == NULL){
		return true;
	}

	errno_t error_w;
	CString errorMessage;

	errorMessage = WritePath;
	errorMessage += _T("に書き込めませんでした");

	if (!wcscmp(FileMode, L"wb") && find.FindFile(WritePath) && bOverWrite == FALSE){
		if((dwCompletionNoticeFlag & 0x00000002) != 0){
			WakeupDisplay();	// モニタON
		}
		errorMessage = WritePath;
		errorMessage += _T("は既に存在します。上書きしますか？");
		int i = MessageBox(errorMessage, L"上書き確認", MB_YESNO);
		if (i == IDNO){
			return false;
		}
		else{
			if ((error_w = _tfopen_s(WriteDatadsd, WritePath, FileMode)) != 0) {
				MessageBox(errorMessage, L"DSFファイル書き込み失敗", MB_OK);
				return false;
			}
		}
	} else {
		if ((error_w = _tfopen_s(WriteDatadsd, WritePath, FileMode)) != 0) {
			MessageBox(errorMessage, L"DSFファイル書き込み失敗", MB_OK);
			return false;
		}
	}
	setvbuf(*WriteDatadsd, NULL, _IOFBF, 512 * 1024);
	return true;
}

////Wavファイルを64bit Float(double)化し、LR分離して一時ファイルとして書き出し
bool CPCMDSD_ConverterDlg::TmpWriteData(EXT_TYPE etExtType, TCHAR *filepath, FILE *tmpl, FILE *tmpr, int DSD_Times, unsigned int *pTimes, int number)
{
	FILE *wavread;
	size_t ullFwRet;

	errno_t error;
	bool flag = true;
	bool floatint = true;
	double dbL = 0;
	double dbR = 0;
	double dbLR = 0;
	double ddbPeak = 0;
	double dDiffdB = 0;
	CString wav;

	unsigned short chnum;
	unsigned __int32 samplingrate;
	unsigned __int32 nDstSamplePerSec;
	unsigned __int32 nDstBaseSamplePerSec;
	unsigned short bitdepth;
	WORD nBlockAlign;
	double dIdxRatio;
	int Times;
	UINT64 samplesize = 0;

//	stAmp.dAmpPeak[0] = 0;
//	stAmp.dAmpPeak[1] = 0;
	// 1トラック目？ ※[全て実行][実行]は必ずnumber=0で来る
	if (number == 0){
		// 振幅データ 初期化
		stAmp.dAmpPeak[0] = 0;
		stAmp.dAmpPeak[1] = 0;
		stAmp.dAmpSum[0] = 0;
		stAmp.dAmpSum[1] = 0;
		stAmp.ullAmpSumCnt[0] = 0;
		stAmp.ullAmpSumCnt[1] = 0;
		m_dAvgPeakdB[0] = 0;
		m_dAvgPeakdB[1] = 0;

		// クリップエラーノーマライズ無効
		m_ClipErrNormalizeEnable = 0;
	}

	// 振幅データピークデシベル初期化
	stAmpPeakdB.dAmpPeak[0] = 0;
	stAmpPeakdB.dAmpPeak[1] = 0;
	stAmpPeakdB.dAmpSum[0] = 0;
	stAmpPeakdB.dAmpSum[1] = 0;
	stAmpPeakdB.ullAmpSumCnt[0] = 0;
	stAmpPeakdB.ullAmpSumCnt[1] = 0;

	m_AveragedB[0] = 0;
	m_AveragedB[1] = 0;
	m_AveragedB[2] = 0;

	error = _tfopen_s(&wavread, filepath, _T("rb"));
	if (error != 0) {
		return false;
	}
	setvbuf(wavread, NULL, _IOFBF, 512 * 1024);

	char tmp4[6];
	short fmtID;

	ST_W64_CHUNK stW64ChunkRiff;
	ST_W64_CHUNK stW64Chunkfmt;
	ST_W64_CHUNK stW64Chunk;
	ST_W64_DATA stData;
	GUID guidWave;
	PCMWAVEFORMAT w64fmt;
	WAVEFORMATEXTENSIBLE stWaveFormatExt;
	GUID SubFormatGUID;

	LONGLONG llOff;

	if (etExtType != EXT_TYPE_WAVE64){
		// WAV
		while (flag){
			if (feof(wavread)){
				fclose(wavread);
				return false;
			}
			fread(tmp4, 1, 1, wavread);
			tmp4[1] = '\0';
			wav = tmp4;
			if (wav == "f"){
				fread(tmp4, 1, 3, wavread);
				tmp4[3] = '\0';
				wav = tmp4;
				if (wav == "mt "){
					flag = false;
				}
			}
		}
		//fmtIDでFloatInt判別
		_fseeki64(wavread, 4, SEEK_CUR);
		fread(&fmtID, 2, 1, wavread);
		if (fmtID == 3){
			floatint = false;
		}
		else{
			floatint = true;
		}

		fread(&chnum, 2, 1, wavread);
		fread(&samplingrate, 4, 1, wavread);
		_fseeki64(wavread, 4, SEEK_CUR);
		fread(&nBlockAlign, 2, 1, wavread);
//		_fseeki64(wavread, 12, SEEK_CUR);
		fread(&bitdepth, 2, 1, wavread);

		flag = true;
		while (flag){
			if (feof(wavread)){
				fclose(wavread);
				return false;
			}
			fread(tmp4, 1, 1, wavread);
			tmp4[1] = '\0';
			wav = tmp4;
			if (wav == "d"){
				fread(tmp4, 1, 3, wavread);
				tmp4[3] = '\0';
				wav = tmp4;
				if (wav == "ata"){
					flag = false;
				}
			}
		}
		fread(&samplesize, 4, 1, wavread);
	} else {
		// SONY WAVE64
		// RIFFチャンク
		fread(&stW64ChunkRiff, sizeof(stW64ChunkRiff), 1, wavread);
//		fileObj.Read(&stW64ChunkRiff, sizeof(stW64ChunkRiff));
		if (stW64ChunkRiff.Guid != s_GUID[W64_RIFF]) {
			return false;
		}

		// WAVEチャンク
		fread(&guidWave, sizeof(guidWave), 1, wavread);
//		fileObj.Read(&guidWave, sizeof(guidWave));
		if (guidWave != s_GUID[W64_WAVE]) {
			return false;
		}

		// fmtチャンク
		fread(&stW64Chunkfmt, sizeof(stW64Chunkfmt), 1, wavread);
//		fileObj.Read(&stW64Chunkfmt, sizeof(stW64Chunkfmt));
		if (stW64Chunkfmt.Guid != s_GUID[W64_FMT]) {
			return false;
		}
		fread(&w64fmt, sizeof(w64fmt), 1, wavread);
//		fileObj.Read(&w64fmt, sizeof(w64fmt));

		switch(w64fmt.wf.wFormatTag){
			case WAVE_FORMAT_PCM:
				samplingrate = w64fmt.wf.nSamplesPerSec;
				bitdepth = w64fmt.wBitsPerSample;
				chnum = w64fmt.wf.nChannels;
				nBlockAlign = w64fmt.wf.nBlockAlign;
				floatint = true;
				break;
			case WAVE_FORMAT_IEEE_FLOAT:
				samplingrate = w64fmt.wf.nSamplesPerSec;
				bitdepth = w64fmt.wBitsPerSample;
				chnum = w64fmt.wf.nChannels;
				nBlockAlign = w64fmt.wf.nBlockAlign;
				floatint = false;
				break;
			case WAVE_FORMAT_EXTENSIBLE:
				llOff = sizeof(w64fmt) * -1;
//				fileObj.Seek(llOff, CFile::current);
				_fseeki64(wavread, llOff, SEEK_CUR);
//				fileObj.Read(&stWaveFormatExt, sizeof(stWaveFormatExt));
				fread(&stWaveFormatExt, sizeof(stWaveFormatExt), 1, wavread);
				SubFormatGUID = (GUID)stWaveFormatExt.SubFormat;
				if (SubFormatGUID == KSDATAFORMAT_SUBTYPE_PCM){
					floatint = true;
				}
				if (SubFormatGUID == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
				floatint = false;
				}
				samplingrate = stWaveFormatExt.Format.nSamplesPerSec;
				bitdepth = stWaveFormatExt.Samples.wValidBitsPerSample;
				chnum = stWaveFormatExt.Format.nChannels;
				nBlockAlign = stWaveFormatExt.Format.nBlockAlign;
				break;
			default:
				TRACE("fmt Chunk FormatTag is Unknown:0x%04X", w64fmt.wf.wFormatTag);
				break;
		}
		int i;

		memset(&stData, NULL, sizeof(stData));
		do{
//			fileObj.Read(&stW64Chunk, sizeof(stW64Chunk));
			fread(&stW64Chunk, sizeof(stW64Chunk), 1, wavread);
			for(i = 0;i < W64_GUID_MAX;i ++){
				if (stW64Chunk.Guid == s_GUID[i]) {
					switch(i){
						case W64_DATA:
							stData.ullSampleLength = stW64Chunk.ullSize;
							break;
						default:
							llOff = stW64Chunk.ullSize - sizeof(stW64Chunk);
//							fileObj.Seek(stW64Chunk.ullSize, CFile::current);
							_fseeki64(wavread, llOff, SEEK_CUR);
							break;
					}
					break;
				}
			}
		} while (stW64Chunk.Guid != s_GUID[W64_DATA]);
		samplesize = stData.ullSampleLength - sizeof(stW64Chunk);
	}
#if 1
	// ダウンサンプリングレート取得 ※1番近い周波数にアップサンプリングする周波数を検索
	GetTargetSample(&nDstSamplePerSec, &nDstBaseSamplePerSec, samplingrate, m_Pcm48KHzEnableDsd3MHzEnable);
	Times = DSD_Times / (nDstSamplePerSec / nDstBaseSamplePerSec);
	*pTimes = Times;
#else
	Times = DSD_Times / (samplingrate / 44100);
	*pTimes = Times;
	nDstSamplePerSec = samplingrate;
#endif

	m_SrcPcmSamplingrate = nDstSamplePerSec;

	dIdxRatio = (double)samplingrate / (double)nDstSamplePerSec;

//	m_SrcPcmBit = bitdepth;
	m_SrcPcmBit = bitdepth + (bitdepth % 8);
//	flag = true;

#if 1
	// ID3v2タグユーザー定義にオリジナルフォーマット設定
	SetID3v2UserTagOriginalFormat(0, m_SrcFileName, samplingrate, bitdepth, &m_stID3tag);
#endif

#if 0
	while (flag){
		if (feof(wavread)){
			fclose(wavread);
			return false;
		}
		fread(tmp4, 1, 1, wavread);
		tmp4[1] = '\0';
		wav = tmp4;
		if (wav == "d"){
			fread(tmp4, 1, 3, wavread);
			tmp4[3] = '\0';
			wav = tmp4;
			if (wav == "ata"){
				flag = false;
			}
		}
	}
	long samplesize;
	fread(&samplesize, 4, 1, wavread);
#endif

	//データがDSD変換時にきれいに割り切れるように、頭にゼロフィルする
	ifstream ifs(".\\FIRFilter.dat");
	int section_1 = 0;
	string str;
	if (ifs.fail())
	{
		return false;
	}
	getline(ifs, str);
	section_1 = atoi(str.c_str());
#if 0	// 未使用?メモリリークしている
	double *firfilter_table = new double[section_1];//
	__int64 i = 0;
	while (getline(ifs, str))
	{
		firfilter_table[i] = atof(str.c_str());
		i++;
	}
#endif
	ifs.close();

	__int64 buffer_int = 0;
	double buffer_double = 0;
	float buffer_float;
	double bit = pow(2, bitdepth - 1);
	UINT64 ullIdxSrc;
	UINT64 ullIdxDst;
	UINT64 ullSrcSheekPos;
	UINT64 ullSrcOffsetSheekPos;
	UINT64 ullDataSizeDst;

	UINT64 nBlock;
//	DWORD bitbyte = bitdepth / 8;
	UINT64 bitbyte = (bitdepth + (bitdepth % 8)) / 8;

	nBlock = nBlockAlign;

	// 変換後サンプルサイズ計算
	ullIdxDst = 0;
	ullSrcOffsetSheekPos = 0;
	ullDataSizeDst = 0;
	while (1) {
		ullIdxSrc = (UINT64)((double)ullIdxDst * dIdxRatio);
		ullSrcSheekPos = ullIdxSrc * nBlock;
		if (ullSrcSheekPos >= samplesize) {
			break;
		}
		ullSrcOffsetSheekPos = ullSrcSheekPos;

		ullIdxDst++;
		ullDataSizeDst += nBlock;
	}
	m_OrigDataSize = ullDataSizeDst;

//	unsigned __int64 writelength = samplesize / ((bitdepth / 8) * 2);
//	unsigned __int64 writelength = samplesize / (bitbyte * 2);
	UINT64 writelength = ullDataSizeDst / (bitbyte * 2);
	UINT64 fillsize = (section_1 + 1)*Times - (writelength % ((section_1 + 1)*Times));
#if 1	// FILLしない DEBUG
	if(m_fillsize == 0){
		for (UINT64 i = 0; i < fillsize; i++){
			ullFwRet = fwrite(&buffer_double, 8, 1, tmpl);
			if(ullFwRet != 1){
				fclose(wavread);
				return false;
			}
			ullFwRet = fwrite(&buffer_double, 8, 1, tmpr);
			if(ullFwRet != 1){
				fclose(wavread);
				return false;
			}
		}
		m_fillsize = ((fillsize * 8) * Times) / 8;
	}
#endif
	ullSrcSheekPos = 0;
	ullIdxDst = 0;
	ullSrcOffsetSheekPos = 0;

	//各種フォーマットの値をdouble型変数に-1,1に正規化して入れる
	if (floatint){
#if 0
		for (int i = 0; i < writelength; i++){
			if (!m_dProgress.Cancelbottun){
				fclose(wavread);
				return false;
			}

			fread(&buffer_int, bitdepth / 8, 1, wavread);
			buffer_int = buffer_int << (64 - bitdepth);
			buffer_int = buffer_int >> (64 - bitdepth);
			buffer_double = buffer_int / bit;
			PcmAnalysis(buffer_double, 0, nDstSamplePerSec);
			fwrite(&buffer_double, 8, 1, tmpl);

			fread(&buffer_int, bitdepth / 8, 1, wavread);
			buffer_int = buffer_int << (64 - bitdepth);
			buffer_int = buffer_int >> (64 - bitdepth);
			buffer_double = buffer_int / bit;
			PcmAnalysis(buffer_double, 1, nDstSamplePerSec);
			fwrite(&buffer_double, 8, 1, tmpr);
		}
		dbLR = GetAveragedB(&dbL, &dbR);
//		TRACE("%3.2fdb(L) %3.2fdb(R)\n", dbL, dbR);
#else
		while(1){
			if (!m_dProgress.Cancelbottun){
				fclose(wavread);
				return false;
			}

			ullIdxSrc = (UINT64)((double)ullIdxDst * dIdxRatio);
			ullSrcSheekPos = ullIdxSrc * nBlock;
			if (ullSrcSheekPos >= samplesize) {
				// 最後の1秒に満たなかった平均ピークデシベル算出
				CalcPeakdB();
				break;
			}
			_fseeki64(wavread, ullSrcSheekPos - ullSrcOffsetSheekPos, SEEK_CUR);
			ullSrcOffsetSheekPos = ullSrcSheekPos;

			fread(&buffer_int, bitbyte, 1, wavread);
			if (bitdepth == 20) {
				buffer_int = buffer_int >> 4;
			}
			ullSrcOffsetSheekPos += (bitbyte);
			buffer_int = buffer_int << (64 - bitdepth);
			buffer_int = buffer_int >> (64 - bitdepth);
			buffer_double = buffer_int / bit;
			buffer_double *= AMP_LIMIT_MAX;
			PcmAnalysis(buffer_double, 0, nDstSamplePerSec);
			ullFwRet = fwrite(&buffer_double, 8, 1, tmpl);
			if(ullFwRet != 1){
				fclose(wavread);
				return false;
			}

			fread(&buffer_int, bitbyte, 1, wavread);
			if (bitdepth == 20) {
				buffer_int = buffer_int >> 4;
			}
			ullSrcOffsetSheekPos += (bitbyte);
			buffer_int = buffer_int << (64 - bitdepth);
			buffer_int = buffer_int >> (64 - bitdepth);
			buffer_double = buffer_int / bit;
			buffer_double *= AMP_LIMIT_MAX;
			PcmAnalysis(buffer_double, 1, nDstSamplePerSec);
			ullFwRet = fwrite(&buffer_double, 8, 1, tmpr);
			if(ullFwRet != 1){
				fclose(wavread);
				return false;
			}

			ullIdxDst++;
		}
		dbLR = GetAveragedB(&dbL, &dbR);
//		TRACE("%3.2fdb(L) %3.2fdb(R)\n", dbL, dbR);
#endif
	}
	else if (bitdepth == 32){
#if 0
		for (int i = 0; i < writelength; i++){
			if (!m_dProgress.Cancelbottun){
				fclose(wavread);
				return false;
			}
			fread(&buffer_float, bitdepth / 8, 1, wavread);
			buffer_double = buffer_float;
			fwrite(&buffer_double, 8, 1, tmpl);
			fread(&buffer_float, bitdepth / 8, 1, wavread);
			buffer_double = buffer_float;
			fwrite(&buffer_double, 8, 1, tmpr);
		}
#else
		while(1){
			if (!m_dProgress.Cancelbottun){
				fclose(wavread);
				return false;
			}

			ullIdxSrc = (UINT64)((double)ullIdxDst * dIdxRatio);
			ullSrcSheekPos = ullIdxSrc * nBlock;
			if (ullSrcSheekPos >= samplesize) {
				// 最後の1秒に満たなかった平均ピークデシベル算出
				CalcPeakdB();
				break;
			}
			_fseeki64(wavread, ullSrcSheekPos - ullSrcOffsetSheekPos, SEEK_CUR);
			ullSrcOffsetSheekPos = ullSrcSheekPos;

			fread(&buffer_float, bitbyte, 1, wavread);
			ullSrcOffsetSheekPos += (bitbyte);
			buffer_double = (double)buffer_float;
			buffer_double *= AMP_LIMIT_MAX;
			PcmAnalysis(buffer_double, 0, nDstSamplePerSec);
			ullFwRet = fwrite(&buffer_double, 8, 1, tmpl);
			if(ullFwRet != 1){
				fclose(wavread);
				return false;
			}

			fread(&buffer_float, bitbyte, 1, wavread);
			ullSrcOffsetSheekPos += (bitbyte);
			buffer_double = (double)buffer_float;
			buffer_double *= AMP_LIMIT_MAX;
			PcmAnalysis(buffer_double, 1, nDstSamplePerSec);
			ullFwRet = fwrite(&buffer_double, 8, 1, tmpr);
			if(ullFwRet != 1){
				fclose(wavread);
				return false;
			}

			ullIdxDst++;
		}
		dbLR = GetAveragedB(&dbL, &dbR);
//		TRACE("%3.2fdb(L) %3.2fdb(R)\n", dbL, dbR);
#endif
	} else{
#if 0
		for (int i = 0; i < writelength; i++){
			if (!m_dProgress.Cancelbottun){
				fclose(wavread);
				return false;
			}
			fread(&buffer_double, bitdepth / 8, 1, wavread);
			fwrite(&buffer_double, 8, 1, tmpl);
			fread(&buffer_double, bitdepth / 8, 1, wavread);
			fwrite(&buffer_double, 8, 1, tmpr);
		}
#else
		while (1) {
			if (!m_dProgress.Cancelbottun){
				fclose(wavread);
				return false;
			}

			ullIdxSrc = (UINT64)((double)ullIdxDst * dIdxRatio);
			ullSrcSheekPos = ullIdxSrc * nBlock;
			if (ullSrcSheekPos >= samplesize) {
				// 最後の1秒に満たなかった平均ピークデシベル算出
				CalcPeakdB();
				break;
			}
			_fseeki64(wavread, ullSrcSheekPos - ullSrcOffsetSheekPos, SEEK_CUR);
			ullSrcOffsetSheekPos = ullSrcSheekPos;

			fread(&buffer_double, bitbyte, 1, wavread);
			ullSrcOffsetSheekPos += (bitbyte);
			buffer_double *= AMP_LIMIT_MAX;
			PcmAnalysis(buffer_double, 0, nDstSamplePerSec);
			ullFwRet = fwrite(&buffer_double, 8, 1, tmpl);
			if(ullFwRet != 1){
				fclose(wavread);
				return false;
			}

			fread(&buffer_double, bitbyte, 1, wavread);
			ullSrcOffsetSheekPos += (bitbyte);
			buffer_double *= AMP_LIMIT_MAX;
			PcmAnalysis(buffer_double, 1, nDstSamplePerSec);
			ullFwRet = fwrite(&buffer_double, 8, 1, tmpr);
			if(ullFwRet != 1){
				fclose(wavread);
				return false;
			}

			ullIdxDst++;
		}
		dbLR = GetAveragedB(&dbL, &dbR);
//		TRACE("%3.2fdB(L) %3.2fdB(R)\n", dbL, dbR);
#endif
	}
//	dbL = dbL / writelength;
//	dbR = dbR / writelength;
	m_AveragedB[0] = dbL;
	m_AveragedB[1] = dbR;
	m_AveragedB[2] = dbLR;
	ddbPeak = GetAvgPeakdB();
	m_dProgress.SetAveragedB(ddbPeak);

	if(m_radioGainModeDdv == 0){
		m_dGain = m_dVolGain;
		TRACE("平均 %3.2fdB(L) %3.2fdB(R) %3.2fdB(L+R) ゲイン調整モード\n", dbL, dbR, dbLR);
	} else {
		// 制限dB差分 ※プラスの場合はゲイン調整は行わない
//		m_dGain = 1;
//		m_DiffdB = m_LimitdB - m_AveragedB[2];
//		dDiffdB = m_LimitdB - m_AveragedB[2];

		m_DiffdB = m_LimitdB - ddbPeak;
		if (m_DiffdB < 0) {
			m_dGain = GetdBtoPower(m_DiffdB);
		}
		// ゲイン調整の掛け合わせ
		if(m_CrossGainLevel == 1){
			m_dGain *= m_dVolGain;
		}
//		TRACE("平均 %3.2fdB(L) %3.2fdB(R) %3.2fdB(L+R) ゲイン制限モード:(%3.2fdB) - (%3.2fdB) = %3.2fdB差分\n", dbL, dbR, dbLR, m_LimitdB, dbLR, m_DiffdB);
		TRACE("最大 %3.2fdB 平均 %3.2fdB(L) %3.2fdB(R) %3.2fdB(L+R) ゲイン制限モード:(%3.2fdB) - (%3.2fdB) = %3.2fdB差分\n", ddbPeak, dbL, dbR, dbLR, m_LimitdB, dbLR, m_DiffdB);
	}

	_fseeki64(tmpl, 0, SEEK_SET);
	_fseeki64(tmpr, 0, SEEK_SET);
	fclose(wavread);

	return true;
}

// 振幅を解析　戻り値：デシベル変換値 ※解析っていっても登録してるだけ
void CPCMDSD_ConverterDlg::PcmAnalysis(double amp_value, int ch, int nPcmSamplingRate)
{
	double amp_value_tmp;

	// 絶対値に変換
	amp_value_tmp = fabs(amp_value);
	if(amp_value_tmp > AMP_LIMIT_MAX){
		TRACE("Clip Over:%f\n", amp_value);
#if 0
		// 強制補正 ※ノーマライズで全体を補正した方が波形が崩れない
//		amp_value_tmp = AMP_LIMIT_MAX;
#else
		// クリップエラーノーマライズ有効 ※ノーマライズで全体を補正する
		m_ClipErrNormalizeEnable = 1;
#endif
	}

	//*** 振幅データ ***
	stAmp.dAmpSum[ch] += amp_value_tmp;
	stAmp.ullAmpSumCnt[ch] ++;

	// 振幅最大値
	if(amp_value_tmp > stAmp.dAmpPeak[ch]){
		stAmp.dAmpPeak[ch] = amp_value_tmp;
	}

	//*** 振幅データピークデシベル ***
	stAmpPeakdB.dAmpSum[ch] += amp_value_tmp;;
	stAmpPeakdB.ullAmpSumCnt[ch] ++;

	// 振幅最大値
	if(amp_value_tmp > stAmpPeakdB.dAmpPeak[ch]){
		stAmpPeakdB.dAmpPeak[ch] = amp_value_tmp;
	}

	// 1秒分積算?
	if(nPcmSamplingRate == stAmpPeakdB.ullAmpSumCnt[ch]){
		// 平均ピークデシベル算出
		CalcPeakdB(ch);
	}
}

// 振幅ピーク値取得 ※左右Chの大きい方
double CPCMDSD_ConverterDlg::GetPeakAmp()
{
	double dAmp_peak;

	// 左Ch、右Chの大きい方のピーク値を返却
	dAmp_peak = stAmp.dAmpPeak[0];
	if(stAmp.dAmpPeak[0] < stAmp.dAmpPeak[1]){
		dAmp_peak = stAmp.dAmpPeak[1];
	}

	return dAmp_peak;
}

// 平均デシベルピーク取得 ※左右Chの大きい方
double CPCMDSD_ConverterDlg::GetAvgPeakdB()
{
	double ddB_peak;

	// 平均ピーク値 ※左右Chの大きい方
	if(m_dAvgPeakdB[0] > m_dAvgPeakdB[1]){
		ddB_peak = m_dAvgPeakdB[0];
	} else {
		ddB_peak = m_dAvgPeakdB[1];
	}

	return ddB_peak;
}

// 平均ピークデシベル算出
void CPCMDSD_ConverterDlg::CalcPeakdB()
{
	// 左Ch
	CalcPeakdB(0);
	// 右Ch
	CalcPeakdB(1);
}

void CPCMDSD_ConverterDlg::CalcPeakdB(int ch)
{
	double amp_avg;
	double db;

	if(stAmpPeakdB.ullAmpSumCnt[ch] == 0){
		return;
	}

	// 平均
	amp_avg = stAmpPeakdB.dAmpSum[ch] / (double)stAmpPeakdB.ullAmpSumCnt[ch];
	if(amp_avg < DBL_EPSILON){
		amp_avg = DBL_EPSILON;
	}

	// 平均RMS dB
	db = GetPowertodB(amp_avg);

	if(m_dAvgPeakdB[ch] == 0){
		// 平均ピークdB初回
		m_dAvgPeakdB[ch] = db;
	} else {
		if(m_dAvgPeakdB[ch] < db){
			// 平均ピークdB更新
			m_dAvgPeakdB[ch] = db;
		}
	}

	// クリア
	stAmpPeakdB.dAmpSum[ch] = 0;
	stAmpPeakdB.ullAmpSumCnt[ch] = 0;
}

// 平均RMSデシベル取得
double CPCMDSD_ConverterDlg::GetAveragedB(double *dbL, double *dbR)
{
	double amp_avg[2];
	double db[2];
	double db_ret;

	// 平均
	amp_avg[0] = (double)(stAmp.dAmpSum[0] / stAmp.ullAmpSumCnt[0]);
	amp_avg[1] = (double)(stAmp.dAmpSum[1] / stAmp.ullAmpSumCnt[1]);

	// 振幅0.0は、無限なので最小値未満は0に近い値とする
	if (amp_avg[0] < DBL_EPSILON) {
		amp_avg[0] = DBL_EPSILON;
	}
	if (amp_avg[1] < DBL_EPSILON) {
		amp_avg[1] = DBL_EPSILON;
	}

	// RMS dB ※20 × log10(amp ÷ 1.0)
	db[0] = 20 * log10(amp_avg[0]);
	db[1] = 20 * log10(amp_avg[1]);

	if(dbL != NULL){
		*dbL = db[0];
	}
	if(dbR != NULL){
		*dbR = db[1];
	}

	// 左右の平均dB
	db_ret = (db[0] + db[1]) / 2.0;

	// 平均ピークdB値 [0]:Ch L [1]:Ch R
//	if(db[0] < m_dAvgPeakdB[0]){
//		m_dAvgPeakdB[0] = db[0];
//	}
//	if(db[1] < m_dAvgPeakdB[1]){
//		m_dAvgPeakdB[1] = db[1];
//	}

	return db_ret;
}

// デシベルから倍率取得
double CPCMDSD_ConverterDlg::GetdBtoPower(double ddB)
{
	double dPower;

	// 倍率=10の(dB ÷ 20)乗
	dPower = pow(10, ddB / 20.0);

	return dPower;
}

// 振幅からデシベル取得
double CPCMDSD_ConverterDlg::GetPowertodB(double dAmpPower)
{
	double db;

	// RMS dB
	db = 20 * log10(dAmpPower / 1);

	return db;
}

//FFTプラン初期化
//FFT FIRで処理は削減しているものの、アップサンプリングの最後の方ではさすがにFFTサイズが大きく、処理が重い
//FFTW_Wisdomや、CPU拡張命令を試したが、目に見える改善はせず。要対策。
void CPCMDSD_ConverterDlg::FFTInit(fftw_plan *plan, unsigned int fftsize, int Times, double *fftin, fftw_complex *ifftout){
	fftw_plan_with_nthreads(omp_get_num_procs() / 2);
	*plan = fftw_plan_dft_r2c_1d(int(fftsize / Times), fftin, ifftout, FFTW_ESTIMATE);
}
void CPCMDSD_ConverterDlg::iFFTInit(fftw_plan *plan, unsigned int fftsize, int Times, fftw_complex *ifftin, double *fftout){
	fftw_plan_with_nthreads(omp_get_num_procs() / 2);
	*plan = fftw_plan_dft_c2r_1d(int(fftsize / Times), ifftin, fftout, FFTW_ESTIMATE);
}

//改良版FIRフィルタ版PCM-DSD変換
//#define FIR_DELTA_GAIN	0.4
//#define FIR_DELTA_GAIN	0.45
#define FIR_DELTA_GAIN	0.5
bool CPCMDSD_ConverterDlg::WAV_Filter(FILE *UpSampleData, FILE *OrigData, unsigned int Times, omp_lock_t *myLock, int *pErr, int mode)
{
	omp_set_lock(myLock);

	bool bRet = true;

	*pErr = 0;

	//FIRフィルタ係数読み込み
	//タップ数は2^N-1
	ifstream ifs(".\\FIRFilter.dat");
	unsigned int section_1 = 0;
	string str;
	unsigned __int64 samplesize;
	unsigned __int64 nRemainingSize;
	size_t ullFwRet;
	size_t ullFwCount;
	if (ifs.fail())
	{
		return false;
	}
	getline(ifs, str);
	section_1 = atoi(str.c_str());
	double* firfilter_table = new double[section_1];
	unsigned __int32 i = 0;
	unsigned __int32 j = 0;
	while (getline(ifs, str))
	{
		firfilter_table[i] = atof(str.c_str());
		i++;
	}

	ifs.close();

	//ノイズシェーピング係数読み込み
	ifstream ifsNS(".\\NoiseShapingCoeff.dat");
	if (ifsNS.fail())
	{
		exit(EXIT_FAILURE);
	}
	i = 0; int s = 0;
	getline(ifsNS, str);
	unsigned int order = atoi(str.c_str());

	double** NS = new double* [2];
	NS[0] = new double[order];
	NS[1] = new double[order];
	while (getline(ifsNS, str))
	{
		if (str != "0") {
			if (i == 0)
				NS[i][s] = atof(str.c_str());
			else {
				NS[i][order - s - 1] = atof(str.c_str());
			}
			s++;
		}
		else {
			s = 0;
			i++;
		}
	}
	ifsNS.close();
	for (i = 0; i < order; i++) {
		NS[1][i] = NS[1][i];
		NS[0][i] = NS[0][i] - NS[1][i];
	}
	_fseeki64(OrigData, 0, SEEK_SET);
	samplesize = _filelengthi64(_fileno(OrigData));
	nRemainingSize = samplesize;
	samplesize = samplesize / 8;

	//FFT Overlap add Methodを用いたFIRフィルタ畳みこみ演算
	//x(L),h(N),FFT(M)としたとき、M>=L+N-1になる必要があるので
	//最終アップサンプリング時にM=2*L=2*(N+1)となるように定義
	const unsigned int logtimes = unsigned int(log(Times) / log(2));
	const unsigned int fftsize = (section_1 + 1) * Times;
	const unsigned int datasize = fftsize / 2;
	unsigned int *nowfftsize = new  unsigned int[logtimes];
	unsigned int *zerosize = new  unsigned int[logtimes];
	unsigned int *puddingsize = new  unsigned int[logtimes];
	unsigned int *realfftsize = new unsigned int[logtimes];
//未使用かつメモリリークするのでコメント	unsigned int *addsize = new  unsigned int[logtimes];
	double **prebuffer = new double*[logtimes];
	double gain = 1.0;

	double *buffer = new double[fftsize];
	unsigned char *out = new unsigned char[datasize];
	for (i = 0; i < datasize; i++) {
		out[i] = 0;
	}

	double* deltabuffer = new double[order + 1];
	for (i = 0; i < order; i++) {
		deltabuffer[i] = 0;
	}

	double x_in = 0;
	double error_y = 0;
	double deltagain = 0.5;
	double amp_peak;
	double normalizeratio;

	// FIR FFT用変数
	double** fftin = (double**)fftw_malloc(sizeof(double) * logtimes);
	fftw_complex** fftout = (fftw_complex**)fftw_malloc(sizeof(fftw_complex) * logtimes);
	fftw_complex** ifftin = (fftw_complex**)fftw_malloc(sizeof(fftw_complex) * logtimes);
	double** ifftout = (double**)fftw_malloc(sizeof(double) * logtimes);
	fftw_complex** firfilter_table_fft = (fftw_complex**)fftw_malloc(sizeof(fftw_complex) * logtimes);

	fftw_plan* FFT = (fftw_plan*)fftw_malloc(sizeof(fftw_plan) * (logtimes));
	fftw_plan* iFFT = (fftw_plan*)fftw_malloc(sizeof(fftw_plan) * (logtimes));

	unsigned int p = 0;
	unsigned int k = 0;
	unsigned int t = 0;
	unsigned int q = 0;
	for (i = 1; i < Times; i = i * 2) {
		nowfftsize[p] = fftsize / (Times / (i * 2));
		realfftsize[p] = nowfftsize[p] / 2 + 1;
		zerosize[p] = nowfftsize[p] / 4;
		puddingsize[p] = nowfftsize[p] - zerosize[p] * 2;
		gain = gain * (2.0 / nowfftsize[p]);
//		addsize[p] = zerosize[p] * 2;

		prebuffer[p] = new double[fftsize];
		firfilter_table_fft[logtimes - p - 1] = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * unsigned int(fftsize / i));
		fftin[logtimes - p - 1] = (double*)fftw_malloc(sizeof(double) * unsigned int(fftsize / i));
		fftout[logtimes - p - 1] = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * unsigned int(fftsize / i));
		ifftin[logtimes - p - 1] = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * unsigned int((fftsize / i + 1) / 2 + 1));
		ifftout[logtimes - p - 1] = (double*)fftw_malloc(sizeof(double) * unsigned int(fftsize / i));

		for (k = 0; k < fftsize / i; k++) {
			fftin[logtimes - p - 1][k] = 0;
			ifftout[logtimes - p - 1][k] = 0;
			fftout[logtimes - p - 1][k][0] = 0;
			fftout[logtimes - p - 1][k][1] = 0;
			ifftin[logtimes - p - 1][k / 2][0] = 0;
			ifftin[logtimes - p - 1][k / 2][1] = 0;
		}
		for (k = 0; k < fftsize; k++) {
			prebuffer[p][k] = 0;
		}
		p++;
	}
	p = 0;
	//!!!FFTW3ライブラリに同時アクセスすると落ちる
	for (i = 1; i < Times; i = i * 2){
		FFTInit(&FFT[logtimes - p - 1], fftsize, int(i), fftin[logtimes - p - 1], fftout[logtimes - p - 1]);
		iFFTInit(&iFFT[logtimes - p - 1], fftsize, int(i), ifftin[logtimes - p - 1], ifftout[logtimes - p - 1]);
		p++;
	}
	///H(n)準備
	for (k = 0; k < logtimes; k++) {
		for (i = 0; i < section_1; i++) {
			fftin[k][i] = firfilter_table[i];
		}
		for (i = section_1; i < fftsize / pow(2, k); i++) {
			fftin[logtimes - k - 1][i] = 0;
		}
	}
	for (i = 0; i < logtimes; i++) {
		fftw_execute(FFT[logtimes - i - 1]);
		for (p = 0; p < fftsize / pow(2, i + 1) + 1; p++) {
			firfilter_table_fft[logtimes - i - 1][p][0] = fftout[logtimes - i - 1][p][0];
			firfilter_table_fft[logtimes - i - 1][p][1] = fftout[logtimes - i - 1][p][1];
		}
	}
//	deltagain = gain * deltagain;
	omp_unset_lock(myLock);
	// 予定出力ファイルサイズ 64BIT WAV / double8Byte * DSD倍数
	UINT64 nDestFileSize = (nRemainingSize / 8) * Times;
	// ループ回数算出
	UINT SplitNum = (UINT)((nDestFileSize / (UINT64)datasize) + ((nDestFileSize % (UINT64)datasize) != 0));
	// 64BIT PCM読込みデータ数
	UINT nReadCnt = datasize / Times;

#if GAIN_CONTROL_MODE
	deltagain = gain * FIR_DELTA_GAIN;
#else
//	deltagain = gain * (0.5 - m_dGain);
	deltagain = gain * (m_dGain / 2);
#endif
	// 左右同じ比率でノーマライズする為、またクリップオーバーしない為に左Ch、右Chの大きい方のピーク値を利用する
//	amp_peak = stAmp.dAmpPeak[0];
//	if(stAmp.dAmpPeak[0] < stAmp.dAmpPeak[1]){
//		amp_peak = stAmp.dAmpPeak[1];
//	}
	amp_peak = GetPeakAmp();
	// ピーク値からノーマライズ乗算値を求める
	normalizeratio = AMP_LIMIT_MAX / amp_peak;

	/*LARGE_INTEGER cpuFreq;
	LARGE_INTEGER count1, count2, count3, count4;
	QueryPerformanceFrequency(&cpuFreq);

	QueryPerformanceCounter(&count1);

	*/

	//Convolution&UpSampling
	for (i = 0; i < SplitNum; i++){
		m_dProgress.Process(2, i + 1, SplitNum, m_DSD_Times);//子ダイアログのプログレスバーに値を投げる
//			fread(buffer, 8, datasize / Times, OrigData);
		if(nRemainingSize > nReadCnt * 8){
			fread(buffer, 8, nReadCnt, OrigData);
		} else {
			memset(buffer, 0x00, fftsize);
			fread(buffer, 8, nRemainingSize / 8, OrigData);
		}
		// ノーマライズ有効?
		if(m_NormalizeEnable == 1 || m_ClipErrNormalizeEnable == 1){
			// ノーマライズする
//				for(j = 0;j < datasize / Times;j ++){
			for(j = 0;j < nReadCnt;j ++){
				buffer[j] *= normalizeratio;
			}
		}
#if GAIN_CONTROL_MODE
		// ゲイン
		for(j = 0;j < nReadCnt;j ++){
			buffer[j] *= m_dGain;
		}
#endif
		for (t = 0; t < logtimes; t++){
			q = 0;
			for (p = 0; p < zerosize[t]; p++){
				fftin[t][q] = buffer[p];
				q++;
				fftin[t][q] = 0;
				q++;
			}
			memset(fftin[t] + q, 0, 8 * (nowfftsize[t] - q));
			/*for (p = q; p < nowfftsize[t]; p++){
				fftin[t][p] = 0;
				}*/
			fftw_execute(FFT[t]);
			for (p = 0; p < realfftsize[t]; p++){
				ifftin[t][p][0] = fftout[t][p][0] * firfilter_table_fft[t][p][0] - fftout[t][p][1] * firfilter_table_fft[t][p][1];
				ifftin[t][p][1] = fftout[t][p][0] * firfilter_table_fft[t][p][1] + firfilter_table_fft[t][p][0] * fftout[t][p][1];
			}
			fftw_execute(iFFT[t]);
			for (p = 0; p < puddingsize[t]; p++){
				buffer[p] = prebuffer[t][p] + ifftout[t][p];
			}
			q = 0;
			for (p = puddingsize[t]; p < nowfftsize[t]; p++){
				buffer[p] = prebuffer[t][q] = ifftout[t][p];
				q++;
			}
		}
		//1bit化
		//Direct Form II型で⊿Σ変調
		for (q = 0; q < datasize; q++){

			x_in = buffer[q] * deltagain;

			for (t = 0; t < order; t++) {
				x_in += NS[0][t] * deltabuffer[t];
			}

			if (x_in >= 0.0) {
				out[q] = 1;
				error_y = -1.0;
			}
			else {
				out[q] = 0;
				error_y = 1.0;
			}
			for (t = order; t > 0; t--) {
				deltabuffer[t] = deltabuffer[t - 1];
			}

			deltabuffer[0] = x_in + error_y;

			if(fabs(deltabuffer[0]) > 1.0){
//					TRACE("FIRクリップオーバー\n");
				TRACE("FIRクリップオーバー Ch:%d Amp:%f\n", mode, deltabuffer[0]);
				if(m_DsdClipOverChkEnable == 1){
					// クリップオーバー+1
					m_nDsdClipOverCnt += (m_nDsdClipOverCnt < UINT64_MAX);
					m_bDsdClipOver = TRUE;
					break;
				}
			}

			for (t = 0; t < order; t++) {
				deltabuffer[0] += NS[1][t] * deltabuffer[t + 1];
			}
		}

		if(m_bDsdClipOver == TRUE){
			// FIRクリップオーバー
			bRet = false;
			break;
		}

//			fwrite(out, 1, datasize, UpSampleData);
		if(nRemainingSize > nReadCnt * 8){
			ullFwCount = datasize;
			ullFwRet = fwrite(out, 1, ullFwCount, UpSampleData);
			nRemainingSize -= (nReadCnt * 8);
			if(ullFwCount != ullFwRet){
				*pErr = -1;
				bRet = false;
				break;
			}
		} else {
			ullFwCount = (nRemainingSize / 8) * Times;
			ullFwRet = fwrite(out, 1, ullFwCount, UpSampleData);
			m_dProgress.Process(3, SplitNum, SplitNum, m_DSD_Times);
			if(ullFwCount != ullFwRet){
				*pErr = -1;
				bRet = false;
				break;
			}
			break;
		}

//			if (!m_dProgress.Cancelbottun) return false;//子ダイアログで中止ボタンが押された
		if (!m_dProgress.Cancelbottun){	//子ダイアログで中止ボタンが押された
			bRet = false;
			break;
		}
	}
	//	QueryPerformanceCounter(&count3);
	//	_fseeki64(OrigData, 0, SEEK_SET);
	//	_fseeki64(UpSampleData, 0, SEEK_SET);
	//	QueryPerformanceCounter(&count4);
	//}
	//QueryPerformanceCounter(&count2);

	//CStringW Timeword;
	//double time = 1000.0*(((double)count2.QuadPart - count1.QuadPart - (count3.QuadPart - count4.QuadPart) * 6) / (cpuFreq.QuadPart * 6));
	//Timeword.Format(L"%gms", time);
	//MessageBox(Timeword, L"計測結果", MB_OK);


	//お掃除
	for (i = 0; i < logtimes; i++) {
		fftw_destroy_plan(FFT[i]);
		fftw_destroy_plan(iFFT[i]);
		fftw_free(ifftin[i]);
		fftw_free(ifftout[i]);
		fftw_free(fftin[i]);
		fftw_free(fftout[i]);
		fftw_free(firfilter_table_fft[i]);
		delete[] prebuffer[i];
	}
	fftw_free(iFFT);
	fftw_free(FFT);
	fftw_free(ifftin);
	fftw_free(ifftout);
	fftw_free(fftin);
	fftw_free(fftout);
	fftw_free(firfilter_table_fft);
	delete[] NS[0];
	delete[] NS[1];
	delete[] NS;
	delete[] nowfftsize;
	delete[] zerosize;
	delete[] puddingsize;
	delete[] realfftsize;
//	delete[] addsize;
	delete[] out;
	delete[] prebuffer;
	delete[] buffer;
	delete[] deltabuffer;
	delete[] firfilter_table;
	return bRet;
}

//改良版IIRフィルタ版軽量PCM-DSD変換
//#define IIR_DELTA_GAIN	0.9
#define IIR_DELTA_GAIN	0.95
//#define IIR_DELTA_GAIN	1.0
bool CPCMDSD_ConverterDlg::WAV_FilterLight(FILE *UpSampleData, FILE *OrigData, unsigned int Times, int *pErr, int mode)
{
	bool bRet = true;

	*pErr = 0;

	unsigned __int64 samplesize;
	unsigned __int64 nRemainingSize;
	_fseeki64(OrigData, 0, SEEK_END);
	samplesize = _ftelli64(OrigData);
	_fseeki64(OrigData, 0, SEEK_SET);
	nRemainingSize = samplesize;
	samplesize = samplesize / 8;
	unsigned int SplitNum = 4096;
	const unsigned int logtimes = unsigned int(log(Times) / log(2));
//	const unsigned int datasize = unsigned int(samplesize / SplitNum);
	const unsigned int datasize = unsigned int(samplesize / SplitNum + ((samplesize % SplitNum) != 0));
	const unsigned int Updatasize = datasize*Times;
	double *buffer = new double[Updatasize];
	double *databuffer = new double[Updatasize];

	unsigned char* out = new unsigned char[Updatasize];

	double A[12] = { 0 };
	double B[12] = { 0 };
	double deltabuffer[13] = { 0 };
	double x_in = 0;
	double error_y = 0;


#if GAIN_CONTROL_MODE
//	double deltagain = Times / 2;
	double deltagain = (Times / 2) * IIR_DELTA_GAIN;
#else
//	double deltagain = (Times / 2) * (1.0 - m_dGain);
	double deltagain = (Times / 2) * (m_dGain);
#endif
	double amp_peak;
	double normalizeratio;

	size_t ullFwRet;
	size_t ullFwCount;

	// 左右同じ比率でノーマライズする為、またクリップオーバーしない為に左Ch、右Chの大きい方のピーク値を利用する
//	amp_peak = stAmp.dAmpPeak[0];
//	if(stAmp.dAmpPeak[0] < stAmp.dAmpPeak[1]){
//		amp_peak = stAmp.dAmpPeak[1];
//	}
	amp_peak = GetPeakAmp();
	// ピーク値からノーマライズ乗算値を求める
	normalizeratio = AMP_LIMIT_MAX / amp_peak;

	//FIRフィルタ係数読み込み
	//タップ数は2^N-1
	unsigned int  section_1 = 0;
	unsigned int i = 0;
	unsigned int j;
	ifstream ifs(".\\IIRFilter.dat");
	string str;
	unsigned int  s = 0;
	if (ifs.fail())
	{
		exit(EXIT_FAILURE);
	}
	getline(ifs, str);
	section_1 = atoi(str.c_str());
	getline(ifs, str);
	double **iirfilter_table = new double*[section_1];
	for (i = 0; i < section_1; i++){
		iirfilter_table[i] = new double[6];
	}
	i = 0;
	while (getline(ifs, str))
	{
		if (str != ""){
			iirfilter_table[i][s] = atof(str.c_str());
			s++;
		}
		else{
			s = 0;
			i++;
		}
	}
	ifs.close();

	//ノイズシェーピング係数読み込み
	ifstream ifsNS(".\\NoiseShapingCoeff.dat");
	if (ifsNS.fail())
	{
		exit(EXIT_FAILURE);
	}
	i = 0; s = 0;
	getline(ifsNS, str);
	unsigned int order = atoi(str.c_str());
	double** NS = new double* [2];
	NS[0] = new double[order];
	NS[1] = new double[order];
	while (getline(ifsNS, str))
	{
		if (str != "0") {
			if (i == 0)
				NS[i][s] = atof(str.c_str());
			else {
				NS[i][order - s - 1] = atof(str.c_str());
			}
			s++;
		}
		else {
			s = 0;
			i++;
		}
	}
	ifsNS.close();
	for (i = 0; i < order; i++) {
		NS[1][i] = NS[1][i];
		NS[0][i] = NS[0][i] - NS[1][i];
	}

	double *nowdatasize = new double[logtimes];
	double ***qe_1 = new double**[logtimes];
	for (i = 0; i < logtimes; i++){
		nowdatasize[i] = datasize*pow(2, i);
		qe_1[i] = new double*[section_1];
		for (s = 0; s < section_1; s++){
			qe_1[i][s] = new double[3];
			qe_1[i][s][0] = 0; qe_1[i][s][1] = 0; qe_1[i][s][2] = 0;
		}
	}
	double tmp_iir = 0;
	unsigned int  x = 0;
	unsigned int a = 0; unsigned int p = 0; unsigned int q = 0; unsigned int t = 0; s = 0;

	for (i = 0; i < SplitNum; i++){
		m_dProgress.Process(2, i + 1, SplitNum, m_DSD_Times);//子ダイアログのプログレスバーに値を投げる
//		fread(databuffer, 8, datasize, OrigData);
		if(nRemainingSize > datasize * 8){
			fread(databuffer, 8, datasize, OrigData);
		} else {
			memset(databuffer, 0x00, Updatasize);
			fread(databuffer, 8, nRemainingSize / 8, OrigData);
		}

		// ノーマライズ有効?
		if(m_NormalizeEnable == 1 || m_ClipErrNormalizeEnable == 1){
			// ノーマライズする
			for(j = 0;j < datasize;j ++){
				databuffer[j] *= normalizeratio;
			}
		}
		// ゲイン
		for(j = 0;j < datasize;j ++){
			databuffer[j] *= m_dGain;
		}
		s = 0;
		for (t = 0; t < logtimes; t++){
			p = 0;
			for (q = 0; q < nowdatasize[t]; q++){
				buffer[p] = databuffer[q];
				p++;
				buffer[p] = 0;
				p++;
			}
			for (q = 0; q < nowdatasize[t] * 2; q++){
				tmp_iir = buffer[q];
				for (x = 0; x < section_1; x++){
					qe_1[s][x][0] = iirfilter_table[x][0] * tmp_iir - iirfilter_table[x][1] * qe_1[s][x][1] - iirfilter_table[x][2] * qe_1[s][x][2];
					tmp_iir = iirfilter_table[x][3] * qe_1[s][x][0] + iirfilter_table[x][4] * qe_1[s][x][1] + iirfilter_table[x][5] * qe_1[s][x][2];
					qe_1[s][x][2] = qe_1[s][x][1];
					qe_1[s][x][1] = qe_1[s][x][0];
				}
				buffer[q] = tmp_iir;
				databuffer[q] = tmp_iir;
			}
			s++;
		}
		//1bit化
		for (q = 0; q < Updatasize; q++){
			x_in = buffer[q] * deltagain;
			for (t = 0; t < order; t++) {
				x_in += NS[0][t] * deltabuffer[t];
			}

			if (x_in >= 0.0) {
				out[q] = 1;
				error_y = -1.0;
			}
			else {
				out[q] = 0;
				error_y = 1.0;
			}

			for (t = order; t > 0; t--) {
				deltabuffer[t] = deltabuffer[t - 1];
			}

			deltabuffer[0] = x_in + error_y;

			if (fabs(deltabuffer[0]) > 1.0) {
				TRACE("IIRクリップオーバー\n");
				if (m_DsdClipOverChkEnable == 1) {
					// クリップオーバー+1
					m_nDsdClipOverCnt += (m_nDsdClipOverCnt < UINT64_MAX);
					m_bDsdClipOver = TRUE;
					break;
				}
			}

			for (t = 0; t < order; t++) {
				deltabuffer[0] += NS[1][t] * deltabuffer[t + 1];
			}
		}

		if(m_bDsdClipOver == TRUE){
			// IIRクリップオーバー
			bRet = false;
			break;
		}

//		fwrite(out, 1, Updatasize, UpSampleData);
		if(nRemainingSize > datasize * 8){
			ullFwCount = Updatasize;
			ullFwRet = fwrite(out, 1, Updatasize, UpSampleData);
			nRemainingSize -= (datasize * 8);
			if(ullFwCount != ullFwRet){
				*pErr = -1;
				bRet = false;
				break;
			}
		} else {
			ullFwCount = (nRemainingSize / 8) * Times;
			ullFwRet = fwrite(out, 1, ullFwCount, UpSampleData);
			m_dProgress.Process(3, SplitNum, SplitNum, m_DSD_Times);
			if(ullFwCount != ullFwRet){
				*pErr = -1;
				bRet = false;
				break;
			}
			break;
		}
//		if (!m_dProgress.Cancelbottun) return false;//子ダイアログで中止ボタンが押された
		if (!m_dProgress.Cancelbottun){//子ダイアログで中止ボタンが押された
			bRet = false;
			break;
		}
	}
	/*QueryPerformanceCounter(&count3);
	_fseeki64(OrigData, 0, SEEK_SET);
	_fseeki64(UpSampleData, 0, SEEK_SET);
	QueryPerformanceCounter(&count4);
	}
	QueryPerformanceCounter(&count2);

	CStringW Timeword;
	double time = 1000.0*(((double)count2.QuadPart - count1.QuadPart - (count3.QuadPart - count4.QuadPart) * 6) / (cpuFreq.QuadPart * 6));
	Timeword.Format(L"%gms", time);
	MessageBox(Timeword, L"計測結果", MB_OK);*/

	//お掃除
	for (i = 0; i < logtimes; i++) {
		for (s = 0; s < section_1; s++) {
			delete[] qe_1[i][s];
		}
		delete[] qe_1[i];
	}
	delete []qe_1;
	for (i = 0; i < section_1; i++){
		delete[] iirfilter_table[i];
	}
	delete[] iirfilter_table;
//	for (t = 0; t < logtimes; t++){
//		delete[] qe_1[t];
//	}
//	delete[] qe_1;
//	delete[] iirfilter_table;
	delete[] NS[0];
	delete[] NS[1];
	delete[] NS;
	delete[] out;
	delete[] databuffer;
	delete[] buffer;
	delete[] nowdatasize;
	return bRet;
}

// 最終フォルダ名取得
CString CPCMDSD_ConverterDlg::GetLastPath(TCHAR *filepath)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	CString strLastPathName;

	_tsplitpath_s(filepath, drive, dir, filename, fileext);
	strLastPathName = PathFindFileName(dir);

	return strLastPathName;
}

// ファイル名取得
CString CPCMDSD_ConverterDlg::GetFileName(TCHAR *filepath)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	CString strFileName;

	_tsplitpath_s(filepath, drive, dir, filename, fileext);
	CString tmp = PathFindFileName(dir);
	strFileName = filename;
	strFileName += fileext;

	return strFileName;
}

//フリーズ防止のためにスレッド作成
UINT __cdecl CPCMDSD_ConverterDlg::WorkThread(LPVOID pParam)
{
	CPCMDSD_ConverterDlg* pDlg = (CPCMDSD_ConverterDlg*)pParam;
	int DSD_Times;
//	TCHAR ext_tmp[MAX_PATH];
	CString ext_tmp;
	CString strFileName;
	CString strFolderName;
//	string *metadata = new string[6];
	bool bDelFile = false;
	BOOL bTagEnable = FALSE;
	bool bFlacDecodeRet;
	int nConvertCount = 0;
	int nConvertCountTotal = 0;
	bool bLastData = true;
	EXT_TYPE etExtType;
	CString strClipOver = _T("DSD変換中にクリップオーバーが発生しました。\nゲインを下げて変換して下さい。");
	CString strTmp;
	CString strMsg;

	if(pDlg->m_evPath.GetLength() > 0){
		if (!PathFileExists(pDlg->m_evPath)) {
			pDlg->MessageBox(L"出力先ルートパスがありません。パスを選択して下さい。", L"処理エラー", MB_OK);
			return FALSE;
		}
	}

	// 変換完了電源制御
	pDlg->m_nCompletePowerCrl = pDlg->m_combCompleteOption.GetCurSel();
	if(pDlg->m_nCompletePowerCrl != 0){
		if(pDlg->m_DsfOverWriteEnable == FALSE){
			pDlg->m_combCompleteOption.GetWindowText(strTmp);
			strMsg = _T("[終了時の処理](");
			strMsg += strTmp;
			strMsg += _T(")を使用する場合は、[出力ファイル上書き]を有効にして下さい。");
			pDlg->MessageBox(strMsg, L"処理エラー", MB_OK);
			return FALSE;
		}
	}

	pDlg->m_nDsdClipOverCnt = 0;
	pDlg->m_bDsdClipOver = FALSE;
	pDlg->m_fillsize = 0;
	pDlg->m_DSD_Times = 0;
	pDlg->m_DataIdxArray.RemoveAll();
#if 0
	switch (pDlg->m_cSamplingRate.GetCurSel()) {
	case 0:											// DSD 64(2.8MHz)
		DSD_Times = 6;
		break;
	case 1:											// DSD128(5.6MHz)
		DSD_Times = 7;
		break;
	case 2:											// DSD256(11.2MHz)
		DSD_Times = 8;
		break;
	case 3:											// DSD512(22.5MHz)
		DSD_Times = 9;
		break;
	case 4:											// DSD1024(45.1MHz)
		DSD_Times = 10;
		break;
	case 5:											// DSD2048(90.3MHz)
		DSD_Times = 11;
		break;
	default:
		DSD_Times = 6;
		break;
	}

	// DSD64～2048倍を計算
	pDlg->m_DSD_Times = (unsigned int)pow(2, DSD_Times);
#endif
	DSD_Times = 6;	// 暫定
	pDlg->m_DSDsamplingRateCurSel = pDlg->m_cSamplingRate.GetCurSel();

	// ゲイン調整
	int nVol = pDlg->m_combVol.GetCurSel();
//	pDlg->m_dVolGain = nVol * 0.05;
//	pDlg->m_dVolGain = nVol * 0.025;
//	pDlg->m_dVolGain = nVol * 0.01;
	pDlg->m_dVolGain = pDlg->GetdBtoPower(nVol * -0.1);

	// ゲイン制限
	int nVolLimit = pDlg->m_combLimitVol.GetCurSel();
	pDlg->m_LimitdB = (GAIN_LIMIT_DB_OFFSET + nVolLimit) * -1;
	pDlg->m_DiffdB = 0;
	pDlg->m_dGain = 1;

	// ノーマライズ ※ゲイン調整のみ有効
	if(pDlg->m_radioGainModeDdv == 0){
		pDlg->m_NormalizeEnable = pDlg->m_NormalizeFlag;
	} else {
		pDlg->m_NormalizeEnable = FALSE;
	}

	pDlg->m_dProgress.CenterWindow();
	pDlg->m_dProgress.Start(_T(""));
	pDlg->Disable();

//	CString strTmp;

	pDlg->m_nTagMode = 0;
	// ID3v2タグ情報初期化
	pDlg->ID3v2TagInfoInit();

	// スリープ抑止
	::SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS);

	TCHAR runfile_tmp[ABSOLUTE_PATH_MAX];	// 本当はCStringに修正したい
	TCHAR chkfile_tmp[ABSOLUTE_PATH_MAX];	// 本当はCStringに修正したい

//	if (pDlg->flag_Bottun == "All"){
//	if (pDlg->m_dwConvertProcessFlag == 0 || pDlg->m_dwConvertProcessFlag == 1){
	if (pDlg->m_dwConvertProcessFlag == 0){
		//*** 全て実行 ***
		bool metaWav = false;
		int row_now = pDlg->m_lFileList.GetItemCount();
		int n = 0;
		int i = 0;
		bLastData = false;

		for (i = 0; i < row_now; i++){
			pDlg->m_bDsdClipOver = FALSE;
			pDlg->SetAlbumAndAlbumArtistOutputDir(_T(""), _T(""), _T(""), _T(""));

			_tcscpy_s(chkfile_tmp, pDlg->m_lFileList.GetItemText(n, EN_LIST_COLUMN_PATH));
			strFileName = pDlg->GetFileName(chkfile_tmp);
			ext_tmp = pDlg->m_lFileList.GetItemText(n, EN_LIST_COLUMN_EXT);
			etExtType = pDlg->GetExtType(ext_tmp);

//			pDlg->m_dProgress.Start(chkfile_tmp);
//			pDlg->m_dProgress.Process(0, 100);
//			if(_tcscmp(ext_tmp, _T("FLAC")) == 0){
			if(etExtType == EXT_TYPE_FLAC){
				// FLACならTAG取得、WAVにデコードしてから処理を開始
//				pDlg->m_dProgress.Start(chkfile_tmp);
				pDlg->m_dProgress.Start(strFileName);
				pDlg->m_dProgress.Process(0, 0, 100);
				// TAG取得
				pDlg->FLAC_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm, &bTagEnable);
				if (bTagEnable == TRUE) {
					pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
					pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				}
				// WAVにデコード
				memset(runfile_tmp, NULL, sizeof(runfile_tmp));
				bFlacDecodeRet = pDlg->FlacDecode(chkfile_tmp, runfile_tmp, sizeof(runfile_tmp));
				bDelFile = true;
			} else if (etExtType == EXT_TYPE_ALAC){
				// ALACならTAG取得、WAVにデコードしてから処理を開始
//				pDlg->m_dProgress.Start(chkfile_tmp);
				pDlg->m_dProgress.Start(strFileName);
				pDlg->m_dProgress.Process(5, 0, 100);
				// TAG取得
				pDlg->ALAC_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm);
				pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
				pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				// WAVにデコード
				memset(runfile_tmp, NULL, sizeof(runfile_tmp));
				bFlacDecodeRet = pDlg->AlacDecode(chkfile_tmp, runfile_tmp);
				bDelFile = true;
			} else if (etExtType == EXT_TYPE_WAV){
				// WAVならTAG取得
				// TAG取得
				pDlg->WAV_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm, &bTagEnable);
				if(bTagEnable == TRUE){
					pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
					pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				} else {
					pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, nConvertCount + 1, row_now, &pDlg->m_stID3tag);
				}
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(n, EN_LIST_COLUMN_PATH));
				bFlacDecodeRet = true;
				bDelFile = false;
			} else if (etExtType == EXT_TYPE_WAVE64) {
				// SONY WAVE64はTAG未対応なのでファイル名フォルダ名をTAGにする ※いつか対応させたいと思う・・・需要無さそうだけど(笑)
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(n, EN_LIST_COLUMN_PATH));
				pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, nConvertCount + 1, row_now, &pDlg->m_stID3tag);
				bFlacDecodeRet = true;
				bDelFile = false;
			} else {
				// DFFならそのまま処理を開始
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(n, EN_LIST_COLUMN_PATH));
				// TAG設定
				pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, 1, 1, &pDlg->m_stID3tag);

				bFlacDecodeRet = true;
				bDelFile = false;
			}
//			pDlg->m_dProgress.Start(chkfile_tmp);
			pDlg->m_dProgress.Start(strFileName);
			if(etExtType == EXT_TYPE_DFF){
				pDlg->m_dProgress.Process(4, 0, 100);
			} else {
				pDlg->m_dProgress.Process(1, 0, 100);
			}
			if (bFlacDecodeRet == true && pDlg->WAV_ConvertProc(etExtType, chkfile_tmp, runfile_tmp, n, bDelFile, bLastData, pDlg->m_dwConvertProcessFlag)) {
				pDlg->m_lFileList.DeleteItem(n);
				// リストコントロール登録件数表示
				pDlg->ListRegistDisp();
				nConvertCount ++;
			} else {
//				pDlg->MessageBox(_T("3"), _T("STEP"), MB_OK);
				int Times = 0;
				int samplingrate = _ttoi(pDlg->m_lFileList.GetItemText(n, EN_LIST_COLUMN_SAMPLING_RATE));
				if (0 == samplingrate % 44100){
					Times = DSD_Times - (samplingrate / 44100) + 1;
				}
				else{
					Times = DSD_Times - (samplingrate / 48000) + 1;
				}
//				strTmp.Format(_T("4 %d"), Times);
//				pDlg->MessageBox(strTmp, _T("STEP"), MB_OK);
#if 0	// for分の意味は？
				for (int i = 0; i < Times; i++){
					CString TimesStr;
					TimesStr.Format(L"%d", Times);
					pDlg->TrushFile(runfile_tmp, _T("_tmpL") + TimesStr);
					pDlg->TrushFile(runfile_tmp, _T("_tmpR") + TimesStr);
				}
#endif
				if(bDelFile == true){
//					TrushFile(runfile_tmp, _T(".wav"));
					// 暫定処理
					CString strDel(runfile_tmp);
//					strDel += _T(".wav");
					DeleteFile(strDel);
				}
				n++;
			}
			// ID3v2タグ情報初期化
			pDlg->ID3v2TagInfoInit();

			if (!pDlg->m_dProgress.Cancelbottun) break;
		}

		// ID3v2タグ情報初期化
//		pDlg->ID3v2TagInfoInit();
		// スリープ抑止解除
		::SetThreadExecutionState(ES_CONTINUOUS);

		if (nConvertCount != 0 || pDlg->m_nDsdClipOverCnt > 0) {
			// 完了通知処理
			pDlg->CompletionNotice();
		}

		// クリップオーバー?
		if (pDlg->m_nDsdClipOverCnt > 0) {
			pDlg->MessageBox(strClipOver, L"エラー", MB_OK);
		}

		pDlg->Enable();

		// 変換完了時の電源制御
		if (pDlg->m_nDsdClipOverCnt == 0) {
			pDlg->CompletePowerControl(pDlg->m_nCompletePowerCrl);
		}

//		if (i != 0){
		if (nConvertCount != 0){
			TCHAR errorMessage1[128];
//			wsprintf(errorMessage1, TEXT("%d"), i - n);
			wsprintf(errorMessage1, TEXT("%d"), nConvertCount);
			TCHAR *errorMessage2 = L"件の処理を実行しました";
			lstrcat(errorMessage1, errorMessage2);
			pDlg->MessageBox(errorMessage1, L"処理完了", MB_OK);
		}
	} else if (pDlg->m_dwConvertProcessFlag == 1){
		//*** アルバム実行 ***
		bool metaWav = false;
		int row_max = pDlg->m_lFileList.GetItemCount();
		int row_now;
		bLastData = false;

//		for (i = 0; i < row_now; i++){
		while(0 < pDlg->m_lFileList.GetItemCount()){
			pDlg->SetAlbumAndAlbumArtistOutputDir(_T(""), _T(""), _T(""), _T(""));

			// アルバムのトラック数取得
			row_now = pDlg->GetAlbumCount();

			if(nConvertCount == row_now - 1){
				bLastData = true;
			}
			_tcscpy_s(chkfile_tmp, pDlg->m_lFileList.GetItemText(nConvertCount, EN_LIST_COLUMN_PATH));
			strFolderName = pDlg->GetLastPath(chkfile_tmp);
			strFileName = pDlg->GetFileName(chkfile_tmp);
//			strFileName = chkfile_tmp;
			ext_tmp = pDlg->m_lFileList.GetItemText(nConvertCount, EN_LIST_COLUMN_EXT);
			etExtType = pDlg->GetExtType(ext_tmp);

			// フルパスからディレクトリパスだけ取得
//			pDlg->m_strSrcAlbumPath = pDlg->GetDirectoryPath(chkfile_tmp);
			pDlg->m_strSrcAlbumPath = pDlg->GetLastPath(chkfile_tmp);

//			pDlg->m_dProgress.Start(chkfile_tmp);
//			pDlg->m_dProgress.Process(0, 100);
//			pDlg->m_dProgress.StartSeq(0, chkfile_tmp, nConvertCount, row_now - 1);
			pDlg->m_dProgress.StartSeq(0, strFolderName, strFileName, nConvertCount, row_now - 1);

//			if(_tcscmp(ext_tmp, _T("FLAC")) == 0){
			if(etExtType == EXT_TYPE_FLAC){
				// FLACならTAG取得、WAVにデコードしてから処理を開始
//				pDlg->m_dProgress.StartSeq(0, chkfile_tmp, nConvertCount, row_now - 1);
				pDlg->m_dProgress.Process(0, nConvertCount, row_now - 1);
				// TAG取得
				pDlg->FLAC_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm, &bTagEnable);
				if (bTagEnable == TRUE) {
					pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
					pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				}
				// WAVにデコード
				memset(runfile_tmp, NULL, sizeof(runfile_tmp));
				bFlacDecodeRet = pDlg->FlacDecode(chkfile_tmp, runfile_tmp, sizeof(runfile_tmp));
				bDelFile = true;
			} else if (etExtType == EXT_TYPE_ALAC){
				// ALACならTAG取得、WAVにデコードしてから処理を開始
//				pDlg->m_dProgress.Start(chkfile_tmp);
//				pDlg->m_dProgress.Process(5, 0, 100);
////				pDlg->m_dProgress.StartSeq(0, chkfile_tmp, nConvertCount, row_now - 1);
				pDlg->m_dProgress.Process(5, nConvertCount, row_now - 1);
				// TAG取得
				pDlg->ALAC_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm);
				pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
				pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				// WAVにデコード
				memset(runfile_tmp, NULL, sizeof(runfile_tmp));
				bFlacDecodeRet = pDlg->AlacDecode(chkfile_tmp, runfile_tmp);
				bDelFile = true;
			} else if (etExtType == EXT_TYPE_WAV){
				// WAVならTAG取得
				// TAG取得
				pDlg->WAV_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm, &bTagEnable);
				if(bTagEnable == TRUE){
					pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
					pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				} else {
					pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, nConvertCount + 1, row_now, &pDlg->m_stID3tag);
				}
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(nConvertCount, EN_LIST_COLUMN_PATH));
				bFlacDecodeRet = true;
				bDelFile = false;
			} else if (etExtType == EXT_TYPE_WAVE64) {
				// SONY WAVE64はTAG未対応なのでファイル名フォルダ名をTAGにする ※いつか対応させたいと思う・・・需要無さそうだけど(笑)
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(nConvertCount, EN_LIST_COLUMN_PATH));
				pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, nConvertCount + 1, row_now, &pDlg->m_stID3tag);
				bFlacDecodeRet = true;
				bDelFile = false;
			} else {
				// DFFならそのまま処理を開始
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(nConvertCount, EN_LIST_COLUMN_PATH));

				// TAG設定
				pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, nConvertCount + 1, row_now, &pDlg->m_stID3tag);

				bFlacDecodeRet = true;
				bDelFile = false;
			}
//			pDlg->m_dProgress.StartSeq(0, chkfile_tmp);
			if(etExtType == EXT_TYPE_DFF){
//				pDlg->m_dProgress.Start(chkfile_tmp);
				pDlg->m_dProgress.Start(strFileName);
				pDlg->m_dProgress.Process(4, 0, 100);
			} else {
				pDlg->m_dProgress.Process(1, nConvertCount, row_now - 1);
			}
			if (bFlacDecodeRet == true && pDlg->WAV_ConvertProc(etExtType, chkfile_tmp, runfile_tmp, nConvertCount, bDelFile, bLastData, pDlg->m_dwConvertProcessFlag)) {
				nConvertCount ++;
				nConvertCountTotal ++;
			}
			else{
//				pDlg->MessageBox(_T("3"), _T("STEP"), MB_OK);
				int Times = 0;
				int samplingrate = _ttoi(pDlg->m_lFileList.GetItemText(nConvertCount, EN_LIST_COLUMN_SAMPLING_RATE));
				if (0 == samplingrate % 44100){
					Times = DSD_Times - (samplingrate / 44100) + 1;
				}
				else{
					Times = DSD_Times - (samplingrate / 48000) + 1;
				}
//				strTmp.Format(_T("4 %d"), Times);
				if(bDelFile == true){
					CString strDel(runfile_tmp);
					DeleteFile(strDel);
				}
				// ID3v2タグ情報初期化
				pDlg->ID3v2TagInfoDataIdxArrayClear();
				break;
			}
			// ID3v2タグ情報初期化
//			pDlg->ID3v2TagInfoInit();

			if (!pDlg->m_dProgress.Cancelbottun) {
				if(bDelFile == true){
					CString strDel(runfile_tmp);
					BOOL bRet;
					bRet = DeleteFile(strDel);
					TRACE(_T("DeleteFile:%d, %d\n"), bRet, GetLastError());
				}
				pDlg->TrushFile(pDlg->m_strSeqModeTempFile.GetBuffer(), _T("_tmpL0"));
				pDlg->TrushFile(pDlg->m_strSeqModeTempFile.GetBuffer(), _T("_tmpR0"));
				pDlg->TrushFile(pDlg->m_strSeqModeTempFile.GetBuffer(), _T("_tmpLDSD"));
				pDlg->TrushFile(pDlg->m_strSeqModeTempFile.GetBuffer(), _T("_tmpRDSD"));
				// ID3v2タグ情報初期化
				pDlg->ID3v2TagInfoDataIdxArrayClear();
				break;
			}
			
			if(bLastData == true){
				// リスト削除
				for (int i = 0; i < row_now; i++){
					pDlg->m_lFileList.DeleteItem(0);
				}
				// リストコントロール登録件数表示
				pDlg->ListRegistDisp();

				pDlg->m_fillsize = 0;
				pDlg->m_DSD_Times = 0;
				pDlg->m_nTagMode = 0;

//				pDlg->m_DataIdxArray.RemoveAll();
//				// ID3v2タグ情報初期化
//				pDlg->ID3v2TagInfoInit();
				// ID3v2タグ情報初期化
				pDlg->ID3v2TagInfoDataIdxArrayClear();

				nConvertCount = 0;
				bLastData = false;
			}
		}

		// ID3v2タグ情報初期化
//		pDlg->ID3v2TagInfoInit();
		// スリープ抑止解除
		::SetThreadExecutionState(ES_CONTINUOUS);

		if (nConvertCountTotal == row_max || pDlg->m_nDsdClipOverCnt > 0){
			// 完了通知処理
			pDlg->CompletionNotice();
		}

		// クリップオーバー?
		if (pDlg->m_nDsdClipOverCnt > 0) {
			pDlg->MessageBox(strClipOver, L"エラー", MB_OK);
		}

		pDlg->Enable();

		// 変換完了時の電源制御
		if (pDlg->m_nDsdClipOverCnt == 0) {
			pDlg->CompletePowerControl(pDlg->m_nCompletePowerCrl);
		}

//		if (i != 0){
		if (nConvertCountTotal == row_max){
			TCHAR errorMessage1[128];
//			wsprintf(errorMessage1, TEXT("%d"), i - n);
			wsprintf(errorMessage1, TEXT("%d"), nConvertCountTotal);
			TCHAR *errorMessage2 = L"件の処理を実行しました";
			lstrcat(errorMessage1, errorMessage2);
			pDlg->MessageBox(errorMessage1, L"処理完了", MB_OK);
		}
	} else {
		//*** 実行 ***
		int i = 0;
		int iRow;
		bool metaWav = false;
		POSITION pos = pDlg->m_lFileList.GetFirstSelectedItemPosition();
		while (pos){
			pDlg->SetAlbumAndAlbumArtistOutputDir(_T(""), _T(""), _T(""), _T(""));

			iRow = pDlg->m_lFileList.GetNextSelectedItem(pos);
			_tcscpy_s(chkfile_tmp, pDlg->m_lFileList.GetItemText(iRow - i, EN_LIST_COLUMN_PATH));
			strFileName = pDlg->GetFileName(chkfile_tmp);
			ext_tmp = pDlg->m_lFileList.GetItemText(iRow - i, EN_LIST_COLUMN_EXT);
			etExtType = pDlg->GetExtType(ext_tmp);

//			pDlg->m_dProgress.Start(chkfile_tmp);
//			pDlg->m_dProgress.Process(0, 100);

//			if(_tcscmp(ext_tmp, _T("FLAC")) == 0){
			if(etExtType == EXT_TYPE_FLAC){
				// FLACならTAG取得、WAVにデコードしてから処理を開始
//				pDlg->m_dProgress.Start(chkfile_tmp);
				pDlg->m_dProgress.Start(strFileName);
				pDlg->m_dProgress.Process(0, 0, 100);
				// TAG取得
				pDlg->FLAC_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm, &bTagEnable);
				if (bTagEnable == TRUE) {
					pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
					pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				}
				// WAVにデコード
				memset(runfile_tmp, NULL, sizeof(runfile_tmp));
				bFlacDecodeRet = pDlg->FlacDecode(chkfile_tmp, runfile_tmp, sizeof(runfile_tmp));
				bDelFile = true;
			} else if (etExtType == EXT_TYPE_ALAC){
				// ALACならTAG取得、WAVにデコードしてから処理を開始
//				pDlg->m_dProgress.Start(chkfile_tmp);
				pDlg->m_dProgress.Start(strFileName);
				pDlg->m_dProgress.Process(5, 0, 100);
				// TAG取得
				pDlg->ALAC_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm);
				pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
				pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				// WAVにデコード
				memset(runfile_tmp, NULL, sizeof(runfile_tmp));
				bFlacDecodeRet = pDlg->AlacDecode(chkfile_tmp, runfile_tmp);
				bDelFile = true;
			} else if (etExtType == EXT_TYPE_WAV){
				// WAVならTAG取得
				// TAG取得
				pDlg->WAV_Tagdata(chkfile_tmp, &pDlg->m_stFlacComm, &bTagEnable);
				if(bTagEnable == TRUE){
					pDlg->m_nTagMode = pDlg->FlacTagToID3v2Tag(&pDlg->m_stFlacComm, &pDlg->m_stID3tag);
					pDlg->SetAlbumAndAlbumArtistOutputDir(pDlg->m_stFlacComm.strAlbum, pDlg->m_stFlacComm.strAlbumArtist, pDlg->m_stFlacComm.strDiscnumber, pDlg->m_stFlacComm.strDisctotal);
				} else {
					pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, 1, 1, &pDlg->m_stID3tag);
				}
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(iRow - i, EN_LIST_COLUMN_PATH));
				bFlacDecodeRet = true;
				bDelFile = false;
			} else if (etExtType == EXT_TYPE_WAVE64) {
				// SONY WAVE64はTAG未対応なのでファイル名フォルダ名をTAGにする ※いつか対応させたいと思う・・・需要無さそうだけど(笑)
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(iRow - i, EN_LIST_COLUMN_PATH));
				pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, 1, 1, &pDlg->m_stID3tag);
				bFlacDecodeRet = true;
				bDelFile = false;
			} else {
				// DFFならそのまま処理を開始
				_tcscpy_s(runfile_tmp, pDlg->m_lFileList.GetItemText(iRow - i, EN_LIST_COLUMN_PATH));
				// TAG設定
				pDlg->m_nTagMode = pDlg->SetTagdataForWav(chkfile_tmp, 1, 1, &pDlg->m_stID3tag);

				bFlacDecodeRet = true;
				bDelFile = false;
			}
//			pDlg->m_dProgress.Start(chkfile_tmp);
			pDlg->m_dProgress.Start(strFileName);
			if(etExtType == EXT_TYPE_DFF){
				pDlg->m_dProgress.Process(4, 0, 100);
			} else {
				pDlg->m_dProgress.Process(1, 0, 100);
			}
			if (bFlacDecodeRet == true && pDlg->WAV_ConvertProc(etExtType, chkfile_tmp, runfile_tmp, iRow - i, bDelFile)){
				pDlg->m_lFileList.DeleteItem(iRow - i);
				// リストコントロール登録件数表示
				pDlg->ListRegistDisp();
				nConvertCount ++;
				i++;
			}
			else{
				int Times = 0;
				int samplingrate = _ttoi(pDlg->m_lFileList.GetItemText(iRow - i, EN_LIST_COLUMN_SAMPLING_RATE));
				if (0 == samplingrate % 44100){
					Times = DSD_Times - (samplingrate / 44100) + 1;
				}
				else{
					Times = DSD_Times - (samplingrate / 48000) + 1;
				}
#if 0	// for分の意味は？
				for (int i = 0; i < Times; i++){
					CString TimesStr;
					TimesStr.Format(L"%d", Times);
					pDlg->TrushFile(runfile_tmp, _T("_tmpL") + TimesStr);
					pDlg->TrushFile(runfile_tmp, _T("_tmpR") + TimesStr);
				}
#endif
				if(bDelFile == true){
//					TrushFile(runfile_tmp, _T(".wav"));
					// 暫定処理
					CString strDel(runfile_tmp);
//					strDel += _T(".wav");
					DeleteFile(strDel);
				}
			}
			// ID3v2タグ情報初期化
			pDlg->ID3v2TagInfoInit();
		}

		// ID3v2タグ情報初期化
//		pDlg->ID3v2TagInfoInit();

		// スリープ抑止解除
		::SetThreadExecutionState(ES_CONTINUOUS);

		if (nConvertCount != 0 || pDlg->m_nDsdClipOverCnt > 0) {
			// 完了通知処理
			pDlg->CompletionNotice();
		}

		// クリップオーバー?
		if (pDlg->m_nDsdClipOverCnt > 0) {
			pDlg->MessageBox(strClipOver, L"エラー", MB_OK);
		}

		pDlg->Enable();

		// 変換完了時の電源制御
		if (pDlg->m_nDsdClipOverCnt == 0) {
			pDlg->CompletePowerControl(pDlg->m_nCompletePowerCrl);
		}
//		if (i != 0){
		if (nConvertCount != 0){
			TCHAR errorMessage1[128];
//			wsprintf(errorMessage1, TEXT("%d"), i);
			wsprintf(errorMessage1, TEXT("%d"), nConvertCount);
			TCHAR *errorMessage2 = L"件の処理を実行しました";
			lstrcat(errorMessage1, errorMessage2);
			pDlg->MessageBox(errorMessage1, L"処理完了", MB_OK);
		}
	}

	pDlg->Enable();

//	delete[] metadata;
	return TRUE;
}

// 拡張子種別取得 
EXT_TYPE CPCMDSD_ConverterDlg::GetExtType(CString strExt)
{
	EXT_TYPE etRet;

	strExt = strExt.Trim();
	strExt = strExt.MakeUpper();

	etRet = EXT_TYPE_UNKNOW;

	if (strExt == _T("WAV")) {
		etRet = EXT_TYPE_WAV;
	}
	if (strExt == _T("W64")) {
		etRet = EXT_TYPE_WAVE64;
	}
	if (strExt == _T("FLAC")) {
		etRet = EXT_TYPE_FLAC;
	}
	if (strExt == _T("ALAC")) {
		etRet = EXT_TYPE_ALAC;
	}
	if (strExt == _T("DFF")) {
		etRet = EXT_TYPE_DFF;
	}

	return etRet;
}

// 出力ディレクトリ用アルバム＆アルバムアーティスト設定
void CPCMDSD_ConverterDlg::SetAlbumAndAlbumArtistOutputDir(CString strAlbum, CString strAlbumArtist, CString strDiscNo, CString strDiscTotal)
{
	CString strCmp;
	int nDiscNo = 0;
	int nDiscTotal = 0;
	
	strAlbum.TrimRight();
	strAlbumArtist.TrimRight();
	strDiscNo = strDiscNo.Trim();
	strDiscTotal = strDiscTotal.Trim();

	// 出力ディレクトリ用アルバムアーティスト
	m_strAlbumArtistOutDir = ConvertInvalidFileName(strAlbumArtist);

	// 出力ディレクトリ用アルバム＋サフィックス
	m_strAlbumOutDir = ConvertInvalidFileName(strAlbum);
	m_strAlbumOutDir += ConvertInvalidFileName(m_evAlbumTagSuffix);

	// 出力ディレクトリ用Disc NO
	m_strDiscNoOutDir = _T("");
	if (strDiscNo.GetLength() > 0 && strDiscTotal.GetLength() > 0) {
		// Disc NO数字のみ取り出してみる
		strCmp = strDiscNo.SpanIncluding(_T("0123456789"));
		if (strCmp == strDiscNo) {
			nDiscNo = _ttoi(strDiscNo);
		}

		// Disc Toal数字のみ取り出してみる
		strCmp = strDiscTotal.SpanIncluding(_T("0123456789"));
		if (strCmp == strDiscTotal) {
			nDiscTotal = _ttoi(strDiscTotal);
		}

		// Disc Totalが複数枚あり？
		if (nDiscTotal > 1) {
			// Disc NOが1以上定義あり？
			if (nDiscNo > 0) {
				m_strDiscNoOutDir = _T("Disc ");
				m_strDiscNoOutDir += strDiscNo.Trim();
			}
		}
	}
}

// ファイル、ディレクトリに使えない文字を全角変換
CString CPCMDSD_ConverterDlg::ConvertInvalidFileName(CString strText)
{
	CString strTmp;

	strTmp = strText;

	// ["*/:<>?|\]を全角変換
	strTmp.Replace(_T("\""), _T("”"));
	strTmp.Replace(_T("*"), _T("＊"));
	strTmp.Replace(_T("/"), _T("／"));
	strTmp.Replace(_T(":"), _T("："));
	strTmp.Replace(_T("<"), _T("＜"));
	strTmp.Replace(_T(">"), _T("＞"));
	strTmp.Replace(_T("?"), _T("？"));
	strTmp.Replace(_T("|"), _T("｜"));
	strTmp.Replace(_T("\\"), _T("￥"));

	return strTmp;
}

// 完了通知処理
void CPCMDSD_ConverterDlg::CompletionNotice()
{
	BOOL bEndPlay;

	if ((dwCompletionNoticeFlag & 0x00000001) != 0) {
		bEndPlay = PlaySound(_T("complete.wav"), NULL, SND_FILENAME | SND_ASYNC);
	}
	if ((dwCompletionNoticeFlag & 0x00000002) != 0) {
		WakeupDisplay();
	}
}

// 変換完了時の電源制御
// dwMode	0:何もしない
//			1:シャットダウン
//			2:スタンバイ
//			3:休止
//			4:ダイアログを閉じる
BOOL CPCMDSD_ConverterDlg::CompletePowerControl(DWORD dwMode)
{
	HANDLE				hToken;
	TOKEN_PRIVILEGES	TokenPri;

	switch (dwMode) {
		case 0:		// 何もしない
			return TRUE;
		case 1:		// シャットダウン
		case 2:		// スタンバイ
		case 3:		// 休止
			break;
		case 4:		// ダイアログを閉じる
			// INIファイル保存
			SaveIniFilee();
			// ダイアログ終了
			EndDialog(IDOK);
			return TRUE;
		default:
			return FALSE;
	}

	// INIファイル保存
	SaveIniFilee();

	// プロセストークンを取得
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) == FALSE){
		return FALSE;
	}
	// シャットダウン権限の LUID を取得
	if(LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &TokenPri.Privileges[0].Luid) == FALSE){
		return FALSE;
	}

	// シャットダウン権限を与える
	TokenPri.PrivilegeCount = 1;
	TokenPri.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &TokenPri, 0,  (PTOKEN_PRIVILEGES)NULL, 0);
	if(GetLastError() != ERROR_SUCCESS){
		return FALSE;
	}

	switch (dwMode) {
		case 1:		// シャットダウン
			ExitWindowsEx(EWX_POWEROFF, 0);
			break;
		case 2:		// スタンバイ
			SetSystemPowerState(TRUE, FALSE);
			break;
		case 3:		// 休止
			SetSystemPowerState(FALSE, FALSE);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

// フルパスからディレクトリパスだけ取得
CString CPCMDSD_ConverterDlg::GetDirectoryPath(TCHAR *pPath)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	CString strPath;

	_tsplitpath_s(pPath, drive, dir, filename, fileext);

	PathRemoveFileSpec(dir);
	strPath = drive;
	strPath += dir;

	return strPath;
}

int CPCMDSD_ConverterDlg::GetAlbumCount()
{
	int nRowCnt;
	int nRetCnt;
	int i;
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	CString strDir;
	CString strDirCmp;
	CString strPath;
	CString strPathCmp;

	nRowCnt = m_lFileList.GetItemCount();

	if (nRowCnt <= 1) {
		return nRowCnt;
	}

	nRetCnt = 0;

	i = 0;

	// 1件目のパス取得
	strPath = m_lFileList.GetItemText(i, EN_LIST_COLUMN_PATH);
	_tsplitpath_s(strPath, drive, dir, filename, fileext);
	strDir = drive;
	strDir += dir;
	nRetCnt ++;

	// 2件目以降のパスチェック
	for (i = 1; i < nRowCnt; i++) {
		// パス
		strPathCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_PATH);
		_tsplitpath_s(strPathCmp, drive, dir, filename, fileext);
		strDirCmp = drive;
		strDirCmp += dir;
		if (strDir != strDirCmp) {
			break;
		}

		nRetCnt ++;
	}

	return nRetCnt;
}

//PCM-DSD変換の管理
bool CPCMDSD_ConverterDlg::WAV_ConvertProc(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile, bool bLastDataFlag, int mode)
{
	bool bRet;

	// ソースファイル名取得
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	// 本当はWAV_ConvertProcの呼び出し元でEXT_TYPE_WAV、EXT_TYPE_WAVE64、EXT_TYPE_DFFで呼び出すべきだけど面倒なのでここでマスクする
	switch(etExtType){
		case EXT_TYPE_FLAC:
		case EXT_TYPE_ALAC:
			etExtType = EXT_TYPE_WAV;
			break;
		default:
			break;
	}

	_tsplitpath_s(orgfilepath, drive, dir, filename, fileext);

	m_SrcFileName = filename;
	m_SrcFileName += fileext;

	if (etExtType != EXT_TYPE_DFF) {
		if (mode == 0) {
			bRet = WAV_Convert(etExtType, orgfilepath, filepath, number, bDelWavFile);
		}
		else {
			bRet = WAV_ConvertSequential(etExtType, orgfilepath, filepath, number, bDelWavFile, bLastDataFlag);
		}
	} else {
		bRet = DFFtoDSFconvert(filepath);
	}

	return bRet;
}

bool CPCMDSD_ConverterDlg::WAV_Convert(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile)
{
	int iErrL = 0;
	int iErrR = 0;
	BOOL bDsfErr = FALSE;
	TCHAR strDsfFile[MAX_PATH];
	//	m_dProgress.Start(filepath);
//	m_dProgress.Process(0, 100);
	if (!m_dProgress.Cancelbottun) return false;

#if 1
	// アプリケーション名をソフトウェアタグに設定
	SetID3v2SoftwareTag(m_AppName, &m_stID3tag);
#endif
#if 1
	// エンコーダー者をタグに設定
//	SetID3v2EncodedByTag(m_strEncodedBy, &m_stID3tag);
	SetID3v2EncodedByTag(m_evEncoderPerson, &m_stID3tag);
#endif

	//精度取得
	int Precision = m_ccPrecision.GetCurSel();
	//DSDのサンプリングレートにするには何倍すればいいのか計算
	unsigned int DSD_Times;
#if 0
	switch (m_cSamplingRate.GetCurSel()) {
	case 0:
		DSD_Times = (unsigned int)pow(2, 6);
		break;
	case 1:
		DSD_Times = (unsigned int)pow(2, 7);
		break;
	case 2:
		DSD_Times = (unsigned int)pow(2, 8);
		break;
	case 3:
		DSD_Times = (unsigned int)pow(2, 9);
		break;
	case 4:
		DSD_Times = (unsigned int)pow(2, 10);
		break;
	case 5:
		DSD_Times = (unsigned int)pow(2, 11);
		break;
	}
#else
	int nSamplePerSec;
//	string *metadata = new string[7];
//	CString *metadata = new CString[EN_LIST_COLUMN_MAX];
//	WAV_Metadata(filepath, metadata, &nSamplePerSec);
//	delete[] metadata;
	if(etExtType == EXT_TYPE_WAVE64){
		// SONY WAVE64
		GeyWave64SamplePerSec(filepath, &nSamplePerSec);
	} else {
		// WAV
		GeyWavSamplePerSec(filepath, &nSamplePerSec);
	}

	switch(m_DSDsamplingRateCurSel){
		case 0:											// AUTO
			m_DSD_Times = GeDsdTimesWithtSamplePerSec(nSamplePerSec);
			break;
		case 1:											// DSD 64(2.8MHz)
			m_DSD_Times = 64;
//			DSD_Times = 6;
			break;
		case 2:											// DSD128(5.6MHz)
			m_DSD_Times = 128;
//			DSD_Times = 7;
			break;
		case 3:											// DSD256(11.2MHz)
			m_DSD_Times = 256;
//			DSD_Times = 8;
			break;
		case 4:											// DSD512(22.5MHz)
			m_DSD_Times = 512;
//			DSD_Times = 9;
			break;
		case 5:											// DSD1024(45.1MHz)
			m_DSD_Times = 1024;
//			DSD_Times = 10;
			break;
		case 6:											// DSD2048(90.3MHz)
			m_DSD_Times = 2048;
//			DSD_Times = 11;
			break;
		default:
			m_DSD_Times = 64;
//			DSD_Times = 6;
			break;
	}

	DSD_Times = m_DSD_Times;
	m_fillsize = 0;
#endif
	unsigned int DSD_TimesDes = 0;
	bool flag = true;
	bool flagl = true;
	bool flagr = true;
	FILE *tmpl;
	FILE *tmpr;
	//LR分離&64bit化
	if (!RequireWriteData(filepath, _T("_tmpL0"), L"wb", &tmpl)){
		flagl = false;
		flag = false;
	}
	if (!RequireWriteData(filepath, _T("_tmpR0"), L"wb", &tmpr)){
		flagr = false;
		flag = false;
	}
//	if (flag)if (!TmpWriteData(filepath, tmpl, tmpr, DSD_TimesDes)){
	if (flag)if (!TmpWriteData(etExtType, filepath, tmpl, tmpr, DSD_Times, &DSD_TimesDes, number)){
		if (m_dProgress.Cancelbottun == true) {
			MessageBox(_T("ディスクに空きがありません。"), _T("処理完了"), MB_OK);
		}
		flag = false;
	}
	if (flagl){
		fclose(tmpl);
	}
	if (flagr){
		fclose(tmpr);
	}
	omp_lock_t myLock;
	omp_init_lock(&myLock);
#pragma omp parallel
#pragma omp sections
	{
#pragma omp section
	{
		//Lch処理
		if (flag){
			bool flagUpl = true;
			bool flagOrigl = true;
			FILE *tmpl;
			FILE *UpsampleDataL;
			if (!RequireWriteData(filepath, _T("_tmpL0"), L"rb", &tmpl)){
				flagOrigl = false;
				flag = false;
			}
			if (!RequireWriteData(filepath, _T("_tmpLDSD"), L"wb", &UpsampleDataL)){
				flagUpl = false;
				flag = false;
			}
			if (Precision == 0){
				if (flag)if (!WAV_Filter(UpsampleDataL, tmpl, DSD_TimesDes, &myLock, &iErrL, 1)){
					flag = false;
				}
			}
			else{
				if (flag)if (!WAV_FilterLight(UpsampleDataL, tmpl, DSD_TimesDes, &iErrL)){
					flag = false;
				}
			}
			if (flagUpl){
				fclose(UpsampleDataL);
			}
			if (flagOrigl){
				fclose(tmpl);
			}
		}
	}
#pragma omp section  
	{
		//Rch処理
		if (flag){
			bool flagUpr = true;
			bool flagOrigr = true;
			FILE *tmpr;
			FILE *UpsampleDataR;
			if (!RequireWriteData(filepath, _T("_tmpR0"), L"rb", &tmpr)){
				flagOrigr = false;
				flag = false;
			}
			if (!RequireWriteData(filepath, _T("_tmpRDSD"), L"wb", &UpsampleDataR)){
				flagUpr = false;
				flag = false;
			}
			if (Precision == 0){
				if (flag)if (!WAV_Filter(UpsampleDataR, tmpr, DSD_TimesDes, &myLock, &iErrR, 2)){
					flag = false;
				}
			}
			else{
				if (flag)if (!WAV_FilterLight(UpsampleDataR, tmpr, DSD_TimesDes, &iErrR)){
					flag = false;
				}
			}
			if (flagUpr){
				fclose(UpsampleDataR);
			}
			if (flagOrigr){
				fclose(tmpr);
			}
		}
	}
	}

	if(iErrL != 0 || iErrR != 0){
		MessageBox(_T("ディスクに空きがありません。"), _T("処理完了"), MB_OK);
	}

	//LRマージ
	if (flag){
#ifdef ENABLE_OUTPUT_DFF	// DFF出力
		FILE *tmpDSD;
#endif
#ifdef ENABLE_OUTPUT_DSF	// DSF出力
		FILE *tmpDSF;
#endif
		CString DSDName;
		bool flagOrigl = true;
		bool flagOrigr = true;
		bool flagDSD = true;
		DSDName.Format(L"%d", (int)DSD_Times);
		if (!RequireWriteData(filepath, _T("_tmpLDSD"), L"rb", &tmpl)){
			TrushFile(filepath, _T("_tmpLDSD"));
			flagOrigl = false;
			flag = false;
		}
		if (!RequireWriteData(filepath, _T("_tmpRDSD"), L"rb", &tmpr)){
			TrushFile(filepath, _T("_tmpLDSD"));
			TrushFile(filepath, _T("_tmpRDSD"));
			flagOrigr = false;
			flag = false;
		}
#ifdef ENABLE_OUTPUT_DFF	// DFF出力
		if (!RequireWriteData(filepath, _T("_DSD") + DSDName + _T(".dff"), L"wb", &tmpDSD)){
		TrushFile(filepath, _T("_tmpLDSD"));
			TrushFile(filepath, _T("_tmpRDSD"));
			flagDSD = false;
			flag = false;
		}
		if (flag)if (!DSD_Write(tmpl, tmpr, tmpDSD, number)){
			TrushFile(filepath, _T("_tmpLDSD"));
			TrushFile(filepath, _T("_tmpRDSD"));
		}
#endif
#ifdef ENABLE_OUTPUT_DSF	// DSF出力
//		if (!RequireWriteData(filepath, _T("_DSD") + DSDName + _T(".dsf"), L"wb", &tmpDSF)){
//		if (!RequireWriteData(filepath, _T(".dsf"), L"wb", &tmpDSF)){	// ファイル名にサフィックスを付与しない
		if (!RequireWriteData(orgfilepath, _T(".dsf"), L"wb", &tmpDSF, strDsfFile, m_DsfOverWriteEnable)){	// ファイル名にサフィックスを付与しない
			TrushFile(filepath, _T("_tmpLDSD"));
			TrushFile(filepath, _T("_tmpRDSD"));
			flagDSD = false;
			flag = false;
		}
		if (flag)if (!DSD_WriteDSF(tmpl, tmpr, m_fillsize, tmpDSF, number)){
			TrushFile(filepath, _T("_tmpLDSD"));
			TrushFile(filepath, _T("_tmpRDSD"));
			flag = false;
			bDsfErr = TRUE;
		}
#endif
		if (flagOrigl){
			fclose(tmpl);
		}
		if (flagOrigr){
			fclose(tmpr);
		}
#ifdef ENABLE_OUTPUT_DFF	// DFF出力
		if (flagDSD){
			fclose(tmpDSD);
		}
#endif
#ifdef ENABLE_OUTPUT_DSF	// DSF出力
		if (flagDSD){
			fclose(tmpDSF);
		}
#endif
	}

	TrushFile(filepath, _T("_tmpL0"));
	TrushFile(filepath, _T("_tmpR0"));
	TrushFile(filepath, _T("_tmpLDSD"));
	TrushFile(filepath, _T("_tmpRDSD"));

	if(bDsfErr == TRUE){
		DeleteFile(strDsfFile);
		MessageBox(_T("ディスクに空きがありません。"), _T("処理完了"), MB_OK);
	}

	if(bDelWavFile == true){
//		TrushFile(filepath, _T(".wav"));
		// 暫定処理
		CString strDel(filepath);
//		strDel += _T(".wav");
		DeleteFile(strDel);
	}

	if (!flag){
		return false;
	}
	return true;
}

// PCM-DSD連続データ変換
bool CPCMDSD_ConverterDlg::WAV_ConvertSequential(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile, bool bLastDataFlag)
{
	int iErrL = 0;
	int iErrR = 0;
	BOOL bDsfErr = FALSE;
	TCHAR strDsfFile[MAX_PATH];

	//	if (!m_dProgress.Cancelbottun) return false;	// 多分このタイミングでキャンセルするとテンポラリファイルの残骸が残るからコメント。対応出来そうなら修正する。
#if 1
	// アプリケーション名をソフトウェアタグに設定
	SetID3v2SoftwareTag(m_AppName, &m_stID3tag);
#endif
#if 1
	// エンコーダー者をタグに設定
//	SetID3v2EncodedByTag(m_strEncodedBy, &m_stID3tag);
	SetID3v2EncodedByTag(m_evEncoderPerson, &m_stID3tag);
#endif

	//精度取得
	int Precision = m_ccPrecision.GetCurSel();
	//DSDのサンプリングレートにするには何倍すればいいのか計算
	unsigned int DSD_Times;

	int nSamplePerSec;
	BOOL bFirstData = FALSE;

	if(m_DSD_Times == 0){
		bFirstData = TRUE;
//		CString *metadata = new CString[EN_LIST_COLUMN_MAX];
//		WAV_Metadata(filepath, metadata, &nSamplePerSec);
//		delete[] metadata;
		if(etExtType == EXT_TYPE_WAVE64){
			// SONY WAVE64
			GeyWave64SamplePerSec(filepath, &nSamplePerSec);
		} else {
			// WAV
			GeyWavSamplePerSec(filepath, &nSamplePerSec);
		}

		switch(m_DSDsamplingRateCurSel){
			case 0:											// AUTO
				m_DSD_Times = GeDsdTimesWithtSamplePerSec(nSamplePerSec);
				break;
			case 1:											// DSD 64(2.8MHz)
				m_DSD_Times = 64;
				break;
			case 2:											// DSD128(5.6MHz)
				m_DSD_Times = 128;
				break;
			case 3:											// DSD256(11.2MHz)
				m_DSD_Times = 256;
				break;
			case 4:											// DSD512(22.5MHz)
				m_DSD_Times = 512;
				break;
			case 5:											// DSD1024(45.1MHz)
				m_DSD_Times = 1024;
				break;
			case 6:											// DSD2048(90.3MHz)
				m_DSD_Times = 2048;
			default:
				m_DSD_Times = 64;
				break;
		}
	}

	DSD_Times = m_DSD_Times;

	unsigned int DSD_TimesDes = 0;
	bool flag = true;
	bool flagl = true;
	bool flagr = true;
	FILE *tmpl;
	FILE *tmpr;

	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	_tsplitpath_s(filepath, drive, dir, filename, fileext);

	CString strTempFile;
	CString strTempFullPath;
	TCHAR *filepathTmp;

	// 連続変換用のファイル名を作成
	strTempFile = ::PathFindFileName(dir);
	// ルートフォルダだった場合の救済処置としてドライブ+固定名(_Drive_Root)でファイル名とする。
	if (strTempFile == _T("\\")) {
		strTempFile = drive[0];
		strTempFile += _T("_Drive_Root");
		strTempFile += _T("\\");
	}
	int pos = strTempFile.ReverseFind(_T('\\'));
	strTempFile.Delete(pos, 1);
	strTempFullPath = drive;
	strTempFullPath += dir;
	strTempFullPath += strTempFile;
	strTempFullPath += _T(".wav");

	m_strSeqModeTempFile = strTempFullPath;
	filepathTmp = strTempFullPath.GetBuffer();

	if (!m_dProgress.Cancelbottun){
		if(bDelWavFile == true){
			CString strDel(filepath);
			DeleteFile(strDel);
		}
		TrushFile(filepathTmp, _T("_tmpL0"));
		TrushFile(filepathTmp, _T("_tmpR0"));
		TrushFile(filepathTmp, _T("_tmpLDSD"));
		TrushFile(filepathTmp, _T("_tmpRDSD"));
		return false;
	}


	CString strOpenMode;

	if (bFirstData == TRUE) {
		strOpenMode = _T("wb");
	} else {
		strOpenMode = _T("ab");
	}

	ST_64BIT_DATA_INDEX stDataIndex;
	ST_64BIT_DATA_INDEX stDataIndexLasttime;

	stDataIndex.strOrgFile = orgfilepath;

	//LR分離&64bit化
	if (!RequireWriteData(filepathTmp, _T("_tmpL0"), strOpenMode.GetBuffer(), &tmpl)){
		flagl = false;
		flag = false;
	}
	if (!RequireWriteData(filepathTmp, _T("_tmpR0"), strOpenMode.GetBuffer(), &tmpr)){
		flagr = false;
		flag = false;
	}
	if (flag)if (!TmpWriteData(etExtType, filepath, tmpl, tmpr, DSD_Times, &DSD_TimesDes, number)){
		if (m_dProgress.Cancelbottun == true) {
			MessageBox(_T("ディスクに空きがありません。"), _T("処理完了"), MB_OK);
		}
		flag = false;
	}
	if (flagl){
		fclose(tmpl);
	}
	if (flagr){
		fclose(tmpr);
	}
	if (bFirstData == TRUE) {
		stDataIndex.iStartPos = m_fillsize;
	} else {
		stDataIndexLasttime = m_DataIdxArray.GetAt(m_DataIdxArray.GetCount() - 1);
		stDataIndex.iStartPos = stDataIndexLasttime.iStartPos + ((stDataIndexLasttime.iOrigDataSize / ((m_SrcPcmBit / 8) * 2)) * DSD_TimesDes);
	}
	stDataIndex.iOrigDataSize = m_OrigDataSize;

	stDataIndex.nTagMode = m_nTagMode;

	// タグをコピー
	stDataIndex.stID3tag = m_stID3tag;
	// 現在のタグ情報を破棄
	ID3v2TagInfoClear(&m_stID3tag, 1);

	m_DataIdxArray.Add(stDataIndex);

	if(bDelWavFile == true){
		CString strDel(filepath);
		DeleteFile(strDel);
	}

	// まだ続きのデータある？
	if(bLastDataFlag == false){
		// 64bit化したデータをファイルに結合だけして終了
		return true;
	}

	//*** DSD変換処理 ***
	m_dProgress.StartSeq(1, m_strSrcAlbumPath);
	omp_lock_t myLock;
	omp_init_lock(&myLock);
#pragma omp parallel
#pragma omp sections
	{
#pragma omp section
		{
			//Lch処理
			if (flag){
				bool flagUpl = true;
				bool flagOrigl = true;
				FILE *tmpl;
				FILE *UpsampleDataL;
				if (!RequireWriteData(filepathTmp, _T("_tmpL0"), L"rb", &tmpl)){
					flagOrigl = false;
					flag = false;
				}
				if (!RequireWriteData(filepathTmp, _T("_tmpLDSD"), L"wb", &UpsampleDataL)){
					flagUpl = false;
					flag = false;
				}
				if (Precision == 0){
					if (flag)if (!WAV_Filter(UpsampleDataL, tmpl, DSD_TimesDes, &myLock, &iErrL, 1)){
						flag = false;
					}
				}
				else{
					if (flag)if (!WAV_FilterLight(UpsampleDataL, tmpl, DSD_TimesDes, &iErrL)){
						flag = false;
					}
				}
				if (flagUpl){
					fclose(UpsampleDataL);
				}
				if (flagOrigl){
					fclose(tmpl);
				}
			}
		}
#pragma omp section  
		{
			//Rch処理
			if (flag){
				bool flagUpr = true;
				bool flagOrigr = true;
				FILE *tmpr;
				FILE *UpsampleDataR;
				if (!RequireWriteData(filepathTmp, _T("_tmpR0"), L"rb", &tmpr)){
					flagOrigr = false;
					flag = false;
				}
				if (!RequireWriteData(filepathTmp, _T("_tmpRDSD"), L"wb", &UpsampleDataR)){
					flagUpr = false;
					flag = false;
				}
				if (Precision == 0){
					if (flag)if (!WAV_Filter(UpsampleDataR, tmpr, DSD_TimesDes, &myLock, &iErrR, 2)){
						flag = false;
					}
				}
				else{
					if (flag)if (!WAV_FilterLight(UpsampleDataR, tmpr, DSD_TimesDes, &iErrR)){
						flag = false;
					}
				}
				if (flagUpr){
					fclose(UpsampleDataR);
				}
				if (flagOrigr){
					fclose(tmpr);
				}
			}
		}
	}

	if(iErrL != 0 || iErrR != 0){
		MessageBox(_T("ディスクに空きがありません。"), _T("処理完了"), MB_OK);
	}

	//*** DSF出力処理 ***
	//LRマージ
	if (flag) {
		FILE *tmpDSF;
		int i;
		int splitmax = (int)m_DataIdxArray.GetCount();
		TCHAR *pDstFilePath;
		CString strFolderName;
		CString strFileName;

		CString DSDName;
		bool flagOrigl = true;
		bool flagOrigr = true;
		bool flagDSD = true;
		DSDName.Format(L"%d", (int)DSD_Times);
		if (!RequireWriteData(filepathTmp, _T("_tmpLDSD"), L"rb", &tmpl)) {
			TrushFile(filepathTmp, _T("_tmpLDSD"));
			flagOrigl = false;
			flag = false;
		}
		if (!RequireWriteData(filepathTmp, _T("_tmpRDSD"), L"rb", &tmpr)) {
			TrushFile(filepathTmp, _T("_tmpLDSD"));
			TrushFile(filepathTmp, _T("_tmpRDSD"));
			flagOrigr = false;
			flag = false;
		}
		for(i = 0;i < splitmax;i ++){
			stDataIndex = m_DataIdxArray.GetAt(i);
			pDstFilePath = stDataIndex.strOrgFile.GetBuffer();
			m_OrigDataSize = stDataIndex.iOrigDataSize;
			m_nTagMode = stDataIndex.nTagMode;
			m_stID3tag = stDataIndex.stID3tag;
			strFolderName = GetLastPath(pDstFilePath);
			strFileName = GetFileName(pDstFilePath);
			TRACE(_T("No%d %d, %s\n"), i, m_OrigDataSize, pDstFilePath);
//			m_dProgress.StartSeq(0, pDstFilePath);
//			m_dProgress.StartSeq(1, _T(""), pDstFilePath, i, splitmax - 1);
			m_dProgress.StartSeq(0, strFolderName, strFileName, i, splitmax - 1);
			m_dProgress.Process(3, i, splitmax - 1);
			if (!RequireWriteData(pDstFilePath, _T(".dsf"), L"wb", &tmpDSF, strDsfFile, m_DsfOverWriteEnable)){	// ファイル名にサフィックスを付与しない
				TrushFile(filepathTmp, _T("_tmpLDSD"));
				TrushFile(filepathTmp, _T("_tmpRDSD"));
				flagDSD = false;
				flag = false;
			}
//			if (flag)if (!DSD_WriteDSF(tmpl, tmpr, stDataIndex.iStartPos * DSD_TimesDes, tmpDSF, number)){
			if (flag)if (!DSD_WriteDSF(tmpl, tmpr, stDataIndex.iStartPos, tmpDSF, number)){
				TrushFile(filepathTmp, _T("_tmpLDSD"));
				TrushFile(filepathTmp, _T("_tmpRDSD"));
				flag = false;
				bDsfErr = TRUE;
			}
			if (flagDSD) {
				fclose(tmpDSF);
			}
			// タグ情報をクリア
//			ID3v2TagInfoClear(&m_stID3tag);
		}
		if (flagOrigl){
			fclose(tmpl);
		}
		if (flagOrigr){
			fclose(tmpr);
		}
	}

	TrushFile(filepathTmp, _T("_tmpL0"));
	TrushFile(filepathTmp, _T("_tmpR0"));
	TrushFile(filepathTmp, _T("_tmpLDSD"));
	TrushFile(filepathTmp, _T("_tmpRDSD"));

	if(bDsfErr == TRUE){
		DeleteFile(strDsfFile);
		MessageBox(_T("ディスクに空きがありません。"), _T("処理完了"), MB_OK);
	}

	if (!flag){
		return false;
	}
	return true;
}

// DFF-DSF変換
bool CPCMDSD_ConverterDlg::DFFtoDSFconvert(TCHAR *filepath)
{
	TCHAR WriteFilepath[MAX_PATH];
	BOOL bConvertRet;
	FILE *fp;
	errno_t error;
	unsigned __int64 binary = 0;
	bool bRet = true;

	if (!m_dProgress.Cancelbottun) return false;

	if (!RequireWriteData(filepath, _T(".dsf"), WriteFilepath)){
		return false;
	}

#if 1
	// アプリケーション名をソフトウェアタグに設定
	SetID3v2SoftwareTag(m_AppName, &m_stID3tag);
#endif
#if 1
	// エンコーダー者をタグに設定
//	SetID3v2EncodedByTag(m_strEncodedBy, &m_stID3tag);
	SetID3v2EncodedByTag(m_evEncoderPerson, &m_stID3tag);
#endif
#if 1
	STDFFHEAD stDffHead;

	m_DffToDsfObj.GetHead(filepath, &stDffHead);
	// ID3v2タグユーザー定義にオリジナルフォーマット設定
	SetID3v2UserTagOriginalFormat(1, m_SrcFileName, stDffHead.stSampChunk.sampleRate, 1, &m_stID3tag);
#endif

	// DSD LSBデータで変換
	m_DffToDsfObj.SetBitsPerSample(1);

	// DFFファイルをDSFファイルに変換
	bConvertRet = m_DffToDsfObj.Convert(filepath, WriteFilepath, DFFtoDSFconvertProgress, this);
	if (bConvertRet == FALSE) {
		CString strDel(WriteFilepath);
		DeleteFile(strDel);
		return false;
	}

#if 1	// DEBUGの為無効にする
	// Tagあり?
	if(m_nTagMode == 1 && bRet == TRUE){
		error = _tfopen_s(&fp, WriteFilepath, _T("rb+"));
		if(error != 0) {
			return false;
		}
		bRet = DSFID3V2Write(fp);

		fclose(fp);
	}
#endif

	return  bRet;
}

// DFF-DSF変換進捗コールバック関数
// nProgress:0～100%
BOOL CPCMDSD_ConverterDlg::DFFtoDSFconvertProgress(UINT nProgress, void *pPrm)
{
	CPCMDSD_ConverterDlg *pDlg;

	pDlg = (CPCMDSD_ConverterDlg*)pPrm;

//	TRACE("DFF to DSF変換:%d%%\n", nProgress);
	pDlg->m_dProgress.Process(4, nProgress, 100);

	if (!pDlg->m_dProgress.Cancelbottun){
		return FALSE;
	}

	return TRUE;
}

// FLACデコード
static FLAC__uint64 total_samples = 0;
static unsigned sample_rate = 0;
static unsigned channels = 0;
static unsigned bps = 0;

static bool write_little_endian_uint16(FILE *f, FLAC__uint16 x);
static bool write_little_endian_int16(FILE *f, FLAC__int16 x);
static bool write_little_endian_uint24(FILE *f, FLAC__uint32 x);
static bool write_little_endian_uint32(FILE *f, FLAC__uint32 x);

bool CPCMDSD_ConverterDlg::FlacDecode(TCHAR *psrcfile, TCHAR *pdecodefile, int decodefilebuffsize)
{
	char *filename_utf8 = convert_to_utf8(psrcfile);

	FILE *fout;
	FLAC__bool ok = true;
	FLAC__StreamDecoder *decoder = 0;
	FLAC__StreamDecoderInitStatus init_status;
	CString strUid;
	SYSTEMTIME stTime;
	bool bRet = true;

	GetSystemTime(&stTime);

	strUid.Format(_T("%04hu%02hu%02hu%02hu%02hu%02hu%03hu.wav"),
					stTime.wYear,
					stTime.wMonth,
					stTime.wDay,
					stTime.wHour + 9,
					stTime.wMinute,
					stTime.wSecond,
					stTime.wMilliseconds);

	if (!RequireWriteData(psrcfile, strUid, L"wb", &fout, pdecodefile)){
		return false;
	}

	// 変換ファイル名
	if ((decoder = FLAC__stream_decoder_new()) == NULL) {
		fclose(fout);
		return false;
	}

	(void)FLAC__stream_decoder_set_md5_checking(decoder, true);

	init_status = FLAC__stream_decoder_init_file(decoder, filename_utf8, write_callback, metadata_callback, error_callback, /*client_data=*/fout);
	if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		ok = false;
		bRet = false;
	}

	if (ok) {
		ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
		if (ok == false) {
			bRet = false;
		}
		fprintf(stderr, "decoding: %s\n", ok ? "succeeded" : "FAILED");
//		fprintf(stderr, "   state: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
		fprintf(stderr, "   state: %s\n", FLAC__stream_decoder_get_resolved_state_string(decoder));
		
	}

	delete filename_utf8;
	FLAC__stream_decoder_delete(decoder);
	fclose(fout);

	return bRet;
}

FLAC__StreamDecoderWriteStatus CPCMDSD_ConverterDlg::write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	FILE *f = (FILE*)client_data;
	const FLAC__uint32 total_size = (FLAC__uint32)(total_samples * channels * (bps/8));
	size_t i;

	(void)decoder;

	if(total_samples == 0) {
		fprintf(stderr, "ERROR: this example only works for FLAC files that have a total_samples count in STREAMINFO\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(channels != 2) {
		fprintf(stderr, "ERROR: this example only supports stereo streams\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(bps != 16 && bps != 24 && bps != 32) {
		fprintf(stderr, "ERROR: this example only supports 16/24/32bit stereo streams\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(frame->header.channels != 2) {
		fprintf(stderr, "ERROR: This frame contains %d channels (should be 2)\n", frame->header.channels);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(buffer [0] == NULL) {
		fprintf(stderr, "ERROR: buffer [0] is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(buffer [1] == NULL) {
		fprintf(stderr, "ERROR: buffer [1] is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	/* write WAVE header before we write the first frame */
	if(frame->header.number.sample_number == 0) {
		if(
			fwrite("RIFF", 1, 4, f) < 4 ||
			!write_little_endian_uint32(f, total_size + 36) ||
			fwrite("WAVEfmt ", 1, 8, f) < 8 ||
			!write_little_endian_uint32(f, 16) ||
			!write_little_endian_uint16(f, 1) ||
			!write_little_endian_uint16(f, (FLAC__uint16)channels) ||
			!write_little_endian_uint32(f, sample_rate) ||
			!write_little_endian_uint32(f, sample_rate * channels * (bps/8)) ||
			!write_little_endian_uint16(f, (FLAC__uint16)(channels * (bps/8))) || /* block align */
			!write_little_endian_uint16(f, (FLAC__uint16)bps) ||
			fwrite("data", 1, 4, f) < 4 ||
			!write_little_endian_uint32(f, total_size)
		) {
			fprintf(stderr, "ERROR: write error\n");
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}

	/* write decoded PCM samples */
	for(i = 0; i < frame->header.blocksize; i++) {
		if(bps == 16){
			if(
				!write_little_endian_int16(f, (FLAC__int16)buffer[0][i]) ||  /* left channel */
				!write_little_endian_int16(f, (FLAC__int16)buffer[1][i])     /* right channel */
			) {
				fprintf(stderr, "ERROR: write error\n");
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
			}
		} else if(bps == 24){
			if(
				!write_little_endian_uint24(f, (FLAC__int32)buffer[0][i]) ||  /* left channel */
				!write_little_endian_uint24(f, (FLAC__int32)buffer[1][i])     /* right channel */
			) {
				fprintf(stderr, "ERROR: write error\n");
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
			}
		} else {
			if(
				!write_little_endian_uint32(f, (FLAC__int32)buffer[0][i]) ||  /* left channel */
				!write_little_endian_uint32(f, (FLAC__int32)buffer[1][i])     /* right channel */
			) {
				fprintf(stderr, "ERROR: write error\n");
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
			}
		}
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void CPCMDSD_ConverterDlg::metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)decoder, (void)client_data;

	/* print some stats */
	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		/* save for later */
		total_samples = metadata->data.stream_info.total_samples;
		sample_rate = metadata->data.stream_info.sample_rate;
		channels = metadata->data.stream_info.channels;
		bps = metadata->data.stream_info.bits_per_sample;

		TRACE("sample rate    : %u Hz\n", sample_rate);
		TRACE("channels       : %u\n", channels);
		TRACE("bits per sample: %u\n", bps);
//		TRACE("total samples  : %" PRIu64 "\n", total_samples);
		TRACE("total samples  : %llu\n", total_samples);
	}
}

void CPCMDSD_ConverterDlg::error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder, (void)client_data;

//	TRACE("Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

// ALACデコード
bool CPCMDSD_ConverterDlg::AlacDecode(TCHAR *psrcfile, TCHAR *pdecodefile)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	CString strFile;
	CString strUid;
	SYSTEMTIME stTime;
	BOOL bRet;

	GetSystemTime(&stTime);

	strUid.Format(_T("%04hu%02hu%02hu%02hu%02hu%02hu%03hu.wav"),
					stTime.wYear,
					stTime.wMonth,
					stTime.wDay,
					stTime.wHour + 9,
					stTime.wMinute,
					stTime.wSecond,
					stTime.wMilliseconds);

	_tsplitpath_s(psrcfile, drive, dir, filename, fileext);

	strFile = drive;
	strFile += dir;
	strFile += filename;
	strFile += strUid;

	int strLen;
	strLen = strFile.GetLength();
	if (strLen >= MAX_PATH) {
		return false;
	}
	_tcscpy_s(pdecodefile, strFile.GetLength()*sizeof(TCHAR), strFile);

	// ALACオープン
	bRet = m_AlacDecodeObj.Open(psrcfile);
	if (bRet == FALSE) {
		return false;
	}
	// WAVにデコード
	m_AlacDecodeObj.WaveExport(strFile);
	// クローズ
	m_AlacDecodeObj.Close();

	return true;
}

//Wav/FLAC/ALAC/DFFファイルがドロップされた時の初動
void CPCMDSD_ConverterDlg::WAV_FileRead(TCHAR *FileName, BOOL bErrMsgEnable)
{
	CString *metadata = new CString[EN_LIST_COLUMN_MAX];
	bool flag = true;
	bool metaWav = false;
	bool metaWave64 = false;
	bool metaFlac = false;
	bool metaAlac = false;
	bool metaDff = false;

	// WAVファイル解析
	metaWav = WAV_Metadata(FileName, metadata);
	if(metaWav == false){
		//Sony Wave64(W64)ファイル解析
		metaWave64 = Wave64_Metadata(FileName, metadata);
		if(metaWave64 == false){
			// FLACファイル解析
			metaFlac = FLAC_Metadata(FileName, metadata);
			if(metaFlac == false){
				// ALACファイル解析
				metaAlac = ALAC_Metadata(FileName, metadata);
				if(metaAlac == false){
					// DFFファイル解析
					metaDff = DFF_Metadata(FileName, metadata);
				}
			}
		}
	}

	if (metaWav == true || metaWave64 == true || metaFlac == true || metaAlac == true || metaDff == true) {
		int row_now = m_lFileList.GetItemCount();
		for (int i = 0; i < row_now; i++){
			// 重複パスは登録しない
			if (metadata[EN_LIST_COLUMN_PATH] == m_lFileList.GetItemText(i, EN_LIST_COLUMN_PATH)){
				flag = false;
			}
		}
		if (flag){
			m_lFileList.InsertItem(row_now, FileName);
			for (int n = 0; n < EN_LIST_COLUMN_MAX; n++){
				m_lFileList.SetItemText(row_now, n, metadata[n]);
			}
		}
	}
	else {
		CString strErrFile;
		CString strErrMsg;
		strErrFile = FileName;
		strErrMsg = "WAV_FileRead() 未対応ファイル:";
		strErrMsg += strErrFile;
		strErrMsg += "\n";
		TRACE(strErrMsg);
	}
	delete[] metadata;
}

//ディレクトリの再帰的検索
void CPCMDSD_ConverterDlg::DirectoryFind(TCHAR *DirectoryPath){
	CFileFind find;
	CString FilePathTemp;
	CString strExt;
	CString dirPath = DirectoryPath;
	dirPath += _T("\\*"); // ファイルの条件
	if (find.FindFile(dirPath)){
		BOOL flag;
		do{
			flag = find.FindNextFile();
			FilePathTemp = find.GetFilePath();
			// *.DSDIFFファイルが存在したときの処理
			if (!find.IsDots()){
				if (find.IsDirectory()){
					DirectoryFind(FilePathTemp.GetBuffer());
				}
				else{
					CString Filepath = find.GetFilePath();
					TCHAR fileext[_MAX_EXT];
					_tsplitpath_s(Filepath, NULL, 0, NULL, 0, NULL, 0, fileext, sizeof(fileext) / sizeof(fileext[0]));
					strExt = fileext;
					strExt = strExt.MakeUpper();
					if ((strExt == _T(".WAV")) || (strExt == _T(".W64")) || (strExt == _T(".FLAC")) || (strExt == _T(".M4A")) || (strExt == _T(".DFF"))) {
						WAV_FileRead(FilePathTemp.GetBuffer());
					}
				}
			}
		} while (flag); // これなら対象ファイル全てが処理出来る
	}
}

//ファイル/ディレクトリがドラッグ&ドロップ
void CPCMDSD_ConverterDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
#if FALSE	// DragQueryFileにバッファサイズを渡しているのでバッファーオーバーランしないはずだからどっちのロジック使っても問題ない気がする
	TCHAR FileName[ABSOLUTE_PATH_MAX];
	int NameSize = sizeof(FileName);
	int FileNumber;
	CString str;
	int i;

//	FileNumber = DragQueryFile(hDropInfo, 0xffffffff, FileName, NameSize);
	FileNumber = DragQueryFile(hDropInfo, 0xffffffff, NULL, 0);
	for (i = 0; i < FileNumber; i++){
		DragQueryFile(hDropInfo, i, FileName, NameSize);
		if (PathIsDirectory(FileName)){
			DirectoryFind(FileName);
		}
		else{
			WAV_FileRead(FileName);
		}
	}
	CDialogEx::OnDropFiles(hDropInfo);
#else
	TCHAR* ptcFileName;
	CString strInputPath;
	int FileNumber;
	int nLength;
	int i;

	// ファイル数取得
	FileNumber = DragQueryFile(hDropInfo, 0xffffffff, NULL, 0);
	for (i = 0; i < FileNumber; i++) {
		// パスの長さ取得 ※NULL終端除く
		nLength = DragQueryFile(hDropInfo, i, NULL, 0);
		nLength += 1;
		ptcFileName = new TCHAR[nLength];
		ZeroMemory(ptcFileName, nLength);
		// パス取得
		DragQueryFile(hDropInfo, i, ptcFileName, nLength);
		if (PathIsDirectory(ptcFileName)) {
			DirectoryFind(ptcFileName);
		} else {
			WAV_FileRead(ptcFileName);
		}
		delete[] ptcFileName;
	}
#endif
	// リストに追加があるなら全て実行ボタンをフォーカスする
	if (m_lFileList.GetItemCount() > 0) {
		m_bAllRun.SetFocus();
	}
}

//処理中はボタンなど無効に
void CPCMDSD_ConverterDlg::Disable(){
	CMenu* pMenu = GetSystemMenu(FALSE);
	pMenu->EnableMenuItem(SC_CLOSE, MF_GRAYED);

	m_dProgress.EnableWindow(TRUE);
//	m_dProgress.ShowWindow(TRUE);
	m_dProgress.ShowWindow(SW_SHOWNORMAL);

	m_lFileList.EnableWindow(FALSE);
	m_btnAlbumRun.EnableWindow(FALSE);
	m_bAllRun.EnableWindow(FALSE);
	m_bAllListDelete.EnableWindow(FALSE);
	m_bRun.EnableWindow(FALSE);
	m_bListDelete.EnableWindow(FALSE);
	m_bcPath.EnableWindow(FALSE);
	m_bcPathClear.EnableWindow(FALSE);

	m_cSamplingRate.EnableWindow(FALSE);
	m_ccPrecision.EnableWindow(FALSE);

	m_combVol.EnableWindow(FALSE);

	m_chkboxFileOverWrite.EnableWindow(FALSE);
	m_chkboxNormalize.EnableWindow(FALSE);
	m_editAlbumTagSuffix.EnableWindow(FALSE);

	m_editEncoderPerson.EnableWindow(FALSE);

	m_ecPath.EnableWindow(FALSE);

	if(m_DsfOverWriteEnable == FALSE){
		m_combCompleteOption.EnableWindow(FALSE);
	}

	DragAcceptFiles(FALSE);
}

//処理が終わったらボタンなど有効化
void CPCMDSD_ConverterDlg::Enable(){
	CMenu* pMenu = GetSystemMenu(FALSE);
	pMenu->EnableMenuItem(SC_CLOSE, MF_DEFAULT);

	m_dProgress.EnableWindow(FALSE);
//	m_dProgress.ShowWindow(FALSE);
	m_dProgress.ShowWindow(SW_HIDE);
	m_dProgress.Cancelbottun = true;

	m_lFileList.EnableWindow(TRUE);
	m_btnAlbumRun.EnableWindow(TRUE);
	m_bAllRun.EnableWindow(TRUE);
	m_bAllListDelete.EnableWindow(TRUE);
	m_bRun.EnableWindow(TRUE);
	m_bListDelete.EnableWindow(TRUE);
	m_bcPath.EnableWindow(TRUE);
	m_bcPathClear.EnableWindow(TRUE);

	m_cSamplingRate.EnableWindow(TRUE);
	m_ccPrecision.EnableWindow(TRUE);

	m_combVol.EnableWindow(TRUE);

	m_chkboxFileOverWrite.EnableWindow(TRUE);
	m_chkboxNormalize.EnableWindow(TRUE);
	m_editAlbumTagSuffix.EnableWindow(TRUE);

	m_editEncoderPerson.EnableWindow(TRUE);

	m_ecPath.EnableWindow(TRUE);

	m_combCompleteOption.EnableWindow(TRUE);

	DragAcceptFiles(TRUE);
}

//アルバムモード実行
void CPCMDSD_ConverterDlg::OnBnClickedAlbumrun()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	UpdateData();

	CString strMsg;
	int nErrRow;
	BOOL bRet;
	int nRowCnt;
	int i;

	m_dwConvertProcessFlag = 1;
	if (m_lFileList.GetItemCount() != 0){

		bRet = ChkListAlbumSeq(0,&nErrRow);
		if (bRet == TRUE) {
			bRet = ChkListAlbumMode(&nErrRow);
			if (bRet == TRUE) {
				AfxBeginThread(WorkThread, this);
			}
			else {
				m_lFileList.SetFocus();
				// 選択解除
				nRowCnt = m_lFileList.GetItemCount();
				for (i = 0; i < nRowCnt; i++) {
					m_lFileList.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
				}
				// エラー行を選択
				m_lFileList.SetItemState(nErrRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				strMsg = _T("異なるフォーマットが登録されています。\n[アルバム実行]は、フォルダ内のフォーマットが同じ形式（拡張子、サンプリングレート、ビット）、\n同じタグ（アルバム、ディスクNo、アルバムアーティスト）である必要があります。");
				MessageBox(strMsg, L"処理完了", MB_OK);
			}
		} else {
			m_lFileList.SetFocus();
			// 選択解除
			nRowCnt = m_lFileList.GetItemCount();
			for (i = 0; i < nRowCnt; i++) {
				m_lFileList.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
			}
			// エラー行を選択
			m_lFileList.SetItemState(nErrRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			strMsg = _T("フォルダが連続していません。。\n[アルバム実行]は、連続したフォルダである必要があります。");
			MessageBox(strMsg, L"処理完了", MB_OK);
		}
	}
}

BOOL CPCMDSD_ConverterDlg::ChkListAlbumMode(int *pnErrRow)
{
	int nRowCnt;
	int i;
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	CString strDir;
	CString strDirCmp;
	CString strPath;
	CString strPathCmp;
	CString strExt;
	CString strExtCmp;
	CString strSampling;
	CString strSamplingCmp;
	CString strBit;
	CString strBitCmp;
	CString strAlbum;
	CString strAlbumCmp;
	CString strDiscNo;
	CString strDiscNoCmp;
	CString strAlbumArtist;
	CString strAlbumArtistCmp;

	EXT_TYPE etExtType;
	BOOL bTagEnable = FALSE;

	nRowCnt = m_lFileList.GetItemCount();
	*pnErrRow = 0;

	if (nRowCnt <= 1) {
		return TRUE;
	}

	for (i = 0; i < nRowCnt; i++) {
		*pnErrRow = i;
		// パス
		strPathCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_PATH);
		_tsplitpath_s(strPathCmp, drive, dir, filename, fileext);
		strDirCmp = drive;
		strDirCmp += dir;
		if (strDir != strDirCmp) {
			// パスが異なるならアルバムが変わったのでチェックしない
			// パス
			strPath = m_lFileList.GetItemText(i, EN_LIST_COLUMN_PATH);
			_tsplitpath_s(strPath, drive, dir, filename, fileext);
			strDir = drive;
			strDir += dir;
			// 拡張子
			strExt = m_lFileList.GetItemText(i, EN_LIST_COLUMN_EXT);
			etExtType = GetExtType(strExt);
			// サンプリングレート
			strSampling = m_lFileList.GetItemText(i, EN_LIST_COLUMN_SAMPLING_RATE);
			// ビット
			strBit = m_lFileList.GetItemText(i, EN_LIST_COLUMN_BIT);
			// アルバム
			strAlbum = m_lFileList.GetItemText(i, EN_LIST_COLUMN_ALBUM);
			// Disc No
			strDiscNo = m_lFileList.GetItemText(i, EN_LIST_COLUMN_DISC_NO);
			// アルバムアーティスト
			strAlbumArtist = m_lFileList.GetItemText(i, EN_LIST_COLUMN_ALBUM_ARTIST);

			continue;
		}

		// 拡張子
		strExtCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_EXT);
		etExtType = GetExtType(strExtCmp);
		if (strExt != strExtCmp) {
			return FALSE;
		}

		// サンプリングレート
		strSamplingCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_SAMPLING_RATE);
		if (strSampling != strSamplingCmp) {
			return FALSE;
		}

		// ビット
		strBitCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_BIT);
		if (strBit != strBitCmp) {
			return FALSE;
		}

		// アルバム
		strAlbumCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_ALBUM);
		if (strAlbum != strAlbumCmp) {
			return FALSE;
		}

		// Disc No
		strDiscNoCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_DISC_NO);
		if (strDiscNo != strDiscNoCmp) {
			return FALSE;
		}

		// アルバムアーティスト
		strAlbumArtistCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_ALBUM_ARTIST);
		if (strAlbumArtist != strAlbumArtistCmp) {
			return FALSE;
		}
	}

	return TRUE;
}

// アルバム実行連続フォルダチェック ※再起関数なので注意
BOOL CPCMDSD_ConverterDlg::ChkListAlbumSeq(int nStartPos, int *pnErrRow)
{
	int nRowCnt;
	int i;
	int i_diff;
	int nStartPosNext;
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	CString strDir;
	CString strDirCmp;
	CString strPath;
	CString strPathCmp;
	BOOL bRet;

	nRowCnt = m_lFileList.GetItemCount();

	if (nRowCnt <= 1) {
		return TRUE;
	}

	if (nStartPos == nRowCnt - 1) {
		return TRUE;
	}

	i = nStartPos;
	i_diff = nStartPos;
	nStartPosNext = nStartPos;
	*pnErrRow = nStartPos;
	bRet = TRUE;

	// 1件目のパス取得
	strPath = m_lFileList.GetItemText(i, EN_LIST_COLUMN_PATH);
	_tsplitpath_s(strPath, drive, dir, filename, fileext);
	strDir = drive;
	strDir += dir;

	// 2件目以降のパスチェック
	for (i = nStartPos + 1; i < nRowCnt; i++) {
		// パス
		strPathCmp = m_lFileList.GetItemText(i, EN_LIST_COLUMN_PATH);
		_tsplitpath_s(strPathCmp, drive, dir, filename, fileext);
		strDirCmp = drive;
		strDirCmp += dir;
		if (strDir == strDirCmp) {
			if((i - i_diff) > 1){
				*pnErrRow = i;
				return FALSE;
			}
			i_diff = i;
		} else {
			if (nStartPosNext == nStartPos) {
				nStartPosNext = i;
			}
		}
	}

	if (nStartPosNext != nStartPos) {
		bRet = ChkListAlbumSeq(nStartPosNext, pnErrRow);
	}

	return bRet;
}

//全てを実行
void CPCMDSD_ConverterDlg::OnBnClickedAllrun()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	UpdateData();

//	flag_Bottun = "All";
	m_dwConvertProcessFlag = 0;
	if (m_lFileList.GetItemCount() != 0){
		AfxBeginThread(WorkThread, this);
	}
}

//実行
void CPCMDSD_ConverterDlg::OnBnClickedRun()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	UpdateData();

//	flag_Bottun == "One";
	m_dwConvertProcessFlag = 2;
	if (m_lFileList.GetFirstSelectedItemPosition() != 0){
		AfxBeginThread(WorkThread, this);
	}
}

//全てを削除
void CPCMDSD_ConverterDlg::OnBnClickedAlllistdelete()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_lFileList.DeleteAllItems();

	// リストコントロール登録件数表示
	ListRegistDisp();
}

//削除
void CPCMDSD_ConverterDlg::OnBnClickedListdelete()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	// 選択行を削除
	int iRow = m_lFileList.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
	while (iRow != -1){
		m_lFileList.DeleteItem(iRow);
		iRow = m_lFileList.GetNextItem(iRow - 1, LVNI_ALL | LVNI_SELECTED);
	}

	// リストコントロール登録件数表示
	ListRegistDisp();
}

//参照,ここから丸パクリ http://www.jade.dti.ne.jp/~arino/sample6.htm
void CPCMDSD_ConverterDlg::OnBnClickedPathcheck()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	BROWSEINFO bInfo;
	LPITEMIDLIST pIDList;
	TCHAR szDisplayName[MAX_PATH];

	// BROWSEINFO構造体に値を設定
	bInfo.hwndOwner = AfxGetMainWnd()->m_hWnd;		// ダイアログの親ウインドウのハンドル
	bInfo.pidlRoot = NULL;							// ルートフォルダを示すITEMIDLISTのポインタ (NULLの場合デスクトップフォルダが使われます）
	bInfo.pszDisplayName = szDisplayName;			// 選択されたフォルダ名を受け取るバッファのポインタ
	bInfo.lpszTitle = _T("フォルダの選択");			// ツリービューの上部に表示される文字列 
//	bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;			// 表示されるフォルダの種類を示すフラグ
	bInfo.ulFlags = BIF_RETURNONLYFSDIRS;			// 表示されるフォルダの種類を示すフラグ
//	bInfo.lpfn = NULL;								// BrowseCallbackProc関数のポインタ
//	bInfo.lParam = (LPARAM)0;						// コールバック関数に渡す値
	bInfo.lpfn = BrowseCallbackProc;				// BrowseCallbackProc関数のポインタ
	bInfo.lParam = (LPARAM)m_evPath.GetBuffer();	// コールバック関数に渡す値 ※初期表示パス

	// フォルダ選択ダイアログを表示
	pIDList = ::SHBrowseForFolder(&bInfo);
	// フォルダが選択あり？
	if (pIDList != NULL){
		// ItemIDListをパス名に変換します
		if (::SHGetPathFromIDList(pIDList, szDisplayName)){
			m_evPath = szDisplayName;
		}
		// 最後にpIDListのポイントしているメモリを開放します
		::CoTaskMemFree(pIDList);
		UpdateData(FALSE);
	}
}

// コールバック関数
static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	// フォルダ選択ダイアログの初期化
	if ((BFFM_INITIALIZED == uMsg) && lpData)
	{
		// 初期表示パスを設定
		SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}

 //閉じる動作をオーバーライド
void CPCMDSD_ConverterDlg::OnCancel(){
	m_dProgress.DestroyWindow();
	CDialogEx::OnCancel();
}

//F1ヘルプ無効化
BOOL CPCMDSD_ConverterDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	return true;
}

static bool write_little_endian_uint16(FILE *f, FLAC__uint16 x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF
		;
}

static bool write_little_endian_int16(FILE *f, FLAC__int16 x)
{
	return write_little_endian_uint16(f, (FLAC__uint16)x);
}

static bool write_little_endian_uint24(FILE *f, FLAC__uint32 x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF &&
		fputc(x >> 16, f) != EOF
		;
}

static bool write_little_endian_uint32(FILE *f, FLAC__uint32 x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF &&
		fputc(x >> 16, f) != EOF &&
		fputc(x >> 24, f) != EOF
		;
}

bool CPCMDSD_ConverterDlg::GetTargetSample(unsigned int *pnDstSamplePerSec, unsigned int *pnDstBaseSamplePerSec, unsigned int nSrcSamplePerSec, int SearchMode)
{
	int i;
	DWORD dwRatio;
	DWORD dwSampleCompare;
	bool bResult = true;
	DWORD dwSearchSampling;

	// 768KHz以上はエラー
	if (nSrcSamplePerSec > (48000 * 16)) {
		return false;
	}
	// 44.1KHz未満はエラー
	if (nSrcSamplePerSec < 44100) {
		return false;
	}

	if(SearchMode == 1 && (nSrcSamplePerSec % 48000) == 0){
		// 48000の倍数で検索
		dwSearchSampling = 48000;
	} else {
		// 44100の倍数で検索
		dwSearchSampling = 44100;
	}

	*pnDstBaseSamplePerSec = dwSearchSampling;
	*pnDstSamplePerSec = nSrcSamplePerSec;

	// 44.1/48KHzの倍数ならアップサンプリングの周波数の検索不要
	if (nSrcSamplePerSec % dwSearchSampling == 0) {
		return bResult;
	}

	// ターゲットリサンプリング検索
	dwRatio = 1;
	for (i = 0; i < 6; i++) {
		dwSampleCompare = dwSearchSampling * dwRatio;
		if (dwSampleCompare >= nSrcSamplePerSec) {
			break;
		}
		dwRatio *= 2;
	}

	// 目標サンプリング設定	
	*pnDstSamplePerSec = dwSampleCompare;

	return bResult;
}

int CPCMDSD_ConverterDlg::GeDsdTimesWithtSamplePerSec(unsigned int nSrcSamplePerSec)
{
	int nRet;

	switch(nSrcSamplePerSec){
		case 44100:
		case 48000:
			nRet = m_nDsdSampling44100;
			break;
		case 88200:
		case 96000:
			nRet = m_nDsdSampling88200;
			break;
		case 176400:
		case 192000:
			nRet = m_nDsdSampling176400;
			break;
		case 352800:
		case 384000:
			nRet = m_nDsdSampling352800;
			break;
		case 705600:
		case 768000:
			nRet = m_nDsdSampling705600;
			break;
		default:
			nRet = 64;
			break;
	}

	return nRet;
}

void CPCMDSD_ConverterDlg::WakeupDisplay()
{
#if 1	// SC_MONITORPOWERではモニタがONになってもすぐOFFになるのでマウス移動イベントで行う
	INPUT stInput;
	UINT count;

	memset(&stInput, NULL, sizeof(stInput));

	// マウス移動イベント
	stInput.type = INPUT_MOUSE;
	stInput.mi.dx = 0;
	stInput.mi.dy = 0;
	stInput.mi.dwFlags = MOUSEEVENTF_MOVE;

	count = SendInput(1, &stInput, sizeof(INPUT));
	TRACE("SendInput:%d\n", count);
#else
	::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, DISPLAY_ON);
#endif
}

void CPCMDSD_ConverterDlg::LoadSafeWindowPos(RECT *prcWnd, INT *pnShowCmd)
{
	// 既定のウィンドウ位置取得
	INT nShowCmd;
	WINDOWPLACEMENT wndPlace;
	wndPlace.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wndPlace);
//	RECT rcWnd = wndPlace.rcNormalPosition;
	RECT rcWnd;
	RECT rcWndRestore;

	DesktopCenterWindow(&rcWnd);
	rcWndRestore = rcWnd;

	// 前回ウィンドウ位置取得　※取れない場合は既定のウィンドウ位置
	m_IniFile.GetPrivateProfile(_T("WINDOWINFO"), _T("Left"), rcWnd.left, (INT*)&rcWnd.left);
	m_IniFile.GetPrivateProfile(_T("WINDOWINFO"), _T("Top"), rcWnd.top, (INT*)&rcWnd.top);
	m_IniFile.GetPrivateProfile(_T("WINDOWINFO"), _T("Right"), rcWnd.right, (INT*)&rcWnd.right);
	m_IniFile.GetPrivateProfile(_T("WINDOWINFO"), _T("Bottom"), rcWnd.bottom, (INT*)&rcWnd.bottom);
	m_IniFile.GetPrivateProfile(_T("WINDOWINFO"), _T("Show"), SW_SHOWNORMAL, (INT*)&nShowCmd);
	m_IniFile.GetPrivateProfile(_T("WINDOWINFO"), _T("MonitorBorder"), FALSE, (INT*)&m_bWindowMonitorBorder);

	// 四角が破綻してないかチェック
	if(IsRectEmpty(&rcWnd)){
		// 破綻してるなら既定値に戻す
//		rcWnd = wndPlace.rcNormalPosition;
		rcWnd = rcWndRestore;
	}

	// ウィンドウスタイル取得
#if 1
	LONG lStyle;
	lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
#else
	LONG_PTR lStyle;
	lStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
#endif
	// 境界線がダイアログ枠(サイズ固定ダイアログ)？
	if ((lStyle & (WS_CAPTION | DS_MODALFRAME)) == (WS_CAPTION | DS_MODALFRAME)) {
		// ダイアログサイズが破綻してないかチェック
		if ((rcWndRestore.right - rcWndRestore.left) != (rcWnd.right - rcWnd.left) ||
			(rcWndRestore.bottom - rcWndRestore.top) != (rcWnd.bottom - rcWnd.top)) {
			rcWnd = rcWndRestore;
		}
	}
	RECT rcVirtualScreen;	// 仮想スクリーン座標
	LONG marginLeft = 0;
	LONG marginRight = 0;
	LONG marginBottom = 0;

	if (m_bWindowMonitorBorder == FALSE) {
		// 仮想スクリーンサイズを取得
		rcVirtualScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
		rcVirtualScreen.right = GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		rcVirtualScreen.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
		rcVirtualScreen.bottom = GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN);

		// ウィンドウ画面範囲外許可マージン
		marginLeft = abs(rcWnd.right - rcWnd.left) / 2;
		marginRight = abs(rcWnd.right - rcWnd.left) / 2;
		marginBottom = abs(rcWnd.bottom - rcWnd.top) / 2;

	} else {
		// ウィンドウに最も近いモニタのハンドルを取得
		HMONITOR hMonitor = MonitorFromRect(&rcWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mInfo;
		mInfo.cbSize = sizeof(MONITORINFO);
		// 対象モニタの仮想スクリーンサイズを取得
		GetMonitorInfo(hMonitor, &mInfo);
		rcVirtualScreen = mInfo.rcMonitor;

		// 画面範囲外許可マージン無し
		marginLeft = 0;
		marginRight = 0;
		marginBottom = 0;
	}

	// 位置を補正
	if (rcWnd.right > (rcVirtualScreen.right + marginRight)) {
		rcWnd.left -= rcWnd.right - rcVirtualScreen.right;
		rcWnd.right = rcVirtualScreen.right;
	}
	if (rcWnd.left < (rcVirtualScreen.left - marginLeft)) {
		rcWnd.right += rcVirtualScreen.left - rcWnd.left;
		rcWnd.left = rcVirtualScreen.left;
	}
	if (rcWnd.bottom > (rcVirtualScreen.bottom + marginBottom)) {
		rcWnd.top -= rcWnd.bottom - rcVirtualScreen.bottom;
		rcWnd.bottom = rcVirtualScreen.bottom;
	}
	if (rcWnd.top < rcVirtualScreen.top) {
		rcWnd.bottom += rcVirtualScreen.top - rcWnd.top;
		rcWnd.top = rcVirtualScreen.top;
	}

	// ウィンドウ表示状態チェック
	switch (nShowCmd) {
		case SW_SHOWNORMAL:				// ウィンドウ通常化
//		case SW_SHOWMINIMIZED:			// ウィンドウ最小化
		case SW_SHOWMAXIMIZED:			// ウィンドウ最大化
			break;
		default:						// 上記以外
			nShowCmd = SW_SHOWNORMAL;	// ウィンドウ通常化に修正
			break;
	}

	*prcWnd = rcWnd;
	*pnShowCmd = nShowCmd;
}

// ワークエリア領域の取得
#define GetMonitorRect(rc)  SystemParametersInfo(SPI_GETWORKAREA,0,rc,0)

// デスクトップの画面中央に移動
void CPCMDSD_ConverterDlg::DesktopCenterWindow(RECT *prcWnd)
{
	RECT	rc1;		// デスクトップ領域
	RECT	rc2;		// ウインドウ領域
	INT 	cx, cy; 	// ウインドウ位置
	INT 	sx, sy; 	// ウインドウサイズ

	GetMonitorRect(&rc1);									// デスクトップのサイズ取得
	GetWindowRect(&rc2);									// ウインドウのサイズ取得

	sx = (rc2.right - rc2.left);							// ウインドウの横幅
	sy = (rc2.bottom - rc2.top);							// ウインドウの高さ
	cx = (((rc1.right - rc1.left) - sx) / 2 + rc1.left);	// 横方向の中央座標軸
	cy = (((rc1.bottom - rc1.top) - sy) / 2 + rc1.top); 	// 縦方向の中央座標軸
	
	prcWnd->left = cx;
	prcWnd->right = cx + sx;
	prcWnd->top = cy;
	prcWnd->bottom = cy + sy;
}


void CPCMDSD_ConverterDlg::OnLvnKeydownFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	NMLVKEYDOWN *nmkd = (NMLVKEYDOWN*)pNMHDR;
	int i;

	switch (nmkd->wVKey) {
		case 'A':
			// CTRL + A？
			if (GetAsyncKeyState(VK_CONTROL)){
				// 全行選択
				for(i = 0;i < m_lFileList.GetItemCount();i ++){
					m_lFileList.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				}
			}
			break;
		case VK_DELETE:
			// 選択行を削除
			OnBnClickedListdelete();
			break;
		default:
			break;
	}

	*pResult = 0;
}


void CPCMDSD_ConverterDlg::OnBnClickedCheckNormalize()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
//	m_NormalizeEnable = m_chkboxNormalize.GetCheck();
	m_NormalizeFlag = m_chkboxNormalize.GetCheck();
	GainModeGroupDisp();
}


void CPCMDSD_ConverterDlg::OnBnClickedButtonPathClear()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	// 出力先クリア
	m_ecPath.SetWindowText(_T(""));
	UpdateData(TRUE);
}


void CPCMDSD_ConverterDlg::OnBnClickedCheckFileoverwrite()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_DsfOverWriteEnable = m_chkboxFileOverWrite.GetCheck();
}


void CPCMDSD_ConverterDlg::OnEnChangeEditalbumTagSuffix()
{
	// TODO: これが RICHEDIT コントロールの場合、このコントロールが
	// この通知を送信するには、CDialogEx::OnInitDialog() 関数をオーバーライドし、
	// CRichEditCtrl().SetEventMask() を関数し呼び出します。
	// OR 状態の ENM_CHANGE フラグをマスクに入れて呼び出す必要があります。

	// TODO: ここにコントロール通知ハンドラー コードを追加してください。
	UpdateData();
}


void CPCMDSD_ConverterDlg::OnHdnDividerdblclickFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// カラムをデフォルト幅に戻す
	if (phdr->iItem < EN_LIST_COLUMN_MAX) {
		m_lFileList.SetColumnWidth(phdr->iItem, m_nColWidthDef[phdr->iItem]);
	}

	*pResult = 0;
}


void CPCMDSD_ConverterDlg::OnEnKillfocusEditalbumTagSuffix()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_evAlbumTagSuffix.TrimRight();
	UpdateData(FALSE);
}


void CPCMDSD_ConverterDlg::OnCbnSelchangeComboCompleteOption()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_nCompletePowerCrl = m_combCompleteOption.GetCurSel();
}

void CPCMDSD_ConverterDlg::GainModeGroupDisp()
{
	UpdateData();

	if (m_radioGainModeDdv == 0) {
		//*** ゲイン調整 ***
		m_combVol.EnableWindow(TRUE);
		m_chkboxNormalize.EnableWindow(TRUE);
		m_combLimitVol.EnableWindow(FALSE);
		m_chkboxCrossGainLevel.EnableWindow(FALSE);
	} else {
		//*** ゲイン制限 ***
		if(m_CrossGainLevel == 0){
			m_combVol.EnableWindow(FALSE);
		} else {
			m_combVol.EnableWindow(TRUE);
		}
		m_chkboxNormalize.EnableWindow(FALSE);
		m_combLimitVol.EnableWindow(TRUE);
		m_chkboxCrossGainLevel.EnableWindow(TRUE);
	}
}

void CPCMDSD_ConverterDlg::OnBnClickedRadioGainMode1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	GainModeGroupDisp();
}


void CPCMDSD_ConverterDlg::OnBnClickedRadioGainMode2()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	GainModeGroupDisp();
}


void CPCMDSD_ConverterDlg::OnBnClickedCheckCrossGainlevel()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_CrossGainLevel = m_chkboxCrossGainLevel.GetCheck();
	GainModeGroupDisp();
}

void CPCMDSD_ConverterDlg::OnBnClickedButtonSetting()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	CAutoSettingDlg dlg;
	INT_PTR dlgRet;

	// 現在の設定をAUTO設定ダイアログに反映
	dlg.DsdCombBoxSetCurSel(m_nDsdSampling44100,
							m_nDsdSampling88200,
							m_nDsdSampling176400,
							m_nDsdSampling352800,
							m_nDsdSampling705600,
							m_Pcm48KHzEnableDsd3MHzEnable);
	// AUTO設定ダイアログ表示
	dlgRet = dlg.DoModal();
	if(dlgRet == IDOK){
		// OKボタンなら設定内容を反映
		dlg.DsdCombBoxGetDsdSamplingRate(	&m_nDsdSampling44100,
											&m_nDsdSampling88200,
											&m_nDsdSampling176400,
											&m_nDsdSampling352800,
											&m_nDsdSampling705600,
											&m_Pcm48KHzEnableDsd3MHzEnable);
	}
}


// DSDサンプリングレートがAUTOなら設定ボタンを有効、AUTO以外の固定なら無効にする
void CPCMDSD_ConverterDlg::AutoSettingBtnEnableProc()
{
	int idx;
	BOOL bState;

	idx = m_cSamplingRate.GetCurSel();
	if (idx == 0) {
		// AUTOを選択中
		bState = TRUE;
	} else {
		// AUTO以外を選択中
		bState = FALSE;
	}

	m_btnAutSetting.EnableWindow(bState);
}

void CPCMDSD_ConverterDlg::OnCbnSelchangeSamplingrate()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	AutoSettingBtnEnableProc();
}


void CPCMDSD_ConverterDlg::OnLvnInsertitemFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// リストコントロール登録件数表示
	ListRegistDisp();

	*pResult = 0;
}


