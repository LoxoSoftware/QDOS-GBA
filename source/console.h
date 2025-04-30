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

#define __console_print_maxch	2048

#define VGA_BLACK				RGB5(0,0,0)
#define VGA_DKRED				RGB5(24,0,0)
#define VGA_DKGREEN				RGB5(0,24,0)
#define VGA_DKYELLOW			RGB5(24,24,0)
#define VGA_DKBLUE				RGB5(0,0,24)
#define VGA_DKMAGENTA			RGB5(24,0,24)
#define VGA_DKAQUA				RGB5(0,24,24)
#define VGA_DKGRAY				RGB5(16,16,16)
#define VGA_GRAY				RGB5(24,24,24)
#define VGA_RED					RGB5(31,8,8)
#define VGA_GREEN				RGB5(8,31,8)
#define VGA_YELLOW				RGB5(31,31,8)
#define VGA_BLUE				RGB5(8,8,31)
#define VGA_MAGENTA				RGB5(31,8,31)
#define VGA_AQUA				RGB5(8,31,31)
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

#define __CONSOLE_CHFMT(ch)		(ch+((__console_fgcol&0x0F)<<12)+((__console_bgcol&0x0F)<<8))

#define __CONSOLE_BGCOL_DEFAULT	IVGA_BLACK
#define __CONSOLE_FGCOL_DEFAULT	IVGA_GRAY

const u16 vga_palette[16]=
{
	VGA_BLACK, VGA_DKRED, VGA_DKGREEN, VGA_DKYELLOW,
	VGA_DKBLUE, VGA_DKMAGENTA, VGA_DKAQUA, VGA_GRAY,
	VGA_DKGRAY, VGA_RED, VGA_GREEN, VGA_YELLOW,
	VGA_BLUE, VGA_MAGENTA, VGA_AQUA, VGA_WHITE
};

					//X					  Y
u16 __console_buffer[__console_charperw][__console_charperh];
char __console_stdout[512]; int __console_stdout_index= 0;

int __console_icolumn= 0;
int __console_irow= 0;
int __console_icolumn_saved= 0;
int __console_irow_saved= 0;
u8 __console_bgcol= __CONSOLE_BGCOL_DEFAULT;
u8 __console_fgcol= __CONSOLE_FGCOL_DEFAULT;
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
			__console_buffer[ix][iy]= __CONSOLE_CHFMT(' ');
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

void console_colors(u8 bgcol, u8 fgcol, u8 cursorcol)
{
	__console_bgcol= bgcol;
	__console_fgcol= fgcol;
	SPRITE_PALETTE[__console_cur_palind]= vga_palette[cursorcol];
}

IWRAM_CODE ARM_CODE
void console_drawbuffer()
{
	//Clear the stdout buffer
	__console_stdout_index= 0;
	memset(__console_stdout, '\0', sizeof(__console_stdout));

	for(int iy= 0; iy<__console_charperh; iy++)
	{
		for(int ix= 0; ix<__console_charperw; ix++)
		{
			u16 tch= __console_buffer[ix][iy];

			draw_set_color(vga_palette[(tch&0x0F00)>>8]);
			if (iy*__console_charh-__console_yoffset >= 0)
				draw_rectangle(ix*__console_charw, iy*__console_charh-__console_yoffset, (ix+1)*__console_charw, (iy+1)*__console_charh-__console_yoffset, 0);
			draw_set_color(vga_palette[(tch&0xF000)>>12]);
			if (iy*__console_charh-__console_yoffset >= 0)
				draw_char(ix*__console_charw, iy*__console_charh-__console_yoffset, tch&0x00FF);
		}
	}
}

void console_scrolldown()
{
	__console_icolumn= 0;

	for(int iy= 1; iy<=__console_charperh; iy++)
    {
		for(int ix= 0; ix<__console_charperw; ix++)
        {
			if (iy == __console_charperh)
				__console_buffer[ix][iy-1]= __CONSOLE_CHFMT(' ');
			else
				__console_buffer[ix][iy-1] = __console_buffer[ix][iy];
		}
	}

	//console_drawbuffer()
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

void console_newline()
{
	__console_irow++;
	__console_icolumn=0;

	if (__console_irow >= __console_charperh)
    {
		console_scrolldown();
		__console_irow= __console_charperh-1;
	}
}

int console_ansiiparse(char* str)
{
	const int escape_pfx_len= 2;

	//NOTE: str begins from after the prefix ('&[')
	//Returns length of the escape sequence

	int first_alpha= strspn(str, "0123456789");
	char* first_alpha_ptr= "\0";
	if (first_alpha)
		first_alpha_ptr= &str[first_alpha];

	///// Cursor commands /////

	if (str[0] == 'H')
	{
		__console_icolumn= 0;
		__console_irow= 0;
		goto __console_ansiparse_finish;
	}
	if (str[0] == 'M')
	{
		if (__console_irow>0)
			__console_irow--;
		goto __console_ansiparse_finish;
	}
	if (str[0] == 's')
	{
		__console_icolumn_saved= __console_icolumn;
		__console_irow_saved= __console_irow;
		goto __console_ansiparse_finish;
	}
	if (str[0] == 'u')
	{
		__console_icolumn= __console_icolumn_saved;
		__console_irow= __console_irow_saved;
		goto __console_ansiparse_finish;
	}
	if (*first_alpha_ptr == 'A')
	{
		*first_alpha_ptr= '\0';
		u8 n= strtol(str, NULL, 10);
		__console_irow-=n;
		goto __console_ansiparse_finish;
	}
	if (*first_alpha_ptr == 'B')
	{
		*first_alpha_ptr= '\0';
		u8 n= strtol(str, NULL, 10);
		__console_irow+=n;
		goto __console_ansiparse_finish;
	}
	if (*first_alpha_ptr == 'C')
	{
		*first_alpha_ptr= '\0';
		u8 n= strtol(str, NULL, 10);
		__console_icolumn+=n;
		goto __console_ansiparse_finish;
	}
	if (*first_alpha_ptr == 'D')
	{
		*first_alpha_ptr= '\0';
		u8 n= strtol(str, NULL, 10);
		__console_icolumn-=n;
		goto __console_ansiparse_finish;
	}

	///// Erase commands /////

	if (str[0] == 'K')
	{
		for (int ix=0; ix<__console_charperw; ix++)
			__console_buffer[ix][__console_irow]= __CONSOLE_CHFMT(' ');
		goto __console_ansiparse_finish;
	}
	if (str[0] == '2' && str[1] == 'J')
	{
		console_clear();
		goto __console_ansiparse_finish;
	}

	///// Color commands /////

	if (*first_alpha_ptr == 'm')
	{
		*first_alpha_ptr= '\0';
		u8 n= strtol(str, NULL, 10);

		if (n == 0)
			__console_bgcol= __CONSOLE_BGCOL_DEFAULT,
			__console_fgcol= __CONSOLE_FGCOL_DEFAULT;
		if (n == 39)
			__console_fgcol= __CONSOLE_FGCOL_DEFAULT;
		if (n == 49)
			__console_bgcol= __CONSOLE_BGCOL_DEFAULT;

		//Dark colors
		if (n>=30 && n<=37)
			__console_fgcol= n-30;
		if (n>=40 && n<=47)
			__console_bgcol= n-40;
		//Bright colors
		if (n>=90 && n<=97)
			__console_fgcol= n-90+8;
		if (n>=100 && n<=107)
			__console_bgcol= n-100+8;

		goto __console_ansiparse_finish;
	}

	return 0;

__console_ansiparse_finish:
	return escape_pfx_len+first_alpha;
}

IWRAM_CODE ARM_CODE
void console_printc(char ch)
{
	if (__console_silent)
		return;

	if (__console_icolumn >= __console_charperw)
	{
		__console_icolumn= 0;
		__console_irow++;
	}

	if (__console_irow >= __console_charperh)
	{
		console_scrolldown();
		__console_irow= __console_charperh-1;
	}

	__console_buffer[__console_icolumn][__console_irow]= __CONSOLE_CHFMT(ch);

	if (__console_stdout_index < sizeof(__console_stdout))
	{
		__console_stdout[__console_stdout_index]= ch;
		__console_stdout_index++;
	}

	__console_icolumn++;
}

void console_prints(char* str)
{
	if (__console_silent)
		return;

	for (int c=0; str[c] && c<__console_print_maxch; c++)
		console_printc(str[c]);
}

void console_printf(char* str, ...)
{
	va_list valist;
	va_start(valist, (int)str[0]);

	char tfmtstr[16];
	char tescstr[8];

	for (int i=0; str[i] && i<__console_print_maxch; i++)
    {
		if (str[i] == '&')
		{
			switch(str[i+1])
			{
				case '&':
					console_printc('&');
					i++;
					break;
				case '%':
					console_printc('%');
					i++;
					break;
				case 'n':
					console_newline();
					i++;
					break;
				case '[':
					i+= console_ansiiparse(&str[i+2]);
					break;
				case '\0':
					console_prints("\\0");
					break;
				default:
					break;
			}
		}
		else if (str[i] == '%')
		{
			memcpy(tescstr, &str[i], sizeof(tescstr));
			int tescstr_len= strspn(tescstr,"0123456789")+1;
			if (tescstr_len > sizeof(tescstr))
				tescstr_len= sizeof(tescstr)-1;
			tescstr[tescstr_len+1]= '\0';
			snprintf(tfmtstr, sizeof(tfmtstr), tescstr, va_arg(valist,int));
			console_prints(tfmtstr);
			i+=tescstr_len;
		}
		else
			console_printc(str[i]);
	}

	va_end(valist);
}

#endif
