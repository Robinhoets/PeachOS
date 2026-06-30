section .asm

; load interrupt descriptor table
global idt_load
idt_load:
    push ebp            ; push base pointer
    mov ebp, esp        ; move stack pointer to begining


    mov ebx, [ebp+8]    ; point to first argument of this function
    lidt [ebx]
    pop ebp
    ret