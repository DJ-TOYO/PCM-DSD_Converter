#pragma once


// CAutoSettingDlg ダイアログ

class CAutoSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAutoSettingDlg)

public:
	CAutoSettingDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~CAutoSettingDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AUTOSETTING_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()

	int m_nPcm44100toDSDidx;
	int m_nPcm88200toDSDidx;
	int m_nPcm176400toDSDidx;
	int m_nPcm352800toDSDidx;
	int m_nPcm705600toDSDidx;
	int m_nPcm48KHzToDsd3MHzEnable;	// 48KHz系PCMは、DSD3.0MHzとして出力する　FALSE:無効 TRUE:有効

	CString DsdConvertNote(int nPcm, int nDsd, int nPcm48KHzToDsd3MHzEnable);
	void StaticTextDisp(int nID, CString strText);
	void DsdConvertDescriptionDisp();
	int DsdSamplingRateToComboBoxIdx(int nDsdSamplingRate);
	int ComboBoxIdxToDsdSamplingRate(int nIdx);
public:
	virtual BOOL OnInitDialog();
	CComboBox m_cbPcm44100toDSD;
	CComboBox m_cbPcm88200toDSD;
	CComboBox m_cbPcm176400toDSD;
	CComboBox m_cbPcm352800toDSD;
	CComboBox m_cbPcm705600toDSD;
	CButton m_chkboxEnableDsd3MHz;
	void DsdCombBoxSetCurSel(int nDsdSampling44100, int nDsdSampling88200, int nDsdSampling176400, int nDsdSampling352800, int nDsdSampling705600, BOOL bPcm48KHzToDsd3MHzEnable);
	void DsdCombBoxGetDsdSamplingRate(int* pnDsdSampling44100, int* pnDsdSampling88200, int* pnDsdSampling176400, int* pnDsdSampling352800, int* pnDsdSampling705600, BOOL *pbPcm48KHzToDsd3MHzEnable);
	afx_msg void OnCbnSelchangeComboPcm44100();
	afx_msg void OnCbnSelchangeComboPcm88200();
	afx_msg void OnCbnSelchangeComboPcm176400();
	afx_msg void OnCbnSelchangeComboPcm352800();
	afx_msg void OnCbnSelchangeComboPcm705600();
	afx_msg void OnBnClickedButtonDefault();
	afx_msg void OnBnClickedCheckDsd3mhzEnable();
};
