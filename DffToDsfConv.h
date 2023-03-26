#pragma once
// DFFネイティブDSF変換クラス

#define BIT_REVERSE_HIGHT_SPEED_ENABLE	// 高速化有効。予めBYTE反転テーブルを用意しておいてそれで反転する。

#define _BYTE1(x) (  x        & 0xFF )
#define _BYTE2(x) ( (x >>  8) & 0xFF )
#define _BYTE3(x) ( (x >> 16) & 0xFF )
#define _BYTE4(x) ( (x >> 24) & 0xFF )
#define _BYTE5(x) ( (x >> 32) & 0xFF )
#define _BYTE6(x) ( (x >> 40) & 0xFF )
#define _BYTE7(x) ( (x >> 48) & 0xFF )
#define _BYTE8(x) ( (x >> 56) & 0xFF )

#define BYTE_SWAP_16(x) ((USHORT)( _BYTE1(x)<<8 | _BYTE2(x) ))
#define BYTE_SWAP_32(x) ((UINT)( _BYTE1(x)<<24 | _BYTE2(x)<<16 | _BYTE3(x)<<8 | _BYTE4(x) ))
#define BYTE_SWAP_64(x) ((UINT)( _BYTE1(x)<<56 | _BYTE2(x)<<48 | _BYTE3(x)<<40 | _BYTE4(x)<<32 | _BYTE5(x)<<24 | _BYTE6(x)<<16 | _BYTE7(x)<<8 | _BYTE8(x) ))

#pragma pack(push, 1)

//*** DFFヘッダー構造体 ***
// 共通Chunk
typedef struct
{
	UCHAR ckID[4];
	UINT64 ckDataSize;
}STCOMMONCHUNK,*PSTCOMMONCHUNK;

// Form DSD Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
	UCHAR fromType[4];
}STFROMDSDCHUNK,*PSTFROMDSDCHUNK;

// Format Version Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
	DWORD version;
}STFORMATVERSION,*PSTFORMATVERSION;

// Property Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
	UCHAR propType[4];
}STPROPERTYCHUNK,*PSTPROPERTYCHUNK;

// Property Data Chunk
// Sample Rate Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
	DWORD sampleRate;
}STSAMPLERATECHUNK,*PSTSAMPLERATECHUNK;

// Property Data Chunk
// Channels Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
	USHORT numChannel;	// 2chのみとし
	UCHAR chID[8];		// SLFTSRGT固定とする。本当ならnumChannel×4Byteの可変長データです。
}STCHANNELSCHUNK,*PSTCHANNELSCHUNK;

// Property Data Chunk
// Compression Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
	DWORD compressionType;
	UCHAR Count;					// 14
	UCHAR compressionName[14 + 1];	// "not compressed "
}STCOMPTYPECHUNK,*PSTCOMPTYPECHUNK;

// Property Data Chunk
// Absolute Start Time Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
	USHORT hours;
	UCHAR minutes;
	UCHAR seconds;
	DWORD samples;
}STABSSCHUNK,*PSTABSSCHUNK;

// Property Data Chunk
// Absolute Start Time Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
	USHORT IsConfig;
}STLSCOCHUNK,*PSTLSCOCHUNK;

// Property Data Chunk
// Loundspeaker Configuration Chunk
typedef struct
{
	STCOMMONCHUNK stChunk;
}STDSDSOUNDDATACHUNK,*PSTDSDSOUNDDATACHUNK;

// DFF HEADER情報
typedef struct
{
	STFROMDSDCHUNK stFromDsdChunk;
	STFORMATVERSION stFrmatVer;
	STPROPERTYCHUNK stPropChunk;
	STSAMPLERATECHUNK stSampChunk;
	STCHANNELSCHUNK stChChunk;
	STCOMPTYPECHUNK stCompTypeChunk;
	STABSSCHUNK stAbbsChunk;
	STLSCOCHUNK stLscChunk;
	STDSDSOUNDDATACHUNK stDsdDataChunk;
	ULONGLONG ullDsdDataSeekPos;
}STDFFHEAD, *PSTDFFHEAD;

//*** DSFヘッダー構造体 ***
// 基本的にリトルエンディアン
#define CHUNK_SIZE_DSD				28
#define CHUNK_SIZE_FMT				52
#define CHUNK_DATA_HEADER_SIZE		12
#define BLOCK_SIZE_PER_CHANNEL		4096

// DSD chunk構造体
typedef struct
{
	char cHeader[4];					// HEADER(ASC II"DSD ")
	UINT64	uiChunkSize;				// Chunk Size(28Byte) ※CHUNK_SIZE_DSD
	UINT64	uiTotalFileSize;			// ファイルサイズ
	UINT64	uiPointerToMetadataChunk;	// メタデータ(ID3v2タグ)位置
}STDSDCHUNK, *PSTDSDCHUNK;

// FMT chunk
typedef struct
{
	char cHeader[4];					// HEADER(ASC II"fmt ")
	UINT64	uiChunkSize;				// Chunk Size(52Byte) ※CHUNK_SIZE_FMT
	ULONG	ulFormatVersion;			// Format Version(1)
	ULONG	uiFormatID;					// Format ID(0:DSD raw)
	ULONG	ulChannelType;				// Channelタイプ
	ULONG	ulChannelNum;				// Channel数
	ULONG	ulSamplingFrequency;		// サンプリング周波数(Hz) 2822400, 5644800,･･･
	ULONG	ulBitsPerSample;			// Bits Per Sample(1:DSD LSB or 8:DSD MSB)
	UINT64	uiSampleCount;				// Sample count ※1chのサンプリング周波数 × n秒
	ULONG	ulBlockSizePerChannel;		// Block size per channel(4096)　※BLOCK_SIZE_PER_CHANNEL
	ULONG	ulReserved;					// 予備
}STFMTCHUNK, *PSTFMTCHUNK;

// DSD Data chunk
typedef struct
{
	char cHeader[4];					// HEADER(ASC II"data")
	UINT64	uiChunkSize;				// Chunk Size　※CHUNK_DATA_HEADER_SIZE + Sample DataのByte数
}STDATACHUNK, *PSTDATACHUNK;

// DSFヘッダー情報
typedef struct
{
	STDSDCHUNK stDsdChunk;
	STFMTCHUNK stFmtChunk;
	STDATACHUNK stDataChunk;
}STDSFHEADER, *PSTDSFHEADER;
#pragma pack(pop)

typedef BOOL(*PROGRESS_FUNC)(UINT, void *);

class CDffToDsfConv
{
protected:
	ULONG m_ulBitsPerSample;			// BitsPerSample:1(LSB) or 8(MSB)

	BOOL GetHead(CFile *pFileDff, STDFFHEAD *pstHead);
	BOOL DffHeaderChk(STDFFHEAD *pstDffHead);
#ifndef BIT_REVERSE_HIGHT_SPEED_ENABLE
	UCHAR ReverseBit(UCHAR ucData);
#endif

public:
	CDffToDsfConv();
	CDffToDsfConv(ULONG ulBitsPerSample);
	~CDffToDsfConv();

	BOOL GetHead(CString strDffFile, STDFFHEAD *pstHead);
	void SetBitsPerSample(ULONG ulBitsPerSample);

	BOOL Convert(CString strDffFile, PROGRESS_FUNC pFunc = NULL, void *pPrm = NULL);
	BOOL Convert(CString strDffFile, CString strDsfFile, PROGRESS_FUNC pFunc = NULL, void *pPrm = NULL);
};
