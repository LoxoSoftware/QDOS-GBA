//elflib.h
#ifndef ELFLIB_H
#define ELFLIB_H

#include <elf.h>
#include "flashfs.h"

typedef enum {
    EXE_INVALIDARCH = -3,
    EXE_WRONGMAGIC  = -2,
    EXE_UNSUPPORTED = -1,
    EXE_INVALID     = 0,
    EXE_RELOC       = 1,
    EXE_DYNAMIC     = 2,
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
        case ET_CORE:
        case ET_EXEC:
            return EXE_UNSUPPORTED;
        case ET_DYN:
            ttype= EXE_DYNAMIC;
            break;
        case ET_REL:
            ttype= EXE_RELOC;
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

void elf_runDynamic(Elf32_Ehdr* elf)
{
    void (*new_entry)(void);

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

    if (dbg_elfexec)
    {
        console_printf("Allocating %d bytes&n", allocsz);
        console_drawbuffer();
    }
    new_entry= malloc(allocsz);
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
                console_printf("copy 0x%h to 0x%h&n", (u32)elf+read32(&tph->p_offset), (u32)tmi);
                console_drawbuffer();
            }
            for (u32 ib=0; ib<p_filesz; ib++)
                tmi[ib]= *(u8*)((u32)elf+read32(&tph->p_offset)+ib);
            tmi+= read32(&tph->p_filesz);
        }
    }

    if (dbg_elfexec)
    {
        console_printf("run 0x%h...&n", (u32)new_entry);
        console_drawbuffer();
    }

    (*new_entry)();

    free(new_entry);
}

void elf_runReloc(Elf32_Ehdr* elf)
{
    Elf32_Shdr* sec_text_startup= elf_getSectionByInd(elf, elf_getSectionIndByName(elf, ".text.startup"));
    Elf32_Shdr* sec_text= elf_getSectionByInd(elf, elf_getSectionIndByName(elf, ".text"));
    Elf32_Shdr* sec_rel_text_startup= elf_getSectionByInd(elf, elf_getSectionIndByName(elf, ".rel.text.startup"));
    //Elf32_Shdr* sec_rel_text= elf_getSectionByInd(elf, elf_getSectionIndByName(elf, ".rel.text"));
    Elf32_Shdr* sec_symtab= elf_getSectionByInd(elf, elf_getSectionIndByName(elf, ".symtab"));

    void (*new_entry)(void);
    u32 totalsz= 0;
    u32 startupsz= 0;
    u32 textsz= 0;

    //Count total code size to allocate after
    if (!sec_text && !sec_text_startup)
    {
        console_printf("EXE: ERR! No executable code found.&n");
        console_drawbuffer();
        return;
    }
    if (sec_text_startup)
    {
        totalsz += read32(&sec_text_startup->sh_size);
        startupsz = read32(&sec_text_startup->sh_size);
    }
    else
    {
        console_printf("EXE: WARN: Init code not found, this may not work!&n");
        console_drawbuffer();
    }
    if (sec_text)
    {
        totalsz += read32(&sec_text->sh_size);
        textsz = read32(&sec_text->sh_size);
    }
    else
    {
        console_printf("EXE: WARN: Main code not found, ignoring.&n");
        console_drawbuffer();
    }

    //Allocate space for code
    if (dbg_elfexec)
    {
        console_printf("Allocating %d bytes&n", totalsz);
        console_drawbuffer();
    }
    new_entry= malloc(totalsz);
    if (!new_entry)
    {
        console_printf("EXE: ERR! Memory allocation failed&n");
        console_drawbuffer();
        return;
    }

    //Copy code
    if (sec_text_startup)
    {
        if (dbg_elfexec)
        {
            console_printf(".text.startup -> 0x%h&n", (u32)new_entry);
            console_drawbuffer();
        }

        for (int i=0; i<startupsz; i++)
            *((u8*)new_entry+i)= *(u8*)((u32)elf+read32(&sec_text_startup->sh_offset)+i);

        if (sec_text)
        {
            if (dbg_elfexec)
            {
                console_printf(".text -> 0x%h&n", (u32)new_entry+startupsz);
                console_drawbuffer();
            }

            for (int i=0; i<textsz; i++)
                *((u8*)new_entry+startupsz+i)= *(u8*)((u32)elf+read32(&sec_text->sh_offset)+i);
        }
    }
    else if (sec_text)
    {
        if (dbg_elfexec)
        {
            console_printf(".text -> 0x%h&n", (u32)new_entry);
            console_drawbuffer();
        }

        for (int i=0; i<textsz; i++)
            *((u8*)new_entry+i)= *(u8*)((u32)elf+read32(&sec_text->sh_offset)+i);
    }

    //Relocate
    if (sec_rel_text_startup)
    {
        u16 rels= read32(&sec_rel_text_startup->sh_size)/sizeof(Elf32_Rel);

        for (int ir=0; ir<rels; ir++)
        {
            Elf32_Rel* relptr= (Elf32_Rel*)((u32)elf+read32(&sec_rel_text_startup->sh_offset)+ir*sizeof(Elf32_Rel));
            u32* modptr= (u32*)((u32)new_entry+read32(&relptr->r_offset));
            Elf32_Sym* symptr= (Elf32_Sym*)((u32)elf+read32(&sec_symtab->sh_offset)+ELF32_R_SYM(read32(&relptr->r_info))*sizeof(Elf32_Sym));

            switch (ELF32_R_TYPE(relptr->r_info))
            {
                case R_ARM_CALL:
                    *modptr= 0xEB000000+(startupsz-((u32)modptr-(u32)new_entry)+read32(&symptr->st_value))/4-2;
                    break;
                case R_ARM_REL32:
                    break;
                default:
                    continue;
            }

            if (dbg_elfexec)
            {
                console_printf("rel 0x%h -> 0x%h&n", relptr, modptr);
                console_drawbuffer();
            }
        }
    }
    else
    {
        console_printf("EXE: WARN: Main relocation table not found, this may not work!");
        console_drawbuffer();
    }

    if (dbg_elfexec)
    {
        console_printf("run 0x%h...&n", (u32)new_entry);
        console_drawbuffer();
    }

    (*new_entry)();

    free(new_entry);
}

#pragma GCC pop_options

#endif
