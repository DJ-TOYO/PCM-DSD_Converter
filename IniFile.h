#pragma once
#include "stdafx.h"

class CIniFile
{
public:
	CIniFile();
	~CIniFile();
	void SetIniFile(CString strFile);
	DWORD GetPrivateProfile(CString strSection, CString strKey, CString strDefault, CString *pstrValue);
	DWORD GetPrivateProfile(CString strSection, CString strKey, int nDefault, INT *pnValue);
	BOOL WritePrivateProfile(CString strSection, CString strKey, CString strValue);
	BOOL WritePrivateProfile(CString strSection, CString strKey, int nValue);

protected:

protected:
	CString m_strIniFile;
};

