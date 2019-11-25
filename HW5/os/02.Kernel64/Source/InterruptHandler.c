#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"

void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags)
{
    pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
    pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}

void kDistinguishException(int iVectorNumber, QWORD qwErrorCode)
{
    int page_mask = 0x00000001;
    int protection_mask = 0x00000002;

    if ((qwErrorCode & page_mask) == 0)
    {
        kPageFaultExceptionHandler(iVectorNumber, qwErrorCode);
    }
    else if ((qwErrorCode & protection_mask) == 2)
    {
        kProtectionFaultExceptionHandler(iVectorNumber, qwErrorCode);
    }
}
void kPageFaultExceptionHandler(int iVectorNumber, QWORD qwErrorCode)
{
    char vcBuffer[7] = {
        0,
    };
    int number = 0;
    int mask = 0x00F00000;
    PTENTRY *pstPTEntry;

    for (int i = 0; i < 6; i++)
    {
        if ((number = ((iVectorNumber & mask) >> (20 - i * 4))) <= 9)
        {
            vcBuffer[i] = '0' + number;
        }
        else
        {
            vcBuffer[i] = 87 + number;
        }
        mask >>= 4;
    }

    kPrintf("====================================================\n");
    kPrintf("                 Page Fault Occur~!!!!               \n");
    kPrintf("                    Address: 0x%s ", vcBuffer);
    kPrintf("\n====================================================\n");

    pstPTEntry = (PTENTRY *)0x142000;
    kSetPageEntryData(&(pstPTEntry[511]), 0, 0x1FF000, 0x00000001, 0); 
    // invlpg(&iVectorNumber);

    //EOI
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}
void kProtectionFaultExceptionHandler(int iVectorNumber, QWORD qwErrorCode)
{
    char vcBuffer[7] = {
        0,
    };
    int number = 0;
    int mask = 0x00F00000;
    PTENTRY *pstPTEntry;

    for (int i = 0; i < 6; i++)
    {
        if ((number = ((iVectorNumber & mask) >> (20 - i * 4))) <= 9)
            vcBuffer[i] = '0' + number;
        else
            vcBuffer[i] = 87 + number;
        mask >>= 4;
    }

    kPrintf("====================================================\n");
    kPrintf("                 Protection Fault Occur~!!!!               \n");
    kPrintf("                    Address: 0x%s ", vcBuffer);
    kPrintf("\n====================================================\n");

    pstPTEntry = (PTENTRY *)0x142000;
    kSetPageEntryData(&(pstPTEntry[511]), 0, 0x1FF000, 0x00000003, 0);
    // invlpg(&iVectorNumber);

    //EOI
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}


void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode )
{
    char vcBuffer[ 3 ] = { 0, };

   
    vcBuffer[ 0 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 1 ] = '0' + iVectorNumber % 10;

    kPrintStringXY( 0, 0, "====================================================" );
    kPrintStringXY( 0, 1, "                 Exception Occur~!!!!               " );
    kPrintStringXY( 0, 2, "                    Vector:                         " );
    kPrintStringXY( 27, 2, vcBuffer );
    kPrintStringXY( 0, 3, "====================================================" );

    while( 1 ) ;
}

void kCommonInterruptHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;

    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
   
    vcBuffer[ 8 ] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = ( g_iCommonInterruptCount + 1 ) % 10;
    kPrintStringXY( 70, 0, vcBuffer );
   

    // EOI 
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}

void kKeyboardHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;

    
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    
    vcBuffer[ 8 ] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = ( g_iKeyboardInterruptCount + 1 ) % 10;
    kPrintStringXY( 0, 0, vcBuffer );
    //====================================================
    if( kIsOutputBufferFull() == TRUE )
    {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue( bTemp );
    }

    // EOI 
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}

//타이머 인터럽트의 핸들러
void kTimerHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iTimerInterruptCount = 0;

    //========================================================
    //인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    //인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    //발생한 횟수 출력
    vcBuffer[8] = '0' + g_iTimerInterruptCount;
    g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
    kPrintStringXY(70,0,vcBuffer);
    //========================================================

    //EOI전송
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

    //타이머 발생 횟수를 증가
    g_qwTickCount++;

    //태스크가 사용한 프로세서의 시간을 줄임
    kDecreaseProcessorTime();
    //프로세서가 사용할 수 있는 시간을 다 썼다면 태스크 전환 수행
    if(kIsProcessorTimeExpired() == TRUE)
    {
        kScheduleInInterrupt();
    }
}

// static inline void invlpg(int *m)
static inline void invlpg(void *m)
{
    //kPrintf("%x",*m);
    asm volatile("invlpg (%0)"
                 :
                 : "b"(m)
                 : "memory");
}
