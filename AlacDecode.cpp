#include "stdafx.h"
#include "AlacDecode.h"

extern "C" int32_t mp4ff_meta_get_by_index_added_by_kobarin(const mp4ff_t *f, 
                                                            uint32_t index,
                                                            const char **item, 
                                                            const char **value,
                                                            uint32_t   *value_size);

CAlacDecode::CAlacDecode()
{
	int i;
	
	m_callback.read = read_callback;
    m_callback.seek = seek_callback;
    m_callback.truncate = NULL;
    m_callback.write = NULL;
    m_callback.user_data = NULL;

	m_pMp4ff = NULL;
	m_pAlacDecoder = NULL;

	for (i = 0; i < ALAC_COVER_MAX; i++) {
		m_stAlacTag.stCover[i].dwPictureSize = 0;
		m_stAlacTag.stCover[i].pPictureData = NULL;
	}
	m_stAlacTag.nCoverCount = 0;

	InitTagInfo();
}


CAlacDecode::~CAlacDecode()
{
	Close();
}

BOOL CAlacDecode::Open(CString strFile)
{
	CFileException fe;
	BOOL bRet;
	int type;

	if (m_File.m_hFile != CFile::hFileNull){
		return FALSE;
	}

	if(m_pAlacDecoder  != NULL){
		return FALSE;
	}

	m_pAlacDecoder = new ALACDecoder();

	bRet = m_File.Open(strFile, CFile::modeRead | CFile::typeBinary, &fe);
	if (bRet == FALSE) {
		return FALSE;
	}

	m_callback.user_data = &m_File;

	mp4ff_t *pMp4ff = mp4ff_open_read_metaonly(&m_callback);

	if (mp4ff_total_tracks(pMp4ff) > 0) {
		type = mp4ff_get_track_type(pMp4ff, 0);
	}
	mp4ff_close(pMp4ff);

	// ALAC以外はエラー
	if (type != 4) {
		m_File.Close();
		return FALSE;
	}

	mp4ff_t *infile;

	unsigned char *buffer;
	unsigned int buffer_size;
	int track = 0;
	buffer = NULL;
	buffer_size = 0;

	m_File.SeekToBegin();
	infile = mp4ff_open_read(&m_callback);

	if (mp4ff_get_decoder_config(infile, track, &buffer, &buffer_size) != 0) {
		mp4ff_close(infile);
		m_callback.user_data = NULL;
		return FALSE;
	}

	if (m_pAlacDecoder->Init(buffer, buffer_size) != 0) {
		free(buffer);
		mp4ff_close(infile);
		m_callback.user_data = NULL;
		return FALSE;
	}
	free(buffer);

    m_qwLastSample = mp4ff_get_track_duration(infile, 0);
	m_dwSampleRate = m_pAlacDecoder->mConfig.sampleRate;
	m_dwChannels = m_pAlacDecoder->mConfig.numChannels;
	m_dwBitsPerSample = m_pAlacDecoder->mConfig.bitDepth;
	m_dwSizeSample = m_pAlacDecoder->mConfig.frameLength;

	if (m_dwChannels != 2) {
		mp4ff_close(infile);
		m_callback.user_data = NULL;
		return FALSE;
	}

	m_nTrack = track;
	m_nNumSampleId = mp4ff_num_samples(infile, track);

	m_pMp4ff = infile;

	ReadTagInfo();

	return TRUE;
}

void CAlacDecode::Close()
{
	InitTagInfo();

	if (m_pMp4ff) {
		mp4ff_close(m_pMp4ff);
		m_pMp4ff = NULL;
	}

	if (m_File.m_hFile != CFile::hFileNull) {
		m_File.Close();
	}

	if(m_pAlacDecoder  != NULL){
		delete m_pAlacDecoder;
		m_pAlacDecoder = NULL;
	}
}

BOOL CAlacDecode::GetFormatInfo(STALACFORMAT *pstFormat)
{
	if (m_pMp4ff == NULL) {
		return FALSE;
	}

	// 演奏時間
	pstFormat->nPlayTimeSec = (INT)(m_qwLastSample / (UINT64)m_dwSampleRate);
	// サンプリングレート
	pstFormat->nSampleRate = m_dwSampleRate;
	// ビット
	pstFormat->nBitDepth = m_dwBitsPerSample;
	// サンプル数
	pstFormat->qwTotalSample = m_qwLastSample;

	return TRUE;
}

void CAlacDecode::InitTagInfo()
{
	int i;

	m_stAlacTag.strTitle = _T("");
	m_stAlacTag.strArtist = _T("");
	m_stAlacTag.strWriter = _T("");
	m_stAlacTag.strAlbum = _T("");
	m_stAlacTag.strAlbumArtist = _T("");
	m_stAlacTag.strDate = _T("");
	m_stAlacTag.strTool = _T("");
	m_stAlacTag.strComment = _T("");
	m_stAlacTag.strGenre = _T("");
	m_stAlacTag.strTrack = _T("");
	m_stAlacTag.strTotalTracks = _T("");
	m_stAlacTag.strDisc = _T("");
	m_stAlacTag.strTotalDiscs = _T("");
	m_stAlacTag.strCompilation = _T("");
	m_stAlacTag.strTempo = _T("");
	m_stAlacTag.strCopyright = _T("");

	for (i = 0; i < ALAC_COVER_MAX; i++) {
		if (m_stAlacTag.stCover[i].pPictureData != NULL) {
			delete 	m_stAlacTag.stCover[i].pPictureData;
		}
		m_stAlacTag.stCover[i].dwPictureSize = 0;
		m_stAlacTag.stCover[i].pPictureData = NULL;
	}
	m_stAlacTag.nCoverCount = 0;
}

void CAlacDecode::ReadTagInfo()
{
	char *value;
	TCHAR *tcValue;
	int32_t ret;

	InitTagInfo();

	if (!m_pMp4ff) {
		return;
	}

	ret = mp4ff_meta_get_title(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strTitle = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_artist(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strArtist = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_writer(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strWriter = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_album(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strAlbum = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_album_artist(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strAlbumArtist = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_date(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strDate = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_tool(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strTool = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_comment(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strComment = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_genre(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strGenre = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_track(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strTrack = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_totaltracks(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strTotalTracks = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_disc(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strDisc = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_totaldiscs(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strTotalDiscs = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_compilation(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strCompilation = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_tempo(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strTempo = tcValue;
		free(tcValue);
		free(value);
	}

	ret = mp4ff_meta_get_copyright(m_pMp4ff, &value);
	if (ret) {
		tcValue = convert_from_utf8(value);
		m_stAlacTag.strCopyright = tcValue;
		free(tcValue);
		free(value);
	}
#if 0
	ret = mp4ff_meta_get_coverart(m_pMp4ff, &value);
	if (ret) {
		m_stAlacTag.stCover[0].dwPictureSize = ret;
		m_stAlacTag.stCover[0].pPictureData = new BYTE[ret];
		memcpy(m_stAlacTag.stCover[0].pPictureData, value, ret);
		free(value);
	}
#endif

	const int nCount = mp4ff_meta_get_num_items(m_pMp4ff);
	int i;
	int nCoverCnt = 0;
	const char *item, *constvalue;
	uint32_t value_size;
	for (i = 0; i < nCount; i++) {
		// ALAC_COVER_MAX以上は保存出来ないから読み捨て
		if (nCoverCnt >= ALAC_COVER_MAX) {
			break;
		}
		if (!mp4ff_meta_get_by_index_added_by_kobarin(m_pMp4ff, i, &item, &constvalue, &value_size)) {
			break;
		}
		if (_stricmp(item, "cover") == 0) {
			m_stAlacTag.stCover[nCoverCnt].dwPictureSize = value_size;
			m_stAlacTag.stCover[nCoverCnt].pPictureData = new BYTE[value_size];
			memcpy(m_stAlacTag.stCover[nCoverCnt].pPictureData, constvalue, value_size);

			nCoverCnt++;
			m_stAlacTag.nCoverCount = nCoverCnt;
		}
	}
}

// ALACタグ取得
void CAlacDecode::GetTagInfo(STALACTAG *pstTag)
{
	*pstTag = m_stAlacTag;
}

// 戻り値	0:エラー
//			1:正常
//			2:キャンセル
int CAlacDecode::WaveExport(CString strOutFile)
{
	return WaveExport(strOutFile, NULL);
}

// 戻り値	0:エラー
//			1:正常
//			2:キャンセル
int CAlacDecode::WaveExport(CString strOutFile, ALAC_DECODE_PROGRESS_CALLBACK pFunc)
{
	BYTE *buffer = NULL;
	UINT buffer_size = 0;
	INT nCurrentSampleId;

	BYTE *pBuffer;
	DWORD dwBuffSize;
	DWORD dwCurrentSample = 0;
	DWORD dwBytesPerFrame;
	DWORD dwWriteSize;
	DWORD dwTotalDataSize;
	DWORD dwSample = 0;

	CFile OutFile;
	CFileException fe;
	BOOL bRet;
	int nRet;

	int nProgress = 0;
	int nProgressLatest = 0;

	nRet = 1;

	if (m_pMp4ff == NULL) {
		return 0;
	}

	if (pFunc) {
		pFunc(nProgress);
	}

	bRet = OutFile.Open(strOutFile, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &fe);
	if(bRet == FALSE){
		return 0;
	}
	// WAVヘッダー書込み
	dwTotalDataSize = 0;
	WriteWaveHeader(&OutFile, (WORD)m_dwChannels, m_dwSampleRate, (WORD)m_dwBitsPerSample);

	dwBytesPerFrame = m_dwChannels * ((m_dwBitsPerSample + (m_dwBitsPerSample % 8)) / 8);
	dwBuffSize = m_dwSizeSample * dwBytesPerFrame;
	pBuffer = new BYTE[dwBuffSize];

	nCurrentSampleId = 0;

	while(nCurrentSampleId < m_nNumSampleId){
		// データ読込んでデコード
		buffer_size = 0;
		int rc = mp4ff_read_sample(m_pMp4ff, m_nTrack, nCurrentSampleId, &buffer, &buffer_size);
		nCurrentSampleId ++;
		if (rc == 0) {
			OutFile.Close();
			return 0;
		}
		BitBufferInit(&m_BitBuffer, buffer, buffer_size);
		dwSample = 0;
		int ret = m_pAlacDecoder->Decode(&m_BitBuffer, pBuffer, m_dwSizeSample, m_dwChannels, (uint32_t*)&dwSample);
		if (ret != 0) {
			free(buffer);
			OutFile.Close();
			return 0;
		}
		dwWriteSize = dwSample * dwBytesPerFrame;
		// WAV書込み
		WriteWaveData(&OutFile, pBuffer, dwWriteSize);
		dwTotalDataSize += dwWriteSize;

		// 初期化
		BitBufferReset(&m_BitBuffer);
		free(buffer);
		buffer = NULL;

		if (pFunc) {
			// 進捗計算
			nProgress = (nCurrentSampleId * 100) / m_nNumSampleId;
			if (nProgressLatest != nProgress) {
				// 進捗通知
				bRet = pFunc(nProgress);
				if (bRet == FALSE) {
					nRet = 2;
					break;
				}
				nProgressLatest = nProgress;
			}
		}

		// 念の為の終了チェック
		dwCurrentSample += dwSample;
		if(dwCurrentSample > m_qwLastSample){
			break;
		}
	}

	// WAVヘッダー更新
	OutFile.SeekToBegin();
	WriteWaveHeader(&OutFile, (WORD)m_dwChannels, m_dwSampleRate, (WORD)m_dwBitsPerSample, dwTotalDataSize);

	delete pBuffer;

	OutFile.Close();

	return nRet;
}

void CAlacDecode::WriteWaveHeader(CFile *pWavFile, WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample)
{
	WriteWaveHeader(pWavFile, nChannels, nSamplesPerSec, wBitsPerSample, 0);
}

void CAlacDecode::WriteWaveHeader(CFile *pWavFile, WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample, DWORD nDataSize)
{	
	STWAVHEADER stWh;

	memset(&stWh, NULL, sizeof(stWh));

	stWh.sRiff[0] = 'R';
	stWh.sRiff[1] = 'I';
	stWh.sRiff[2] = 'F';
	stWh.sRiff[3] = 'F';

	stWh.nFileSize = nDataSize + sizeof(STWAVHEADER) - 8;

	stWh.sWave[0] = 'W';
	stWh.sWave[1] = 'A';
	stWh.sWave[2] = 'V';
	stWh.sWave[3] = 'E';

	stWh.sFmt[0] = 'f';
	stWh.sFmt[1] = 'm';
	stWh.sFmt[2] = 't';
	stWh.sFmt[3] = ' ';

	stWh.nFmtSize = 16;
	stWh.wavefmt.wFormatTag = 1;
	stWh.wavefmt.nChannels = nChannels;
	stWh.wavefmt.nSamplesPerSec = nSamplesPerSec;
	stWh.wavefmt.nAvgBytesPerSec = nChannels * nSamplesPerSec * ((wBitsPerSample + (wBitsPerSample % 8))  / 8);
	stWh.wavefmt.nBlockAlign = ((wBitsPerSample + (wBitsPerSample % 8)) / 8) * nChannels;
	stWh.wBitsPerSample = wBitsPerSample;
	stWh.sData[0] = 'd';
	stWh.sData[1] = 'a';
	stWh.sData[2] = 't';
	stWh.sData[3] = 'a';
	stWh.nDataSize = nDataSize;

	pWavFile->Write(&stWh, sizeof(stWh));
}

void CAlacDecode::WriteWaveData(CFile *pWavFile, BYTE *pBuffer, DWORD dwSize)
{
	pWavFile->Write(pBuffer, dwSize);
}
