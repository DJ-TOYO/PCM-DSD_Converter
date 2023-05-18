// CAutoSettingDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "PCM-DSD_Converter.h"
#include "AutoSettingDlg.h"
#include "afxdialogex.h"


// CAutoSettingDlg ダイアログ
CString strDsdSamplingRate[] = {
	_T("64"),
	_T("128"),
	_T("256"),
	_T("512"),
	_T("1024"),
	_T("2048"),
};


IMPLEMENT_DYNAMIC(CAutoSettingDlg, CDialogEx)

CAutoSettingDlg::CAutoSettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AUTOSETTING_DIALOG, pParent)
{
	m_nPcm44100toDSDidx = 0;
	m_nPcm88200toDSDidx = 0;
	m_nPcm176400toDSDidx = 0;
	m_nPcm352800toDSDidx = 0;
	m_nPcm705600toDSDidx = 0;
	m_nPcm48KHzToDsd3MHzEnable = BST_UNCHECKED;
}

CAutoSettingDlg::~CAutoSettingDlg()
{
}

void CAutoSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PCM44100, m_cbPcm44100toDSD);
	DDX_Control(pDX, IDC_COMBO_PCM88200, m_cbPcm88200toDSD);
	DDX_Control(pDX, IDC_COMBO_PCM176400, m_cbPcm176400toDSD);
	DDX_Control(pDX, IDC_COMBO_PCM352800, m_cbPcm352800toDSD);
	DDX_Control(pDX, IDC_COMBO_PCM705600, m_cbPcm705600toDSD);
	DDX_Control(pDX, IDC_CHECK_DSD3MHZ_ENABLE, m_chkboxEnableDsd3MHz);
}


BEGIN_MESSAGE_MAP(CAutoSettingDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO_PCM44100, &CAutoSettingDlg::OnCbnSelchangeComboPcm44100)
	ON_CBN_SELCHANGE(IDC_COMBO_PCM88200, &CAutoSettingDlg::OnCbnSelchangeComboPcm88200)
	ON_CBN_SELCHANGE(IDC_COMBO_PCM176400, &CAutoSettingDlg::OnCbnSelchangeComboPcm176400)
	ON_CBN_SELCHANGE(IDC_COMBO_PCM352800, &CAutoSettingDlg::OnCbnSelchangeComboPcm352800)
	ON_CBN_SELCHANGE(IDC_COMBO_PCM705600, &CAutoSettingDlg::OnCbnSelchangeComboPcm705600)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT, &CAutoSettingDlg::OnBnClickedButtonDefault)
	ON_BN_CLICKED(IDC_CHECK_DSD3MHZ_ENABLE, &CAutoSettingDlg::OnBnClickedCheckDsd3mhzEnable)
END_MESSAGE_MAP()


// CAutoSettingDlg メッセージ ハンドラー


BOOL CAutoSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO: ここに初期化を追加してください
	CString strNote;

	strNote = _T("PCMサンプリングレートからDSDに変換するサンプリングレートを設定出来ます。\n");
	strNote += _T("\n");
	strNote += _T("【DSDサンプリングレート】\n");
	strNote += _T("DSD  64： 2.8 / 3.0MHz\n");
	strNote += _T("DSD 128： 5.6 / 6.1MHz\n");
	strNote += _T("DSD 256：11.2 / 12.2MHz\n");
	strNote += _T("DSD 512：22.5 / 24.5MHz\n");
	strNote += _T("DSD1024：45.1 / 49.1MHz\n");
	strNote += _T("DSD2048：90.3 / 98.3MHz\n");

	StaticTextDisp(IDC_STATIC_SETTINGNOTE, strNote);

	int i;
	for(i = 0;i < 6;i ++){
		m_cbPcm44100toDSD.AddString(strDsdSamplingRate[i]);
		m_cbPcm88200toDSD.AddString(strDsdSamplingRate[i]);
		m_cbPcm176400toDSD.AddString(strDsdSamplingRate[i]);
		m_cbPcm352800toDSD.AddString(strDsdSamplingRate[i]);
		m_cbPcm705600toDSD.AddString(strDsdSamplingRate[i]);
	}
	m_cbPcm44100toDSD.SetCurSel(m_nPcm44100toDSDidx);
	m_cbPcm88200toDSD.SetCurSel(m_nPcm88200toDSDidx);
	m_cbPcm176400toDSD.SetCurSel(m_nPcm176400toDSDidx);
	m_cbPcm352800toDSD.SetCurSel(m_nPcm352800toDSDidx);
	m_cbPcm705600toDSD.SetCurSel(m_nPcm705600toDSDidx);
	m_chkboxEnableDsd3MHz.SetCheck(m_nPcm48KHzToDsd3MHzEnable);

	DsdConvertDescriptionDisp();

	// 親ウィンドウの中央に表示	
	CenterWindow();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

CString CAutoSettingDlg::DsdConvertNote(int nPcm, int nDsd, int nPcm48KHzToDsd3MHzEnable)
{
	CString strNote;
	int nPcmSamplingRate44;
	int nPcmSamplingRate48;
	int nDsdRatio;
	double nDsdSamplingRate28;
	double nDsdSamplingRate30;

	// PCMサンプリングレート
	switch (nPcm) {
		case 0:								// 44.1KHz/48.0KHz
			nPcmSamplingRate44 = 44100;
			nPcmSamplingRate48 = 48000;
			break;
		case 1:								// 88.2KHz/96.0KHz
			nPcmSamplingRate44 = 88200;
			nPcmSamplingRate48 = 96000;
			break;
		case 2:								// 176.4KHz/192.0KHz
			nPcmSamplingRate44 = 176400;
			nPcmSamplingRate48 = 192000;
			break;
		case 3:								// 352.8KHz/384.0KHz
			nPcmSamplingRate44 = 352800;
			nPcmSamplingRate48 = 384000;
			break;
		case 4:								// 705.6KHz/768.0KHz
			nPcmSamplingRate44 = 705600;
			nPcmSamplingRate48 = 768000;
			break;
		default:
			nPcmSamplingRate44 = 44100;
			nPcmSamplingRate48 = 48000;
			break;
	}

	// DSD倍数
	switch (nDsd) {
		case 0:								// DSD64(2.8MHz/3.0MHz)
			nDsdRatio = 64;
			break;
		case 1:								// DSD128(5.6MHz/6.1MHz)
			nDsdRatio = 128;
			break;
		case 2:								// DSD256(11.2MHz/12.2MHz)
			nDsdRatio = 256;
			break;
		case 3:								// DSD512(22.5MHz/24.5MHz)
			nDsdRatio = 512;
			break;
		case 4:								// DSD1024(45.1MHz/49.1MHz)
			nDsdRatio = 1024;
			break;
		case 5:								// DSD2048(90.3MHz/98.3MHz)
			nDsdRatio = 2048;
			break;
		default:
			nDsdRatio = 64;
			break;
	}

	// PCMサンプリングレートHz → DSD サンプリングレート
	nDsdSamplingRate28 = (double)(nPcmSamplingRate44 * nDsdRatio);
	nDsdSamplingRate30 = (double)(nPcmSamplingRate48 * nDsdRatio);

	// Hz → MHz ※少数第２位以下切り捨て
	nDsdSamplingRate28 /= 100000;
	nDsdSamplingRate30 /= 100000;
	nDsdSamplingRate28 = floor(nDsdSamplingRate28);
	nDsdSamplingRate30 = floor(nDsdSamplingRate30);
	nDsdSamplingRate28 /= 10;
	nDsdSamplingRate30 /= 10;

	if(nPcm48KHzToDsd3MHzEnable == 0){
		strNote.Format(_T("DSD%0.1lfMHz"), nDsdSamplingRate28);
	} else {
		strNote.Format(_T("DSD%0.1lfMHz / %0.1lfMHz"), nDsdSamplingRate28, nDsdSamplingRate30);
	}
	return strNote;
}

void CAutoSettingDlg::StaticTextDisp(int nID, CString strText)
{
	CStatic *pStaticCtrl;
	CWnd* pWnd;

	pWnd = GetDlgItem(nID);
	pStaticCtrl = (CStatic*)GetDlgItem(nID);
	pStaticCtrl->SetWindowText(strText);
}

void CAutoSettingDlg::DsdConvertDescriptionDisp()
{
	CString strDescription;

	strDescription = DsdConvertNote(0, m_nPcm44100toDSDidx, m_nPcm48KHzToDsd3MHzEnable);
	StaticTextDisp(IDC_STATIC_PCM44100DSD ,strDescription);
	strDescription = DsdConvertNote(0, m_nPcm88200toDSDidx, m_nPcm48KHzToDsd3MHzEnable);
	StaticTextDisp(IDC_STATIC_PCM88200DSD ,strDescription);
	strDescription = DsdConvertNote(0, m_nPcm176400toDSDidx, m_nPcm48KHzToDsd3MHzEnable);
	StaticTextDisp(IDC_STATIC_PCM176400DSD, strDescription);
	strDescription = DsdConvertNote(0, m_nPcm352800toDSDidx, m_nPcm48KHzToDsd3MHzEnable);
	StaticTextDisp(IDC_STATIC_PCM352800DSD, strDescription);
	strDescription = DsdConvertNote(0, m_nPcm705600toDSDidx, m_nPcm48KHzToDsd3MHzEnable);
	StaticTextDisp(IDC_STATIC_PCM705600DSD, strDescription);
}

int CAutoSettingDlg::DsdSamplingRateToComboBoxIdx(int nDsdSamplingRate)
{
	int nIdx;

	switch (nDsdSamplingRate) {
	case 64:	nIdx = 0;	break;
	case 128:	nIdx = 1;	break;
	case 256:	nIdx = 2;	break;
	case 512:	nIdx = 3;	break;
	case 1024:	nIdx = 4;	break;
	case 2048:	nIdx = 5;	break;
	default:	nIdx = 0;	break;
	}

	return nIdx;
}

int CAutoSettingDlg::ComboBoxIdxToDsdSamplingRate(int nIdx)
{
	int nDsdSamplingRate;

	switch (nIdx) {
	case 0:		nDsdSamplingRate = 64;		break;
	case 1:		nDsdSamplingRate = 128;		break;
	case 2:		nDsdSamplingRate = 256;		break;
	case 3:		nDsdSamplingRate = 512;		break;
	case 4:		nDsdSamplingRate = 1024;	break;
	case 5:		nDsdSamplingRate = 2048;	break;
	default:	nDsdSamplingRate = 64;		break;
	}

	return nDsdSamplingRate;
}

void CAutoSettingDlg::DsdCombBoxSetCurSel(int nDsdSampling44100, int nDsdSampling88200, int nDsdSampling176400, int nDsdSampling352800, int nDsdSampling705600, int nPcm48KHzToDsd3MHzEnable)
{
	m_nPcm44100toDSDidx = DsdSamplingRateToComboBoxIdx(nDsdSampling44100);
	m_nPcm88200toDSDidx = DsdSamplingRateToComboBoxIdx(nDsdSampling88200);
	m_nPcm176400toDSDidx = DsdSamplingRateToComboBoxIdx(nDsdSampling176400);
	m_nPcm352800toDSDidx = DsdSamplingRateToComboBoxIdx(nDsdSampling352800);
	m_nPcm705600toDSDidx = DsdSamplingRateToComboBoxIdx(nDsdSampling705600);
	if (nPcm48KHzToDsd3MHzEnable != 0) {
		nPcm48KHzToDsd3MHzEnable = 1;
	}
	m_nPcm48KHzToDsd3MHzEnable = nPcm48KHzToDsd3MHzEnable;
}

void CAutoSettingDlg::DsdCombBoxGetDsdSamplingRate(int* pnDsdSampling44100, int* pnDsdSampling88200, int* pnDsdSampling176400, int* pnDsdSampling352800, int* pnDsdSampling705600, int* pnPcm48KHzToDsd3MHzEnable)
{
	*pnDsdSampling44100 = ComboBoxIdxToDsdSamplingRate(m_nPcm44100toDSDidx);
	*pnDsdSampling88200 = ComboBoxIdxToDsdSamplingRate(m_nPcm88200toDSDidx);
	*pnDsdSampling176400 = ComboBoxIdxToDsdSamplingRate(m_nPcm176400toDSDidx);
	*pnDsdSampling352800 = ComboBoxIdxToDsdSamplingRate(m_nPcm352800toDSDidx);
	*pnDsdSampling705600 = ComboBoxIdxToDsdSamplingRate(m_nPcm705600toDSDidx);
	*pnPcm48KHzToDsd3MHzEnable = m_nPcm48KHzToDsd3MHzEnable;
}


void CAutoSettingDlg::OnCbnSelchangeComboPcm44100()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_nPcm44100toDSDidx = m_cbPcm44100toDSD.GetCurSel();
	DsdConvertDescriptionDisp();
}


void CAutoSettingDlg::OnCbnSelchangeComboPcm88200()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_nPcm88200toDSDidx = m_cbPcm88200toDSD.GetCurSel();
	DsdConvertDescriptionDisp();
}


void CAutoSettingDlg::OnCbnSelchangeComboPcm176400()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_nPcm176400toDSDidx = m_cbPcm176400toDSD.GetCurSel();
	DsdConvertDescriptionDisp();
}


void CAutoSettingDlg::OnCbnSelchangeComboPcm352800()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_nPcm352800toDSDidx = m_cbPcm352800toDSD.GetCurSel();
	DsdConvertDescriptionDisp();
}


void CAutoSettingDlg::OnCbnSelchangeComboPcm705600()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_nPcm705600toDSDidx = m_cbPcm705600toDSD.GetCurSel();
	DsdConvertDescriptionDisp();
}


void CAutoSettingDlg::OnBnClickedButtonDefault()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int i = MessageBox(_T("初期値に戻しますか？"), _T(""), MB_YESNO);
	if (i == IDYES) {
		m_nPcm44100toDSDidx = 0;
		m_nPcm88200toDSDidx = 1;
		m_nPcm176400toDSDidx = 2;
		m_nPcm352800toDSDidx = 2;
		m_nPcm705600toDSDidx = 2;
		m_nPcm48KHzToDsd3MHzEnable = BST_UNCHECKED;
		m_cbPcm44100toDSD.SetCurSel(m_nPcm44100toDSDidx);
		m_cbPcm88200toDSD.SetCurSel(m_nPcm88200toDSDidx);
		m_cbPcm176400toDSD.SetCurSel(m_nPcm176400toDSDidx);
		m_cbPcm352800toDSD.SetCurSel(m_nPcm352800toDSDidx);
		m_cbPcm705600toDSD.SetCurSel(m_nPcm705600toDSDidx);
		m_chkboxEnableDsd3MHz.SetCheck(m_nPcm48KHzToDsd3MHzEnable);
		DsdConvertDescriptionDisp();
	}
}


void CAutoSettingDlg::OnBnClickedCheckDsd3mhzEnable()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	m_nPcm48KHzToDsd3MHzEnable = (BOOL)m_chkboxEnableDsd3MHz.GetCheck();
	DsdConvertDescriptionDisp();
}
