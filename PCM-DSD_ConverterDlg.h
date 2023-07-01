
// PCM-DSD_ConverterDlg.h : �w�b�_�[ �t�@�C��
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
#define GAIN_LIMIT_DB_OFFSET		10	// �Q�C�������̊J�ndB

typedef enum
{
	EN_LIST_COLUMN_FILE_NAME = 0,				//  0:�t�@�C����
	EN_LIST_COLUMN_TITLE,						//  1:�^�C�g��
	EN_LIST_COLUMN_ARTIST,						//  2:�A�[�e�B�X�g
	EN_LIST_COLUMN_ALBUM,						//  3:�A���o��
	EN_LIST_COLUMN_TRACK_NO,					//  4:�g���b�NNo
	EN_LIST_COLUMN_DISC_NO,						//  5:Disc No
	EN_LIST_COLUMN_DISC_TOTAL,					//  6:Disc ��
	EN_LIST_COLUMN_ALBUM_ARTIST,				//  7:�A���o���A�[�e�B�X�g
	EN_LIST_COLUMN_SAMPLING_RATE,				//  8:�T���v�����O���[�g
	EN_LIST_COLUMN_BIT,							//  9:�r�b�g��
	EN_LIST_COLUMN_LENGTH,						// 10:����
	EN_LIST_COLUMN_SAMPLING_COUNT,				// 11:�T���v����
	EN_LIST_COLUMN_PATH,						// 12:�t�@�C���p�X
	EN_LIST_COLUMN_EXT,							// 13:�g���q

	EN_LIST_COLUMN_MAX,							// 13:�ő區�ڐ�
}EN_LIST_COLUMN;

// �g���q���
typedef enum
{
	EXT_TYPE_UNKNOW = 0,	// �s��
	EXT_TYPE_WAV,			// .wav
	EXT_TYPE_WAVE64,		// .w64 ��SONY WAVE64
	EXT_TYPE_FLAC,			// .flac
	EXT_TYPE_ALAC,			// .m4a ��ALAC�̂�
	EXT_TYPE_DFF,			// .dff ��DSDIFF
	EXT_TYPE_MAX			// �g���q��ʍő吔
}EXT_TYPE;

typedef struct {
	UINT32 nLength;
	BYTE *pData;
}STPICTUREINFO, *PSTPICTUREINFO;

// FLAC Comment Field Recommendations
// https://www.xiph.org/vorbis/doc/v-comment.html
// https://www.legroom.net/2009/05/09/ogg-vorbis-and-flac-comment-field-recommendations
typedef struct {
	CString strAlbum;			// �A���o��
	CString strArtist;			// �A�[�e�B�X�g
	CString strAlbumArtist;		// �A���o���A�[�e�B�X�g
	CString strPublisher;		// ���s��
	CString strCopyright;		// ���쌠
	CString strDiscnumber;		// Disc No
	CString strDisctotal;		// Disc Total
	CString strIsrc;			// ���ەW���^��(ISRC)�R�[�h
	CString strUpn;				// Universal Product Number or EAN
	CString strLabel;			// ���x��
	CString strLabelNo;			// ���x��No
	CString strLicense;			// ���C�Z���X
	CString strOpus;			// �I�[�p�X
	CString strSourcemedia;		// �\�[�X���f�B�A
	CString strTitle;			// �ȃ^�C�g��
	CString strTracknumber;		// �g���b�NNo
	CString strTracktotal;		// �g���b�NTotal
	CString strVersion;			// �o�[�W����
	CString strEncodedBy;		// �G���R�[�h��
	CString strEncoded;			// �G���R�[�h�ݒ�
	CString strLyrics;			// �̎�
	CString strWebsite;			// Web Site
	CString strComposer;		// ��ȉ�
	CString strArranger;		// �ҋ�
	CString strLyricist;		// �쎌��
	CString strAuthor;			// ����
	CString strConductor;		// �w����
	CString strPerformer;		// ���t��
	CString strEnsemble;		// ���t�O���[�v
	CString strPart;			// �g���b�N�̃p�[�g
	CString strPartnumber;		// �g���b�N�̃p�[�gNo
	CString strGenre;			// �W������
	CString strDate;			// ���t
	CString strLocation;		// ���^�ꏊ
	CString strComment;			// �R�����g
	CString strOrganization;	// �g�D
	CString strDescription;		// ����
	CString strContact;			// �R���^�N�g
	STPICTUREINFO stPicture[FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED];
} STFLAC_COMMENT, *PSTFLAC_COMMENT;

typedef struct {
	CString strTALB;			// �A���o��
	CString strTPE1;			// �A�[�e�B�X�g
	CString strTPE2;			// �o���h���A�[�e�B�X�g
	CString strTIT2;			// �ȃ^�C�g��
	CString strTRCK;			// �g���b�NNo�@�� ISO-8859-1
	CString strTPOS;			// �Z�b�g���̈ʒu ��CD���� 2���g�Ȃ�u1/2�v�u2/2�v�Ȃ�
	CString strTCON;			// �W������
	CString strCOMM;			// �R�����g
	CString strTCOM;			// ��ȉ�
	CString strTYER;			// �N�@�� ISO-8859-1
	CString strTSSE;			// �\�t�g�E�F�A
	CString strTCOP;			// ���쌠
	CString strTENC;			// �G���R�[�h�����l
	CString strTEXT;			// �쎌��
	CString strTXXX;			// ���[�U�[��`
	STPICTUREINFO stPicture[FLAC__STREAM_METADATA_PICTURE_TYPE_UNDEFINED];
} STID3TAGINFO, *PSTID3TAGINFO;

typedef struct {
	char cFlameID[4];			// �t���[��ID
	DWORD dwSize;				// �T�C�Y
	WORD wFlag;					// �t���O
} STID3_TAG_FLAME_HEADER, *PSTID3_TAG_FLAME_HEADER;

typedef struct {
	CString strOrgFile;				// �\�[�X�t�@�C����
	UINT64 iStartPos;				// 64BIT PCM�Ǎ��݃I�t�Z�b�g�ʒu
	UINT64 iOrigDataSize;			// �\�[�X�f�[�^�T�C�Y
	UINT nTagMode;					// 0:TAG���� 1:FLAC TAG����
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

// �U���f�[�^
typedef struct {
	double dAmpPeak[2];				// �U���s�[�N�l [0]:Ch L [1]:Ch R
	double dAmpSum[2];				// �U���ώZ�l [0]:Ch L [1]:Ch R
	UINT64 ullAmpSumCnt[2];			// �U���ώZ�� [0]:Ch L [1]:Ch R
}ST_AMP_POWER,*PST_AMP_POWER;

// CPCMDSD_ConverterDlg �_�C�A���O
class CPCMDSD_ConverterDlg : public CDialogEx
{
	// �R���X�g���N�V����
public:
	CPCMDSD_ConverterDlg(CWnd* pParent = NULL);	// �W���R���X�g���N�^�[
//	string flag_Bottun = "";
	DWORD m_dwConvertProcessFlag;
	// �_�C�A���O �f�[�^
	enum { IDD = IDD_PCMDSD_CONVERTER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �T�|�[�g

private:
	ProgressDlg m_dProgress;
	static UINT __cdecl WorkThread(LPVOID pParam);
	// ����
protected:
	HICON m_hIcon;
	// �������ꂽ�A���b�Z�[�W���蓖�Ċ֐�
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void ListInit();
	void ListRegistDisp();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	// ���X�g�r���[�J�����f�t�H���g��
	int m_nColWidthDef[EN_LIST_COLUMN_MAX] = {
		150,				// [0]:�t�@�C����
		150,				// [1]:�^�C�g��(�g���b�N��)
		110,				// [2]:�A�[�e�B�X�g
		150,				// [3]:�A���o��
		 70,				// [4]:�g���b�NNo
		 70,				// [5]:�f�B�X�NNo
		 70,				// [6]:�f�B�X�N��
		110,				// [7]:�A���o���A�[�e�B�X�g
		100,				// [8]:�T���v�����O���[�g
		70, 				// [9]:�r�b�g��
		50, 				// [10]:����
		75, 				// [11]:�T���v����
		150,				// [12]:�t�@�C���p�X
		70					// [13]:�g���q
	};

	CIniFile m_IniFile;					// INI�t�@�C��

	CString m_strOutputPath;			// �o�͐�p�X
	int m_nDSDidx;						// DSD�T���v�����O�I��
	int m_nPrecisionIdx;				// ���x�I��
	int m_nGainLevelIdx;				// �Q�C�������I��
	int m_nGainLimitIdx;				// �Q�C�������I��
	int m_NormalizeFlag;				// �m�[�}���C�Y 0:���� 1:�L��
	int m_NormalizeFlagLast;			// �N�����̃m�[�}���C�Y 0:���� 1:�L��
	int m_CrossGainLevel;				// �Q�C�������|�����킹 0:���� 1:�L��
	int m_CrossGainLevelLast;			// �N�����̃Q�C�������|�����킹 0:���� 1:�L��
	int m_Pcm48KHzEnableDsd3MHzFlag;	// 48KHz�nPCM�́ADSD3.0MHz�Ƃ��ďo�͂���@0:���� 1:�L��
	int m_DsfOverWriteFlag;				// DSF�㏑�����[�h 0:���Ȃ� 1:����
	int m_radioGainMode;				// �N�����̃Q�C�����[�h �����݂̃Q�C�����[�h��m_radioGainModeDdv(DDV�ϐ�)���Q�Ƃ��鎖�B

	int m_DSDsamplingRateCurSel;		// DSD�ϊ��I���ʒu
	int m_nCompletePowerCrl;			// �ϊ������d������

	CString m_SrcFileName;				// �\�[�X�t�@�C����
	int m_SrcPcmSamplingrate;			// �\�[�X�T���v�����O���g��
	int m_SrcPcmBit;					// �\�[�XBIT
	unsigned __int64 m_OrigDataSize;	// �\�[�X�f�[�^�T�C�Y
	unsigned int m_DSD_Times;			// DSD�{��(64/128/256/1024/2048)

	int m_Pcm48KHzEnableDsd3MHzEnable;	// 48KHz�nPCM�́ADSD3.0MHz�Ƃ��ďo�͂���@0:���� 1:�L��
	int m_NormalizeEnable;				// �m�[�}���C�Y 0:���� 1:�L��
	int m_ClipErrNormalizeEnable;		// �m�[�}���C�Y 0:���� 1:�L�� ���N���b�v�G���[�����������ꍇ�ɋ����I�Ƀm�[�}���C�Y���s���␳����
	int m_DsfOverWriteEnable;			// DSF�㏑�����[�h 0:���� 1:�L��
	int m_SequentialProcessEnable;		// �A���f�[�^���� 0:���� 1:�L��
	int m_DsdClipOverChkEnable;			// DSD�ϊ��N���b�v�I�[�o�[�`�F�b�N 0:���� 1:�L��
	double m_DsdClipOverLimit;			// DSD�ϊ��N���b�v�I�[�o�[�ő�l

	STFLAC_COMMENT m_stFlacComm;		// FLAC COMMENT
	STALACTAG m_stAlacTag;				// ALAC TAG
	STID3TAGINFO m_stID3tag;			// ID3�^�O�o�͗p���
	UINT m_nTagMode;					// 0:TAG���� 1:FLAC TAG����
	CString m_AppName;					// �A�v���P�[�V������
	CString m_strEncodedBy;				// �G���R�[�_�[��

	double m_dVolGain;					// ���ʒ���
	double m_dGain;						// �Q�C��

	UINT64 m_nDsdClipOverCnt;			// DSD�ϊ����ɃN���b�v�I�[�o�[�����g�[�^���J�E���g 0:�G���[�Ȃ� 1�`:�N���b�v�I�[�o�[
	BOOL m_bDsdClipOver;				// ���݂̃t�@�C����DSD�ϊ����ɃN���b�v�I�[�o�[���� TRUE:�G���[ FALSE:����

	// DSD�ϊ��}�b�v
	int m_nDsdSampling44100;			// 44100 or 48000
	int m_nDsdSampling88200;			// 88200  or 96000
	int m_nDsdSampling176400;			// 176400 or 192000
	int m_nDsdSampling352800;			// 352800 or 384000
	int m_nDsdSampling705600;			// 705600 or 768000
	int m_nDsdSampling44100Diff;		// 44100 or 48000 ���ύX�`�F�b�N�p
	int m_nDsdSampling88200Diff;		// 88200  or 96000 ���ύX�`�F�b�N�p
	int m_nDsdSampling176400Diff;		// 176400 or 192000 ���ύX�`�F�b�N�p
	int m_nDsdSampling352800Diff;		// 352800 or 384000 ���ύX�`�F�b�N�p
	int m_nDsdSampling705600Diff;		// 705600 or 768000 ���ύX�`�F�b�N�p

	DWORD dwCompletionNoticeFlag;		// �����ʒm 00B:�Ȃ� 01B:����炷 10B:��ʂ𕜋A������

	RECT m_InitPos;
	int m_ShowCmd;
	int m_nWindowGetPosMode;			// �O��E�B���h�E�ʒu�̎擾���@ 0:GetSystemMetrics�ɂ�鉼�z�X�N���[���T�C�Y 1:MonitorFromRect�ɂ��߂����j�^
	BOOL m_bWindowMonitorBorder;		// �O��E�B���h�E�ʒu�����j�^�O�̏ꍇ�@0:�������Ȃ��@1:��ԋ߂����j�^�̃��j�^���ɕ␳����

//	double m_amp_peak[2];				// �U���s�[�N�l [0]:Ch L [1]:Ch R
//	double m_amp_sum[2];				// �U���ώZ�l [0]:Ch L [1]:Ch R
//	double m_amp_sum_cnt[2];			// �U���ώZ�� [0]:Ch L [1]:Ch R
	ST_AMP_POWER stAmp;					// �U���f�[�^
	ST_AMP_POWER stAmpPeakdB;			// �U���f�[�^�s�[�N�f�V�x���p
	double m_dAvgPeakdB[2];				// ���σs�[�NdB [0]:Ch L [1]:Ch R
	double m_AveragedB[3];				// ����dB [0]:Ch L [1]:Ch R [2]:Ch L+R
	double m_LimitdB;					// ����dB
	double m_DiffdB;					// ����db����

	UINT64 m_fillsize;
	CArray <ST_64BIT_DATA_INDEX> m_DataIdxArray;

	CString m_strSeqModeTempFile;		// �A���o�����s(�A���ϊ�)���̃e���|�����t�@�C��
	CString m_strSrcAlbumPath;			// �A���o�����s(�A���ϊ�)���̃\�[�X�f�B���N�g��

	CDffToDsfConv m_DffToDsfObj;		// DFF to DSF�ϊ�
	CAlacDecode m_AlacDecodeObj;		// ALAC�f�R�[�h

	int m_nListRegistCnt;				// ���X�g�R���g���[���o�^����

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

	CStringArray m_strCmdLineArray;		// �R�}���h���C��

	// Version�擾
	VS_FIXEDFILEINFO GetVersionInfo();
	//�S�Ď��s
	afx_msg void OnBnClickedAllrun();
	//�S�č폜
	afx_msg void OnBnClickedAlllistdelete();
	//���s
	afx_msg void OnBnClickedRun();
	//�폜
	afx_msg void OnBnClickedListdelete();
	//�Q��
	afx_msg void OnBnClickedPathcheck();
	//�t�@�C��/�f�B���N�g�����h���b�v&�h���b�O
	afx_msg void OnDropFiles(HDROP hDropInfo);
	//����
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	// INI�t�@�C���ۑ�
	void SaveIniFilee();
	//�T�C�Y�ύX
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//Wav�t�@�C�����h���b�v���ꂽ���̏���
	afx_msg void WAV_FileRead(TCHAR *FileName, BOOL bErrMsgEnable = TRUE);
	//�f�B���N�g���̍ċA�I����
	afx_msg void DirectoryFind(TCHAR *DirectoryPath);
	// �L���T���v�����O���[�g�`�F�b�N
	BOOL IsSamplingRate(DWORD dwSamplingRate);
	// �L��BIT�[�x�`�F�b�N
	BOOL IsBitDepth(DWORD dwBit);
	// WAV�T���v�����O���[�g�擾
	bool GeyWavSamplePerSec(TCHAR *filepath, int *pnSamplePerSec);
	//WAV�t�@�C���`�F�b�N�y�у��^�f�[�^�ǂݎ��
	afx_msg bool WAV_Metadata(TCHAR *filepath, CString *metadata);
	afx_msg bool WAV_Metadata(TCHAR *filepath, CString *metadata, int *pnSamplePerSec);
	// SONY WAVE64�T���v�����O���[�g�擾
	bool CPCMDSD_ConverterDlg::GeyWave64SamplePerSec(TCHAR *filepath, int *pnSamplePerSec);
	//SONT WAVE64(W64)�t�@�C���`�F�b�N�y�у��^�f�[�^�ǂݎ��
	afx_msg bool Wave64_Metadata(TCHAR *filepath, CString *metadata);
	afx_msg bool Wave64_Metadata(TCHAR *filepath, CString *metadata, int *pnSamplePerSec);
	//Flac�t�@�C���`�F�b�N�y�у��^�f�[�^�ǂݎ��
	afx_msg bool FLAC_Metadata(TCHAR *filepath, CString *metadata);
	// ALAC(.m4a)�t�@�C���`�F�b�N�y�у��^�f�[�^�ǂݎ��
	bool ALAC_Metadata(TCHAR *filepath, CString *metadata);
	// DFF�t�@�C���`�F�b�N�y�у��^�f�[�^�ǂݎ��
	bool DFF_Metadata(TCHAR *filepath, CString *metadata);
	//Flac�^�O�f�[�^�ǂݎ��
	afx_msg bool FLAC_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag, BOOL *pbEnable);
	//  ALAC�t�@�C���^�O�f�[�^�ǂݎ��
	bool ALAC_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag);
	//  WAV�t�@�C���^�O(RIFF)�f�[�^�ǂݎ��
	bool WAV_Tagdata(TCHAR *filepath, PSTFLAC_COMMENT pstTag, BOOL *pbEnable);
	// WAV�t�@�C���p�^�O�f�[�^�ݒ�
	afx_msg UINT SetTagdataForWav(TCHAR *filepath, int nTrackNo, int nTrackTotal, PSTID3TAGINFO pstID3v2Tag);
	// FLAC COMMENT�^�O������
	void FlacCommentInit(PSTFLAC_COMMENT pstTag, int mode);
	// ID3v2�^�O��񏉊���
	void ID3v2TagInfoInit();
	void ID3v2TagInfoClear(STID3TAGINFO *pstID3Tag, int mode = 0);
	void ID3v2TagInfoDataIdxArrayClear();
	// ID3v2�^�O�\�t�g�E�F�A�ݒ�
	void SetID3v2SoftwareTag(CString strSoftware, PSTID3TAGINFO pstID3v2Tag);
	// ID3v2�^�O�G���R�[�_�[�Ґݒ�
	void SetID3v2EncodedByTag(CString strEncodedBy, PSTID3TAGINFO pstID3v2Tag);
	// ID3v2�^�O���[�U�[��`�ݒ�
	void SetID3v2UserTag(CString strUser, PSTID3TAGINFO pstID3v2Tag);
	// ID3v2�^�O���[�U�[��`�ɃI���W�i���t�H�[�}�b�g�ݒ�
	void SetID3v2UserTagOriginalFormat(int nMode, CString strFile, DWORD dwSamplingRate, DWORD dwBitDepth, PSTID3TAGINFO pstID3v2Tag);
	// ���O�C�����[�U�[�擾
	CString GetUserName();
	// Flac�^�O��� to ID3v2�^�O���R�s�[
	UINT FlacTagToID3v2Tag(PSTFLAC_COMMENT pstFlacTag, PSTID3TAGINFO pstID3v2Tag);
	// ID3v2�^�O�T�C�Y
	afx_msg DWORD GetID3V2Length();
	// TTAG�쐬
	DWORD MakeTtag(char *strID, int nStrCode, CString strTtag, BYTE *pBuff);
	// CTAG�쐬
	DWORD MakeCtag(char *strID, CString strCtag, BYTE *pBuff);
	// APIC TAG�쐬
	DWORD MakeAPICtag(BYTE *pBuff);
	// Syncsafe Integer�T�C�Y�쐬
	afx_msg void MakeSynchsafeIntegerSize(DWORD dwSize, unsigned char size[4]);
	// �t���[���T�C�Y�擾
	void GetFrameSize(DWORD dwSize, BOOL bSynchsafeIntegerSize, unsigned char size[4]);
	//PCM-DSD�ϊ��̊Ǘ�
	afx_msg bool WAV_ConvertProc(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile, bool bLastDataFlag = true, int mode = 0);
	// PCM-DSD�ϊ�
	afx_msg bool WAV_Convert(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile);
	// PCM-DSD�A���f�[�^�ϊ�
	afx_msg bool WAV_ConvertSequential(EXT_TYPE etExtType, TCHAR *orgfilepath, TCHAR *filepath, int number, bool bDelWavFile, bool bLastDataFlag);
	// DFF-DSF�ϊ�
	afx_msg bool DFFtoDSFconvert(TCHAR *filepath);
	// DFF-DSF�ϊ��i���R�[���o�b�N�֐�
	static BOOL CALLBACK DFFtoDSFconvertProgress(UINT nProgress, void *pPrm);
	//DSDIFF�`���ŏ�������
	afx_msg bool DSD_Write(FILE *LData, FILE *RData, FILE *WriteData, int number);
	//DSF�`���ŏ�������
	afx_msg bool DSD_WriteDSF(FILE *LData, FILE *RData, INT64 iSeekOffset, FILE *WriteData, int number);
	// DSF�t�@�C��ID3v2�^�O��������
	afx_msg bool DSFID3V2Write(FILE *WriteData);
	//�ǂݏ����f�[�^����
	bool RequireWriteData(TCHAR *filepath, CString flag, TCHAR *WriteFilepath);
	bool RequireWriteData(TCHAR *filepath, CString flag, wchar_t *FileMode, FILE **WriteDatadsd, TCHAR *WriteFilepath = NULL, BOOL bOverWrite = TRUE);
	//�ꎞ�t�@�C���폜
	afx_msg bool TrushFile(TCHAR *filepath, CString flag);
	//Wav�t�@�C����64bit Float(double)�����ALR�������Ĉꎞ�t�@�C���Ƃ��ď����o��
//	afx_msg bool TmpWriteData(TCHAR *filepath, FILE *tmpl, FILE *tmpr, int Times);
	afx_msg bool TmpWriteData(EXT_TYPE etExtType, TCHAR *filepath, FILE *tmpl, FILE *tmpr, int DSD_Times, unsigned int *Times, int number);
	// �U������́@�߂�l�F�f�V�x���ϊ��l
	void PcmAnalysis(double amp_value, int ch, int nPcmSamplingRate);
	// �U���s�[�N�l�擾 �����ECh�̑傫����
	double GetPeakAmp();
	// ���σf�V�x���s�[�N�擾 �����ECh�̑傫����
	double GetAvgPeakdB();
	// ���σs�[�N�f�V�x���擾
	void CalcPeakdB();
	void CalcPeakdB(int ch);
	// ���σf�V�x���擾
	double GetAveragedB(double *dbL, double *dbR);
	// �f�V�x������{���擾
	double GetdBtoPower(double ddB);
	// �U������f�V�x���擾
	double GetPowertodB(double dAmpPower);
	//PCM-DSD�ϊ�
	afx_msg bool WAV_Filter(FILE *UpSampleData, FILE *OrigData, unsigned int Times, omp_lock_t *myLock, int *pErr, int mode = 0);
	afx_msg bool WAV_FilterLight(FILE *UpSampleData, FILE *OrigData, unsigned int Times, int *pErr, int mode = 0);
	// �I�[�o�[�T���v�����O
	bool WAV_Oversampling(FILE *UpSampleData, FILE *OrigData, unsigned int Times, omp_lock_t *myLock);
	// �ŏI�t�H���_�����擾
	CString GetLastPath(TCHAR *filepath);
	// �t�@�C�����擾
	CString GetFileName(TCHAR *filepath);
	//�t���[�Y�h�~�̂��߂ɃX���b�h�쐬
	void WorkThread();
	// �g���q��ʎ擾 
	EXT_TYPE GetExtType(CString strExt);
	// �o�̓f�B���N�g���p�A���o�����A���o���A�[�e�B�X�g��Disc NO�ݒ�
	void SetAlbumAndAlbumArtistOutputDir(CString strAlbum, CString strAlbumArtist, CString strDiscNo, CString strDiscTotal);
	// �t�@�C���A�f�B���N�g���Ɏg���Ȃ�������S�p�ϊ�
	CString ConvertInvalidFileName(CString strText);
	// �����ʒm����
	void CompletionNotice();
	// �ϊ��������̓d������
	BOOL CompletePowerControl(DWORD dwMode);
	// �t���p�X����f�B���N�g���p�X�����擾
	CString GetDirectoryPath(TCHAR *pstrText);
	// �A���o�����s�A���o���t�@�C�����擾
	int GetAlbumCount();
	// DSD�T���v�����O���[�g�I������ϊ�DSD�擾
	unsigned int GetDsdSamplingrateComboBoxToDsdTimes(int nSamplePerSec);
	//�������̓{�^���Ȃǖ�����
	void Disable();
	//�������I�������{�^���ȂǗL����
	void Enable();
	//F1�w���v������
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//FFT�v����������
	void FFTInit(fftw_plan *plan, unsigned int fftsize, int Times, double *fftin, fftw_complex *ifftout);
	void iFFTInit(fftw_plan *plan, unsigned int fftsize, int Times, fftw_complex *ifftin, double *fftout);
	// FLAC�f�R�[�h
	bool FlacDecode(TCHAR *psrcfile, TCHAR *pdecodefile, int decodefilebuffsize);

	static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
	static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
	static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

	// ALAC�f�R�[�h
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
