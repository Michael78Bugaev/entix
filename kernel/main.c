#include <gdt.h>
#include <vga.h>
#include <paging.h>
#include <tss.h>
#include <idt.h>
#include <entix.h>
#include <pic.h>
#include <string.h>
#include <cpu.h>
#include <heap.h>
#include <debug.h>
#include <ps2.h>
#include <timer.h> // Добавлен для функций таймера
#include <pci.h>
#include <ata.h>

void kernel_main(uint32_t magic, uint32_t addr)
{
    __asm__("cli");
    gdt_init();
    tss_init();
    kprintf("\n<(0F)>%s %s Operating System\n\n", KERNEL_FNAME, KERNEL_VERSION);
    idt_init();
    kdbg(KINFO, "kernel_main: base success\n");
    kdbg(KINFO, "pic_remap: remapping 0x20, 0x28\n");
    pic_remap(0x20, 0x28);

    pci_init();

    kdbg(KINFO, "pic_set_mask: masking all IRQs by default\n");
    for (uint8_t irq = 0; irq < 16; ++irq) {
        pic_set_mask(irq);
    }
    kdbg(KINFO, "pic_clear_mask: enabling IRQ0 (timer) and IRQ1 (keyboard)\n");
    pic_clear_mask(0); 
    pic_clear_mask(1); 
    
    init_timer(); 
    idt_register_handler(0x20, timer_isr_wrapper); 

    __asm__("sti");

    paging_init();
    ata_init();

    kprintf("%s %s simple shell\n\n", KERNEL_NAME, KERNEL_VERSION);
    set_cursor(get_cursor());
    while (1) {
        kprintf("entix> ");
        char *buf = kgets();
        if (strcmp(buf, "exit") == 0) {
            break;
        }
        else if (strcmp(buf, "help") == 0) {
            kprint("help command\n");
        }
        else {
            kprintf("You typed: %s\n", buf);
        }
    }
    kdbg(KWARN, "kernel end. You may now shutdown");
    for (;;);
}
