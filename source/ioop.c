//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/ioop.c      IO operation function implementations                   //
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

//Special system files
file_t* sys_stdout= NULL;
file_t* sys_stdin= NULL;

fhandle_t* fd_get_handle(fdesc_t fd)
{
    if (fd>FS_MAX_FHANDLES || fd<0)
    {
        sprintf(fs_error, "OPEN: Cannot get file handle");
        return NULL;
    }

    return STRUCT_ARR(fs_handles,fd,fhandle_t);
}

file_t* __open_sysfile(char* fname, char mode)
{
    char* dot= strrchr(fname,'.');

    if (!dot || strcmp(dot, ".!sys"))
        return NULL;

    //Cannot create files if mode doesn't allow to do so
    if (strchr("RWA", mode))
        return NULL;

    //Create file in EWRAM
    file_t* fptr= (file_t*)malloc(FS_ALLOC_DEFAULT);
    if (!fptr)
        return NULL;

    int name_len= (u32)dot-(u32)fname;
    int type_len= strlen(dot)-1;

    //Get file extension and copy it + the name
    *(char*)((u32)&fptr->type+type_len)= '\0';
    for (int i = 0; i < type_len; i++)
        *(char*)((u32)&fptr->type+i)= *(dot+i+1);

    *(char*)((u32)&fptr->name+name_len)= '\0';
    strncpy((char*)fptr->name, fname, name_len);

    fptr->size= FS_ALLOC_DEFAULT-FS_FILEHEAD_SZ;
    fptr->attr= 0x55555555;

    if (!strcmp(fname, "stdout.!sys"))
        sys_stdout= fptr;
    if (!strcmp(fname, "stdin.!sys"))
        sys_stdin= fptr;

    return fptr;
}

fdesc_t fs_fopen(char* fname, char mode)
{
    //Returns <0 on error

    file_t* fptr= __open_sysfile(fname, mode);

    //Handle special system files
    if (fptr)
        goto skip_fname_checks;

    fptr= fs_getfileptr(fname);

    if (!fptr && strchr("rwa", mode))
    {
        fptr= fs_newfile(fname, FS_ALLOC_DEFAULT-FS_FILEHEAD_SZ);

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

skip_fname_checks:

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

    if (!strchr("rRwWaA", mode))
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

    if (hdl->file->attr == 0x55555555)
    {
        //Deallocate system file
        //  WARNING: Subject to change in the future

        if (_strncmp(hdl->file->name, "stdout", FS_FNAME_SZ))
            sys_stdout= NULL;
        if (_strncmp(hdl->file->name, "stdin", FS_FNAME_SZ))
            sys_stdin= NULL;

        free(hdl->file);
    }

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

    if (!strchr("rRwWaA", hdl->mode))
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

    if (!strchr("wWaA", hdl->mode))
    {
        sprintf(fs_error, "WRITE: File is not open for writing");
        return;
    }

    if (strchr("WA", hdl->mode))
        if (hdl->cursor >= read32(&hdl->file->size))
        {
            hdl->cursor= read32(&hdl->file->size);
            sprintf(fs_error, "WRITE: EOF reached");
            return;
        }

    if (strchr("WA", hdl->mode))
    {
        //TODO: Append handling
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
