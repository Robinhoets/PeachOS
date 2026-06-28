ORG 0
BITS 16
_start:             ; account for flash drive issues. Createa bias.
    jmp short start
    nop

times 33 db 0       ; creates a bias of 33 bytes to account for flash drive issues. This is because some flash drives may not properly read the first few bytes of the boot sector, so we add a small offset to ensure that our code is executed correctly.

start:
    jmp 0x7c0:step2 ; code segment set to 0x7c0

step2:
    cli             ; clear interrupts - disable

    mov ax, 0x07C0  ; setting registers to 0x07C0
    mov ds, ax
    mov es, ax      

    mov ax, 0x00    ; set stack segment to 0x7c00
    mov ss, ax
    mov sp, 0x7c00    

    sti             ; enable interrupts

    mov ah, 2       ; read sector command (AH = 02h)
    mov al, 1       ; Only one sector to read (AL = number of sectors to read (must be nonzero)
    mov ch, 0       ; Cylinder is low eight bits (CH = low eight bits of cylinder number)
    mov cl, 2       ; Start at one for chs; as opposed to LBA. (CL = sector number 1-63 (bits 0-5))
    mov dh, 0       ; Head number is 0 (DH = head number)
                    ; dl is set for us (DL = drive number (bit 7 set for hard disk))
                    ; extra segment already points to 0x07C0
    mov bx, buffer
    int 0x13        ; Interupt 13: DISK - READ SECTOR(S) INTO MEMORY
    jc error

    mov si, buffer
    call print

    jmp $           ; everthing fine loading memory - infinite jump

error:
    mov si, error_message
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

error_message: db 'Failed to load Sector', 0

times 510-($ - $$) db 0
dw 0xAA55

buffer:         ; label set here to make sure we don't overwrite our code