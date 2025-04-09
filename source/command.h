//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/command.h   Command interpreter                                     //
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

#ifndef ARGV
#define CMD_TOKEN_SIZE  0x40
#define CMD_TOKEN_MAX   8
char cmd_argv[CMD_TOKEN_SIZE*CMD_TOKEN_MAX];
#define ARGV(n)         ((char*)&cmd_argv[n*CMD_TOKEN_SIZE])
#endif

#ifndef COMMAND_H
#define COMMAND_H

#include "tools.h"
#include "console.h"
#include "elflib.h"
#include "binexec.h"

void main_checkerr(int error)
{
	if(error != 0)
	{
		//console_colors(c_maroon, c_yellow, c_red);
		console_redrawrow(__console_irow-1, "x\xb3");
		console_printf("returned %d&n", error);
		console_drawbuffer();

	} else if (!error)
	{
		console_redrawrow(__console_irow-1, "\xdf\xb3");
		console_drawbuffer();
	}
}

void command_prompt() {

	console_printf(__com_promptstr);
	console_drawbuffer();
	__com_prompt_active= false;
}

#pragma GCC push_options
#pragma GCC optimize ("O1")
int tokenstr(char* instr, char* tokens[])
{
    int len= strlen(instr);
    int tc= 0; //Token Count
    bool openq= false; //Open quotes
    bool trimq= false; //Trim quoted token
    memset(cmd_argv, '\0', CMD_TOKEN_SIZE*CMD_TOKEN_MAX);
    int lsi= 0; //last si, to determine token size

    for (int si=0; si<len; si++)
    {
        if (instr[si] == '\"' && instr[si-1] != '\\')
        {
            if (openq)
                trimq= true;
            openq = !openq;
        }

        if (( instr[si] == ' ' && !openq )  || si==len-1)
        {
            //Make a token if not in "" or when a substring ends
            tokens[tc]= (char*)((u32)cmd_argv+CMD_TOKEN_SIZE*tc);
            memcpy(tokens[tc], instr+lsi, si-lsi+1);
            if (trimq)
            {
                //Trim quotes at the beginning and the end of the string
                memmove(tokens[tc], tokens[tc]+1, si-lsi);
                *strchr(tokens[tc],'\"')='\0';
            }
            if (tokens[tc][si-lsi]==' ')
                tokens[tc][si-lsi]='\0';
            lsi= si+1;
            tc++;
        }
    }

    return tc;
}
#pragma GCC pop_options

void runtime_patch_code(void* prg_start, u32 prg_size)
{
    for (u32 byt=(u32)prg_start; byt<(u32)prg_start+prg_size; byt++)
    {
        u32 tb_long= read32((void*)byt);

        if (tb_long >= EWRAM && tb_long < EWRAM+prg_size)
        {
            tb_long += (u32)prg_start-EWRAM;
            write32((u8*)((u32)prg_start+byt), tb_long);
            byt += sizeof(u32);
            continue;
        }

    }
}

void execute_command(char* cmd)
{
    char* cmdtok[CMD_TOKEN_MAX];
    int tokc= tokenstr(cmd, cmdtok);

    console_newline();

    if (!strcmp(cmdtok[0],"m"))
    {
        console_printf("FLASH: %d KiB free&n", (sram_size-fs_used())/1024);
        console_printf("EWRAM: %d KiB free&n", EWRAM_SIZE/1024);
    }
    else
    if (!strcmp(cmdtok[0],"c"))
        console_clear();
    else
    if (!strcmp(cmdtok[0],":"))
    {
        u8* addr1= NULL;
        u8* addr2= NULL;
        if (tokc>1)
            addr1= (u8*)strtol(cmdtok[1],NULL,16);
        else
        {
            console_printf("No address specified&n");
            goto endparse;
        }
        if (tokc>2)
            addr2= (u8*)strtol(cmdtok[2],NULL,16);

        if (!addr2)
            console_printf("%h&n", *addr1);
        else
        {
            //Print monitor
            for (u32 ix=(u32)addr1; ix<(u32)addr2; )
            {
                if (!(ix%8))
                {
                    if (!((ix/0x10)%0x10) && !((ix/0x100)%0x10) && !((ix/0x1000)%0x10))
                        console_printf("0");
                    if (!((ix/0x100)%0x10) && !((ix/0x1000)%0x10))
                        console_printf("0");
                    if (!((ix/0x1000)%0x10))
                        console_printf("0");
                    console_printf("%h: ", ix%0x10000);
                }
                if (*(u8*)ix<10)
                    console_printf("0");
                console_printf("%h ", *(u8*)ix);
                ix++;
                if (!(ix%8))
                {
                    console_printf(" ");
                    for (int iix= 0; iix<8; iix++)
                        console_printf(".");
                    console_newline();
                }
            }
        }
    }
    else
    if (!strcmp(cmdtok[0],"."))
    {
        u8* addr= NULL;
        u32 size= 1;
        if (tokc==1)
        {
            console_printf("No address specified&n");
            goto endparse;
        }
        if (tokc==2)
        {
            console_printf("No value specified&n");
            goto endparse;
        }
        if (tokc==3)
        {
            addr= (u8*)strtol(cmdtok[1],NULL,16);
            *addr= (u8)strtol(cmdtok[2],NULL,16);
            console_printf("%h&n", *addr);
        }
        if (tokc>3)
        {
            size= (u32)strtol(cmdtok[3],NULL,16);
            addr= (u8*)strtol(cmdtok[1],NULL,16);
            u8 val= (u8)strtol(cmdtok[2],NULL,16);
            memset(addr,val,size);
            console_printf("Filled %d bytes&n",size);
        }
    }
    else
    if (!strcmp(cmdtok[0],"r"))
    {
        //Run whatever it's at the multiboot entrypoint
        //NOTE: This is for experimenting only
        //TODO: Add the possibility to select an address
        asm("BL 0x02000000");
    }
    else
    if (!strcmp(cmdtok[0],"exit"))
    {
        __system_mainloop= false;
        goto endparse;
    }
    else if (tokc && strcmp(cmdtok[0],""))
    {
        void (*program)(void);
        program= (void (*)(void))romtools_funcptr(cmdtok[0]);
        if (program)
        {
            //Execute buitin tool
            (*program)();
            goto endparse;
        }
        file_t* file= fs_getfileptr(cmdtok[0]);
        if (!file) file= fs_getfileptr_trunc(cmdtok[0]);
        if (file)
        {
            os_exec(file);
            goto endparse;
        }
        if (!file)
        {
            console_printf("No such file or command&n");
            goto endparse;
        }
    }

endparse:
    //console_printf("%h&n",cmdtok[0]);
    keyboard_clear();
    cmdtok[0]=0;
}

#endif
