//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/io.h        Everything that's needed to handle I/O                  //
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

#ifndef IO_H
#define IO_H

#include <gba.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FL_DOMAIN           0x08000000              //Where the writable backup medium starts at
#define FS_DOMAIN           0x08F80000              //Where we want to write the data
#define FL_SECTOR_SZ_BS     17                      //Default is 12 (bit shift of sector size)
#define FL_SECTOR_SZ        (1<<FL_SECTOR_SZ_BS)    //Default is 0x1000
#define FL_SECTOR_BUF_PTR   0x0E000000              //Location of sector buffer, -1 = Static alloc in IWRAM (default)

#if FL_DOMAIN >= 0x08000000
#define FL_16BIT_ACCESS     true
#else
#define FL_16BIT_ACCESS     false
#endif

#define FL_MAGIC1_ADDR      0x0AAA                  //Default is 0x5555
#define FL_MAGIC1_DATA      0xA9                    //Default is 0xAA
#define FL_MAGIC2_ADDR      0x0555                  //Default is 0x2AAA
#define FL_MAGIC2_DATA      0x56                    //Default is 0x55

#define SRAM_MAX            0x00020000  //128k
#define FILE_MAX_SECTS      128         //Maximum file size is limited to 128 KiB
#define FS_SECTOR_SZ        1024
#define FS_ROOTHEAD_SZ      24
#define FS_INIT_SEQ         0x694200A5
#define FS_FILEHEAD_SZ      24
#define MAXALLOCS           ((FS_SECTOR_SZ-FS_ROOTHEAD_SZ)>>1)
#define MAXFILECHUNK        (FS_SECTOR_SZ-FS_FILEHEAD_SZ)

#define FS_FNAME_SZ         12
#define FS_FTYPE_SZ         4

#define FS_MAX_FHANDLES     16
#define FS_ALLOC_DEFAULT    FS_SECTOR_SZ

#define FL_AUTOSAVE_FAT     true   //If set to false, use the "exit" command to save the FAT

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
//  'r' File open for reading only(+ create)
//  'w' File open for reading and writing (+ create)
//  'a' File open for writing in append mode (+ create)
//  'R' File open for reading only (fail if file doesn't exist)
//  'W' File open for reading and writing (fail if file doesn't exist)
//  'A' File open for writing in append mode (fail if file doesn't exist)
// Anything else is invalid, file is not open
} fhandle_t;

typedef s16 fdesc_t;

extern fhandle_t*  fs_handles[];

extern u32         sram_size;
extern u16         fs_secmap[];
extern fsroot_t*   fs_root;

/// Flash driving ///

//Get flash sector from address
#define FL_SECTOR(addr)         (((u32)addr-FL_DOMAIN)>>FL_SECTOR_SZ_BS)

//Get byte index in the secbuf from address
#define FL_BUFIND(addr)         (((u32)addr-FS_DOMAIN)&(FL_SECTOR_SZ-1))

//Get filesystem block index from file address
#define FS_PTRBLOCKID(ptr)      (((u32)ptr-(u32)fs_root)/FS_SECTOR_SZ-1)

#define STRUCT_ARR(arr, index, type) ((type*)((u32)&arr+index*sizeof(type*)))

extern u8  fl_prevbank;    //Last used FLASH bank
extern u16 fl_lastsector;  //Last cached FLASH sector
#if FL_SECTOR_BUF_PTR < 0
extern u8 fl_secbuf[];     //Sector cache
#else
extern u8* fl_secbuf;
#endif

extern char fs_error[];

//Inline a NOP delay
void mdelay(int delay);

//Poke data into flash with a delay
void fl_wspoke(u32 offset, u8 val);

u16 fl_getid();

void fl_selbank(u8 bank);

void fl_eraseALL();

//Flash sector operations
void fl_drop4k(u16 sector);
void fl_erase4k(u16 sector);
void fl_get4k(u16 sector);
void fl_restore4k();

//Write a byte directly to flash
//  WARNING: The sector has to be already cleared
void fl_writeDirect(u32 ofs, u8 value);
//Write a byte in the secbuf syncing with the flash if necessary
void fl_write8(u8* ptr, u8 value);
//Read a byte from the secbuf syncing it with the flash if necessary
u8 fl_read8(u8* ptr);

/// Helper functions ///

int _strcmp(char* str1, char* str2);

int _strncmp(char* str1, char* str2, u16 max);

int _strncmp_flsrc(char* flstr, char* srcstr, u16 max);

//Read/write in little-endian (reverse order)
u32 read32(void* ptr);
void write32(void* ptr, u32 value);
u16 read16(void* ptr);
void write16(void* ptr, u16 value);
u32 fl_read32(u8* ptr);
u16 fl_read16(u8* ptr);

/// Filesystem implementation ///

u8 fs_in_domain(void* ptr);

int fs_first_slot();

void fs_init();

void fs_saveFlash();

void fs_loadFlash();

void fs_format();

int fs_alloc(u32 blocksize);

int fs_dealloc(int blockid);

file_t* fs_getfileptr(char* fname);

file_t* fs_getfileptr_trunc(char* fname);

bool fs_check_fname(char* fname);

file_t* fs_newfile(char* fname, u32 fsize);

bool fs_rmfile(char* fname);

void fs_flush();

u32 fs_used();

u8 fs_check();

/// I/O operations ///

fhandle_t* fd_get_handle(fdesc_t fd);

fdesc_t fs_fopen(char* fname, char mode);

void fs_fclose(fdesc_t fd);

u32 fs_fread(fdesc_t fd, u8* buffer, u32 size);

void fs_fwrite(fdesc_t fd, u8* buffer, u32 size);

void fs_fseek(fdesc_t fd, u32 pos);

u32 fs_ftell(fdesc_t fd);

#endif
