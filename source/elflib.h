//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/elflib.h    ELF file parser and program execution                   //
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
#ifndef ELFLIB_H
#define ELFLIB_H

#include <elf.h>

void    __exe_weighArgs(int*, int*);
void    __exe_copyArgs(char*, char**);

typedef enum {
    EXE_INVALIDARCH = -3,
    EXE_WRONGMAGIC  = -2,
    EXE_UNSUPPORTED = -1,
    EXE_INVALID     = 0,
    EXE_STATIC      = 1,
} exe_t;

bool dbg_elfexec= false;

exe_t elf_check(Elf32_Ehdr* exeptr)
{
    if (memcmp(exeptr->e_ident, "\x7f""ELF", 4))
        return EXE_WRONGMAGIC;

    if (exeptr->e_ident[EI_CLASS] != ELFCLASS32 || exeptr->e_ident[EI_DATA] != ELFDATA2LSB ||
        read16(&exeptr->e_machine) != EM_ARM)
        return EXE_INVALIDARCH;

    exe_t ttype= EXE_INVALID;

    switch (read16(&exeptr->e_type))
    {
        case ET_EXEC:
            return EXE_STATIC;
        case ET_DYN:
        case ET_CORE:
        case ET_REL:
            ttype= EXE_UNSUPPORTED;
            break;
        default:
            return EXE_INVALID;
    }

    return ttype;
}

char* seekstrlist(char* strptr, int n)
{
    int i=0;

    while(n>0)
    {
        while (strptr[i]!='\0') i++;
        n--, i++;
    }

    return &strptr[i];
}

#pragma GCC push_options
#pragma GCC optimize ("O1")

void elf_printSections(Elf32_Ehdr* elf)
{
    u16 secs= read16(&elf->e_shnum);
    u16 secsz= read16(&elf->e_shentsize);

    if (secs <= 0)
    {
        console_printf("No sections found&n");
        return;
    }

    if (elf->e_shstrndx == SHN_UNDEF)
    {
        console_printf("No section names found&n");
        console_printf("%d sections&n", secs);
        return;
    }

    Elf32_Shdr* sections= (Elf32_Shdr*)((u32)elf+read32(&elf->e_shoff));
    Elf32_Shdr* names_sec= (Elf32_Shdr*)((u32)sections+read16(&elf->e_shstrndx)*secsz);
    char* secnames= (char*)((u32)elf+read32(&names_sec->sh_offset));

    char tname[32];
    char* tsp;
    for (int sec=0; sec<secs; sec++)
    {
        memset(tname, '\0', sizeof(tname));
        tsp= seekstrlist(secnames, sec);

        for (int is=0; is<sizeof(tname) && tsp[is] != '\0'; is++)
            tname[is]= tsp[is];

        console_newline();
        console_prints(tname);
    }

    console_printf("%d sections&n", secs);
    console_drawbuffer();
}

Elf32_Shdr* elf_getSectionByInd(Elf32_Ehdr* elf, u16 index);

char* elf_getSectionName(Elf32_Ehdr* elf, u16 index, char* outstr, u8 outstrlen)
{
    Elf32_Shdr* secptr= elf_getSectionByInd(elf, index);
    if (!secptr) return NULL;

    u16 secsz= read16(&elf->e_shentsize);
    Elf32_Shdr* sections= (Elf32_Shdr*)((u32)elf+read32(&elf->e_shoff));
    Elf32_Shdr* names_sec= (Elf32_Shdr*)((u32)sections+read16(&elf->e_shstrndx)*secsz);
    char* secnames= (char*)((u32)elf+read32(&names_sec->sh_offset));

    char* tsp= secnames+read16(&secptr->sh_name);
    memset(outstr, '\0', outstrlen);

    for (int is=0; is<outstrlen && tsp[is] != '\0'; is++)
        outstr[is]= tsp[is];

    return tsp;
}

u16 elf_getSectionIndByName(Elf32_Ehdr* elf, char* name)
{
    //Returns the index of the section in the table
    //Returns 0 on error
    u16 secs= read16(&elf->e_shnum);

    char tn[32];
    for (int sec=0; sec<secs; sec++)
    {
        elf_getSectionName(elf, sec, tn, sizeof(tn));
        if (!_strcmp(name, tn)) return sec;
    }


    return 0;
}

Elf32_Shdr* elf_getSectionByInd(Elf32_Ehdr* elf, u16 index)
{
    //if (!index) return NULL;

    u16 secs= read16(&elf->e_shnum);
    u16 secsz= read16(&elf->e_shentsize);

    if (secs <= 0)
        return NULL;
    if (elf->e_shstrndx == SHN_UNDEF)
        return NULL;
    if (index == secs)
        return NULL;

    return (Elf32_Shdr*)((u32)elf+read32(&elf->e_shoff)+secsz*index);
}

void elf_runExecutable(Elf32_Ehdr* elf)
{
    //Temporarely disable interrupts to avoid race conditions with other programs
    REG_IME= 0;

    void (*new_entry)(int,char**);

    Elf32_Phdr* pheaders= (Elf32_Phdr*)((u32)elf+read32(&elf->e_phoff));
    u16 nheaders= read16(&elf->e_phnum);

    if (!elf->e_phoff || !nheaders)
    {
        console_printf("EXE: ERR! No program header found.&n");
        console_drawbuffer();
    }

    Elf32_Phdr* ph_code= NULL;
    u32 codesz= 0, allocsz= 0;

    //Parse program headers
    for (u16 ih=0; ih<nheaders; ih++)
    {
        Elf32_Phdr* tph= (Elf32_Phdr*)((u32)pheaders+ih*sizeof(Elf32_Phdr));

        if (read32(&tph->p_type) == PT_LOAD)
        {
            if (read32(&tph->p_flags) & PF_R && read32(&tph->p_flags) & PF_X)
            {
                ph_code= tph;
            }

            codesz+= read32(&tph->p_filesz);
            allocsz+= read32(&tph->p_memsz);
        }
    }

    if (!ph_code || !codesz || !allocsz)
    {
        console_printf("EXE: ERR! No executable code found.&n");
        console_drawbuffer();
        return;
    }

    int new_argc= 0, argsz= 0;
    __exe_weighArgs(&new_argc, &argsz);
    allocsz+= argsz+new_argc*sizeof(char*);

    if (dbg_elfexec)
    {
        console_printf("Allocating %d bytes&n", allocsz);
        console_printf("            \\_ %dB : Memory image&n", codesz);
        console_printf("            \\_ %dB : All arguments&n", argsz);
        console_printf("            \\_ %dB : New argv&n", new_argc*sizeof(char*));
        console_drawbuffer();
    }
    new_entry= malloc(allocsz+16);
    if (!new_entry)
    {
        console_printf("EXE: ERR! Memory allocation failed&n");
        console_drawbuffer();
        return;
    }

    //Copy contents of program headers
    u8* tmi= (u8*)new_entry;
    for (u16 ih=0; ih<nheaders; ih++)
    {
        Elf32_Phdr* tph= (Elf32_Phdr*)((u32)pheaders+ih*sizeof(Elf32_Phdr));

        if (read32(&tph->p_type) == PT_LOAD)
        {
            u32 p_filesz= read32(&tph->p_filesz);

            if (dbg_elfexec)
            {
                console_printf("copy %p to %p&n", (u32)elf+read32(&tph->p_offset), (u32)tmi);
                console_drawbuffer();
            }
            for (u32 ib=0; ib<p_filesz; ib++)
                tmi[ib]= *(u8*)((u32)elf+read32(&tph->p_offset)+ib);
            tmi+= read32(&tph->p_filesz);
        }
    }

    char* new_argstring= (char*)tmi;
    char** new_argv= (char**)(new_argstring+argsz+1);
    new_argv= (char**)((u32)new_argv+((u32)new_argv&3)); //Align new argv to 32 bit

    __exe_copyArgs(new_argstring, new_argv);

    if (dbg_elfexec)
    {
        console_printf("new argv is at %p&n", (u32)new_argv);
        console_printf("run %p...&n", (u32)new_entry);
        console_drawbuffer();
    }

    REG_IME= 1;
    (*new_entry)(new_argc, new_argv);

    free(new_entry);
}

#pragma GCC pop_options

#endif
