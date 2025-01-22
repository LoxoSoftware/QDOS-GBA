//#define __FS_DEBUG

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//File header size: 24 bytes (version 1.0)
// 16 bytes for filename
// 1 null byte for filename termination
// 1 null byte for fsize padding
// 3 bytes for fsize (up to 16MB but limited to 256k)
// 2 bytes of free space
// 1 byte for standard os attributes
//  (0,0,is folder,is system file,is compressed,is writeable,is readable,is executable)
// nnnnnnnn
// nnnnnnnn
// 00sssaff

//File header: 24 bytes (version 2.1)
// 4 bytes for file size (up to 32MB, includes header size)
// 4 bytes for next segment (pointer) on disk or remaining bytes
// 4 bytes for file type ("GBA\0"/"DIR\0"/...) (char[4])
// 12 bytes for file name (char[12])
// sssspppp
// ttttnnnn
// nnnnnnnn

//Generic memory segment: 12 bytes (file header v2.1)
// 4 bytes for allocated size (up to 128KB)
// 4 bytes for next segment pointer
// 4 bytes for type (always set to "!SEG" (char[4]))
// sssspppp
// !SEG....

// FILESYSTEM TUNING //
#define MAXFILES        255
#define FS_INTERLEAVE   4   //Spacing between files, must be even
#define FS_ALIGN        0   //0= no align; 1= YES align
#define FS_HARDDEL      0   //0= only unlink from filetable; 1= wipe file contents and unlink
#define FS_AUTOSAVE     1
///////////////////////

//BUG: Deleting the first file does not count as free space to the allocation algorithm

typedef struct seg_s
{
    u32  size;
    u32  next;
    char type[4];
    u8 data[];
} seg_t;

typedef struct file_s
{
    u32  size;
    u32  next;
    char type[4];
    char name[12];
    u8 data[];
} file_t;

//Do not touch
#define FHEADER_SIZE        24
#define FILETABLE           ((file_t**)(SRAM+2))
#define FILETABLE_SIZE      (MAXFILES*sizeof(file_t*))

#define SRAM_INITBYTE       ((u8*)SRAM)[0]
#define SRAM_FAT_SIZE       ((u8*)SRAM)[1]

#define SRAM_MAX            0x0001FFFF  //128k

#define IS_FILE(ptr)        (memcmp(ptr->type,"!SEG",sizeof(ptr->type)))

u32 sram_size= SRAM_MAX;

#define EWRAM_DOMAIN        (void*)(EWRAM+131072),131072
    //NOTE: 128k of WRAM is reserved for multiboot code
#define SRAM_DOMAIN         (void*)SRAM,sram_size
#define FILESYSTEM_DOMAIN   (void*)((void*)FILETABLE+FILETABLE_SIZE),sram_size-FILETABLE_SIZE-2

#define MAXALLOCS   MAXFILES
seg_t* __fs_allocs[MAXALLOCS];

char* fs_error;

//FLASH driving

u8 fl_prevbank = 0;
u16 fl_prevaddr = 0;
u8 fl_lastsector = 0;

u8 fl_secbuf[4096];

//NOTE: An offset of 12 is the size of a sector (4096)
#define GETSEC(addr)    0//(((u32)addr-SRAM)>>12)
#define TOSECIND(addr)  (((u32)addr-SRAM)&0x0FFF)

inline void mdelay(int delay)
{
    for (int i=0; i<delay; i++)
        asm volatile ("NOP");
}

inline void fl_wspoke(u16 offset, u8 val)
{
    mdelay(8);
    *(u8*)(SRAM+offset)= val;
}

IWRAM_CODE u16 fl_getid()
{
    u16 dat= 0x0000;
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0x90);
    dat= (((u8*)SRAM)[1]<<8)+((u8*)SRAM)[0];
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0xF0);
    return dat;
}

IWRAM_CODE void fl_selbank(u8 bank)
{
    //Select the bank in the flash
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0xB0);
    fl_wspoke(0x0000, bank);
    fl_prevbank = bank;
}

IWRAM_CODE void fl_erase()
{
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0x80);
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0x10);
    VBlankIntrWait();
    VBlankIntrWait();
}

IWRAM_CODE void fl_erase4k(u8 sector)
{
    //Backup the sector before erasing it
    for (int i=0; i<4096; i++)
        fl_secbuf[i]= ((u8*)SRAM)[sector*0x1000+i];
    //Now erase it
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0x80);
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(sector*0x1000, 0x30);
    mdelay(100);
    fl_lastsector= sector;
    //NOTE: Suggest to make modifications to fl_secbuf before restoring it
    //      instead of directly manipulating the flash
}

IWRAM_CODE void fl_purge4k(u8 sector)
{
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0x80);
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(sector*0x1000, 0x30);
    mdelay(100);
}

IWRAM_CODE void fl_write8(u8* ptr, u8 value)
{
    u32 ofs= (u32)ptr;
    if (ofs < SRAM && ofs > SRAM+SRAM_MAX)
        return; //Pointer out of bounds

    ofs &= SRAM_MAX;
    if (ofs>>16 != fl_prevbank)
        fl_selbank(ofs>>16);
    ofs &= 0x0000FFFF; //Truncate to 16 bit

    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0xA0);
    *(u8*)(SRAM+ofs)= value;
    fl_prevaddr= ofs;

    while(*(u8*)(SRAM+ofs) != value) ;
}

IWRAM_CODE void fl_write32(u8* ptr, u32 value)
{
    fl_write8(ptr, (value)&0xFF);
    fl_write8(ptr+1, (value>>8)&0xFF);
    fl_write8(ptr+2, (value>>16)&0xFF);
    fl_write8(ptr+3, (value>>24)&0xFF);
}

IWRAM_CODE void fl_restore4k()
{
    for (int i=0; i<4096; i++)
        fl_write8((u8*)(SRAM+fl_lastsector*0x1000+i), fl_secbuf[i]);
}

IWRAM_CODE void fl_chsec(void* ptr)
{
    //Check if we need to change the sector to write to
    //NOTE: Use this if the pointer points to SRAM
    if (GETSEC(ptr) == fl_lastsector)
        return;

    fl_purge4k(fl_lastsector);
    fl_restore4k();
    fl_erase4k(GETSEC(ptr));
}

IWRAM_CODE void fl_chsec_buf(void* ptr)
{
    //Check if we need to change the sector to write to
    //NOTE: Use this if the pointer points to fl_secbuf
    if ((u32)ptr-(u32)&fl_secbuf >= 0 && (u32)ptr-(u32)&fl_secbuf < 4096)
        return;

    fl_purge4k(fl_lastsector);
    fl_restore4k();
    if ((u32)ptr-(u32)&fl_secbuf < 0)
        fl_erase4k(fl_lastsector-1);
    if ((u32)ptr-(u32)&fl_secbuf >= 4096)
        fl_erase4k(fl_lastsector+1);
}

//

int _strcmp(char* str1, char* str2)
{
    if (!str1 || !str2)
        return -1;
    for (int i=0; ; i++)
    {
        if (str1[i] != str2[i])
            return i+1;
        if (!str1[i] || !str2[i])
            return 0;
    }
}

u32 read32(void* ptr)
{
    u32 result= 0;
    result+= *(u8*)(ptr+3) << 24;
    result+= *(u8*)(ptr+2) << 16;
    result+= *(u8*)(ptr+1) << 8;
    result+= *(u8*)(ptr+0);
    return result;
}

char __fs_in_domain(void* addr, void* base_addr, u32 memsize);
void write32(void* ptr, u32 value)
{
    if (__fs_in_domain(ptr, fl_secbuf, 4096))
    {
        //If writing to secbuf, check if it's
        //  writing outside of it and change sector
        //fl_erase4k(GETSEC(ptr));
        fl_chsec_buf(ptr+0); *(u8*)(ptr+0)= value;
        fl_chsec_buf(ptr+1); *(u8*)(ptr+1)= value >> 8;
        fl_chsec_buf(ptr+2); *(u8*)(ptr+2)= value >> 16;
        fl_chsec_buf(ptr+3); *(u8*)(ptr+3)= value >> 24;
        //fl_restore4k();
    }
    else
    {
        *(u8*)(ptr+0) = value;
        *(u8*)(ptr+1) = value >> 8;
        *(u8*)(ptr+2) = value >> 16;
        *(u8*)(ptr+3) = value >> 24;
    }
}

u16 read16(void* ptr)
{
    u16 result= 0;
    result+= *(u8*)(ptr+1) << 8;
    result+= *(u8*)(ptr+0);
    return result;
}

void write16(void* ptr, u16 value)
{
    if (__fs_in_domain(ptr, fl_secbuf, 4096))
    {
        //If writing to secbuf, check if it's
        //  writing outside of it and change sector
        fl_chsec_buf(ptr+0);
        fl_secbuf[TOSECIND(ptr+0)]= value;
        fl_chsec_buf(ptr+1);
        fl_secbuf[TOSECIND(ptr+1)]= value >> 8;
    }
    else
    {
        *(u8*)(ptr+0) = value;
        *(u8*)(ptr+1) = value >> 8;
    }
}

void __fs_debug(char* msg)
{
    //Hold R to make the massage pop up
    if (!keypad_check(key_R))
        return;
    draw_set_color(c_white);
    draw_rectangle(8,18,150,70,0);
    draw_set_color(c_black);
    draw_text(10,20,msg);
    sleep(30);
    while (keypad_check(key_start))
        screen_wait_vsync();
}

void __fs_message(char* msg)
{
    draw_set_color(c_yellow);
    draw_rectangle(8,18,232,70,0);
    draw_set_color(c_black);
    draw_text(10,20,msg);
    sleep(15);
    while (!(keypad_check(key_start)&&pressed))
        screen_wait_vsync();
    draw_set_color(draw_getpixel(0,0));
    draw_rectangle(8,18,232,70,0);
    sleep(15);
}

int __fs_first_slot()
{
    for (int i=0; i<MAXALLOCS; i++)
        if (!__fs_allocs[i])
        {
            return i;
        }
    return -1;
}

int __fs_valid_name(char* name)
{
    return 1;
}

char __fs_in_domain(void* addr, void* base_addr, u32 memsize)
{
    return addr >= base_addr && addr <= base_addr+memsize;
}

seg_t* fs_alloc(void* base_addr, u32 memsize, u32 blocksize)
{
    //Also creates a file where memory is allocated
    blocksize+=FHEADER_SIZE;

    //Check if there is enough memory & blocks
    u32 used_mem= 0;
    int used_blocks= 0;
    int first_block_free= -1;
    s32 max_free= -1;
    seg_t* max_free_ptr= NULL;
    for (int a=0; a<MAXALLOCS; a++)
    {
        //Find the largest free memory block
        //Check all the free space between allocations
        if (__fs_allocs[a])
        {
            seg_t* here= __fs_allocs[a]; //Hopefully reduces memory reads
            if (!__fs_in_domain(here, base_addr, memsize))
                continue;
            u32 mysize= read32(&here->size)+FS_INTERLEAVE;
            seg_t* here_end= (seg_t*)((u32)here+mysize+((u32)here%2));
            used_blocks++;
            used_mem+= mysize;
            s32 t_max_free= -1;
            seg_t* t_max_free_ptr= NULL;
#ifdef __FS_DEBUG
            int closest_block= -1;
#endif
            s32 t_min_distance= memsize;
            //file_t* closest_block_ptr= NULL;
            for (int aa=0; aa<MAXALLOCS; aa++)
            {
                if (!__fs_allocs[aa]) continue;
                if ((s32)__fs_allocs[aa]-(s32)here_end>t_max_free)
                {
                    t_max_free= (u32)__fs_allocs[aa]-(u32)here_end;
                    t_max_free_ptr= here_end;
                }
                if ((s32)__fs_allocs[aa]-(s32)here_end>0 && (s32)__fs_allocs[aa]-(s32)here_end<t_min_distance)
                {
                    t_min_distance= (u32)__fs_allocs[aa]-(u32)here_end;
                    //closest_block_ptr= __fs_allocs[aa];
#ifdef __FS_DEBUG
                    closest_block= aa;
#endif
                }
            }
            if (t_min_distance<=blocksize)
                continue;
            if (t_max_free>max_free)
            {
                max_free= t_max_free;
                max_free_ptr= t_max_free_ptr;
            } else
            if (t_max_free==-1)
            {
                //Not enough blocks for comparison (<2)
                if (memsize-((u32)here_end-(u32)base_addr)<=max_free && max_free>0)
                    continue;
                max_free= memsize-((u32)here_end-(u32)base_addr);
                max_free_ptr= here_end;
            }
#ifdef __FS_DEBUG
                char deb[128];
                sprintf(deb,"Max space between %d-%d:&nmax space:%ld&ntill the end:%ld&nrequired:%ld&nmin dist.:%ld",a,closest_block,max_free,t_max_free,blocksize,t_min_distance);
                __fs_debug(deb);
#endif
        }
        else if (first_block_free==-1)
            first_block_free= a;
    }
    u32 free_mem= memsize-used_mem;
    int free_blocks= MAXALLOCS-used_blocks;
    if (!used_blocks)
    {
        max_free= memsize;
        max_free_ptr= base_addr;
    }
    if (free_blocks<=0 || first_block_free<0)
    {
        fs_error= "Max allocation blocks limit reached";
        return NULL;
    }
    if (((u32)base_addr+blocksize)-(u32)max_free_ptr >= free_mem || free_mem<blocksize)
    {
        fs_error= "Not enough free space available in memory domain";
        return NULL;
    }
    if (!max_free_ptr)
    {
        fs_error= "Nullptr while allocating space";
        return NULL;
    }
    if (max_free < blocksize)
    {
        fs_error= "Disk too fragmented";
        return NULL;
    }
    seg_t* new_fptr= (seg_t*)((u32)max_free_ptr+FS_INTERLEAVE+1+FS_ALIGN*((u32)max_free_ptr%2));
    __fs_allocs[first_block_free]= new_fptr;

    if (__fs_in_domain(new_fptr, SRAM_DOMAIN))
    {
        //Use flash writing functions
        // for (int i=0; i<blocksize; i++)
        // {
        //     fl_chsec(new_fptr+i);
        //     fl_secbuf[TOSECIND(new_fptr+i)]= 0x00;
        // }
        fl_erase4k(GETSEC(new_fptr));
        fl_chsec(&new_fptr->size);
        write32(&fl_secbuf[TOSECIND(&new_fptr->size)], blocksize);
        fl_chsec(&new_fptr->next);
        write32(&fl_secbuf[TOSECIND(&new_fptr->next)], 0x00000000);
        fl_restore4k();
    }
    else
    {
        memset(new_fptr, 0, blocksize);
        write32(&new_fptr->size, blocksize);
        write32(&new_fptr->next, 0); //TODO: Implement segment scatter in the future
        memcpy(new_fptr->type,"!SEG",sizeof(new_fptr->type));
    }
#ifdef __FS_DEBUG
    char deb[256];
    sprintf(deb,"FS_ALLOC...&ndest:0x%lx&noffset:%ld&ntblindex:%d&ndatasz:%ld",(u32)new_fptr,used_mem,first_block_free,(u32)blocksize);
    __fs_debug(deb);
#endif
    return new_fptr;
}

void fs_dealloc(seg_t* ptr)
{
    if (!ptr) return;

    for (int a=0; a<MAXALLOCS; a++)
    {
        if (__fs_allocs[a] == ptr)
        {
            __fs_allocs[a]= NULL;
            if (FS_HARDDEL)
            {
                u32 blocksize= read32(&ptr->size);
                memset((u8*)ptr,0,blocksize);
            }
            return;
        }
    }
}

int fs_loadFAT()
{
    //Returns the number of files loaded (-1 on error)
    int loaded= 0;

    /*
    for (int i=0; i<MAXALLOCS; i++)
    {
        seg_t* segptr= __fs_allocs[i];
        if (IS_FILE(segptr))
        {
            //If the segment is a file clear the FAT pointer in IWRAM
            __fs_allocs[i]= NULL;
        }
    }*/
    for (int i=0; i<SRAM_FAT_SIZE; i++)
    {
        //Copy file pointers from SRAM to IWRAM
        __fs_allocs[__fs_first_slot()]= (seg_t*)read32(&FILETABLE[i]);
        loaded++;
    }

    return loaded;
}

int fs_loadFAT_from(u8* base_addr)
{
    //Returns the number of files loaded (-1 on error)
    int loaded= 0;

    /*
    for (int i=0; i<MAXALLOCS; i++)
    {
        seg_t* segptr= __fs_allocs[i];
        if (IS_FILE(segptr))
        {
            //If the segment is a file clear the FAT pointer in IWRAM
            __fs_allocs[i]= NULL;
        }
    }*/
    for (int i=0; i<base_addr[1]; i++)
    {
        //Copy file pointers from SRAM to IWRAM
        __fs_allocs[__fs_first_slot()]= (seg_t*)read32(&(base_addr+sizeof(seg_t*)*base_addr[1])[i]);
        loaded++;
    }

    return loaded;
}

int fs_loadFAT_custom(file_t** filearray, int file_n)
{
    //Returns the number of files loaded (-1 on error)
    int loaded= 0;

    /*
    for (int i=0; i<MAXALLOCS; i++)
    {
        seg_t* segptr= __fs_allocs[i];
        if (IS_FILE(segptr))
        {
            //If the segment is a file clear the FAT pointer in IWRAM
            __fs_allocs[i]= NULL;
        }
    }*/
    for (int i=0; i<file_n; i++)
    {
        //Copy file pointers from SRAM to IWRAM
        __fs_allocs[__fs_first_slot()]= (seg_t*)filearray[i];
        loaded++;
    }

    return loaded;
}

void fs_initFAT()
{
    for (int i=0; i<MAXALLOCS; i++)
        if (IS_FILE(__fs_allocs[i]))
            __fs_allocs[i]= NULL;
}

void fs_initALL()
{
    for (int i=0; i<MAXALLOCS; i++)
        __fs_allocs[i]= NULL;
}

int fs_saveFAT()
{
    //Returns the number of files saved (-1 on error)
    int saved= 0;

    fl_erase4k(0);
    for (int i=0; i<SRAM_FAT_SIZE; i++)
        fl_secbuf[TOSECIND(FILETABLE+i*sizeof(file_t*))]= 0x00;

    for (int i=0; i<MAXFILES; i++)
    {
        seg_t* fptr= __fs_allocs[i];
        if (!fptr) continue;
        if (!IS_FILE(fptr)) continue;
        if ((u32)fptr<SRAM+SRAM_FAT_SIZE+2||(u32)fptr>=(u32)((u32)SRAM+SRAM_FAT_SIZE+2+sram_size))
                continue;

        write32(&fl_secbuf[TOSECIND(FILETABLE+i*sizeof(file_t*))], (u32)fptr);

        saved++;
    }

    fl_restore4k();
    return saved;
}

int fs_init()
{
    fl_erase();

    for (int i=0; i<SRAM_FAT_SIZE; i++)
        fl_write32((u8*)(FILETABLE+i), 0x00000000);
    fl_write8(&SRAM_INITBYTE, 0x69);
    fl_write8(&SRAM_FAT_SIZE, MAXFILES);

    return SRAM_INITBYTE;
}

// u32 fs_check()
// {
//     //Returns the sram init byte
//     //returns 0 if SRAM does not seem to be initialized
//     u8 ibyte= SRAM_INITBYTE;
//     u8 temp= 0;
//     sram_size= 0;
//     temp= ((u8*)SRAM)[sram_size];
//     do
//     {
//         ((u8*)SRAM)[sram_size]=temp;
//         sram_size++;
//         temp= ((u8*)SRAM)[sram_size];
//         ((u8*)SRAM)[sram_size]=0x55;
//     }
//     while(((u8*)SRAM)[sram_size]!=((u8*)SRAM)[0] && sram_size<SRAM_MAX);
//     SRAM_INITBYTE= ibyte;
//
//     if (ibyte==0xFF || ibyte==0x00)
//         return 0;
//     else
//         return ibyte;
// }

file_t* fs_newfile(char* name, u32 size)
{
    file_t* fptr= (file_t*)fs_alloc(FILESYSTEM_DOMAIN, size);
    if (!fptr) return NULL;

    fl_erase4k(GETSEC(fptr));
    fl_chsec_buf(&fl_secbuf[TOSECIND(&fptr->type)]);
    fl_secbuf[TOSECIND(fptr->type)]= '\0';
    fl_restore4k();
    // if (strrchr(name,'.')) //What is after the '.' becomes the file type
    //     for (int i= 0;
    //          i < ((u32)strrchr(name,'\0')-(u32)strrchr(name,'.')-1 < sizeof(fptr->type)?
    //          (u32)strrchr(name,'\0')-(u32)strrchr(name,'.')-1 : sizeof(fptr->type)); i++)
    //     {
    //         fl_chsec_buf(&fl_secbuf[TOSECIND(&fptr->type+i)]);
    //         fl_secbuf[TOSECIND(&fptr->type+i)]= *(strrchr(name,'.')+1);
    //     }
        // memcpy(fptr->type,strrchr(name,'.')+1,
        //     (u32)strrchr(name,'\0')-(u32)strrchr(name,'.')-1 < sizeof(fptr->type)?
        //     (u32)strrchr(name,'\0')-(u32)strrchr(name,'.')-1 : sizeof(fptr->type)
        // );
    fl_erase4k(GETSEC(&fptr->name));
    int namesz= 0;
    for (int i=0;
         i < (strrchr(name,'.') ?
            (int)(strrchr(name,'.')-name)
          : (int)(strrchr(name,'\0')-name)); i++)
        {
            fl_chsec_buf(&fl_secbuf[TOSECIND(&fptr->name+i)]);
            fl_secbuf[TOSECIND(&fptr->name+i)]= name[i];
            namesz++;
        }
    fl_chsec_buf(&fl_secbuf[TOSECIND(&fptr->name+namesz+1)]);
    fl_secbuf[TOSECIND(&fptr->name+namesz+1)]= '\0';
    fl_restore4k();
    // strncpy((char*)fptr->name,name,strrchr(name,'.')?
    //                                  (u16)(strrchr(name,'.')-name)
    //                                : (u16)(strrchr(name,'\0')-name));
    // fl_erase4k(GETSEC(&fptr->next));
    // write32(&fptr->next, 0);
    // fl_restore4k();

    if (FS_AUTOSAVE)
        fs_saveFAT();
    return fptr;
}

IWRAM_CODE ARM_CODE file_t* fs_getfileptr_trunc(char* name)
{
    for (int a=0; a<MAXFILES; a++)
    {
        file_t* file= (file_t*)__fs_allocs[a];
        if (!file) continue;
        #ifdef __FS_DEBUG
            char deb[256];
            sprintf(deb,"FS_GET...&nmatch:%s&nptr@index:0x%lx&nstr@index:%lc&nindex:%d",name,(u32)file,*file->name,a);
            __fs_debug(deb);
        #endif
        if (file)
            if(!_strcmp(file->name,name))
                return file;
    }
    return NULL;
}

IWRAM_CODE ARM_CODE file_t* fs_getfileptr(char* name)
{
    char match[24];
    char m_fname[13];
    char m_type[5];

    for (int a=0; a<MAXALLOCS; a++)
    {
        file_t* file= (file_t*)__fs_allocs[a];
        if (!file) continue;
        memset(m_fname,0,sizeof(file->name)+1);
        memset(m_type,0,sizeof(file->type)+1);
        memcpy(m_fname,file->name,sizeof(file->name));
        memcpy(m_type,file->type,sizeof(file->type));
        if (*file->type)
            sprintf(match,"%s.%s",m_fname,m_type);
        else
            sprintf(match,"%s",m_fname);
        #ifdef __FS_DEBUG
            char deb[256];
            sprintf(deb,"FS_GET...&nmatch:%s&nptr@index:0x%lx&nstr@index:%s&nindex:%d",name,(u32)file,match,a);
            __fs_debug(deb);
        #endif
        if(!_strcmp(match,name))
            return file;
    }
    return NULL;
}

int fs_delfile(char* name)
{
    //Returns 0 on success,
    //Returns 1 if file not found

    char match[24];
    char m_fname[13];
    char m_type[5];

    for (int a=0; a<MAXALLOCS; a++)
    {
        file_t* file= (file_t*)__fs_allocs[a];
        if (!file) continue;
        if (!IS_FILE(file)) continue;
        memset(m_fname,0,sizeof(file->name)+1);
        memset(m_type,0,sizeof(file->type)+1);
        memcpy(m_fname,file->name,sizeof(file->name));
        memcpy(m_type,file->type,sizeof(file->type));
        if (*file->type)
            sprintf(match,"%s.%s",m_fname,m_type);
        else
            sprintf(match,"%s",m_fname);
        #ifdef __FS_DEBUG
            char deb[256];
            sprintf(deb,"FS_DEL...&nmatch:%s&nptr@index:0x%lx&nstr@index:%s&nindex:%d",name,(u32)file,match,a);
            __fs_debug(deb);
        #endif
        if(!_strcmp(match,name))
        {
            fs_dealloc((seg_t*)file);
            if (FS_AUTOSAVE)
                fs_saveFAT();
            return 0;
        }
    }
    return 1;
}

int fs_renfile(char* name, char* newname)
{
    //Returns 0 on success,
    //Returns 1 if file not found

    file_t* fptr= fs_getfileptr(name);
    if (!fptr) return 1;
    char* extptr= strrchr(newname,'.');
    if (extptr)
    {
        memcpy(fptr->name,newname,
               (u32)strrchr(newname,'.')-(u32)newname < sizeof(fptr->name)?
               (u32)strrchr(newname,'.')-(u32)newname : sizeof(fptr->name)
        );
        memcpy(fptr->type,strrchr(newname,'.')+1,
               (u32)strrchr(newname,'\0')-(u32)strrchr(newname,'.') < sizeof(fptr->type)?
               (u32)strrchr(newname,'\0')-(u32)strrchr(newname,'.') : sizeof(fptr->type)
        );
    }
    else
    {
        //Newname has no extension
        memset(fptr->name,'\0',sizeof(fptr->name));
        memset(fptr->type,'\0',sizeof(fptr->type));
        memcpy(fptr->name,newname,
               strlen(newname) < sizeof(fptr->name)?
               strlen(newname) : sizeof(fptr->name)
        );
    }
    return 0;
}

u32 fs_getfilesize(char* name)
{
    file_t* fptr= fs_getfileptr(name);
    if (!fptr) return 0;
    return read32(&fptr->size)-FHEADER_SIZE;
}

u32 fs_used(void* base_addr, u32 memsize)
{
    u32 amount= 2+FILETABLE_SIZE;
    for (int a=0; a<MAXALLOCS; a++)
    {
        seg_t* file= __fs_allocs[a];
        if (file)
        {
            if ((void*)file<base_addr||(void*)file>=(void*)((u32)base_addr+memsize))
                continue;
            amount+=read32(&file->size)+FS_INTERLEAVE;
        }
    }
    return amount;
}
