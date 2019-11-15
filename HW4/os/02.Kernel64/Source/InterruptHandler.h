#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"
#include "Page.h"

///////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////
void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode );
void kCommonInterruptHandler( int iVectorNumber );
void kKeyboardHandler( int iVectorNumber );
void kDistinguishException(int iVectorNumber, QWORD qwErrorCode);
void kPageFaultExceptionHandler( int iVectorNumber, QWORD qwErrorCode );
void kProtectionFaultExceptionHandler( int iVectorNumber, QWORD qwErrorCode );
void kSetPageEntryData( PTENTRY* pstEntry, DWORD dwUpperBaseAddress, 
        DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags );
static inline void invlpg(void* m);
#endif /*__INTERRUPTHANDLER_H__*/