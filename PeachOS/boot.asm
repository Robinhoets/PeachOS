ORG 0
BITS 16
_start:             ; account for flash drive issues. Createa bias.
    jmp short start
    nop

times 33 db 0       ; creates a bias of 33 bytes to account for flash drive issues. This is because some flash drives may not properly read the first few bytes of the boot sector, so we add a small offset to ensure that our code is executed correctly.

start:
    jmp 0x7c0:step2 ; code segment set to 0x7c0

handle_zero:        ; someone does int 0, it will print A to the screen
    mov ah, 0eh
    mov al, 'A'
    mov bx, 0x00
    int 0x10
    iret            ; return from interrupt

step2:
    cli             ; clear interrupts - disable

    mov ax, 0x07C0  ; setting registers to 0x07C0
    mov ds, ax
    mov es, ax      

    mov ax, 0x00    ; set stack segment to 0x7c00
    mov ss, ax
    mov sp, 0x7c00    

    sti             ; enable interrupts

    ; ss signifies the stack segment. Data segment points at 0x7c00. We need first byte in ram.
    mov word [ss:0x00], handle_zero ; interupt 0 starts at 0, the first address in memory. Two bytes offset. Two bytes segment.
    mov word [ss:0x02], 0x7c0

    int 0


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