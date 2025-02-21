//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/flashfs.h   Filesystem implementation and FLASH driver              //
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

#ifndef FLASHFS_H
#define FLASHFS_H

//TODO: Implement flash bank switching

//NOTE: This will only allocate in SRAM, use standard allocator functions
//      to allocate in WRAM

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SRAM_MAX        0x00020000  //128k
#define FILE_MAX_SECTS  128         //Maximum file size is limited to 128 KiB
#define FS_SECTOR_SZ    1024
#define FS_ROOTHEAD_SZ  24
#define FS_INIT_SEQ     0x694200A5
#define FS_FILEHEAD_SZ  24
#define MAXALLOCS       ((FS_SECTOR_SZ-FS_ROOTHEAD_SZ)>>1)
#define MAXFILECHUNK    (FS_SECTOR_SZ-FS_FILEHEAD_SZ)

#define FS_FNAME_SZ     12
#define FS_FTYPE_SZ     4

#define FS_MAX_FHANDLES 16

u32 sram_size= SRAM_MAX;

typedef struct
{
    u32 init_sequence;
    u8  reserved[FS_ROOTHEAD_SZ-sizeof(u32)];
    u16 secmap[MAXALLOCS];
    //NOTE: MSB of secmap[x] is set if the sector is formatted as a file
} fsroot_t;

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
//  'r' File open for reading (+ create)
//  'w' File open for reading and writing (+ create)
//  'p' File open for writing only (+ create) (pipe)
//  'R' File open for reading (fail if file doesn't exist)
//  'W' File open for reading and writing (fail if file doesn't exist)
//  'P' File open for writing only (fail if file doesn't exist) (pipe)
//  Set MSB to use append mode
// Anything else is invalid, file is not open
} fhandle_t;

typedef s16 fdesc_t;                //File descriptor

u16         fs_secmap[MAXALLOCS];   //Buffer copy in IWRAM
fsroot_t*   fs_root= (fsroot_t*)SRAM;
fhandle_t*  fs_handles[FS_MAX_FHANDLES];

char fs_error[128];

/// Flash driving ///

#define FL_SECTOR(addr)         (((u32)addr-SRAM)>>12)    //Get sector from address
#define FL_BUFIND(addr)         (((u32)addr-SRAM)&0x0FFF) //Get buffer index from address
#define FS_PTRBLOCKID(ptr)      (((u32)ptr-(u32)fs_root)/FS_SECTOR_SZ-1) //Get fs sector index from file address

u8 fl_prevbank = 0;
u8 fl_lastsector = 0;
u8 fl_secbuf[4096];

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

u16 fl_getid()
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

void fl_selbank(u8 bank)
{
    //Select the bank in the flash
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0xB0);
    fl_wspoke(0x0000, bank);
    fl_prevbank = bank;
}

void fl_eraseALL()
{
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0x80);
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0x10);
    mdelay(1000);
}

void fl_drop4k(u8 sector)
{
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0x80);
    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(sector*0x1000, 0x30);
    mdelay(100);
}

IWRAM_CODE void fl_erase4k(u8 sector)
{
    //Backup the sector before erasing it
    for (int i=0; i<4096; i++)
        fl_secbuf[i]= ((u8*)SRAM)[sector*0x1000+i];
    //Now erase it
    fl_drop4k(sector);
    fl_lastsector= sector;
    //NOTE: Suggest to make modifications to fl_secbuf before restoring it
    //      instead of directly manipulating the flash
}

void fl_writeDirect(u32 ofs, u8 value);
IWRAM_CODE void fl_restore4k()
{
    fl_drop4k(fl_lastsector);
    for (int i=0; i<4096; i++)
        fl_writeDirect(fl_lastsector*0x1000+i, fl_secbuf[i]);
}

IWRAM_CODE void fl_writeDirect(u32 ofs, u8 value)
{
    if (ofs>>16 != fl_prevbank)
        fl_selbank((ofs-SRAM)>>16);

    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0xA0);
    *(u8*)(SRAM+ofs)= value;

    while(*(u8*)(SRAM+ofs) != value) ;
}

IWRAM_CODE void fl_write8(u8* ptr, u8 value)
{
    //NOTE: Does not write directly to flash, writes to the buffer
    //      you can access it with SRAM addresses

    u32 ofs= (u32)ptr-SRAM;

    if (ofs < 0 && ofs >= SRAM_MAX)
        return; //Pointer out of bounds

    if (ofs>>16 != fl_prevbank)
        fl_selbank(ofs>>16);
    ofs &= 0xFFFF;

    if (ofs < 0x1000*fl_lastsector || ofs >= 0x1000*(fl_lastsector+1))
    {
        //Need to write to another flash sector
        fl_restore4k();
        if ((u32)ptr-(u32)&fl_secbuf < 0)
            fl_erase4k(fl_lastsector-1);
        if ((u32)ptr-(u32)&fl_secbuf >= 4096)
            fl_erase4k(fl_lastsector+1);
    }

    fl_secbuf[ofs]= value;
}

IWRAM_CODE u8 fl_read8(u8* ptr)
{
    u32 ofs= (u32)ptr-SRAM;

    if (ofs < 0 && ofs >= SRAM_MAX)
        return 0x0069; //Pointer out of bounds

    if (ofs>>16 != fl_prevbank)
        fl_selbank(ofs>>16);
    ofs &= 0xFFFF;

    return ((u8*)SRAM)[ofs];
}

/// Filesystem implementation ///

IWRAM_CODE int _strcmp(char* str1, char* str2)
{
    if (!str1 || !str2)
        return -1;
    for (int i=0; ; i++)
    {
        if (str1[i] != str2[i])
            return i+1;
        if (!str1[i] && !str2[i])
            return 0;
    }
}

IWRAM_CODE int _strncmp(char* str1, char* str2, u16 max)
{
    if (!str1 || !str2)
        return -1;
    for (int i=0; i<max; i++)
    {
        if (str1[i] != str2[i])
            return i+1;
        if (!str1[i] && !str2[i])
            return 0;
    }
    return 0;
}

u8 fs_in_domain(void* ptr)
{
    return ptr >= (void*)fs_root && ptr <= (void*)fs_root+sram_size;
}

int fs_first_slot()
{
    for (int i=0; i<MAXALLOCS; i++)
        if (fs_secmap[i] == 0xFFFF)
            return i;
    return -1;
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

void write32(void* ptr, u32 value)
{
    if (fs_in_domain(ptr))
    {
        //fl_erase4k(GETSEC(ptr));
        fl_write8(ptr+0, value);
        fl_write8(ptr+1, value >> 8);
        fl_write8(ptr+2, value >> 16);
        fl_write8(ptr+3, value >> 24);
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
    if (fs_in_domain(ptr))
    {
        fl_write8(ptr+0, value);
        fl_write8(ptr+1, value >> 8);
    }
    else
    {
        *(u8*)(ptr+0) = value;
        *(u8*)(ptr+1) = value >> 8;
    }
}

void fs_init()
{
    //Only initializes the driver's variables,
    //  it is non destructive

    memset(fl_secbuf, 0xFF, 4096);
    memset((u8*)fs_secmap, 0xFF, sizeof(fs_secmap));
}

void fs_saveFlash()
{
    //Copy temporary secmap to flash
    fl_erase4k(FL_SECTOR(fs_root->secmap));
    for (int f=0; f<MAXALLOCS; f++)
        write16(fl_secbuf+FL_BUFIND(fs_root->secmap)+f*sizeof(u16), fs_secmap[f]);
    fl_restore4k(FL_SECTOR(fs_root->secmap));
}

void fs_loadFlash()
{
    memset((u8*)fs_secmap, 0xFF, sizeof(fs_secmap));
    for (int i=0; i<MAXALLOCS; i++)
        fs_secmap[i]= read16((u8*)((u32)&fs_root->secmap+i*sizeof(u16)));
}

void fs_format()
{
    fl_eraseALL();
    fl_erase4k(0);
    write32(fs_root, FS_INIT_SEQ);
    fl_restore4k(0);
}

int fs_alloc(u32 blocksize)
{
    //Returns block position in secmap

    u16 slot[FILE_MAX_SECTS];
    u16 slot_count= 0; //Slots actually allocated
    u16 slots_req= blocksize/FS_SECTOR_SZ+1;

    for (int i=0; i < slots_req && i < FILE_MAX_SECTS; i++)
    {
        slot[i]= fs_first_slot();
        if (slot[i] < 0) break;
        //Preemptively set next sector to last sector id.
        fs_secmap[slot[i]]= 0x0000;
        if (i>0 && i<slots_req-1) //Link to next sector
            fs_secmap[slot[i-1]]= slot[i];
        slot_count++;
    }

    if (slot_count < slots_req)
    {
        sprintf(fs_error, "ALLOC: Not enough sectors available in filesystem"
            " (%d/%d)", slot_count,slots_req
        );
        //Revert slot allocation
        for (int i=0; i<slot_count; i++)
            fs_secmap[slot[i]]= 0xFFFF;
        return -1;
    }

    sprintf(fs_error, "ALLOC: ok (%d)", slot[0]);

    //NOTE: set MSB of the first secmap slot if the block is a file
    return slot[0];
}

int fs_dealloc(int blockid)
{
    //Returns the index of the last block deleted in secmap

    if (fs_secmap[blockid]==0xFFFF)
    {
        sprintf(fs_error, "FREE: Sector is not allocated");
        return -1;
    }

    int s= blockid;
    while (1)
    {
        if ((fs_secmap[s]&0x7FFF)==0x0000)
        {
            fs_secmap[s]= 0xFFFF;
            break;
        }
        if ((fs_secmap[s]&0x7FFF)==0x7FFF)
        {
            sprintf(fs_error, "FREE: secmap entry does not contain a valid pointer. You have stray sectors");
            return -1;
        }
        int next_s= fs_secmap[s]&0x7FFF;
        fs_secmap[s]= 0xFFFF;
        s= next_s;
    }

    fs_secmap[blockid] = 0xFFFF;
    sprintf(fs_error, "FREE: ok (%d)", blockid);

    return s;
}

file_t* fs_getfileptr(char* fname)
{
    char* dot= strrchr(fname,'.');
    // if (dot) if ((u32)dot-(u32)fname==1)
    //     return NULL; //Filenames with an ampty type after the '.' are invalid

    for (int f=0; f<MAXALLOCS; f++)
    {
        if (!(read16(&fs_root->secmap[f])&0x8000) || (read16(&fs_root->secmap[f])==0xFFFF))
            continue;

        file_t* fptr= (file_t*)((u32)fs_root+(FS_SECTOR_SZ*(f+1)));

        if (dot)
        {
            if (fptr->type[0]=='\0' || fptr->type[0]==0xFF)
                continue;
            if (!_strncmp(fptr->name, fname, (u32)dot-(u32)fname))
                if (!_strncmp(fptr->type, dot+1, FS_FTYPE_SZ))
                    return fptr;
        }
        else
        {
            if (fptr->type[0]!='\0' && fptr->type[0]!=0xFF)
                continue;
            if (!_strncmp(fptr->name, fname, FS_FNAME_SZ))
                return fptr;
        }
    }

    return NULL;
}

file_t* fs_getfileptr_trunc(char* fname)
{
    //Omits the file extension

    for (int f=0; f<MAXALLOCS; f++)
    {
        if (!(read16(&fs_root->secmap[f])&0x8000) || (read16(&fs_root->secmap[f])==0xFFFF))
            continue;

        file_t* fptr= (file_t*)((u32)fs_root+(FS_SECTOR_SZ*(f+1)));

        if (!_strncmp(fptr->name, fname, FS_FNAME_SZ))
            return fptr;
    }

    return NULL;
}

bool fs_check_fname(char* fname)
{
    char* dot= strrchr(fname,'.');

    if (dot)
    {
        if ((u32)dot-(u32)fname>FS_FNAME_SZ+1)
        {
            sprintf(fs_error, "Name part is too long");
            return false;
        }
        if (strlen(fname)-((u32)dot-(u32)fname)>FS_FTYPE_SZ+1)
        {
            sprintf(fs_error, "File extension is too long");
            return false;
        }
        if (dot[1] == '!')
        {
            sprintf(fs_error, "Invalid attempt to select system file");
            return false;
        }
    }
    else
    {
        if (strlen(fname)>FS_FNAME_SZ)
        {
            sprintf(fs_error, "File name is too long");
            return false;
        }
    }

    if (strpbrk(fname,"/\\$#\""))
    {
        sprintf(fs_error, "File name contains invalid characters");
        return false;
    }

    return true;
}

file_t* fs_newfile(char* fname, u32 fsize)
{
    fs_loadFlash();

    if (!fs_check_fname(fname)) return NULL;

    int fid= fs_alloc(fsize+FS_FILEHEAD_SZ);
    if (fid < 0) return NULL;
    file_t* fptr= (file_t*)((u32)fs_root+FS_SECTOR_SZ*(fid+1));

    fs_secmap[fid] += 0x8000; //Set MSB to indicate that the block is a file

    fs_saveFlash();
    //Write file header
    fl_erase4k(FL_SECTOR(fptr));
    file_t* fptr_buf= (file_t*)(fl_secbuf+FL_BUFIND(fptr));

    int name_len= strrchr(fname,'.')?
            (u16)(strrchr(fname,'.')-fname)
            : (u16)(strrchr(fname,'\0')-fname);
    int type_len= (u32)strrchr(fname,'\0')-(u32)strrchr(fname,'.')-1 < sizeof(fptr_buf->type)?
            (u32)strrchr(fname,'\0')-(u32)strrchr(fname,'.')-1 : sizeof(fptr_buf->type);

    //Get file extension and copy it + the name
    if (strrchr(fname,'.'))
    {
        *(char*)((u32)&fptr_buf->type+type_len)= '\0';
        for (int i = 0; i < type_len; i++)
            *(char*)((u32)&fptr_buf->type+i)= *(strrchr(fname,'.')+i+1);
    }
    else *(char*)((u32)&fptr_buf->type)= '\0';

    *(char*)((u32)&fptr_buf->name+name_len)= '\0';
    strncpy((char*)fptr_buf->name,fname,name_len);

    // strncpy(fptr_buf->type, "", 1);
    // strncpy(fptr_buf->name, fname, 11);
    // *(u8*)((u32)fptr_buf->name+11)= '\0';
    fptr_buf->size= fsize;
    fptr_buf->attr= 0x00000000;

    fl_restore4k();

    return fptr;
}

bool fs_rmfile(char* fname)
{
    //Returns false on error

    fs_loadFlash(); //Sync temporary secmap to flash contents

    if (!fs_check_fname(fname))
        return false;

    file_t* file= fs_getfileptr(fname);
    if (!file)
    {
        sprintf(fs_error, "No such file");
        return false;
    }

    if (fs_dealloc(FS_PTRBLOCKID(file))<0)
        return false;

    fs_saveFlash(); //Flush changes to flash

    return true;
}

fdesc_t fs_fopen(char* fname, char mode)
{
    //Returns <0 on error

    file_t* fptr= fs_getfileptr(fname);

    if (!fptr || !fs_check_fname(fname))
    {
        sprintf(fs_error, "OPEN: No such file \"%s\"", fname);
        return -1;
    }

    char tstr[2]= { mode&0x7F, '\0' };

    if (!strpbrk(tstr,"rRwWpP"))
    {
        sprintf(fs_error, "OPEN: Invalid fopen mode");
        return -2;
    }

    for (fdesc_t ifd=0; ifd<FS_MAX_FHANDLES; ifd++)
    {
        if (!fs_handles[ifd]->mode)
        {
            //Allocate new handle
            fs_handles[ifd]->mode= mode;
            fs_handles[ifd]->file= fptr;
            fs_handles[ifd]->cursor= 0;
            sprintf(fs_error, "OPEN: ok (%d)", ifd);
            return ifd;
        }
    }

    sprintf(fs_error, "OPEN: Too many open files");
    return -3;
}

void fs_fclose(fdesc_t fd)
{
    if (fd>FS_MAX_FHANDLES || fd<0)
        return;

    fs_handles[fd]->mode= '\0';
    fs_handles[fd]->file= NULL;
}

u32 fs_used()
{
    u32 bytes_used= FS_SECTOR_SZ;

    for (int f=0; f<MAXALLOCS; f++)
    {
        if (!(read16(&fs_root->secmap[f])&0x8000) || (read16(&fs_root->secmap[f])==0xFFFF))
            continue;

        bytes_used += FS_SECTOR_SZ;
        int s=f;

        while (read16(&fs_root->secmap[s])&0x7FFF)
        {
            bytes_used += FS_SECTOR_SZ;
            s = (read16(&fs_root->secmap[s]))&0x7FFF;
        }
    }

    return bytes_used;
}

u8 fs_check()
{
    //Returns false if filesystem is invalid
    return (read32(&fs_root->init_sequence) == FS_INIT_SEQ);
}

#endif
