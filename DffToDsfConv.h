#pragma once
// DFF�l�C�e�B�uDSF�ϊ��N���X

#define BIT_REVERSE_HIGHT_SPEED_ENABLE	// �������L���B�\��BYTE���]�e�[�u����p�ӂ��Ă����Ă���Ŕ��]����B

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

//*** DFF�w�b�_�[�\���� ***
// ����Chunk
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
	USHORT numChannel;	// 2ch�݂̂Ƃ�
	UCHAR chID[8];		// SLFTSRGT�Œ�Ƃ���B�{���Ȃ�numChannel�~4Byte�̉ϒ��f�[�^�ł��B
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

// DFF HEADER���
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

//*** DSF�w�b�_�[�\���� ***
// ��{�I�Ƀ��g���G���f�B�A��
#define CHUNK_SIZE_DSD				28
#define CHUNK_SIZE_FMT				52
#define CHUNK_DATA_HEADER_SIZE		12
#define BLOCK_SIZE_PER_CHANNEL		4096

// DSD chunk�\����
typedef struct
{
	char cHeader[4];					// HEADER(ASC II"DSD ")
	UINT64	uiChunkSize;				// Chunk Size(28Byte) ��CHUNK_SIZE_DSD
	UINT64	uiTotalFileSize;			// �t�@�C���T�C�Y
	UINT64	uiPointerToMetadataChunk;	// ���^�f�[�^(ID3v2�^�O)�ʒu
}STDSDCHUNK, *PSTDSDCHUNK;

// FMT chunk
typedef struct
{
	char cHeader[4];					// HEADER(ASC II"fmt ")
	UINT64	uiChunkSize;				// Chunk Size(52Byte) ��CHUNK_SIZE_FMT
	ULONG	ulFormatVersion;			// Format Version(1)
	ULONG	uiFormatID;					// Format ID(0:DSD raw)
	ULONG	ulChannelType;				// Channel�^�C�v
	ULONG	ulChannelNum;				// Channel��
	ULONG	ulSamplingFrequency;		// �T���v�����O���g��(Hz) 2822400, 5644800,���
	ULONG	ulBitsPerSample;			// Bits Per Sample(1:DSD LSB or 8:DSD MSB)
	UINT64	uiSampleCount;				// Sample count ��1ch�̃T���v�����O���g�� �~ n�b
	ULONG	ulBlockSizePerChannel;		// Block size per channel(4096)�@��BLOCK_SIZE_PER_CHANNEL
	ULONG	ulReserved;					// �\��
}STFMTCHUNK, *PSTFMTCHUNK;

// DSD Data chunk
typedef struct
{
	char cHeader[4];					// HEADER(ASC II"data")
	UINT64	uiChunkSize;				// Chunk Size�@��CHUNK_DATA_HEADER_SIZE + Sample Data��Byte��
}STDATACHUNK, *PSTDATACHUNK;

// DSF�w�b�_�[���
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
