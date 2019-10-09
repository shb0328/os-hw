#ifndef __PAGE_H__
#define __PAGE_H__

#include "Types.h"

#define PAGE_FLAGES_P     0x00000001 //present
#define PAGE_FLAGES_RW    0x00000002 // read/write
#define PAGE_FLAGES_US    0x00000004 // user/supervisor
#define PAGE_FLAGES_PWT   0x00000008 // page level write-through
#define PAGE_FLAGES_PCD   0x00000010
#define PAGE_FLAGES_A     0x00000020
#define PAGE_FLAGES_D     0x00000040
#define PAGE_FLAGES_PS    0x00000080 
#define PAGE_FLAGES_G     0x00000100
#define PAGE_FLAGES_PAT   0x00001000
#define PAGE_FLAGES_EXB   0x80000000
#define PAGE_FLAGES_DEFAULT (PAGE_FLAGES_P | PAGE_FLAGES_RW)
#define PAGE_TABLESIZE    0x1000 // 8byte
#define PAGE_MAXENTRYCOUNT 512
#define PAGE_DEFAULTSIZE  0x200000

#pragma pack (push , 1)

typedef struct kPageTableEntryStruct
{

	DWORD dwAttributeAndLowerBaseAddress;
	DWORD dwUpperBaseAddressAndEXB;

} PML4TENTRY, PDPTENTRY, PDENTRY, PTENTRY;

#pragma pack (pop)

void kInitializePageTables (void);
void kSetPageEntryData( PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlages, DWORD dwUpperFlags);

#endif /*__page_H__*/
