#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"

// 함수 선언
void kPrintString( int iX, int iY, const char* pcString );
BOOL ReadTest();
BOOL WriteTest();
// 아래 함수는 C 언어 커널의 시작 부분
void Main( void )
{
	char vcTemp[2] = {0,};
	BYTE bFlags;
	BYTE bTemp;
	int i=0;
	KEYDATA stData;
	int iCursorX, iCursorY;

	kInitializeConsole(0,11);


	kPrintf( "Switch To IA-32e Mode Success~!!\n" );
	kPrintf( "IA-32e C Language Kernel Start..............[Pass]\n" );
	kPrintf( "Initialize Console...............[Pass]\n");

	kGetCursor(&iCursorX,&iCursorY);
	kPrintf( "GDT Initialize And Switch For IA-32e Mode...[    ]" );
	kInitializeGDTTableAndTSS();
	kLoadGDTR( GDTR_STARTADDRESS );
	kSetCursor( 45, iCursorY++);
	kPrintf( "Pass\n" );

	kPrintf( "TSS Segment Load............................[    ]" );
	kLoadTR( GDT_TSSSEGMENT );
	kSetCursor( 45, iCursorY++);
	kPrintf( "Pass\n" );

	kPrintf( "IDT Initialize..............................[    ]" );
	kInitializeIDTTables();    
	kLoadIDTR( IDTR_STARTADDRESS );
	kSetCursor( 45, iCursorY++);
	kPrintf( "Pass\n" );

	
    kPrintf( "Total RAM Size Check........................[    ]" );
    kCheckTotalRAMSize();
    kSetCursor( 45, iCursorY++ );
    kPrintf( "Pass], Size = %d MB\n", kGetTotalRAMSize() );

	kPrintf("TCB Pool And Scheduler Initialize.......[Pass]\n");
	iCursorY++;
	kInitializeScheduler();

	kPrintf( "Dynamic Memory Initialize...................[Pass]\n" );
    iCursorY++;
    kInitializeDynamicMemory();

	//1mx당 한번의 인터럽트
	kInitializePIT(MSTOCOUNT(1),1);


	kPrintf( "Keyboard Activate And Queue Initialize......[    ]" );
	// 키보드를 활성화
	if( kInitializeKeyboard()  == TRUE )
	{
		kSetCursor( 45, iCursorY++);
		kPrintf( "Pass\n" );
		kChangeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else{
		kSetCursor( 45, iCursorY++);
		kPrintf( "Fail\n" );
		while(1);
	}


	kPrintf( "PIC Controller And Interrupt Initialize.....[    ]");
	// PIC 컨트롤러 초기화 및 모든 인터럽트 활성화
	kInitializePIC();
	kMaskPICInterrupt(0); 
	//kSetCursor( 45, iCursorY++ );
	//kPrintf( "Pass2\n");
	kEnableInterrupt();
	kSetCursor( 45, iCursorY++ );
	kPrintf( "Pass\n");

	kPrintf("HDD Initialize..........[    ]");
	if(kInitializeHDD() == TRUE)
	{
		kSetCursor(25,iCursorY++);
		kPrintf("Pass\n");
	}
	else{
		kSetCursor(25,iCursorY++);
		kPrintf("Fail\n");
	}

    
    // 파일 시스템을 초기화
    kPrintf( "File System Initialize..    " );
  
    if( kInitializeFileSystem() == TRUE )
    {
        kSetCursor(25, iCursorY++ );
        kPrintf( "Pass\n" );
    }
    else
    {
        kSetCursor(25, iCursorY++ );
        kPrintf( "Fail\n" );
    }

	// 유휴 태스크를 생성하고 셸을 시작
	kCreateTask( TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE,0,0,(QWORD)kIdleTask);
	kStartConsoleShell();

}

// 문자열 출력 함수
void kPrintString( int iX, int iY, const char* pcString )
{
	CHARACTER* pstScreen = ( CHARACTER* ) 0xAB8000;
	int i;

	pstScreen += ( iY * 80  ) + iX;
	for( i = 0 ; pcString[ i ] != 0 ; i++ )
	{
		pstScreen[ i ].bCharactor = pcString[ i ];
	}
}

// Read_Only  Read 기능을 테스트하는 함수
BOOL ReadTest( void )
{
	DWORD testValue = 0xFF;

	DWORD* pdwCurrentAddress = ( DWORD* ) 0x1ff000;

	testValue = *pdwCurrentAddress;

	if(testValue != 0xFF)
	{
		return TRUE;
	}
	return FALSE;
}

// Read_Only  Write 기능을 테스트하는 함수
BOOL WriteTest( void )
{
	DWORD* pdwCurrentAddress = ( DWORD* ) 0x1ff000;
	*pdwCurrentAddress =0x12345678;

	if(*pdwCurrentAddress !=0x12345678){
		return FALSE;
	}
	return TRUE;
}
