//QDOS API (WIP)

#ifndef QDOS_H
#define QDOS_H

#include <gba.h>

int _start();

#define FS_FNAME_SZ         12
#define FS_FTYPE_SZ         4

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

int (*syscall)(int function, ...)= (void*)0x03007FE0;

void print(char* str)
{ syscall(SCALL_CONSOLE_WRITE, (u32)str); }

void redraw()
{ syscall(SCALL_CONSOLE_WRITE, (u32)"&r"); }

fdesc_t fopen(char* fname, char mode)
{ return (fdesc_t)syscall(SCALL_OPEN, mode, (u32)fname); }

void fclose(fdesc_t fd)
{ syscall(SCALL_CLOSE, fd); }

u32 fread(fdesc_t fd, u8* buffer, u32 len)
{ return (u8)syscall(SCALL_READ, fd, (u32)buffer, len); }

void fwrite(fdesc_t fd, u8* buffer, u32 len)
{ syscall(SCALL_WRITE, fd, (u32)buffer, len); }

void fseek(fdesc_t fd, u32 pos)
{ syscall(SCALL_FSEEK, fd, pos); }

u32 ftell(fdesc_t fd)
{ return (u32)syscall(SCALL_FTELL, fd); }

void flushfs()
{ syscall(SCALL_SYNC); }

void _putchar(char ch)
{
    char str[2]= { ch,'\0' };
    print(str);
}

#endif
