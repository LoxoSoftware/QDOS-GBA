//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/video.h     Quick and dirty GBA graphics library                    //
// Copyright (C) 2020-2025  Lorenzo C. (aka LoxoSoftware)                   //
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

#ifndef __VIDEO_H
#define __VIDEO_H

#include <gba.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define c_aqua		RGB8(25,255,224)
#define c_black		RGB8(0,0,0)
#define c_blue		RGB8(0,0,255)
#define c_dkgray	RGB8(64,64,64)
#define c_fuchsia	RGB8(255,0,255)
#define c_gray		RGB8(128,128,128)
#define c_green		RGB8(0,128,0)
#define c_lime		RGB8(0,255,0)
#define c_ltgray	RGB8(196,196,196)
#define c_maroon	RGB8(128,32,8)
#define c_navy		RGB8(0,0,128)
#define c_olive		RGB8(128,128,0)
#define c_orange	RGB8(255,128,0)
#define c_purple	RGB8(128,0,128)
#define c_red		RGB8(255,0,0)
#define c_silver	RGB8(192,192,192)
#define c_teal		RGB8(0,128,128)
#define c_white		RGB8(255,255,255)
#define c_yellow	RGB8(255,255,0)

u16 __gmklib__pen_color= c_white;

#include                        "font_default.h"
#define __GMKLIB_FONT           font_defaultBitmap
#define __GMKLIB_FONT_MARGIN    6
#define __GMKLIB_FONT_CHWIDTH   8
#define __GMKLIB_FONT_CHHEIGHT  8
#define __GMKLIB_FONT_MAPW  	4	//Number of character in a line of the map (X^2)

void display_init()
{
    SetMode(MODE_3|BG2_ON|OBJ_ON|OBJ_1D_MAP);
	irqInit();
	irqEnable(IRQ_VBLANK);
}

#define screen_wait_vsync()     VBlankIntrWait()

#define draw_set_color(col)     __gmklib__pen_color= col

IWRAM_CODE ARM_CODE
void draw_point(int x, int y)
{
	if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT)
		((u16*)VRAM)[x+y*SCREEN_WIDTH]= __gmklib__pen_color;
}

IWRAM_CODE ARM_CODE
void draw_pixel(int x, int y, u16 col)
{
	if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT)
		((u16*)VRAM)[x+y*SCREEN_WIDTH]= col;
}

IWRAM_CODE ARM_CODE
u16 draw_getpixel(int x, int y)
{
	return ((u16*)VRAM)[x+y*SCREEN_WIDTH];
}

IWRAM_CODE ARM_CODE
void draw_clear(u16 col)
{
    for(int i=0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i+= 2)
        ((u16*)VRAM)[i]= col;
}

IWRAM_CODE ARM_CODE
void draw_rectangle(int x1, int y1, int x2, int y2, bool outl)
{
	for (int iy=0;iy<y2-y1;iy++)
	{
		for (int ix=0;ix<x2-x1;ix++)
		{
			if (outl)
			{
				if ((iy == 0 || iy == y2-y1-1)||(ix == 0 || ix == x2-x1-1))
					draw_point(x1+ix, y1+iy);
			} else
			{
				draw_point(x1+ix, y1+iy);
			}
		}
	}
}

IWRAM_CODE ARM_CODE
void draw_text(int x, int y, char* str, ...)
{
	int inlen= strlen(str);
	const int iix= x;
	va_list valist;
	va_start(valist, (int)str[0]);

	for(int i=0; i<inlen; i++)
    {
		if (i>0) x+= __GMKLIB_FONT_MARGIN;

		//Escape code parsing

		if (str[i] == '&' && str[i+1] == 'n') {y+= __GMKLIB_FONT_CHHEIGHT+1; x= iix-__GMKLIB_FONT_MARGIN; i++;} //else
		// if (str[i] == '%' && str[i+1] == 'd') {x+= __GMKLIB_FONT_MARGIN*draw_number(x,y,va_arg(valist, int)); i++;} else
		// if (str[i] == '%' && str[i+1] == 'h') {x+= __GMKLIB_FONT_MARGIN*draw_number_hex(x,y,va_arg(valist, int)); i++;}

		else
        {
			//Actual printing of characters

			for (int iy=0; iy<__GMKLIB_FONT_CHHEIGHT; iy++)
            {
                u8 fbyte= __GMKLIB_FONT[str[i]+((__GMKLIB_FONT_MAPW*__GMKLIB_FONT_MAPW)*iy)+
				(((__GMKLIB_FONT_MAPW*__GMKLIB_FONT_MAPW)*(__GMKLIB_FONT_CHHEIGHT-1))*(str[i]>>__GMKLIB_FONT_MAPW))];

				if (fbyte&0b00000001) draw_point(x+0, y+iy);
				if (fbyte&0b00000010) draw_point(x+1, y+iy);
				if (fbyte&0b00000100) draw_point(x+2, y+iy);
				if (fbyte&0b00001000) draw_point(x+3, y+iy);
				if (fbyte&0b00010000) draw_point(x+4, y+iy);
				if (fbyte&0b00100000) draw_point(x+5, y+iy);
				// if (fbyte&0b01000000) draw_point(x+6, y+iy);
				// if (fbyte&0b10000000) draw_point(x+7, y+iy);
			}
		}
	}
	va_end(valist);
}

IWRAM_CODE ARM_CODE
void draw_char(int x, int y, char ch)
{
	for (int iy=0; iy<__GMKLIB_FONT_CHHEIGHT; iy++)
    {
		u8 fbyte= __GMKLIB_FONT[ch+((__GMKLIB_FONT_MAPW*__GMKLIB_FONT_MAPW)*iy)+
		(((__GMKLIB_FONT_MAPW*__GMKLIB_FONT_MAPW)*(__GMKLIB_FONT_CHHEIGHT-1))*(ch>>__GMKLIB_FONT_MAPW))];

		if (fbyte&0b00000001) draw_point(x+0, y+iy);
		if (fbyte&0b00000010) draw_point(x+1, y+iy);
		if (fbyte&0b00000100) draw_point(x+2, y+iy);
		if (fbyte&0b00001000) draw_point(x+3, y+iy);
		if (fbyte&0b00010000) draw_point(x+4, y+iy);
		if (fbyte&0b00100000) draw_point(x+5, y+iy);
		// if (fbyte&0b01000000) draw_point(x+6, y+iy);
		// if (fbyte&0b10000000) draw_point(x+7, y+iy);
	}
}

#endif
