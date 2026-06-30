#include "idt.h"
#include "config.h"
#include "memory/memory.h"
#include "kernel.h"
#include "io/io.h"

struct idt_desc idt_descriptors[PEACHOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

/*
    - Section - externs
    Built in .asm
*/

// Function prototype for loading interrupt description table.
// Loading done in idt.asm
extern void idt_load(struct idtr_desc* ptr);
// Interrupt handler for keyboard
extern void int21h();
// There is no interrupt
extern void no_interrupt();

/*
    - End Section - externs
*/

void int21h_handler()
{
    print("Keyboard pressed!");
    // send acknowledgement to hardware port
    outb(0x20, 0x20);
}

// if there are no interrtupts
void no_interrupt_handler()
{
    outb(0x20, 0x20);
}

void idt_zero()
{
    print("Divide by zero error\n");
}

/*
    https://wiki.osdev.org/Interrupt_Descriptor_Table
*/
void idt_set(int interrupt_no, void * address)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t) address & 0x0000ffff;
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xEE;
    desc->offset_2 = (uint32_t) address >> 16;
}

void idt_init()
{
    // set our memory size to be the size of idt descriptors (512 of size idt_desc struct)
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t) idt_descriptors;

    for(int i=0; i<PEACHOS_TOTAL_INTERRUPTS; i++)
    {
        // call assembly function
        idt_set(i, no_interrupt);
    }

    idt_set(0, idt_zero);
    idt_set(0x20, int21h);      // cal handle for keyboard interrupt

    // Load the interrupt description table
    // pass in address of table
    idt_load(&idtr_descriptor);
}