//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// QDOS/keyboard.h  Builtin virtual keyboard                                //
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

#ifndef KEYBOARD_H
#define KEYBOARD_H

#define __keyboard_consoleup __console_yoffset= (__console_irow*__console_charh) - __keyboard_height

int __keyboard_lastselx= 0;
int __keyboard_lastsely= 0;
int __keyboard_lastcase= 0;

bool __keyboard_isopen= false;

//void execute_command(char* cmd);
void go_console_keyboard(void (*fnptr)(char*))
{
	if (__keyboard_isopen) return;

	const int KEYS_W = SCREEN_WIDTH;
	const int ROWS = 4;
	const int COLUMNS = 10;

	int selx= 0;
	int sely= 1;
	int kbcase= 0;
	int start_column= __console_icolumn;
	int start_index= kbstring_len;

	//console_moveback(kbstring_len);

	char instr[64];
	strcpy(instr, kbstring);
	int inlen= kbstring_len;
	int inindex= kbstring_len;
	//if (inindex == 0) inindex=1;

	char* kbkeys_uppercase[4][10]=
	{
		{"! ", ": ", ". ", "# ", "& ", "| ", "/ ", "- ", "\"", "$ "},
		{"Q ", "W ", "E ", "R ", "T ", "Y ", "U ", "I ", "O ", "P "},
		{"A ", "S ", "D ", "F ", "G ", "H ", "J ", "K ", "L ", ".."},
		{"::", "Z ", "X ", "C ", "V ", "B ", "N ", "M ", "__", "OK"},
	};

	char* kbkeys_lowercase[4][10]=
	{
		{"1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ", "0 "},
		{"q ", "w ", "e ", "r ", "t ", "y ", "u ", "i ", "o ", "p "},
		{"a ", "s ", "d ", "f ", "g ", "h ", "j ", "k ", "l ", ".."},
		{"::", "z ", "x ", "c ", "v ", "b ", "n ", "m ", "__", "OK"},
	};

	char* kbkeys_otherlow[4][10]=
	{
		{"a ", "b ", "c ", "d ", "e ", "f ", "$ ", "# ", "- ", "_ "},
		{"1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ", "0 "},
		{". ", ", ", "; ", ": ", "( ", ") ", "[ ", "] ", "\"", "ab"},
		{"::", "/ ", "* ", "+ ", "< ", "> ", "{ ", "} ", "__", "= "},
	};

	char* kbkeys_otherhigh[4][10]=
	{
		{"1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ", "0 "},
		{"! ", "@ ", "# ", "$ ", "% ", "^ ", "& ", "* ", "| ", "~ "},
		{". ", ", ", "; ", ": ", "( ", ") ", "[ ", "] ", "\'", "AB"},
		{"::", "\\", "* ", "+ ", "< ", "> ", "{ ", "} ", "__", "? "},
	};

	int keyboard_y= SCREEN_HEIGHT-__keyboard_height;

	draw_clear(c_navy);
	__keyboard_consoleup;
	console_drawbuffer();

	void redraw()
	{
		__keyboard_consoleup;
		draw_set_color(c_navy);
		draw_rectangle(0, keyboard_y, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

		//Draw buttons
		for(int iy=0; iy<ROWS; iy++)
		{
			for(int ix=0; ix<COLUMNS; ix++)
			{
				if (selx == ix && sely == iy)
				{
					draw_set_color(c_aqua);
					draw_rectangle(ix*(KEYS_W/COLUMNS)+1, keyboard_y+2+iy*(__keyboard_height/ROWS),  (ix+1)*(KEYS_W/COLUMNS)-1, keyboard_y+(iy+1)*(__keyboard_height/ROWS), 0);

				} else
				{
					draw_set_color(c_white);
					draw_rectangle(ix*(KEYS_W/COLUMNS)+1, keyboard_y+2+iy*(__keyboard_height/ROWS),  (ix+1)*(KEYS_W/COLUMNS)-1, keyboard_y+(iy+1)*(__keyboard_height/ROWS), 1);
				}

				if (selx == ix && sely == iy) draw_set_color(c_black);
				else						  draw_set_color(c_white);

				if (kbcase == 1) //Uppercase
					draw_text(ix*(KEYS_W/COLUMNS)+1+4, keyboard_y+2+iy*(__keyboard_height/ROWS)+4, kbkeys_uppercase[iy][ix]);
				if (kbcase == 0) //Lowercase
					draw_text(ix*(KEYS_W/COLUMNS)+1+4, keyboard_y+2+iy*(__keyboard_height/ROWS)+4, kbkeys_lowercase[iy][ix]);
				if (kbcase == 3) //Other high
					draw_text(ix*(KEYS_W/COLUMNS)+1+4, keyboard_y+2+iy*(__keyboard_height/ROWS)+4, kbkeys_otherhigh[iy][ix]);
				if (kbcase == 2) //Other low
					draw_text(ix*(KEYS_W/COLUMNS)+1+4, keyboard_y+2+iy*(__keyboard_height/ROWS)+4, kbkeys_otherlow[iy][ix]);
			}
		}
	}

	void drawin()
    {
		int iy= __console_irow;
		int ix= start_column-start_index-1;

		for(int i=0; i<inlen; i++)
		{
			if (ix >= __console_charperw)
			{
				ix= 0; iy++;
			} else
				ix++;

			if (iy >= __console_charperh)
				return;

			draw_set_color(__keyboard_txtbgcol);//__console_bgcol);
			draw_rectangle(ix*__console_charw, iy*__console_charh-__console_yoffset, (ix+1)*__console_charw, (iy+1)*__console_charh-__console_yoffset, 0);
			draw_set_color(__keyboard_txtfgcol);//__console_fgcol);
			draw_char(ix*__console_charw, iy*__console_charh-__console_yoffset, instr[i]);
		}
	}

	redraw();
	drawin();

    u16 kd;

	while(1)
    {
		scanKeys();
		kd= keysDown();

        if (kd&KEY_LEFT) selx--;
		if (selx < 0) selx= COLUMNS-1;
		if (kd&KEY_RIGHT) selx++;
		if (selx >= COLUMNS) selx= 0;
		if (kd&KEY_UP) sely--;
		if (sely < 0) sely= ROWS-1;
		if (kd&KEY_DOWN) sely++;
		if (sely >= ROWS) sely= 0;

		if (kd&KEY_A)
        {
			char* buttstr= kbkeys_uppercase[sely][selx];

			if (kbcase == 1)
            {
                //Uppercase
				if (strcmp(buttstr,"v-") && strcmp(buttstr,"::") && strcmp(buttstr,"OK") && strcmp(buttstr,"..") && strcmp(buttstr,"__")) {

					instr[inindex] = buttstr[0];
					inlen++; inindex++;
					drawin();
					console_printf(".");
				}
			}
			if (kbcase == 0)
            {
                //Lowercase
				buttstr= kbkeys_lowercase[sely][selx];

				if (strcmp(buttstr,"^-") && strcmp(buttstr,"::") && strcmp(buttstr,"OK") && strcmp(buttstr,"..") && strcmp(buttstr,"__")) {

					instr[inindex] = buttstr[0];
					inlen++; inindex++;
					drawin();
					console_printf(".");
				}
			}
			if (kbcase == 3)
            {
                //Otherhigh
				buttstr= kbkeys_otherhigh[sely][selx];

				if (strcmp(buttstr,"v-") && strcmp(buttstr,"::") && strcmp(buttstr,"OK") && strcmp(buttstr,"AB") && strcmp(buttstr,"__"))
                {
					instr[inindex] = buttstr[0];
					inlen++; inindex++;
					drawin();
					console_printf(".");
				}
			}
			if (kbcase == 2)
            {
                //Otherlow
				buttstr= kbkeys_otherlow[sely][selx];

				if (strcmp(buttstr,"^-") && strcmp(buttstr,"::") && strcmp(buttstr,"OK") && strcmp(buttstr,"ab") && strcmp(buttstr,"__"))
                {

					instr[inindex] = buttstr[0];
					inlen++; inindex++;
					drawin();
					console_printf(".");
				}
			}

		  	if (!strcmp(buttstr,"__"))
            {
		  		//Add a space
		  		instr[inindex] = ' ';
				inlen++; inindex++;
				drawin();
				console_printf("-");
		  	}
		  	if (!strcmp(buttstr,"..") || !strcmp(buttstr,"AB") || !strcmp(buttstr,"ab"))
            {
		  		//switch to other keys to alphabet and viceversa
		  		if (kbcase == 0) kbcase= 2;
		  		else
				if (kbcase == 1) kbcase= 3;
				else
				if (kbcase == 2) kbcase= 0;
				else
				if (kbcase == 3) kbcase= 1;
				redraw();
		  	}
		}

		if (kd&KEY_B)
        {
			if (inindex > 0)
            {
				instr[inindex] = 0;
				if (inindex > 1)
                {
					console_moveback(1);
					console_printf(" ");
					console_moveback(2);
					console_printf(" ");
				}
				else
					console_moveback(1);

				draw_set_color(__console_bgcol);
				draw_rectangle(__console_icolumn*__console_charw, __console_irow*__console_charh-__console_yoffset, (__console_icolumn+1)*__console_charw, (__console_irow+1)*__console_charh-__console_yoffset, 0);

				inindex--; inlen--;
			}

			drawin();
		}

		if (kd&DPAD || kd&KEY_A)
			redraw();

		if (kd&KEY_SELECT)
        {
			if (kbcase == 0) kbcase= 1;
			else
			if (kbcase == 1) kbcase= 0;
			else
			if (kbcase == 2) kbcase= 3;
			else
			if (kbcase == 3) kbcase= 2;

			redraw();
		}

		if (kd&KEY_START)
        {

			console_moveback(inlen);

			instr[inlen]= '\0';

			kbstring= (char*)instr;
			kbstring_len= inlen;
			inindex=0;
			inlen= 0;

			if (__keyboard_autohide)
			{
				__keyboard_isopen= false;
				__console_yoffset= 0;
			}

			console_printf(kbstring);
			console_drawbuffer();

			if (__shell_activeproc == -1)
			{
				(*fnptr)(kbstring);
				console_printf(__com_promptstr);
			}

			kbstring_len= 0;

			__com_prompt_active= false;

			console_drawbuffer();

			if (__keyboard_autohide)
			{
				return;
			}

			redraw();
		}

		if (kd&(KEY_L|KEY_R))
        {
			console_moveback(inlen);

			instr[inlen]= '\0';

			kbstring= (char*)instr;
			kbstring_len= inlen;
			inindex= 0;
			inlen= 0;

			__console_yoffset= 0;
			console_printf(kbstring);
			console_drawbuffer();

			__keyboard_isopen= false;
			return;
		}

		console_idle();
		screen_wait_vsync();
	}
}

void keyboard_clear()
{
	kbstring= "";
	kbstring_len= 0;
}

#endif
