//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/define.h    OS misc definitions                                     //
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

#include "video.h"

int __system_debuglevel= 0;
bool __system_mainloop= true;

#define __int_argsize 4
#define __int_argword 16

//u8   __shell_stdout_queue[__PROGRAMDATA_MAX];
int  __shell_activeproc= -1; //-1 is the system shell

char* __com_promptstr= "\\";
bool  __com_prompt_show= true;
bool  __com_prompt_active= false;
#define __com_history_size 5
#define __com_history_word 64
int   __com_history_last= 0;
int   __com_history_index= 0;
char* __com_history;

char* kbstring= "";
int   kbstring_len= 0;
int   __keyboard_height= 64;
bool  __keyboard_autohide= true;
u16   __keyboard_txtfgcol= c_black;
u16   __keyboard_txtbgcol= c_aqua;

#define IWRAM_SIZE  (IWRAM_END-IWRAM)   //32k
#define EWRAM_SIZE  (EWRAM_END-EWRAM)   //256k
#define SRAM_SIZE   sram_size

#define MBCODE_SIZE (EWRAM_SIZE>>1)     //128k hard limit

#define MALLOCS_MAX    32
