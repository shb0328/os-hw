/**
 *  file    FileSystem.h
 *  date    2009/05/01
 *  author  kkamagui 
 *          Copyright(c)2008 All rights reserved by kkamagui
 *  brief  ÆÄÀÏ œÃœºÅÛ¿¡ °ü·ÃµÈ ÇìŽõ ÆÄÀÏ
 */

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

////////////////////////////////////////////////////////////////////////////////
//
// žÅÅ©·Î¿Í ÇÔŒö Æ÷ÀÎÅÍ
//
////////////////////////////////////////////////////////////////////////////////
// MINT ÆÄÀÏ œÃœºÅÛ œÃ±×³ÊÃ³(Signature)
#define FILESYSTEM_SIGNATURE                0x7E38CF10
// Å¬·¯œºÅÍÀÇ Å©±â(ŒœÅÍ Œö), 4Kbyte
#define FILESYSTEM_SECTORSPERCLUSTER        8
// ÆÄÀÏ Å¬·¯œºÅÍÀÇ ž¶Áöž· Ç¥œÃ
#define FILESYSTEM_LASTCLUSTER              0xFFFFFFFF
// ºó Å¬·¯œºÅÍ Ç¥œÃ
#define FILESYSTEM_FREECLUSTER              0x00
// ·çÆ® µð·ºÅÍž®¿¡ ÀÖŽÂ ÃÖŽë µð·ºÅÍž® ¿£Æ®ž®ÀÇ Œö
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT   ( ( FILESYSTEM_SECTORSPERCLUSTER * 512 ) / \
        sizeof( DIRECTORYENTRY ) )
// Å¬·¯œºÅÍÀÇ Å©±â(¹ÙÀÌÆ® Œö)
#define FILESYSTEM_CLUSTERSIZE              ( FILESYSTEM_SECTORSPERCLUSTER * 512 )

// ÇÚµéÀÇ ÃÖŽë °³Œö, ÃÖŽë ÅÂœºÅ© ŒöÀÇ 3¹è·Î »ýŒº
#define FILESYSTEM_HANDLE_MAXCOUNT          ( TASK_MAXCOUNT * 3 )

// ÆÄÀÏ ÀÌž§ÀÇ ÃÖŽë ±æÀÌ
#define FILESYSTEM_MAXFILENAMELENGTH        24

// ÇÚµéÀÇ ÅžÀÔÀ» Á€ÀÇ
#define FILESYSTEM_TYPE_FREE                0
#define FILESYSTEM_TYPE_FILE                1
#define FILESYSTEM_TYPE_DIRECTORY           2

// SEEK ¿ÉŒÇ Á€ÀÇ
#define FILESYSTEM_SEEK_SET                 0
#define FILESYSTEM_SEEK_CUR                 1
#define FILESYSTEM_SEEK_END                 2

// ÇÏµå µðœºÅ© ÁŠŸî¿¡ °ü·ÃµÈ ÇÔŒö Æ÷ÀÎÅÍ ÅžÀÔ Á€ÀÇ
typedef BOOL (* fReadHDDInformation ) ( BOOL bPrimary, BOOL bMaster, 
        HDDINFORMATION* pstHDDInformation );
typedef int (* fReadHDDSector ) ( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, 
        int iSectorCount, char* pcBuffer );
typedef int (* fWriteHDDSector ) ( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, 
        int iSectorCount, char* pcBuffer );

// MINT ÆÄÀÏ œÃœºÅÛ ÇÔŒöžŠ Ç¥ÁØ ÀÔÃâ·Â ÇÔŒö ÀÌž§Àž·Î ÀçÁ€ÀÇ
#define fopen       kOpenFile
#define fread       kReadFile
#define fwrite      kWriteFile
#define fseek       kSeekFile
#define fclose      kCloseFile
#define remove      kRemoveFile
#define opendir     kOpenDirectory
#define readdir     kReadDirectory
#define rewinddir   kRewindDirectory
#define closedir    kCloseDirectory

// MINT ÆÄÀÏ œÃœºÅÛ žÅÅ©·ÎžŠ Ç¥ÁØ ÀÔÃâ·ÂÀÇ žÅÅ©·ÎžŠ ÀçÁ€ÀÇ
#define SEEK_SET    FILESYSTEM_SEEK_SET
#define SEEK_CUR    FILESYSTEM_SEEK_CUR
#define SEEK_END    FILESYSTEM_SEEK_END

// MINT ÆÄÀÏ œÃœºÅÛ ÅžÀÔ°ú ÇÊµåžŠ Ç¥ÁØ ÀÔÃâ·ÂÀÇ ÅžÀÔÀž·Î ÀçÁ€ÀÇ
#define size_t      DWORD       
#define dirent      kDirectoryEntryStruct
#define d_name      vcFileName

////////////////////////////////////////////////////////////////////////////////
//
// ±žÁ¶ÃŒ
//
////////////////////////////////////////////////////////////////////////////////
// 1¹ÙÀÌÆ®·Î Á€·Ä
#pragma pack( push, 1 )

// ÆÄÆŒŒÇ ÀÚ·á±žÁ¶
typedef struct kPartitionStruct
{
    // ºÎÆÃ °¡ŽÉ ÇÃ·¡±×. 0x80ÀÌžé ºÎÆÃ °¡ŽÉÀ» ³ªÅž³»žç 0x00Àº ºÎÆÃ ºÒ°¡
    BYTE bBootableFlag;
    // ÆÄÆŒŒÇÀÇ œÃÀÛ Ÿîµå·¹œº. ÇöÀçŽÂ °ÅÀÇ »ç¿ëÇÏÁö ŸÊÀžžç ŸÆ·¡ÀÇ LBA Ÿîµå·¹œºžŠ ŽëœÅ »ç¿ë
    BYTE vbStartingCHSAddress[ 3 ];
    // ÆÄÆŒŒÇ ÅžÀÔ
    BYTE bPartitionType;
    // ÆÄÆŒŒÇÀÇ ž¶Áöž· Ÿîµå·¹œº. ÇöÀçŽÂ °ÅÀÇ »ç¿ë ŸÈ ÇÔ
    BYTE vbEndingCHSAddress[ 3 ];
    // ÆÄÆŒŒÇÀÇ œÃÀÛ Ÿîµå·¹œº. LBA Ÿîµå·¹œº·Î ³ªÅž³œ °ª
    DWORD dwStartingLBAAddress;
    // ÆÄÆŒŒÇ¿¡ Æ÷ÇÔµÈ ŒœÅÍ Œö
    DWORD dwSizeInSector;
} PARTITION;


// MBR ÀÚ·á±žÁ¶
typedef struct kMBRStruct
{
    // ºÎÆ® ·ÎŽõ ÄÚµå°¡ À§Ä¡ÇÏŽÂ ¿µ¿ª
    BYTE vbBootCode[ 430 ];

    // ÆÄÀÏ œÃœºÅÛ œÃ±×³ÊÃ³, 0x7E38CF10
    DWORD dwSignature;
    // ¿¹ŸàµÈ ¿µ¿ªÀÇ ŒœÅÍ Œö
    DWORD dwReservedSectorCount;
    // Å¬·¯œºÅÍ žµÅ© Å×ÀÌºí ¿µ¿ªÀÇ ŒœÅÍ Œö
    DWORD dwClusterLinkSectorCount;
    // Å¬·¯œºÅÍÀÇ ÀüÃŒ °³Œö
    DWORD dwTotalClusterCount;
    
    // ÆÄÆŒŒÇ Å×ÀÌºí
    PARTITION vstPartition[ 4 ];
    
    // ºÎÆ® ·ÎŽõ œÃ±×³ÊÃ³, 0x55, 0xAA
    BYTE vbBootLoaderSignature[ 2 ];
} MBR;

// 디렉터리 엔트리 자료구조
typedef struct kDirectoryEntryStruct   //////////
{
    
    int flag;
   
    char vcFileName[ FILESYSTEM_MAXFILENAMELENGTH ];
    // 파일의 실제 크기
    DWORD dwFileSize;
    // 파일이 시작하는 클러스터 인덱스
    DWORD dwStartClusterIndex;

    
    char ParentDirectoryPath[FILESYSTEM_MAXFILENAMELENGTH];
    DWORD ParentDirectoryCluserIndex;
	
        BYTE bSecond, bMinute, bHour;
	BYTE bDayOfWeek, bDayOfMonth, bMonth;
	WORD wYear;
	
  int f;

} DIRECTORYENTRY;

#pragma pack( pop )

// ÆÄÀÏÀ» °üž®ÇÏŽÂ ÆÄÀÏ ÇÚµé ÀÚ·á±žÁ¶
typedef struct kFileHandleStruct
{
    // ÆÄÀÏÀÌ ÁžÀçÇÏŽÂ µð·ºÅÍž® ¿£Æ®ž®ÀÇ ¿ÀÇÁŒÂ
    int iDirectoryEntryOffset;
    // ÆÄÀÏ Å©±â
    DWORD dwFileSize;
    // ÆÄÀÏÀÇ œÃÀÛ Å¬·¯œºÅÍ ÀÎµŠœº
    DWORD dwStartClusterIndex;
    // ÇöÀç I/O°¡ ŒöÇàÁßÀÎ Å¬·¯œºÅÍÀÇ ÀÎµŠœº
    DWORD dwCurrentClusterIndex;
    // ÇöÀç Å¬·¯œºÅÍÀÇ ¹Ù·Î ÀÌÀü Å¬·¯œºÅÍÀÇ ÀÎµŠœº
    DWORD dwPreviousClusterIndex;
    // ÆÄÀÏ Æ÷ÀÎÅÍÀÇ ÇöÀç À§Ä¡
    DWORD dwCurrentOffset;
} FILEHANDLE;

// µð·ºÅÍž®žŠ °üž®ÇÏŽÂ µð·ºÅÍž® ÇÚµé ÀÚ·á±žÁ¶
typedef struct kDirectoryHandleStruct
{
    // ·çÆ® µð·ºÅÍž®žŠ ÀúÀåÇØµÐ ¹öÆÛ
    DIRECTORYENTRY* pstDirectoryBuffer;
    
    // µð·ºÅÍž® Æ÷ÀÎÅÍÀÇ ÇöÀç À§Ä¡
    int iCurrentOffset;
} DIRECTORYHANDLE;

// ÆÄÀÏ°ú µð·ºÅÍž®¿¡ ŽëÇÑ Á€ºž°¡ µéŸîÀÖŽÂ ÀÚ·á±žÁ¶
typedef struct kFileDirectoryHandleStruct
{
    // ÀÚ·á±žÁ¶ÀÇ ÅžÀÔ Œ³Á€. ÆÄÀÏ ÇÚµéÀÌ³ª µð·ºÅÍž® ÇÚµé, ¶ÇŽÂ ºó ÇÚµé ÅžÀÔ ÁöÁ€ °¡ŽÉ
    BYTE bType;

    // bTypeÀÇ °ª¿¡ µû¶ó ÆÄÀÏ ¶ÇŽÂ µð·ºÅÍž®·Î »ç¿ë
    union
    {
        // ÆÄÀÏ ÇÚµé
        FILEHANDLE stFileHandle;
        // µð·ºÅÍž® ÇÚµé
        DIRECTORYHANDLE stDirectoryHandle;
    };    
} FILE, DIR;

// ÆÄÀÏ œÃœºÅÛÀ» °üž®ÇÏŽÂ ±žÁ¶ÃŒ
typedef struct kFileSystemManagerStruct
{
    // ÆÄÀÏ œÃœºÅÛÀÌ Á€»óÀûÀž·Î ÀÎœÄµÇŸúŽÂÁö ¿©ºÎ
    BOOL bMounted;
    
    // °¢ ¿µ¿ªÀÇ ŒœÅÍ Œö¿Í œÃÀÛ LBA Ÿîµå·¹œº
    DWORD dwReservedSectorCount;
    DWORD dwClusterLinkAreaStartAddress;
    DWORD dwClusterLinkAreaSize;
    DWORD dwDataAreaStartAddress;   
    // µ¥ÀÌÅÍ ¿µ¿ªÀÇ Å¬·¯œºÅÍÀÇ ÃÑ °³Œö
    DWORD dwTotalClusterCount;
    
    // ž¶Áöž·Àž·Î Å¬·¯œºÅÍžŠ ÇÒŽçÇÑ Å¬·¯œºÅÍ žµÅ© Å×ÀÌºíÀÇ ŒœÅÍ ¿ÀÇÁŒÂÀ» ÀúÀå
    DWORD dwLastAllocatedClusterLinkSectorOffset;
    
    // ÆÄÀÏ œÃœºÅÛ µ¿±âÈ­ °ŽÃŒ
    MUTEX stMutex;    
    
    // ÇÚµé Ç®(Handle Pool)ÀÇ Ÿîµå·¹œº
    FILE* pstHandlePool;
} FILESYSTEMMANAGER;


////////////////////////////////////////////////////////////////////////////////
//
// ÇÔŒö
//
////////////////////////////////////////////////////////////////////////////////
BOOL kInitializeFileSystem( void );
BOOL kFormat( void );
BOOL kMount( void );
BOOL kGetHDDInformation( HDDINFORMATION* pstInformation);

//  ÀúŒöÁØ ÇÔŒö(Low Level Function)
static BOOL kReadClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer );
static BOOL kWriteClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer );
static BOOL kReadCluster( DWORD dwOffset, BYTE* pbBuffer );
static BOOL kWriteCluster( DWORD dwOffset, BYTE* pbBuffer );
static DWORD kFindFreeCluster( void );
static BOOL kSetClusterLinkData( DWORD dwClusterIndex, DWORD dwData );
static BOOL kGetClusterLinkData( DWORD dwClusterIndex, DWORD* pdwData );
static int kFindFreeDirectoryEntry( void );
static BOOL kSetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry );
static BOOL kGetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry );
static int kFindDirectoryEntry( const char* pcFileName, DIRECTORYENTRY* pstEntry );
void kGetFileSystemInformation( FILESYSTEMMANAGER* pstManager );

//  °íŒöÁØ ÇÔŒö(High Level Function)
FILE* kOpenFile( const char* pcFileName, const char* pcMode );
DWORD kReadFile( void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile );
DWORD kWriteFile( const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile );
int kSeekFile( FILE* pstFile, int iOffset, int iOrigin );
int kCloseFile( FILE* pstFile );
int kRemoveFile( const char* pcFileName );
DIR* kOpenDirectory( const char* pcDirectoryName );
struct kDirectoryEntryStruct* kReadDirectory( DIR* pstDirectory );
void kRewindDirectory( DIR* pstDirectory );
int kCloseDirectory( DIR* pstDirectory );
BOOL kWriteZero( FILE* pstFile, DWORD dwCount );
BOOL kIsFileOpened( const DIRECTORYENTRY* pstEntry );


static void* kAllocateFileDirectoryHandle( void );
static void kFreeFileDirectoryHandle( FILE* pstFile );
static BOOL kCreateFile( const char* pcFileName, DIRECTORYENTRY* pstEntry, 
        int* piDirectoryEntryIndex );
static BOOL kFreeClusterUntilEnd( DWORD dwClusterIndex );
static BOOL kUpdateDirectoryEntry( FILEHANDLE* pstFileHandle );
static BOOL kCreateDirectory( const char* pcFileName, DIRECTORYENTRY* pstEntry, 
        int* piDirectoryEntryIndex );               //
 DIRECTORYENTRY* kFindDirectory( DWORD currentCluster );      //
 void kSetDotInDirectory();
 void kSetClusterIndex(DWORD currentDirectoryClusterIndex);
 BOOL kUpdateDirectory( int piDirectoryEntryIndex,const char* fileName,const char* parentPath, int parentIndex );

#endif /*__FILESYSTEM_H__*/
