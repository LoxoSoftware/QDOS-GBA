#ifndef _gba_interrupt_h_
    #include <gba_interrupt.h>
#endif

#define REG_IFBIOS			*(vu16*)(REG_BASE-0x0008)	//!< IRQ ack for IntrWait functions

typedef struct IntTable IRQ_REC;

IRQ_REC IntrTable[MAX_INTS+1];

IWRAM_CODE void IntrMain()
{
    u32 ie= REG_IE;
    u32 ieif= ie & REG_IF;
    IRQ_REC* pir;

    // (1) Acknowledge IRQ for hardware and BIOS.
    REG_IF      = ieif;
    REG_IFBIOS |= ieif;

    // (2) Find raised irq
    for(pir= IntrTable; pir->mask!=0; pir++)
        if(pir->mask & ieif)
            break;

    // (3) Just return if irq not found in list or has no isr.
    if(pir->mask == 0 || pir->handler == NULL)
        return;

    // --- If we're here have an interrupt routine ---
    // (4a) Disable IME and clear the current IRQ in IE
    u32 ime= REG_IME;
    REG_IME= 0;
    REG_IE &= ~ieif;

    // (5a) CPU back to system mode
    //> *(--sp_irq)= lr_irq;
    //> *(--sp_irq)= spsr
    //> cpsr &= ~(CPU_MODE_MASK | CPU_IRQ_OFF);
    //> cpsr |= CPU_MODE_SYS;
    //> *(--sp_sys) = lr_sys;

    pir->handler();             // (6) Run the ISR

    REG_IME= 0;             // Clear IME again (safety)

    // (5b) Back to irq mode
    //> lr_sys = *sp_sys++;
    //> cpsr &= ~(CPU_MODE_MASK | CPU_IRQ_OFF);
    //> cpsr |= CPU_MODE_IRQ | CPU_IRQ_OFF;
    //> spsr = *sp_irq++
    //> lr_irq = *sp_irq++;

    // (4b) Restore original ie and ime
    REG_IE= ie;
    REG_IME= ime;
}
