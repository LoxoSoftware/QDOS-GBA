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
#include "video.h"
#include "console.h"
#include "mallocw.h"
#include "command.h"
#include "keyboard.h"
#include "irq.h"
#include "syscall.h"
#include "tools.h"
#include "flashfs.h"

#define OS_NAME 		"QDOS"
#define OS_VERSION 		"0.0.10.2"

#define ROMFS_DOMAIN    romfs, romfs_sz/sizeof(file_t*)

ALIGN(16) const char save_type[16]= "FLASH1M_V420\0\0\0\0";

int main()
{
	display_init();
	console_init();
	console_colors(IVGA_BLACK, IVGA_GRAY, IVGA_RED);

	//Setup hardware interrupt for syscalls
	//	(sadly I can't use software interrupts)
	irqSet(IRQ_DMA0, isr_IRQReceiver);
	irqEnable(IRQ_DMA0);
	//Setup hardware interrupt for virtual keyboard
	//REG_KEYCNT= KEY_L | KEY_R | KEYIRQ_ENABLE | KEYIRQ_AND;
	//irqSet(IRQ_KEYPAD, isr_vkeyboard);
	//irqEnable(IRQ_KEYPAD);

	console_printf("\xde\xb3 SRAM check...&n");
	console_drawbuffer();

	bool firsttime= !fs_check();

	fs_init();

	main_checkerr(firsttime);
	console_printf("\xde\xb3 Load filesystem...&n");
	console_drawbuffer();

	if (!firsttime) fs_loadFlash();

	__com_prompt_active= true;

	//__com_history= (char*)0x203FFF0-__com_history_word*(__com_history_size+1);

	__shell_activeproc= -1;

	console_clear();
	console_printf("i\xb3 Flash identification: %p&n", fl_getid());
	screen_wait_vsync();
	console_drawbuffer();
	if (firsttime)
	{
		console_printf("&[93m&nFlash is not properly formatted,&nuse the \"fmt\" command to initialize it.&[39m&n");
		console_drawbuffer();
	}
	console_newline();
	console_printf("    &[96mWelcome to %s &[36mversion %s&[0m&n",OS_NAME,OS_VERSION);
	execute_command("m");
	console_drawbuffer();

	u16 kd, kh;

	while(__system_mainloop)
	{
		//Main loop

		if (__com_prompt_active && __shell_activeproc == -1)
			command_prompt();

		console_idle();
		screen_wait_vsync();
		scanKeys();
		kd= keysDown();
		kh= keysHeld();

		if ((kh&KEY_L) && (kh&KEY_R))
		{
			go_console_keyboard(execute_command);

			while (((kh&KEY_L) && (kh&KEY_R)))
			{
				VBlankIntrWait();
				scanKeys();
				kh= keysHeld();
			}
		}

		if (kd&KEY_START && __shell_activeproc == -1)
		{
			execute_command(kbstring);
			console_printf(__com_promptstr);
			console_drawbuffer();
		}
	}

	//Out of main thread? Initiate graceful shutdown

	REG_DISPCNT= MODE_3|BG2_ON|OBJ_ON|OBJ_1D_MAP;

	console_printf("&[0m&[2J&[6B           &[97mShutting down...&n&n");
	console_drawbuffer();
	fs_flush();

	//Wait for a bit...
	//This is not just for fun, but waiting a few seconds ensures that all data
	// is being saved to the flashcart's SD card in time to avoid corruption
	for (int i=0; i<120; i++)
		VBlankIntrWait();

	REG_DISPCNT= MODE_3|BG2_ON;

	console_printf("&[0m&[2J&[6B  &[93mIt's now safe to turn off your GBA.");
	console_drawbuffer();

	REG_IME= 0; //Turn off all interrupts

	while (1) ;
}

