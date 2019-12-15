/**
 *  file    FileSystem.c
 *  date    2009/05/01
 *  author  kkamagui 
 *          Copyright(c)2008 All rights reserved by kkamagui
 *  brief  ÆÄÀÏ œÃœºÅÛ¿¡ °ü·ÃµÈ ÇìŽõ ÆÄÀÏ
 */

#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "Utility.h"


static FILESYSTEMMANAGER   gs_stFileSystemManager;

static BYTE gs_vbTempBuffer[ FILESYSTEM_SECTORSPERCLUSTER * 512 ];


fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector gs_pfReadHDDSector = NULL;
fWriteHDDSector gs_pfWriteHDDSector = NULL;

char prompt_path[100];
DWORD currentClusterIndex = 0;

void kSetClusterIndex(DWORD currentDirectoryClusterIndex)
{
    currentClusterIndex = currentDirectoryClusterIndex;
}
/**
 *  ÆÄÀÏ œÃœºÅÛÀ» ÃÊ±âÈ­
 */
BOOL kInitializeFileSystem( void )
{
  
    kMemSet( &gs_stFileSystemManager, 0, sizeof( gs_stFileSystemManager ) );
    kInitializeMutex( &( gs_stFileSystemManager.stMutex ) );
    
    
    if( kInitializeHDD() == TRUE )
    {
        
        gs_pfReadHDDInformation = kReadHDDInformation;
        gs_pfReadHDDSector = kReadHDDSector;
        gs_pfWriteHDDSector = kWriteHDDSector;
    }
    else
    {
        return FALSE;
    }
    
    
    if( kMount() == FALSE )
    {
        return FALSE;
    }
    
    // ÇÚµéÀ» À§ÇÑ °ø°£À» ÇÒŽç
    gs_stFileSystemManager.pstHandlePool = ( FILE* ) kAllocateMemory( 
        FILESYSTEM_HANDLE_MAXCOUNT * sizeof( FILE ) );
    
    // žÞžðž® ÇÒŽçÀÌ œÇÆÐÇÏžé ÇÏµå µðœºÅ©°¡ ÀÎœÄµÇÁö ŸÊÀº °ÍÀž·Î Œ³Á€
    if( gs_stFileSystemManager.pstHandlePool == NULL )
    {
        gs_stFileSystemManager.bMounted = FALSE;
        return FALSE;
    }
    
    // ÇÚµé Ç®À» žðµÎ 0Àž·Î Œ³Á€ÇÏ¿© ÃÊ±âÈ­
    kMemSet( gs_stFileSystemManager.pstHandlePool, 0, 
            FILESYSTEM_HANDLE_MAXCOUNT * sizeof( FILE ) );    
    
    return TRUE;
}

//==============================================================================
//  ÀúŒöÁØ ÇÔŒö(Low Level Function)
//==============================================================================
/**
 *  ÇÏµå µðœºÅ©ÀÇ MBRÀ» ÀÐŸîŒ­ MINT ÆÄÀÏ œÃœºÅÛÀÎÁö È®ÀÎ
 *      MINT ÆÄÀÏ œÃœºÅÛÀÌ¶óžé ÆÄÀÏ œÃœºÅÛ¿¡ °ü·ÃµÈ °¢ÁŸ Á€ºžžŠ ÀÐŸîŒ­
 *      ÀÚ·á±žÁ¶¿¡ »ðÀÔ
 */
BOOL kMount( void )
{
    MBR* pstMBR;
    
    // µ¿±âÈ­ Ã³ž®
    kLock( &( gs_stFileSystemManager.stMutex ) );

    // MBRÀ» ÀÐÀœ
    if( gs_pfReadHDDSector( TRUE, TRUE, 0, 1, gs_vbTempBuffer ) == FALSE )
    {
        // µ¿±âÈ­ Ã³ž®
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return FALSE;
    }
    
    // œÃ±×³ÊÃ³žŠ È®ÀÎÇÏ¿© °°ŽÙžé ÀÚ·á±žÁ¶¿¡ °¢ ¿µ¿ª¿¡ ŽëÇÑ Á€ºž »ðÀÔ
    pstMBR = ( MBR* ) gs_vbTempBuffer;
    if( pstMBR->dwSignature != FILESYSTEM_SIGNATURE )
    {
        // µ¿±âÈ­ Ã³ž®
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return FALSE;
    }
    
    // ÆÄÀÏ œÃœºÅÛ ÀÎœÄ Œº°ø
    gs_stFileSystemManager.bMounted = TRUE;
    
    // °¢ ¿µ¿ªÀÇ œÃÀÛ LBA Ÿîµå·¹œº¿Í ŒœÅÍ ŒöžŠ °è»ê
    gs_stFileSystemManager.dwReservedSectorCount = pstMBR->dwReservedSectorCount;
    gs_stFileSystemManager.dwClusterLinkAreaStartAddress =
        pstMBR->dwReservedSectorCount + 1;
    gs_stFileSystemManager.dwClusterLinkAreaSize = pstMBR->dwClusterLinkSectorCount;
    gs_stFileSystemManager.dwDataAreaStartAddress = 
        pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount + 1;
    gs_stFileSystemManager.dwTotalClusterCount = pstMBR->dwTotalClusterCount;

    // µ¿±âÈ­ Ã³ž®
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    return TRUE;
}

/**
 *  ÇÏµå µðœºÅ©¿¡ ÆÄÀÏ œÃœºÅÛÀ» »ýŒº
 */
BOOL kFormat( void )
{
    HDDINFORMATION* pstHDD;
    MBR* pstMBR;
    DWORD dwTotalSectorCount, dwRemainSectorCount;
    DWORD dwMaxClusterCount, dwClsuterCount;
    DWORD dwClusterLinkSectorCount;
    DWORD i;
   
    kLock( &( gs_stFileSystemManager.stMutex ) );

    //==========================================================================
    //  하드 디스크 정보를 읽어서 메타 영역의 크기와 클러스터의 개수를 계산
    //==========================================================================
    // 하드 디스크의 정보를 얻어서 하드 디스크의 총 섹터 수를 구함
    pstHDD = ( HDDINFORMATION* ) gs_vbTempBuffer;
    if( gs_pfReadHDDInformation( TRUE, TRUE, pstHDD ) == FALSE )
    {
        
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return FALSE;
    }    
    dwTotalSectorCount = pstHDD->dwTotalSectors;
  
    // 전체 섹터 수를 4Kbyte, 즉 클러스터 크기로 나누어 최대 클러스터 수를 계산
    dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORSPERCLUSTER;
        // 최대 클러스터의 수에 맞추어 클러스터 링크 테이블의 섹터 수를 계산
    // 링크 데이터는 4바이트이므로, 한 섹터에는 128개가 들어감. 따라서 총 개수를
    // 128로 나눈 후 올림하여 클러스터 링크의 섹터 수를 구함
    
    dwClusterLinkSectorCount = ( dwMaxClusterCount + 127 ) / 128;
    
    dwRemainSectorCount = dwTotalSectorCount - dwClusterLinkSectorCount - 1;
    dwClsuterCount = dwRemainSectorCount / FILESYSTEM_SECTORSPERCLUSTER;
    
    
    dwClusterLinkSectorCount = ( dwClsuterCount + 127 ) / 128;

  
    if( gs_pfReadHDDSector( TRUE, TRUE, 0, 1, gs_vbTempBuffer ) == FALSE )
    {
        // µ¿±âÈ­ Ã³ž®
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return FALSE;
    }        
     
    pstMBR = ( MBR* ) gs_vbTempBuffer;
    kMemSet( pstMBR->vstPartition, 0, sizeof( pstMBR->vstPartition ) );
    pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
    pstMBR->dwReservedSectorCount = 0;
    pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
    pstMBR->dwTotalClusterCount = dwClsuterCount;
    
  
    if( gs_pfWriteHDDSector( TRUE, TRUE, 0, 1, gs_vbTempBuffer ) == FALSE )
    {
       
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return FALSE;
    }
    
    // MBR 이후부터 루트 디렉터리까지 모두 0으로 초기화
    kMemSet( gs_vbTempBuffer, 0, 512 );
    for( i = 0 ; i < ( dwClusterLinkSectorCount + FILESYSTEM_SECTORSPERCLUSTER );
         i++ )
    {
         // 루트 디렉터리(클러스터 0)는 이미 파일 시스템이 사용하고 있으므로,
        // 할당된 것으로 표시
        if( i == 0 )
        {
            ( ( DWORD* ) ( gs_vbTempBuffer ) )[ 0 ] = FILESYSTEM_LASTCLUSTER;
        }
        else
        {
            ( ( DWORD* ) ( gs_vbTempBuffer ) )[ 0 ] = FILESYSTEM_FREECLUSTER;
        }
    // 1 섹터씩 씀
        if( gs_pfWriteHDDSector( TRUE, TRUE, i + 1, 1, gs_vbTempBuffer ) == FALSE )
        {
          
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return FALSE;
        }
    }


    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;


   kReadRTCTime( &bHour, &bMinute, &bSecond );
   kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );  

   kSetDotInDirectory( bSecond, bMinute, bHour, bDayOfWeek, bDayOfMonth, bMonth, wYear, 100);
    
   
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    return TRUE;
}

/** 
 * 디렉토리의 dot, dot dot 생성
 */

void kSetDotInDirectory(BYTE bSecond, BYTE bMinute, BYTE bHour, BYTE bDayOfWeek, BYTE bDayOfMonth, BYTE bMonth,WORD wYear, WORD dwCluster){
    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset = 0;

DWORD currentDirectoryClusterIndex =0;

 FILESYSTEMMANAGER stManager;
DIRECTORYENTRY* directoryInfo;
 kGetFileSystemInformation( &stManager );
  directoryInfo = kFindDirectory(currentDirectoryClusterIndex);

currentDirectoryClusterIndex = directoryInfo[0].ParentDirectoryCluserIndex;
   // kSetClusterIndex(currentDirectoryClusterIndex);


    // 디렉터리 엔트리를 설정
    kMemCpy( stEntry.vcFileName, ".", 2 );
    stEntry.dwStartClusterIndex = -1;
    stEntry.dwFileSize = 0;
    stEntry.flag=1;
    stEntry.f =1;
    stEntry.ParentDirectoryCluserIndex = 0; //0
    stEntry.ParentDirectoryPath[0] = '/';
    stEntry.ParentDirectoryPath[1] = '\0';
    
    stEntry.bSecond = bSecond;
    stEntry.bMinute = bMinute;
    stEntry.bHour = bHour;
    stEntry.wYear = wYear;
    stEntry.bDayOfWeek= bDayOfWeek;
    stEntry.bDayOfMonth = bDayOfMonth;
    stEntry.bMonth =bMonth;

//kPrintf("%d",stEntry.bDayOfMonth);
    // 디렉터리 엔트리 등록
    if( kSetDirectoryEntryData( iDirectoryEntryOffset, &stEntry ) == FALSE )
    {
        kPrintf("\nyes\n");
        return FALSE;
    }
kPrintf("\nyes2\n");
    DIRECTORYENTRY stEntry2;
    kMemCpy( stEntry2.vcFileName, "..", 3 );
    stEntry2.dwStartClusterIndex = -2;
    stEntry2.dwFileSize = 0;
    stEntry2.flag=1;
    stEntry2.f=0;  //0
    stEntry2.ParentDirectoryCluserIndex = 0;
    stEntry2.ParentDirectoryPath[0] = '/';
    stEntry2.ParentDirectoryPath[1] = '\0';
   
      stEntry2.bSecond = bSecond;
    stEntry2.bMinute = bMinute;
    stEntry2.bHour = bHour;
    stEntry2.wYear = wYear;
    stEntry2.bDayOfWeek= bDayOfWeek;
    stEntry2.bDayOfMonth = bDayOfMonth;
    stEntry2.bMonth =bMonth;
    
    
    iDirectoryEntryOffset = 1;
    // 디렉터리 엔트리를 등록
    if( kSetDirectoryEntryData( iDirectoryEntryOffset, &stEntry2 ) == FALSE )
    {
        return FALSE;
    }
kPrintf("%d/%d/%d ", stEntry2.wYear, stEntry2.bDayOfMonth, 0);
	kPrintf("%d:%d:%d\n",stEntry2.bHour ,0, stEntry2.bSecond );

}


/**
 *  파일 시스템에 연결된 하드 디스크의 정보를 반환
 */
BOOL kGetHDDInformation( HDDINFORMATION* pstInformation)
{
    BOOL bResult;
     
    kLock( &( gs_stFileSystemManager.stMutex ) );
    
    bResult = gs_pfReadHDDInformation( TRUE, TRUE, pstInformation );
    
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    
    return bResult;
}


static BOOL kReadClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer )
{
   
    return gs_pfReadHDDSector( TRUE, TRUE, dwOffset + 
              gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer );
}


static BOOL kWriteClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer )
{
  
    return gs_pfWriteHDDSector( TRUE, TRUE, dwOffset + 
               gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer );
}


static BOOL kReadCluster( DWORD dwOffset, BYTE* pbBuffer )
{
    
    return gs_pfReadHDDSector( TRUE, TRUE, ( dwOffset * FILESYSTEM_SECTORSPERCLUSTER ) + 
              gs_stFileSystemManager.dwDataAreaStartAddress, 
              FILESYSTEM_SECTORSPERCLUSTER, pbBuffer );
}


static BOOL kWriteCluster( DWORD dwOffset, BYTE* pbBuffer )
{
    // µ¥ÀÌÅÍ ¿µ¿ªÀÇ œÃÀÛ Ÿîµå·¹œºžŠ ŽõÇÔ
    return gs_pfWriteHDDSector( TRUE, TRUE, ( dwOffset * FILESYSTEM_SECTORSPERCLUSTER ) + 
              gs_stFileSystemManager.dwDataAreaStartAddress, 
              FILESYSTEM_SECTORSPERCLUSTER, pbBuffer );
}

/**
 *  Å¬·¯œºÅÍ žµÅ© Å×ÀÌºí ¿µ¿ª¿¡Œ­ ºó Å¬·¯œºÅÍžŠ °Ë»öÇÔ
 */
static DWORD kFindFreeCluster( void )
{
    DWORD dwLinkCountInSector;
    DWORD dwLastSectorOffset, dwCurrentSectorOffset;
    DWORD i, j;
    
    // ÆÄÀÏ œÃœºÅÛÀ» ÀÎœÄÇÏÁö žøÇßÀžžé œÇÆÐ
    if( gs_stFileSystemManager.bMounted == FALSE )
    {
        return FILESYSTEM_LASTCLUSTER;
    }
    
    // ž¶Áöž·Àž·Î Å¬·¯œºÅÍžŠ ÇÒŽçÇÑ Å¬·¯œºÅÍ žµÅ© Å×ÀÌºíÀÇ ŒœÅÍ ¿ÀÇÁŒÂÀ» °¡Á®¿È
    dwLastSectorOffset = gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;

    // ž¶Áöž·Àž·Î ÇÒŽçÇÑ À§Ä¡ºÎÅÍ ·çÇÁžŠ µ¹žéŒ­ ºó Å¬·¯œºÅÍžŠ °Ë»ö
    for( i = 0 ; i < gs_stFileSystemManager.dwClusterLinkAreaSize ; i++ )
    {
        // Å¬·¯œºÅÍ žµÅ© Å×ÀÌºíÀÇ ž¶Áöž· ŒœÅÍÀÌžé ÀüÃŒ ŒœÅÍžžÅ­ µµŽÂ °ÍÀÌ ŸÆŽÏ¶ó
        // ³²Àº Å¬·¯œºÅÍÀÇ ŒöžžÅ­ ·çÇÁžŠ µ¹ŸÆŸß ÇÔ
        if( ( dwLastSectorOffset + i ) == 
            ( gs_stFileSystemManager.dwClusterLinkAreaSize - 1 ) )
        {
            dwLinkCountInSector = gs_stFileSystemManager.dwTotalClusterCount % 128; 
        }
        else
        {
            dwLinkCountInSector = 128;
        }
        
        // ÀÌ¹ø¿¡ ÀÐŸîŸß ÇÒ Å¬·¯œºÅÍ žµÅ© Å×ÀÌºíÀÇ ŒœÅÍ ¿ÀÇÁŒÂÀ» ±žÇØŒ­ ÀÐÀœ
        dwCurrentSectorOffset = ( dwLastSectorOffset + i ) % 
            gs_stFileSystemManager.dwClusterLinkAreaSize;
        if( kReadClusterLinkTable( dwCurrentSectorOffset, gs_vbTempBuffer ) == FALSE )
        {
            return FILESYSTEM_LASTCLUSTER;
        }
        
        // ŒœÅÍ ³»¿¡Œ­ ·çÇÁžŠ µ¹žéŒ­ ºó Å¬·¯œºÅÍžŠ °Ë»ö
        for( j = 0 ; j < dwLinkCountInSector ; j++ )
        {
            if( ( ( DWORD* ) gs_vbTempBuffer )[ j ] == FILESYSTEM_FREECLUSTER )
            {
                break;
            }
        }
            
        // Ã£ŸÒŽÙžé Å¬·¯œºÅÍ ÀÎµŠœºžŠ ¹ÝÈ¯
        if( j != dwLinkCountInSector )
        {
            // ž¶Áöž·Àž·Î Å¬·¯œºÅÍžŠ ÇÒŽçÇÑ Å¬·¯œºÅÍ žµÅ© ³»ÀÇ ŒœÅÍ ¿ÀÇÁŒÂÀ» ÀúÀå
            gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = 
                dwCurrentSectorOffset;
            
            // ÇöÀç Å¬·¯œºÅÍ žµÅ© Å×ÀÌºíÀÇ ¿ÀÇÁŒÂÀ» °šŸÈÇÏ¿© Å¬·¯œºÅÍ ÀÎµŠœºžŠ °è»ê
            return ( dwCurrentSectorOffset * 128 ) + j;
        }
    }
    
    return FILESYSTEM_LASTCLUSTER;
}

/**
 *  Å¬·¯œºÅÍ žµÅ© Å×ÀÌºí¿¡ °ªÀ» Œ³Á€
 */
static BOOL kSetClusterLinkData( DWORD dwClusterIndex, DWORD dwData )
{
    DWORD dwSectorOffset;
    
    // ÆÄÀÏ œÃœºÅÛÀ» ÀÎœÄÇÏÁö žøÇßÀžžé œÇÆÐ
    if( gs_stFileSystemManager.bMounted == FALSE )
    {
        return FALSE;
    }
    
    // ÇÑ ŒœÅÍ¿¡ 128°³ÀÇ Å¬·¯œºÅÍ žµÅ©°¡ µéŸî°¡¹Ç·Î 128·Î ³ªŽ©žé ŒœÅÍ ¿ÀÇÁŒÂÀ» 
    // ±žÇÒ Œö ÀÖÀœ
    dwSectorOffset = dwClusterIndex / 128;

    // ÇØŽç ŒœÅÍžŠ ÀÐŸîŒ­ žµÅ© Á€ºžžŠ Œ³Á€ÇÑ ÈÄ, ŽÙœÃ ÀúÀå
    if( kReadClusterLinkTable( dwSectorOffset, gs_vbTempBuffer ) == FALSE )
    {
        return FALSE;
    }    
    
    ( ( DWORD* ) gs_vbTempBuffer )[ dwClusterIndex % 128 ] = dwData;

    if( kWriteClusterLinkTable( dwSectorOffset, gs_vbTempBuffer ) == FALSE )
    {
        return FALSE;
    }

    return TRUE;
}

/**
 *  Å¬·¯œºÅÍ žµÅ© Å×ÀÌºíÀÇ °ªÀ» ¹ÝÈ¯
 */
static BOOL kGetClusterLinkData( DWORD dwClusterIndex, DWORD* pdwData )
{
    DWORD dwSectorOffset;
    
    // ÆÄÀÏ œÃœºÅÛÀ» ÀÎœÄÇÏÁö žøÇßÀžžé œÇÆÐ
    if( gs_stFileSystemManager.bMounted == FALSE )
    {
        return FALSE;
    }
    
    // ÇÑ ŒœÅÍ¿¡ 128°³ÀÇ Å¬·¯œºÅÍ žµÅ©°¡ µéŸî°¡¹Ç·Î 128·Î ³ªŽ©žé ŒœÅÍ ¿ÀÇÁŒÂÀ» 
    // ±žÇÒ Œö ÀÖÀœ
    dwSectorOffset = dwClusterIndex / 128;
    
    if( dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize )
    {
        return FALSE;
    }
    
    
    // ÇØŽç ŒœÅÍžŠ ÀÐŸîŒ­ žµÅ© Á€ºžžŠ ¹ÝÈ¯
    if( kReadClusterLinkTable( dwSectorOffset, gs_vbTempBuffer ) == FALSE )
    {
        return FALSE;
    }    

    *pdwData = ( ( DWORD* ) gs_vbTempBuffer )[ dwClusterIndex % 128 ];
    return TRUE;
}


/**
 *  ·çÆ® µð·ºÅÍž®¿¡Œ­ ºó µð·ºÅÍž® ¿£Æ®ž®žŠ ¹ÝÈ¯
 */
static int kFindFreeDirectoryEntry( void )
{
    DIRECTORYENTRY* pstEntry;
    int i;

    // ÆÄÀÏ œÃœºÅÛÀ» ÀÎœÄÇÏÁö žøÇßÀžžé œÇÆÐ
    if( gs_stFileSystemManager.bMounted == FALSE )
    {
        return -1;
    }

    //  µð·ºÅÍž®žŠ ÀÐÀœ
    if( kReadCluster( currentClusterIndex, gs_vbTempBuffer ) == FALSE )
    {
        return -1;
    }
    
    // ·çÆ® µð·ºÅÍž® ŸÈ¿¡Œ­ ·çÇÁžŠ µ¹žéŒ­ ºó ¿£Æ®ž®, Áï œÃÀÛ Å¬·¯œºÅÍ ¹øÈ£°¡ 0ÀÎ
    // ¿£Æ®ž®žŠ °Ë»ö
    pstEntry = ( DIRECTORYENTRY* ) gs_vbTempBuffer;
    for( i = 0 ; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT ; i++ )
    {
        if( pstEntry[ i ].dwStartClusterIndex == 0 )
        {
            return i;
        }
    }
    return -1;
}

/**
 *  ÇØŽç µð·ºÅäž®ÀÇ ¿£Æ®ž® ¹è¿­ ž®ÅÏ
 */
 DIRECTORYENTRY* kFindDirectory( DWORD currentCluster )
{
    DIRECTORYENTRY* pstEntry = NULL;
    int i;

    // ÆÄÀÏ œÃœºÅÛÀ» ÀÎœÄÇÏÁö žøÇßÀžžé œÇÆÐ
    if( gs_stFileSystemManager.bMounted == FALSE )
    {
        return NULL;
    }

    // µð·ºÅÍž®žŠ ÀÐÀœ
    if( kReadCluster( currentCluster, gs_vbTempBuffer ) == FALSE )
    {
        return NULL;
    }
    
    // ·çÆ® µð·ºÅÍž® ŸÈ¿¡Œ­ ·çÇÁžŠ µ¹žéŒ­ ºó ¿£Æ®ž®, Áï œÃÀÛ Å¬·¯œºÅÍ ¹øÈ£°¡ 0ÀÎ
    // ¿£Æ®ž®žŠ °Ë»ö
    pstEntry = ( DIRECTORYENTRY* ) gs_vbTempBuffer;
    if(pstEntry == NULL)
        return NULL;

    return pstEntry;
    
}

/**
 *  ·çÆ® µð·ºÅÍž®ÀÇ ÇØŽç ÀÎµŠœº¿¡ µð·ºÅÍž® ¿£Æ®ž®žŠ Œ³Á€
 */
static BOOL kSetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry )
{
    DIRECTORYENTRY* pstRootEntry;
    
    // ÆÄÀÏ œÃœºÅÛÀ» ÀÎœÄÇÏÁö žøÇß°Å³ª ÀÎµŠœº°¡ ¿Ã¹Ùž£Áö ŸÊÀžžé œÇÆÐ
    if( ( gs_stFileSystemManager.bMounted == FALSE ) ||
        ( iIndex < 0 ) || ( iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT ) )
    {
        return FALSE;
    }

    //  µð·ºÅÍž®žŠ ÀÐÀœ
    if( kReadCluster( currentClusterIndex, gs_vbTempBuffer ) == FALSE )
    {
        return FALSE;
    }    
    
    // ·çÆ® µð·ºÅÍž®¿¡ ÀÖŽÂ ÇØŽç µ¥ÀÌÅÍžŠ °»œÅ
    pstRootEntry = ( DIRECTORYENTRY* ) gs_vbTempBuffer;
    kMemCpy( pstRootEntry + iIndex, pstEntry, sizeof( DIRECTORYENTRY ) );


      BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;





    // ·çÆ® µð·ºÅÍž®¿¡ Ÿž
    if( kWriteCluster( currentClusterIndex, gs_vbTempBuffer ) == FALSE )
    {
        return FALSE;
    }    
    return TRUE;
}

/**
 *  ·çÆ® µð·ºÅÍž®ÀÇ ÇØŽç ÀÎµŠœº¿¡ À§Ä¡ÇÏŽÂ µð·ºÅÍž® ¿£Æ®ž®žŠ ¹ÝÈ¯
 */
static BOOL kGetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry )
{
    DIRECTORYENTRY* pstRootEntry;
    
    // ÆÄÀÏ œÃœºÅÛÀ» ÀÎœÄÇÏÁö žøÇß°Å³ª ÀÎµŠœº°¡ ¿Ã¹Ùž£Áö ŸÊÀžžé œÇÆÐ
    if( ( gs_stFileSystemManager.bMounted == FALSE ) ||
        ( iIndex < 0 ) || ( iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT ) )
    {
        return FALSE;
    }

    // ·çÆ® µð·ºÅÍž®žŠ ÀÐÀœ
    if( kReadCluster( 0, gs_vbTempBuffer ) == FALSE )
    {
        return FALSE;
    }    
    
    // ·çÆ® µð·ºÅÍž®¿¡ ÀÖŽÂ ÇØŽç µ¥ÀÌÅÍžŠ °»œÅ
    pstRootEntry = ( DIRECTORYENTRY* ) gs_vbTempBuffer;
    kMemCpy( pstEntry, pstRootEntry + iIndex, sizeof( DIRECTORYENTRY ) );
    return TRUE;
}

/**
 *  ·çÆ® µð·ºÅÍž®¿¡Œ­ ÆÄÀÏ ÀÌž§ÀÌ ÀÏÄ¡ÇÏŽÂ ¿£Æ®ž®žŠ Ã£ŸÆŒ­ ÀÎµŠœºžŠ ¹ÝÈ¯
 */
static int kFindDirectoryEntry( const char* pcFileName, DIRECTORYENTRY* pstEntry )
{
    DIRECTORYENTRY* pstRootEntry;
    int i;
    int iLength;

    // ÆÄÀÏ œÃœºÅÛÀ» ÀÎœÄÇÏÁö žøÇßÀžžé œÇÆÐ
    if( gs_stFileSystemManager.bMounted == FALSE )
    {
        return -1;
    }

    // ·çÆ® µð·ºÅÍž®žŠ ÀÐÀœ
    if( kReadCluster( currentClusterIndex, gs_vbTempBuffer ) == FALSE )
    {
        return -1;
    }
    
    iLength = kStrLen( pcFileName );
    // ·çÆ® µð·ºÅÍž® ŸÈ¿¡Œ­ ·çÇÁžŠ µ¹žéŒ­ ÆÄÀÏ ÀÌž§ÀÌ ÀÏÄ¡ÇÏŽÂ ¿£Æ®ž®žŠ ¹ÝÈ¯
    pstRootEntry = ( DIRECTORYENTRY* ) gs_vbTempBuffer;
    for( i = 0 ; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT ; i++ )
    {
        if( kMemCmp( pstRootEntry[ i ].vcFileName, pcFileName, iLength ) == 0 )
        {
            kMemCpy( pstEntry, pstRootEntry + i, sizeof( DIRECTORYENTRY ) );




            return i;
        }
    }
    return -1;
}

/**
 *  ÆÄÀÏ œÃœºÅÛÀÇ Á€ºžžŠ ¹ÝÈ¯
 */
void kGetFileSystemInformation( FILESYSTEMMANAGER* pstManager )
{
    kMemCpy( pstManager, &gs_stFileSystemManager, sizeof( gs_stFileSystemManager ) );
}

//==============================================================================
//  °íŒöÁØ ÇÔŒö(High Level Function)
//==============================================================================
/**
 *  ºñŸîÀÖŽÂ ÇÚµéÀ» ÇÒŽç
 */
static void* kAllocateFileDirectoryHandle( void )
{
    int i;
    FILE* pstFile;
    
    // ÇÚµé Ç®(Handle Pool)À» žðµÎ °Ë»öÇÏ¿© ºñŸîÀÖŽÂ ÇÚµéÀ» ¹ÝÈ¯
    pstFile = gs_stFileSystemManager.pstHandlePool;
    for( i = 0 ; i < FILESYSTEM_HANDLE_MAXCOUNT ; i++ )
    {
        // ºñŸîÀÖŽÙžé ¹ÝÈ¯
        if( pstFile->bType == FILESYSTEM_TYPE_FREE )
        {
            pstFile->bType = FILESYSTEM_TYPE_FILE;
            return pstFile;
        }
        
        // ŽÙÀœÀž·Î ÀÌµ¿
        pstFile++;
    }
    
    return NULL;
}

/**
 *  »ç¿ëÇÑ ÇÚµéÀ» ¹ÝÈ¯
 */
static void kFreeFileDirectoryHandle( FILE* pstFile )
{
    // ÀüÃŒ ¿µ¿ªÀ» ÃÊ±âÈ­
    kMemSet( pstFile, 0, sizeof( FILE ) );
    
    // ºñŸîÀÖŽÂ ÅžÀÔÀž·Î Œ³Á€
    pstFile->bType = FILESYSTEM_TYPE_FREE;
}

/**
 *  ÆÄÀÏÀ» »ýŒº
 */
static BOOL kCreateFile( const char* pcFileName, DIRECTORYENTRY* pstEntry, 
        int* piDirectoryEntryIndex )
{
	BYTE bSecond, bMinute, bHour;
	BYTE bDayOfWeek, bDayOfMonth, bMonth;
	WORD wYear;

    DWORD dwCluster;

    dwCluster = kFindFreeCluster();
    if( ( dwCluster == FILESYSTEM_LASTCLUSTER ) ||
        ( kSetClusterLinkData( dwCluster, FILESYSTEM_LASTCLUSTER ) == FALSE ) )
    {
        return FALSE;
    }

     
    *piDirectoryEntryIndex = kFindFreeDirectoryEntry();
    if( *piDirectoryEntryIndex == -1 )
    {
        
        kSetClusterLinkData( dwCluster, FILESYSTEM_FREECLUSTER );
        return FALSE;
    }

    
    kMemCpy( pstEntry->vcFileName, pcFileName, kStrLen( pcFileName ) + 1 );
    pstEntry->dwStartClusterIndex = dwCluster;
    pstEntry->dwFileSize = 0;
    pstEntry->flag = 0;
   // pstEntyr->f =0;


   kReadRTCTime( &bHour, &bMinute, &bSecond );
   kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );

    pstEntry-> bSecond = bSecond;
    pstEntry-> bMinute = bMinute;
    pstEntry-> bHour = bHour;
    pstEntry-> wYear = wYear;
    pstEntry-> bDayOfWeek= bDayOfWeek;
    pstEntry-> bDayOfMonth =  bDayOfMonth;
    pstEntry-> bMonth = bMonth;

	
    // µð·ºÅÍž® ¿£Æ®ž®žŠ µî·Ï
    if( kSetDirectoryEntryData( *piDirectoryEntryIndex, pstEntry ) == FALSE )
    {
        // œÇÆÐÇÒ °æ¿ì ÇÒŽç ¹ÞÀº Å¬·¯œºÅÍžŠ ¹ÝÈ¯ÇØŸß ÇÔ
        kSetClusterLinkData( dwCluster, FILESYSTEM_FREECLUSTER );
        return FALSE;
    }
    return TRUE;
}


static BOOL kCreateDirectory( const char* pcFileName, DIRECTORYENTRY* pstEntry, 
        int* piDirectoryEntryIndex )
{
    DWORD dwCluster;
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    
    dwCluster = kFindFreeCluster();
    if( ( dwCluster == FILESYSTEM_LASTCLUSTER ) ||
        ( kSetClusterLinkData( dwCluster, FILESYSTEM_LASTCLUSTER ) == FALSE ) )
    {
        return FALSE;
    }


   
    *piDirectoryEntryIndex = kFindFreeDirectoryEntry();
    if( *piDirectoryEntryIndex == -1 )
    {
        
        kSetClusterLinkData( dwCluster, FILESYSTEM_FREECLUSTER );
        return FALSE;
    }
    
   
    kMemCpy( pstEntry->vcFileName, pcFileName, kStrLen( pcFileName ) + 1 );
    pstEntry->dwStartClusterIndex = dwCluster;
    pstEntry->dwFileSize = 0;
    pstEntry->flag=1;
    pstEntry->f=2;
    pstEntry->ParentDirectoryPath[0] = '/';
    pstEntry->ParentDirectoryPath[1] = '\0';
    pstEntry->ParentDirectoryCluserIndex = 0;
   

   kReadRTCTime( &bHour, &bMinute, &bSecond );
   kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );

    pstEntry-> bSecond = bSecond;
    pstEntry-> bMinute = bMinute;
    pstEntry-> bHour = bHour;
    pstEntry-> wYear = wYear;
    pstEntry-> bDayOfWeek= bDayOfWeek;
    pstEntry-> bDayOfMonth =  bDayOfMonth;
    pstEntry-> bMonth = bMonth;


    // µð·ºÅÍž® ¿£Æ®ž®žŠ µî·Ï
    if( kSetDirectoryEntryData( *piDirectoryEntryIndex, pstEntry ) == FALSE )
    {
        // œÇÆÐÇÒ °æ¿ì ÇÒŽç ¹ÞÀº Å¬·¯œºÅÍžŠ ¹ÝÈ¯ÇØŸß ÇÔ
        kSetClusterLinkData( dwCluster, FILESYSTEM_FREECLUSTER );
        return FALSE;
    }
    
    kSetDotInDirectory(bSecond, bMinute, bHour, bDayOfWeek, bDayOfMonth, bMonth, wYear, dwCluster);

    return TRUE;
}

/**
 *  µð·ºÅäž®žŠ Ÿ÷µ¥ÀÌÆ®
 */
BOOL kUpdateDirectory( int piDirectoryEntryIndex,const char* fileName,const char* parentPath, int parentIndex )
{
    DWORD dwCluster;
    DIRECTORYENTRY* psEntry;


//    BYTE bSecond, bMinute, bHour;
//    BYTE bDayOfWeek, bDayOfMonth, bMonth;
//    WORD wYear;

//   kReadRTCTime( &bHour, &bMinute, &bSecond );
//   kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );  

    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset = piDirectoryEntryIndex;

    // µð·ºÅÍž® ¿£Æ®ž®žŠ Œ³Á€
    kMemCpy( stEntry.vcFileName, fileName, kStrLen(fileName)+1 );
    stEntry.dwStartClusterIndex = -1;
    stEntry.dwFileSize = 0;
    stEntry.flag=1;
    stEntry.ParentDirectoryCluserIndex = parentIndex;
    
//    stEntry-> bSecond = bSecond;
//    stEntry-> bMinute = bMinute;
//    stEntry-> bHour = bHour;
//    stEntry-> wYear = wYear;
//    stEntry-> bDayOfWeek= bDayOfWeek;
//    stEntry-> bDayOfMonth =  bDayOfMonth;
//    stEntry-> bMonth = bMonth;

    kMemCpy(stEntry.ParentDirectoryPath,parentPath,kStrLen(parentPath)+1);
    
    
    // µð·ºÅÍž® ¿£Æ®ž®žŠ µî·Ï
    if( kSetDirectoryEntryData( iDirectoryEntryOffset, &stEntry ) == FALSE )
    {
        
        return FALSE;
    }

    return TRUE;
}

/**
 *  ÆÄ¶ó¹ÌÅÍ·Î ³ÑŸî¿Â Å¬·¯œºÅÍºÎÅÍ ÆÄÀÏÀÇ ³¡±îÁö ¿¬°áµÈ Å¬·¯œºÅÍžŠ žðµÎ ¹ÝÈ¯
 */
static BOOL kFreeClusterUntilEnd( DWORD dwClusterIndex )
{
    DWORD dwCurrentClusterIndex;
    DWORD dwNextClusterIndex;
    
    // Å¬·¯œºÅÍ ÀÎµŠœºžŠ ÃÊ±âÈ­
    dwCurrentClusterIndex = dwClusterIndex;
    
    while( dwCurrentClusterIndex != FILESYSTEM_LASTCLUSTER )
    {
        // ŽÙÀœ Å¬·¯œºÅÍÀÇ ÀÎµŠœºžŠ °¡Á®¿È
        if( kGetClusterLinkData( dwCurrentClusterIndex, &dwNextClusterIndex )
                == FALSE )
        {
            return FALSE;
        }
        
        // ÇöÀç Å¬·¯œºÅÍžŠ ºó °ÍÀž·Î Œ³Á€ÇÏ¿© ÇØÁŠ
        if( kSetClusterLinkData( dwCurrentClusterIndex, FILESYSTEM_FREECLUSTER )
                == FALSE )
        {
            return FALSE;
        }
        
        // ÇöÀç Å¬·¯œºÅÍ ÀÎµŠœºžŠ ŽÙÀœ Å¬·¯œºÅÍÀÇ ÀÎµŠœº·Î ¹Ù²Þ
        dwCurrentClusterIndex = dwNextClusterIndex;
    }
    
    return TRUE;
}

/**
 *  ÆÄÀÏÀ» ¿­°Å³ª »ýŒº 
 */
FILE* kOpenFile( const char* pcFileName, const char* pcMode )
{
    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset;
    int iFileNameLength;
    DWORD dwSecondCluster;
    FILE* pstFile;

    // ÆÄÀÏ ÀÌž§ °Ë»ç
    iFileNameLength = kStrLen( pcFileName );
    if( ( iFileNameLength > ( sizeof( stEntry.vcFileName ) - 1 ) ) || 
        ( iFileNameLength == 0 ) )
    {
        return NULL;
    }
    
    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );
    
    //==========================================================================
    // ÆÄÀÏÀÌ žÕÀú ÁžÀçÇÏŽÂ°¡ È®ÀÎÇÏ°í, ŸøŽÙžé ¿ÉŒÇÀ» ºž°í ÆÄÀÏÀ» »ýŒº
    //==========================================================================
    iDirectoryEntryOffset = kFindDirectoryEntry( pcFileName, &stEntry );
    if( iDirectoryEntryOffset == -1 )
    {
        // ÆÄÀÏÀÌ ŸøŽÙžé ÀÐ±â(r, r+) ¿ÉŒÇÀº œÇÆÐ
        if( pcMode[ 0 ] == 'r' )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return NULL;
        }
        
        // ³ªžÓÁö ¿ÉŒÇµéÀº ÆÄÀÏÀ» »ýŒº
        if( kCreateFile( pcFileName, &stEntry, &iDirectoryEntryOffset ) == FALSE )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return NULL;
        }
    }    
    //==========================================================================
    // ÆÄÀÏÀÇ ³»¿ëÀ» ºñ¿öŸß ÇÏŽÂ ¿ÉŒÇÀÌžé ÆÄÀÏ¿¡ ¿¬°áµÈ Å¬·¯œºÅÍžŠ žðµÎ ÁŠ°ÅÇÏ°í
    // ÆÄÀÏ Å©±âžŠ 0Àž·Î Œ³Á€
    //==========================================================================
    else if( pcMode[ 0 ] == 'w' )
    {
        // œÃÀÛ Å¬·¯œºÅÍÀÇ ŽÙÀœ Å¬·¯œºÅÍžŠ Ã£Àœ
        if( kGetClusterLinkData( stEntry.dwStartClusterIndex, &dwSecondCluster )
                == FALSE )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return NULL;
        }
        
        // œÃÀÛ Å¬·¯œºÅÍžŠ ž¶Áöž· Å¬·¯œºÅÍ·Î žžµê
        if( kSetClusterLinkData( stEntry.dwStartClusterIndex, 
               FILESYSTEM_LASTCLUSTER ) == FALSE )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return NULL;
        }
        
        // ŽÙÀœ Å¬·¯œºÅÍºÎÅÍ ž¶Áöž· Å¬·¯œºÅÍ±îÁö žðµÎ ÇØÁŠ
        if( kFreeClusterUntilEnd( dwSecondCluster ) == FALSE )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return NULL;
        }
       
        // ÆÄÀÏÀÇ ³»¿ëÀÌ žðµÎ ºñ¿öÁ³Àž¹Ç·Î, Å©±âžŠ 0Àž·Î Œ³Á€
        stEntry.dwFileSize = 0;
        if( kSetDirectoryEntryData( iDirectoryEntryOffset, &stEntry ) == FALSE )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return NULL;
        }
    }
    
    //==========================================================================
    // ÆÄÀÏ ÇÚµéÀ» ÇÒŽç ¹ÞŸÆ µ¥ÀÌÅÍžŠ Œ³Á€ÇÑ ÈÄ ¹ÝÈ¯
    //==========================================================================
    // ÆÄÀÏ ÇÚµéÀ» ÇÒŽç ¹ÞŸÆ µ¥ÀÌÅÍ Œ³Á€
    pstFile = kAllocateFileDirectoryHandle();
    if( pstFile == NULL )
    {
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return NULL;
    }
    
    // ÆÄÀÏ ÇÚµé¿¡ ÆÄÀÏ Á€ºžžŠ Œ³Á€
    pstFile->bType = FILESYSTEM_TYPE_FILE;
    pstFile->stFileHandle.iDirectoryEntryOffset = iDirectoryEntryOffset;
    pstFile->stFileHandle.dwFileSize = stEntry.dwFileSize;
    pstFile->stFileHandle.dwStartClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwCurrentClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwPreviousClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwCurrentOffset = 0;
       
    // žžŸà Ãß°¡ ¿ÉŒÇ(a)ÀÌ Œ³Á€µÇŸî ÀÖÀžžé, ÆÄÀÏÀÇ ³¡Àž·Î ÀÌµ¿
    if( pcMode[ 0 ] == 'a' )
    {
        kSeekFile( pstFile, 0, FILESYSTEM_SEEK_END );
    }

    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    return pstFile;
}

/**
 *  ÆÄÀÏÀ» ÀÐŸî ¹öÆÛ·Î º¹»ç
 */
DWORD kReadFile( void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile )
{
    DWORD dwTotalCount;
    DWORD dwReadCount;
    DWORD dwOffsetInCluster;
    DWORD dwCopySize;
    FILEHANDLE* pstFileHandle;
    DWORD dwNextClusterIndex;    

    // ÇÚµéÀÌ ÆÄÀÏ ÅžÀÔÀÌ ŸÆŽÏžé œÇÆÐ
    if( ( pstFile == NULL ) ||
        ( pstFile->bType != FILESYSTEM_TYPE_FILE ) )
    {
        return 0;
    }
    pstFileHandle = &( pstFile->stFileHandle );
    
    // ÆÄÀÏÀÇ ³¡ÀÌ°Å³ª ž¶Áöž· Å¬·¯œºÅÍÀÌžé ÁŸ·á
    if( ( pstFileHandle->dwCurrentOffset == pstFileHandle->dwFileSize ) ||
        ( pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER ) )
    {
        return 0;
    }

    // ÆÄÀÏ ³¡°ú ºñ±³ÇØŒ­ œÇÁŠ·Î ÀÐÀ» Œö ÀÖŽÂ °ªÀ» °è»ê
    dwTotalCount = MIN( dwSize * dwCount, pstFileHandle->dwFileSize - 
                        pstFileHandle->dwCurrentOffset );
    
    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );
    
    // °è»êµÈ °ªžžÅ­ ŽÙ ÀÐÀ» ¶§±îÁö ¹Ýº¹
    dwReadCount = 0;
    while( dwReadCount != dwTotalCount )
    {
        //======================================================================
        // Å¬·¯œºÅÍžŠ ÀÐŸîŒ­ ¹öÆÛ¿¡ º¹»ç
        //======================================================================
        // ÇöÀç Å¬·¯œºÅÍžŠ ÀÐÀœ
        if( kReadCluster( pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer )
                == FALSE )
        {
            break;
        }

        // Å¬·¯œºÅÍ ³»¿¡Œ­ ÆÄÀÏ Æ÷ÀÎÅÍ°¡ ÁžÀçÇÏŽÂ ¿ÀÇÁŒÂÀ» °è»ê
        dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
        
        // ¿©·¯ Å¬·¯œºÅÍ¿¡ °ÉÃÄÁ® ÀÖŽÙžé ÇöÀç Å¬·¯œºÅÍ¿¡Œ­ ³²Àº žžÅ­ ÀÐ°í ŽÙÀœ 
        // Å¬·¯œºÅÍ·Î ÀÌµ¿
        dwCopySize = MIN( FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, 
                          dwTotalCount - dwReadCount );
        kMemCpy( ( char* ) pvBuffer + dwReadCount, 
                gs_vbTempBuffer + dwOffsetInCluster, dwCopySize );

        // ÀÐÀº ¹ÙÀÌÆ® Œö¿Í ÆÄÀÏ Æ÷ÀÎÅÍÀÇ À§Ä¡žŠ °»œÅ
        dwReadCount += dwCopySize;
        pstFileHandle->dwCurrentOffset += dwCopySize;

        //======================================================================
        // ÇöÀç Å¬·¯œºÅÍžŠ ³¡±îÁö ŽÙ ÀÐŸúÀžžé ŽÙÀœ Å¬·¯œºÅÍ·Î ÀÌµ¿
        //======================================================================
        if( ( pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE ) == 0 )
        {
            // ÇöÀç Å¬·¯œºÅÍÀÇ žµÅ© µ¥ÀÌÅÍžŠ Ã£ŸÆ ŽÙÀœ Å¬·¯œºÅÍžŠ ŸòÀœ
            if( kGetClusterLinkData( pstFileHandle->dwCurrentClusterIndex, 
                                     &dwNextClusterIndex ) == FALSE )
            {
                break;
            }
            
            // Å¬·¯œºÅÍ Á€ºžžŠ °»œÅ
            pstFileHandle->dwPreviousClusterIndex = 
                pstFileHandle->dwCurrentClusterIndex;
            pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
        }
    }
    
    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    
    // ÀÐÀº ·¹ÄÚµå ŒöžŠ ¹ÝÈ¯
    return ( dwReadCount / dwSize );
}

/**
 *  ·çÆ® µð·ºÅÍž®¿¡Œ­ µð·ºÅÍž® ¿£Æ®ž® °ªÀ» °»œÅ
 */
static BOOL kUpdateDirectoryEntry( FILEHANDLE* pstFileHandle )
{
    DIRECTORYENTRY stEntry;
    
    // µð·ºÅÍž® ¿£Æ®ž® °Ë»ö
    if( ( pstFileHandle == NULL ) ||
        ( kGetDirectoryEntryData( pstFileHandle->iDirectoryEntryOffset, &stEntry)
            == FALSE ) )
    {
        return FALSE;
    }
    
    // ÆÄÀÏ Å©±â¿Í œÃÀÛ Å¬·¯œºÅÍžŠ º¯°æ
    stEntry.dwFileSize = pstFileHandle->dwFileSize;
    stEntry.dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;

    // º¯°æµÈ µð·ºÅÍž® ¿£Æ®ž®žŠ Œ³Á€
    if( kSetDirectoryEntryData( pstFileHandle->iDirectoryEntryOffset, &stEntry )
            == FALSE )
    {
        return FALSE;
    }
    return TRUE;
}

/**
 *  ¹öÆÛÀÇ µ¥ÀÌÅÍžŠ ÆÄÀÏ¿¡ Ÿž
 */
DWORD kWriteFile( const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile )
{
    DWORD dwWriteCount;
    DWORD dwTotalCount;
    DWORD dwOffsetInCluster;
    DWORD dwCopySize;
    DWORD dwAllocatedClusterIndex;
    DWORD dwNextClusterIndex;
    FILEHANDLE* pstFileHandle;

    // ÇÚµéÀÌ ÆÄÀÏ ÅžÀÔÀÌ ŸÆŽÏžé œÇÆÐ
    if( ( pstFile == NULL ) ||
        ( pstFile->bType != FILESYSTEM_TYPE_FILE ) )
    {
        return 0;
    }
    pstFileHandle = &( pstFile->stFileHandle );

    // ÃÑ ¹ÙÀÌÆ® Œö
    dwTotalCount = dwSize * dwCount;
    
    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );

    // ŽÙ Ÿµ ¶§±îÁö ¹Ýº¹
    dwWriteCount = 0;
    while( dwWriteCount != dwTotalCount )
    {
        //======================================================================
        // ÇöÀç Å¬·¯œºÅÍ°¡ ÆÄÀÏÀÇ ³¡ÀÌžé Å¬·¯œºÅÍžŠ ÇÒŽçÇÏ¿© ¿¬°á
        //======================================================================
        if( pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER )
        {
            // ºó Å¬·¯œºÅÍ °Ë»ö
            dwAllocatedClusterIndex = kFindFreeCluster();
            if( dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER )
            {
                break;
            }
            
            // °Ë»öµÈ Å¬·¯œºÅÍžŠ ž¶Áöž· Å¬·¯œºÅÍ·Î Œ³Á€
            if( kSetClusterLinkData( dwAllocatedClusterIndex, FILESYSTEM_LASTCLUSTER )
                    == FALSE )
            {
                break;
            }
            
            // ÆÄÀÏÀÇ ž¶Áöž· Å¬·¯œºÅÍ¿¡ ÇÒŽçµÈ Å¬·¯œºÅÍžŠ ¿¬°á
            if( kSetClusterLinkData( pstFileHandle->dwPreviousClusterIndex, 
                                     dwAllocatedClusterIndex ) == FALSE )
            {
                // œÇÆÐÇÒ °æ¿ì ÇÒŽçµÈ Å¬·¯œºÅÍžŠ ÇØÁŠ
                kSetClusterLinkData( dwAllocatedClusterIndex, FILESYSTEM_FREECLUSTER );
                break;
            }
            
            // ÇöÀç Å¬·¯œºÅÍžŠ ÇÒŽçµÈ °ÍÀž·Î º¯°æ
            pstFileHandle->dwCurrentClusterIndex = dwAllocatedClusterIndex;
            
            // »õ·Î ÇÒŽç¹ÞŸÒÀžŽÏ ÀÓœÃ Å¬·¯œºÅÍ ¹öÆÛžŠ 0Àž·Î Ã€¿ò
            kMemSet( gs_vbTempBuffer, 0, FILESYSTEM_LASTCLUSTER );
        }        
        //======================================================================
        // ÇÑ Å¬·¯œºÅÍžŠ Ã€¿ìÁö žøÇÏžé Å¬·¯œºÅÍžŠ ÀÐŸîŒ­ ÀÓœÃ Å¬·¯œºÅÍ ¹öÆÛ·Î º¹»ç
        //======================================================================
        else if( ( ( pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE ) != 0 ) ||
                 ( ( dwTotalCount - dwWriteCount ) < FILESYSTEM_CLUSTERSIZE ) )
        {
            // ÀüÃŒ Å¬·¯œºÅÍžŠ µ€ŸîŸ²ŽÂ °æ¿ì°¡ ŸÆŽÏžé ºÎºÐžž µ€ŸîœáŸß ÇÏ¹Ç·Î 
            // ÇöÀç Å¬·¯œºÅÍžŠ ÀÐÀœ
            if( kReadCluster( pstFileHandle->dwCurrentClusterIndex, 
                              gs_vbTempBuffer ) == FALSE )
            {
                break;
            }
        }

        // Å¬·¯œºÅÍ ³»¿¡Œ­ ÆÄÀÏ Æ÷ÀÎÅÍ°¡ ÁžÀçÇÏŽÂ ¿ÀÇÁŒÂÀ» °è»ê
        dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
        
        // ¿©·¯ Å¬·¯œºÅÍ¿¡ °ÉÃÄÁ® ÀÖŽÙžé ÇöÀç Å¬·¯œºÅÍ¿¡Œ­ ³²Àº žžÅ­ Ÿ²°í ŽÙÀœ 
        // Å¬·¯œºÅÍ·Î ÀÌµ¿
        dwCopySize = MIN( FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, 
                          dwTotalCount - dwWriteCount );
        kMemCpy( gs_vbTempBuffer + dwOffsetInCluster, ( char* ) pvBuffer + 
                 dwWriteCount, dwCopySize );
        
        // ÀÓœÃ ¹öÆÛ¿¡ »ðÀÔµÈ °ªÀ» µðœºÅ©¿¡ Ÿž
        if( kWriteCluster( pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer ) 
                == FALSE )
        {
            break;
        }
        
        // ŸŽ ¹ÙÀÌÆ® Œö¿Í ÆÄÀÏ Æ÷ÀÎÅÍÀÇ À§Ä¡žŠ °»œÅ
        dwWriteCount += dwCopySize;
        pstFileHandle->dwCurrentOffset += dwCopySize;

        //======================================================================
        // ÇöÀç Å¬·¯œºÅÍÀÇ ³¡±îÁö ŽÙ œèÀžžé ŽÙÀœ Å¬·¯œºÅÍ·Î ÀÌµ¿
        //======================================================================
        if( ( pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE ) == 0 )
        {
            // ÇöÀç Å¬·¯œºÅÍÀÇ žµÅ© µ¥ÀÌÅÍ·Î ŽÙÀœ Å¬·¯œºÅÍžŠ ŸòÀœ
            if( kGetClusterLinkData( pstFileHandle->dwCurrentClusterIndex, 
                                     &dwNextClusterIndex ) == FALSE )
            {
                break;
            }
            
            // Å¬·¯œºÅÍ Á€ºžžŠ °»œÅ
            pstFileHandle->dwPreviousClusterIndex = 
                pstFileHandle->dwCurrentClusterIndex;
            pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
        }
    }

    //==========================================================================
    // ÆÄÀÏ Å©±â°¡ º¯ÇßŽÙžé ·çÆ® µð·ºÅÍž®¿¡ ÀÖŽÂ µð·ºÅÍž® ¿£Æ®ž® Á€ºžžŠ °»œÅ
    //==========================================================================
    if( pstFileHandle->dwFileSize < pstFileHandle->dwCurrentOffset )
    {
        pstFileHandle->dwFileSize = pstFileHandle->dwCurrentOffset;
        kUpdateDirectoryEntry( pstFileHandle );
    }
    
    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    
    // ŸŽ ·¹ÄÚµå ŒöžŠ ¹ÝÈ¯
    return ( dwWriteCount / dwSize );
}

/**
 *  ÆÄÀÏÀ» CountžžÅ­ 0Àž·Î Ã€¿ò
 */
BOOL kWriteZero( FILE* pstFile, DWORD dwCount )
{
    BYTE* pbBuffer;
    DWORD dwRemainCount;
    DWORD dwWriteCount;
    
    // ÇÚµéÀÌ NULLÀÌžé œÇÆÐ
    if( pstFile == NULL )
    {
        return FALSE;
    }
    
    // ŒÓµµ Çâ»óÀ» À§ÇØ žÞžðž®žŠ ÇÒŽç ¹ÞŸÆ Å¬·¯œºÅÍ ŽÜÀ§·Î Ÿ²±â ŒöÇà
    // žÞžðž®žŠ ÇÒŽç
    pbBuffer = ( BYTE* ) kAllocateMemory( FILESYSTEM_CLUSTERSIZE );
    if( pbBuffer == NULL )
    {
        return FALSE;
    }
    
    // 0Àž·Î Ã€¿ò
    kMemSet( pbBuffer, 0, FILESYSTEM_CLUSTERSIZE );
    dwRemainCount = dwCount;
    
    // Å¬·¯œºÅÍ ŽÜÀ§·Î ¹Ýº¹ÇØŒ­ Ÿ²±â ŒöÇà
    while( dwRemainCount != 0 )
    {
        dwWriteCount = MIN( dwRemainCount , FILESYSTEM_CLUSTERSIZE );
        if( kWriteFile( pbBuffer, 1, dwWriteCount, pstFile ) != dwWriteCount )
        {
            kFreeMemory( pbBuffer );
            return FALSE;
        }
        dwRemainCount -= dwWriteCount;
    }
    kFreeMemory( pbBuffer );
    return TRUE;
}

/**
 *  ÆÄÀÏ Æ÷ÀÎÅÍÀÇ À§Ä¡žŠ ÀÌµ¿
 */
int kSeekFile( FILE* pstFile, int iOffset, int iOrigin )
{
    DWORD dwRealOffset;
    DWORD dwClusterOffsetToMove;
    DWORD dwCurrentClusterOffset;
    DWORD dwLastClusterOffset;
    DWORD dwMoveCount;
    DWORD i;
    DWORD dwStartClusterIndex;
    DWORD dwPreviousClusterIndex;
    DWORD dwCurrentClusterIndex;
    FILEHANDLE* pstFileHandle;

    // ÇÚµéÀÌ ÆÄÀÏ ÅžÀÔÀÌ ŸÆŽÏžé ³ª°š
    if( ( pstFile == NULL ) ||
        ( pstFile->bType != FILESYSTEM_TYPE_FILE ) )
    {
        return 0;
    }
    pstFileHandle = &( pstFile->stFileHandle );
    
    //==========================================================================
    // Origin°ú OffsetÀ» Á¶ÇÕÇÏ¿© ÆÄÀÏ œÃÀÛÀ» ±âÁØÀž·Î ÆÄÀÏ Æ÷ÀÎÅÍžŠ ¿Å°ÜŸß ÇÒ À§Ä¡žŠ 
    // °è»ê
    //==========================================================================
    // ¿ÉŒÇ¿¡ µû¶óŒ­ œÇÁŠ À§Ä¡žŠ °è»ê
    // ÀœŒöÀÌžé ÆÄÀÏÀÇ œÃÀÛ ¹æÇâÀž·Î ÀÌµ¿ÇÏžç ŸçŒöÀÌžé ÆÄÀÏÀÇ ³¡ ¹æÇâÀž·Î ÀÌµ¿
    switch( iOrigin )
    {
    // ÆÄÀÏÀÇ œÃÀÛÀ» ±âÁØÀž·Î ÀÌµ¿
    case FILESYSTEM_SEEK_SET:
        // ÆÄÀÏÀÇ Ã³ÀœÀÌ¹Ç·Î ÀÌµ¿ÇÒ ¿ÀÇÁŒÂÀÌ ÀœŒöÀÌžé 0Àž·Î Œ³Á€
        if( iOffset <= 0 )
        {
            dwRealOffset = 0;
        }
        else
        {
            dwRealOffset = iOffset;
        }
        break;

    // ÇöÀç À§Ä¡žŠ ±âÁØÀž·Î ÀÌµ¿
    case FILESYSTEM_SEEK_CUR:
        // ÀÌµ¿ÇÒ ¿ÀÇÁŒÂÀÌ ÀœŒöÀÌ°í ÇöÀç ÆÄÀÏ Æ÷ÀÎÅÍÀÇ °ªºžŽÙ Å©ŽÙžé
        // Žõ ÀÌ»ó °¥ Œö ŸøÀž¹Ç·Î ÆÄÀÏÀÇ Ã³ÀœÀž·Î ÀÌµ¿
        if( ( iOffset < 0 ) && 
            ( pstFileHandle->dwCurrentOffset <= ( DWORD ) -iOffset ) )
        {
            dwRealOffset = 0;
        }
        else
        {
            dwRealOffset = pstFileHandle->dwCurrentOffset + iOffset;
        }
        break;

    // ÆÄÀÏÀÇ ³¡ºÎºÐÀ» ±âÁØÀž·Î ÀÌµ¿
    case FILESYSTEM_SEEK_END:
        // ÀÌµ¿ÇÒ ¿ÀÇÁŒÂÀÌ ÀœŒöÀÌ°í ÇöÀç ÆÄÀÏ Æ÷ÀÎÅÍÀÇ °ªºžŽÙ Å©ŽÙžé 
        // Žõ ÀÌ»ó °¥ Œö ŸøÀž¹Ç·Î ÆÄÀÏÀÇ Ã³ÀœÀž·Î ÀÌµ¿
        if( ( iOffset < 0 ) && 
            ( pstFileHandle->dwFileSize <= ( DWORD ) -iOffset ) )
        {
            dwRealOffset = 0;
        }
        else
        {
            dwRealOffset = pstFileHandle->dwFileSize + iOffset;
        }
        break;
    }

    //==========================================================================
    // ÆÄÀÏÀ» ±žŒºÇÏŽÂ Å¬·¯œºÅÍÀÇ °³Œö¿Í ÇöÀç ÆÄÀÏ Æ÷ÀÎÅÍÀÇ À§Ä¡žŠ °í·ÁÇÏ¿©
    // ¿Å°ÜÁú ÆÄÀÏ Æ÷ÀÎÅÍ°¡ À§Ä¡ÇÏŽÂ Å¬·¯œºÅÍ±îÁö Å¬·¯œºÅÍ žµÅ©žŠ Åœ»ö
    //==========================================================================
    // ÆÄÀÏÀÇ ž¶Áöž· Å¬·¯œºÅÍÀÇ ¿ÀÇÁŒÂ
    dwLastClusterOffset = pstFileHandle->dwFileSize / FILESYSTEM_CLUSTERSIZE;
    // ÆÄÀÏ Æ÷ÀÎÅÍ°¡ ¿Å°ÜÁú À§Ä¡ÀÇ Å¬·¯œºÅÍ ¿ÀÇÁŒÂ
    dwClusterOffsetToMove = dwRealOffset / FILESYSTEM_CLUSTERSIZE;
    // ÇöÀç ÆÄÀÏ Æ÷ÀÎÅÍ°¡ ÀÖŽÂ Å¬·¯œºÅÍÀÇ ¿ÀÇÁŒÂ
    dwCurrentClusterOffset = pstFileHandle->dwCurrentOffset / FILESYSTEM_CLUSTERSIZE;

    // ÀÌµ¿ÇÏŽÂ Å¬·¯œºÅÍÀÇ À§Ä¡°¡ ÆÄÀÏÀÇ ž¶Áöž· Å¬·¯œºÅÍÀÇ ¿ÀÇÁŒÂÀ» ³ÑŸîŒ­žé
    // ÇöÀç Å¬·¯œºÅÍ¿¡Œ­ ž¶Áöž·±îÁö ÀÌµ¿ÇÑ ÈÄ, Write ÇÔŒöžŠ ÀÌ¿ëÇØŒ­ °ø¹éÀž·Î ³ªžÓÁöžŠ
    // Ã€¿ò
    if( dwLastClusterOffset < dwClusterOffsetToMove )
    {
        dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
        dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
    }
    // ÀÌµ¿ÇÏŽÂ Å¬·¯œºÅÍÀÇ À§Ä¡°¡ ÇöÀç Å¬·¯œºÅÍ¿Í °°°Å³ª ŽÙÀœ¿¡ À§Ä¡ÇØ
    // ÀÖŽÙžé ÇöÀç Å¬·¯œºÅÍžŠ ±âÁØÀž·Î Â÷ÀÌžžÅ­žž ÀÌµ¿ÇÏžé µÈŽÙ.
    else if( dwCurrentClusterOffset <= dwClusterOffsetToMove )
    {
        dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
        dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
    }
    // ÀÌµ¿ÇÏŽÂ Å¬·¯œºÅÍÀÇ À§Ä¡°¡ ÇöÀç Å¬·¯œºÅÍ ÀÌÀü¿¡ ÀÖŽÙžé, Ã¹ ¹øÂ° Å¬·¯œºÅÍºÎÅÍ
    // ÀÌµ¿ÇÏžéŒ­ °Ë»ö
    else
    {
        dwMoveCount = dwClusterOffsetToMove;
        dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
    }

    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );

    // Å¬·¯œºÅÍžŠ ÀÌµ¿
    dwCurrentClusterIndex = dwStartClusterIndex;
    for( i = 0 ; i < dwMoveCount ; i++ )
    {
        // °ªÀ» ºž°ü
        dwPreviousClusterIndex = dwCurrentClusterIndex;
        
        // ŽÙÀœ Å¬·¯œºÅÍÀÇ ÀÎµŠœºžŠ ÀÐÀœ
        if( kGetClusterLinkData( dwPreviousClusterIndex, &dwCurrentClusterIndex ) ==
            FALSE )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return -1;
        }
    }

    // Å¬·¯œºÅÍžŠ ÀÌµ¿ÇßÀžžé Å¬·¯œºÅÍ Á€ºžžŠ °»œÅ
    if( dwMoveCount > 0 )
    {
        pstFileHandle->dwPreviousClusterIndex = dwPreviousClusterIndex;
        pstFileHandle->dwCurrentClusterIndex = dwCurrentClusterIndex;
    }
    // Ã¹ ¹øÂ° Å¬·¯œºÅÍ·Î ÀÌµ¿ÇÏŽÂ °æ¿ìŽÂ ÇÚµéÀÇ Å¬·¯œºÅÍ °ªÀ» œÃÀÛ Å¬·¯œºÅÍ·Î Œ³Á€
    else if( dwStartClusterIndex == pstFileHandle->dwStartClusterIndex )
    {
        pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwStartClusterIndex;
        pstFileHandle->dwCurrentClusterIndex = pstFileHandle->dwStartClusterIndex;
    }
    
    //==========================================================================
    // ÆÄÀÏ Æ÷ÀÎÅÍžŠ °»œÅÇÏ°í ÆÄÀÏ ¿ÀÇÁŒÂÀÌ ÆÄÀÏÀÇ Å©±âžŠ ³ÑŸúŽÙžé ³ªžÓÁö ºÎºÐÀ»
    // 0Àž·Î Ã€¿öŒ­ ÆÄÀÏÀÇ Å©±âžŠ ŽÃž²
    //==========================================================================
    // œÇÁŠ ÆÄÀÏÀÇ Å©±âžŠ ³ÑŸîŒ­ ÆÄÀÏ Æ÷ÀÎÅÍ°¡ ÀÌµ¿ÇßŽÙžé, ÆÄÀÏ ³¡¿¡Œ­ºÎÅÍ 
    // ³²Àº Å©±âžžÅ­ 0Àž·Î Ã€¿öÁÜ
    if( dwLastClusterOffset < dwClusterOffsetToMove )
    {
        pstFileHandle->dwCurrentOffset = pstFileHandle->dwFileSize;
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );

        // ³²Àº Å©±âžžÅ­ 0Àž·Î Ã€¿ò
        if( kWriteZero( pstFile, dwRealOffset - pstFileHandle->dwFileSize )
                == FALSE )
        {
            return 0;
        }
    }

    pstFileHandle->dwCurrentOffset = dwRealOffset;

    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );

    return 0;    
}

/**
 *  ÆÄÀÏÀ» ŽÝÀœ
 */
int kCloseFile( FILE* pstFile )
{
    // ÇÚµé ÅžÀÔÀÌ ÆÄÀÏÀÌ ŸÆŽÏžé œÇÆÐ
    if( ( pstFile == NULL ) ||
        ( pstFile->bType != FILESYSTEM_TYPE_FILE ) )
    {
        return -1;
    }
    
    // ÇÚµéÀ» ¹ÝÈ¯
    kFreeFileDirectoryHandle( pstFile );
    return 0;
}

/**
 *  ÇÚµé Ç®À» °Ë»çÇÏ¿© ÆÄÀÏÀÌ ¿­·ÁÀÖŽÂÁöžŠ È®ÀÎ
 */
BOOL kIsFileOpened( const DIRECTORYENTRY* pstEntry )
{
    int i;
    FILE* pstFile;
    
    // ÇÚµé Ç®ÀÇ œÃÀÛ Ÿîµå·¹œººÎÅÍ ³¡±îÁö ¿­ž° ÆÄÀÏžž °Ë»ö
    pstFile = gs_stFileSystemManager.pstHandlePool;
    for( i = 0 ; i < FILESYSTEM_HANDLE_MAXCOUNT ; i++ )
    {
        // ÆÄÀÏ ÅžÀÔ Áß¿¡Œ­ œÃÀÛ Å¬·¯œºÅÍ°¡ ÀÏÄ¡ÇÏžé ¹ÝÈ¯
        if( ( pstFile[ i ].bType == FILESYSTEM_TYPE_FILE ) &&
            ( pstFile[ i ].stFileHandle.dwStartClusterIndex == 
              pstEntry->dwStartClusterIndex ) )
        {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 *  ÆÄÀÏÀ» »èÁŠ
 */
int kRemoveFile( const char* pcFileName )
{
    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset;
    int iFileNameLength;

    // ÆÄÀÏ ÀÌž§ °Ë»ç
    iFileNameLength = kStrLen( pcFileName );
    if( ( iFileNameLength > ( sizeof( stEntry.vcFileName ) - 1 ) ) || 
        ( iFileNameLength == 0 ) )
    {
        return NULL;
    }
    
    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );
    
    // ÆÄÀÏÀÌ ÁžÀçÇÏŽÂ°¡ È®ÀÎ
    iDirectoryEntryOffset = kFindDirectoryEntry( pcFileName, &stEntry );
    if( iDirectoryEntryOffset == -1 ) 
    { 
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return -1;
    }
    
    // ŽÙž¥ ÅÂœºÅ©¿¡Œ­ ÇØŽç ÆÄÀÏÀ» ¿­°í ÀÖŽÂÁö ÇÚµé Ç®À» °Ë»öÇÏ¿© È®ÀÎ
    // ÆÄÀÏÀÌ ¿­·Á ÀÖÀžžé »èÁŠÇÒ Œö ŸøÀœ
    if( kIsFileOpened( &stEntry ) == TRUE )
    {
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return -1;
    }
    
    // ÆÄÀÏÀ» ±žŒºÇÏŽÂ Å¬·¯œºÅÍžŠ žðµÎ ÇØÁŠ
    if( kFreeClusterUntilEnd( stEntry.dwStartClusterIndex ) == FALSE )
    { 
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return -1;
    }

    // µð·ºÅÍž® ¿£Æ®ž®žŠ ºó °ÍÀž·Î Œ³Á€
    kMemSet( &stEntry, 0, sizeof( stEntry ) );
    if( kSetDirectoryEntryData( iDirectoryEntryOffset, &stEntry ) == FALSE )
    {
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return -1;
    }
    
    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    return 0;
}

/**
 *  µð·ºÅÍž®žŠ ¿®
 */
DIR* kOpenDirectory( const char* pcDirectoryName )
{
    DIR* pstDirectory;
    DIRECTORYENTRY* pstDirectoryBuffer;

    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset;
    int iFileNameLength;
    DWORD dwSecondCluster;
    FILE* pstFile;
    
    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );
    
    
    //==========================================================================
    // ÆÄÀÏÀÌ žÕÀú ÁžÀçÇÏŽÂ°¡ È®ÀÎÇÏ°í, ŸøŽÙžé ¿ÉŒÇÀ» ºž°í ÆÄÀÏÀ» »ýŒº
    //==========================================================================

    iDirectoryEntryOffset = kFindDirectoryEntry( pcDirectoryName, &stEntry );
    if( iDirectoryEntryOffset == -1 && kMemCmp(pcDirectoryName,"/",2)!= 0)
    {
       
        // ³ªžÓÁö ¿ÉŒÇµéÀº ÆÄÀÏÀ» »ýŒº
        if( kCreateDirectory( pcDirectoryName, &stEntry, &iDirectoryEntryOffset ) == FALSE )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return NULL;
        }
    }    
    
//------------------------------------------------------------------------------------
    // ·çÆ® µð·ºÅÍž® ¹Û¿¡ ŸøÀž¹Ç·Î µð·ºÅÍž® ÀÌž§Àº ¹«œÃÇÏ°í ÇÚµéžž ÇÒŽç¹ÞŸÆŒ­ ¹ÝÈ¯
    pstDirectory = kAllocateFileDirectoryHandle();
    if( pstDirectory == NULL )
    {
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return NULL;
    }
    
    // ·çÆ® µð·ºÅÍž®žŠ ÀúÀåÇÒ ¹öÆÛžŠ ÇÒŽç
    pstDirectoryBuffer = ( DIRECTORYENTRY* ) kAllocateMemory( FILESYSTEM_CLUSTERSIZE );
    if( pstDirectory == NULL )
    {
        // œÇÆÐÇÏžé ÇÚµéÀ» ¹ÝÈ¯ÇØŸß ÇÔ
        kFreeFileDirectoryHandle( pstDirectory );
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return NULL;
    }
    
    // ·çÆ® µð·ºÅÍž®žŠ ÀÐÀœ
    if( kReadCluster( 0, ( BYTE* ) pstDirectoryBuffer ) == FALSE )
    {
        // œÇÆÐÇÏžé ÇÚµé°ú žÞžðž®žŠ žðµÎ ¹ÝÈ¯ÇØŸß ÇÔ
        kFreeFileDirectoryHandle( pstDirectory );
        kFreeMemory( pstDirectoryBuffer );
        
        // µ¿±âÈ­
        kUnlock( &( gs_stFileSystemManager.stMutex ) );
        return NULL;
        
    }
    
    // µð·ºÅÍž® ÅžÀÔÀž·Î Œ³Á€ÇÏ°í ÇöÀç µð·ºÅÍž® ¿£Æ®ž®ÀÇ ¿ÀÇÁŒÂÀ» ÃÊ±âÈ­
    pstDirectory->bType = FILESYSTEM_TYPE_DIRECTORY;
    pstDirectory->stDirectoryHandle.iCurrentOffset = 0;
    pstDirectory->stDirectoryHandle.pstDirectoryBuffer = pstDirectoryBuffer;

    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    return pstDirectory;
}

/**
 *  µð·ºÅÍž® ¿£Æ®ž®žŠ ¹ÝÈ¯ÇÏ°í ŽÙÀœÀž·Î ÀÌµ¿
 */
struct kDirectoryEntryStruct* kReadDirectory( DIR* pstDirectory )
{
    DIRECTORYHANDLE* pstDirectoryHandle;
    DIRECTORYENTRY* pstEntry;
    
    // ÇÚµé ÅžÀÔÀÌ µð·ºÅÍž®°¡ ŸÆŽÏžé œÇÆÐ
    if( ( pstDirectory == NULL ) ||
        ( pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY ) )
    {
        return NULL;
    }
    pstDirectoryHandle = &( pstDirectory->stDirectoryHandle );
    
    // ¿ÀÇÁŒÂÀÇ ¹üÀ§°¡ Å¬·¯œºÅÍ¿¡ ÁžÀçÇÏŽÂ ÃÖŽñ°ªÀ» ³ÑŸîŒ­žé œÇÆÐ
    if( ( pstDirectoryHandle->iCurrentOffset < 0 ) ||
        ( pstDirectoryHandle->iCurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT ) )
    {
        return NULL;
    }

    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );
    
    // ·çÆ® µð·ºÅÍž®¿¡ ÀÖŽÂ ÃÖŽë µð·ºÅÍž® ¿£Æ®ž®ÀÇ °³ŒöžžÅ­ °Ë»ö
    pstEntry = pstDirectoryHandle->pstDirectoryBuffer;
    while( pstDirectoryHandle->iCurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT )
    {
        // ÆÄÀÏÀÌ ÁžÀçÇÏžé ÇØŽç µð·ºÅÍž® ¿£Æ®ž®žŠ ¹ÝÈ¯
        if( pstEntry[ pstDirectoryHandle->iCurrentOffset ].dwStartClusterIndex
                != 0 )
        {
            // µ¿±âÈ­
            kUnlock( &( gs_stFileSystemManager.stMutex ) );
            return &( pstEntry[ pstDirectoryHandle->iCurrentOffset++ ] );
        }
        
        pstDirectoryHandle->iCurrentOffset++;
    }

    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
    return NULL;
}

/**
 *  µð·ºÅÍž® Æ÷ÀÎÅÍžŠ µð·ºÅÍž®ÀÇ Ã³ÀœÀž·Î ÀÌµ¿
 */
void kRewindDirectory( DIR* pstDirectory )
{
    DIRECTORYHANDLE* pstDirectoryHandle;
    
    // ÇÚµé ÅžÀÔÀÌ µð·ºÅÍž®°¡ ŸÆŽÏžé œÇÆÐ
    if( ( pstDirectory == NULL ) ||
        ( pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY ) )
    {
        return ;
    }
    pstDirectoryHandle = &( pstDirectory->stDirectoryHandle );
    
    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );
    
    // µð·ºÅÍž® ¿£Æ®ž®ÀÇ Æ÷ÀÎÅÍžž 0Àž·Î ¹Ù²ãÁÜ
    pstDirectoryHandle->iCurrentOffset = 0;
    
    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );
}


/**
 *  ¿­ž° µð·ºÅÍž®žŠ ŽÝÀœ
 */
int kCloseDirectory( DIR* pstDirectory )
{
    DIRECTORYHANDLE* pstDirectoryHandle;
    
    // ÇÚµé ÅžÀÔÀÌ µð·ºÅÍž®°¡ ŸÆŽÏžé œÇÆÐ
    if( ( pstDirectory == NULL ) ||
        ( pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY ) )
    {
        return -1;
    }
    pstDirectoryHandle = &( pstDirectory->stDirectoryHandle );

    // µ¿±âÈ­
    kLock( &( gs_stFileSystemManager.stMutex ) );
    
    // ·çÆ® µð·ºÅÍž®ÀÇ ¹öÆÛžŠ ÇØÁŠÇÏ°í ÇÚµéÀ» ¹ÝÈ¯
    kFreeMemory( pstDirectoryHandle->pstDirectoryBuffer );
    kFreeFileDirectoryHandle( pstDirectory );    
    
    // µ¿±âÈ­
    kUnlock( &( gs_stFileSystemManager.stMutex ) );

    return 0;
}
