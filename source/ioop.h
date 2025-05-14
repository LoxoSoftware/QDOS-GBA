//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/ioop.h      Header for IO operations functions and definitions      //
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

#ifndef IOOP_H
#define IOOP_H

#include "flashfs.h"

fhandle_t* fd_get_handle(fdesc_t fd)
{
    if (fd>FS_MAX_FHANDLES || fd<0)
    {
        sprintf(fs_error, "OPEN: Cannot get file handle");
        return NULL;
    }

    return STRUCT_ARR(fs_handles,fd,fhandle_t);
}

fdesc_t fs_fopen(char* fname, char mode)
{
    //Returns <0 on error

    char tstr[2]= { mode&0x7F, '\0' };
    file_t* fptr= fs_getfileptr(fname);

    if (!fptr && strpbrk(tstr,"rwa"))
    {
        fptr= fs_newfile(fname, 0);

        if (!fptr)
        {
            sprintf(fs_error, "OPEN: Cannot create new file \"%s\"", fname);
            return -5;
        }
    }

    if (!fptr || !fs_check_fname(fname))
    {
        sprintf(fs_error, "OPEN: No such file \"%s\"", fname);
        return -1;
    }

    for (fdesc_t ifd=0; ifd<FS_MAX_FHANDLES; ifd++)
    {
        if (!fs_handles[ifd]->mode)
        {
            if (fs_handles[ifd]->file == fptr)
            {
                sprintf(fs_error, "OPEN: Resource \"%s\" is already in use", fname);
                return -4;
            }
        }
    }

    if (!strpbrk(tstr,"rRwWaA"))
    {
        sprintf(fs_error, "OPEN: Invalid fopen mode");
        return -2;
    }

    for (fdesc_t ifd=0; ifd<FS_MAX_FHANDLES; ifd++)
    {
        if (!STRUCT_ARR(fs_handles,ifd,fhandle_t)->mode)
        {
            //Allocate new handle
            fhandle_t* hdl= fd_get_handle(ifd);
            if (!hdl) return -3;
            hdl->mode= mode;
            hdl->file= fptr;
            hdl->cursor= 0;
            sprintf(fs_error, "OPEN: ok (0x%lx)", (u32)hdl);
            return ifd;
        }
    }

    sprintf(fs_error, "OPEN: Too many open files");
    return -3;
}

void fs_fclose(fdesc_t fd)
{
    fhandle_t* hdl= fd_get_handle(fd);
    if (!hdl) return;

    hdl->mode= '\0';
    hdl->file= NULL;
}

IWRAM_CODE ARM_CODE
u8 fs_fread(fdesc_t fd)
{
    //TODO: Fix reliance on continous file access

    fhandle_t* hdl= fd_get_handle(fd);
    if (!hdl) return 0x69;

    if (!hdl->file)
    {
        sprintf(fs_error, "READ: Null file pointer");
        return 0x69;
    }

    char mode[2]= { hdl->mode&0x7F, '\0' };
    if (!strpbrk(mode, "rRwWaA"))
    {
        sprintf(fs_error, "READ: File is not open for reading");
        return 0x00;
    }
    if (hdl->cursor >= read32(&hdl->file->size))
    {
        hdl->cursor= read32(&hdl->file->size);
        sprintf(fs_error, "READ: EOF reached");
        return '\0';
    }

    hdl->cursor++;
    return ((char*)(&hdl->file->data))[hdl->cursor-1];
}

IWRAM_CODE ARM_CODE
void fs_fwrite(fdesc_t fd, u8 ch)
{
    //TODO: Fix reliance on continous file access
    //WARNING: Flush the buffer when done! use fl_flush()

    fhandle_t* hdl= fd_get_handle(fd);
    if (!hdl) return;

    if (!hdl->file)
    {
        sprintf(fs_error, "WRITE: Null file pointer");
        return;
    }

    char mode[2]= { hdl->mode&0x7F, '\0' };
    if (!strpbrk(mode, "wWaA"))
    {
        sprintf(fs_error, "WRITE: File is not open for writing");
        return;
    }

    if (strpbrk(mode, "WA"))
        if (hdl->cursor >= read32(&hdl->file->size))
        {
            hdl->cursor= read32(&hdl->file->size);
            sprintf(fs_error, "WRITE: EOF reached");
            return;
        }

    if (strpbrk(mode, "wa"))
    {
        //Append handling
    }

    if (fl_lastsector != FL_SECTOR((u8*)(&hdl->file->data)))
    {
        if (fl_lastsector != 255)
            fl_restore4k(); //Just in case
        fl_get4k(FL_SECTOR(&hdl->file->data));
    }

    hdl->cursor++;
    fl_write8(&((u8*)(&hdl->file->data))[hdl->cursor-1], ch);
}

void fs_fseek(fdesc_t fd, u32 pos)
{
    fhandle_t* hdl= fd_get_handle(fd);
    if (!hdl) return;

    if (!hdl->file)
    {
        sprintf(fs_error, "SEEK: Null file pointer");
        return;
    }
    if (pos >= hdl->file->size)
        pos= hdl->file->size;

    hdl->cursor= pos;
}

u32 fs_ftell(fdesc_t fd)
{
    fhandle_t* hdl= fd_get_handle(fd);
    if (!hdl) return 0;

    s32 result= read32(&hdl->file->size)-(hdl->cursor);
    return result>=0?result:0;
}

#endif
