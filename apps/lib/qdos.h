//QDOS API (WIP)

#ifndef QDOS_H
#define QDOS_H

#include <gba.h>

int main();
int _start()
{
    return main();
}

#define FS_FNAME_SZ     12
#define FS_FTYPE_SZ     4

#define SYSCALL_ARGBASE     ((u8*)(EWRAM_END-16))
#define SYSCALL_ARGS        ((syscall_args_t*)SYSCALL_ARGBASE)

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
    u16 function;
    u16 arg0;
    u32 arg1;
    u32 arg2;
    u32 arg3;
} syscall_args_t;

typedef struct
{
    char type[FS_FTYPE_SZ];
    char name[FS_FNAME_SZ];
    u32  size;
    u32  attr;
    u8*  data;
} file_t;

typedef struct
{
    file_t* file;
    u32     cursor;
    char    mode;
//Mode legend:
//  'r' File open for reading only(+ create)
//  'w' File open for reading and writing (+ create)
//  'p' File open for writing only (+ create)
//  'R' File open for reading only (fail if file doesn't exist)
//  'W' File open for reading and writing (fail if file doesn't exist)
//  'P' File open for writing only (fail if file doesn't exist)
//  Set MSB to use append mode
// Anything else is invalid, file is not open
} fhandle_t;

typedef s16 fdesc_t;                //File descriptor

#define syscall(fun) \
    SYSCALL_ARGS->function= fun; \
    REG_DMA0CNT=DMA_IRQ|DMA_ENABLE|DMA_IMMEDIATE; \
    asm volatile ("NOP"); asm volatile ("NOP");

void print(char* str)
{
    SYSCALL_ARGS->arg1= (u32)str;
    syscall(SCALL_CONSOLE_PRINT);
}

void redraw() { syscall(SCALL_CONSOLE_DRAW); }

fdesc_t fopen(char* fname, char mode)
{
    SYSCALL_ARGS->arg1= (u32)fname;
    SYSCALL_ARGS->arg0= mode;
    syscall(SCALL_OPEN);
    return (fdesc_t)SYSCALL_ARGS->arg0;
}

void fclose(fdesc_t fd)
{
    SYSCALL_ARGS->arg0= fd;
    syscall(SCALL_CLOSE);
}

u8 fread(fdesc_t fd)
{
    SYSCALL_ARGS->arg0= fd;
    syscall(SCALL_READ);
    return (u8)SYSCALL_ARGS->arg0;
}

void fwrite(fdesc_t fd, u8 ch)
{
    SYSCALL_ARGS->arg0= fd;
    SYSCALL_ARGS->arg1= ch;
    syscall(SCALL_WRITE);
}

void fseek(fdesc_t fd, u32 pos)
{
    SYSCALL_ARGS->arg0= fd;
    SYSCALL_ARGS->arg1= pos;
    syscall(SCALL_FSEEK);
}

u32 ftell(fdesc_t fd)
{
    SYSCALL_ARGS->arg0= fd;
    syscall(SCALL_FTELL);
    return SYSCALL_ARGS->arg1;
}

void flushfs() { syscall(SCALL_FSFLUSH); }

void _putchar(char ch)
{
    char str[2]= { ch,'\0' };
    print(str);
}

#endif
