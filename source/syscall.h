//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/syscall.h   OS systemcall handler with definitions                  //
// Copyright (C) 2024-2025  Lorenzo C. (aka LoxoSoftware)                   //
//                                                                          //
//   This program is free software: you can redistribute it and/or modify   //
//   it under the terms of the GNU General Public License as published by   //
//   the Free Software Foundation, either version 3 of the License, or      //
//   (at your option) any later version.                                    //
//                                                                          //
//   This program is distributed in the hope that it will be useful,        //
//   but WITHOUT ANY WARRANTY; without even the implied warranty of         //
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
//   GNU General Public License for more details.                           //
//                                                                          //
//   You should have received a copy of the GNU General Public License      //
//   along with this program.  If not, see https://www.gnu.org/licenses/.   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef _gba_interrupt_h_
    #include <gba_interrupt.h>
#endif

#define SYSCALL_TRIGGER     DMA0CNT=DMA_IRQ|DMA_ENABLE|DMA_IMMEDIATE;REG_DMA0CNT=0
#define SYSCALL_ARGBASE     ((u8*)(EWRAM+EWRAM_SIZE-16))
#define SYSCALL_ARGS        ((syscall_args_t*)SYSCALL_ARGBASE)

typedef u16 syscall_t;
#define SCALL_CONSOLE_PRINT 1
#define SCALL_CONSOLE_DRAW  2
#define SCALL_OPEN          3
#define SCALL_CLOSE         4
#define SCALL_READ          5
#define SCALL_WRITE         6
#define SCALL_RENAME        7
#define SCALL_UNLINK        8
#define SCALL_LSEEK         9
#define SCALL_EXECVE        10
#define SCALL_EXIT          11
#define SCALL_KILL          12
#define SCALL_STATFS        13
#define SCALL_CREAT         14

typedef struct syscall_args_s
{
    syscall_t   function;
    u16         arg0;
    u32         arg1;
    u32         arg2;
    u32         arg3;
} syscall_args_t;

bool dbg_syscall= true;

ARM_CODE void isr_IRQReceiver()
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
        case SCALL_OPEN:
        case SCALL_CLOSE:
        case SCALL_READ:
        case SCALL_WRITE:
        case SCALL_RENAME:
        case SCALL_UNLINK:
        case SCALL_LSEEK:
        case SCALL_EXECVE:
        case SCALL_EXIT:
        case SCALL_KILL:
        case SCALL_STATFS:
        case SCALL_CREAT:
            if (dbg_syscall)
            {
                console_printf("SYSCALL: %d: function not implemented&n&n", SYSCALL_ARGS->function);
                console_drawbuffer();
            }
            break;
        default:
            //Ignore
            if (dbg_syscall)
            {
                console_printf("SYSCALL: %d: undefined systemcall&n", SYSCALL_ARGS->function);
                console_drawbuffer();
            }
            break;
    }

    //memset(SYSCALL_ARGBASE, 0, 16);
}
