[BITS 32]           ; all code under here is 32 bit code

global _start
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
    mov esp, ebp        ; set stack pointer to base pointer because now we can access more memory

    ; Enable A20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Start - Remap the master PIC - Programmable Interrupt Controller
    mov al, 000100001b  ; Put PIC into initialization mode
    out 0x20, al        ; Tell master PIC

    mov al, 0x20        ; Interrupt 0x20 is where master ISR should start
    out 0x21, al

    ; put PIC in x86 mode
    mov al, 00000001b
    out 0x21, al
    ; End - Remap of the master PIC

    call kernel_main   ; call c code
    
    jmp $

times 512-($ - $$) db 0 ; handle alignment