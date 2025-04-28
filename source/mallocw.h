//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/mallocw.h   Wrapper functions for memory allocations                //
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

#ifndef MALLOCW_H
#define MALLOCW_H

u32 ewram_used= 0;
u8  malloc_count= 0;

typedef struct
{
    void* memptr;
    u32 size;
} memblock_t;

memblock_t malloc_list[MALLOCS_MAX];

//Find empty spot in malloc_list, returns < 0 on error
int new_alloc()
{
    for (int i=0; i<MALLOCS_MAX; i++)
    {
        if (!malloc_list[i].memptr)
            return i;
    }
    return -1;
}

void* __real_malloc(size_t);
void* __real_calloc(size_t);
void* __real_realloc(void*,size_t);
void __real_free(void*);

//Returns < 0 on error
int find_alloc(void* ptr)
{
    if (!ptr)
        return -2;
    for (int i=0; i<MALLOCS_MAX; i++)
    {
        if (malloc_list[i].memptr == ptr)
            return i;
    }
    return -1;
}

void* __wrap_malloc(size_t sz)
{
    int new_alloc_slot= new_alloc();
    if (new_alloc_slot < 0)
    {
        console_printf("ERR: Allocation limit reached!&n");
        console_drawbuffer();
        return NULL;
    }
    void* newptr= __real_malloc(sz);
    if (!newptr) return NULL;
    ewram_used+= sz;
    malloc_list[new_alloc_slot].memptr= newptr;
    malloc_list[new_alloc_slot].size= sz;
    return newptr;
}

void* __wrap_calloc(size_t sz)
{
    int new_alloc_slot= new_alloc();
    if (new_alloc_slot < 0)
    {
        console_printf("ERR: Allocation limit reached!&n");
        console_drawbuffer();
        return NULL;
    }
    void* newptr= __real_malloc(sz);
    if (!newptr) return NULL;
    ewram_used+= sz;
    malloc_list[new_alloc_slot].memptr= newptr;
    malloc_list[new_alloc_slot].size= sz;
    return newptr;
}

void* __wrap_realloc(void* ptr, size_t newsz)
{
    int allocid= find_alloc(ptr);
    if (allocid < 0)
    {
        console_printf("ERR: Trying to realloc non allocated memory&n");
        console_drawbuffer();
        return NULL;
    }
    void* newptr= __real_realloc(ptr, newsz);
    if (!newptr) return NULL;
    ewram_used-= malloc_list[allocid].size;
    malloc_list[allocid].size= newsz;
    ewram_used+= newsz;
    malloc_list[allocid].memptr= newptr;
    return newptr;
}

void __wrap_free(void* ptr)
{
    int allocid= find_alloc(ptr);
    if (allocid < 0)
    {
        console_printf("ERR: Trying to free non allocated memory&n");
        console_drawbuffer();
        return;
    }
    __real_free(ptr);
    ewram_used-= malloc_list[allocid].size;
    malloc_list[allocid].memptr= NULL;
    malloc_list[allocid].size= 0;
}

#endif
