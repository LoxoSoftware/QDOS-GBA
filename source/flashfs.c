//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/flashfs.c   Filesystem implementation and FLASH driver              //
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

#include "io.h"

u32         sram_size= SRAM_MAX;
u8          fl_prevbank= 255;
u8          fl_lastsector= 255;
u8          fl_secbuf[4096];

u16         fs_secmap[MAXALLOCS];
fsroot_t*   fs_root= (fsroot_t*)SRAM;

fhandle_t*  fs_handles[FS_MAX_FHANDLES];
char        fs_error[128];

inline void mdelay(int delay)
{
    for (int i=0; i<delay; i++)
        asm volatile ("MOV R11,R11");
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

IWRAM_CODE ARM_CODE void fl_erase4k(u8 sector)
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

IWRAM_CODE ARM_CODE void fl_get4k(u8 sector)
{
    for (int i=0; i<4096; i++)
        fl_secbuf[i]= ((u8*)SRAM)[sector*0x1000+i];
    fl_lastsector= sector;
}

IWRAM_CODE ARM_CODE
void fl_restore4k()
{
    fl_drop4k(fl_lastsector);
    for (int i=0; i<4096; i++)
        fl_writeDirect(fl_lastsector*0x1000+i, fl_secbuf[i]);
}

IWRAM_CODE ARM_CODE
void fl_writeDirect(u32 ofs, u8 value)
{
    if (ofs>>16 != fl_prevbank)
        fl_selbank((ofs-SRAM)>>16);

    fl_wspoke(0x5555, 0xAA);
    fl_wspoke(0x2AAA, 0x55);
    fl_wspoke(0x5555, 0xA0);
    *(u8*)(SRAM+ofs)= value;

    while(*(u8*)(SRAM+ofs) != value) ;
}

IWRAM_CODE ARM_CODE
void fl_write8(u8* ptr, u8 value)
{
    //NOTE: Does not write directly to flash, writes to the buffer
    //      you can access it with SRAM addresses
    //WARNING: You MUST copy the active sector to the buffer before writing to it!!
    //              fl_erase4k(FL_SECTOR(write_address))  and
    //         remeber to flush the buffer when done!! use fl_restore4k()

    u32 ofs= (u32)ptr-SRAM;

    if (ofs < 0 && ofs >= SRAM_MAX)
        return; //Pointer out of bounds

    if (ofs>>16 != fl_prevbank)
        fl_selbank(ofs>>16);

    if (ofs < 0x1000*fl_lastsector || ofs >= 0x1000*(fl_lastsector+1))
    {
        //Need to write to another flash sector
        fl_restore4k();
        if ((u32)ptr-(u32)&fl_secbuf < 0)
            fl_erase4k(fl_lastsector-1);
        if ((u32)ptr-(u32)&fl_secbuf >= 4096)
            fl_erase4k(fl_lastsector+1);
    }

    fl_secbuf[ofs&0x0FFF]= value;
}

IWRAM_CODE ARM_CODE
u8 fl_read8(u8* ptr)
{
    u32 ofs= (u32)ptr-SRAM;

    if (ofs < 0 && ofs >= SRAM_MAX)
        return 0x0069; //Pointer out of bounds

    if (ofs>>16 != fl_prevbank)
        fl_selbank(ofs>>16);

    if (ofs < 0x1000*fl_lastsector || ofs >= 0x1000*(fl_lastsector+1) || fl_lastsector == 255)
    {
        //Requested address is not stored in the buffer, so get the byte directly from flash
        //Caching a new sector may not probably be worth it for the performance gained
        return ((u8*)SRAM)[ofs&0xFFFF];
    }
    else
    return fl_secbuf[ofs&0x0FFF];
}

inline u32 fl_read32(u8* ptr)
{
    return fl_read8(ptr)+(fl_read8(ptr+1)<<8)
           +(fl_read8(ptr+2)<<16)+(fl_read8(ptr+3)<<24);
}

inline u16 fl_read16(u8* ptr)
{
    return fl_read8(ptr)+(fl_read8(ptr+1)<<8);
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

/// Helper functions ///

IWRAM_CODE ARM_CODE
int _strcmp(char* str1, char* str2)
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

IWRAM_CODE ARM_CODE
int _strncmp(char* str1, char* str2, u16 max)
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

IWRAM_CODE ARM_CODE
int _strncmp_flsrc(char* flstr, char* srcstr, u16 max)
{
    //Compare between a string in flash and a string not in flash
    // comparing with fl_secbuf if necessary
    //You could use the normal _strncmp but you should use this for
    // reliability and predictability if comparing from something in flash
    //  flstr is a string IN flash
    //  srcstr is a string NOT IN flash

    if (!flstr || !srcstr)
        return -1;
    for (int i=0; i<max; i++)
    {
        u8 a= fl_read8((u8*)(flstr+i)), b= *(u8*)(srcstr+i);

        if (a != b)
            return i+1;
        if (!a && !b)
            return 0;
    }
    return 0;
}

/// Filesystem implementation ///

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

void fs_init()
{
    //Only initializes the driver's variables,
    //  it is non destructive

    memset(fl_secbuf, 0xFF, 4096);
    memset((u8*)fs_secmap, 0xFF, sizeof(fs_secmap));
    memset(&((*fs_handles)->mode), '\0', sizeof(fhandle_t));

    //fs_loadFlash();
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
        fs_secmap[i]= fl_read16((u8*)((u32)&fs_root->secmap+i*sizeof(u16)));
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
        if (!(fs_secmap[f]&0x8000) || (fs_secmap[f]==0xFFFF))
            continue;

        file_t* fptr= (file_t*)((u32)fs_root+(FS_SECTOR_SZ*(f+1)));

        if (dot)
        {
            if (fptr->type[0]=='\0' || fptr->type[0]==0xFF)
                continue;
            if (!_strncmp_flsrc(fptr->name, fname, (u32)dot-(u32)fname))
                if (!_strncmp_flsrc(fptr->type, dot+1, FS_FTYPE_SZ))
                    return fptr;
        }
        else
        {
            if (fptr->type[0]!='\0' && fptr->type[0]!=0xFF)
                continue;
            if (!_strncmp_flsrc(fptr->name, fname, FS_FNAME_SZ))
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
        if (!(fs_secmap[f]&0x8000) || (fs_secmap[f]==0xFFFF))
            continue;

        file_t* fptr= (file_t*)((u32)fs_root+(FS_SECTOR_SZ*(f+1)));

        if (!_strncmp_flsrc(fptr->name, fname, FS_FNAME_SZ))
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
    if (FL_AUTOSAVE_FAT)
        fs_loadFlash();

    if (!fs_check_fname(fname)) return NULL;

    int fid= fs_alloc(fsize+FS_FILEHEAD_SZ);
    if (fid < 0) return NULL;
    file_t* fptr= (file_t*)((u32)fs_root+FS_SECTOR_SZ*(fid+1));

    fs_secmap[fid] += 0x8000; //Set MSB to indicate that the block is a file

    if (FL_AUTOSAVE_FAT)
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

    if (FL_AUTOSAVE_FAT)
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

    if (FL_AUTOSAVE_FAT)
        fs_saveFlash(); //Flush changes to flash

    return true;
}

inline void fs_flush()
{
    fl_restore4k();
}

u32 fs_used()
{
    u32 bytes_used= FS_SECTOR_SZ;

    for (int f=0; f<MAXALLOCS; f++)
    {
        if (!(read16(&fs_root->secmap[f])&0x8000) || (read16(&fs_root->secmap[f])==0xFFFF))
            continue;

        bytes_used += FS_SECTOR_SZ;
        int s= f;

        while (read16(&fs_root->secmap[s])&0x7FFF)
        {
            bytes_used += FS_SECTOR_SZ;
            s= (read16(&fs_root->secmap[s]))&0x7FFF;
        }
    }

    return bytes_used;
}

u8 fs_check()
{
    //Returns false if filesystem is invalid
    return (read32(&fs_root->init_sequence) == FS_INIT_SEQ);
}
