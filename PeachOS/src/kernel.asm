[BITS 32]           ; all code under here is 32 bit code

global _start
global problem
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG    ; set data registers
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp    ; set stack pointer to base pointer because now we can access more memory

    ; Enable A20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    call kernel_main   ; call c code

    jmp $

; cause issue to see if interrupt works
problem:
    int 0

times 512-($ - $$) db 0 ; handle alignment