;  --- Bootloader ---
;   Load kernel into memory
;   Switches processor into 32-bit protected mode
;   Then execute kernel
; -------------------
ORG 0x7c00                  ; location of first instruction ((0x7c0*16)+0) = 0x7c00
BITS 16                     ; bootloader only uses 16 bits

CODE_SEG equ gdt_code - gdt_start   ; offset 0x8 for gdt code
DATA_SEG equ gdt_data - gdt_start   ; offset 0x10 for gdt code

_start:                     ; account for flash drive issues. Createa bias.
    jmp short start         ; unconditional jump - code after accounts for bias (times 33 db 0)
    nop

; creates a bias of 33 bytes to account for flash drive issues. 
; This is because some flash drives may not properly read the first few bytes 
; of the boot sector, so we add a small offset to ensure that our code is executed correctly.
; BPB (BIOS Parameter Block) https://wiki.osdev.org/FAT
; Basically the usb is FAT (File Allocation Table)
; It creates a Boot Record on the usb that contains code and data mixed together.
; In total, it is 33 bytes. We create a gap of 33 bytes for where our code starts 
; so in the case the usb uses this, bios doesn't corrupt our code.
times 33 db 0      
start:
    jmp 0:step2             ; code segment set to 0x7c0

step2:
    cli                     ; clear interrupts - disable. We don't want them to interrupt setting the registers

    mov ax, 0x00            ; setting registers to 0x7C0
    mov ds, ax              ; mov 0x7c0 into data segment register
    mov es, ax              ; mov 0x7c0 into extra segment register

    mov ss, ax              ; mov ax into stack segment
    mov sp, 0x7c00          ; stack pointer

    sti                     ; enable interrupts

.load_protected:
    cli
    lgdt[gdt_descriptor]    ; load global descriptor table
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:load32     ; jumps to code_seg which goes to load32 absolute address

; create global descriptor table.
; GDT - this contains entries telling the CPU about memory Segments.
; Code and Data.
; We use default values because kernel will use Paging.
gdt_start:

gdt_null:
    dd 0x0
    dd 0x0                  ; 64 bits of 0s

; offset 0x8
; CS: Code Segment (holds the executable code) .
gdt_code:                   ; CS should point to this
    dw 0xffff               ; Segment limit of first 0-15 bits
    dw 0                    ; Base first 0-15 bits
    db 0                    ; Base 16-32 bits
    db 0x9a                 ; Access byte
    db 11001111b            ; High 4 bit flags and low 4 bit flags
    db 0                    ; Base 24-31 bits

; offset 0x10
; DS: Data Segment (used for general data access).
; SS: Stack Segment (points to the memory where program variables and calls are stacked).
; ES: Extra Segment (an additional data segment for far pointer addressing) .
; FS & GS: Additional Extra Segments added to the x86 architecture . In modern operating 
; systems like Linux and Windows, these are used for special purposes such as Thread 
; Local Storage (TLS) .
gdt_data:                   ; Linked to DS, SS, ES, FS, GS
    dw 0xffff               ; Segment limit of first 0-15 bits
    dw 0                    ; Base first 0-15 bits
    db 0                    ; Base 16-32 bits
    db 0x92                 ; Access byte
    db 11001111b            ; High 4 bit flags and low 4 bit flags
    db 0                    ; Base 24-31 bits    

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start-1    ; size of descriptor
    dd gdt_start

; Switch kernel to run in 32-bit mode.
; No longer access bios.
; Driver reads from disk now.
[BITS 32]
load32:             
    mov eax, 1              ; starting sector to load from (0 is boot sector)
    mov ecx, 100            ; bc 100 sectors of null loaded in makefile
    mov edi, 0x0100000      ; address to load sectors into
    call ata_lba_read       ; talk with drive and load sectors into memory.
    jmp CODE_SEG:0x0100000  ; now that we've loaded lba, jump to code segment

; Purpose: Driver to talk to drive and load sectors into memory.
; 
; In computing, LBA (Logical Block Addressing) is a standardized, 
; linear method used by operating systems to locate data on storage devices like 
; hard disk drives (HDDs) and SSDs. ATA (Advanced Technology Attachment) is the 
; interface standard that dictates how these storage drives communicate with the 
; rest of the computer.
ata_lba_read:
    mov ebx, eax            ; Backup the LBA
    ; Send the highest 8 bits of the lba to hard disk controller
    shr eax, 24             ; shift eax 24 bits to right. It will then contain highest 8 bits. 32-24=8
    or eax, 0xE0            ; selects the master drive
    mov dx, 0x1F6           ; port expecting 8 bits to be written to
    out dx, al              ; out talks to bus on motherboard and controller listens
    ; Finished sending the highest 8 bits of the lba

    ; Send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending the total sectors to read

    ; Send more bits of the LBA
    mov eax, ebx            ; restore backup LBA
    mov dx, 0x1F3
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send more bits of the LBA
    mov dx, 0x1F4
    mov eax, ebx            ; restore the backup LBA
    shr eax, 8
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send upper 16 bits of the LBA
    mov dx, 0x1F5
    mov eax, ebx            ; restore the backup LBA
    shr eax, 16
    out dx, al
    ; Finished sending upper 16 bits of the LBA

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

; Read all sectors into memory
.next_sector:
    push ecx

; Checking if we need to read
.try_again:
    mov dx, 0x1F7
    in al, dx               ; Read from port 0x1F7
    test al, 8              ; Test al register to see if bit set (8 is a magic #)
    jz .try_again           ; If fails, try again - bit not set to read.

    ; We need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw                ; Read word from port. Store it in 0x0100000 (edi). Do this 256 times (512 bytes).
    pop ecx
    loop .next_sector
    ; End of reading sectors into memory
    ret

; Set boot signature.
; Bios looks for this signature to know to use this file too boot.
times 510-($ - $$) db 0     ; Fill 510 bytes of data. ex) if we use 500, it will pad 10.
dw 0xAA55                   ; Little endian (0x55AA)