//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/tools.h     OS builtin commands                                     //
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

#ifndef TOOLS_H
#define TOOLS_H

#include "console.h"
#include "keyboard.h"
#include "command.h"
#include "flashfs.h"

void main_checkerr(int);

typedef struct ostool_s
{
    char* name;
    char* desc;
    void (*entry)(void);
} ostool_t;

extern ostool_t     romtools[];
extern const int    romtools_sz;

void fcmd_cmdlist()
{
    for (int i=0; i < romtools_sz; i++)
    {
        console_printf(romtools[i].name);
        console_printf(" : ");
        console_printf(romtools[i].desc);
        console_newline();
    }
}

void fcmd_ls()
{
    for (int i=0; i<MAXALLOCS; i++)
    {
        u16 sec= read16(&fs_root->secmap[i]);
        file_t* fptr= (file_t*)((u32)fs_root+(i+1)*FS_SECTOR_SZ);

        if (sec == 0xFFFF) continue;
        if (!(sec & 0x8000)) continue; //If it's a file

        char tname[13]= {0,0,0,0,0,0,0,0,0,0,0,0,0};
        memcpy(tname, fptr->name, 12);
        char ttype[5]= {0,0,0,0,0};
        memcpy(ttype, fptr->type, 4);

        console_printf(tname);
        if (ttype[0])
        {
            console_printf(".");
            console_printf(ttype);
        }
        console_newline();
    }
}

void fcmd_nf()
{
    if (!ARGV(1)[0])
    {
        console_printf("No filename specified&n");
        return;
    }
    file_t* ret;
    if (!ARGV(2)[0])
        ret= fs_newfile(ARGV(1), 0);
    else
        ret= fs_newfile(ARGV(1), strtol(ARGV(2),NULL,10));
    if (!ret)
    {
        console_printf("NF ERR: ");
        console_printf(fs_error);
        console_newline();
    }
}

void fcmd_rm()
{
    if (!ARGV(1)[0])
    {
        console_printf("No filename specified&n");
        return;
    }
    if (!fs_getfileptr(ARGV(1)))
    {
        console_printf("No such file to remove&n");
        return;
    }
    if (!fs_rmfile(ARGV(1)))
    {
        console_printf("RM ERR: ");
        console_printf(fs_error);
        console_newline();
        console_printf("WARNING: Changes were not saved to flash. Reboot the GBA now to avoid corruption&n");
    }
}

void fcmd_fdisk__kint1()
{
    if (strcmp(kbstring, "y") && strcmp(kbstring, "Y"))
        console_printf("&nCanceled.&n");
    else
    {
        console_printf("&n[...] Flash format&n");
        console_drawbuffer();
        fs_format();
        main_checkerr(!fs_check());
        console_drawbuffer();
    }
}

void fcmd_fdisk()
{
    console_printf("This will wipe flash clean, are you sure you want to continue?&n(y/N)");
    console_drawbuffer();
    keyboard_clear();
    go_console_keyboard(fcmd_fdisk__kint1);
}

void fcmd_fread()
{
    if (!ARGV(1)[0])
    {
        console_printf("No filename specified&n");
        return;
    }

    fdesc_t fd= fs_fopen(ARGV(1), 'r');

    if (fd<0)
    {
        console_printf("ERR: %s", fs_error);
        console_newline();
        console_drawbuffer();
        return;
    }

    char chstr[2]= { '\0', '\0' };

    if (!fs_ftell(fd))
    {
        fs_fclose(fd);
        return;
    }

    do
    {
        chstr[0]= fs_fread(fd);
        console_printf(chstr);
    }
    while (fs_ftell(fd));

    fs_fclose(fd);
}

void fcmd_fwrite()
{
    if (!ARGV(1)[0])
    {
        console_printf("No filename specified&n");
        return;
    }
    if (!ARGV(2)[0])
    {
        console_printf("No data&n");
        return;
    }

    fdesc_t fd= fs_fopen(ARGV(1), 'p');

    if (fd<0)
    {
        console_printf("ERR: %s", fs_error);
        console_newline();
        console_drawbuffer();
        return;
    }

    u32 strsz= strlen(ARGV(2));

    for (u32 i=0; i<strsz && fs_ftell(fd)>0; i++)
        fs_fwrite(fd, ARGV(2)[i]);
    fs_flush();

    fs_fclose(fd);
}

void fcmd_ansitest()
{
    console_printf("&[34m&[49m          _____          &n\
         /\\    \\&n\
        /::\\____\\&n\
       /:::/    /&n\
      /:::/    /&n\
     /:::/    /&n\
    /:::/____/&n\
   /::::\\    \\&n\
  /::::::\\    \\   _____&n\
 /:::/\\:::\\    \\ /\\    \\&n\
/:::/  \\:::\\    /::\\____\\&n\
\\::/    \\:::\\  /:::/    /&n\
 \\/____/ \\:::\\/:::/    /&n\
          \\::::::/    /&n\
           \\::::/    /&n\
           /:::/    /&n\
          /:::/    /&n\
         /:::/    /&n\
        /:::/    /&n\
        \\::/    /&n\
         \\/____/&[H");
    console_printf("&[s&[H&[16B&[22C&[40m  &[41m  &[42m  &[43m  &[44m  &[45m  &[46m  &[47m  &[0m&[u");
    console_printf("&[s&[H&[17B&[22C&[100m  &[101m  &[102m  &[103m  &[104m  &[105m  &[106m  &[107m  &[0m&[u");
    console_printf("&[s&[H&[19B&[44m&[K&[96m(c) 2025 Loxo Software&[0m&[u");
    console_printf("&[s&[H&[41m&[K&[93mANSI test&[0m&[u&n");
    console_drawbuffer();
}

ostool_t romtools[]=
{
    { "cmd", "Show builtin tools", &fcmd_cmdlist },
    { "ls", "List files", &fcmd_ls },
    { "new", "New file", &fcmd_nf },
    { "rm", "Delete file", &fcmd_rm },
    { "fw", "Write data to file", &fcmd_fwrite },
    { "fr", "Read data from file", &fcmd_fread },
    { "fmt", "Format flash", &fcmd_fdisk },
    { "ansi", "Test ANSI capabilities", &fcmd_ansitest },
};
const int romtools_sz= sizeof(romtools)/sizeof(ostool_t);

u32 romtools_funcptr(char* fname)
{
    for (int f=0; f<romtools_sz; f++)
    {
        ostool_t* fptr= &romtools[f];

        if (!_strcmp(fptr->name, fname))
            return (u32)fptr->entry;
    }

    return 0;
}

#endif
