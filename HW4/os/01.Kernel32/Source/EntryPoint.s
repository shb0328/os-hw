[ORG 0x00]
[BITS 16]

SECTION .text

START:
    mov ax, 0x1000
    mov ds, ax
    mov es, ax

    mov ss, ax
    mov sp, 0xFFFE
    mov bp, 0xFFFE

    mov ax, 0xB800
    mov fs, ax  

RAMSIZE_INIT:
    xor ebx, ebx
    xor edi, edi
    xor esi, esi ;acc high
    xor ebp, ebp ;acc low
RAMSIZELOOP:
    mov ecx, 20
    mov edx, 0x534d4150
    mov eax, 0xE820
    int 0x15

    jc .RAMSIZEINTURRUPT_ERROR

.RAMSIZEINTURRUPT_SUCCESS:
    mov eax, dword [es : di + 16] ;TYPE
    cmp eax, 1 ;ARM = 1, ARR = 2
    jne .RAMSIZEEND

.RAMSIZE_SUM:
    mov eax, dword [es : di + 12] ;HIGH
    add esi, eax

    mov eax, dword [es : di + 8] ;LOW
    add ebp, eax
    jnc .RAMSIZEEND

.ADD_CARRY:
    inc esi

.RAMSIZEEND:
    ;if complete, bx(offset) = 0   
    or bx, bx
    jne RAMSIZELOOP

.PARSE_RAMSIZE:
    mov eax, esi
    and eax, 0xFFFFF

    shr ebp, 20
    shl eax, 12
    or ebp, eax 

    shr esi, 20 

.RAMSIZE_PRINT:
    push (RAMSIZEMESSAGE - $$ + 0x10000)
    push 3
    push 0
    call .16BITPRINTMESSAGE
    add sp, 6

    mov ax, bp
    call .PRINTDECNUM

;A20 GATE 활성화
    mov ax, 0x2401
    int 0x15 ; using bios, enable a20 gate

    jc .A20GATEERROR
    jmp .A20GATESUCCESS

.RAMSIZEINTURRUPT_ERROR:
    push (RAMSIZEINTURRUPTERRORMESSAGE - $$ + 0x10000)
    push 3
    push 0
    call .16BITPRINTMESSAGE
    add sp, 6

.A20GATEERROR:
    in al, 0x92 ; using system port, enable a20 gate
    or al, 0x02
    and al, 0xFE
    out 0x92, al

.A20GATESUCCESS:    
    cli
    lgdt [ GDTR ]

    mov eax, 0x4000003B
    mov cr0, eax

    jmp dword 0x18: ( PROTECTEDMODE - $$ + 0x10000 )

;function
.PRINTDECNUM:
    ;input num : ax
    push ax
    push dx
    push si
    push di
    push cx

    mov si, 10
    xor cx, cx
.DIV:
    xor dx, dx
    div si
    push dx

    inc cx
    or ax, ax
    jne .DIV

    mov di, 160*3 + 18
.PRINT:
    pop dx
    add dx, '0'

    mov byte[fs : di], dl
    add di,2

    dec cx
    or cx, cx
    jne .PRINT

.PRINTDECNUMEND:
    pop cx
    pop di
    pop si
    pop dx
    pop ax
    ret

;function
.16BITPRINTMESSAGE:
    push bp          
    mov bp, sp       
                     
    push es          
    push si          
    push di          
    push ax
    push cx
    push dx
        
    mov ax, 0xB800                                                
    mov es, ax                   
      
    mov ax, word [ bp + 6 ]      
    mov si, 160                  
    mul si                       
    mov di, ax                   
        
    mov ax, word [ bp + 4 ]      
    mov si, 2                    
    mul si                       
    add di, ax                   
 
    mov si, word [ bp + 8 ]      

.16BITMESSAGELOOP:                
    mov cl, byte [ si ]      
 
    cmp cl, 0                
    je .16BITMESSAGEEND           

    mov byte [ es: di ], cl  
    
    add si, 1                
    add di, 2                
                                                     
    jmp .16BITMESSAGELOOP         

.16BITMESSAGEEND:
    pop dx       
    pop cx       
    pop ax       
    pop di       
    pop si       
    pop es
    pop bp       
    ret

;;;;;;;;;;;;;;;;;;
;;보호모드로 진입;;
;;;;;;;;;;;;;;;;;;

[BITS 32]
PROTECTEDMODE:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ss, ax
    mov esp, 0xFFFE
    mov ebp, 0xFFFE
      
    push ( SWITCHSUCCESSMESSAGE - $$ + 0x10000 )
    push 4
    push 0
    call PRINTMESSAGE
    add esp, 12

    jmp dword 0x18: 0x10400 ; C 언어 커널이 존재하는 0x10400 어드레스로 이동하여 C 언어 커널 수행

PRINTMESSAGE:
    push ebp
    mov ebp, esp
    push esi
    push edi
    push eax
    push ecx
    push edx
    
    mov eax, dword [ ebp + 12 ]
    mov esi, 160
    mul esi
    mov edi, eax

    mov eax, dword [ ebp + 8 ]
    mov esi, 2
    mul esi
    add edi, eax

    mov esi, dword [ ebp + 16 ]

.MESSAGELOOP:
    mov cl, byte [ esi ]
    cmp cl, 0
    je .MESSAGEEND

    mov byte [ edi + 0xB8000 ], cl

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

;;;;;;;;;;;;;;;;;;;;
;;   데이터 영역   ;;
;;;;;;;;;;;;;;;;;;;;

align 8, db 0

dw 0x0000
GDTR:
    dw GDTEND - GDT - 1
    dd ( GDT - $$ + 0x10000 )
GDT:
    NULLDescriptor:
        dw 0x0000
        dw 0x0000
        db 0x00
        db 0x00
        db 0x00
        db 0x00

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

    CODEDESCRIPTOR:     
        dw 0xFFFF       ; Limit [15:0]
        dw 0x0000       ; Base [15:0]
        db 0x00         ; Base [23:16]
        db 0x9A         ; P=1, DPL=0, Code Segment, Execute/Read
        db 0xCF         ; G=1, D=1, L=0, Limit[19:16]
        db 0x00         ; Base [31:24]  

    DATADESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92         ; P=1, DPL=0, Data Segment, Read/Write
        db 0xCF
        db 0x00
GDTEND:

SWITCHSUCCESSMESSAGE: db 'Switch To Protected Mode Success~!!', 0
RAMSIZEINTURRUPTERRORMESSAGE: db 'INTURRUPT ERROR~~!', 0
RAMSIZEMESSAGE: db 'RAMSIZE :  MB', 0

times 1024 - ( $ - $$ )  db  0x00
