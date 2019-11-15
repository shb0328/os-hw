#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"


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
    {"raisefault", "exception handler", kRaisefault},
    {"shutdummy", "dummy", kDummy}    

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
void kHelp( const char* pcCommandBuffer )
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

void kCls( const char* pcParameterBuffer )
{
	kClearScreen();
	kSetCursor( 0, 1 );
}

void kShowTotalRAMSize( const char* pcParameterBuffer )
{
	kPrintf( "Total RAM Size = %d MB\n", kGetTotalRAMSize() );
}

void kStringToDecimalHexTest( const char* pcParameterBuffer )
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

void kShutdown( const char* pcParamegerBuffer )
{
	kPrintf( "System Shutdown Start...\n" );

	kPrintf( "Press Any Key To Reboot PC..." );
	kGetCh();
	kReboot();
}

void kRaisefault(const char *pcParamegerBuffer)
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

void kDummy(const char *dummy)
{
    kPrintf("Dummy\n");
}
