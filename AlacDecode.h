#pragma once
#include "mmsystem.h"
#include "ALACDecoder.h"
#include "ALACBitUtilities.h"
#include "mp4ff.h"
#include "strconverter.h"

TCHAR* convert_from_utf8(const char* utf8_str);

// WAV�w�b�_�[
typedef struct {
	char    sRiff[4];          // RIFF�w�b�_
	DWORD   nFileSize;         // ����ȍ~�̃t�@�C���T�C�Y�i�t�@�C���T�C�Y�|�W�j
	char    sWave[4];          // WAVE�w�b�_
	char    sFmt[4];           // fmt �`�����N
	DWORD   nFmtSize;          // fmt �`�����N�̃o�C�g���B���j�APCM �Ȃ�� 16
	WAVEFORMAT  wavefmt;
	WORD    wBitsPerSample;
	char    sData[4];
	DWORD   nDataSize;
} STWAVHEADER, *PSTWAVHEADER;

// ALAC�t�H�[�}�b�g
typedef struct {
	INT nPlayTimeSec;						// ���t�Đ�(�b)
	DWORD nSampleRate;						// �T���v�����O���[�g
	DWORD nBitDepth;						// �r�b�g��
	UINT64 qwTotalSample;					// ���T���v����
}STALACFORMAT, *PSTALACFORMAT;

#define ALAC_COVER_MAX		20					// �ő�J�o�[�� ���Œ�Ǘ��Ƃ���B20������Ώ\�����낤�B0��FRONT 1�ȍ~��OTHER�Ȃ̂�3���ڈȍ~�͈Ӗ����Ȃ��C������B

// ALAC�^�O(�J�o�[�W���P�b�g)
typedef struct {
	DWORD dwPictureSize;
	BYTE *pPictureData;
}STCOVERART, *PSTCOVERART;

// ALAC�^�O
typedef struct {
	CString strTitle;						// �ȃ^�C�g��
	CString strArtist;						// �A�[�e�B�X�g
	CString strWriter;						// ��ȉ�
	CString strAlbum;						// �A���o��
	CString strAlbumArtist;					// �A���o���A�[�e�B�X�g
	CString strDate;						// ���t
	CString strTool;						// �c�[�� �����炭�G���R�[�h�\�t�g��
	CString strComment;						// �R�����g
	CString strGenre;						// �W������
	CString strTrack;						// �g���b�NNo
	CString strTotalTracks;					// �g���b�NTotal
	CString strDisc;						// Disc No
	CString strTotalDiscs;					// Disc Total
	CString strCompilation;					// �R���s���[�V�����A���o���̗L��
	CString strTempo;						// �e���|
	CString strCopyright;					// ���쌠
	STCOVERART stCover[ALAC_COVER_MAX];			// �J�o�[�W���P�b�g  0:FRONT COVER 1�ȍ~:OTHER
	DWORD nCoverCount;						// �J�o�[�W���P�b�g�� ���ő�ALAC_COVER_MAX�܂�
}STALACTAG, *PSTALACTAG;


typedef int(*ALAC_DECODE_PROGRESS_CALLBACK)(int nProgress);

class CAlacDecode
{
protected:
	ALACDecoder *m_pAlacDecoder;
	CFile m_File;
	mp4ff_callback_t m_callback;
	mp4ff_t *m_pMp4ff;

	BitBuffer        m_BitBuffer;

	UINT64	m_qwLastSample;   //�f�R�[�h�ŏI�T���v���ʒu(m_dwSkipSample ���܂܂Ȃ�)
	
	DWORD m_dwSampleRate;
	DWORD m_dwChannels;
	DWORD m_dwBitsPerSample;
	DWORD m_dwSizeSample;

	int m_nTrack;
	int m_nNumSampleId;

	STALACTAG m_stAlacTag;

	void InitTagInfo();
	void ReadTagInfo();
	void WriteWaveHeader(CFile *pWavFile, WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample);
	void WriteWaveHeader(CFile *pWavFile, WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample, DWORD nDataSize);
	void WriteWaveData(CFile *pWavFile, BYTE *pBuffer, DWORD dwSize);

public:
	CAlacDecode();
	~CAlacDecode();

	BOOL Open(CString strFile);
	void Close();
	BOOL GetFormatInfo(STALACFORMAT *pstFormat);
	void GetTagInfo(STALACTAG *pstTag);
	int WaveExport(CString strOutFile);
	int WaveExport(CString strOutFile, ALAC_DECODE_PROGRESS_CALLBACK pFunc);

};

inline UINT32 read_callback(void *user_data, void *buffer, UINT32 length)
{
	CFile *pFile = (CFile*)user_data;
	return pFile->Read(buffer, length);
}
inline UINT32 seek_callback(void *user_data, UINT64 position)
{
	CFile *pFile = (CFile*)user_data;
	return (UINT32)pFile->Seek(position, FILE_BEGIN);
}
