//binexec.h

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
            case EXE_DYNAMIC:
                elf_runDynamic(exeptr);
                break;
            default:
                console_printf("Invalid exec. format&n");
                return;
        }
    }
}

#endif
