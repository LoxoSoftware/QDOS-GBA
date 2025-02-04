//syscall.h

#ifndef _gba_interrupt_h_
    #include <gba_interrupt.h>
#endif

#define SYSCALL_TRIGGER     DMA0CNT=DMA_IRQ|DMA_ENABLE|DMA_IMMEDIATE;REG_DMA0CNT=0
#define SYSCALL_ARGBASE     ((u8*)(EWRAM+EWRAM_SIZE-16))
#define SYSCALL_ARGS        ((syscall_args_t*)SYSCALL_ARGBASE)

#define SCALL_CONSOLE_PRINT 1
#define SCALL_CONSOLE_DRAW  2

typedef struct syscall_args_s
{
    u16 function;
    u16 arg0;
    u32 arg1;
    u32 arg2;
    u32 arg3;
} syscall_args_t;

bool dbg_syscall= true;

ARM_CODE void isr_IRQReceived()
{
    //Handle syscalls

    // draw_clear(c_green);
    // draw_clear(c_yellow);
    // draw_clear(c_green);

    //asm("\t movl %%ebx,%0" : "=r"(i)); //Get register value

    switch(SYSCALL_ARGS->function)
    {
        case SCALL_CONSOLE_PRINT:
            console_prints((char*)(SYSCALL_ARGS->arg1));
            break;
        case SCALL_CONSOLE_DRAW:
            console_drawbuffer();
            break;
        default:
            //Ignore
            if (dbg_syscall)
            {
                console_printf("SYSCALL: WARN: IRQ ignored&n  (FN=%d, A0=0x%h, A1=0x%h, A2=0x%h, A3=0x%h)&n", SYSCALL_ARGS->function, SYSCALL_ARGS->arg0, SYSCALL_ARGS->arg1, SYSCALL_ARGS->arg2, SYSCALL_ARGS->arg3);
                console_drawbuffer();
            }
            break;
    }

    //memset(SYSCALL_ARGBASE, 0, 16);
}
