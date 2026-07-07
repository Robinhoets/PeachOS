[BITS 32]

section .asm

global paging_load_directory
global enable_paging

; move pointer value (address of page directory) passed into eax
; move that into cr3
paging_load_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax
    pop ebp
    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000     ; apply bit 31 to enable paging
    mov cr0, eax
    pop ebp
    ret 