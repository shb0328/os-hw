# file      BootLoader.asm

[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07C0:START ;bios가 MBR을 읽어들여 메모리 0x07C00에 복사한후에 0x7C00으로 점프, cs:0, ip: 0x07c0

START:
    mov ax, 0x07C0
    mov ds, ax ; ds = cs
    mov ax, 0xB800
    mov fs, ax ; fs video memory segment

    xor si, si

.SCREENCLEARLOOP:
    mov byte [ fs: si ], 0
    mov byte [ fs: si + 1 ], 0x0A

    inc si
    inc si
    cmp si, 80 * 25 * 2 ; video memory size

    jl .SCREENCLEARLOOP

    xor si, si

.MESSAGELOOP:
    mov cl, byte [ si + MESSAGE1 ]
    cmp cl, 0
    je .MESSAGEEND

    mov byte [ fs: di ], cl
    inc si
    inc di
    inc di

    jmp .MESSAGELOOP

.MESSAGEEND:

.DATEMESSAGECINIT:
    xor si, si; string index
    mov di, 80 * 2; fs index, second line
.DATEMESSAGECONSTLOOP:
    mov cl, byte [DATEMESSAGECONST + si]
    cmp cl, 0
    je .DATEINTERRUPT

    mov byte [fs:di], cl
    inc si
    inc di
    inc di
    jmp .DATEMESSAGECONSTLOOP

.DATEINTERRUPT:
    mov ax, 0x0400 ; ah: 0x04, al: 0x00
    xor cx, cx
    xor dx, dx
    int 0x1a ; centry: ch, year: cl, month: dh, day: dl

.PRINT_DATE:
    mov al, dl
    call .PRINT_BCD_AL_CONST

    mov byte [fs:di], '/' ; print slash
    inc di
    inc di

    mov al, dh
    call .PRINT_BCD_AL_CONST

    mov byte [fs:di], '/' ; print slash
    inc di
    inc di

    mov al, ch
    call .PRINT_BCD_AL_CONST

    mov al, cl
    call .PRINT_BCD_AL_CONST

    mov byte [fs:di], ' '
    inc di 
    inc di

    ; ch: centry, cl: year, dh: month, dl: day

.CONVERT_YEAR:
    mov bx, cx ; cx, bx is 2019 in bcd
    call .BX_BCD_TO_INT_TO_SI_BYTE ; si = 19, integer
    mov bp, si 


    shr bx, 8
    call .BX_BCD_TO_INT_TO_SI_BYTE 


    mov ax, 100
    push dx
    mul si 
    pop dx

    add bp, ax 

.CONVERT_MONTH_DAY:
    ; bp = 2019 , dh: month bcd, dl: day bcd
    mov bx, dx
    call .BX_BCD_TO_INT_TO_SI_BYTE
    mov cx, si; cl = si, cl = day integer
    xor ch, ch

    shr bx, 8
    call .BX_BCD_TO_INT_TO_SI_BYTE
    mov bx, si; bl = si = month integer
    sub bp, 1899
    ; bx = month integer, cx = day integer, bp = year integer, 2019

.CHECK_MONTH_UNDER_FEB:
    cmp bx, 3
    jge .ELSE

.IF_UNDER_FEB:
    add cx, bp
    dec bp
    jmp .CALC_WEEKDAY

.ELSE:
    dec bp
    dec bp
    add cx, bp
    inc bp
    inc bp
    ;cx : d, bp: y, bx: m

.CALC_WEEKDAY:
    ; cx = d = accumulator
    add cx, 4

    mov si, 23
    mov ax, bx
    mul si 

    mov si, 9
    xor dx, dx
    div si 

    add cx, ax
  
    push bp
    mov ax, bp

    mov si, 100
    xor dx, dx
    div si 
    sub cx, ax

    shr ax, 2 
    add cx, ax

    pop bp
    shr bp, 2 
    add cx, bp

.GET_REMAINDER:
    mov ax, cx
    mov cx, 7
    xor dx, dx
    div cx ; current day / 7, dx is remainder of day


.PRINT_DAY_OF_WEEK:
    mov bx, 3
    mov ax, dx
    mul bx ; ax = 3dx
    mov si, ax ; si = ax = 3dx
    
    mov dl, byte [DAYOFWEEK + si]
    mov byte [fs:di], dl

    mov dl, byte [DAYOFWEEK + si + 1]
    mov byte [fs:di + 2], dl
    
    mov dl, byte [DAYOFWEEK + si + 2]
    mov byte [fs:di + 4], dl



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                               *chain loading                                    ;;
;;                               Kernel Loader 읽어오고 제어를 넘김                 ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.RESET_DISK:
    xor ax, ax
    xor dl, dl
    int 0x13
    jc .ERROR

    mov si, 0x07E0
    mov es, si
    xor bx, bx

.READ_DATA:

    mov ah, 0x02  ;      BOIS service number : read sector                
    mov al, 0x1   ;      #sector to read                 
    mov ch, byte [ TRACKNUMBER ]         
    mov cl, byte [ SECTORNUMBER ]        
    mov dh, byte [ HEADNUMBER ] 
    mov dl, 0x00                         
    int 0x13    

    jc .ERROR

.READ_END:

    jmp 0x07E0:0x0000


;예외처리
.ERROR:
    xor si, si
    mov di, 80 * 2 * 2

.ERRORMESSAGELOOP:
    mov cl, byte [ si + ERRORMESSAGE ]
    cmp cl, 0
    je .END

    mov byte [ fs: di ], cl
    inc si
    inc di
    inc di
    jmp .ERRORMESSAGELOOP

.END:
    jmp $



;;;;;;;;;;;;;;;;;;;;;;
;;     함수 call    ;;
;;;;;;;;;;;;;;;;;;;;;;

.BX_BCD_TO_INT_TO_SI_BYTE:

    push dx ; mul dx:ax val store

    push bx
    and bx, 0x000f 
    mov si, bx 
    pop bx 

    push bx
    shr bx, 4
    and bx, 0x000f 
    mov ax, 10
    mul bx 
    add si, ax 

    pop bx
    pop dx
    ret

.PRINT_BCD_AL_CONST:
    push ax
    and al, 0x0f ; bcd first digit
    add al, 48 ; to ascii
    mov byte [fs:di + 2], al

    pop ax
    push ax
    shr al, 4 ; bcd second digit
    and al, 0x0f ; remove other
    add al, 48 ; to ascii
    mov byte [fs:di], al
    add di, 4

    pop ax
    ret

ERRORMESSAGE: db 'KernelLoader Read ERROR', 0
MESSAGE1:    db 'MINT64 OS Boot Loader Start~!!', 0
DATEMESSAGECONST: db 'Current Date: ', 0
DAYOFWEEK: db 'SUN','MON','TUE','WED', 'THU', 'FRI', 'SAT'

SECTORNUMBER:           db  0x02     
HEADNUMBER:             db  0x00     
TRACKNUMBER:            db  0x00

times 510 - ( $ - $$ )    db    0x00

db 0x55
db 0xAA