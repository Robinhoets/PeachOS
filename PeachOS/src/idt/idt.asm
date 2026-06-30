section .asm

extern int21h_handler
extern no_interrupt_handler

; load interrupt descriptor table
global idt_load
global int21h
global no_interrupt

; ------------ testing ----------------
global enable_interrupts
global disable_interrupts

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret
; ----------------------------------------


idt_load:
    push ebp            ; push base pointer
    mov ebp, esp        ; move stack pointer to begining


    mov ebx, [ebp+8]    ; point to first argument of this function
    lidt [ebx]
    pop ebp
    ret

; Keyboard Interrupt
; Cannot point things to functions because interrupt return
int21h:
    cli                 ; clear interrupts
    pushad              ; push all general purpose registers
    call int21h_handler  ; 
    popad               ; restore all general purpose registers
    sti                 ; enable interrupts
    iret                ; interrupt return

; If there are no interrupts
no_interrupt:
    cli                 
    pushad              
    call no_interrupt_handler
    popad               
    sti                 
    iret                