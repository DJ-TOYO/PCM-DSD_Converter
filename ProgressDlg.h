#pragma once
#include <Shobjidl.h>
#include "afxwin.h"
#include "afxcmn.h"

// ProgressDlg ダイアログ

class ProgressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ProgressDlg)

public:
	ProgressDlg(CWnd* pParent = NULL);   // 標準コンストラクター
	virtual ~ProgressDlg();

	// ダイアログ データ
	enum { IDD = IDD_PROGRESS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()

	unsigned int m_PercentLatest;

//	ITaskbarList3 *m_ptrTaskbarList3;
	CComPtr<ITaskbarList3> m_ptrTaskbarList3;
	CWnd* m_pParent;
	CString m_strSrcPath;
	double m_dAvgdB;
	int m_nProgressVal;
	BOOL m_bAlbumMode;

public:
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();
	void Start(CString strPath);
//	void StartSeq(CString strText);
	void StartSeq(unsigned int state, CString strFolder);
	void StartSeq(unsigned int state, CString strFolder, CString strFileName, unsigned int percent, unsigned int position);
	void Process(unsigned int state, unsigned int percent, unsigned int position, unsigned int nDSDrate = 0);
	void SetAveragedB(double dVal);
	CProgressCtrl m_pProgress;
	CString m_evTEXT;
	afx_msg BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	CEdit m_ecTEXT;
	virtual void OnSetFocus();
	CEdit m_ecTimes;
	afx_msg void OnBnClickedCancelbottun();
	bool Cancelbottun = true;
	afx_msg void OnBnClickedBtnMonitorOff();
	void MonitorPower(BOOL flag);
	CButton m_btnStop;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	CStatic m_scProgressVal;
};
