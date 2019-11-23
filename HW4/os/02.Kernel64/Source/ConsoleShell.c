#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
#include "Task.h"

#include "Synchronization.h"


char command_history[10][100]={""};  
int count =0; 
// 커맨드 테이블 
SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
	{ "help", "Show Help", kHelp },
    { "cls", "Clear Screen", kCls },
    { "totalram", "Show Total RAM Size", kShowTotalRAMSize },
    { "strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest },
    { "shutdown", "Shutdown And Reboot OS", kShutdown },
    { "raisefault", "exception handler", kRaisefault},
    { "shutdummy", "dummy", kDummy},

	{ "settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)", kSetTimer },
    { "wait", "Wait ms Using PIT, ex)wait 100(ms)", kWaitUsingPIT },
    { "rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter },
    { "cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed },
    { "date", "Show Date And Time", kShowDateAndTime },
    { "createtask", "Create Task, ex)createtask 1(type) 10(count)", kCreateTestTask },
    { "changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)",kChangeTaskPriority },
    { "tasklist", "Show Task List", kShowTaskList },
    { "killtask", "End Task, ex)killtask 1(ID) or 0xffffffff(All Task)", kKillTask },
    { "cpuload", "Show Processor Load", kCPULoad },
    { "testmutex", "Test Mutex Function", kTestMutex },
    { "testthread", "Test Thread And Process Function", kTestThread },
    { "showmatrix", "Show Matrix Screen", kShowMatrix },    
    {"showresult","Show result",kShowResult},    

};       

//======================
//  실제 셸을 구성하는 코드 
//======================
/**
 *  셸의 메인 루프
 */
void kStartConsoleShell( void )
{
    char vcCommandBuffer[ CONSOLESHELL_MAXCOMMANDBUFFERCOUNT ];
    int iCommandBufferIndex = 0;
    BYTE bKey;
    int iCursorX, iCursorY;

    int current = 0; 
    int current_history_flag =0 ;
    int i; 
    int up_count = 0;
    int down_count =0;

    int commandCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
    int commandIndex;
    SHELLCOMMANDENTRY command;
    int commandStrLen;
    int commandStrIndex;
    int matchedCount;
    char *matchedCommandStrs[commandCount];
    int iCommandTempBufferIndex;
    int commonStrSize;
    
    
    //프롬프트 출력
    kPrintf( CONSOLESHELL_PROMPTMESSAGE );

    while( 1 )
    {       
        // 키가 수신될 때까지 대기
        bKey = kGetCh();

        // Backspace 키 처리
        if( bKey == KEY_BACKSPACE )
        {
                // 현재 커서 위치를 얻어서 한 문자 앞으로 이동한 다음 공백을 출력하고  
                // 커맨드 버퍼에서 마지막 문자 삭제 

            if( iCommandBufferIndex > 0 )
            {
                
                kGetCursor( &iCursorX, &iCursorY );
                kPrintStringXY( iCursorX - 1, iCursorY, " " );
                kSetCursor( iCursorX - 1, iCursorY );
                iCommandBufferIndex--;
            }
        }
        // 엔터 키 처리
        else if( bKey == KEY_ENTER )
        {
            kPrintf( "\n" );            
            
            if( iCommandBufferIndex > 0 )
            {
                // 커맨드 버퍼에 있는 명령을 실행
                vcCommandBuffer[ iCommandBufferIndex ] = '\0';
                kExecuteCommand( vcCommandBuffer );

                    
                if(count != 10)
                {
                    
                    kMemCpy(command_history[count++],vcCommandBuffer,iCommandBufferIndex);
                    current = count;
                    
                }
                else   // history 명령어 10개
                {
                    
                    count =0;
                    current_history_flag = 1;                   
                    kMemCpy(command_history[count++],vcCommandBuffer,iCommandBufferIndex);  
                                
                }

                // kExecuteCommand(vcCommandBuffer);
        

            }       
            // 프롬프트 출력 및 커맨드 버퍼 초기화
            kPrintf( "%s", CONSOLESHELL_PROMPTMESSAGE );                
            kMemSet( vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT );
            iCommandBufferIndex = 0;
            up_count = 0;
            down_count = 0;
            
        }
        // 시프트 키, CAPS Lock, NUM Lock, Scroll Lock은 무시
        else if( ( bKey == KEY_LSHIFT ) || ( bKey == KEY_RSHIFT ) ||
                ( bKey == KEY_CAPSLOCK ) || ( bKey == KEY_NUMLOCK ) ||
                ( bKey == KEY_SCROLLLOCK ) )
        {
            ;
        }
        else if (bKey == KEY_TAB) // for Autocomplete
        {
            matchedCount = 0;
            for(commandIndex = 0; commandIndex < commandCount; commandIndex++)
            {
                command = gs_vstCommandTable[commandIndex];
                commandStrLen = kStrLen(command.pcCommand);
                for(commandStrIndex = 0; commandStrIndex < commandStrLen && commandStrIndex < iCommandBufferIndex; commandStrIndex++)
                {
                    if(command.pcCommand[commandStrIndex] != vcCommandBuffer[commandStrIndex]) break;
                }

                if(commandStrIndex == iCommandBufferIndex) // Matched
                {
                    matchedCommandStrs[matchedCount++] = command.pcCommand; 
                }
            }

            if(matchedCount == 0) continue;
            else if(matchedCount == 1)
            {
                commandStrLen = kStrLen(matchedCommandStrs[0]);
                for(commandStrIndex = iCommandBufferIndex; commandStrIndex < commandStrLen; commandStrIndex++)
                {
                    vcCommandBuffer[iCommandBufferIndex++] = matchedCommandStrs[0][commandStrIndex];
                    kPrintf("%c", matchedCommandStrs[0][commandStrIndex]);
                }
            }
            else
            {
                int minLen = 0x7fffffff;
                for(commandIndex = 0; commandIndex < matchedCount; commandIndex++)
                {
                    commandStrLen = kStrLen(matchedCommandStrs[commandIndex]);
                    if(minLen > commandStrLen) minLen = commandStrLen;
                }

                int maxSharedIndex = commandStrIndex;
                for(; maxSharedIndex < minLen; maxSharedIndex++)
                {
                    char cur = matchedCommandStrs[0][maxSharedIndex];
                    for(commandIndex = 1; commandIndex < matchedCount; commandIndex++)
                    {
                        if(cur != matchedCommandStrs[commandIndex][maxSharedIndex]) break;
                    }
                    if(commandIndex != matchedCount) break;
                }

                if(maxSharedIndex > commandStrIndex)
                {
                    for(int idx = commandStrIndex; idx < maxSharedIndex; idx++)
                    {
                        vcCommandBuffer[iCommandBufferIndex++] = matchedCommandStrs[0][idx];
                        kPrintf("%c", matchedCommandStrs[0][idx]);
                    }
                }
                else
                {
                    kPrintf("\n");
                    for(commandIndex = 0; commandIndex < matchedCount; commandIndex++)
                    {
                        kPrintf("%s", matchedCommandStrs[commandIndex]);
                        if(commandIndex < matchedCount - 1)
                        {
                            kPrintf(" ");
                        }
                    }
                    kPrintf("\n");
                    kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
                    for(iCommandTempBufferIndex = 0; iCommandTempBufferIndex < iCommandBufferIndex; iCommandTempBufferIndex++)
                    {
                        kPrintf("%c", vcCommandBuffer[iCommandTempBufferIndex]);
                    }
                }
            }
        }
        
		else if(bKey==KEY_RIGHT){
			kGetCursor( &iCursorX, &iCursorY );
			if(iCommandBufferIndex+7!=iCursorX)
			kSetCursor( iCursorX+1, iCursorY );
		}
		else if(bKey==KEY_LEFT){
			kGetCursor( &iCursorX, &iCursorY );
			if(7<iCursorX)
			kSetCursor( iCursorX-1, iCursorY );
		}

		else if( bKey == KEY_UP){
			

			if(current != 0 && current_history_flag ==0){
				up_count++;				
				current--;										
			}
			else if(current_history_flag ==1)
			{
				up_count++;
				

				current--;
				if(current == 9)
					current = 0;
				if(current == -1 )
					current =9;
			}
			
			kGetCursor( &iCursorX, &iCursorY );
			for( i=80;i>0;--i){
				kPrintStringXY( i , iCursorY, " " );
				kSetCursor( i, iCursorY );
			}
			kSetCursor( 0, iCursorY );
			kPrintf( "%s", CONSOLESHELL_PROMPTMESSAGE );
			kPrintf("%s",command_history[current]);
			
			iCommandBufferIndex = iCursorX;
	        	kMemCpy(vcCommandBuffer,command_history[current],iCommandBufferIndex);
			
		}

		else if( bKey == KEY_DOWN){
			
			down_count++;
			
				kGetCursor( &iCursorX, &iCursorY );
				int tmpCursorX = iCursorX;
				for( i=80;i>0;--i){
					kPrintStringXY( i , iCursorY, " " );
					kSetCursor( i, iCursorY );
				}
				kSetCursor( 0, iCursorY );
				kPrintf( "%s", CONSOLESHELL_PROMPTMESSAGE );
			
				
				if(current_history_flag == 0){	
					if(down_count >= up_count)
						kPrintf("");
					
					current++;	
					kPrintf("%s",command_history[current]);
					iCommandBufferIndex = tmpCursorX;
					kMemCpy(vcCommandBuffer,command_history[current],iCommandBufferIndex);
					
				}
			
				else if(current_history_flag == 1)
				{	
					if(down_count >= up_count)
					
						kPrintf("");
					
					else if(down_count--)
						kPrintf("");
					else {
						current++;				 
						if(current == 10)
							current = 0;
						kPrintf("%s",command_history[current]);
						iCommandBufferIndex = tmpCursorX;
						kMemCpy(vcCommandBuffer,command_history[current],iCommandBufferIndex);
					}
				}	

			
			/*
			else if(current == count )
				kPrintf("");
			else{
				current--;
				kPrintf("");
			}
			*/
		}
		else
        {
            // TAB은 공백으로 전환
            // if( bKey == KEY_TAB )
            // {
            //     bKey = ' ';
            // }
            
            // 버퍼에 공간이 남아있을 때만 가능
            if( iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT )
            {
                vcCommandBuffer[ iCommandBufferIndex++ ] = bKey;
                kPrintf( "%c", bKey );
            }
        }
		
	}
}

void kExecuteCommand( const char* pcCommandBuffer )
{
	int i, iSpaceIndex;
	int iCommandBufferLength, iCommandLength;
	int iCount;

	iCommandBufferLength = kStrLen( pcCommandBuffer );
	for( iSpaceIndex = 0 ; iSpaceIndex < iCommandBufferLength ; iSpaceIndex++ )
	{
		if( pcCommandBuffer[ iSpaceIndex ] == ' ' )
		{
			break;
		}
	}

	
	iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY );
	for( i = 0 ; i < iCount ; i++ )
	{
		iCommandLength = kStrLen( gs_vstCommandTable[ i ].pcCommand );
		
		if( ( iCommandLength == iSpaceIndex ) &&
				( kMemCmp( gs_vstCommandTable[ i ].pcCommand, pcCommandBuffer,
						   iSpaceIndex ) == 0 ) )
		{
			gs_vstCommandTable[ i ].pfFunction( pcCommandBuffer + iSpaceIndex + 1 );
			break;
		}
	}

	
	if( i >= iCount )
	{
		kPrintf( "'%s' is not found.\n", pcCommandBuffer );
	}
}

void kInitializeParameter( PARAMETERLIST* pstList, const char* pcParameter )
{
	pstList->pcBuffer = pcParameter;
	pstList->iLength = kStrLen( pcParameter );
	pstList->iCurrentPosition = 0;
}


int kGetNextParameter( PARAMETERLIST* pstList, char* pcParameter )
{
	int i;
	int iLength;

	
	if( pstList->iLength <= pstList->iCurrentPosition )
	{
		return 0;
	}

	for( i = pstList->iCurrentPosition ; i < pstList->iLength ; i++ )
	{
		if( pstList->pcBuffer[ i ] == ' ' )
		{
			break;
		}
	}

	kMemCpy( pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i );
	iLength = i - pstList->iCurrentPosition;
	pcParameter[ iLength ] = '\0';

	pstList->iCurrentPosition += iLength + 1;
	return iLength;
}

/**
 * Command functions
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

	
	for( i = 0 ; i < iCount ; i++ )
	{
		iLength = kStrLen( gs_vstCommandTable[ i ].pcCommand );
		if( iLength > iMaxCommandLength )
		{
			iMaxCommandLength = iLength;
		}
	}

	
	for( i = 0 ; i < iCount ; i++ )
	{
		kPrintf( "%s", gs_vstCommandTable[ i ].pcCommand );
		kGetCursor( &iCursorX, &iCursorY );
		kSetCursor( iMaxCommandLength, iCursorY );
		kPrintf( "  - %s\n", gs_vstCommandTable[ i ].pcHelp );
	}
}

static void kCls( const char* pcParameterBuffer )
{
	kClearScreen();
	kSetCursor( 0, 1 );
}

static void kShowTotalRAMSize( const char* pcParameterBuffer )
{
	kPrintf( "Total RAM Size = %d MB\n", kGetTotalRAMSize() );
}

static void kStringToDecimalHexTest( const char* pcParameterBuffer )
{
	char vcParameter[ 100 ];
	int iLength;
	PARAMETERLIST stList;
	int iCount = 0;
	long lValue;

	
	kInitializeParameter( &stList, pcParameterBuffer );

	while( 1 )
	{
		
		iLength = kGetNextParameter( &stList, vcParameter );
		if( iLength == 0 )
		{
			break;
		}

		kPrintf( "Param %d = '%s', Length = %d, ", iCount + 1, 
				vcParameter, iLength );

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

static void kShutdown( const char* pcParamegerBuffer )
{
	kPrintf( "System Shutdown Start...\n" );

	kPrintf( "Press Any Key To Reboot PC..." );
	kGetCh();
	kReboot();
}

static void kRaisefault(const char *pcParamegerBuffer)
{
    long *ptr = 0x1ff000;
	/**
	 * 0x1ff000 에 write(access) 시도 (Read-Only)
	 * Protection Fault & Page Fault 발생
     */
	*ptr = 0;
	/**
	 * 0x1ff000 에 read(access) 시도 (non-present)
	 * Page Fault 발생
	 */
	// kPrintf(*ptr);
}

static void kDummy(const char *dummy)
{
    kPrintf("Dummy\n");
}

static void kSetTimer( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    PARAMETERLIST stList;
    long lValue;
    BOOL bPeriodic;

 
    kInitializeParameter( &stList, pcParameterBuffer );
    
 
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)settimer 10(ms) 1(periodic)\n" );
        return ;
    }
    lValue = kAToI( vcParameter, 10 );

 
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)settimer 10(ms) 1(periodic)\n" );
        return ;
    }    
    bPeriodic = kAToI( vcParameter, 10 );
    
    kInitializePIT( MSTOCOUNT( lValue ), bPeriodic );
    kPrintf( "Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic );
}

 
static void kWaitUsingPIT( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;
    int i;
    
 
    kInitializeParameter( &stList, pcParameterBuffer );
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)wait 100(ms)\n" );
        return ;
    }
    
    lMillisecond = kAToI( pcParameterBuffer, 10 );
    kPrintf( "%d ms Sleep Start...\n", lMillisecond );
    
 
    kDisableInterrupt();
    for( i = 0 ; i < lMillisecond / 30 ; i++ )
    {
        kWaitUsingDirectPIT( MSTOCOUNT( 30 ) );
    }
    kWaitUsingDirectPIT( MSTOCOUNT( lMillisecond % 30 ) );   
    kEnableInterrupt();
    kPrintf( "%d ms Sleep Complete\n", lMillisecond );
    
 
    kInitializePIT( MSTOCOUNT( 1 ), TRUE );
}

 
static void kReadTimeStampCounter( const char* pcParameterBuffer )
{
    QWORD qwTSC;
    
    qwTSC = kReadTSC();
    kPrintf( "Time Stamp Counter = %q\n", qwTSC );
}

static void kMeasureProcessorSpeed( const char* pcParameterBuffer )
{
    int i;
    QWORD qwLastTSC, qwTotalTSC = 0;
        
    kPrintf( "Now Measuring." );
    
 
    kDisableInterrupt();    
    for( i = 0 ; i < 200 ; i++ )
    {
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT( MSTOCOUNT( 50 ) );
        qwTotalTSC += kReadTSC() - qwLastTSC;

        kPrintf( "." );
    }
 
    kInitializePIT( MSTOCOUNT( 1 ), TRUE );    
    kEnableInterrupt();
    
    kPrintf( "\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000 );
}

 
static void kShowDateAndTime( const char* pcParameterBuffer )
{
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

 
    kReadRTCTime( &bHour, &bMinute, &bSecond );
    kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );
    
    kPrintf( "Date: %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth,
             kConvertDayOfWeekToString( bDayOfWeek ) );
    kPrintf( "Time: %d:%d:%d\n", bHour, bMinute, bSecond );
}

static void kTestTask1( void )
{
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin, j;
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    
 
    pstRunningTask = kGetRunningTask();
    iMargin = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) % 10;
    
 
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
        
 
        pstScreen[ iY * CONSOLE_WIDTH + iX ].bCharactor = bData;
        pstScreen[ iY * CONSOLE_WIDTH + iX ].bAttribute = bData & 0x0F;
        bData++;
        
 
        //kSchedule();
    }

    //kExitTask();
}

static void kTestTask2( void )
{
    int i = 0, iOffset;
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    char vcData[ 4 ] = { '-', '\\', '|', '/' };
    
 
    pstRunningTask = kGetRunningTask();
    iOffset = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - 
        ( iOffset % ( CONSOLE_WIDTH * CONSOLE_HEIGHT ) );

    while( 1 )
    {
 
        pstScreen[ iOffset ].bCharactor = vcData[ i % 4 ];
 
        pstScreen[ iOffset ].bAttribute = ( iOffset % 15 ) + 1;
        i++;
        
 
        //kSchedule();
    }
}
 
static void kCreateTestTask( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcType[ 30 ];
    char vcCount[ 30 ];
    int i;
    
 
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcType );
    kGetNextParameter( &stList, vcCount );

    switch( kAToI( vcType, 10 ) )
    {
 
    case 0:
        for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        {    
            if( kCreateTask( TASK_FLAGS_HIGH | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 ) == NULL )
            {
                break;
            }
        }
        
        kPrintf( "Task1 %d Created\n", i );	

        
       
	for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        {    
            if( kCreateTask( TASK_FLAGS_MEDIUM | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 ) == NULL )
            {
                break;
            }
        }
        
        kPrintf( "Task1 %d Created\n", i );
       
        
 
    case 1:
	for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        {    
            if( kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 ) == NULL )
            {
                break;
            }
        }
        kPrintf( "Task2 %d Created\n", i );


        break;
    case 2:
        for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        {    
            if( kCreateTask( TASK_FLAGS_MEDIUM | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 ) == NULL )
            {
                break;
            }
        }
        kPrintf( "Task2 %d Created\n", i );
        break;
      
}   

}
static void kChangeTaskPriority( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcID[ 30 ];
    char vcPriority[ 30 ];
    QWORD qwID;
    BYTE bPriority;
    
 
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );
    kGetNextParameter( &stList, vcPriority );
    
 
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

 /*
static void kShowTaskList( const char* pcParameterBuffer )
{
    int i;
    TCB* pstTCB;
    int iCount = 0;
    
    kPrintf( "=========== Task Total Count [%d] ===========\n", kGetTaskCount() );
    for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
    {
 
        pstTCB = kGetTCBInTCBPool( i );
        if( ( pstTCB->stLink.qwID >> 32 ) != 0 )
        {
 
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
*/
static void kShowTaskList( const char* pcParameterBuffer )
{
    int i;
    TCB* pstTCB;
    int iCount = 0;
    
    kPrintf( "=========== Task Total Count [%d] ===========\n", kGetTaskCount() );
    for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
    {
        // TCB를 구해서 TCB가 사용 중이면 ID를 출력
        pstTCB = kGetTCBInTCBPool( i );
        if( ( pstTCB->stLink.qwID >> 32 ) != 0 )
        {
            // 태스크가 10개 출력될 때마다, 계속 태스크 정보를 표시할지 여부를 확인
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
            kPrintf( "Got_Time[%d]\n ", pstTCB->got_time);
		// kPrintf( " pass[%d]\n", pstTCB->pass);
        }
    }
}

 
static void kKillTask( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcID[ 30 ];
    QWORD qwID;
    TCB* pstTCB;
    int i;
    
 
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );
    
 
    if( kMemCmp( vcID, "0x", 2 ) == 0 )
    {
        qwID = kAToI( vcID + 2, 16 );
    }
    else
    {
        qwID = kAToI( vcID, 10 );
    }
    
 
    if( qwID != 0xFFFFFFFF )
    {
        pstTCB = kGetTCBInTCBPool( GETTCBOFFSET( qwID ) );
        qwID = pstTCB->stLink.qwID;

 
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
 
    else
    {
        for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
        {
            pstTCB = kGetTCBInTCBPool( i );
            qwID = pstTCB->stLink.qwID;

 
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

 
static void kCPULoad( const char* pcParameterBuffer )
{
    kPrintf( "Processor Load : %d%%\n", kGetProcessorLoad() );
}
    
 
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

 
static void kPrintNumberTask( void )
{
    int i;
    int j;
    QWORD qwTickCount;

 
    qwTickCount = kGetTickCount();
    while( ( kGetTickCount() - qwTickCount ) < 50 )
    {
        kSchedule();
    }    
    
 
    for( i = 0 ; i < 5 ; i++ )
    {
        kLock( &( gs_stMutex ) );
        kPrintf( "Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID,
                gs_qwAdder );
        
        gs_qwAdder += 1;
        kUnlock( & ( gs_stMutex ) );
    
 
        for( j = 0 ; j < 30000 ; j++ ) ;
    }
    
 
    qwTickCount = kGetTickCount();
    while( ( kGetTickCount() - qwTickCount ) < 1000 )
    {
        kSchedule();
    }    
    
 
    //kExitTask();
}

 
static void kTestMutex( const char* pcParameterBuffer )
{
    int i;
    
    gs_qwAdder = 1;
    
 
    kInitializeMutex( &gs_stMutex );
    
    for( i = 0 ; i < 3 ; i++ )
    {
 
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kPrintNumberTask );
    }    
    kPrintf( "Wait Util %d Task End...\n", i );
    kGetCh();
}

 
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

 
static volatile QWORD gs_qwRandomValue = 0;

 
QWORD kRandom( void )
{
    gs_qwRandomValue = ( gs_qwRandomValue * 412153 + 5571031 ) >> 16;
    return gs_qwRandomValue;
}

 
static void kDropCharactorThread( void )
{
    int iX, iY;
    int i;
    char vcText[ 2 ] = { 0, };

    iX = kRandom() % CONSOLE_WIDTH;
    
    while( 1 )
    {
 
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

 
    kGetCh();
}

 
static void kShowMatrix( const char* pcParameterBuffer )
{
    TCB* pstProcess;
    
    pstProcess = kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, ( void* ) 0xE00000, 0xE00000, 
                              ( QWORD ) kMatrixProcess );
    if( pstProcess != NULL )
    {
        kPrintf( "Matrix Process [0x%Q] Create Success\n" );

 
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
 * Command functions
 */
 extern TCB* result[];

 static void kShowResult()
 {
     kPrintf( "=========================================================\n" );
     for(int i = 0; i<TASK_MAXREADYLISTCOUNT; ++i){
         kPrintf( "stride : %d, got_time : %d\n", cal_stride(result[i]->qwFlags & 7),result[i]->got_time);
     }
     kPrintf( "=========================================================\n" );
 }


