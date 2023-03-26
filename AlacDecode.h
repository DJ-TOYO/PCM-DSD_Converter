#pragma once
#include "mmsystem.h"
#include "ALACDecoder.h"
#include "ALACBitUtilities.h"
#include "mp4ff.h"
#include "strconverter.h"

TCHAR* convert_from_utf8(const char* utf8_str);

// WAVヘッダー
typedef struct {
	char    sRiff[4];          // RIFFヘッダ
	DWORD   nFileSize;         // これ以降のファイルサイズ（ファイルサイズ－８）
	char    sWave[4];          // WAVEヘッダ
	char    sFmt[4];           // fmt チャンク
	DWORD   nFmtSize;          // fmt チャンクのバイト数。リニアPCM ならば 16
	WAVEFORMAT  wavefmt;
	WORD    wBitsPerSample;
	char    sData[4];
	DWORD   nDataSize;
} STWAVHEADER, *PSTWAVHEADER;

// ALACフォーマット
typedef struct {
	INT nPlayTimeSec;						// 演奏再生(秒)
	DWORD nSampleRate;						// サンプリングレート
	DWORD nBitDepth;						// ビット数
	UINT64 qwTotalSample;					// 総サンプル数
}STALACFORMAT, *PSTALACFORMAT;

#define ALAC_COVER_MAX		20					// 最大カバー数 ※固定管理とする。20枚あれば十分だろう。0がFRONT 1以降がOTHERなので3枚目以降は意味がない気がする。

// ALACタグ(カバージャケット)
typedef struct {
	DWORD dwPictureSize;
	BYTE *pPictureData;
}STCOVERART, *PSTCOVERART;

// ALACタグ
typedef struct {
	CString strTitle;						// 曲タイトル
	CString strArtist;						// アーティスト
	CString strWriter;						// 作曲家
	CString strAlbum;						// アルバム
	CString strAlbumArtist;					// アルバムアーティスト
	CString strDate;						// 日付
	CString strTool;						// ツール ※恐らくエンコードソフト名
	CString strComment;						// コメント
	CString strGenre;						// ジャンル
	CString strTrack;						// トラックNo
	CString strTotalTracks;					// トラックTotal
	CString strDisc;						// Disc No
	CString strTotalDiscs;					// Disc Total
	CString strCompilation;					// コンピレーションアルバムの有無
	CString strTempo;						// テンポ
	CString strCopyright;					// 著作権
	STCOVERART stCover[ALAC_COVER_MAX];			// カバージャケット  0:FRONT COVER 1以降:OTHER
	DWORD nCoverCount;						// カバージャケット数 ※最大ALAC_COVER_MAXまで
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

	UINT64	m_qwLastSample;   //デコード最終サンプル位置(m_dwSkipSample を含まない)
	
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
