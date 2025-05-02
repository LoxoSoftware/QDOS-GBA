#include "../lib/qdos.h"

void draw_rect(int x, int y, int w, int h, u16 color)
{
    for (int iy=y; iy<y+h; iy++)
        for (int ix=x; ix<x+w; ix++)
            ((u16*)VRAM)[ix+iy*SCREEN_WIDTH]= color+ix;
}

int main()
{
    int xr=0, yr= 0;
    s8 xdir= 1, ydir= 1;

    while (1)
    {
        if (xr<=0) { xdir= 1; print("Bounce!&n"); }
        if (xr+32>=SCREEN_WIDTH) { xdir= -1; print("Bounce!&n"); }
        if (yr<=0) { ydir= 1; print("Bounce!&n"); }
        if (yr+32>=SCREEN_HEIGHT) { ydir= -1; print("Bounce!&n"); }
        xr+=xdir, yr+=ydir;

        redraw();
        draw_rect(xr, yr, 32, 32, RGB5(0,yr>>2,31));
    }
}
