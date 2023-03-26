// DFFネイティブDSF変換クラス

#include "stdafx.h"
#include "DffToDsfConv.h"

#define DFF_READ_BUFF_SIZE		(BLOCK_SIZE_PER_CHANNEL * 2)

#ifdef BIT_REVERSE_HIGHT_SPEED_ENABLE
// 1BYTEのBIT反転テーブル
static UCHAR s_ucBitReverseTbl[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};
#endif

CDffToDsfConv::CDffToDsfConv()
{
	m_ulBitsPerSample = 1;
}

CDffToDsfConv::CDffToDsfConv(ULONG ulBitsPerSample)
{
	SetBitsPerSample(ulBitsPerSample);
}

CDffToDsfConv::~CDffToDsfConv()
{
}

// BitsPerSample設定
// ulBitsPerSample
//	1:LSB
//	8:MSB
void CDffToDsfConv::SetBitsPerSample(ULONG ulBitsPerSample)
{
	if (ulBitsPerSample != 8) {
		ulBitsPerSample = 1;
	}

	m_ulBitsPerSample = ulBitsPerSample;
}

// DFFヘッダー情報取得
BOOL CDffToDsfConv::GetHead(CString strDffFile, STDFFHEAD *pstHead)
{
	CFile FileDff;
	CFileException exp;
	BOOL bRet;

	memset(pstHead, NULL, sizeof(STDFFHEAD));

	// DFFオープン
	bRet = FileDff.Open(strDffFile, CFile::modeRead | CFile::typeBinary, &exp);
	if (bRet == FALSE) {
		return FALSE;
	}

	GetHead(&FileDff, pstHead);

	FileDff.Close();

	return bRet;
}

// DFFヘッダー情報取得
BOOL CDffToDsfConv::GetHead(CFile *pFileDff, STDFFHEAD *pstHead)
{
	BOOL bRet;
	int iRet;
	STCOMMONCHUNK stChunk;
	ULONGLONG ullSheekPos = 0;
	ULONGLONG ullPropRead = 0;
	ULONGLONG ullPropSize = 0;

	memset(pstHead, NULL, sizeof(STDFFHEAD));

	// DFFヘッダー読込み
	int nChunkSeq = 0;
	// 各Chunk ID読込み
	while (nChunkSeq >= 0) {
		switch (nChunkSeq) {
			case 0:						// Form DSD
				pFileDff->Read(&pstHead->stFromDsdChunk.stChunk, sizeof(STFROMDSDCHUNK));
				// ckID
				nChunkSeq = -1;
				iRet = memcmp(pstHead->stFromDsdChunk.stChunk.ckID, "FRM8", 4);
				if (iRet == 0) {
					// fromType
					iRet = memcmp(pstHead->stFromDsdChunk.fromType, "DSD ", 4);
					if (iRet == 0) {
						nChunkSeq = 1;
					}
				}
				break;
			case 1:						// Format Version
				pFileDff->Read(&pstHead->stFrmatVer, sizeof(STFORMATVERSION));
				nChunkSeq = -1;
				iRet = memcmp(pstHead->stFrmatVer.stChunk.ckID, "FVER", 4);
				if (iRet == 0) {
					nChunkSeq = 2;
				}
				break;
			case 2:						// Property
				pFileDff->Read(&pstHead->stPropChunk, sizeof(STPROPERTYCHUNK));
				nChunkSeq = -1;
				// Property Chunk:ckID
				iRet = memcmp(pstHead->stPropChunk.stChunk.ckID, "PROP", 4);
				if (iRet == 0) {
					iRet = memcmp(pstHead->stPropChunk.propType, "SND ", 4);
					if (iRet == 0) {
						nChunkSeq = 3;
					}
				}
				break;
			case 3:						// Property Chunk Data
				nChunkSeq = 4;
				ullPropSize = BYTE_SWAP_64(pstHead->stPropChunk.stChunk.ckDataSize) - 4;
				while (ullPropRead < ullPropSize) {
					ullPropRead += pFileDff->Read(&stChunk, sizeof(STCOMMONCHUNK));
					// Sample Rate Chunk:ckID
					iRet = memcmp(stChunk.ckID, "FS  ", 4);
					if (iRet == 0) {
						pstHead->stSampChunk.stChunk = stChunk;

						ullPropRead += pFileDff->Read(&pstHead->stSampChunk.sampleRate, sizeof(STSAMPLERATECHUNK) - 12);

					}
					// Channels Chunk:ckID
					iRet = memcmp(stChunk.ckID, "CHNL", 4);
					if (iRet == 0) {
						pstHead->stChChunk.stChunk = stChunk;

						ullPropRead += pFileDff->Read(&pstHead->stChChunk.numChannel, sizeof(STCHANNELSCHUNK) - 12);
					}
					// Compression Chunk:ckID
					iRet = memcmp(stChunk.ckID, "CMPR", 4);
					if (iRet == 0) {
						pstHead->stCompTypeChunk.stChunk = stChunk;

						ullPropRead += pFileDff->Read(&pstHead->stCompTypeChunk.compressionType, sizeof(STCOMPTYPECHUNK) - 12);
					}

					// Absolute Start Time Chunk:ckID
					iRet = memcmp(stChunk.ckID, "ABSS", 4);
					if (iRet == 0) {
						pstHead->stAbbsChunk.stChunk = stChunk;

						ullPropRead += pFileDff->Read(&pstHead->stAbbsChunk.hours, sizeof(STABSSCHUNK) - 12);
					}

					// Loundspeaker Configuration Chunk:ckID
					iRet = memcmp(stChunk.ckID, "LSCO", 4);
					if (iRet == 0) {
						pstHead->stLscChunk.stChunk = stChunk;

						ullPropRead += pFileDff->Read(&pstHead->stLscChunk.IsConfig, sizeof(STLSCOCHUNK) - 12);
					}
				}
				break;
			case 4:						// DSD Sound Data
				// DSD Sound Data Chunk:ckID
				pFileDff->Read(&stChunk.ckID[0], 4);
				// DSD Sound Data Chunk ID("DSD ")を検索
				while (1) {
					iRet = memcmp(stChunk.ckID, "DSD ", 4);
					if (iRet == 0) {
						pFileDff->Read(&stChunk.ckDataSize, sizeof(stChunk.ckDataSize));
						pstHead->stDsdDataChunk.stChunk = stChunk;
						pstHead->ullDsdDataSeekPos = pFileDff->GetPosition();
						nChunkSeq = 5;
						break;
					}
					stChunk.ckID[0] = stChunk.ckID[1];
					stChunk.ckID[1] = stChunk.ckID[2];
					stChunk.ckID[2] = stChunk.ckID[3];
					pFileDff->Read(&stChunk.ckID[3], 1);
					ullSheekPos = pFileDff->GetPosition();
					if (ullSheekPos >= pFileDff->GetLength()) {
						nChunkSeq = -1;
					}
				}
				break;
			case 5:					// 読込み正常終了
				nChunkSeq = -1;
				break;
			default:
				break;
		}
	}

	// リトルエンディアンに変換
	pstHead->stFromDsdChunk.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stFromDsdChunk.stChunk.ckDataSize);
	pstHead->stFrmatVer.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stFrmatVer.stChunk.ckDataSize);
	pstHead->stFrmatVer.version = BYTE_SWAP_32(pstHead->stFrmatVer.version);
	pstHead->stPropChunk.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stPropChunk.stChunk.ckDataSize);
	pstHead->stSampChunk.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stSampChunk.stChunk.ckDataSize);
	pstHead->stSampChunk.sampleRate = BYTE_SWAP_32(pstHead->stSampChunk.sampleRate);
	pstHead->stChChunk.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stChChunk.stChunk.ckDataSize);
	pstHead->stChChunk.numChannel = BYTE_SWAP_16(pstHead->stChChunk.numChannel);
	pstHead->stCompTypeChunk.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stCompTypeChunk.stChunk.ckDataSize);
	pstHead->stCompTypeChunk.compressionType = BYTE_SWAP_32(pstHead->stCompTypeChunk.compressionType);
	pstHead->stAbbsChunk.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stAbbsChunk.stChunk.ckDataSize);
	pstHead->stAbbsChunk.hours = BYTE_SWAP_16(pstHead->stAbbsChunk.hours);
	pstHead->stAbbsChunk.samples = BYTE_SWAP_32(pstHead->stAbbsChunk.samples);
	pstHead->stLscChunk.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stLscChunk.stChunk.ckDataSize);
	pstHead->stLscChunk.IsConfig = BYTE_SWAP_16(pstHead->stLscChunk.IsConfig);
	pstHead->stDsdDataChunk.stChunk.ckDataSize = BYTE_SWAP_64(pstHead->stDsdDataChunk.stChunk.ckDataSize);

	// DFFヘッダーチェック
	bRet = DffHeaderChk(pstHead);

	return bRet;
}

// ヘッダーチェック ※適当に識別子と2chくらいしか見てないが・・・
BOOL CDffToDsfConv::DffHeaderChk(STDFFHEAD *pstDffHead)
{
	int iRet;
	// Form DSD:ckID
	iRet = memcmp(pstDffHead->stFromDsdChunk.stChunk.ckID, "FRM8", 4);
	if (iRet != 0) {
		return FALSE;
	}
	// Form DSD:fromType
	iRet = memcmp(pstDffHead->stFromDsdChunk.fromType, "DSD ", 4);
	if (iRet != 0) {
		return FALSE;
	}
	// Format Version:ckID
	iRet = memcmp(pstDffHead->stFrmatVer.stChunk.ckID, "FVER", 4);
	if (iRet != 0) {
		return FALSE;
	}
	// Property Chunk:ckID
	iRet = memcmp(pstDffHead->stPropChunk.stChunk.ckID, "PROP", 4);
	if (iRet != 0) {
		return FALSE;
	}
	// Sample Rate Chunk:ckID
	iRet = memcmp(pstDffHead->stSampChunk.stChunk.ckID, "FS  ", 4);
	if (iRet != 0) {
		return FALSE;
	}
	// Channels Chunk:ckID
	iRet = memcmp(pstDffHead->stChChunk.stChunk.ckID, "CHNL", 4);
	if (iRet != 0) {
		return FALSE;
	}
	// Channels Chunk:numChannel 2ch
	if (pstDffHead->stChChunk.numChannel != 2) {
		return FALSE;
	}
	// Compression Chunk:ckID
	iRet = memcmp(pstDffHead->stCompTypeChunk.stChunk.ckID, "CMPR", 4);
	if (iRet != 0) {
		return FALSE;
	}
#if 0	// このチャンクがないDFFもあるのでチェックしない
	// Absolute Start Time Chunk:ckID
	iRet = memcmp(pstDffHead->stAbbsChunk.ckID, "ABSS", 4);
	if (iRet != 0) {
		return FALSE;
	}
#endif
#if 0	// このチャンクがないDFFもあるのでチェックしない
	// Loundspeaker Configuration Chunk:ckID
	iRet = memcmp(pstDffHead->stLscChunk.ckID, "LSCO", 4);
	if (iRet != 0) {
		return FALSE;
	}
#endif
	// DSD Sound Data Chunk:ckID
	iRet = memcmp(pstDffHead->stDsdDataChunk.stChunk.ckID, "DSD ", 4);
	if (iRet != 0) {
		return FALSE;
	}

	return TRUE;
}

// DFFをDSFに変換
BOOL CDffToDsfConv::Convert(CString strDffFile,PROGRESS_FUNC pFunc, void *pPrm)
{
	TCHAR filename[_MAX_FNAME];
	TCHAR fileext[_MAX_EXT];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];

	CString strDsfFile;
	BOOL bRet;
	
	_tsplitpath_s(strDffFile, drive, dir, filename, fileext);


	strDsfFile = drive;
	strDsfFile += dir;
	strDsfFile += filename;
	strDsfFile += _T(".dsf");

	bRet = Convert(strDffFile, strDsfFile, pFunc, pPrm);

	return bRet;
}

// DFFをDSFに変換
BOOL CDffToDsfConv::Convert(CString strDffFile, CString strDsfFile, PROGRESS_FUNC pFunc, void *pPrm)
{
	CFile FileDff;
	CFile FileDsf;
	CFileException exp;

	STDFFHEAD stDffHead;
	STDSFHEADER stDsfHead;
	UCHAR *pDffDsdBuff;
	UINT64 dwlDffDsdBuffSize;
	UINT64 uiDffSize;
	UINT64 uiDffRest;
	UINT64 uiDffRead;
	UINT64 uiProgressRest;
	UINT64 iDff;
	UCHAR *DsfL;
	UCHAR *DsfR;
	UCHAR ucDsd;
	UINT idx;
	UINT64 i;
	UINT64 uiProgress = 0;
	UINT64 uiProgressLatest = 0;

	UINT64 DSD_DataSize;
	UINT64 block_cnt;

	BOOL bRet;

	// DFFオープン
	bRet = FileDff.Open(strDffFile, CFile::modeRead | CFile::typeBinary, &exp);
	if (bRet == FALSE) {
		return FALSE;
	}

	// DFFヘッダー読込み ※DSD Dataにシークされる
	bRet = GetHead(&FileDff, &stDffHead);
	if (bRet == FALSE) {
		FileDff.Close();
		return FALSE;
	}

	// DSFオープン
	bRet = FileDsf.Open(strDsfFile, CFile::modeWrite | CFile::typeBinary | CFile::modeCreate, &exp);
	if (bRet == FALSE) {
		FileDff.Close();
		return FALSE;
	}

	uiDffSize = stDffHead.stDsdDataChunk.stChunk.ckDataSize;
	uiDffRest = uiDffSize;
	uiProgressRest = uiDffSize;

	//*** DSFヘッダ書込み ***
	// DSD chunk
	memcpy(stDsfHead.stDsdChunk.cHeader, "DSD ", 4);
	stDsfHead.stDsdChunk.uiChunkSize = CHUNK_SIZE_DSD;
	// Block Cnt = Sample ÷ 4096Byte ÷ 2Ch 切り上げ  
	block_cnt = (uiDffSize / BLOCK_SIZE_PER_CHANNEL / 2) + ((uiDffSize % BLOCK_SIZE_PER_CHANNEL / 2) != 0);
	// Data Size = Block × 4096 × ch
	DSD_DataSize = block_cnt * BLOCK_SIZE_PER_CHANNEL * 2;
	stDsfHead.stDsdChunk.uiTotalFileSize = CHUNK_SIZE_DSD + CHUNK_SIZE_FMT + CHUNK_DATA_HEADER_SIZE + DSD_DataSize;
	stDsfHead.stDsdChunk.uiPointerToMetadataChunk = 0;
	// FMT chunk
	memcpy(stDsfHead.stFmtChunk.cHeader, "fmt ", 4);
	stDsfHead.stFmtChunk.uiChunkSize = CHUNK_SIZE_FMT;
	stDsfHead.stFmtChunk.ulFormatVersion = 1;					// Format Version 1
	stDsfHead.stFmtChunk.uiFormatID = 0;						// 0(DSD raw)
	stDsfHead.stFmtChunk.ulChannelType = 2;						// STEREO
	stDsfHead.stFmtChunk.ulChannelNum = 2;						// 2Ch
	stDsfHead.stFmtChunk.ulSamplingFrequency = stDffHead.stSampChunk.sampleRate;	// DSDサンプリングレート
	stDsfHead.stFmtChunk.ulBitsPerSample = m_ulBitsPerSample;	// 1:DSD LSB or 8:DSD MSB
	stDsfHead.stFmtChunk.uiSampleCount = uiDffSize / 2 * 8;		// DSDサンプリング数
	stDsfHead.stFmtChunk.ulBlockSizePerChannel = BLOCK_SIZE_PER_CHANNEL;
	stDsfHead.stFmtChunk.ulReserved = 0;
	// DATA chunk
	memcpy(stDsfHead.stDataChunk.cHeader, "data", 4);
	stDsfHead.stDataChunk.uiChunkSize = CHUNK_DATA_HEADER_SIZE + DSD_DataSize;

	FileDsf.Write(&stDsfHead, sizeof(stDsfHead));

	//*** DSDデータ ***
	// DSF形式に変換
	// DFFはLRの配列順
	//┌─┬─┬      ┬─┬─┐
	//│L │R │・・・│L │R │
	//└─┴─┴      ┴─┴─┘
	//			↓↓↓
	// DSFは4096Block単位のLRの配列順。最後に端数が生じる場合は0x00で埋める。
	//┌─┬─┬      ┬─┬─┐┌─┬─┬      ┬─┬─┐
	//│L │L │・・・│L │L ││R │R │・・・│R │R │
	//└─┴─┴      ┴─┴─┘└─┴─┴      ┴─┴─┘
	//
	UINT64 dwlDsfBuffSize;
	DWORD dwWriteBlockCnt;
//	DWORD dwErr;
	DWORD j;


	// DFF読込みバッファ算出
	MEMORYSTATUSEX msx;
	DWORDLONG dwlFree = 0;

	memset(&msx, 0, sizeof(msx));
	msx.dwLength = sizeof(msx);
	bRet = GlobalMemoryStatusEx(&msx);
	if (bRet == TRUE) {
		// DFF読込みメモリ動的取得
		dwlDffDsdBuffSize = 0;

		// 物理空きメモリの50%を最大空きメモリとする
	//	msx.ullAvailPhys = 1024 * 1024 * 150;	// DEBUG 空きメモリ150MByte
		dwlFree = msx.ullAvailPhys / 2;

		// DFF DSDサイズより空きメモリのがある？
		if (dwlFree > (DWORDLONG)uiDffSize) {
			// 空きメモリが十分にあるならDFF DSDサイズを設定 ※一括読込みが出来る
			dwlDffDsdBuffSize = uiDffSize;
		}
		else {
			// 空きメモリがないなら、空きメモリから50%を確保
			dwlDffDsdBuffSize = DFF_READ_BUFF_SIZE * ((dwlFree / 2) / DFF_READ_BUFF_SIZE);
		}

#ifdef	_WIN64			// x86ビルド、x64ビルドどちらで行っても動作に違いはありません。
		// CFile.Readの最大読込み最大ByteがUINTなのでこれを超えてたらUINTの最大に近いDFF_READ_BUFF_SIZE最大公倍数にする
		if (dwlDffDsdBuffSize > UINT_MAX) {
			// 最大理論空きメモリ
			dwlDffDsdBuffSize = DFF_READ_BUFF_SIZE * (UINT_MAX / DFF_READ_BUFF_SIZE);
		}
#else
		// 32BITは2Gが最大なので
		if (dwlDffDsdBuffSize > UINT_MAX / 2) {
			// 最大理論空きメモリ
			dwlDffDsdBuffSize = DFF_READ_BUFF_SIZE * (UINT_MAX / 2) / DFF_READ_BUFF_SIZE);
		}
#endif
	} else {
//		dwErr = GetLastError();
		// DFF読込みメモリ固定取得
		// 8192 × 128ブロック = 1M固定
		dwlDffDsdBuffSize = DFF_READ_BUFF_SIZE * 128;
	}

	// DFF DSD読込みバッファ確保
	TRACE(_T("DFF DSD読込みバッファ:%d\n"), dwlDffDsdBuffSize);
	pDffDsdBuff = new UCHAR[dwlDffDsdBuffSize];

	// DSF DSD書込みバッファ確保
	memset(&msx, 0, sizeof(msx));
	msx.dwLength = sizeof(msx);
	bRet = GlobalMemoryStatusEx(&msx);
	if (bRet == TRUE) {
		// 物理空きメモリの50%を最大空きメモリとする
		dwlFree = msx.ullAvailPhys / 2;

		// DFF DSDサイズより空きメモリのがある？
		if (dwlFree > (DWORDLONG)DSD_DataSize / 2) {
			// 空きメモリが十分にあるならDFF DSDサイズを設定 ※一括読込みが出来る
			dwlDsfBuffSize = DSD_DataSize / 2;
		}else {
			// 空きメモリがないなら、空きメモリから25%×2chを確保
			dwlDsfBuffSize = BLOCK_SIZE_PER_CHANNEL * ((dwlFree / 4) / BLOCK_SIZE_PER_CHANNEL);
		}

#ifdef	_WIN64			// x86ビルド、x64ビルドどちらで行っても動作に違いはありません。
		// CFile.Readの最大読込み最大ByteがUINTなのでこれを超えてたらUINTの最大に近いBLOCK_SIZE_PER_CHANNEL最大公倍数の半分にする
		if (dwlDsfBuffSize > UINT_MAX) {
			// 最大理論空きメモリ
			dwlDsfBuffSize = (BLOCK_SIZE_PER_CHANNEL * (UINT_MAX / BLOCK_SIZE_PER_CHANNEL)) / 2;
		}
#else
		// 32BITは2Gが最大なので
		if (dwlDsfBuffSize > UINT_MAX / 2) {
			// 最大理論空きメモリ
			dwlDsfBuffSize = (BLOCK_SIZE_PER_CHANNEL * (UINT_MAX / 2) / BLOCK_SIZE_PER_CHANNEL)) / 2;
		}
#endif
	} else {
//		dwErr = GetLastError();
		// 1MByte固定
		dwlDsfBuffSize = (BLOCK_SIZE_PER_CHANNEL * 256) * 1;
	}

//	dwlDsfBuffSize = BLOCK_SIZE_PER_CHANNEL * 1;			// DEBUG 4096Byte
//	dwlDsfBuffSize = (BLOCK_SIZE_PER_CHANNEL * 256) * 10;	// DEBUG 10MByte

	DsfL = new UCHAR[(DWORD)dwlDsfBuffSize];
	DsfR = new UCHAR[(DWORD)dwlDsfBuffSize];

	DWORD dwStartTime;
	DWORD dwDiffTime;

	bRet = TRUE;
	idx = 0;
	while (uiDffRest > 0 && bRet == TRUE) {
		iDff = 0;
		if (uiDffRest > dwlDffDsdBuffSize) {
			uiDffRead = dwlDffDsdBuffSize;
		} else {
			uiDffRead = uiDffRest;
		}
		// DFF DSD読込み
		FileDff.Read(pDffDsdBuff, (UINT)uiDffRead);
		uiDffRest -= uiDffRead;
		dwStartTime = GetTickCount();
		for (i = 0; i < uiDffRead; i++) {
			// キャンセル
			if (bRet == FALSE) {
				break;
			}

			// 進捗
#if 1		// 64BIT整数よりdouble型のが除算が速いみたい
			double dDffSize = (double)uiDffSize;
			double dProgressRest = (double)uiProgressRest;
			double dProgress;

			dProgress = 100.0 - (dProgressRest * 100.0 / dDffSize);
			uiProgress = (UINT64)dProgress;
#else
			uiProgress = 100 - (uiProgressRest * 100 / uiDffSize);
#endif
			if (uiProgressLatest != uiProgress) {
//				TRACE(_T("変換中:%d\n"), uiProgress);
				if (pFunc != NULL) {
					bRet = pFunc((UINT)uiProgress, pPrm);
				}
				uiProgressLatest = uiProgress;
			}

			// DSDデータ
			if (m_ulBitsPerSample == 1) {
#ifdef BIT_REVERSE_HIGHT_SPEED_ENABLE
				ucDsd = s_ucBitReverseTbl[pDffDsdBuff[i]];
#else
				ucDsd = ReverseBit(pDffDsdBuff[i]);
#endif
			} else {
				ucDsd = pDffDsdBuff[i];
			}
			if (i % 2 == 0) {
				// L Ch
				DsfL[idx] = ucDsd;
			} else {
				// R Ch
				DsfR[idx] = ucDsd;
				idx++;
			}

			if (idx == dwlDsfBuffSize) {
				dwWriteBlockCnt = (DWORD)dwlDsfBuffSize / BLOCK_SIZE_PER_CHANNEL;
				for(j = 0;j < dwWriteBlockCnt;j ++){
					FileDsf.Write(&DsfL[j * BLOCK_SIZE_PER_CHANNEL], BLOCK_SIZE_PER_CHANNEL);
					FileDsf.Write(&DsfR[j * BLOCK_SIZE_PER_CHANNEL], BLOCK_SIZE_PER_CHANNEL);
				}
				idx = 0;
			}
			uiProgressRest -= 1;
		}
		dwDiffTime = GetTickCount() - dwStartTime;
		TRACE("経過時間:%d\n", dwDiffTime);
		// 余りがあるなら0x00で埋める。
		if (idx > 0) {
			memset(&DsfL[idx], 0, dwlDsfBuffSize - idx);
			memset(&DsfR[idx], 0, dwlDsfBuffSize - idx);
			dwWriteBlockCnt = ((idx / BLOCK_SIZE_PER_CHANNEL) + ((idx % BLOCK_SIZE_PER_CHANNEL > 0)));
			for(j = 0;j < dwWriteBlockCnt;j ++){
				FileDsf.Write(&DsfL[j * BLOCK_SIZE_PER_CHANNEL], BLOCK_SIZE_PER_CHANNEL);
				FileDsf.Write(&DsfR[j * BLOCK_SIZE_PER_CHANNEL], BLOCK_SIZE_PER_CHANNEL);
			}
		}
	}
	uiProgress = 100;
	if (uiProgressLatest != uiProgress) {
//		TRACE(_T("変換中:%d\n"), uiProgress);
		if (pFunc != NULL) {
			bRet = pFunc((UINT)uiProgress, pPrm);
		}
		uiProgressLatest = uiProgress;
	}

	delete[] DsfL;
	delete[] DsfR;
	delete[] pDffDsdBuff;
	FileDff.Close();
	FileDsf.Close();

	return bRet;
}

// Bit反転
#ifndef BIT_REVERSE_HIGHT_SPEED_ENABLE
UCHAR CDffToDsfConv::ReverseBit(UCHAR ucData)
{
	UCHAR bit = 0;
	UCHAR swap = 0;

	bit = (ucData >> 7) & 0x01;
	swap = bit;
	bit = (ucData >> 6) & 0x01;
	swap |= bit << 1;
	bit = (ucData >> 5) & 0x01;
	swap |= bit << 2;
	bit = (ucData >> 4) & 0x01;
	swap |= bit << 3;
	bit = (ucData >> 3) & 0x01;
	swap |= bit << 4;
	bit = (ucData >> 2) & 0x01;
	swap |= bit << 5;
	bit = (ucData >> 1) & 0x01;
	swap |= bit << 6;
	bit = (ucData) & 0x01;
	swap |= bit << 7;

	return swap;
}
#endif