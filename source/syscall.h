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

#define SYSCALL_PRINT_MAXLEN    2048

typedef u16 syscall_t;
#define SCALL_CONSOLE_WRITE 1
#define SCALL_CONSOLE_READ  2
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
#define SCALL_SYNC          16
#define SCALL_MMAP          17
#define SCALL_MUNMAP        18

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
volatile int isr_IRQReceiver()
{
    //Handle syscalls

    // draw_clear(c_green);
    // draw_clear(c_yellow);
    // draw_clear(c_green);

    register int r_lr asm("lr");
    void* retptr= (void*)r_lr;
    register int r_r0 asm("r0");
    int function= r_r0;
    register int r_r1 asm("r1");
    int arg0= r_r1;
    register int r_r2 asm("r2");
    int arg1= r_r2;
    register int r_r3 asm("r3");
    int arg2= r_r3;

    switch(function)
    {
        case SCALL_CONSOLE_WRITE:
            u16 len= 0;
            char* strptr= (char*)arg0;
            do
            {
                if (strptr[len])
                {
                    len++;
                    if (len > SYSCALL_PRINT_MAXLEN+1)
                    {
                        if (dbg_syscall)
                            syscall_throw("refusing to print string longer than %d chars", SYSCALL_PRINT_MAXLEN);
                        break;
                    }
                }
                else
                    break;
            }
            while (len <= SYSCALL_PRINT_MAXLEN+1);
                console_printf(strptr);
            break;
        case SCALL_OPEN:
            if (dbg_syscall)
                syscall_throw("Opening file \"%s\" with mode '%c'&r", (char*)arg0, (char)arg1);
            arg0= fs_fopen((char*)arg0, (char)arg1);
            if ((s16)arg0 < 0)
                syscall_throw("I/O error: %s&r", fs_error);
            if (dbg_syscall)
                syscall_throw("fopen() returned %d", arg0);
            return arg0;
            break;
        case SCALL_CLOSE:
            fs_fclose((fdesc_t)arg0);
            if (dbg_syscall)
                syscall_throw("file #%d is now closed", (fdesc_t)arg0);
            break;
        case SCALL_READ:
            arg0= fs_fread((fdesc_t)arg0, (u8*)arg1, arg2);
            break;
        case SCALL_WRITE:
            fs_fwrite((fdesc_t)arg0, (u8*)arg1, arg2);
            break;
        case SCALL_FSEEK:
            fs_fseek((fdesc_t)arg0, (u8)arg1);
            if (dbg_syscall)
                syscall_throw("seek to %p of file #%d", (u8)arg1, (fdesc_t)arg0);
            break;
        case SCALL_FTELL:
            arg1= fs_ftell((fdesc_t)arg0);
            break;
        case SCALL_SYNC:
            if (dbg_syscall)
                syscall_throw("Writing sector buffer to FLASH...&r", 0);
            fs_flush();
            break;
        case SCALL_CONSOLE_READ:
            break;
        case SCALL_MMAP:
            if (dbg_syscall)
                syscall_throw("MMAP: Trying to allocate %d bytes...&r", arg0);
            arg0= (int)malloc((size_t)arg0);
            return (int)arg0;
            break;
        case SCALL_MUNMAP:
            if (dbg_syscall)
                syscall_throw("MUNMAP: Trying to free address %p...&r", (void*)arg0);
            free((void*)arg0);
            break;
        case SCALL_RENAME:
        case SCALL_UNLINK:
        case SCALL_EXECVE:
        case SCALL_EXIT:
        case SCALL_KILL:
        case SCALL_STATFS:
        case SCALL_CREAT:
            syscall_throw("%d: function not implemented", function);
            break;
        default:
            //Ignore
            if (dbg_syscall)
                syscall_throw("%d: undefined syscall", function);
            break;
    }

    return 0;
}

u32* syscall_vector= (u32*)0x03007FE0;
