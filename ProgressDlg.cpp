// ProgressDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "PCM-DSD_Converter.h"
#include "ProgressDlg.h"
#include "afxdialogex.h"

// ProgressDlg ダイアログ
#define DISPLAY_ON -1
#define DISPLAY_OFF 2

IMPLEMENT_DYNAMIC(ProgressDlg, CDialogEx)

ProgressDlg::ProgressDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(ProgressDlg::IDD, pParent)
, m_evTEXT(_T(""))
{
	m_ptrTaskbarList3 = NULL;
	m_PercentLatest = 0;

	// COMライブラリ初期化 ※これ呼ばないとCoCreateInstance(CLSID_TaskbarList)が例外エラーになる
	CoInitialize(NULL);
}

ProgressDlg::~ProgressDlg()
{
	// CoInitializeとセットで呼び出すと思ったが解放すると何故か例外になるのでコメントアウトする。
	// 特にこれでもリークしてないように思われる。
//	CoUninitialize();
}

BOOL ProgressDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 親ウィンドウ取得
	m_pParent = GetParent();

	HRESULT hRes = m_ptrTaskbarList3.CoCreateInstance(CLSID_TaskbarList);
	ATLASSERT(SUCCEEDED(hRes));
	
	return TRUE;
}

void ProgressDlg::OnSetFocus() {
	::DestroyCaret();
}

//子ダイアログでの操作は無視する
void ProgressDlg::OnCancel(){
	//DestroyWindow();
}
void ProgressDlg::OnOK(){
	//DestroyWindow();
}
void ProgressDlg::PostNcDestroy()
{
	//delete this;    //BBB追加  ダイアログの破棄
}
void ProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_pProgress);
	DDX_Text(pDX, IDC_PATHTEXT, m_evTEXT);
	DDX_Control(pDX, IDC_PATHTEXT, m_ecTEXT);
	DDX_Control(pDX, IDC_EDIT2, m_ecTimes);
	DDX_Control(pDX, IDC_CancelBottun, m_btnStop);
}

//子ダイアログでのパス名表示
void ProgressDlg::Start(TCHAR *Path){
	CString strMsg;
	strMsg = Path;
//	strMsg += L"をDSDに変換中";
	m_ecTEXT.SetWindowText(strMsg);

	m_pProgress.SetRange32(0, 100);
	m_pProgress.SetPos(0);

	m_ptrTaskbarList3->SetProgressState(m_pParent->m_hWnd, TBPF_NORMAL);
	m_ptrTaskbarList3->SetProgressValue(m_pParent->m_hWnd, 0, 100);

	m_btnStop.EnableWindow(TRUE);
}

void ProgressDlg::StartSeq(unsigned int state, CString strText)
{
	CString strMsg;
	switch(state){
		case 0:
			strMsg = strText;
			break;
		case 1:
			strMsg = "[";
			strMsg += strText;
			strMsg += "]";
	//		strMsg += L"\nフォルダをアルバムモードでDSDに変換中";
			strMsg += L"\nフォルダ";
			break;
		default:
			break;
	}

	m_ptrTaskbarList3->SetProgressState(m_pParent->m_hWnd, TBPF_NORMAL);
	m_pProgress.SetRange32(0, 100);
	m_pProgress.SetPos(0);

	m_ptrTaskbarList3->SetProgressValue(m_pParent->m_hWnd, 0, 100);

	m_PercentLatest = 0;

	m_btnStop.EnableWindow(TRUE);
	m_ecTEXT.SetWindowText(strMsg);
}

//プログレスバー管理
void ProgressDlg::Process(unsigned int state, unsigned int percent, unsigned int position, unsigned int nDSDrate)
{
	CString strMsg;
	m_pProgress.SetRange32(0, position);

	if(state != 2){
		m_pProgress.SetPos(percent);
		m_ptrTaskbarList3->SetProgressValue(m_pParent->m_hWnd, percent, position);
	} else {
		if(m_PercentLatest < percent || percent <= 1){
			m_PercentLatest = percent;
			m_pProgress.SetPos(percent);
			m_ptrTaskbarList3->SetProgressValue(m_pParent->m_hWnd, percent, position);
		}
	}

	switch(state){
		case 0:
			SetWindowText(_T("DSD変換中(STEP 1/3)"));
			strMsg = L"アップサンプリングの準備中(FLACをデコード中)";
			m_ecTimes.SetWindowText(strMsg);
			break;
		case 1:
			SetWindowText(_T("DSD変換中(STEP 1/3)"));
			strMsg = L"アップサンプリングの準備中";
			m_ecTimes.SetWindowText(strMsg);
			break;
		case 2:
			if (percent == 1) {
				SetWindowText(_T("DSD変換中(STEP 2/3)"));
				strMsg.Format(_T("DSD%dに変換中"), nDSDrate);
				m_ecTimes.SetWindowText(strMsg);
			}
			break;
		case 3:
			SetWindowText(_T("DSD変換中(STEP 3/3)"));
			strMsg = L"DSFファイルに出力中";
			m_ecTimes.SetWindowText(strMsg);
			m_btnStop.EnableWindow(FALSE);
			break;
		case 4:
			SetWindowText(_T("DFF to DSF変換中"));
			strMsg = L"DSFファイルに出力中";
			m_ecTimes.SetWindowText(strMsg);
			break;
		case 5:
			SetWindowText(_T("DSD変換中(STEP 1/3)"));
			strMsg = L"アップサンプリングの準備中(ALACをデコード中)";
			m_ecTimes.SetWindowText(strMsg);
			break;
		default:
			break;
	}
}

BEGIN_MESSAGE_MAP(ProgressDlg, CDialogEx)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_CancelBottun, &ProgressDlg::OnBnClickedCancelbottun)
	ON_BN_CLICKED(IDC_BTN_MONITOR_OFF, &ProgressDlg::OnBnClickedBtnMonitorOff)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

//F1ヘルプ無効化
BOOL ProgressDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	return true;
}

//中止ボタンが押された
void ProgressDlg::OnBnClickedCancelbottun()
{
	Cancelbottun = false;
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
}


void ProgressDlg::OnBnClickedBtnMonitorOff()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// モニタOFF
	Sleep(200);	// ユーザーのマウス操作のチャタリング対策 ※ボタンクリック時にすぐに反応しないようにちょっとWait
	MonitorPower(FALSE);
}

void ProgressDlg::MonitorPower(BOOL flag)
{
	int mode;

	if (flag == TRUE) {
		mode = DISPLAY_ON;
	}
	else {
		mode = DISPLAY_OFF;
	}

	::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, mode);
}


void ProgressDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	// ダイアログが非表示になる？
	if(bShow == FALSE){
		// タスクバーのプログレスバーをクリア
		m_ptrTaskbarList3->SetProgressValue(m_pParent->m_hWnd, 0, 100);
	}
}
