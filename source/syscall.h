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

#define SYSCALL_TRIGGER         DMA0CNT=DMA_IRQ|DMA_ENABLE|DMA_IMMEDIATE;REG_DMA0CNT=0
#define SYSCALL_ARGBASE         ((u8*)(EWRAM+EWRAM_SIZE-16))
#define SYSCALL_ARGS            ((syscall_args_t*)SYSCALL_ARGBASE)

#define SYSCALL_PRINT_MAXLEN    2048

typedef u16 syscall_t;
#define SCALL_CONSOLE_PRINT 1
#define SCALL_CONSOLE_DRAW  2
#define SCALL_OPEN          3
#define SCALL_CLOSE         4
#define SCALL_READ          5
#define SCALL_WRITE         6
#define SCALL_RENAME        7
#define SCALL_UNLINK        8
#define SCALL_FSEEK         9
#define SCALL_EXECVE        10
#define SCALL_EXIT          11
#define SCALL_KILL          12
#define SCALL_STATFS        13
#define SCALL_CREAT         14
#define SCALL_FTELL         15
#define SCALL_FSFLUSH       16

typedef struct syscall_args_s
{
    syscall_t   function;
    u16         arg0;
    u32         arg1;
    u32         arg2;
    u32         arg3;
} syscall_args_t;

bool dbg_syscall= true;

#define syscall_throw(msg, args...)    console_printf("%p: "msg"&n",retptr,args)

ARM_CODE
void isr_IRQReceiver()
{
    //Handle syscalls

    // draw_clear(c_green);
    // draw_clear(c_yellow);
    // draw_clear(c_green);

    register u32 retptr asm("lr");

    switch(SYSCALL_ARGS->function)
    {
        case SCALL_CONSOLE_PRINT:
            u16 len= 0;
            char* strptr= (char*)(SYSCALL_ARGS->arg1);
            do
            {
                if (strptr[len])
                {
                    len++;
                    if (len > SYSCALL_PRINT_MAXLEN+1)
                    {
                        if (dbg_syscall)
                            syscall_throw("refusing to print string longer than %d chars", SYSCALL_PRINT_MAXLEN);
                        goto end_syscall_parse;
                    }
                }
                else
                    break;
            }
            while (len <= SYSCALL_PRINT_MAXLEN+1);
            console_prints(strptr);
            break;
        case SCALL_CONSOLE_DRAW:
            console_drawbuffer();
            break;
        case SCALL_OPEN:
            SYSCALL_ARGS->arg0= fs_fopen((char*)(SYSCALL_ARGS->arg1), (char)(SYSCALL_ARGS->arg0));
            if ((s16)(SYSCALL_ARGS->arg0) < 0)
                syscall_throw("I/O error: %s", fs_error);
            if (dbg_syscall)
                syscall_throw("fopen() returned %d", SYSCALL_ARGS->arg0);
            break;
        case SCALL_CLOSE:
            fs_fclose((fdesc_t)(SYSCALL_ARGS->arg0));
            if (dbg_syscall)
                syscall_throw("file #%d is now closed", (fdesc_t)(SYSCALL_ARGS->arg0));
            break;
        case SCALL_READ:
            SYSCALL_ARGS->arg0= fs_fread((fdesc_t)(SYSCALL_ARGS->arg0));
            break;
        case SCALL_WRITE:
            fs_fwrite((fdesc_t)(SYSCALL_ARGS->arg0), (u8)(SYSCALL_ARGS->arg1));
            break;
        case SCALL_FSEEK:
            fs_fseek((fdesc_t)(SYSCALL_ARGS->arg0), (u8)(SYSCALL_ARGS->arg1));
            if (dbg_syscall)
                syscall_throw("seek to %p of file #%d", (u8)(SYSCALL_ARGS->arg1), (fdesc_t)(SYSCALL_ARGS->arg0));
            break;
        case SCALL_FTELL:
            SYSCALL_ARGS->arg1= fs_ftell((fdesc_t)(SYSCALL_ARGS->arg0));
            break;
        case SCALL_FSFLUSH:
            if (dbg_syscall)
                syscall_throw("Writing sector buffer to FLASH...", 0);
            fs_flush();
            break;
        case SCALL_RENAME:
        case SCALL_UNLINK:
        case SCALL_EXECVE:
        case SCALL_EXIT:
        case SCALL_KILL:
        case SCALL_STATFS:
        case SCALL_CREAT:
            syscall_throw("%d: function not implemented", SYSCALL_ARGS->function);
            break;
        default:
            //Ignore
            if (dbg_syscall)
                syscall_throw("%d: undefined syscall", SYSCALL_ARGS->function);
            break;
    }

end_syscall_parse:
    //memset(SYSCALL_ARGBASE, 0, 16);
}
