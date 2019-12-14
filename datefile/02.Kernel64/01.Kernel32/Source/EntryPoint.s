[ORG 0x00]
[BITS 16]

SECTION .text

START:
    mov ax, 0x1000

    mov ds, ax
    mov es, ax

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; A20 게이트를 활성화
    ; BIOS를 이용한 전환이 실패했을 때 시스템 컨트롤 포트로 전환 시도
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; BIOS 서비스를 사용해서 A20 게이트를 활성화
    mov ax, 0x2401  ; A20 게이트 활성화 서비스 설정
    int 0x15        ; BIOS 인터럽트 서비스 호출

    jc .A20GATEERROR ; A20 게이트 활성화가 성공했는지 확인
    jmp .A20GATESUCCESS

.A20GATEERROR:
    ; 에러 발생 시, 시스템 컨트롤 포트로 전환 시도
    in al, 0x92     ; 시스템 컨트롤 포트에서 1바이트를 읽어 AL 레지스터에 저장
    or al, 0x02     ; 읽은 값에 A20 게이트 비트를 1로 설정
    and al, 0xFE    ; 시스템 리셋 방지를 위해 0xFE와 AND 연산하여 비트 0를 0으로 설정
    out 0x92, al    ; 시스템 컨트롤 포트(0x92) 에 변경된 값을 1 바이트 설정

.A20GATESUCCESS:
    cli                       ; 인터럽트 발생하지 못하도록 설정
; GDTR 자료구조 정의

    lgdt [ GDTR ] ; GDTR 자료구조를 프로세서에 설정하여 GDT 테이블을 로드

    mov eax, 0x4000003B ; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
    mov cr0, eax        ; CR0 컨트롤 레지스터에 위에서 저장한 플래그 설정하여 보호모드로 전환

; 커널 코드 세그먼트를 0x00을 기준으로 하는 것으로 교체하고 EIP의 값을 0x00을 기준으로 재설정
; CS 세그먼트 셀렉터: EIP

    jmp dword 0x18: (PROTECTEDMODE - $$ + 0x10000 )
; 실제 코드가 0x10000 기준으로 실행되므로

[BITS 32]
PROTECTEDMODE:
    mov ax, 0x20 ; 보호 모드 커널용 데이터 세그먼트 디스크립터를 AX 레지스터에 저장
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 스택을 0x00000000 ~ 0x0000FFFF 영역에 64KB 크기로 생성
    mov ss, ax  ;
    mov esp, 0xFFFE
    mov ebp, 0xFFFE

    push ( SWITCHSUCESSMESSAGE - $$ + 0x10000 )
    push 3
    push 0
    call PRINTMESSAGE
    add esp, 12

    jmp dword 0x18: 0x10200 ; C 언어 커널이 존재하는 0x10200 어드레스로 이동하여 C 언어 커널 수행


; 메시지를 출력하는 함수
; 스택에 x 좌표, y 좌표, 문자열
PRINTMESSAGE:
    push ebp        ; 베이스 포인터 레지스터를 스택에 삽입
    mov ebp, esp    ; 베이스 포인터 레지스터에 스택 포인터 레지스터의 값 설정
    push esi        ; 함수에서 임시로 사용되는 레지스터로
    push edi        ; 함수의 마지막 부분에서 스택에 삽입된 값을 꺼내 원래 값으로 복원
    push eax
    push ecx
    push edx

    ;;;;;;;;;;;;;;;;;;;;;;
    ; x, y의 좌표로 비디오 메모리의 어드레스 계산
    ;;;;;;;;;;;;;;;;;;;;;;
    ; y 좌표로 라인 어드레스 계산
    mov eax, dword [ ebp + 12 ]     ; 파라미터 2 - y 좌표를 EAX 레지스터에 설정
    mov esi, 160                    ; 한 라인의 바이트 수 2*80 을 ESI 레지스터에 설정
    mul esi                         ; EAX 레지스터와 ESI 레지스터를 곱하여 Y 좌표 계산
    mov edi, eax                    ; 계산된 화면 Y 어드레스  EDI 레지스터에 저장

    ; x 좌표를 이용해서 2를 곱한 후 최종 어드레스를 계산
    mov eax, dword [ ebp + 8 ]
    mov esi, 2                      ; 한 문자를 나타내는 바이트 수 2
    mul esi
    add edi, eax

    ; 출력할 문자열의 어드레스
    mov esi, dword [ ebp + 16 ]

.MESSAGELOOP:
    mov cl, byte [ esi ]            ; cl 레지스터는 ecx 레지스터 하위 1바이트

    cmp cl, 0
    je .MESSAGEEND

    mov byte [ edi + 0xB8000 ], cl  ; 0 이 아니라면 비디오 메모리 어드레스 0xB800 + EDI 에 출력
    ; 보호 모드에서는 32비트 오프셋이 가능하므로 별도의 세그먼트 셀렉터 사용하지 않고 바로 접근

    add esi, 1
    add edi, 2

    jmp .MESSAGELOOP

.MESSAGEEND:
    pop edx
    pop ecx
    pop eax
    pop edi
    pop esi
    pop ebp
    ret

align 8, db 0

dw 0x0000

GDTR:
    dw GDTEND - GDT - 1       ; GDT 전체 크기
    dd ( GDT - $$ + 0x10000 ) ; GDT 테이블 시작 어드레스

; GDT 정의

GDT:
    ; 널 디스크립터, 디스크립터의 크기는 8 바이트
    NULLDESCRIPTOR:
        dw 0x0000
        dw 0x0000
        db 0x00
        db 0x00
        db 0x00
        db 0x00

    ; IA-32e 모드 커널용 코드 세그먼트 디스크립터
    IA_32eCODEDESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A
        db 0xAF
        db 0x00

    IA_32eDATADESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92
        db 0xAF
        db 0x00



; 커널 코드 세그먼트와 데이터 세그먼트 디스크립터 생성 코드

CODEDESCRIPTOR:
    dw 0xFFFF   ; Limit [15:0]
    dw 0x0000   ; Base  [15:0]
    db 0x00     ; Base  [23:16]
    db 0x9A     ; P=1, DPL=0, Code Segment, Execute/Read
    db 0xCF     ; G=1, D=1, L=0 Limit[19:16]
    db 0x00     ; Base [31:24]

DATADESCRIPTOR:
    dw 0xFFFF   ; Limit [15:0]
    dw 0x0000   ; Base  [15:0]
    db 0x00     ; Base  [23:16]
    db 0x92     ; P=1, DPL=0, Data Segment, Read/Write
    db 0xCF     ; G=1, D=1, L=0 Limit[19:16]
    db 0x00     ; Base [31:24]
GDTEND:

SWITCHSUCESSMESSAGE: db 'Switch To Protected Mode Success~!!', 0

times 512 - ( $ - $$ ) db 0x00      ; 512 바이트를 맞추기 위해 남은 부분 0으로 채움

