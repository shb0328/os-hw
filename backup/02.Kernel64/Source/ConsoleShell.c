/**
 *  file    ConsoleShell.c
 *  date    2009/01/31
 *  author  kkamagui 
 *          Copyright(c)2008 All rights reserved by kkamagui
 *  brief   ÄÜŒÖ ŒÐ¿¡ °ü·ÃµÈ ŒÒœº ÆÄÀÏ
 */

#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"

// Ä¿žÇµå Å×ÀÌºí Á€ÀÇ
SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
        { "help", "Show Help", kHelp },
        { "cls", "Clear Screen", kCls },
        { "totalram", "Show Total RAM Size", kShowTotalRAMSize },
        { "strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest },
        { "shutdown", "Shutdown And Reboot OS", kShutdown },
        { "settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)", 
                kSetTimer },
        { "wait", "Wait ms Using PIT, ex)wait 100(ms)", kWaitUsingPIT },
        { "rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter },
        { "cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed },
        { "date", "Show Date And Time", kShowDateAndTime },
        { "createtask", "Create Task, ex)createtask 1(type) 10(count)", kCreateTestTask },
        { "changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)",
                kChangeTaskPriority },
        { "tasklist", "Show Task List", kShowTaskList },
        { "killtask", "End Task, ex)killtask 1(ID) or 0xffffffff(All Task)", kKillTask },
        { "cpuload", "Show Processor Load", kCPULoad },
        { "testmutex", "Test Mutex Function", kTestMutex },
        { "testthread", "Test Thread And Process Function", kTestThread },
        { "showmatrix", "Show Matrix Screen", kShowMatrix },
        { "testpie", "Test PIE Calculation", kTestPIE },               
        { "dynamicmeminfo", "Show Dyanmic Memory Information", kShowDyanmicMemoryInformation },
        { "testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation },
        { "testranalloc", "Test Random Allocation & Free", kTestRandomAllocation },
        { "hddinfo", "Show HDD Information", kShowHDDInformation },
        { "readsector", "Read HDD Sector, ex)readsector 0(LBA) 10(count)", 
                kReadSector },
        { "writesector", "Write HDD Sector, ex)writesector 0(LBA) 10(count)", 
                kWriteSector },
        { "mounthdd", "Mount HDD", kMountHDD },
        { "formathdd", "Format HDD", kFormatHDD },
        { "filesysteminfo", "Show File System Information", kShowFileSystemInformation },
        { "createfile", "Create File, ex)createfile a.txt", kCreateFileInRootDirectory },
        { "deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory },
        { "dir", "Show Directory", kShowDirectory },
        { "writefile", "Write Data To File, ex) writefile a.txt", kWriteDataToFile },
        { "readfile", "Read Data From File, ex) readfile a.txt", kReadDataFromFile },
        { "testfileio", "Test File I/O Function", kTestFileIO },
        { "mkdir", "Make Directory, ex) mkdir folder", kMakeDirectory},
        { "cd", "Move Directory, ex) cd folder", kMoveDirectory},
        { "rmdir", "Remove emptyed Directory ex) rmdir folder", kRemoveDirectory},
};                                     


char path[100] = "/";
DWORD currentDirectoryClusterIndex = 0;
   
void kStartConsoleShell( void )
{
    char vcCommandBuffer[ CONSOLESHELL_MAXCOMMANDBUFFERCOUNT ];
    int iCommandBufferIndex = 0;
    BYTE bKey;
    int iCursorX, iCursorY;
 

  
    // ÇÁ·ÒÇÁÆ® Ãâ·Â
    kPrintf(CONSOLESHELL_PROMPTMESSAGE);
    kPrintf(path);
    kPrintf(">");
    
    kSetClusterIndex(currentDirectoryClusterIndex);
    while( 1 )
    {
        // Å°°¡ ŒöœÅµÉ ¶§±îÁö Žë±â
        bKey = kGetCh();
        // Backspace Å° Ã³ž®
        if( bKey == KEY_BACKSPACE )
        {
            if( iCommandBufferIndex > 0 )
            {
                // ÇöÀç Ä¿Œ­ À§Ä¡žŠ ŸòŸîŒ­ ÇÑ ¹®ÀÚ ŸÕÀž·Î ÀÌµ¿ÇÑ ŽÙÀœ °ø¹éÀ» Ãâ·ÂÇÏ°í 
                // Ä¿žÇµå ¹öÆÛ¿¡Œ­ ž¶Áöž· ¹®ÀÚ »èÁŠ
                kGetCursor( &iCursorX, &iCursorY );
                kPrintStringXY( iCursorX - 1, iCursorY, " " );
                kSetCursor( iCursorX - 1, iCursorY );
                iCommandBufferIndex--;
            }
        }
        // ¿£ÅÍ Å° Ã³ž®
        else if( bKey == KEY_ENTER )
        {
            kPrintf( "\n" );
            
            if( iCommandBufferIndex > 0 )
            {
                // Ä¿žÇµå ¹öÆÛ¿¡ ÀÖŽÂ ží·ÉÀ» œÇÇà
                vcCommandBuffer[ iCommandBufferIndex ] = '\0';
                kExecuteCommand( vcCommandBuffer );
            }
            
            // ÇÁ·ÒÇÁÆ® Ãâ·Â ¹× Ä¿žÇµå ¹öÆÛ ÃÊ±âÈ­
            kPrintf( "%s", CONSOLESHELL_PROMPTMESSAGE );
            kPrintf(path);
            kPrintf(">");            
            kMemSet( vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT );
            iCommandBufferIndex = 0;
        }
        // œÃÇÁÆ® Å°, CAPS Lock, NUM Lock, Scroll LockÀº ¹«œÃ
        else if( ( bKey == KEY_LSHIFT ) || ( bKey == KEY_RSHIFT ) ||
                 ( bKey == KEY_CAPSLOCK ) || ( bKey == KEY_NUMLOCK ) ||
                 ( bKey == KEY_SCROLLLOCK ) )
        {
            ;
        }
        else
        {
            // TABÀº °ø¹éÀž·Î ÀüÈ¯
            if( bKey == KEY_TAB )
            {
                bKey = ' ';
            }
            
            // ¹öÆÛ¿¡ °ø°£ÀÌ ³²ŸÆÀÖÀ» ¶§žž °¡ŽÉ
            if( iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT )
            {
                vcCommandBuffer[ iCommandBufferIndex++ ] = bKey;
                kPrintf( "%c", bKey );
            }
        }
    }
}

/*
 *  Ä¿žÇµå ¹öÆÛ¿¡ ÀÖŽÂ Ä¿žÇµåžŠ ºñ±³ÇÏ¿© ÇØŽç Ä¿žÇµåžŠ Ã³ž®ÇÏŽÂ ÇÔŒöžŠ ŒöÇà
 */
void kExecuteCommand( const char* pcCommandBuffer )
{
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;
    
    // °ø¹éÀž·Î ±žºÐµÈ Ä¿žÇµåžŠ ÃßÃâ
    iCommandBufferLength = kStrLen( pcCommandBuffer );
    for( iSpaceIndex = 0 ; iSpaceIndex < iCommandBufferLength ; iSpaceIndex++ )
    {
        if( pcCommandBuffer[ iSpaceIndex ] == ' ' )
        {
            break;
        }
    }
    
    // Ä¿žÇµå Å×ÀÌºíÀ» °Ë»çÇØŒ­ µ¿ÀÏÇÑ ÀÌž§ÀÇ Ä¿žÇµå°¡ ÀÖŽÂÁö È®ÀÎ
    iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY );
    for( i = 0 ; i < iCount ; i++ )
    {
        iCommandLength = kStrLen( gs_vstCommandTable[ i ].pcCommand );
        // Ä¿žÇµåÀÇ ±æÀÌ¿Í ³»¿ëÀÌ ¿ÏÀüÈ÷ ÀÏÄ¡ÇÏŽÂÁö °Ë»ç
        if( ( iCommandLength == iSpaceIndex ) &&
            ( kMemCmp( gs_vstCommandTable[ i ].pcCommand, pcCommandBuffer,
                       iSpaceIndex ) == 0 ) )
        {
            gs_vstCommandTable[ i ].pfFunction( pcCommandBuffer + iSpaceIndex + 1 );
            break;
        }
    }

    // ž®œºÆ®¿¡Œ­ Ã£À» Œö ŸøŽÙžé ¿¡·¯ Ãâ·Â
    if( i >= iCount )
    {
        kPrintf( "'%s' is not found.\n", pcCommandBuffer );
    }
}

/**
 *  ÆÄ¶ó¹ÌÅÍ ÀÚ·á±žÁ¶žŠ ÃÊ±âÈ­
 */
void kInitializeParameter( PARAMETERLIST* pstList, const char* pcParameter )
{
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen( pcParameter );
    pstList->iCurrentPosition = 0;
}

/**
 *  °ø¹éÀž·Î ±žºÐµÈ ÆÄ¶ó¹ÌÅÍÀÇ ³»¿ë°ú ±æÀÌžŠ ¹ÝÈ¯
 */
int kGetNextParameter( PARAMETERLIST* pstList, char* pcParameter )
{
    int i;
    int iLength;

    // Žõ ÀÌ»ó ÆÄ¶ó¹ÌÅÍ°¡ ŸøÀžžé ³ª°š
    if( pstList->iLength <= pstList->iCurrentPosition )
    {
        return 0;
    }
    
    // ¹öÆÛÀÇ ±æÀÌžžÅ­ ÀÌµ¿ÇÏžéŒ­ °ø¹éÀ» °Ë»ö
    for( i = pstList->iCurrentPosition ; i < pstList->iLength ; i++ )
    {
        if( pstList->pcBuffer[ i ] == ' ' )
        {
            break;
        }
    }
    
    // ÆÄ¶ó¹ÌÅÍžŠ º¹»çÇÏ°í ±æÀÌžŠ ¹ÝÈ¯
    kMemCpy( pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i );
    iLength = i - pstList->iCurrentPosition;
    pcParameter[ iLength ] = '\0';

    // ÆÄ¶ó¹ÌÅÍÀÇ À§Ä¡ Ÿ÷µ¥ÀÌÆ®
    pstList->iCurrentPosition += iLength + 1;
    return iLength;
}
    
//==============================================================================
//  Ä¿žÇµåžŠ Ã³ž®ÇÏŽÂ ÄÚµå
//==============================================================================
/**
 *  ŒÐ µµ¿òž»À» Ãâ·Â
 */
static void kHelp( const char* pcCommandBuffer )
{
    int i;
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;
    
    
    kPrintf( "=========================================================\n" );
    kPrintf( "                    MINT64 Shell Help                    \n" );
    kPrintf( "=========================================================\n" );
    
    iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY );

    // °¡Àå ±ä Ä¿žÇµåÀÇ ±æÀÌžŠ °è»ê
    for( i = 0 ; i < iCount ; i++ )
    {
        iLength = kStrLen( gs_vstCommandTable[ i ].pcCommand );
        if( iLength > iMaxCommandLength )
        {
            iMaxCommandLength = iLength;
        }
    }
    
    // µµ¿òž» Ãâ·Â
    for( i = 0 ; i < iCount ; i++ )
    {
        kPrintf( "%s", gs_vstCommandTable[ i ].pcCommand );
        kGetCursor( &iCursorX, &iCursorY );
        kSetCursor( iMaxCommandLength, iCursorY );
        kPrintf( "  - %s\n", gs_vstCommandTable[ i ].pcHelp );

        // žñ·ÏÀÌ ž¹À» °æ¿ì ³ªŽ²Œ­ ºž¿©ÁÜ
        if( ( i != 0 ) && ( ( i % 20 ) == 0 ) )
        {
            kPrintf( "Press any key to continue... ('q' is exit) : " );
            if( kGetCh() == 'q' )
            {
                kPrintf( "\n" );
                break;
            }        
            kPrintf( "\n" );
        }
    }
}

/**
 *  È­žéÀ» Áö¿ò 
 */
static void kCls( const char* pcParameterBuffer )
{
    // žÇ À­ÁÙÀº µð¹ö±ë ¿ëÀž·Î »ç¿ëÇÏ¹Ç·Î È­žéÀ» Áö¿î ÈÄ, ¶óÀÎ 1·Î Ä¿Œ­ ÀÌµ¿
    kClearScreen();
    kSetCursor( 0, 1 );
}

/**
 *  ÃÑ žÞžðž® Å©±âžŠ Ãâ·Â
 */
static void kShowTotalRAMSize( const char* pcParameterBuffer )
{
    kPrintf( "Total RAM Size = %d MB\n", kGetTotalRAMSize() );
}

/**
 *  ¹®ÀÚ¿­·Î µÈ ŒýÀÚžŠ ŒýÀÚ·Î º¯È¯ÇÏ¿© È­žé¿¡ Ãâ·Â
 */
static void kStringToDecimalHexTest( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;
    
    // ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
    kInitializeParameter( &stList, pcParameterBuffer );
    
    while( 1 )
    {
        // ŽÙÀœ ÆÄ¶ó¹ÌÅÍžŠ ±žÇÔ, ÆÄ¶ó¹ÌÅÍÀÇ ±æÀÌ°¡ 0ÀÌžé ÆÄ¶ó¹ÌÅÍ°¡ ŸøŽÂ °ÍÀÌ¹Ç·Î
        // ÁŸ·á
        iLength = kGetNextParameter( &stList, vcParameter );
        if( iLength == 0 )
        {
            break;
        }

        // ÆÄ¶ó¹ÌÅÍ¿¡ ŽëÇÑ Á€ºžžŠ Ãâ·ÂÇÏ°í 16ÁøŒöÀÎÁö 10ÁøŒöÀÎÁö ÆÇŽÜÇÏ¿© º¯È¯ÇÑ ÈÄ
        // °á°úžŠ printf·Î Ãâ·Â
        kPrintf( "Param %d = '%s', Length = %d, ", iCount + 1, 
                 vcParameter, iLength );

        // 0x·Î œÃÀÛÇÏžé 16ÁøŒö, ±×¿ÜŽÂ 10ÁøŒö·Î ÆÇŽÜ
        if( kMemCmp( vcParameter, "0x", 2 ) == 0 )
        {
            lValue = kAToI( vcParameter + 2, 16 );
            kPrintf( "HEX Value = %q\n", lValue );
        }
        else
        {
            lValue = kAToI( vcParameter, 10 );
            kPrintf( "Decimal Value = %d\n", lValue );
        }
        
        iCount++;
    }
}

/**
 *  PCžŠ ÀçœÃÀÛ(Reboot)
 */
static void kShutdown( const char* pcParamegerBuffer )
{
    kPrintf( "System Shutdown Start...\n" );
    
    // Å°ºžµå ÄÁÆ®·Ñ·¯žŠ ÅëÇØ PCžŠ ÀçœÃÀÛ
    kPrintf( "Press Any Key To Reboot PC..." );
    kGetCh();
    kReboot();
}

/**
 *  PIT ÄÁÆ®·Ñ·¯ÀÇ Ä«¿îÅÍ 0 Œ³Á€
 */
static void kSetTimer( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    PARAMETERLIST stList;
    long lValue;
    BOOL bPeriodic;

    // ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
    kInitializeParameter( &stList, pcParameterBuffer );
    
    // milisecond ÃßÃâ
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)settimer 10(ms) 1(periodic)\n" );
        return ;
    }
    lValue = kAToI( vcParameter, 10 );

    // Periodic ÃßÃâ
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)settimer 10(ms) 1(periodic)\n" );
        return ;
    }    
    bPeriodic = kAToI( vcParameter, 10 );
    
    kInitializePIT( MSTOCOUNT( lValue ), bPeriodic );
    kPrintf( "Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic );
}

/**
 *  PIT ÄÁÆ®·Ñ·¯žŠ Á÷Á¢ »ç¿ëÇÏ¿© ms µ¿ŸÈ Žë±â  
 */
static void kWaitUsingPIT( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;
    int i;
    
    // ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
    kInitializeParameter( &stList, pcParameterBuffer );
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)wait 100(ms)\n" );
        return ;
    }
    
    lMillisecond = kAToI( pcParameterBuffer, 10 );
    kPrintf( "%d ms Sleep Start...\n", lMillisecond );
    
    // ÀÎÅÍ·ŽÆ®žŠ ºñÈ°ŒºÈ­ÇÏ°í PIT ÄÁÆ®·Ñ·¯žŠ ÅëÇØ Á÷Á¢ œÃ°£À» ÃøÁ€
    kDisableInterrupt();
    for( i = 0 ; i < lMillisecond / 30 ; i++ )
    {
        kWaitUsingDirectPIT( MSTOCOUNT( 30 ) );
    }
    kWaitUsingDirectPIT( MSTOCOUNT( lMillisecond % 30 ) );   
    kEnableInterrupt();
    kPrintf( "%d ms Sleep Complete\n", lMillisecond );
    
    // ÅžÀÌžÓ º¹¿ø
    kInitializePIT( MSTOCOUNT( 1 ), TRUE );
}

/**
 *  ÅžÀÓ œºÅÆÇÁ Ä«¿îÅÍžŠ ÀÐÀœ  
 */
static void kReadTimeStampCounter( const char* pcParameterBuffer )
{
    QWORD qwTSC;
    
    qwTSC = kReadTSC();
    kPrintf( "Time Stamp Counter = %q\n", qwTSC );
}

/**
 *  ÇÁ·ÎŒŒŒ­ÀÇ ŒÓµµžŠ ÃøÁ€
 */
static void kMeasureProcessorSpeed( const char* pcParameterBuffer )
{
    int i;
    QWORD qwLastTSC, qwTotalTSC = 0;
        
    kPrintf( "Now Measuring." );
    
    // 10ÃÊ µ¿ŸÈ º¯È­ÇÑ ÅžÀÓ œºÅÆÇÁ Ä«¿îÅÍžŠ ÀÌ¿ëÇÏ¿© ÇÁ·ÎŒŒŒ­ÀÇ ŒÓµµžŠ °£Á¢ÀûÀž·Î ÃøÁ€
    kDisableInterrupt();    
    for( i = 0 ; i < 200 ; i++ )
    {
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT( MSTOCOUNT( 50 ) );
        qwTotalTSC += kReadTSC() - qwLastTSC;

        kPrintf( "." );
    }
    // ÅžÀÌžÓ º¹¿ø
    kInitializePIT( MSTOCOUNT( 1 ), TRUE );    
    kEnableInterrupt();
    
    kPrintf( "\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000 );
}

/**
 *  RTC ÄÁÆ®·Ñ·¯¿¡ ÀúÀåµÈ ÀÏÀÚ ¹× œÃ°£ Á€ºžžŠ Ç¥œÃ
 */
static void kShowDateAndTime( const char* pcParameterBuffer )
{
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    // RTC ÄÁÆ®·Ñ·¯¿¡Œ­ œÃ°£ ¹× ÀÏÀÚžŠ ÀÐÀœ
    kReadRTCTime( &bHour, &bMinute, &bSecond );
    kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );
    
    kPrintf( "Date: %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth,
             kConvertDayOfWeekToString( bDayOfWeek ) );
    kPrintf( "Time: %d:%d:%d\n", bHour, bMinute, bSecond );
}

/**
 *  ÅÂœºÅ© 1
 *      È­žé Å×µÎž®žŠ µ¹žéŒ­ ¹®ÀÚžŠ Ãâ·Â
 */
static void kTestTask1( void )
{
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin, j;
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    
    // ÀÚœÅÀÇ IDžŠ ŸòŸîŒ­ È­žé ¿ÀÇÁŒÂÀž·Î »ç¿ë
    pstRunningTask = kGetRunningTask();
    iMargin = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) % 10;
    
    // È­žé ³× ±ÍÅüÀÌžŠ µ¹žéŒ­ ¹®ÀÚ Ãâ·Â
    for( j = 0 ; j < 20000 ; j++ )
    {
        switch( i )
        {
        case 0:
            iX++;
            if( iX >= ( CONSOLE_WIDTH - iMargin ) )
            {
                i = 1;
            }
            break;
            
        case 1:
            iY++;
            if( iY >= ( CONSOLE_HEIGHT - iMargin ) )
            {
                i = 2;
            }
            break;
            
        case 2:
            iX--;
            if( iX < iMargin )
            {
                i = 3;
            }
            break;
            
        case 3:
            iY--;
            if( iY < iMargin )
            {
                i = 0;
            }
            break;
        }
        
        // ¹®ÀÚ ¹× »ö±ò ÁöÁ€
        pstScreen[ iY * CONSOLE_WIDTH + iX ].bCharactor = bData;
        pstScreen[ iY * CONSOLE_WIDTH + iX ].bAttribute = bData & 0x0F;
        bData++;
        
        // ŽÙž¥ ÅÂœºÅ©·Î ÀüÈ¯
        //kSchedule();
    }

    //kExitTask();
}

/**
 *  ÅÂœºÅ© 2
 *      ÀÚœÅÀÇ IDžŠ Âü°íÇÏ¿© Æ¯Á€ À§Ä¡¿¡ ÈžÀüÇÏŽÂ ¹Ù¶÷°³ºñžŠ Ãâ·Â
 */
static void kTestTask2( void )
{
    int i = 0, iOffset;
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    char vcData[ 4 ] = { '-', '\\', '|', '/' };
    
    // ÀÚœÅÀÇ IDžŠ ŸòŸîŒ­ È­žé ¿ÀÇÁŒÂÀž·Î »ç¿ë
    pstRunningTask = kGetRunningTask();
    iOffset = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - 
        ( iOffset % ( CONSOLE_WIDTH * CONSOLE_HEIGHT ) );

    while( 1 )
    {
        // ÈžÀüÇÏŽÂ ¹Ù¶÷°³ºñžŠ Ç¥œÃ
        pstScreen[ iOffset ].bCharactor = vcData[ i % 4 ];
        // »ö±ò ÁöÁ€
        pstScreen[ iOffset ].bAttribute = ( iOffset % 15 ) + 1;
        i++;
        
        // ŽÙž¥ ÅÂœºÅ©·Î ÀüÈ¯
        //kSchedule();
    }
}

/**
 *  ÅÂœºÅ©žŠ »ýŒºÇØŒ­ žÖÆŒ ÅÂœºÅ· ŒöÇà
 */
static void kCreateTestTask( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcType[ 30 ];
    char vcCount[ 30 ];
    int i;
    
    // ÆÄ¶ó¹ÌÅÍžŠ ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcType );
    kGetNextParameter( &stList, vcCount );

    switch( kAToI( vcType, 10 ) )
    {
    // ÅžÀÔ 1 ÅÂœºÅ© »ýŒº
    case 1:
        for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        {    
            if( kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask1 ) == NULL )
            {
                break;
            }
        }
        
        kPrintf( "Task1 %d Created\n", i );
        break;
        
    // ÅžÀÔ 2 ÅÂœºÅ© »ýŒº
    case 2:
    default:
        for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        {    
            if( kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 ) == NULL )
            {
                break;
            }
        }
        kPrintf( "Task2 %d Created\n", i );
        break;
    }    
}   

/**
 *  ÅÂœºÅ©ÀÇ ¿ìŒ± ŒøÀ§žŠ º¯°æ
 */
static void kChangeTaskPriority( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcID[ 30 ];
    char vcPriority[ 30 ];
    QWORD qwID;
    BYTE bPriority;
    
    // ÆÄ¶ó¹ÌÅÍžŠ ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );
    kGetNextParameter( &stList, vcPriority );
    
    // ÅÂœºÅ©ÀÇ ¿ìŒ± ŒøÀ§žŠ º¯°æ
    if( kMemCmp( vcID, "0x", 2 ) == 0 )
    {
        qwID = kAToI( vcID + 2, 16 );
    }
    else
    {
        qwID = kAToI( vcID, 10 );
    }
    
    bPriority = kAToI( vcPriority, 10 );
    
    kPrintf( "Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority );
    if( kChangePriority( qwID, bPriority ) == TRUE )
    {
        kPrintf( "Success\n" );
    }
    else
    {
        kPrintf( "Fail\n" );
    }
}

/**
 *  ÇöÀç »ýŒºµÈ žðµç ÅÂœºÅ©ÀÇ Á€ºžžŠ Ãâ·Â
 */
static void kShowTaskList( const char* pcParameterBuffer )
{
    int i;
    TCB* pstTCB;
    int iCount = 0;
    
    kPrintf( "=========== Task Total Count [%d] ===========\n", kGetTaskCount() );
    for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
    {
        // TCBžŠ ±žÇØŒ­ TCB°¡ »ç¿ë ÁßÀÌžé IDžŠ Ãâ·Â
        pstTCB = kGetTCBInTCBPool( i );
        if( ( pstTCB->stLink.qwID >> 32 ) != 0 )
        {
            // ÅÂœºÅ©°¡ 10°³ Ãâ·ÂµÉ ¶§ž¶ŽÙ, °èŒÓ ÅÂœºÅ© Á€ºžžŠ Ç¥œÃÇÒÁö ¿©ºÎžŠ È®ÀÎ
            if( ( iCount != 0 ) && ( ( iCount % 10 ) == 0 ) )
            {
                kPrintf( "Press any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    kPrintf( "\n" );
                    break;
                }
                kPrintf( "\n" );
            }
            
            kPrintf( "[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1 + iCount++,
                     pstTCB->stLink.qwID, GETPRIORITY( pstTCB->qwFlags ), 
                     pstTCB->qwFlags, kGetListCount( &( pstTCB->stChildThreadList ) ) );
            kPrintf( "    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
                    pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize );
        }
    }
}

/**
 *  ÅÂœºÅ©žŠ ÁŸ·á
 */
static void kKillTask( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcID[ 30 ];
    QWORD qwID;
    TCB* pstTCB;
    int i;
    
    // ÆÄ¶ó¹ÌÅÍžŠ ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );
    
    // ÅÂœºÅ©žŠ ÁŸ·á
    if( kMemCmp( vcID, "0x", 2 ) == 0 )
    {
        qwID = kAToI( vcID + 2, 16 );
    }
    else
    {
        qwID = kAToI( vcID, 10 );
    }
    
    // Æ¯Á€ IDžž ÁŸ·áÇÏŽÂ °æ¿ì
    if( qwID != 0xFFFFFFFF )
    {
        pstTCB = kGetTCBInTCBPool( GETTCBOFFSET( qwID ) );
        qwID = pstTCB->stLink.qwID;

        // œÃœºÅÛ Å×œºÆ®ŽÂ ÁŠ¿Ü
        if( ( ( qwID >> 32 ) != 0 ) && ( ( pstTCB->qwFlags & TASK_FLAGS_SYSTEM ) == 0x00 ) )
        {
            kPrintf( "Kill Task ID [0x%q] ", qwID );
            if( kEndTask( qwID ) == TRUE )
            {
                kPrintf( "Success\n" );
            }
            else
            {
                kPrintf( "Fail\n" );
            }
        }
        else
        {
            kPrintf( "Task does not exist or task is system task\n" );
        }
    }
    // ÄÜŒÖ ŒÐ°ú À¯ÈÞ ÅÂœºÅ©žŠ ÁŠ¿ÜÇÏ°í žðµç ÅÂœºÅ© ÁŸ·á
    else
    {
        for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
        {
            pstTCB = kGetTCBInTCBPool( i );
            qwID = pstTCB->stLink.qwID;

            // œÃœºÅÛ Å×œºÆ®ŽÂ »èÁŠ žñ·Ï¿¡Œ­ ÁŠ¿Ü
            if( ( ( qwID >> 32 ) != 0 ) && ( ( pstTCB->qwFlags & TASK_FLAGS_SYSTEM ) == 0x00 ) )
            {
                kPrintf( "Kill Task ID [0x%q] ", qwID );
                if( kEndTask( qwID ) == TRUE )
                {
                    kPrintf( "Success\n" );
                }
                else
                {
                    kPrintf( "Fail\n" );
                }
            }
        }
    }
}

/**
 *  ÇÁ·ÎŒŒŒ­ÀÇ »ç¿ë·üÀ» Ç¥œÃ
 */
static void kCPULoad( const char* pcParameterBuffer )
{
    kPrintf( "Processor Load : %d%%\n", kGetProcessorLoad() );
}
    
// ¹ÂÅØœº Å×œºÆ®¿ë ¹ÂÅØœº¿Í º¯Œö
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

/**
 *  ¹ÂÅØœºžŠ Å×œºÆ®ÇÏŽÂ ÅÂœºÅ©
 */
static void kPrintNumberTask( void )
{
    int i;
    int j;
    QWORD qwTickCount;

    // 50ms Á€µµ Žë±âÇÏ¿© ÄÜŒÖ ŒÐÀÌ Ãâ·ÂÇÏŽÂ žÞœÃÁö¿Í °ãÄ¡Áö ŸÊµµ·Ï ÇÔ
    qwTickCount = kGetTickCount();
    while( ( kGetTickCount() - qwTickCount ) < 50 )
    {
        kSchedule();
    }    
    
    // ·çÇÁžŠ µ¹žéŒ­ ŒýÀÚžŠ Ãâ·Â
    for( i = 0 ; i < 5 ; i++ )
    {
        kLock( &( gs_stMutex ) );
        kPrintf( "Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID,
                gs_qwAdder );
        
        gs_qwAdder += 1;
        kUnlock( & ( gs_stMutex ) );
    
        // ÇÁ·ÎŒŒŒ­ ŒÒžðžŠ ŽÃž®·Á°í Ãß°¡ÇÑ ÄÚµå
        for( j = 0 ; j < 30000 ; j++ ) ;
    }
    
    // žðµç ÅÂœºÅ©°¡ ÁŸ·áÇÒ ¶§±îÁö 1ÃÊ(100ms) Á€µµ Žë±â
    qwTickCount = kGetTickCount();
    while( ( kGetTickCount() - qwTickCount ) < 1000 )
    {
        kSchedule();
    }    
    
    // ÅÂœºÅ© ÁŸ·á
    //kExitTask();
}

/**
 *  ¹ÂÅØœºžŠ Å×œºÆ®ÇÏŽÂ ÅÂœºÅ© »ýŒº
 */
static void kTestMutex( const char* pcParameterBuffer )
{
    int i;
    
    gs_qwAdder = 1;
    
    // ¹ÂÅØœº ÃÊ±âÈ­
    kInitializeMutex( &gs_stMutex );
    
    for( i = 0 ; i < 3 ; i++ )
    {
        // ¹ÂÅØœºžŠ Å×œºÆ®ÇÏŽÂ ÅÂœºÅ©žŠ 3°³ »ýŒº
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kPrintNumberTask );
    }    
    kPrintf( "Wait Util %d Task End...\n", i );
    kGetCh();
}

/**
 *  ÅÂœºÅ© 2žŠ ÀÚœÅÀÇ œº·¹µå·Î »ýŒºÇÏŽÂ ÅÂœºÅ©
 */
static void kCreateThreadTask( void )
{
    int i;
    
    for( i = 0 ; i < 3 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 );
    }
    
    while( 1 )
    {
        kSleep( 1 );
    }
}

/**
 *  œº·¹µåžŠ Å×œºÆ®ÇÏŽÂ ÅÂœºÅ© »ýŒº
 */
static void kTestThread( const char* pcParameterBuffer )
{
    TCB* pstProcess;
    
    pstProcess = kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, ( void * )0xEEEEEEEE, 0x1000, 
                              ( QWORD ) kCreateThreadTask );
    if( pstProcess != NULL )
    {
        kPrintf( "Process [0x%Q] Create Success\n", pstProcess->stLink.qwID ); 
    }
    else
    {
        kPrintf( "Process Create Fail\n" );
    }
}

// ³­ŒöžŠ ¹ß»ýœÃÅ°±â À§ÇÑ º¯Œö
static volatile QWORD gs_qwRandomValue = 0;

/**
 *  ÀÓÀÇÀÇ ³­ŒöžŠ ¹ÝÈ¯
 */
QWORD kRandom( void )
{
    gs_qwRandomValue = ( gs_qwRandomValue * 412153 + 5571031 ) >> 16;
    return gs_qwRandomValue;
}

/**
 *  Ã¶ÀÚžŠ Èê·¯³»ž®°Ô ÇÏŽÂ œº·¹µå
 */
static void kDropCharactorThread( void )
{
    int iX, iY;
    int i;
    char vcText[ 2 ] = { 0, };

    iX = kRandom() % CONSOLE_WIDTH;
    
    while( 1 )
    {
        // ÀáœÃ Žë±âÇÔ
        kSleep( kRandom() % 20 );
        
        if( ( kRandom() % 20 ) < 16 )
        {
            vcText[ 0 ] = ' ';
            for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
            {
                kPrintStringXY( iX, i , vcText );
                kSleep( 50 );
            }
        }        
        else
        {
            for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
            {
                vcText[ 0 ] = i + kRandom();
                kPrintStringXY( iX, i, vcText );
                kSleep( 50 );
            }
        }
    }
}

/**
 *  œº·¹µåžŠ »ýŒºÇÏ¿© žÅÆ®ž¯œº È­žéÃ³·³ ºž¿©ÁÖŽÂ ÇÁ·ÎŒŒœº
 */
static void kMatrixProcess( void )
{
    int i;
    
    for( i = 0 ; i < 300 ; i++ )
    {
        if( kCreateTask( TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, 
                         ( QWORD ) kDropCharactorThread ) == NULL )
        {
            break;
        }
        
        kSleep( kRandom() % 5 + 5 );
    }
    
    kPrintf( "%d Thread is created\n", i );

    // Å°°¡ ÀÔ·ÂµÇžé ÇÁ·ÎŒŒœº ÁŸ·á
    kGetCh();
}

/**
 *  žÅÆ®ž¯œº È­žéÀ» ºž¿©ÁÜ
 */
static void kShowMatrix( const char* pcParameterBuffer )
{
    TCB* pstProcess;
    
    pstProcess = kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, ( void* ) 0xE00000, 0xE00000, 
                              ( QWORD ) kMatrixProcess );
    if( pstProcess != NULL )
    {
        kPrintf( "Matrix Process [0x%Q] Create Success\n" );

        // ÅÂœºÅ©°¡ ÁŸ·á µÉ ¶§±îÁö Žë±â
        while( ( pstProcess->stLink.qwID >> 32 ) != 0 )
        {
            kSleep( 100 );
        }
    }
    else
    {
        kPrintf( "Matrix Process Create Fail\n" );
    }
}

/**
 *  FPUžŠ Å×œºÆ®ÇÏŽÂ ÅÂœºÅ©
 */
static void kFPUTestTask( void )
{
    double dValue1;
    double dValue2;
    TCB* pstRunningTask;
    QWORD qwCount = 0;
    QWORD qwRandomValue;
    int i;
    int iOffset;
    char vcData[ 4 ] = { '-', '\\', '|', '/' };
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;

    pstRunningTask = kGetRunningTask();

    // ÀÚœÅÀÇ IDžŠ ŸòŸîŒ­ È­žé ¿ÀÇÁŒÂÀž·Î »ç¿ë
    iOffset = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - 
        ( iOffset % ( CONSOLE_WIDTH * CONSOLE_HEIGHT ) );

    // ·çÇÁžŠ ¹«ÇÑÈ÷ ¹Ýº¹ÇÏžéŒ­ µ¿ÀÏÇÑ °è»êÀ» ŒöÇà
    while( 1 )
    {
        dValue1 = 1;
        dValue2 = 1;
        
        // Å×œºÆ®žŠ À§ÇØ µ¿ÀÏÇÑ °è»êÀ» 2¹ø ¹Ýº¹ÇØŒ­ œÇÇà
        for( i = 0 ; i < 10 ; i++ )
        {
            qwRandomValue = kRandom();
            dValue1 *= ( double ) qwRandomValue;
            dValue2 *= ( double ) qwRandomValue;

            kSleep( 1 );
            
            qwRandomValue = kRandom();
            dValue1 /= ( double ) qwRandomValue;
            dValue2 /= ( double ) qwRandomValue;
        }
        
        if( dValue1 != dValue2 )
        {
            kPrintf( "Value Is Not Same~!!! [%f] != [%f]\n", dValue1, dValue2 );
            break;
        }
        qwCount++;

        // ÈžÀüÇÏŽÂ ¹Ù¶÷°³ºñžŠ Ç¥œÃ
        pstScreen[ iOffset ].bCharactor = vcData[ qwCount % 4 ];

        // »ö±ò ÁöÁ€
        pstScreen[ iOffset ].bAttribute = ( iOffset % 15 ) + 1;
    }
}

/**
 *  ¿øÁÖÀ²(PIE)žŠ °è»ê
 */
static void kTestPIE( const char* pcParameterBuffer )
{
    double dResult;
    int i;
    
    kPrintf( "PIE Cacluation Test\n" );
    kPrintf( "Result: 355 / 113 = " );
    dResult = ( double ) 355 / 113;
    kPrintf( "%d.%d%d\n", ( QWORD ) dResult, ( ( QWORD ) ( dResult * 10 ) % 10 ),
             ( ( QWORD ) ( dResult * 100 ) % 10 ) );
    
    // œÇŒöžŠ °è»êÇÏŽÂ ÅÂœºÅ©žŠ »ýŒº
    for( i = 0 ; i < 100 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kFPUTestTask );
    }
}

/**
 *  µ¿Àû žÞžðž® Á€ºžžŠ Ç¥œÃ
 */
static void kShowDyanmicMemoryInformation( const char* pcParameterBuffer )
{
    QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;
    
    kGetDynamicMemoryInformation( &qwStartAddress, &qwTotalSize, &qwMetaSize, 
            &qwUsedSize );

    kPrintf( "============ Dynamic Memory Information ============\n" );
    kPrintf( "Start Address: [0x%Q]\n", qwStartAddress );
    kPrintf( "Total Size:    [0x%Q]byte, [%d]MB\n", qwTotalSize, 
            qwTotalSize / 1024 / 1024 );
    kPrintf( "Meta Size:     [0x%Q]byte, [%d]KB\n", qwMetaSize, 
            qwMetaSize / 1024 );
    kPrintf( "Used Size:     [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024 );
}

/**
 *  žðµç ºí·Ï ž®œºÆ®ÀÇ ºí·ÏÀ» ŒøÂ÷ÀûÀž·Î ÇÒŽçÇÏ°í ÇØÁŠÇÏŽÂ Å×œºÆ®
 */
static void kTestSequentialAllocation( const char* pcParameterBuffer )
{
    DYNAMICMEMORY* pstMemory;
    long i, j, k;
    QWORD* pqwBuffer;
    
    kPrintf( "============ Dynamic Memory Test ============\n" );
    pstMemory = kGetDynamicMemoryManager();
    
    for( i = 0 ; i < pstMemory->iMaxLevelCount ; i++ )
    {
        kPrintf( "Block List [%d] Test Start\n", i );
        kPrintf( "Allocation And Compare: ");
        
        // žðµç ºí·ÏÀ» ÇÒŽç ¹ÞŸÆŒ­ °ªÀ» Ã€¿î ÈÄ °Ë»ç
        for( j = 0 ; j < ( pstMemory->iBlockCountOfSmallestBlock >> i ) ; j++ )
        {
            pqwBuffer = kAllocateMemory( DYNAMICMEMORY_MIN_SIZE << i );
            if( pqwBuffer == NULL )
            {
                kPrintf( "\nAllocation Fail\n" );
                return ;
            }

            // °ªÀ» Ã€¿î ÈÄ ŽÙœÃ °Ë»ç
            for( k = 0 ; k < ( DYNAMICMEMORY_MIN_SIZE << i ) / 8 ; k++ )
            {
                pqwBuffer[ k ] = k;
            }
            
            for( k = 0 ; k < ( DYNAMICMEMORY_MIN_SIZE << i ) / 8 ; k++ )
            {
                if( pqwBuffer[ k ] != k )
                {
                    kPrintf( "\nCompare Fail\n" );
                    return ;
                }
            }
            // ÁøÇà °úÁ€À» . Àž·Î Ç¥œÃ
            kPrintf( "." );
        }
        
        kPrintf( "\nFree: ");
        // ÇÒŽç ¹ÞÀº ºí·ÏÀ» žðµÎ ¹ÝÈ¯
        for( j = 0 ; j < ( pstMemory->iBlockCountOfSmallestBlock >> i ) ; j++ )
        {
            if( kFreeMemory( ( void * ) ( pstMemory->qwStartAddress + 
                         ( DYNAMICMEMORY_MIN_SIZE << i ) * j ) ) == FALSE )
            {
                kPrintf( "\nFree Fail\n" );
                return ;
            }
            // ÁøÇà °úÁ€À» . Àž·Î Ç¥œÃ
            kPrintf( "." );
        }
        kPrintf( "\n" );
    }
    kPrintf( "Test Complete~!!!\n" );
}

/**
 *  ÀÓÀÇ·Î žÞžðž®žŠ ÇÒŽçÇÏ°í ÇØÁŠÇÏŽÂ °ÍÀ» ¹Ýº¹ÇÏŽÂ ÅÂœºÅ©
 */
static void kRandomAllocationTask( void )
{
    TCB* pstTask;
    QWORD qwMemorySize;
    char vcBuffer[ 200 ];
    BYTE* pbAllocationBuffer;
    int i, j;
    int iY;
    
    pstTask = kGetRunningTask();
    iY = ( pstTask->stLink.qwID ) % 15 + 9;

    for( j = 0 ; j < 10 ; j++ )
    {
        // 1KB ~ 32M±îÁö ÇÒŽçÇÏµµ·Ï ÇÔ
        do
        {
            qwMemorySize = ( ( kRandom() % ( 32 * 1024 ) ) + 1 ) * 1024;
            pbAllocationBuffer = kAllocateMemory( qwMemorySize );

            // žžÀÏ ¹öÆÛžŠ ÇÒŽç ¹ÞÁö žøÇÏžé ŽÙž¥ ÅÂœºÅ©°¡ žÞžðž®žŠ »ç¿ëÇÏ°í 
            // ÀÖÀ» Œö ÀÖÀž¹Ç·Î ÀáœÃ Žë±âÇÑ ÈÄ ŽÙœÃ œÃµµ
            if( pbAllocationBuffer == 0 )
            {
                kSleep( 1 );
            }
        } while( pbAllocationBuffer == 0 );
            
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success", 
                  pbAllocationBuffer, qwMemorySize );
        // ÀÚœÅÀÇ IDžŠ Y ÁÂÇ¥·Î ÇÏ¿© µ¥ÀÌÅÍžŠ Ãâ·Â
        kPrintStringXY( 20, iY, vcBuffer );
        kSleep( 200 );
        
        // ¹öÆÛžŠ ¹ÝÀž·Î ³ªŽ²Œ­ ·£ŽýÇÑ µ¥ÀÌÅÍžŠ ¶È°°ÀÌ Ã€¿ò 
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ", 
                  pbAllocationBuffer, qwMemorySize );
        kPrintStringXY( 20, iY, vcBuffer );
        for( i = 0 ; i < qwMemorySize / 2 ; i++ )
        {
            pbAllocationBuffer[ i ] = kRandom() & 0xFF;
            pbAllocationBuffer[ i + ( qwMemorySize / 2 ) ] = pbAllocationBuffer[ i ];
        }
        kSleep( 200 );
        
        // Ã€¿î µ¥ÀÌÅÍ°¡ Á€»óÀûÀÎÁö ŽÙœÃ È®ÀÎ
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...   ", 
                  pbAllocationBuffer, qwMemorySize );
        kPrintStringXY( 20, iY, vcBuffer );
        for( i = 0 ; i < qwMemorySize / 2 ; i++ )
        {
            if( pbAllocationBuffer[ i ] != pbAllocationBuffer[ i + ( qwMemorySize / 2 ) ] )
            {
                kPrintf( "Task ID[0x%Q] Verify Fail\n", pstTask->stLink.qwID );
                kExitTask();
            }
        }
        kFreeMemory( pbAllocationBuffer );
        kSleep( 200 );
    }
    
    kExitTask();
}

/**
 *  ÅÂœºÅ©žŠ ¿©·¯ °³ »ýŒºÇÏ¿© ÀÓÀÇÀÇ žÞžðž®žŠ ÇÒŽçÇÏ°í ÇØÁŠÇÏŽÂ °ÍÀ» ¹Ýº¹ÇÏŽÂ Å×œºÆ®
 */
static void kTestRandomAllocation( const char* pcParameterBuffer )
{
    int i;
    
    for( i = 0 ; i < 1000 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kRandomAllocationTask );
    }
}

/**
 *  ÇÏµå µðœºÅ©ÀÇ Á€ºžžŠ Ç¥œÃ
 */
static void kShowHDDInformation( const char* pcParameterBuffer )
{
    HDDINFORMATION stHDD;
    char vcBuffer[ 100 ];
    
    // ÇÏµå µðœºÅ©ÀÇ Á€ºžžŠ ÀÐÀœ
    if( kReadHDDInformation( TRUE, TRUE, &stHDD ) == FALSE )
    {
        kPrintf( "HDD Information Read Fail\n" );
        return ;
    }        
    
    kPrintf( "============ Primary Master HDD Information ============\n" );
    
    // žðµš ¹øÈ£ Ãâ·Â
    kMemCpy( vcBuffer, stHDD.vwModelNumber, sizeof( stHDD.vwModelNumber ) );
    vcBuffer[ sizeof( stHDD.vwModelNumber ) - 1 ] = '\0';
    kPrintf( "Model Number:\t %s\n", vcBuffer );
    
    // œÃž®Ÿó ¹øÈ£ Ãâ·Â
    kMemCpy( vcBuffer, stHDD.vwSerialNumber, sizeof( stHDD.vwSerialNumber ) );
    vcBuffer[ sizeof( stHDD.vwSerialNumber ) - 1 ] = '\0';
    kPrintf( "Serial Number:\t %s\n", vcBuffer );

    // Çìµå, œÇž°Žõ, œÇž°Žõ Žç ŒœÅÍ ŒöžŠ Ãâ·Â
    kPrintf( "Head Count:\t %d\n", stHDD.wNumberOfHead );
    kPrintf( "Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder );
    kPrintf( "Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder );
    
    // ÃÑ ŒœÅÍ Œö Ãâ·Â
    kPrintf( "Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors, 
            stHDD.dwTotalSectors / 2 / 1024 );
}

/**
 *  ÇÏµå µðœºÅ©¿¡ ÆÄ¶ó¹ÌÅÍ·Î ³ÑŸî¿Â LBA Ÿîµå·¹œº¿¡Œ­ ŒœÅÍ Œö žžÅ­ ÀÐÀœ
 */
static void kReadSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BYTE bData;
    BOOL bExit = FALSE;
    
    // ÆÄ¶ó¹ÌÅÍ ž®œºÆ®žŠ ÃÊ±âÈ­ÇÏ¿© LBA Ÿîµå·¹œº¿Í ŒœÅÍ Œö ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) readsector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );
    
    // ŒœÅÍ ŒöžžÅ­ žÞžðž®žŠ ÇÒŽç ¹ÞŸÆ ÀÐ±â ŒöÇà
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    if( kReadHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) == iSectorCount )
    {
        kPrintf( "LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount );
        // µ¥ÀÌÅÍ ¹öÆÛÀÇ ³»¿ëÀ» Ãâ·Â
        for( j = 0 ; j < iSectorCount ; j++ )
        {
            for( i = 0 ; i < 512 ; i++ )
            {
                if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
                {
                    kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                    if( kGetCh() == 'q' )
                    {
                        bExit = TRUE;
                        break;
                    }
                }                

                if( ( i % 16 ) == 0 )
                {
                    kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i ); 
                }

                // žðµÎ µÎ ÀÚž®·Î Ç¥œÃÇÏ·Á°í 16ºžŽÙ ÀÛÀº °æ¿ì 0À» Ãß°¡ÇØÁÜ
                bData = pcBuffer[ j * 512 + i ] & 0xFF;
                if( bData < 16 )
                {
                    kPrintf( "0" );
                }
                kPrintf( "%X ", bData );
            }
            
            if( bExit == TRUE )
            {
                break;
            }
        }
        kPrintf( "\n" );
    }
    else
    {
        kPrintf( "Read Fail\n" );
    }
    
    kFreeMemory( pcBuffer );
}

/**
 *  ÇÏµå µðœºÅ©¿¡ ÆÄ¶ó¹ÌÅÍ·Î ³ÑŸî¿Â LBA Ÿîµå·¹œº¿¡Œ­ ŒœÅÍ Œö žžÅ­ Ÿž
 */
static void kWriteSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BOOL bExit = FALSE;
    BYTE bData;
    static DWORD s_dwWriteCount = 0;

    // ÆÄ¶ó¹ÌÅÍ ž®œºÆ®žŠ ÃÊ±âÈ­ÇÏ¿© LBA Ÿîµå·¹œº¿Í ŒœÅÍ Œö ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) writesector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );

    s_dwWriteCount++;
    
    // ¹öÆÛžŠ ÇÒŽç ¹ÞŸÆ µ¥ÀÌÅÍžŠ Ã€¿ò. 
    // ÆÐÅÏÀº 4 ¹ÙÀÌÆ®ÀÇ LBA Ÿîµå·¹œº¿Í 4 ¹ÙÀÌÆ®ÀÇ Ÿ²±â°¡ ŒöÇàµÈ ÈœŒö·Î »ýŒº
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i += 8 )
        {
            *( DWORD* ) &( pcBuffer[ j * 512 + i ] ) = dwLBA + j;
            *( DWORD* ) &( pcBuffer[ j * 512 + i + 4 ] ) = s_dwWriteCount;            
        }
    }
    
    // Ÿ²±â ŒöÇà
    if( kWriteHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) != iSectorCount )
    {
        kPrintf( "Write Fail\n" );
        return ;
    }
    kPrintf( "LBA [%d], [%d] Sector Write Success~!!", dwLBA, iSectorCount );

    // µ¥ÀÌÅÍ ¹öÆÛÀÇ ³»¿ëÀ» Ãâ·Â
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i++ )
        {
            if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
            {
                kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    bExit = TRUE;
                    break;
                }
            }                

            if( ( i % 16 ) == 0 )
            {
                kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i ); 
            }

            // žðµÎ µÎ ÀÚž®·Î Ç¥œÃÇÏ·Á°í 16ºžŽÙ ÀÛÀº °æ¿ì 0À» Ãß°¡ÇØÁÜ
            bData = pcBuffer[ j * 512 + i ] & 0xFF;
            if( bData < 16 )
            {
                kPrintf( "0" );
            }
            kPrintf( "%X ", bData );
        }
        
        if( bExit == TRUE )
        {
            break;
        }
    }
    kPrintf( "\n" );    
    kFreeMemory( pcBuffer );    
}

/**
 *  ÇÏµå µðœºÅ©žŠ ¿¬°á
 */
static void kMountHDD( const char* pcParameterBuffer )
{
    if( kMount() == FALSE )
    {
        kPrintf( "HDD Mount Fail\n" );
        return ;
    }
    kPrintf( "HDD Mount Success\n" );
}

/**
 *  ÇÏµå µðœºÅ©¿¡ ÆÄÀÏ œÃœºÅÛÀ» »ýŒº(Æ÷žË)
 */
static void kFormatHDD( const char* pcParameterBuffer )
{
    if( kFormat() == FALSE )
    {
        kPrintf( "HDD Format Fail\n" );
        return ;
    }
    kPrintf( "HDD Format Success\n" );
}

/**
 *  ÆÄÀÏ œÃœºÅÛ Á€ºžžŠ Ç¥œÃ
 */
static void kShowFileSystemInformation( const char* pcParameterBuffer )
{
    FILESYSTEMMANAGER stManager;
    
    kGetFileSystemInformation( &stManager );
    
    kPrintf( "================== File System Information ==================\n" );
    kPrintf( "Mouted:\t\t\t\t\t %d\n", stManager.bMounted );
    kPrintf( "Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount );
    kPrintf( "Cluster Link Table Start Address:\t %d Sector\n", 
            stManager.dwClusterLinkAreaStartAddress );
    kPrintf( "Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize );
    kPrintf( "Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress );
    kPrintf( "Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount );
}

/**
 *  ·çÆ® µð·ºÅÍž®¿¡ ºó ÆÄÀÏÀ» »ýŒº
 */
static void kCreateFileInRootDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    DWORD dwCluster;
    int i;
    FILE* pstFile;
    
    // ÆÄ¶ó¹ÌÅÍ ž®œºÆ®žŠ ÃÊ±âÈ­ÇÏ¿© ÆÄÀÏ ÀÌž§À» ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    pstFile = fopen( vcFileName, "w" );
    if( pstFile == NULL )
    {
        kPrintf( "File Create Fail\n" );
        return;
    }
    fclose( pstFile );
    kPrintf( "File Create Success\n" );
}

/**
 *  µð·ºÅäž® »ýŒº
 */
static void kMakeDirectory( const char* pcParamegerBuffer ){
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    DWORD dwCluster;
    int i;
    FILE* pstFile;
    
    // ÆÄ¶ó¹ÌÅÍ ž®œºÆ®žŠ ÃÊ±âÈ­ÇÏ¿© ÆÄÀÏ ÀÌž§À» ÃßÃâ
    kInitializeParameter( &stList, pcParamegerBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    pstFile = opendir( vcFileName);
    if( pstFile == NULL )
    {
        kPrintf( "Directory Create Fail\n" );
        return;
    }
    fclose( pstFile );
    kPrintf( "Directory Create Success\n" );
	
}

/**
 * µð·ºÅäž® ÀÌµ¿
 */
static void kMoveDirectory( const char* pcParamegerBuffer){
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    DWORD dwCluster;
    int i;
    FILE* pstFile;
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    kReadRTCTime( &bHour, &bMinute, &bSecond );
    kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );

    kInitializeParameter( &stList, pcParamegerBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    DIR* pstDirectory;
    
    FILESYSTEMMANAGER stManager;
    DIRECTORYENTRY* directoryInfo;
    char temp_path[100] = "\0";
    DWORD temp_index = 0;

      // 파일 시스템 정보를 얻음
    kGetFileSystemInformation( &stManager );
   
   
    directoryInfo = kFindDirectory(currentDirectoryClusterIndex);


    if(kMemCmp(vcFileName,".",2)==0){
        currentDirectoryClusterIndex = directoryInfo[0].ParentDirectoryCluserIndex;
        kSetClusterIndex(currentDirectoryClusterIndex);
        kMemCpy(path,directoryInfo[0].ParentDirectoryPath,kStrLen(directoryInfo[0].ParentDirectoryPath)+1);
	
     }
    else if(kMemCmp(vcFileName,"..",3)==0){
       
        currentDirectoryClusterIndex = directoryInfo[1].ParentDirectoryCluserIndex;
        kSetClusterIndex(currentDirectoryClusterIndex);
        
        kMemCpy(path,directoryInfo[1].ParentDirectoryPath,kStrLen(directoryInfo[1].ParentDirectoryPath)+1);
     }
     
     else{
    for( int j = 0 ; j < FILESYSTEM_MAXDIRECTORYENTRYCOUNT ; j++ )


    {

	/**/
	if( directoryInfo[ j ].dwStartClusterIndex == 0 &&
        	 kMemCmp(directoryInfo[ j ].vcFileName,vcFileName,kStrLen(vcFileName))!=0 && 
      	   directoryInfo[j].flag !=1){
	kPrintf("No such directory\n");
	break;
	}

	/**/

        if( directoryInfo[ j ].dwStartClusterIndex != 0 &&
         kMemCmp(directoryInfo[ j ].vcFileName,vcFileName,kStrLen(vcFileName))==0 && 
         directoryInfo[j].flag ==1)
        {
            kMemCpy(temp_path,path,kStrLen(path)+1);   
            temp_index = currentDirectoryClusterIndex;
            
            if(kMemCmp(path,"/",2)==0)
                kMemCpy(path + kStrLen(path),vcFileName,kStrLen(vcFileName)+1);
            else{
                kMemCpy(path + kStrLen(path),"/",1);
                kMemCpy(path + kStrLen(path),vcFileName,kStrLen(vcFileName)+1);
            }
//kPrintf("\nparent %d %d %d %d %d %d %d %d \n", directoryInfo[j-1].bSecond, directoryInfo[j-1].bMinute, directoryInfo[j-1].bHour, directoryInfo[j-1].bDayOfWeek, directoryInfo[j-1].bDayOfMonth, directoryInfo[j-1].bMonth, directoryInfo[j-1].wYear,  1);


//kPrintf("\n%d %d %d %d %d %d %d %d \n", directoryInfo[j].bSecond, directoryInfo[j].bMinute, directoryInfo[j].bHour, directoryInfo[j].bDayOfWeek, directoryInfo[j].bDayOfMonth, directoryInfo[j].bMonth, directoryInfo[j].wYear,  1);

bSecond=directoryInfo[j].bSecond;
bMinute=directoryInfo[j].bMinute;
bHour=directoryInfo[j].bHour;
bDayOfWeek =directoryInfo[j].bDayOfWeek;
bDayOfMonth=directoryInfo[j].bDayOfMonth;
bMonth=directoryInfo[j].bMonth;
wYear=directoryInfo[j].wYear;

            currentDirectoryClusterIndex = directoryInfo[ j ].dwStartClusterIndex;
            kSetClusterIndex(currentDirectoryClusterIndex);
            directoryInfo = NULL;
            directoryInfo = kFindDirectory(currentDirectoryClusterIndex);


            if( directoryInfo[ 0 ].dwStartClusterIndex != -1 )
            {               
 		kSetDotInDirectory(bSecond, bMinute, bHour, bDayOfWeek, bDayOfMonth, bMonth, wYear,1);

                kUpdateDirectory(0,".",path,currentDirectoryClusterIndex);		           
            }

            directoryInfo[1].ParentDirectoryCluserIndex = temp_index;
	    

            kMemCpy(directoryInfo[1].ParentDirectoryPath,temp_path,kStrLen(temp_path)+1);
            kUpdateDirectory( 1,"..",temp_path, temp_index );
         
            break;      
        }		
        
    }
  }

    
}
/**
 *  ·çÆ® µð·ºÅÍž®¿¡Œ­ ÆÄÀÏÀ» »èÁŠ
 */
static void kDeleteFileInRootDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    
    // ÆÄ¶ó¹ÌÅÍ ž®œºÆ®žŠ ÃÊ±âÈ­ÇÏ¿© ÆÄÀÏ ÀÌž§À» ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    if( remove( vcFileName ) != 0 )
    {
        kPrintf( "File Not Found or File Opened\n" );
        return ;
    }
    
    kPrintf( "File Delete Success\n" );
}

/**
 *  µð·ºÅÍž®žŠ »èÁŠ
 */
static void kRemoveDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    
    // ÆÄ¶ó¹ÌÅÍ ž®œºÆ®žŠ ÃÊ±âÈ­ÇÏ¿© ÆÄÀÏ ÀÌž§À» ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    if( remove( vcFileName ) != 0 )
    {
        kPrintf( "File Not Found or File Opened\n" );
        return ;
    }
    
    kPrintf( "File Delete Success\n" );
}



/**
 *  ·çÆ® µð·ºÅÍž®ÀÇ ÆÄÀÏ žñ·ÏÀ» Ç¥œÃ
 */
static void kShowRootDirectory( const char* pcParameterBuffer )
{
    DIR* pstDirectory;
    int i, iCount, iTotalCount;
    struct dirent* pstEntry;
    char vcBuffer[ 400 ];
    char vcTempValue[ 50 ];
    DWORD dwTotalByte;
    DWORD dwUsedClusterCount;
    FILESYSTEMMANAGER stManager;
    
    // ÆÄÀÏ œÃœºÅÛ Á€ºžžŠ ŸòÀœ
    kGetFileSystemInformation( &stManager );
     
    // ·çÆ® µð·ºÅÍž®žŠ ¿®
    pstDirectory = opendir( "/" );
    if( pstDirectory == NULL )
    {
        kPrintf( "Root Directory Open Fail\n" );
        return ;
    }
    
    // žÕÀú ·çÇÁžŠ µ¹žéŒ­ µð·ºÅÍž®¿¡ ÀÖŽÂ ÆÄÀÏÀÇ °³Œö¿Í ÀüÃŒ ÆÄÀÏÀÌ »ç¿ëÇÑ Å©±âžŠ °è»ê
    iTotalCount = 0;
    dwTotalByte = 0;
    dwUsedClusterCount = 0;
    while( 1 )
    {
        // µð·ºÅÍž®¿¡Œ­ ¿£Æ®ž® ÇÏ³ªžŠ ÀÐÀœ
        pstEntry = readdir( pstDirectory );
        // ŽõÀÌ»ó ÆÄÀÏÀÌ ŸøÀžžé ³ª°š
        if( pstEntry == NULL )
        {
            break;
        }
        iTotalCount++;
        dwTotalByte += pstEntry->dwFileSize;

        // œÇÁŠ·Î »ç¿ëµÈ Å¬·¯œºÅÍÀÇ °³ŒöžŠ °è»ê
        if( pstEntry->dwFileSize == 0 )
        {
            // Å©±â°¡ 0ÀÌ¶óµµ Å¬·¯œºÅÍ 1°³ŽÂ ÇÒŽçµÇŸî ÀÖÀœ
            dwUsedClusterCount++;
        }
        else
        {
            // Å¬·¯œºÅÍ °³ŒöžŠ ¿Ãž²ÇÏ¿© ŽõÇÔ
            dwUsedClusterCount += ( pstEntry->dwFileSize + 
                ( FILESYSTEM_CLUSTERSIZE - 1 ) ) / FILESYSTEM_CLUSTERSIZE;
        }
    }
    
    // œÇÁŠ ÆÄÀÏÀÇ ³»¿ëÀ» Ç¥œÃÇÏŽÂ ·çÇÁ
    rewinddir( pstDirectory );
    iCount = 0;
    while( 1 )
    {
        // µð·ºÅÍž®¿¡Œ­ ¿£Æ®ž® ÇÏ³ªžŠ ÀÐÀœ
        pstEntry = readdir( pstDirectory );
        // ŽõÀÌ»ó ÆÄÀÏÀÌ ŸøÀžžé ³ª°š
        if( pstEntry == NULL )
        {
            break;
        }
        
        // ÀüºÎ °ø¹éÀž·Î ÃÊ±âÈ­ ÇÑ ÈÄ °¢ À§Ä¡¿¡ °ªÀ» ŽëÀÔ
        kMemSet( vcBuffer, ' ', sizeof( vcBuffer ) - 1 );
        vcBuffer[ sizeof( vcBuffer ) - 1 ] = '\0';
        
        if(pstEntry->flag == 0){
        // ÆÄÀÏ ÀÌž§ »ðÀÔ
        kMemCpy( vcBuffer, pstEntry->d_name, 
                 kStrLen( pstEntry->d_name ) );

        // ÆÄÀÏ ±æÀÌ »ðÀÔ
        kSPrintf( vcTempValue, "%d Byte", pstEntry->dwFileSize );
        kMemCpy( vcBuffer + 30, vcTempValue, kStrLen( vcTempValue )  );

        // ÆÄÀÏÀÇ œÃÀÛ Å¬·¯œºÅÍ »ðÀÔ
        kSPrintf( vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex );
        kMemCpy( vcBuffer + 55, vcTempValue, kStrLen( vcTempValue ) + 1 );
 
 

        kPrintf( "    %s\n", vcBuffer );

        }
        
        else if(pstEntry->flag == 1){
            // ÆÄÀÏ ÀÌž§ »ðÀÔ
            kMemCpy( vcBuffer, pstEntry->d_name, kStrLen( pstEntry->d_name ) );

            // ÆÄÀÏ ±æÀÌ »ðÀÔ
            kSPrintf( vcTempValue, "Directory", 10 );
            kMemCpy( vcBuffer + 30, vcTempValue, kStrLen( vcTempValue ) +1);
            //vcBuffer[kStrLen(vcBuffer) + 1] = '\0';
            kPrintf( "    %s\n", vcBuffer );
        
        }
        

        if( ( iCount != 0 ) && ( ( iCount % 20 ) == 0 ) )
        {
            kPrintf( "Press any key to continue... ('q' is exit) : " );
            if( kGetCh() == 'q' )
            {
                kPrintf( "\n" );
                break;
            }        
        }
        iCount++;
    }
    
    // ÃÑ ÆÄÀÏÀÇ °³Œö¿Í ÆÄÀÏÀÇ ÃÑ Å©±âžŠ Ãâ·Â
    kPrintf( "\t\tTotal File Count: %d\n", iTotalCount );
    kPrintf( "\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte, 
             dwUsedClusterCount );
    
    // ³²Àº Å¬·¯œºÅÍ ŒöžŠ ÀÌ¿ëÇØŒ­ ¿©À¯ °ø°£À» Ãâ·Â
    kPrintf( "\t\tFree Space: %d KByte (%d Cluster)\n", 
             ( stManager.dwTotalClusterCount - dwUsedClusterCount ) * 
             FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - 
             dwUsedClusterCount );
    
    // µð·ºÅÍž®žŠ ŽÝÀœ
    closedir( pstDirectory );
}


/**
 *  µð·ºÅÍž®ÀÇ ÆÄÀÏ žñ·ÏÀ» Ç¥œÃ
 */
static void kShowDirectory( const char* pcParameterBuffer )
{
    DIR* pstDirectory;
    int i, iCount, iTotalCount;
    struct dirent* pstEntry;
    char vcBuffer[ 400 ];
    char vcTempValue[ 50 ];
    DWORD dwTotalByte;
    DWORD dwUsedClusterCount;
    FILESYSTEMMANAGER stManager;
    DIRECTORYENTRY* directoryInfo;
    
    // ÆÄÀÏ œÃœºÅÛ Á€ºžžŠ ŸòÀœ
    kGetFileSystemInformation( &stManager );
     
   
    
    iCount = 0;

    directoryInfo = kFindDirectory(currentDirectoryClusterIndex);
     

    for( i = 0 ; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT ; i++ )
    {
        if( directoryInfo[ i ].dwStartClusterIndex != 0 )
        {
            
        pstEntry = &directoryInfo[i];
        



        kMemSet( vcBuffer, ' ', sizeof( vcBuffer ) - 1 );
        vcBuffer[ sizeof( vcBuffer ) - 1 ] = '\0';
        
        if(pstEntry->flag == 0/* && pstEntry->f ==0*/){
        
        kMemCpy( vcBuffer, pstEntry->d_name, 
                 kStrLen( pstEntry->d_name ) );

        
        kSPrintf( vcTempValue, "%d Byte", pstEntry->dwFileSize );
        kMemCpy( vcBuffer + 40, vcTempValue, kStrLen( vcTempValue )  );

        
        kSPrintf( vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex );
        kMemCpy( vcBuffer + 40, vcTempValue, kStrLen( vcTempValue ) + 1 );

        kPrintf( "%s ", vcBuffer );


        kPrintf("%d/%d/%d ", pstEntry->wYear, pstEntry->bMonth, pstEntry->bDayOfMonth);
	kPrintf("%d:%d:%d\n",pstEntry-> bHour,pstEntry-> bMinute, pstEntry->bSecond );
       }


        else if(pstEntry->flag == 1 && pstEntry->f == 1){  // .
            
          kMemCpy( vcBuffer, pstEntry->d_name, kStrLen( pstEntry->d_name ) );

           
           kSPrintf( vcTempValue, "Directory", 10 );
           kMemCpy( vcBuffer + 40, vcTempValue, kStrLen( vcTempValue ) +1);
            
            kPrintf( "%s  ", vcBuffer );
             

	kPrintf("%d/%d/%d ", pstEntry->wYear, pstEntry->bMonth, pstEntry->bDayOfMonth);
	kPrintf("%d:%d:%d\n",pstEntry-> bHour, pstEntry-> bMinute,  pstEntry->bSecond );
}


	  else if(pstEntry->flag == 1 && pstEntry->f == 0){  //..
           
          kMemCpy( vcBuffer, pstEntry->d_name, kStrLen( pstEntry->d_name ) );

           
           kSPrintf( vcTempValue, "Directory", 10 );
           kMemCpy( vcBuffer + 40, vcTempValue, kStrLen( vcTempValue ) +1);
            
            kPrintf( "%s  ", vcBuffer );
             

	

	kPrintf( "\n");
        
        }




       else if(pstEntry->flag == 1 &&pstEntry->f != 0) {
            
           kMemCpy( vcBuffer, pstEntry->d_name, kStrLen( pstEntry->d_name ) );       

           kSPrintf( vcTempValue, "Directory", 15 );

           kMemCpy( vcBuffer + 40, vcTempValue, kStrLen( vcTempValue ) +1);

            

           kPrintf( "%s  ", vcBuffer );

             

        kPrintf("%d/%d/%d ", pstEntry->wYear, pstEntry->bMonth, pstEntry->bDayOfMonth);
	kPrintf("%d:%d:%d\n",pstEntry-> bHour,pstEntry-> bMinute, pstEntry->bSecond );
	//kPrintf( "\n");

        
        }


      
        }
        
    }

   
}


/**
 *  ÆÄÀÏÀ» »ýŒºÇÏ¿© Å°ºžµå·Î ÀÔ·ÂµÈ µ¥ÀÌÅÍžŠ Ÿž
 */
static void kWriteDataToFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    BYTE bKey;
    
    // ÆÄ¶ó¹ÌÅÍ ž®œºÆ®žŠ ÃÊ±âÈ­ÇÏ¿© ÆÄÀÏ ÀÌž§À» ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    
    // ÆÄÀÏ »ýŒº
    fp = fopen( vcFileName, "w" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // ¿£ÅÍ Å°°¡ ¿¬ŒÓÀž·Î 3¹ø Ž­·¯Áú ¶§±îÁö ³»¿ëÀ» ÆÄÀÏ¿¡ Ÿž
    iEnterCount = 0;
    while( 1 )
    {
        bKey = kGetCh();
        // ¿£ÅÍ Å°ÀÌžé ¿¬ŒÓ 3¹ø Ž­·¯Á³ŽÂ°¡ È®ÀÎÇÏ¿© ·çÇÁžŠ ºüÁ® ³ª°š
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            if( iEnterCount >= 3 )
            {
                break;
            }
        }
        // ¿£ÅÍ Å°°¡ ŸÆŽÏ¶óžé ¿£ÅÍ Å° ÀÔ·Â ÈœŒöžŠ ÃÊ±âÈ­
        else
        {
            iEnterCount = 0;
        }
        
        kPrintf( "%c", bKey );
        if( fwrite( &bKey, 1, 1, fp ) != 1 )
        {
            kPrintf( "File Wirte Fail\n" );
            break;
        }
    }
    
    kPrintf( "File Create Success\n" );
    fclose( fp );
}

/**
 *  ÆÄÀÏÀ» ¿­ŸîŒ­ µ¥ÀÌÅÍžŠ ÀÐÀœ
 */
static void kReadDataFromFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    BYTE bKey;
    
    // ÆÄ¶ó¹ÌÅÍ ž®œºÆ®žŠ ÃÊ±âÈ­ÇÏ¿© ÆÄÀÏ ÀÌž§À» ÃßÃâ
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    
    // ÆÄÀÏ »ýŒº
    fp = fopen( vcFileName, "r" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // ÆÄÀÏÀÇ ³¡±îÁö Ãâ·ÂÇÏŽÂ °ÍÀ» ¹Ýº¹
    iEnterCount = 0;
    while( 1 )
    {
        if( fread( &bKey, 1, 1, fp ) != 1 )
        {
            break;
        }
        kPrintf( "%c", bKey );
        
        // žžŸà ¿£ÅÍ Å°ÀÌžé ¿£ÅÍ Å° ÈœŒöžŠ Áõ°¡œÃÅ°°í 20¶óÀÎ±îÁö Ãâ·ÂÇßŽÙžé 
        // Žõ Ãâ·ÂÇÒÁö ¿©ºÎžŠ ¹°Ÿîºœ
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            
            if( ( iEnterCount != 0 ) && ( ( iEnterCount % 20 ) == 0 ) )
            {
                kPrintf( "Press any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    kPrintf( "\n" );
                    break;
                }
                kPrintf( "\n" );
                iEnterCount = 0;
            }
        }
    }
    fclose( fp );
}

/**
 *  ÆÄÀÏ I/O¿¡ °ü·ÃµÈ ±âŽÉÀ» Å×œºÆ®
 */
static void kTestFileIO( const char* pcParameterBuffer )
{
    FILE* pstFile;
    BYTE* pbBuffer;
    int i;
    int j;
    DWORD dwRandomOffset;
    DWORD dwByteCount;
    BYTE vbTempBuffer[ 1024 ];
    DWORD dwMaxFileSize;
    
    kPrintf( "================== File I/O Function Test ==================\n" );
    
    // 4MbyteÀÇ ¹öÆÛ ÇÒŽç
    dwMaxFileSize = 4 * 1024 * 1024;
    pbBuffer = kAllocateMemory( dwMaxFileSize );
    if( pbBuffer == NULL )
    {
        kPrintf( "Memory Allocation Fail\n" );
        return ;
    }
    // Å×œºÆ®¿ë ÆÄÀÏÀ» »èÁŠ
    remove( "testfileio.bin" );

    //==========================================================================
    // ÆÄÀÏ ¿­±â Å×œºÆ®
    //==========================================================================
    kPrintf( "1. File Open Fail Test..." );
    // r ¿ÉŒÇÀº ÆÄÀÏÀ» »ýŒºÇÏÁö ŸÊÀž¹Ç·Î, Å×œºÆ® ÆÄÀÏÀÌ ŸøŽÂ °æ¿ì NULLÀÌ µÇŸîŸß ÇÔ
    pstFile = fopen( "testfileio.bin", "r" );
    if( pstFile == NULL )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
        fclose( pstFile );
    }
    
    //==========================================================================
    // ÆÄÀÏ »ýŒº Å×œºÆ®
    //==========================================================================
    kPrintf( "2. File Create Test..." );
    // w ¿ÉŒÇÀº ÆÄÀÏÀ» »ýŒºÇÏ¹Ç·Î, Á€»óÀûÀž·Î ÇÚµéÀÌ ¹ÝÈ¯µÇŸîŸßÇÔ
    pstFile = fopen( "testfileio.bin", "w" );
    if( pstFile != NULL )
    {
        kPrintf( "[Pass]\n" );
        kPrintf( "    File Handle [0x%Q]\n", pstFile );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    //==========================================================================
    // ŒøÂ÷ÀûÀÎ ¿µ¿ª Ÿ²±â Å×œºÆ®
    //==========================================================================
    kPrintf( "3. Sequential Write Test(Cluster Size)..." );
    // ¿­ž° ÇÚµéÀ» °¡Áö°í Ÿ²±â ŒöÇà
    for( i = 0 ; i < 100 ; i++ )
    {
        kMemSet( pbBuffer, i, FILESYSTEM_CLUSTERSIZE );
        if( fwrite( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) !=
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    %d Cluster Error\n", i );
            break;
        }
    }
    if( i >= 100 )
    {
        kPrintf( "[Pass]\n" );
    }
    
    //==========================================================================
    // ŒøÂ÷ÀûÀÎ ¿µ¿ª ÀÐ±â Å×œºÆ®
    //==========================================================================
    kPrintf( "4. Sequential Read And Verify Test(Cluster Size)..." );
    // ÆÄÀÏÀÇ Ã³ÀœÀž·Î ÀÌµ¿
    fseek( pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END );
    
    // ¿­ž° ÇÚµéÀ» °¡Áö°í ÀÐ±â ŒöÇà ÈÄ, µ¥ÀÌÅÍ °ËÁõ
    for( i = 0 ; i < 100 ; i++ )
    {
        // ÆÄÀÏÀ» ÀÐÀœ
        if( fread( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) !=
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        
        // µ¥ÀÌÅÍ °Ë»ç
        for( j = 0 ; j < FILESYSTEM_CLUSTERSIZE ; j++ )
        {
            if( pbBuffer[ j ] != ( BYTE ) i )
            {
                kPrintf( "[Fail]\n" );
                kPrintf( "    %d Cluster Error. [%X] != [%X]\n", i, pbBuffer[ j ], 
                         ( BYTE ) i );
                break;
            }
        }
    }
    if( i >= 100 )
    {
        kPrintf( "[Pass]\n" );
    }

    //==========================================================================
    // ÀÓÀÇÀÇ ¿µ¿ª Ÿ²±â Å×œºÆ®
    //==========================================================================
    kPrintf( "5. Random Write Test...\n" );
    
    // ¹öÆÛžŠ žðµÎ 0Àž·Î Ã€¿ò
    kMemSet( pbBuffer, 0, dwMaxFileSize );
    // ¿©±â Àú±â¿¡ ¿Å°ÜŽÙŽÏžéŒ­ µ¥ÀÌÅÍžŠ Ÿ²°í °ËÁõ
    // ÆÄÀÏÀÇ ³»¿ëÀ» ÀÐŸîŒ­ ¹öÆÛ·Î º¹»ç
    fseek( pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR );
    fread( pbBuffer, 1, dwMaxFileSize, pstFile );
    
    // ÀÓÀÇÀÇ À§Ä¡·Î ¿Å±âžéŒ­ µ¥ÀÌÅÍžŠ ÆÄÀÏ°ú ¹öÆÛ¿¡ µ¿œÃ¿¡ Ÿž
    for( i = 0 ; i < 100 ; i++ )
    {
        dwByteCount = ( kRandom() % ( sizeof( vbTempBuffer ) - 1 ) ) + 1;
        dwRandomOffset = kRandom() % ( dwMaxFileSize - dwByteCount );
        
        kPrintf( "    [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, 
                dwByteCount );

        // ÆÄÀÏ Æ÷ÀÎÅÍžŠ ÀÌµ¿
        fseek( pstFile, dwRandomOffset, SEEK_SET );
        kMemSet( vbTempBuffer, i, dwByteCount );
              
        // µ¥ÀÌÅÍžŠ Ÿž
        if( fwrite( vbTempBuffer, 1, dwByteCount, pstFile ) != dwByteCount )
        {
            kPrintf( "[Fail]\n" );
            break;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
        
        kMemSet( pbBuffer + dwRandomOffset, i, dwByteCount );
    }
    
    // žÇ ž¶Áöž·Àž·Î ÀÌµ¿ÇÏ¿© 1¹ÙÀÌÆ®žŠ œáŒ­ ÆÄÀÏÀÇ Å©±âžŠ 4Mbyte·Î žžµê
    fseek( pstFile, dwMaxFileSize - 1, SEEK_SET );
    fwrite( &i, 1, 1, pstFile );
    pbBuffer[ dwMaxFileSize - 1 ] = ( BYTE ) i;

    //==========================================================================
    // ÀÓÀÇÀÇ ¿µ¿ª ÀÐ±â Å×œºÆ®
    //==========================================================================
    kPrintf( "6. Random Read And Verify Test...\n" );
    // ÀÓÀÇÀÇ À§Ä¡·Î ¿Å±âžéŒ­ ÆÄÀÏ¿¡Œ­ µ¥ÀÌÅÍžŠ ÀÐŸî ¹öÆÛÀÇ ³»¿ë°ú ºñ±³
    for( i = 0 ; i < 100 ; i++ )
    {
        dwByteCount = ( kRandom() % ( sizeof( vbTempBuffer ) - 1 ) ) + 1;
        dwRandomOffset = kRandom() % ( ( dwMaxFileSize ) - dwByteCount );

        kPrintf( "    [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, 
                dwByteCount );
        
        // ÆÄÀÏ Æ÷ÀÎÅÍžŠ ÀÌµ¿
        fseek( pstFile, dwRandomOffset, SEEK_SET );
        
        // µ¥ÀÌÅÍ ÀÐÀœ
        if( fread( vbTempBuffer, 1, dwByteCount, pstFile ) != dwByteCount )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    Read Fail\n", dwRandomOffset ); 
            break;
        }
        
        // ¹öÆÛ¿Í ºñ±³
        if( kMemCmp( pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount ) 
                != 0 )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    Compare Fail\n", dwRandomOffset ); 
            break;
        }
        
        kPrintf( "[Pass]\n" );
    }
    
    //==========================================================================
    // ŽÙœÃ ŒøÂ÷ÀûÀÎ ¿µ¿ª ÀÐ±â Å×œºÆ®
    //==========================================================================
    kPrintf( "7. Sequential Write, Read And Verify Test(1024 Byte)...\n" );
    // ÆÄÀÏÀÇ Ã³ÀœÀž·Î ÀÌµ¿
    fseek( pstFile, -dwMaxFileSize, SEEK_CUR );
    
    // ¿­ž° ÇÚµéÀ» °¡Áö°í Ÿ²±â ŒöÇà. ŸÕºÎºÐ¿¡Œ­ 2Mbytežž Ÿž
    for( i = 0 ; i < ( 2 * 1024 * 1024 / 1024 ) ; i++ )
    {
        kPrintf( "    [%d] Offset [%d] Byte [%d] Write...", i, i * 1024, 1024 );

        // 1024 ¹ÙÀÌÆ®Ÿ¿ ÆÄÀÏÀ» Ÿž
        if( fwrite( pbBuffer + ( i * 1024 ), 1, 1024, pstFile ) != 1024 )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
    }

    // ÆÄÀÏÀÇ Ã³ÀœÀž·Î ÀÌµ¿
    fseek( pstFile, -dwMaxFileSize, SEEK_SET );
    
    // ¿­ž° ÇÚµéÀ» °¡Áö°í ÀÐ±â ŒöÇà ÈÄ µ¥ÀÌÅÍ °ËÁõ. Random Write·Î µ¥ÀÌÅÍ°¡ Àßžø 
    // ÀúÀåµÉ Œö ÀÖÀž¹Ç·Î °ËÁõÀº 4Mbyte ÀüÃŒžŠ Žë»óÀž·Î ÇÔ
    for( i = 0 ; i < ( dwMaxFileSize / 1024 )  ; i++ )
    {
        // µ¥ÀÌÅÍ °Ë»ç
        kPrintf( "    [%d] Offset [%d] Byte [%d] Read And Verify...", i, 
                i * 1024, 1024 );
        
        // 1024 ¹ÙÀÌÆ®Ÿ¿ ÆÄÀÏÀ» ÀÐÀœ
        if( fread( vbTempBuffer, 1, 1024, pstFile ) != 1024 )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        
        if( kMemCmp( pbBuffer + ( i * 1024 ), vbTempBuffer, 1024 ) != 0 )
        {
            kPrintf( "[Fail]\n" );
            break;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
    }
        
    //==========================================================================
    // ÆÄÀÏ »èÁŠ œÇÆÐ Å×œºÆ®
    //==========================================================================
    kPrintf( "8. File Delete Fail Test..." );
    // ÆÄÀÏÀÌ ¿­·ÁÀÖŽÂ »óÅÂÀÌ¹Ç·Î ÆÄÀÏÀ» Áö¿ì·Á°í ÇÏžé œÇÆÐÇØŸß ÇÔ
    if( remove( "testfileio.bin" ) != 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    //==========================================================================
    // ÆÄÀÏ ŽÝ±â Å×œºÆ®
    //==========================================================================
    kPrintf( "9. File Close Test..." );
    // ÆÄÀÏÀÌ Á€»óÀûÀž·Î ŽÝÇôŸß ÇÔ
    if( fclose( pstFile ) == 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }

    //==========================================================================
    // ÆÄÀÏ »èÁŠ Å×œºÆ®
    //==========================================================================
    kPrintf( "10. File Delete Test..." );
    // ÆÄÀÏÀÌ ŽÝÇûÀž¹Ç·Î Á€»óÀûÀž·Î Áö¿öÁ®Ÿß ÇÔ 
    if( remove( "testfileio.bin" ) == 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    // žÞžðž®žŠ ÇØÁŠ
    kFreeMemory( pbBuffer );    
}
