// ProgressDlg.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "PCM-DSD_Converter.h"
#include "ProgressDlg.h"
#include "afxdialogex.h"

// ProgressDlg �_�C�A���O
#define DISPLAY_ON -1
#define DISPLAY_OFF 2

IMPLEMENT_DYNAMIC(ProgressDlg, CDialogEx)

ProgressDlg::ProgressDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(ProgressDlg::IDD, pParent)
, m_evTEXT(_T(""))
{
	m_ptrTaskbarList3 = NULL;
	m_PercentLatest = 0;

	// COM���C�u���������� ������Ă΂Ȃ���CoCreateInstance(CLSID_TaskbarList)����O�G���[�ɂȂ�
	CoInitialize(NULL);
}

ProgressDlg::~ProgressDlg()
{
	// CoInitialize�ƃZ�b�g�ŌĂяo���Ǝv�������������Ɖ��̂���O�ɂȂ�̂ŃR�����g�A�E�g����B
	// ���ɂ���ł����[�N���ĂȂ��悤�Ɏv����B
//	CoUninitialize();
}

BOOL ProgressDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �e�E�B���h�E�擾
	m_pParent = GetParent();

	HRESULT hRes = m_ptrTaskbarList3.CoCreateInstance(CLSID_TaskbarList);
	ATLASSERT(SUCCEEDED(hRes));
	
	return TRUE;
}

void ProgressDlg::OnSetFocus() {
	::DestroyCaret();
}

//�q�_�C�A���O�ł̑���͖�������
void ProgressDlg::OnCancel(){
	//DestroyWindow();
}
void ProgressDlg::OnOK(){
	//DestroyWindow();
}
void ProgressDlg::PostNcDestroy()
{
	//delete this;    //BBB�ǉ�  �_�C�A���O�̔j��
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

//�q�_�C�A���O�ł̃p�X���\��
void ProgressDlg::Start(TCHAR *Path){
	CString strMsg;
	strMsg = Path;
//	strMsg += L"��DSD�ɕϊ���";
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
	//		strMsg += L"\n�t�H���_���A���o�����[�h��DSD�ɕϊ���";
			strMsg += L"\n�t�H���_";
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

//�v���O���X�o�[�Ǘ�
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
			SetWindowText(_T("DSD�ϊ���(STEP 1/3)"));
			strMsg = L"�A�b�v�T���v�����O�̏�����(FLAC���f�R�[�h��)";
			m_ecTimes.SetWindowText(strMsg);
			break;
		case 1:
			SetWindowText(_T("DSD�ϊ���(STEP 1/3)"));
			strMsg = L"�A�b�v�T���v�����O�̏�����";
			m_ecTimes.SetWindowText(strMsg);
			break;
		case 2:
			if (percent == 1) {
				SetWindowText(_T("DSD�ϊ���(STEP 2/3)"));
				strMsg.Format(_T("DSD%d�ɕϊ���"), nDSDrate);
				m_ecTimes.SetWindowText(strMsg);
			}
			break;
		case 3:
			SetWindowText(_T("DSD�ϊ���(STEP 3/3)"));
			strMsg = L"DSF�t�@�C���ɏo�͒�";
			m_ecTimes.SetWindowText(strMsg);
			m_btnStop.EnableWindow(FALSE);
			break;
		case 4:
			SetWindowText(_T("DFF to DSF�ϊ���"));
			strMsg = L"DSF�t�@�C���ɏo�͒�";
			m_ecTimes.SetWindowText(strMsg);
			break;
		case 5:
			SetWindowText(_T("DSD�ϊ���(STEP 1/3)"));
			strMsg = L"�A�b�v�T���v�����O�̏�����(ALAC���f�R�[�h��)";
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

//F1�w���v������
BOOL ProgressDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	// TODO: �����Ƀ��b�Z�[�W �n���h���[ �R�[�h��ǉ����邩�A����̏������Ăяo���܂��B

	return true;
}

//���~�{�^���������ꂽ
void ProgressDlg::OnBnClickedCancelbottun()
{
	Cancelbottun = false;
	// TODO: �����ɃR���g���[���ʒm�n���h���[ �R�[�h��ǉ����܂��B
}


void ProgressDlg::OnBnClickedBtnMonitorOff()
{
	// TODO: �����ɃR���g���[���ʒm�n���h���[ �R�[�h��ǉ����܂��B

	// ���j�^OFF
	Sleep(200);	// ���[�U�[�̃}�E�X����̃`���^�����O�΍� ���{�^���N���b�N���ɂ����ɔ������Ȃ��悤�ɂ������Wait
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

	// TODO: �����Ƀ��b�Z�[�W �n���h���[ �R�[�h��ǉ����܂��B
	// �_�C�A���O����\���ɂȂ�H
	if(bShow == FALSE){
		// �^�X�N�o�[�̃v���O���X�o�[���N���A
		m_ptrTaskbarList3->SetProgressValue(m_pParent->m_hWnd, 0, 100);
	}
}
