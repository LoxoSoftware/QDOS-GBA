//console.h
//Version 0.5.0

#ifndef CONSOLE_H
#define CONSOLE_H

#define __console_charw 6
#define __console_charh 9
#define __console_charperw 40 //(SCREEN_WIDTH/__console_charw) //40
#define __console_charperh 19 //(SCREEN_HEIGHT/__console_charh) //20

					//X					  Y
u8 __console_buffer[__console_charperw][__console_charperh];
char __console_stdout[512]; int __console_stdout_index= 0;

int __console_icolumn= 0;
int __console_irow= 0;
u16 __console_bgcol= c_black;
u16 __console_fgcol= c_ltgray;

bool __console_silent= false;

int __console_yoffset= 0;

void console_init() {

	sprite_add((u16*)spritesTiles, spritesTilesLen);
	SPRITE_PALETTE[1]= c_blue;
}

int __console_curtoggle= 0;
void console_idle() {

	draw_sprite_4bpp(0, __console_icolumn*__console_charw, (__console_irow*__console_charh)-__console_yoffset, 0, __console_curtoggle);
		
	if (!alarm[10]) {
	
		if (__console_curtoggle) __console_curtoggle= 0;
		else __console_curtoggle= 1;
		alarm[10]= 30;
	}
}

void console_colors(u16 bgcol, u16 fgcol, u16 cursorcol) {

	__console_bgcol= bgcol;
	__console_fgcol= fgcol;
	SPRITE_PALETTE[1]= cursorcol;
}

IWRAM_CODE ARM_CODE void console_drawbuffer() {

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

void console_scrolldown() {

	__console_icolumn= 0;

	for(int iy= 1; iy<__console_charperh; iy++) {
		
		for(int ix= 0; ix<__console_charperw; ix++) {
		
			if (iy == __console_charperh-1)
				__console_buffer[ix][iy-1]= 0;
			else
				__console_buffer[ix][iy-1] = __console_buffer[ix][iy];
		}
	}
	
	//console_drawbuffer()
}

void console_newline() {

	__console_irow++;
	__console_icolumn=0;
	
	if (__console_irow >= __console_charperh-1) {
		
		console_scrolldown();
		__console_irow= __console_charperh-2;
	}
}

void console_print_number(int num) {

	short inlen= 9;
	unsigned int na[inlen];

	if (num == 0) {
	
		__console_buffer[__console_icolumn][__console_irow]= '0'-32;
		return;
		
	} else if (num > 0) {
	
		//Number to digits
		for(int i=0; i<=inlen;i++) {
        	if (num < 1) {inlen= i; break;}
			na[i]= num%10;
        	num/=10;
    	}
		
	} else {
	
		//Number to digits (negative)
		//na[0]= 69; //Minus sign instead of the digit
		num *= -1;
		for(int i=0; i<=inlen;i++) {
        	if (num < 1) {na[i]=69; inlen= i+1; break;}
			na[i]= num%10;
        	num/=10;
    	}
	
	}
	
	for (int i=inlen-1; i>=0; i--) { //Print the digits on to the buffer
	
		if (!__console_silent)
		if (__console_icolumn >= __console_charperw) {
		
			__console_icolumn= 0;
			__console_irow++;
		}
		
		if (!__console_silent)
		if (__console_irow >= __console_charperh-1) {
		
			console_scrolldown();
			__console_irow= __console_charperh-2;
		}
		
		if (na[i] != 69) {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= na[i]+16;
			if (__console_stdout_index < sizeof(__console_stdout)) {
				__console_stdout[__console_stdout_index]= na[i]+'0';
				__console_stdout_index++;
			}
		}
		else {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= '-'-32;
			if (__console_stdout_index < sizeof(__console_stdout)) {
				__console_stdout[__console_stdout_index]= '-';
				__console_stdout_index++;
			}
		}
		
		__console_icolumn++;
	}
	
	__console_icolumn--;
}

void console_print_number_hex(int num, int digits) {

	//if 'digits' is -1, the length is automatic
	short inlen= 9;
	unsigned int na[inlen];

	if (num == 0) {
	
		__console_buffer[__console_icolumn][__console_irow]= '0'-32;
		return;
		
	} else if (num > 0) {
	
		//Number to digits
		for(int i=0; i<=inlen;i++) {
        	if (num < 1) {inlen= i; break;}
			na[i]= num%16;
        	num/=16;
    	}
		
	} else return;
	
	for (int i=inlen-1; i>=0; i--) { //Print the digits on to the buffer
	
		if (!__console_silent)
		if (__console_icolumn >= __console_charperw) {
		
			__console_icolumn= 0;
			__console_irow++;
		}
		
		if (!__console_silent)
		if (__console_irow >= __console_charperh-1) {
		
			console_scrolldown();
			__console_irow= __console_charperh-2;
		}
		
		if (na[i] < 10) {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= na[i]+16;
			if (__console_stdout_index < sizeof(__console_stdout)) {
				__console_stdout[__console_stdout_index]= na[i]+'0';
				__console_stdout_index++;
			}
		}
		else {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= na[i]+'a'-42;
			if (__console_stdout_index < sizeof(__console_stdout)) {
				__console_stdout[__console_stdout_index]= na[i]+'a';
				__console_stdout_index++;
			}
		}
		
		__console_icolumn++;
	}
	
	__console_icolumn--;
}

void console_printf(char* str, ...) {
	
	va_list valist;
	va_start(valist, (int)str[0]);
	
	int tsz= 0;
	
	while(str[tsz] != '\0')
		tsz++;
	
	int c= 0;
	
	//__console_buffer[__console_icolumn][__console_irow]= '>'-32;
	
	while(tsz) {
		
		if (!__console_silent)
		if (__console_icolumn >= __console_charperw) {
		
			__console_icolumn= 0;
			__console_irow++;
		}
		
		if (!__console_silent)
		if (__console_irow >= __console_charperh-1) {
		
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
		else {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= str[c]-32;
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

void console_prints(char* str) {
	int tsz= 0;

	while(str[tsz] != '\0')
		tsz++;

	int c= 0;

	//__console_buffer[__console_icolumn][__console_irow]= '>'-32;

	while(tsz) {

		if (!__console_silent)
		if (__console_icolumn >= __console_charperw) {

			__console_icolumn= 0;
			__console_irow++;
		}

		if (!__console_silent)
		if (__console_irow >= __console_charperh-1) {

			console_scrolldown();
			__console_irow= __console_charperh-2;
		}

		if (str[c] == '&' && str[c+1] == 'n') {c++; tsz--; if (!__console_silent) console_newline(); if (!__console_silent) __console_icolumn--;}
		else {
			if (!__console_silent) __console_buffer[__console_icolumn][__console_irow]= str[c]-32;
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

void console_moveback(int n) {
	
	while(n) {
		
		if (__console_icolumn < 0) {
		
			__console_icolumn= __console_charperw-1;
			__console_irow--;
		}
		
		__console_icolumn--;
		n--;
	}
}

void console_moveon(int n) {
	
	while(n) {
		
		if (__console_icolumn >= __console_charperw) {
		
			__console_icolumn= 0;
			__console_irow++;
		}
		
		__console_icolumn++;
		n--;
	}
}

void console_redrawrow(int row, char* str) {

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

void console_clear() {
	
	//Clear the stdout buffer
	__console_stdout_index= 0;
	memset(__console_stdout, '\0', sizeof(__console_stdout));
	
	__console_irow= 0;
	__console_icolumn= 0;
	
	for(int iy= 0; iy<__console_charperh; iy++) {
		
		for(int ix= 0; ix<__console_charperw; ix++) {
			
			__console_buffer[ix][iy]= 0;
		}	
	}
}

#endif
