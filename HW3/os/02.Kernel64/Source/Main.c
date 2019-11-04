#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"

void kPrintString(int iX, int iY, const char *pcString);
BOOL ReadTest();
BOOL WriteTest();
BOOL RW_ReadTest();
BOOL RW_WriteTest();
void Main(void)
{
    int iCursorX, iCursorY;
    // 콘솔을 먼저 초기화한 후, 다음 작업을 수행
    kInitializeConsole( 0, 13 );

    // 부팅 상황을 화면에 출력
    kPrintf( "Switch To IA-32e Mode Success~!!\n" );
    kPrintf( "IA-32e C Language Kernel Start..............[Pass]\n" );
    kPrintf( "Initialize Console..........................[Pass]\n" );
    // kPrintf("This message is printed through the video memory relocated to 0xAB8000\n");

    kGetCursor( &iCursorX, &iCursorY );
    kPrintf("Read from 0x1fe000 [  ]");
    if (RW_ReadTest()) {
        kSetCursor(20, iCursorY++);
        kPrintf("OK\n");
    }

    kPrintf("Write to 0x1fe000 [  ]");
    if (RW_WriteTest()) {
        kSetCursor(19, iCursorY++);
        kPrintf("OK\n");
    }

    // kPrintf("Read from 0x1ff000 [  ]");
    // if (ReadTest()) {
    //     kSetCursor(20, iCursorY++);
    //     kPrintf("OK\n");
    // }

//HW2 : 아래 주석을 해제하고 프로그램을 실행 시키면, 0x1ff000 주소에 write가 거부되어 재부팅됨
    // kPrintString(0, 19, "Write to 0x1ff000 [  ]");
    // if (WriteTest())
    //     kPrintString(19, 19, "OK");

    kPrintf("GDT Initialize And Switch For IA-32e Mode...[    ]" );
    kInitializeGDTTableAndTSS();
    kLoadGDTR( GDTR_STARTADDRESS );
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");
    
    kPrintf("TSS Segment Load............................[    ]" );
    kLoadTR( GDT_TSSSEGMENT );
    kSetCursor( 45, iCursorY++ );
    kPrintf("Pass\n");
    
    kPrintf("IDT Initialize..............................[    ]" );
    kInitializeIDTTables();
    kLoadIDTR( IDTR_STARTADDRESS );
    kSetCursor( 45, iCursorY++ );
    kPrintf("Pass\n");

    kPrintf( "Total RAM Size Check........................[    ]" );
    kCheckTotalRAMSize();
    kSetCursor( 45, iCursorY++ );
    kPrintf( "Pass], Size = %d MB\n", kGetTotalRAMSize() );

    // 키보드를 활성화
    kPrintf("Keyboard Activate And Queue Initialize......[    ]" );
    if( kInitializeKeyboard() == TRUE )
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
        kChangeKeyboardLED( FALSE, FALSE, FALSE );
    }
    else
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
        while( 1 ) ;
    }

    kPrintf("PIC Controller And Interrupt Initialize.....[    ]" );
    // PIC 컨트롤러 초기화 및 모든 인터럽트 활성화
    kInitializePIC();
    kMaskPICInterrupt( 0 );
    kEnableInterrupt();
    
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    // start shell
    kStartConsoleShell();
}

/**
 *  문자열을 X, Y 위치에 출력
 */
void kPrintString(int iX, int iY, const char *pcString)
{
    CHARACTER *pstScreen = (CHARACTER *)0xAB8000;
    int i;

    // X, Y 좌표를 이용해서 문자열을 출력할 어드레스를 계산
    pstScreen += (iY * 80) + iX;

    // NULL이 나올 때까지 문자열 출력
    for (i = 0; pcString[i] != 0; i++)
    {
        pstScreen[i].bCharactor = pcString[i];
    }
}
// Read_Only  Read 기능을 테스트하는 함수
BOOL ReadTest(void)
{
    DWORD testValue = 0xFF;

    DWORD *pdwCurrentAddress = (DWORD *)0x1FF000;

    testValue = *pdwCurrentAddress;

    if (testValue != 0xFF)
    {
        return TRUE;
    }
    return FALSE;
}

// Read_Only  Write 기능을 테스트하는 함수
BOOL WriteTest(void)
{
    DWORD *pdwCurrentAddress = (DWORD *)0x1FF000;
    *pdwCurrentAddress = 0x12345678;

    if (*pdwCurrentAddress != 0x12345678)
    {
        return FALSE;
    }
    return TRUE;
}

// Read_Write  Read 기능을 테스트하는 함수
BOOL RW_ReadTest(void)
{
    DWORD testValue = 0xFF;

    DWORD *pdwCurrentAddress = (DWORD *)0x1FE000;

    testValue = *pdwCurrentAddress;

    if (testValue != 0xFF)
    {
        return TRUE;
    }
    return FALSE;
}

// Read_Only  Write 기능을 테스트하는 함수
BOOL RW_WriteTest(void)
{
    DWORD *pdwCurrentAddress = (DWORD *)0x1FE000;
    *pdwCurrentAddress = 0x12345678;

    if (*pdwCurrentAddress != 0x12345678)
    {
        return FALSE;
    }
    return TRUE;
}
