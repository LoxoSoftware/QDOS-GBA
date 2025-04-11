//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/binexec.h   File execution wrapper                                  //
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

#ifndef BINEXEC_H
#define BINEXEC_H

#include "flashfs.h"
#include "console.h"

void os_exec(file_t* file)
{
    void (*program)(void);

    char ttype[5]= { 0, 0, 0, 0, 0 };
    memcpy(ttype, file->type, 4);

    if (!strcmp(ttype, "ptr"))
        program= (void*)(*(u32*)(file->data));
    else
    if (!strcmp(ttype, "gba") || !strcmp(ttype, "mb"))
    {
        //Launch directly a multiboot ROM
        for (u32 i=0; i<read32(&file->size); i++)
            *(u8*)(0x02000000+i)= *(u8*)((u32)&file->data+i);
        program= (void*)0x02000000;
        (*program)();
        return;
    }
    else
    {
        //Launch an ELF file (used for OS-native programs)
        Elf32_Ehdr* exeptr= (Elf32_Ehdr*)&(file->data);
        exe_t exetype= elf_check(exeptr);

        switch (exetype)
        {
            case EXE_INVALIDARCH:
                console_printf("Invalid architecture&n");
                return;
            case EXE_WRONGMAGIC:
                console_printf("Invalid ELF format: wrong magic&n");
                return;
            case EXE_UNSUPPORTED:
                console_printf("Unsupported exec. format&n");
                return;
            case EXE_RELOC:
                elf_runReloc(exeptr);
                return;
            case EXE_STATIC:
                elf_runDynamic(exeptr);
                break;
            default:
                console_printf("Invalid exec. format&n");
                return;
        }
    }
}

#endif
