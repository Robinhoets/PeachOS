ORG 0
BITS 16

jmp 0x7c0:start     ; code segment set to 0x7c0

start:
    cli             ; clear interrupts - disable

    mov ax, 0x07C0  ; setting registers to 0x07C0
    mov ds, ax
    mov es, ax      

    mov ax, 0x00    ; set stack segment to 0x7c00
    mov ss, ax
    mov sp, 0x7c00    

    sti             ; enable interrupts 

    mov si, message ; load message in si
    call print
    jmp $

print:
    mov bx, 0

.loop:
    lodsb
    cmp al, 0
    je .done
    call print_char
    jmp .loop

.done:
    ret

print_char:
    mov ah, 0eh
    int 0x10
    ret

message: db 'Hello World!', 0

times 510-($ - $$) db 0
dw 0xAA55