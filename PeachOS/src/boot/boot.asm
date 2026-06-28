ORG 0x7c00
BITS 16

CODE_SEG equ gdt_code - gdt_start   ; offset 0x8 for gdt code
DATA_SEG equ gdt_data - gdt_start   ; offset 0x10 for gdt code

_start:             ; account for flash drive issues. Createa bias.
    jmp short start
    nop

times 33 db 0       ; creates a bias of 33 bytes to account for flash drive issues. This is because some flash drives may not properly read the first few bytes of the boot sector, so we add a small offset to ensure that our code is executed correctly.

start:
    jmp 0:step2     ; code segment set to 0x7c0

step2:
    cli             ; clear interrupts - disable

    mov ax, 0x00    ; setting registers to 0x07C0
    mov ds, ax
    mov es, ax      

    mov ss, ax
    mov sp, 0x7c00    

    sti             ; enable interrupts

.load_protected:
    cli
    lgdt[gdt_descriptor]    ; load global descriptor table
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:load32     ; jumps to code_seg which goes to load32 absolute address

; create global descriptor table
; GDT
gdt_start:

gdt_null:
    dd 0x0
    dd 0x0          ; 64 bits of 0s

; offset 0x8
gdt_code:           ; CS should point to this
    dw 0xffff       ; Segment limit of first 0-15 bits
    dw 0            ; Base first 0-15 bits
    db 0            ; Base 16-32 bits
    db 0x9a         ; Access byte
    db 11001111b    ; High 4 bit flags and low 4 bit flags
    db 0            ; Base 24-31 bits

; offset 0x10
gdt_data:           ; Linked to DS, SS, ES, FS, GS
    dw 0xffff       ; Segment limit of first 0-15 bits
    dw 0            ; Base first 0-15 bits
    db 0            ; Base 16-32 bits
    db 0x92         ; Access byte
    db 11001111b    ; High 4 bit flags and low 4 bit flags
    db 0            ; Base 24-31 bits    

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start-1    ; size of descriptor
    dd gdt_start

[BITS 32]           ; all code under here is 32 bit code
load32:
    mov ax, DATA_SEG    ; set data registers
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp    ; set stack pointer to base pointer because now we can access more memory
    jmp $

times 510-($ - $$) db 0
dw 0xAA55