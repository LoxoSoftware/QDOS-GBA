//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/console.h   Console video renderer for bitmap mode                  //
// Copyright (C) 2021-2025  Lorenzo C. (aka LoxoSoftware)                   //
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

#ifndef CONSOLE_H
#define CONSOLE_H

#include "console_sprites.h"

#define __console_charw         6
#define __console_charh         8
#define __console_charperw      40 //(SCREEN_WIDTH/__console_charw)
#define __console_charperh      20 //(SCREEN_HEIGHT/__console_charh)
#define __console_cur_palind	255
#define __console_cur_sprind	127

#define VGA_BLACK				RGB5(0,0,0)
#define VGA_DKRED				RGB5(16,0,0)
#define VGA_DKGREEN				RGB5(0,16,0)
#define VGA_DKYELLOW			RGB5(16,16,0)
#define VGA_DKBLUE				RGB5(0,0,16)
#define VGA_DKMAGENTA			RGB5(16,0,16)
#define VGA_DKAQUA				RGB5(0,16,16)
#define VGA_DKGRAY				RGB5(16,16,16)
#define VGA_GRAY				RGB5(24,24,24)
#define VGA_RED					RGB5(31,0,0)
#define VGA_GREEN				RGB5(0,31,0)
#define VGA_YELLOW				RGB5(31,31,0)
#define VGA_BLUE				RGB5(0,0,31)
#define VGA_MAGENTA				RGB5(31,0,31)
#define VGA_AQUA				RGB5(0,31,31)
#define VGA_WHITE				RGB5(31,31,31)
#define IVGA_BLACK				0
#define IVGA_DKRED				1
#define IVGA_DKGREEN			2
#define IVGA_DKYELLOW			3
#define IVGA_DKBLUE				4
#define IVGA_DKMAGENTA			5
#define IVGA_DKAQUA				6
#define IVGA_DKGRAY				8
#define IVGA_GRAY				7
#define IVGA_RED				9
#define IVGA_GREEN				10
#define IVGA_YELLOW				11
#define IVGA_BLUE				12
#define IVGA_MAGENTA			13
#define IVGA_AQUA				14
#define IVGA_WHITE				15

const u16 vga_palette[16]=
{
	VGA_BLACK, VGA_DKRED, VGA_DKGREEN, VGA_DKYELLOW,
	VGA_DKBLUE, VGA_DKMAGENTA, VGA_DKAQUA, VGA_GRAY,
	VGA_DKGRAY, VGA_RED, VGA_GREEN, VGA_YELLOW,
	VGA_BLUE, VGA_MAGENTA, VGA_AQUA, VGA_WHITE
};

					//X					  Y
u8 __console_buffer[__console_charperw][__console_charperh];
char __console_stdout[512]; int __console_stdout_index= 0;

int __console_icolumn= 0;
int __console_irow= 0;
u16 __console_bgcol= VGA_BLACK;
u16 __console_fgcol= VGA_GRAY;
bool __console_silent= false;
u32 __console_frames= 0;
bool __console_initialized= false;
int __console_yoffset= 0;
int __console_curtoggle= 0;

void console_idle()
{
	//Run every frame

	OAM[__console_cur_sprind].attr0= ATTR0_COLOR_16|OBJ_Y((__console_irow*__console_charh)-__console_yoffset),
	OAM[__console_cur_sprind].attr1= OBJ_X(__console_icolumn*__console_charw),
	OAM[__console_cur_sprind].attr2= OBJ_CHAR(0x200+((__console_frames&32)>>5))|ATTR2_PALETTE(__console_cur_palind>>4);

	__console_frames--;
}

void console_clear()
{
	//Clear the stdout buffer
	__console_stdout_index= 0;
	memset(__console_stdout, '\0', sizeof(__console_stdout));

	__console_irow= 0;
	__console_icolumn= 0;

	for(int iy= 0; iy<__console_charperh; iy++)
    {
		for(int ix= 0; ix<__console_charperw; ix++)
        {
			__console_buffer[ix][iy]= ' ';
		}
	}
}

void console_init()
{
	if (__console_initialized)
		return;
	REG_DISPCNT= MODE_3|BG2_ON|OBJ_ON|OBJ_1D_MAP;
	CpuFastSet(cnssprTiles, BITMAP_OBJ_BASE_ADR, cnssprTilesLen/4);
	SPRITE_PALETTE[__console_cur_palind]= VGA_RED;
	irqInit();
	irqSet(IRQ_VBLANK, console_idle);
	irqEnable(IRQ_VBLANK);
	console_clear();
	__console_initialized= true;
}

void console_colors(u16 bgcol, u16 fgcol, u16 cursorcol)
{
	__console_bgcol= bgcol;
	__console_fgcol= fgcol;
	SPRITE_PALETTE[1]= cursorcol;
}

IWRAM_CODE ARM_CODE
void console_drawbuffer()
{
	//Clear the stdout buffer
	__console_stdout_index= 0;
	memset(__console_stdout, '\0', sizeof(__console_stdout));

	for(int iy= 0; iy<__console_charperh; iy++) {

		for(int ix= 0; ix<__console_charperw; ix++) {

			draw_set_color(__console_bgcol);
			if (iy*__console_charh-__console_yoffset >= 0)
			draw_rectangle(ix*__console_charw, iy*__console_charh-__console_yoffset, (ix+1)*__console_charw, (iy+1)*__console_charh-__console_yoffset, 0);
			draw_set_color(__console_fgcol);
			if (iy*__console_charh-__console_yoffset >= 0)
			draw_char(ix*__console_charw, iy*__console_charh-__console_yoffset, __console_buffer[ix][iy]);
		}
	}
}

void console_scrolldown()
{
	__console_icolumn= 0;

	for(int iy= 1; iy<__console_charperh; iy++)
    {
		for(int ix= 0; ix<__console_charperw; ix++)
        {
			if (iy == __console_charperh-1)
				__console_buffer[ix][iy-1]= 0;
			else
				__console_buffer[ix][iy-1] = __console_buffer[ix][iy];
		}
	}

	//console_drawbuffer()
}

void console_newline()
{
	__console_irow++;
	__console_icolumn=0;

	if (__console_irow >= __console_charperh-1)
    {
		console_scrolldown();
		__console_irow= __console_charperh-2;
	}
}

void console_print_number(int num)
{
	short inlen= 9;
	unsigned int na[inlen];

	if (num == 0)
    {
		__console_buffer[__console_icolumn][__console_irow]= '0';
		return;

	} else if (num > 0)
    {
		//Number to digits
		for(int i=0; i<=inlen;i++)
        {
        	if (num < 1) { inlen= i; break; }
			na[i]= num%10;
        	num/=10;
    	}

	} else
    {
		//Number to digits (negative)
		//na[0]= 69; //Minus sign instead of the digit
		num *= -1;
		for(int i=0; i<=inlen;i++)
        {
        	if (num < 1) { na[i]=69; inlen= i+1; break; }
			na[i]= num%10;
        	num/=10;
    	}
	}

	for (int i=inlen-1; i>=0; i--)
    {
        //Print the digits on to the buffer
		if (!__console_silent)
		if (__console_icolumn >= __console_charperw)
        {
			__console_icolumn= 0;
			__console_irow++;
		}

		if (!__console_silent)
		if (__console_irow >= __console_charperh-1)
        {
			console_scrolldown();
			__console_irow= __console_charperh-2;
		}

		if (na[i] != 69)
        {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= na[i]+'0';
			if (__console_stdout_index < sizeof(__console_stdout))
            {
				__console_stdout[__console_stdout_index]= na[i]+'0';
				__console_stdout_index++;
			}
		}
		else {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= '-';
			if (__console_stdout_index < sizeof(__console_stdout))
            {
				__console_stdout[__console_stdout_index]= '-';
				__console_stdout_index++;
			}
		}

		__console_icolumn++;
	}

	__console_icolumn--;
}

void console_print_number_hex(int num, int digits)
{
	//if 'digits' is -1, the length is automatic
	short inlen= 9;
	unsigned int na[inlen];

	if (num == 0)
    {
		__console_buffer[__console_icolumn][__console_irow]= '0';
		return;

	} else if (num > 0)
    {
		//Number to digits
		for(int i=0; i<=inlen;i++)
        {
        	if (num < 1) { inlen= i; break; }
			na[i]= num%16;
        	num/=16;
    	}
	} else return;

	for (int i=inlen-1; i>=0; i--)
    {
        //Print the digits on to the buffer

		if (!__console_silent)
		if (__console_icolumn >= __console_charperw)
        {
			__console_icolumn= 0;
			__console_irow++;
		}

		if (!__console_silent)
		if (__console_irow >= __console_charperh-1)
        {
			console_scrolldown();
			__console_irow= __console_charperh-2;
		}

		if (na[i] < 10)
        {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= na[i]+'0';
			if (__console_stdout_index < sizeof(__console_stdout))
            {
				__console_stdout[__console_stdout_index]= na[i]+'0';
				__console_stdout_index++;
			}
		}
		else
        {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= na[i]+'a'-10;
			if (__console_stdout_index < sizeof(__console_stdout))
            {
				__console_stdout[__console_stdout_index]= na[i]+'a'-10;
				__console_stdout_index++;
			}
		}

		__console_icolumn++;
	}

	__console_icolumn--;
}

void console_printf(char* str, ...)
{
	va_list valist;
	va_start(valist, (int)str[0]);

	int tsz= 0;

	while(str[tsz] != '\0')
		tsz++;

	int c= 0;

	while(tsz)
    {
		if (!__console_silent)
		if (__console_icolumn >= __console_charperw)
        {
			__console_icolumn= 0;
			__console_irow++;
		}

		if (!__console_silent)
		if (__console_irow >= __console_charperh-1)
        {
			console_scrolldown();
			__console_irow= __console_charperh-2;
		}

		if (str[c] == '&' && str[c+1] == 'n') {c++; tsz--; if (!__console_silent) console_newline(); if (!__console_silent) __console_icolumn--;}
		else
		if (str[c] == '%' && str[c+1] == 's') {c++; tsz--; console_printf(va_arg(valist, char*)); if (!__console_silent) __console_icolumn--;}
		else
		if (str[c] == '%' && str[c+1] == 'd') {c++; tsz--; console_print_number(va_arg(valist, int));}
		else
		if (str[c] == '%' && str[c+1] == 'h') {c++; tsz--; console_print_number_hex(va_arg(valist, int),-1);}
		else
        {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= str[c];
			if (__console_stdout_index < sizeof(__console_stdout)) {
				__console_stdout[__console_stdout_index]= str[c];
				__console_stdout_index++;
			}
		}

		if (!__console_silent) __console_icolumn++;
		c++;
		tsz--;
	}

	va_end(valist);
}

void console_prints(char* str)
{
	int tsz= 0;

	while(str[tsz] != '\0')
		tsz++;

	int c= 0;

	while(tsz) {

		if (!__console_silent)
		if (__console_icolumn >= __console_charperw)
        {
			__console_icolumn= 0;
			__console_irow++;
		}

		if (!__console_silent)
		if (__console_irow >= __console_charperh-1)
        {
			console_scrolldown();
			__console_irow= __console_charperh-2;
		}

		if (str[c] == '&' && str[c+1] == 'n') {c++; tsz--; if (!__console_silent) console_newline(); if (!__console_silent) __console_icolumn--;}
		else
        {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= str[c];
			if (__console_stdout_index < sizeof(__console_stdout)) {
				__console_stdout[__console_stdout_index]= str[c];
				__console_stdout_index++;
			}
		}

		if (!__console_silent) __console_icolumn++;
		c++;
		tsz--;
	}
}

void console_moveback(int n)
{
	while(n)
    {
		if (__console_icolumn < 0)
        {
			__console_icolumn= __console_charperw-1;
			__console_irow--;
		}

		__console_icolumn--;
		n--;
	}
}

void console_moveon(int n)
{
	while(n)
    {
		if (__console_icolumn >= __console_charperw)
        {
			__console_icolumn= 0;
			__console_irow++;
		}

		__console_icolumn++;
		n--;
	}
}

void console_redrawrow(int row, char* str)
{
	int prevrow= __console_irow;
	int prevcol= __console_icolumn;
	__console_irow= row;
	__console_icolumn= 0;

	//for (int ix=0; ix<__console_charperw; ix++)
		//__console_buffer[ix][__console_irow]= 0;

	console_printf(str);

	__console_irow= prevrow;
	__console_icolumn= prevcol;
}

#endif
