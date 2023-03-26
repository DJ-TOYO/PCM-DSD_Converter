#pragma once

#pragma warning(disable:4786)
#include <map>

#include <Mmsystem.h>

class CRiffSIF  
{
public:
	CRiffSIF();
	virtual ~CRiffSIF();
	void Release();
	BOOL IsEnable(){return m_bEnable;};
	void SetJunkHeader(LPCTSTR junk){m_strJunkHeader = junk;};
	DWORD GetStreamSize(){return m_dwStreamSize;};
	BOOL SetField(char id1,char id2,char id3,char id4,LPCTSTR szData);
	CString GetField(char id1,char id2,char id3,char id4);
	BOOL FindChunk(HANDLE hFile,DWORD dwFileSize,UINT flag,FOURCC type,DWORD *pdwSize,BOOL bModify);
	DWORD GetTotalFieldSize();
	DWORD Load(LPCTSTR szFileName,char id1,char id2,char id3,char id4);
	DWORD Save(LPCTSTR szFileName);

protected:
	BOOL m_bEnable;
//	CString m_szFileName;
//	CDlgWriteProgress m_dlg;
private:
	FOURCC m_type;
	DWORD GetInfoChunkSize();
	typedef std::map<FOURCC,CString> FieldMap;
	FieldMap m_fields;
	DWORD m_dwStreamSize;
	CString m_strJunkHeader;
};

