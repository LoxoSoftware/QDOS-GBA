//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/gmklib.h    Quick and dirty GBA graphics library                    //
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

//NOTE: To be deprecated in the future

#ifndef GMKLIB_H
#define GMKLIB_H

#include <gba.h>
#include <string.h>
#include <stdarg.h>

#define no_img			(u16*)0
#define pressed			key_this != key_last
#define released		key_this == key_last
#define deg				57.29
#define Pi				3.1415

#define tile_mode		0
#define bitmap_mode		3

#define sram(m)			((u8*)SRAM)[m]

#define FONT_MAPADDRESS	MAP_BASE_ADR(31)

#ifndef ARM_CODE
#define ARM_CODE	__attribute__((target("arm")))
#endif
#ifndef THUMB_CODE
#define THUMB_CODE	__attribute__((target("thumb")))
#endif

int vmode=0;
bool gfxset= 0, fntset= 0;
OBJATTR* sprattr[128];

int dcol= 0x7FFF;
int ddel= 0;
bool ard= 0;
bool ild= 0; bool iloff= 0;

u16 alarm[12];

typedef u32 Tile[16];
typedef Tile TileBlock[256];
typedef u16 ScreenBlock[1024];
#define MEM_PALETTE_SPR   	(u16*)0x05000200
#define MEM_TILE_BG         ((TileBlock*)VRAM)
#define MEM_SCREENBLOCKS    ((ScreenBlock*)VRAM)
#define MEM_TILE_SPR_BMP 	((TileBlock*)BITMAP_OBJ_BASE_ADR)
#define MEM_TILE_SPR_TL 	((TileBlock*)OBJ_BASE_ADR)

#define _fntmargin 6

void display_init(int mode) {
	fntset= 0;
	if (!gfxset) {
		irqInit();
		irqEnable(IRQ_VBLANK);
		for (int i = 0; i < 128; i++) {
			sprattr[i] = &OAM[i];
			sprattr[i]->attr2 = OBJ_CHAR(-1);
		}
		for (int i= 0; i<4; i++)
			BGCTRL[i] = BG_16_COLOR | BG_SIZE(0) | BG_PRIORITY(4) | SCREEN_BASE(30) | CHAR_BASE(8);
	}
	if (mode == tile_mode)
		SetMode(MODE_0|BG0_ENABLE|BG1_ENABLE|BG2_ENABLE|BG3_ENABLE|OBJ_ENABLE|0x0040);
	if (mode == bitmap_mode)
		SetMode(MODE_3|BG2_ENABLE|OBJ_ENABLE|0x0040);
	vmode= mode;
	gfxset=1;
}

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

bool Llk[10];
int key_last, key_this;
void screen_wait_vsync() {
	for (int i=0; i < 12; i++) {
		if (alarm[i] > 0) alarm[i]--;
	}
	for (int i=0; i < 10; i++) {
		Llk[i]= 0;
	}
	key_last= REG_KEYINPUT;
	VBlankIntrWait();
	key_this= REG_KEYINPUT;
}

void alarm_reset_all() {
	for (int i=0; i < 12; i++) {
		alarm[i]= 0;
	}
}

void set_automatic_draw(bool en) { ard= en; }

void draw_set_interlaced(bool en) { ild= en; }

void draw_set_color(int col) { dcol= col; }

void draw_set_delay(int del) { ddel= del; }

void sleep(int numb) {
	for(int si=0; si < numb; si++) VBlankIntrWait();
}

IWRAM_CODE ARM_CODE void draw_clear(int col) {
	if (!ild) {
		for(int di=0; di < SCREEN_WIDTH*SCREEN_HEIGHT; di++) {
			((u16*)VRAM)[di]= col;
		}
	} else {
		for(int di=iloff; di < SCREEN_WIDTH*SCREEN_HEIGHT; di+= 2) {
			((u16*)VRAM)[di]= col;
		}
		iloff= !iloff;
	}
}

void draw_point(int x, int y) {
	if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT)
		((u16*)VRAM)[x+y*SCREEN_WIDTH]= dcol;
}

void draw_pixel(int x, int y, int col) {
	if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT)
		((u16*)VRAM)[x+y*SCREEN_WIDTH]= col;
}

 int draw_getpixel(int x, int y) {
	return ((u16*)VRAM)[x+y*SCREEN_WIDTH];
}

IWRAM_CODE ARM_CODE void draw_line(int x1, int y1, int x2, int y2) {
	float x,y,dx,dy,step;
	int i;
 
	dx=(x2-x1);
	dy=(y2-y1);
 
 	/*
	if(dx>=dy)
		step=dx;
	else
		step=dy;
 	*/
 	
 	step=32;
 
	dx=dx/step;
	dy=dy/step;
 
	x=x1;
	y=y1;
 
	i=1;
	while(i<=step)
	{
		draw_point(x,y);
		x=x+dx;
		y=y+dy;
		i=i+1;
		sleep(ddel);
	}
}

IWRAM_CODE ARM_CODE void draw_rectangle(int x1, int y1, int x2, int y2, bool outl) {
	for (int iy=0;iy<y2-y1;iy++) {
		for (int ix=0;ix<x2-x1;ix++) {
			if (outl) {
				if ((iy == 0 || iy == y2-y1-1)||(ix == 0 || ix == x2-x1-1))
					draw_point(x1+ix, y1+iy);
					//((u16*)VRAM)[x1+(y1*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= dcol;
			} else {
				draw_point(x1+ix, y1+iy);
				//((u16*)VRAM)[x1+(y1*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= dcol;
			}
		}
		sleep(ddel);
	}
}

void draw_line_width(int x1, int y1, int x2, int y2, int w) {
	float x,y,dx,dy,step;
	int i;
 
	dx=(x2-x1);
	dy=(y2-y1);
 
	if(dx>=dy)
		step=dx;
	else
		step=dy;
 
	dx=dx/step;
	dy=dy/step;
 
	x=x1;
	y=y1;
 
	i=1;
	while(i<=step)
	{
		draw_rectangle(x,y,x+w,y+w,0);
		x=x+dx;
		y=y+dy;
		i=i+1;
	}
}

void draw_circle(int x0, int y0, int radius, bool outl) {
    int x = radius;
    int y = 0;
    int err = 0;
	int pddel= ddel;
 
	for(int xi= radius; xi>=0; xi--) {
		while (x >= y)
		{
			if (xi == radius) {
				draw_point(x0 + x, y0 + y);
				draw_point(x0 + y, y0 + x);
				draw_point(x0 - y, y0 + x);
				draw_point(x0 - x, y0 + y);
				draw_point(x0 - x, y0 - y);
				draw_point(x0 - y, y0 - x);
				draw_point(x0 + y, y0 - x);
				draw_point(x0 + x, y0 - y);
			} else {
				draw_set_delay(0); sleep(pddel);
				draw_rectangle(x0 + x, y0 + y, x0 + x+2, y0 + y+2, 0);
				draw_rectangle(x0 + y, y0 + x, x0 + y+2, y0 + x+2, 0);
				draw_rectangle(x0 - y, y0 + x, x0 - y+2, y0 + x+2, 0);
				draw_rectangle(x0 - x, y0 + y, x0 - x+2, y0 + y+2, 0);
				draw_rectangle(x0 - x, y0 - y, x0 - x+2, y0 - y+2, 0);
				draw_rectangle(x0 - y, y0 - x, x0 - y+2, y0 - x+2, 0);
				draw_rectangle(x0 + y, y0 - x, x0 + y+2, y0 - x+2, 0);
				draw_rectangle(x0 + x, y0 - y, x0 + x+2, y0 - y+2, 0);
			}
		 
			if (err <= 0)
			{
				y += 1;
				err += 2*y + 1;
			}
		 
			if (err > 0)
			{
				x -= 1;
				err -= 2*x + 1;
			}
			draw_set_delay(pddel);
			if (outl) sleep(ddel);
		}
		if (outl) break; else { 
			x= xi; y=0;
		}
	}
}

void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, bool outl) {
	draw_line(x1, y1, x2, y2);
	draw_line(x2, y2, x3, y3);
	draw_line(x3, y3, x1, y1);
	if (!outl) {
		int pddel= ddel;
		ddel= 0;
		for(int x = x1; x<=x2; x++) {
			for(int y = y1; y<=y2; y++) {
				draw_line(x3, y3, x, y);
			}	
		}
		ddel= pddel;
	}
}

void draw_healthbar(int x1, int y1, int x2, int y2, int amount, int backcol, int barcol, bool showback, bool showborder) {
	int sdcol= dcol;
	int res= x1+(((float)amount/100)*(x2-x1));
	dcol= backcol;
	if (showback) draw_rectangle(res,y1,x2,y2,0);
	dcol= barcol;
	draw_rectangle(x1,y1,res,y2,0);
	dcol= 0x7FFF;
	if (showborder) draw_rectangle(x1-1,y1-1,x2+1,y2+1,1);
	dcol= sdcol;
}

 int make_color_rgb(int r, int g, int b) {
	return RGB8(r,g,b);
}

 int make_color_hsv(int h, int s, int v) {
	typedef struct RgbColor{
		unsigned char r;
		unsigned char g;
		unsigned char b;
	} RgbColor;
	RgbColor rgb;
    unsigned char region, remainder, p, q, t;

    if (s == 0)
    {
        rgb.r = v;
        rgb.g = v;
        rgb.b = v;
        return RGB8(rgb.r,rgb.g,rgb.b);
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6; 

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb.r = v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = v;
            break;
        default:
            rgb.r = v; rgb.g = p; rgb.b = q;
            break;
    }

    return RGB8(rgb.r,rgb.g,rgb.b); 
}

bool rev2=false;
bool lf2=true;
int loop(int n, int A, int B, int inc) {
	//int rev= 1;
	if (lf2) {n=A; lf2=false;}
	if (!rev2 && n < B-1)  return n+=inc;
	if (!rev2 && n == B-1) rev2=true;
	if (rev2 && n > A) return n-=inc;
	if (rev2 && n==A) rev2=false;//else rev=1;
	return 0;
}

// TEXT STUFFS //

const bool img_font_terminal8[]= {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,1,1,1,0,0,0,0,0,1,1, 1,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,0,0,0, 0,0,0,1,1,0,1,1,0,0,0,1,1,0, 1,1,0,0,0,1,0,0,1,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,1,0,0,0,0,0,1, 0,1,0,0,0,0,1,1,1,1,1,0,0,0,
	0,1,0,1,0,0,0,0,0,1,0,1,0,0, 0,0,1,1,1,1,1,0,0,0,0,1,0,1, 0,0,0,0,0,0,1,0,0,0,0,0,0,0, 1,1,1,0,0,0,0,1,0,0,0,0,0,0, 0,0,1,1,0,0,0,0,0,0,0,0,1,0, 0,0,0,1,1,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,0,0,0,0,0,0,1, 1,0,0,1,0,0,0,1,1,0,0,1,0,0,
	0,0,0,0,1,0,0,0,0,0,0,1,0,0, 0,0,0,0,1,0,0,0,0,0,0,1,0,0, 1,1,0,0,0,1,0,0,1,1,0,0,0,0, 0,0,0,0,0,0,0,0,1,0,0,0,0,0, 0,1,0,1,0,0,0,0,0,1,0,1,0,0, 0,0,0,0,1,0,0,0,0,0,0,1,0,1, 0,1,0,0,0,1,0,0,1,0,0,0,0,0, 1,1,0,1,0,0,0,0,0,0,0,0,0,0,
	0,0,1,1,0,0,0,0,0,0,1,1,0,0, 0,0,0,0,1,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,1,0,0,0,0,0,0,0,1,0, 0,0,0,0,0,0,1,0,0,0,0,0,0,0, 1,0,0,0,0,0,0,0,1,0,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,1,0,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,1,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,1,0,1,0,0,0,0,0, 1,1,1,0,0,0,0,1,1,1,1,1,0,0,
	0,0,1,1,1,0,0,0,0,0,1,0,1,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0, 0,1,1,1,1,1,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,1,1, 0,0,0,0,0,0,1,1,0,0,0,0,0,0, 1,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,1,1,1,1,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,1,0,0,0,0,0,0,1,1,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 1,0,0,0,0,0,0,1,0,0,0,0,0,0, 1,0,0,0,0,0,0,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,1,1,1,0,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,1,1,0,0,0,1, 0,1,0,1,0,0,0,1,1,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,0,1,1,1,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,1,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,1,1,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,1,1,1,0,0,0,0,1, 0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,1,1,0,0,0,0,0,1,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,1,1,1, 1,1,0,0,0,0,0,0,0,0,0,0,0,0, 1,1,1,0,0,0,0,1,0,0,0,1,0,0,
	0,0,0,0,0,1,0,0,0,0,1,1,1,0, 0,0,0,0,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,0,1,1,1,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,1,0,0,0, 0,0,0,1,1,0,0,0,0,0,1,0,1,0, 0,0,0,1,0,0,1,0,0,0,0,1,1,1, 1,1,0,0,0,0,0,0,1,0,0,0,0,0, 0,0,1,0,0,0,0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,0,0,0,1,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,1,1,1, 1,0,0,0,0,0,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,0,1,1,1,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,1,1,0, 0,0,0,0,1,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,1,1,1,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0,
	0,0,1,1,1,0,0,0,0,0,0,0,0,0, 0,0,0,1,1,1,1,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,1,0,0,0,0,0, 0,1,0,0,0,0,0,0,1,0,0,0,0,0, 0,0,1,0,0,0,0,0,0,0,1,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,1,1, 1,0,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,0,1,1,1,0,0,0,
	0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,0,1,1,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,1,1,1,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,0,1,1,1,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,1,0,0,0,0,0,1,1, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,1,1,0,0,0,0,0,0,1,1,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,1,1, 0,0,0,0,0,0,1,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,1,1,0,0, 0,0,0,0,1,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,1,1,0,0,0,0,0,0, 1,1,0,0,0,0,0,0,1,0,0,0,0,0,
	0,0,0,0,1,0,0,0,0,0,0,1,0,0, 0,0,0,0,1,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,0,1,0,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,0,1,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,1,1,1, 1,1,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,1,0,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,0,1,0,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,1,0,0,0, 0,0,0,1,0,0,0,0,0,0,1,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,1,1, 1,0,0,0,0,1,0,0,0,1,0,0,0,0, 0,0,0,1,0,0,0,0,0,1,1,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,1,1,1,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,1,1,1,0,0, 0,1,0,1,0,1,0,0,0,1,0,1,1,1, 0,0,0,1,0,0,0,0,0,0,0,0,1,1, 1,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,1,1,0,0,0,0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,1,1,1,1,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,0, 0,0,0,0,0,0,0,1,1,1,1,0,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,1,1,1,0,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,1, 1,1,1,0,0,0,0,0,0,0,0,0,0,0,
	0,0,1,1,1,0,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,1,0,0,0,0,1,1,1,0,0,0, 0,0,0,0,0,0,0,0,0,1,1,1,1,0, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0,
	0,1,1,1,1,0,0,0,0,0,0,0,0,0, 0,0,0,1,1,1,1,1,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 1,1,1,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,1,1,1,1, 0,0,0,0,0,0,0,0,0,0,0,1,1,1, 1,1,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,1,1,1,0,0,0,
	0,1,0,0,0,0,0,0,0,1,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,1,1,1,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,0,0,0, 0,1,0,1,1,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,0,1,1, 1,1,0,0,0,0,0,0,0,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,0,1,1,1,1,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,0, 0,0,0,0,0,0,0,0,1,1,1,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 1,1,1,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,0,1,1,1,0,0,0, 0,0,0,0,0,0,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,1,0,0,0,0,1,0,1, 0,0,0,0,0,1,1,0,0,0,0,0,0,1, 0,1,0,0,0,0,0,1,0,0,1,0,0,0,
	0,1,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,1,1,1,1, 0,0,0,0,0,0,0,0,0,0,0,1,0,0, 0,1,0,0,0,1,1,0,1,1,0,0,0,1, 0,1,0,1,0,0,0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,0,0,0, 0,0,0,0,0,1,0,0,0,1,0,0,0,1, 1,0,0,1,0,0,0,1,0,1,0,1,0,0, 0,1,0,0,1,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,0,0,0,0,0,0,0,0,0, 1,1,1,0,0,0,0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,0,1,1,1,0,0,0,0,0, 0,0,0,0,0,0,0,1,1,1,1,0,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,1,1,1,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,1,1,1,0,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,1,0,1,0,1,0,0,0,1, 0,0,1,0,0,0,0,0,1,1,0,1,0,0, 0,0,0,0,0,0,0,0,0,1,1,1,1,0, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,1,1,1,1,0,0,0,0,1, 0,0,1,0,0,0,0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,0,1,1,1,0,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,0,0,0,0,0, 1,1,1,0,0,0,0,0,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,0,1,1,1,0, 0,0,0,0,0,0,0,0,0,0,0,1,1,1, 1,1,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,0,1,1, 1,0,0,0,0,0,0,0,0,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,0,1,0, 1,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,1,0,1,0,1, 0,0,0,1,0,1,0,1,0,0,0,1,0,1, 0,1,0,0,0,1,0,1,0,1,0,0,0,0, 1,0,1,0,0,0,0,0,0,0,0,0,0,0,
	0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,0,1,0,1,0,0,0,0,0,0,1, 0,0,0,0,0,0,1,0,1,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,0,0,0,0,0,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,0,1,0,1,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,0,0,0, 0,0,0,1,1,1,1,0,0,0,0,0,0,0, 1,0,0,0,0,0,0,1,0,0,0,0,0,0, 1,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,1,1,1,0, 0,0,0,0,0,0,0,0,0,0,0,0,1,1, 1,0,0,0,0,0,1,0,0,0,0,0,0,0, 1,0,0,0,0,0,0,0,1,0,0,0,0,0,
	0,0,1,0,0,0,0,0,0,0,1,0,0,0, 0,0,0,0,1,1,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,0,1,0,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,0,1,0, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,1,1,0,0,0,0,0,0,0,1,0,0,0,
	0,0,0,0,1,0,0,0,0,0,0,0,1,0, 0,0,0,0,0,0,1,0,0,0,0,0,0,0, 1,0,0,0,0,0,1,1,1,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,1,0,0,0,0, 0,0,1,0,1,0,0,0,0,1,0,0,0,1, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,1,1,1,1,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,1,1,1,0,0,0,0,0, 0,0,0,1,0,0,0,0,1,1,1,1,0,0, 0,1,0,0,0,1,0,0,0,0,1,1,1,1, 0,0,0,0,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 1,1,1,0,0,0,0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,1,1,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,1,1,1,0,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,0, 0,0,0,1,0,0,0,1,0,0,0,0,1,1, 1,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,1,0,0,
	0,0,1,1,1,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,0,1,1,1,1,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,1,1,1,0, 0,0,0,1,0,0,0,1,0,0,0,1,1,1, 1,0,0,0,0,1,0,0,0,0,0,0,0,0, 1,1,1,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,1,1,0,0,0,0,0,1,0,0,0, 0,0,0,0,1,0,0,0,0,0,0,1,1,1, 1,0,0,0,0,0,1,0,0,0,0,0,0,0, 1,0,0,0,0,0,0,0,1,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,1,1, 1,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,0,1,1,1,1,0,0,
	0,0,0,0,0,1,0,0,0,0,1,1,1,0, 0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,1,1,0,0,0,0,0,1, 0,0,1,0,0,0,0,1,0,0,1,0,0,0, 0,1,0,0,1,0,0,0,0,1,0,0,1,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,1,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,1,1,0,0,0, 0,0,0,0,1,0,0,0,0,0,0,0,1,0, 0,0,0,0,0,0,1,0,0,0,0,1,0,0, 1,0,0,0,0,0,1,1,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0,0,0,0,0,
	0,1,0,0,1,0,0,0,0,1,0,1,0,0, 0,0,0,1,1,0,0,0,0,0,0,1,0,1, 0,0,0,0,0,1,0,0,1,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,1,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,1,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,1,1,0,1,0,0,0,0,1,0,1, 0,1,0,0,0,1,0,1,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,1,1,1, 0,0,0,0,0,1,0,0,1,0,0,0,0,1, 0,0,1,0,0,0,0,1,0,0,1,0,0,0,
	0,1,0,0,1,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,1,1,1,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,1,0,0,0,1,0,0,0,0,1,1,1,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,1, 1,1,1,0,0,0,0,1,0,0,0,1,0,0,
	0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,1,1,1,0,0,0,0,1,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,1,1,1,1,0,0, 0,1,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,0,1,1, 1,1,0,0,0,0,0,0,0,1,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,0,1,1,0,0,0,0,0,1,0,0,1, 0,0,0,0,1,0,0,0,0,0,0,0,1,0, 0,0,0,0,0,1,1,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,1,1,1,0, 0,0,0,1,0,0,0,0,0,0,0,0,1,1, 1,0,0,0,0,0,0,0,0,1,0,0,0,0, 1,1,1,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,1,0,0,0, 0,0,0,1,1,1,1,0,0,0,0,0,1,0, 0,0,0,0,0,0,1,0,0,0,0,0,0,0, 1,0,1,0,0,0,0,0,0,1,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,1,0,0, 1,0,0,0,0,1,0,0,1,0,0,0,0,1, 0,0,1,0,0,0,0,1,0,1,1,0,0,0,
	0,0,1,0,1,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,0,0,1,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0, 0,0,1,0,1,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,1, 0,0,0,1,0,0,0,1,0,0,0,1,0,0,
	0,1,0,1,0,1,0,0,0,1,1,1,1,1, 0,0,0,0,1,0,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,0,0,1,0,0,0, 0,1,0,0,1,0,0,0,0,0,1,1,0,0, 0,0,0,1,0,0,1,0,0,0,0,1,0,0, 1,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,0,0,1,0,0,0,0,1,0,0,1,0, 0,0,0,1,0,0,1,0,0,0,0,0,1,1, 1,0,0,0,0,0,0,1,0,0,0,0,0,1, 1,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,1,1,1,1,0, 0,0,0,0,0,0,1,0,0,0,0,0,1,1, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 1,1,1,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,1,0,0,0,0, 0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,1,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,1,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,1,0,0, 0,0,0,0,0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,0,0,0,0,0
	//Convert EOL in space and then
	//https://www.browserling.com/tools/word-wrap
};

int draw_number(int x, int y, int in) {
	short inlen= 9;//(in%10)/10;
	int iy=0, w=8, h=8;
	unsigned int na[inlen];
	
	if (in == 0) {
		for (iy=0;iy<h;iy++) {
			int ix=0; for (ix=0;ix<_fntmargin;ix++) {
				if (img_font_terminal8[(ix+((w*h)*0x10)+(w*iy))]) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]=dcol;
				else if (ard) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= ((u16*)VRAM)[0];
			}
		}
		return 0;
	}
	
	for(int i=0; i<=inlen;i++) {
        if (in < 1) {inlen= i; break;}
		na[i]= in%10;
        in/=10;
    }
	for(short i=inlen-1; i>=0 ;i--) {
		if (i<inlen-1) x+= _fntmargin;
		sleep(ddel);
			for (iy=0;iy<h;iy++) {
				int ix=0; for (ix=0;ix<_fntmargin;ix++) {
				if (img_font_terminal8[(ix+((w*h)*(0x10+na[i]))+(w*iy))]) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]=dcol;
				else if (ard) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= ((u16*)VRAM)[0];
			}
		}
	}
	return inlen-1;
}

int draw_number_hex(int x, int y, int in) {
	//Returns the digits count
	short inlen= 9;//(in%10)/10;
	int iy=0, w=8, h=8;
	unsigned int na[inlen];
	
	if (in == 0) {
		for (iy=0;iy<h;iy++) {
				for (int ix=0;ix<_fntmargin;ix++) {
				if (img_font_terminal8[(ix+((w*h)*0x10)+(w*iy))]) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]=dcol;
				else if (ard) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= ((u16*)VRAM)[0];
			}
		}
		return 0;
	}
	
	for(int i=0; i<=inlen;i++) {
        if (in < 1) {inlen= i; break;}
		na[i]= in%16;
        in/=16;
    }
	for(short i=inlen-1; i>=0 ;i--) {
		if (i<inlen-1) x+= _fntmargin;
		sleep(ddel);
			for (iy=0;iy<h;iy++) {
				int ix=0; for (ix=0;ix<_fntmargin;ix++) {
				
				if(na[i] < 10) {
					if (img_font_terminal8[(ix+((w*h)*(0x10+na[i]))+(w*iy))]) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]=dcol;
					else if (ard) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= ((u16*)VRAM)[0];
				} else {
					if (img_font_terminal8[(ix+((w*h)*('a'-42+na[i]))+(w*iy))]) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]=dcol;
					else if (ard) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= ((u16*)VRAM)[0];
				}
			}
		}
	}
	return inlen-1;
}

void draw_text(int x, int y, char* str, ...) {
	u8 inlen= strlen(str);
	int iy=0, w=8, h=8, Ix= x;
	u8 na[inlen];
	va_list valist;
	va_start(valist, (int)str[0]);
	
	for(u8 i=0; i<inlen;i++) na[i]= (int)str[i]-32;
	for(u8 i=0; i<inlen;i++) {
		if (i>0) x+= _fntmargin;
		sleep(ddel);
		//Special function characters or IDK
		if (str[i] == '&' && str[i+1] == 'n') {y+= h+1; x= Ix-_fntmargin; i++;} else
		if (str[i] == '%' && str[i+1] == 'd') {x+= _fntmargin*draw_number(x,y,va_arg(valist, int)); i++;} else
		if (str[i] == '%' && str[i+1] == 'h') {x+= _fntmargin*draw_number_hex(x,y,va_arg(valist, int)); i++;} 
		else {
			//Actual printing of characters
			for (iy=0;iy<h;iy++) {
					int ix=0; for (ix=0;ix<_fntmargin;ix++) {
					if (img_font_terminal8[(ix+((w*h)*na[i])+(w*iy))]) draw_point(x+ix, y+iy);//((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]=dcol;
					else if (ard) draw_pixel(x+ix, y+iy, ((u16*)VRAM)[0]);  //((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)] = ((u16*)VRAM)[0];
				}
			}
		}
	}
	va_end(valist);
}

IWRAM_CODE ARM_CODE void draw_char(int x, int y, char ch) {
	int iy=0, w=8, h=8;
	
	for (iy=0;iy<h;iy++) {
			int ix=0; for (ix=0;ix<_fntmargin;ix++) {
			if (img_font_terminal8[(ix+((w*h)*ch)+(w*iy))]) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]=dcol;
			else if (ard) ((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= ((u16*)VRAM)[0];
		}
	}
}

// "BACKGROUND" STUFFS //

 u16 img[]= {
	00001,00000,00000,00000,00000,00000,00000,00000,00000,00000,00000,00000,00000,00000,
	00000,00001,00000,31744,32080,32080,32080,32080,32080,32080,32080,32080,32080,32080,
	32080,32080,32080,00000,00000,31744,31744,31744,31744,31744,31744,31744,31744,31744,
	31744,31744,31744,31744,32080,00000,00000,31744,31744,31744,31744,31744,31744,31744,
	31744,31744,00031,00031,31744,31744,32080,00000,00000,31744,31744,31744,31744,31744,
	31744,31744,31744,00031,00031,00031,00031,31744,32080,00000,00000,31744,31744,31744,
	31744,31744,31744,31744,31744,00031,00031,00031,00031,31744,32080,00000,00000,31744,
	31744,31744,31744,31744,31744,31744,31744,31744,00031,00031,31744,31744,32080,00000,
	00000,31744,31744,00031,31744,31744,31744,31744,31744,31744,31744,31744,31744,31744,
	32080,00000,00000,16384,31744,00031,00031,00031,00031,31744,31744,31744,31744,31744,
	00031,31744,31744,00000,00000,16384,31744,00031,00031,00031,00031,00031,31744,31744,
	31744,00031,00031,31744,31744,00000,00000,16384,31744,00031,00031,00031,00031,00031,
	00031,00031,00031,00031,00031,31744,31744,00000,00000,16384,31744,00031,00031,00031,
	00031,00031,00031,00031,00031,00031,00031,31744,31744,00000,00000,16384,31744,00031,
	00031,00031,00031,00031,00031,00031,00031,00031,00031,31744,31744,00000,00000,16384,
	31744,31744,31744,31744,31744,31744,31744,31744,31744,31744,31744,31744,31744,00000,
	00000,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,
	31744,00000,00001,00000,00000,00000,00000,00000,00000,00000,00000,00000,00000,00000,
	00000,00000,00000,00001 
};
//NOTE: 0x0001 is transparent.

void draw_background(u16* back, int x, int y, int w, int h) {
	if (back == 0)
		back= img;
	for (int iy=0;iy<h;iy++) {
		for (int ix=0;ix<w;ix++) {
			if (back[ix+iy*w] != 1)
				((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= back[ix+iy*h];
		}
		sleep(ddel);
	}
}

void draw_background_8bpp(u8* back, int x, int y, int w, int h) {
	for (int iy=0;iy<h;iy++) {
		for (int ix=0;ix<w;ix++) {
				((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= 256*back[ix+iy*h];
		}
		sleep(ddel);
	}
}

void draw_background_1bpp(bool* back, int x, int y, int w, int h, int col) {
	for (int iy=0;iy<h;iy++) {
		for (int ix=0;ix<w;ix++) {
			if (back[ix+iy*w])
				((u16*)VRAM)[x+(y*SCREEN_WIDTH)+ix+(iy*SCREEN_WIDTH)]= col;
		}
		sleep(ddel);
	}
}

// SPRITE STUFFS //
void sprite_add(const u16* simg, int simglen) {
	if (vmode == bitmap_mode)
		memcpy(MEM_TILE_SPR_BMP, simg, simglen);
	if (vmode == tile_mode)
		memcpy(MEM_TILE_SPR_TL, simg, simglen);
}

void sprite_add_offset(const u16* simg, int simglen, int off) {
	if (vmode == bitmap_mode)
		memcpy(((TileBlock*)(VRAM + 0x14000 + off*64)), simg, simglen);
	if (vmode == tile_mode)
		memcpy(((TileBlock*)(VRAM + 0x10000 + off*64)), simg, simglen);
}

void sprite_reset(int ind) {
	sprattr[ind]->attr1 = OBJ_SIZE(0) | OBJ_X(248);
}

void sprite_reset_all() {
	for (int i=0; i < 128; i++) {
		sprattr[i]->attr1 = OBJ_SIZE(0) | OBJ_X(248);
	}
}

void sprite_set_palette(const u16* spal, int spallen) {
	memcpy(MEM_PALETTE_SPR, spal, spallen);
}

void draw_sprite(int ind, int x, int y, int size, int ch) {
	sprattr[ind]->attr0 = OBJ_SHAPE(size/10) | ATTR0_COLOR_256 |OBJ_Y(y);
	sprattr[ind]->attr1 = OBJ_SIZE(size%4) | OBJ_X(x);
	if (vmode == tile_mode) {
		if (size%4 == 0) sprattr[ind]->attr2 = OBJ_CHAR(ch*2);
		if (size%4 == 1) sprattr[ind]->attr2 = OBJ_CHAR(ch*8);
		if (size%4 == 2) sprattr[ind]->attr2 = OBJ_CHAR(ch*32);
		if (size%4 == 3) sprattr[ind]->attr2 = OBJ_CHAR(ch*128);
		return;
	}
	if (size%4 == 0) sprattr[ind]->attr2 = OBJ_CHAR(ch*2+512);
	if (size%4 == 1) sprattr[ind]->attr2 = OBJ_CHAR(ch*8+512);
	if (size%4 == 2) sprattr[ind]->attr2 = OBJ_CHAR(ch*32+512);
	if (size%4 == 3) sprattr[ind]->attr2 = OBJ_CHAR(ch*128+512);
	((u16*)VRAM)[0]= ((u16*)VRAM)[1];
}

void draw_sprite_4bpp(int ind, int x, int y, int size, int ch) {
	sprattr[ind]->attr0 = OBJ_SHAPE(size/10) | ATTR0_COLOR_16 |OBJ_Y(y);
	sprattr[ind]->attr1 = OBJ_SIZE(size%4) | OBJ_X(x);
	if (vmode == tile_mode) {
		if (size%4 == 0) sprattr[ind]->attr2 = OBJ_CHAR(ch*1);
		if (size%4 == 1) sprattr[ind]->attr2 = OBJ_CHAR(ch*4);
		if (size%4 == 2) sprattr[ind]->attr2 = OBJ_CHAR(ch*16);
		if (size%4 == 3) sprattr[ind]->attr2 = OBJ_CHAR(ch*64);
		return;
	}
	if (size%4 == 0) sprattr[ind]->attr2 = OBJ_CHAR(ch*1+512);
	if (size%4 == 1) sprattr[ind]->attr2 = OBJ_CHAR(ch*4+512);
	if (size%4 == 2) sprattr[ind]->attr2 = OBJ_CHAR(ch*16+512);
	if (size%4 == 3) sprattr[ind]->attr2 = OBJ_CHAR(ch*64+512);
	((u16*)VRAM)[0]= ((u16*)VRAM)[1];
}

// TILE BG STUFFS //

const unsigned short portfontTiles[1536] __attribute__((aligned(4)))={
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0xFF00,0x0000,0xFF00,0x0000,0xFF00,0x0000,0xFF00,0x0000,     0xFF00,0x0000,0x0000,0x0000,0xFF00,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0F00,0x000F,0xFF00,0x00FF,0xF000,0x00F0,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0F00,0x00F0,0x0FF0,0x00FF,0xFFFF,0x0FFF,0x0FF0,0x00FF,     0xFFFF,0x0FFF,0x0FF0,0x00FF,0x00F0,0x000F,0x0000,0x0000,
	0xFFF0,0x00FF,0xF0FF,0x0FF0,0xF0FF,0x0000,0xFFF0,0x00FF,     0xF000,0x0FF0,0xF0FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,     0x0000,0x0000,0x0FF0,0x0FF0,0x0FF0,0x00FF,0xF000,0x000F,     0xFF00,0x0FF0,0x0FF0,0x0FF0,0x0000,0x0000,0x0000,0x0000,     0x0F00,0x0000,0xF0F0,0x0000,0x0F00,0x0000,0xF0F0,0x0F00,     0x000F,0x00FF,0x000F,0x00FF,0xFFF0,0x0F00,0x0000,0x0000,     0xFF00,0x0000,0xFF00,0x0000,0xF000,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x000F,0xF000,0x0000,0x0F00,0x0000,0x0F00,0x0000,     0x0F00,0x0000,0xF000,0x0000,0x0000,0x000F,0x0000,0x0000,     0x0F00,0x0000,0xF000,0x0000,0x0000,0x000F,0x0000,0x000F,     0x0000,0x000F,0xF000,0x0000,0x0F00,0x0000,0x0000,0x0000,     0x0000,0x0000,0x00FF,0x0FF0,0x0FF0,0x00FF,0xFF00,0x000F,     0x0FF0,0x00FF,0x00FF,0x0FF0,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0xF000,0x0000,0xF000,0x0000,0xFFF0,0x00FF,     0xF000,0x0000,0xF000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0xFF00,0x0000,0xFF00,0x0000,0xF000,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xFFF0,0x00FF,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0xFF00,0x0000,0xFF00,0x0000,0x0000,0x0000,     0x0000,0x0F00,0x0000,0x0FF0,0x0000,0x00FF,0xF000,0x000F,     0xFF00,0x0000,0x0FF0,0x0000,0x00F0,0x0000,0x0000,0x0000,
	0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FFF,0xF0FF,0x0FF0,     0x0FFF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,     0xF000,0x000F,0xFF00,0x000F,0xFFF0,0x000F,0xF000,0x000F,     0xF000,0x000F,0xF000,0x000F,0xF000,0x000F,0x0000,0x0000,     0xFFFF,0x00FF,0x0000,0x0FF0,0x0000,0x0FF0,0xFFF0,0x00FF,     0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x0FFF,0x0000,0x0000,     0xFFFF,0x00FF,0x0000,0x0FF0,0x0000,0x0FF0,0xFF00,0x00FF,     0x0000,0x0FF0,0x0000,0x0FF0,0xFFFF,0x00FF,0x0000,0x0000,
	0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x0FFF,     0x0000,0x0FF0,0x0000,0x0FF0,0x0000,0x0FF0,0x0000,0x0000,     0xFFFF,0x0FFF,0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x00FF,     0x0000,0x0FF0,0x0000,0x0FF0,0xFFFF,0x00FF,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x00FF,     0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,     0xFFFF,0x0FFF,0x0000,0x0FF0,0x0000,0x0FF0,0x0000,0x0FF0,     0x0000,0x0FF0,0x0000,0x0FF0,0x0000,0x0FF0,0x0000,0x0000,
	0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,     0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x0FFF,     0x0000,0x0FF0,0x0000,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,     0x0000,0x0000,0xFF00,0x0000,0xFF00,0x0000,0x0000,0x0000,     0xFF00,0x0000,0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0xFF00,0x0000,0xFF00,0x0000,0x0000,0x0000,     0xFF00,0x0000,0xFF00,0x0000,0xF000,0x0000,0x0000,0x0000,
	0x0000,0x000F,0xF000,0x000F,0xFF00,0x000F,0xFFF0,0x000F,     0xFF00,0x000F,0xF000,0x000F,0x0000,0x000F,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFF0,0x00FF,0x0000,0x0000,     0xFFF0,0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0F00,0x0000,0xFF00,0x0000,0xFF00,0x000F,0xFF00,0x00FF,     0xFF00,0x000F,0xFF00,0x0000,0x0F00,0x0000,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0FF0,0x0000,0x0FF0,0xFF00,0x00FF,     0xFF00,0x0000,0x0000,0x0000,0xFF00,0x0000,0x0000,0x0000,
	0xFFF0,0x00FF,0x000F,0x0F00,0xFF0F,0x0F0F,0x0F0F,0x0F0F,     0xFF0F,0x0FFF,0x000F,0x0000,0xFFF0,0x00FF,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFFF,0x0FFF,     0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,0x0000,0x0000,     0xFFFF,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFFF,0x00FF,     0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFFF,0x00FF,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0000,0x00FF,0x0000,     0x00FF,0x0000,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,
	0xFFFF,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,     0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFFF,0x00FF,0x0000,0x0000,     0xFFFF,0x0FFF,0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x000F,     0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x0FFF,0x0000,0x0000,     0xFFFF,0x0FFF,0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x000F,     0x00FF,0x0000,0x00FF,0x0000,0x00FF,0x0000,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0000,0x00FF,0x0FFF,     0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,
	0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFFF,0x0FFF,     0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,0x0000,0x0000,     0xFFFF,0x00FF,0xFF00,0x0000,0xFF00,0x0000,0xFF00,0x0000,     0xFF00,0x0000,0xFF00,0x0000,0xFFFF,0x00FF,0x0000,0x0000,     0x0000,0x0FF0,0x0000,0x0FF0,0x0000,0x0FF0,0x00FF,0x0FF0,     0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,     0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x00FF,0xFFFF,0x000F,     0x00FF,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0x0000,0x0000,
	0x00FF,0x0000,0x00FF,0x0000,0x00FF,0x0000,0x00FF,0x0000,     0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x0FFF,0x0000,0x0000,     0x00FF,0x0FF0,0x0FFF,0x0FFF,0xFFFF,0x0FFF,0xF0FF,0x0FF0,     0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,0x0000,0x0000,     0x00FF,0x0FF0,0x0FFF,0x0FF0,0xFFFF,0x0FF0,0xF0FF,0x0FFF,     0x00FF,0x0FFF,0x00FF,0x0FF0,0x00FF,0x0FF0,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,     0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,
	0xFFFF,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFFF,0x00FF,     0x00FF,0x0000,0x00FF,0x0000,0x00FF,0x0000,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,     0x00FF,0x0FF0,0x00FF,0x0FFF,0xFFF0,0x0FFF,0x0000,0x0000,     0xFFFF,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFFF,0x00FF,     0xF0FF,0x000F,0x00FF,0x00FF,0x00FF,0x0FF0,0x0000,0x0000,     0xFFF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0000,0xFFF0,0x00FF,     0x0000,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,
	0xFFF0,0x0FFF,0xF000,0x000F,0xF000,0x000F,0xF000,0x000F,     0xF000,0x000F,0xF000,0x000F,0xF000,0x000F,0x0000,0x0000,     0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,     0x00FF,0x0FF0,0x00FF,0x0FF0,0xFFF0,0x00FF,0x0000,0x0000,     0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,     0x00FF,0x0FF0,0x0FF0,0x00FF,0xFF00,0x000F,0x0000,0x0000,     0x00FF,0x0FF0,0x00FF,0x0FF0,0x00FF,0x0FF0,0xF0FF,0x0FF0,     0xFFFF,0x0FFF,0x0FFF,0x0FFF,0x00FF,0x0FF0,0x0000,0x0000,
	0x00FF,0x0FF0,0x00FF,0x0FF0,0x0FF0,0x00FF,0xFF00,0x000F,     0x0FF0,0x00FF,0x00FF,0x0FF0,0x00FF,0x0FF0,0x0000,0x0000,     0x0FF0,0x0FF0,0x0FF0,0x0FF0,0x0FF0,0x0FF0,0xFF00,0x00FF,     0xF000,0x000F,0xF000,0x000F,0xF000,0x000F,0x0000,0x0000,     0xFFF0,0x0FFF,0x0000,0x0FF0,0x0000,0x00FF,0xF000,0x000F,     0xFF00,0x0000,0x0FF0,0x0000,0xFFF0,0x0FFF,0x0000,0x0000,     0xFFF0,0x000F,0x0FF0,0x0000,0x0FF0,0x0000,0x0FF0,0x0000,     0x0FF0,0x0000,0x0FF0,0x0000,0xFFF0,0x000F,0x0000,0x0000,
	0x000F,0x0000,0x00F0,0x0000,0x0F00,0x0000,0xF000,0x0000,     0x0000,0x000F,0x0000,0x00F0,0x0000,0x0F00,0x0000,0x0000,     0xFFF0,0x000F,0xF000,0x000F,0xF000,0x000F,0xF000,0x000F,     0xF000,0x000F,0xF000,0x000F,0xFFF0,0x000F,0x0000,0x0000,     0xF000,0x0000,0x0F00,0x000F,0x00F0,0x00F0,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFFF,0x0FFF,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFF0,0x000F,0x00FF,0x00FF,     0x00FF,0x00FF,0x00FF,0x00FF,0xFFF0,0x0FF0,0x0000,0x0000,     0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x000F,0x00FF,0x00FF,     0x00FF,0x00FF,0x00FF,0x00FF,0xFFFF,0x000F,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFF0,0x00FF,0x00FF,0x0000,     0x00FF,0x0000,0x00FF,0x0000,0xFFF0,0x00FF,0x0000,0x0000,
	0x0000,0x00FF,0x0000,0x00FF,0xFFF0,0x00FF,0x00FF,0x00FF,     0x00FF,0x00FF,0x00FF,0x00FF,0xFFF0,0x00FF,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFF0,0x000F,0x00FF,0x00FF,     0xFFFF,0x00FF,0x00FF,0x0000,0xFFF0,0x00FF,0x0000,0x0000,     0xF000,0x00FF,0xFF00,0x0000,0xFF00,0x0000,0xFFFF,0x00FF,     0xFF00,0x0000,0xFF00,0x0000,0xFF00,0x0000,0x0000,0x0000,     0x0000,0x0000,0xFFF0,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,     0xFFF0,0x00FF,0x0000,0x00FF,0xFFF0,0x000F,0x0000,0x0000,
	0x00FF,0x0000,0x00FF,0x0000,0x00FF,0x0000,0xFFFF,0x000F,     0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x0000,0x0000,     0x0000,0x0000,0xF000,0x000F,0x0000,0x0000,0xF000,0x000F,     0xF000,0x000F,0xF000,0x000F,0xF000,0x000F,0x0000,0x0000,     0x0000,0x0000,0xF000,0x000F,0x0000,0x0000,0xF000,0x000F,     0xF000,0x000F,0xF0FF,0x000F,0xFFF0,0x0000,0x0000,0x0000,     0x00FF,0x0000,0x00FF,0x0000,0x00FF,0x00FF,0xF0FF,0x000F,     0xFFFF,0x0000,0xF0FF,0x000F,0x00FF,0x00FF,0x0000,0x0000,
	0xFF00,0x000F,0xF000,0x000F,0xF000,0x000F,0xF000,0x000F,     0xF000,0x000F,0xF000,0x000F,0xF000,0x000F,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x0FFF,0x00FF,0xF0FF,0x0FF0,     0xF0FF,0x0FF0,0xF0FF,0x0FF0,0xF0FF,0x0FF0,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFFF,0x000F,0x00FF,0x00FF,     0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFF0,0x000F,0x00FF,0x00FF,     0x00FF,0x00FF,0x00FF,0x00FF,0xFFF0,0x000F,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0xFFFF,0x000F,0x00FF,0x00FF,     0x00FF,0x00FF,0xFFFF,0x000F,0x00FF,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFF0,0x00FF,0x00FF,0x00FF,     0x00FF,0x00FF,0xFFF0,0x00FF,0x0000,0x00FF,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xF0FF,0x00FF,0x0FFF,0x0000,     0x00FF,0x0000,0x00FF,0x0000,0x00FF,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFF0,0x000F,0x00FF,0x0000,     0xFFF0,0x000F,0x0000,0x00FF,0xFFFF,0x000F,0x0000,0x0000,
	0xFFF0,0x0000,0xFF00,0x0000,0xFFFF,0x00FF,0xFF00,0x0000,     0xFF00,0x0000,0xFF00,0x0000,0xF000,0x00FF,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x00FF,0x00FF,0x00FF,0x00FF,     0x00FF,0x00FF,0x00FF,0x00FF,0xFFF0,0x000F,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x00FF,0x00FF,0x00FF,0x00FF,     0x00FF,0x00FF,0xFFF0,0x000F,0xFF00,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x00FF,0x0FF0,0xF0FF,0x0FF0,     0xF0FF,0x0FF0,0xF0FF,0x0FF0,0x0FF0,0x00FF,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x00FF,0x0FF0,0x0FF0,0x00FF,     0xFF00,0x000F,0x0FF0,0x00FF,0x00FF,0x0FF0,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0x00FF,0x00FF,0x00FF,0x00FF,     0xFFF0,0x000F,0xFF00,0x0000,0x0FF0,0x0000,0x0000,0x0000,     0x0000,0x0000,0x0000,0x0000,0xFFFF,0x00FF,0xF000,0x000F,     0xFF00,0x0000,0x0FF0,0x0000,0xFFFF,0x00FF,0x0000,0x0000,     0xFFF0,0x00FF,0xF00F,0x0F00,0x0F0F,0x0F0F,0xFF0F,0x0F0F,     0x0F0F,0x0F0F,0x0F0F,0x0F0F,0xFFF0,0x00FF,0x0000,0x0000,
	0xFFF0,0x00FF,0xFF0F,0x0F00,0x0F0F,0x0F0F,0xFF0F,0x0F00,     0x0F0F,0x0F0F,0xFF0F,0x0F00,0xFFF0,0x00FF,0x0000,0x0000,     0xF000,0x0FFF,0x0FF0,0x0F00,0xF0FF,0x0F00,0xF00F,0x0F00,     0xF00F,0x0F00,0xF00F,0x0F0F,0xFFFF,0x0FFF,0x0000,0x0000,     0xFFFF,0x0000,0x000F,0x00FF,0xFF0F,0x0FF0,0x0F0F,0x0F0F,     0xFF0F,0x0F00,0x0F0F,0x0F0F,0xFFFF,0x0FFF,0x0000,0x0000,     0xFFF0,0x00FF,0x000F,0x0F00,0xFF0F,0x0F0F,0x0F0F,0x0F00,     0xFF0F,0x0F0F,0x000F,0x0F00,0xFFF0,0x00FF,0x0000,0x0000,
};
#define portfontTilesLen	3072

int bgpr[4]= {1,2,3,0};
int bgmof[4]= {0,1,2,3};
void background_add_tiled(int bgno, const u16* bimg, int bimglen, const u16* bmap, int bmaplen) {
	if (vmode == bitmap_mode) {
		draw_pixel(0,0,0);
		draw_set_color(c_red);
		draw_text(1,1,"Tiled backgrounds must$nbe used in tile mode!");
		return;
	}
	memmove(&MEM_TILE_BG[bgno+1][0], bimg, bimglen);
	memmove(&MEM_SCREENBLOCKS[bgmof[bgno]], bmap, bmaplen);
}

void background_add_tiled_ext(int bgno, const u16* bimg, int bimglen, int ch, const u16* bmap, int bmaplen, int bmof) {
	if (vmode == bitmap_mode) {
		draw_pixel(0,0,0);
		draw_set_color(c_red);
		draw_text(1,1,"Tiled backgrounds must$nbe used in tile mode!");
		return;
	}
	bgmof[bgno]= bmof;
	memcpy(&MEM_TILE_BG[ch+1][0], bimg, bimglen);
	memcpy(&MEM_SCREENBLOCKS[bgmof[bgno]], bmap, bmaplen);
}

void background_load_tiled(int bgno, const u16* bimg, int bimglen) {
	if (vmode == bitmap_mode) {
		draw_pixel(0,0,0);
		draw_set_color(c_red);
		draw_text(1,1,"Tiled backgrounds must$nbe used in tile mode!");
		return;
	}
	memmove(&MEM_TILE_BG[bgno+1][0], bimg, bimglen);
}

void background_set_palette(const u16* bpal, int bpallen) {
	memmove(BG_PALETTE, bpal, bpallen);
}

void background_set_palette_offset(const u16* bpal, int bpallen, int pofs) {
	memmove(((u16 *)0x05000000+pofs), bpal, bpallen);
}

void background_set_priority(int bgno, int pr) {
	if (pr < 4)
		bgpr[bgno]= pr;
}

void background_set_map(int bgno, int sb) {
	if (sb < 4)
		bgmof[bgno]= sb;
}

void draw_background_tiled(int bgno, int xoff, int yoff, int size, int ch) {
	BGCTRL[bgno] = BG_256_COLOR | BG_SIZE(size) | BG_PRIORITY(bgpr[bgno]) | SCREEN_BASE(bgmof[bgno]) | CHAR_BASE(ch+1);
	BG_OFFSET[bgno].x=-xoff;
	BG_OFFSET[bgno].y=-yoff;
}

void draw_background_tiled_4bpp(int bgno, int xoff, int yoff, int size, int ch) {
	BGCTRL[bgno] = BG_16_COLOR | BG_SIZE(size) | BG_PRIORITY(bgpr[bgno]) | SCREEN_BASE(bgmof[bgno]) | CHAR_BASE(ch+1);
	BG_OFFSET[bgno].x=-xoff;
	BG_OFFSET[bgno].y=-yoff;
}

void background_reset_all() {
	for (int i= 0; i<4; i++)
		BGCTRL[i] = BG_16_COLOR | BG_SIZE(0) | BG_PRIORITY(4) | SCREEN_BASE(30) | CHAR_BASE(8);
}

void print(char* str, ...) {
	int col= 0, row= 0;
	va_list valist;
	va_start(valist, (int)str[0]);
	
	if (!fntset) {
		memcpy(&MEM_TILE_BG[0][128], portfontTiles, portfontTilesLen);
		fntset= 1;
	}
	((u16*)BG_PALETTE)[0xF]= dcol;
	BG_OFFSET[3].x= 0;
	BG_OFFSET[3].y= 0;
	u8 inlen= strlen(str);
	u8 na[inlen];
	u32 Cursor= 32*row+col;
	BGCTRL[3] = BG_16_COLOR | BG_SIZE(0) | BG_PRIORITY(bgpr[3]) | SCREEN_BASE(31) | CHAR_BASE(0);
	
	for(u8 i=0; i<inlen;i++) na[i]= (int)str[i]-32;
	for(u16 ti=0; ti < inlen; ti++) {
		sleep(ddel);
		//Special function characters or IDK
		if (str[ti] == '&' && str[ti+1] == 'n') {Cursor= ((row++)+1)*32+col; ti++;} else
		if (str[ti] == '%' && str[ti+1] == 'c') {col= va_arg(valist, int); Cursor= 32*row+col; ti++;} else
		if (str[ti] == '%' && str[ti+1] == 'r') {row= va_arg(valist, int); Cursor= 32*row+col; ti++;} else
		if (str[ti] == '%' && str[ti+1] == 'd') {
			int in= va_arg(valist, int);
			u16 uin= (u16)in;
			if (in < 0) {
				*((u16 *)FONT_MAPADDRESS + Cursor) = 269;
				Cursor++;
			}
			short ninlen= 9;//(in%10)/10;
			unsigned int nna[ninlen];
			
			for(int i=0; i<ninlen;i++) {
				if (uin < 1) {ninlen= i; break;}
				nna[i]= uin%10;
				uin/=10;
			}
			for(short i=ninlen; i>0; i--) {
				sleep(ddel);
				*((u16 *)FONT_MAPADDRESS + Cursor) = nna[i-1]+272;
				Cursor++;
			}
			ti++;
		}
		else {
			//Actual printing of characters
			*((u16 *)FONT_MAPADDRESS + Cursor) = na[ti]+256;
			Cursor++;
		}
	}
	va_end(valist);
}

void pclear() {
	for (u32 i= 0; i < 1024; i++)
		*((u16 *)FONT_MAPADDRESS + i) = 256;
}

// KEYPAD STUFFS //

#define		key_A		0
#define 	key_B		1
#define 	key_select	2
#define		key_start	3
#define 	key_right	4
#define 	key_left	5
#define		key_up		6
#define 	key_down	7
#define 	key_R		8
#define 	key_L		9
#define 	key_any		10

bool keypad[11];
//bool lk[10];
int ik;
bool keypad_check(int key) {
	int n=REG_KEYINPUT;
	for(int i=0; i<10 ;i++){    
		keypad[i]=-n%2+1;
		if (-n%2+1) keypad[key_any]= 1;
		n=n/2;    
	}
	if (!Llk[key]) { Llk[key]= 1;}
	else { Llk[key]= 1;}
	return keypad[key];
}

bool keypad_check_pressed(int key) {
	int n=REG_KEYINPUT;
	for(int i=0; n>0;i++){    
		keypad[i]=-n%2+1;
		if (-n%2+1) keypad[key_any]= 1;
		n=n/2;    
	}
	if (key_last != REG_KEYINPUT) { key_last= REG_KEYINPUT; return keypad[key];}
	else { key_last= REG_KEYINPUT; return false;}
}

bool keypad_check_released(int key) {
	int n=REG_KEYINPUT;
	for(int i=0; n>0;i++){    
		keypad[i]=-n%2+1;    
		n=n/2;    
	}
	if (ik != REG_KEYINPUT && REG_KEYINPUT == 1023) { ik= REG_KEYINPUT; return !keypad[key];}
	else { ik= REG_KEYINPUT; return false;}
}

 int keypad_get() {
	int n=REG_KEYINPUT;
	for(int i=0; n>0;i++){    
		if (-n%2+1)	return i;
		n=n/2;    
	}
	return 10;
}

struct object {
	int id;
	int x, y;
	float xf, yf;
	int w, h;
	int image_index;
	int direction;	//in degrees
	bool enabled;
};

/*int place_meeting(struct *inobj,int x, int y, int smin, int smax) {
	for (int i= smin; i < smax; i++) {
		if (x >= inobj[i].x && inobj[i].w <= 
	}
}*/

#endif
