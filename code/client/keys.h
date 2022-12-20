/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once
#include "keycodes.h"

using qkey_t = struct qkey_s
{
	qboolean down;
	int repeats; // if > 1, it is autorepeating
	char* binding;
};

using keyGlobals_t = struct keyGlobals_s
{
	qboolean anykeydown;
	qboolean key_overstrikeMode;
	int keyDownCount;

	qkey_t keys[MAX_KEYS];
};

using keyname_t = struct keyname_s
{
	word upper, lower;
	const char* name;
	int keynum;
	bool menukey;
};

extern keyGlobals_t kg;
extern keyname_t keynames[MAX_KEYS];

// console
extern field_t g_consoleField;
extern int nextHistoryLine; // the last line in the history buffer, not masked
extern int historyLine; // the line being displayed from history buffer will be <= nextHistoryLine
extern field_t historyEditLines[COMMAND_HISTORY];

void Field_KeyDownEvent(field_t* edit, int key);
void Field_CharEvent(field_t* edit, int ch);
void Field_Draw(field_t* edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape);
void Field_BigDraw(field_t* edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape);

void Key_SetBinding(int keynum, const char* binding);
const char* Key_GetBinding(int keynum);
qboolean Key_IsDown(int keynum);
int Key_StringToKeynum(const char* str);
qboolean Key_GetOverstrikeMode(void);
void Key_SetOverstrikeMode(qboolean state);
void Key_ClearStates(void);
int Key_GetKey(const char* binding);
