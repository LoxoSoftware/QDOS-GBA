//define.h
#include "gmklib.h"

int __system_debuglevel = 0;

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

#define IWRAM_SIZE  32768
#define EWRAM_SIZE  262144
#define SRAM_SIZE   sram_size

#define MBCODE_SIZE 131072
