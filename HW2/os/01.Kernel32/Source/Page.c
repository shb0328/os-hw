#include "Page.h"

void kInitializePageTables(void){
	PML4TENTRY* pstPML4TEntry;
	PDPTENTRY* pstPDPTEntry;
	PDENTRY* pstPDEntry;
	PTENTRY* pstPTEntry;
	DWORD dwMappingAddress;
	int i;

	// PML4 테이블 생성
	// 첫 번째 엔트리 외에 나머지는 모두 0으로 초기화
	pstPML4TEntry = ( PML4TENTRY* ) 0x100000;
	kSetPageEntryData( &( pstPML4TEntry[ 0 ] ), 0x00, 0x101000, PAGE_FLAGES_DEFAULT, 0);
	for( i=1; i<PAGE_MAXENTRYCOUNT; i++ ){
		kSetPageEntryData( &( pstPML4TEntry[ i ] ), 0, 0, 0, 0 );
	}

	// 페이지 디렉터리 포인터 테이블 생성
	// 하나의 PDPT로 512GByte까지 매핑 가능하므로 하나로 충분함
	// 64개의 엔트리를 설정하여 64GByte까지 매핑함
	pstPDPTEntry = ( PDPTENTRY* ) 0x101000;
	for( i =0; i<64; i++ ){
		kSetPageEntryData( &( pstPDPTEntry[ i ] ), 0, 0x102000 + ( i*PAGE_TABLESIZE ), PAGE_FLAGES_DEFAULT, 0 );
			}
	for( i=64; i < PAGE_MAXENTRYCOUNT; i++ ){
		kSetPageEntryData( &( pstPDPTEntry[ i ] ), 0, 0, 0, 0 );
	}

	// Create Table Page Directory
	pstPDEntry = ( PDENTRY*) 0x102000;
	dwMappingAddress = 0;
	for( i=0; i<PAGE_MAXENTRYCOUNT*64; i++){
		kSetPageEntryData( &(pstPDEntry[i] ), (i * (PAGE_DEFAULTSIZE >>20)) >>12, dwMappingAddress, PAGE_FLAGES_DEFAULT | PAGE_FLAGES_PS, 0 );
	
		dwMappingAddress += PAGE_DEFAULTSIZE;
	}
}

// Set Flag to Page Entry
void kSetPageEntryData( PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags ){
	pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
	pstEntry->dwUpperBaseAddressAndEXB = ( dwUpperBaseAddress & 0xFF ) | dwUpperFlags;
}
