#include "Types.h"

void kPrintString(int iX, int iY, const char *pcString);
BOOL ReadTest();
BOOL WriteTest();
BOOL RW_ReadTest();
BOOL RW_WriteTest();
void Main(void)
{
    kPrintString(0, 13, "Switch To IA-32e Mode Success~!!");
    kPrintString(0, 14, "IA-32e C Languege Kernel Start..............[Pass]");
    kPrintString(0, 15, "This message is printed through the video memory relocated to 0xAB8000");

    kPrintString(0, 16, "Read from 0x1fe000 [  ]");
    if (RW_ReadTest())
        kPrintString(20, 16, "OK");

    kPrintString(0, 17, "Write to 0x1fe000 [  ]");
    if (RW_WriteTest())
        kPrintString(19, 17, "OK");

    kPrintString(0, 18, "Read from 0x1ff000 [  ]");
    if (ReadTest())
        kPrintString(20, 18, "OK");

//HW2 : 아래 주석을 해제하고 프로그램을 실행 시키면, 0x1ff000 주소에 write가 거부되어 재부팅됨
/*
    kPrintString(0, 19, "Write to 0x1ff000 [  ]");
    if (WriteTest())
        kPrintString(19, 19, "OK");
*/
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
