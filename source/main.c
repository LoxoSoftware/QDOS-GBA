//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS             An experimental operating system for the GBA            //
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

//Libraries
#include <gba.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "define.h"
#include "gmklib.h"
#include "console.h"
#include "command.h"
#include "keyboard.h"
#include "syscall.h"
#include "tools.h"
#include "flashfs.h"

#define OS_NAME 		"QDOS"
#define OS_VERSION 		"0.0.7.5"

#define GETBIT(x,n) 	(((x) >> (n)) & 1)

#define ROMFS_DOMAIN    romfs, romfs_sz/sizeof(file_t*)

ALIGN(16) const char save_type[16]= "FLASH1M_V420\0\0\0\0";

int main()
{
	display_init(bitmap_mode);
	console_init();
	console_colors(c_black, c_ltgray, c_ltgray);

	irqSet(IRQ_DMA0, isr_IRQReceiver);
	irqEnable(IRQ_DMA0);
	
	void drawhistory() {
		
		__console_icolumn= strlen(__com_promptstr) + kbstring_len;
		
		int iy= __console_irow;
		int ix= __console_icolumn-kbstring_len-1;
		
		for(int i=0; i<kbstring_len; i++) {
		
			if (ix >= __console_charperw) {
		
				ix= 0; iy++;
			} else
				ix++;
			
			if (iy >= __console_charperh-1)
				return;
		
			draw_set_color(__console_fgcol);
			draw_rectangle(ix*__console_charw, iy*__console_charh-__console_yoffset, (ix+1)*__console_charw, (iy+1)*__console_charh-__console_yoffset, 0);
			draw_set_color(__console_bgcol);
			draw_char(ix*__console_charw, iy*__console_charh-__console_yoffset, kbstring[i]-32);
		}
	}
	
	console_printf("[...] SRAM check...&n");
	console_drawbuffer();

	bool firsttime= !fs_check();

	fs_init();

	main_checkerr(firsttime);
	console_printf("[...] Load filesystem...&n");
	console_drawbuffer();

	if (!firsttime) fs_loadFlash();

	__com_prompt_active= true;
	
	__com_history= (char*)0x203FFF0-__com_history_word*(__com_history_size+1);
	
	__shell_activeproc= -1;

	console_clear();
	console_printf("[INF] Flash identification: 0x%h&n", fl_getid());
	screen_wait_vsync();
	console_drawbuffer();
	if (firsttime)
	{
		console_printf("&nFlash is not properly formatted,&nuse the \"fmt\" command to fix it.&n");
		console_drawbuffer();
	}
	console_newline();
	console_printf("Welcome to %s version %s&n",OS_NAME,OS_VERSION);
	execute_command("m");
	//execute_command("hello.qdos");
	console_drawbuffer();

	while(1)
	{
		//Main loop
		if (__com_prompt_active && __shell_activeproc == -1)
			command_prompt();
			
		if (keypad_check(key_L) && keypad_check(key_R) && pressed)
			go_console_keyboard(&execute_command);
		
		console_idle();		
		screen_wait_vsync();
		
		if (__shell_activeproc == -1)
		{
			//Command history browser
			if (keypad_check(key_up) && pressed && __com_history_index <= __com_history_last)
			{
				draw_set_color(__console_bgcol);
				draw_rectangle(__console_charw*strlen(__com_promptstr), __console_charh*__console_irow, SCREEN_WIDTH, __console_charh*(__console_irow+1), 0);
				
				kbstring= (char*)(__com_history+((__com_history_index)*__com_history_word));
				__com_history_index++;
				kbstring_len= strlen(kbstring);
				drawhistory();
			}
			
			if (keypad_check(key_down) && pressed && __com_history_index >= 0)
			{
				draw_set_color(__console_bgcol);
				draw_rectangle(__console_charw*strlen(__com_promptstr), __console_charh*__console_irow, SCREEN_WIDTH, __console_charh*(__console_irow+1), 0);
			
				if (__com_history_index)
				{
					__com_history_index--;
					kbstring= (char*)(__com_history+((__com_history_index)*__com_history_word));
				}
				else
					kbstring= "";
				kbstring_len= strlen(kbstring);
				drawhistory();
			}
		}
		
		if (keypad_check(key_start) && __shell_activeproc == -1 && pressed)
		{
			execute_command(kbstring);
			console_printf(__com_promptstr);
			console_drawbuffer();
		}

		// if (keypad_check(key_select) && __shell_activeproc == -1 && pressed)
		// {
		// 	kbstring= "y";
		// 	fcmd_fdisk__kint1();
		// }
	}
	
	console_printf("System halted! Please reboot to retry&n");
	console_drawbuffer();
}
