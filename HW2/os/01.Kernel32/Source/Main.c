/**
 *  file    Main.c
 *  date    2008/12/14
 *  author  kkamagui 
 *          Copyright(c)2008 All rights reserved by kkamagui
 *  brief   C 언어로 작성된 커널의 엔트리 포인트 파일
 */

#include "Types.h"

void kPrintString( int iX, int iY, const char* pcString );
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough(void);
/**
 *  아래 함수는 C 언어 커널의 시작 부분임
 *      반드시 다른 함수들 보다 가장 앞쪽에 존재해야 함
 */
void Main( void )
{
    DWORD i;
    kPrintString( 0, 5, "C Language Kernel Started~!!!" );

    kPrintString(0, 6, "Minimum Memory Size Check...................[    ]");
    if(kIsMemoryEnough() == FALSE) {
        kPrintString(45, 6, "Fail");
        kPrintString(0, 7, "Not Enough Memory~!! MINT64 OS Requires Over 64 Mbyte Memory~!!");
        while(1);
    } else {
        kPrintString(45, 6, "Pass");
    }

    kPrintString(0, 7, "IA-32e Kernel Area Initialize...............[    ]");
    if(kInitializeKernel64Area() == FALSE) {
        kPrintString(45, 7, "Fail");
        kPrintString(0, 8, "Kernel Area Initialization Fail~!!");
        while (1);
    }
    kPrintString(45, 7, "Pass");


    while( 1 ) ;
}

/**
 *  문자열을 X, Y 위치에 출력
 */
void kPrintString( int iX, int iY, const char* pcString )
{
    CHARACTER* pstScreen = ( CHARACTER* ) 0xB8000;
    int i;
    
    // X, Y 좌표를 이용해서 문자열을 출력할 어드레스를 계산
    pstScreen += ( iY * 80 ) + iX;
    
    // NULL이 나올 때까지 문자열 출력
    for( i = 0 ; pcString[ i ] != 0 ; i++ )
    {
        pstScreen[ i ].bCharactor = pcString[ i ];
    }
}

BOOL kInitializeKernel64Area(void) {
    DWORD* pdwCurrentAddress;

    pdwCurrentAddress = (DWORD*) 0x100000; // initialize start address

    while ((DWORD) pdwCurrentAddress < 0x600000){ // til 6mb, initialize with 0
        *pdwCurrentAddress = 0x00;

        if (*pdwCurrentAddress !=0) {
            return FALSE;
        }
        pdwCurrentAddress++;
    }
    return TRUE;
}

BOOL kIsMemoryEnough(void) {
    DWORD* pdwCurrentAddress;

    pdwCurrentAddress = (DWORD*) 0x100000;

    while((DWORD) pdwCurrentAddress < 0x4000000) {
        *pdwCurrentAddress = 0x12345678;

        if (*pdwCurrentAddress != 0x12345678) {
            return FALSE;
        }

        pdwCurrentAddress += (0x100000 / 4);
    }
    return TRUE;
}