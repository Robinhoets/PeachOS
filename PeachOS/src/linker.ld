/*  
    When we link our object files, make sure origin starts at 1 Mb.
*/
ENTRY(_start)
OUTPUT_FORMAT(binary)
/* Load kernel into 1 Megabyte of memory */
SECTIONS
{
    . = 1M;
    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)
    }
}