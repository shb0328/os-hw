[BITS 32]

global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text

;CPUID를 반환
;PARAM : DWORD dwEAX, DWORD* pdwEAX, * pdw EBX, * pdwECX, *pdwEDX
kReadCPUID:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi

	mov eax, dword [ ebp + 8 ]
	cpuid

	;*pdwEAX
	mov esi, dword [ ebp + 12 ]
	mov dword [ esi ], eax

	;*pwdEBX
	mov esi, dword [ebp + 16]
	mov dword [ esi ], ebx

	;*pwdECX	
	mov esi, dword [ebp + 20]
	mov dword [ esi ], ecx

	;*pwdEDX
	mov esi, dword [ebp + 24]
	mov dword [ esi ], edx

	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret

;IA-32e 모드로 전환하고 64bit 커널을 수행
kSwitchAndExecute64bitKernel:

	;CR4 레지스터의 PAE 비트를 1로 설정
	mov eax, cr4
	or eax, 0x20
	mov cr4, eax

	;CR3 레지스터에 PML4 테이블의 어드레스와 캐시 활성화
	mov eax, 0x100000
	mov cr3, eax

	;IA32_EFER.LME 를 1로 설정하여 IA-32e 모드 활성화
	mov ecx, 0x0C0000080
	rdmsr

	or eax, 0x0100

	wrmsr

	;캐시 기능과 페이징 기능 활성화
	;CR0 레지스터의 NW(29bit) = 0, CD(30 bit) = 0, PG(31bit) = 1
	;WP(16bit) = 1 커널도 페이지테이블엔트리의 R/W flag의 영향을 받게 하기 위해서
	mov eax, cr0
	or eax,  0xE0010000
	xor eax, 0x60000000
	mov cr0, eax

	jmp 0x08:0x200000

	;jmp &

