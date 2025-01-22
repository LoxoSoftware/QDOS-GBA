//tools.h

#ifndef TOOLS_H
#define TOOLS_H

#include "console.h"
#include "keyboard.h"
#include "command.h"

typedef struct ostool_s
{
    char* name;
    char* desc;
    void (*entry)(void);
} ostool_t;

extern ostool_t     romtools[];
extern const int    romtools_sz;

// void fcmd_testfunc()
// {
//     console_printf("It friggin' works!&n");
// }

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
        console_printf("ERR: ");
        console_printf(fs_error);
        console_newline();
    }
    else
    {
        console_printf("%h&n", (u32)ret);
    }
}

// void fcmd_rm()
// {
//
// }

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

ostool_t romtools[]=
{
    // &cmd_test,
    { "cmd", "Show builtin tools", &fcmd_cmdlist },
    { "ls", "List files", &fcmd_ls },
    { "new", "New file", &fcmd_nf },
    // &cmd_rm,
    { "fmt", "Format flash", &fcmd_fdisk },
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
