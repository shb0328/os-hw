[BITS 64]

SECTION .text

extern Main ;Import

START:
    mov ax, 0x10 ;IA-32e's Data Segment Descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;stack 1MB
    mov ss, ax
    mov rsp, 0x6FFFF8
    mov rbp, 0x6FFFF8

    call Main ;C 언어 entry point 함수 호출

    jmp $

