/*
===========================================================================
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

#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#define _STR(x) #x
#define STR(x) _STR(x)

// Current version of the multi player game

#define VERSION_MAJOR_RELEASE		22
#define VERSION_MINOR_RELEASE		11
#define VERSION_INTERNAL_BUILD		28

#define VERSION_STRING				"Year-22,Month-11,Day-28,BuildNum-06"
#define VERSION_STRING_DOTTED		"Year-22,Month-11,Day-28,BuildNum-06"

#if defined(_DEBUG)
#define	JK_VERSION		"(debug)SerenityJediEngine2023-MP: " VERSION_STRING_DOTTED
#define JK_VERSION_OLD	"(debug)JAmp: " VERSION_STRING_DOTTED
#else
#define	JK_VERSION		"SerenityJediEngine2023-MP: " VERSION_STRING_DOTTED
#define JK_VERSION_OLD	"JAmp: " VERSION_STRING_DOTTED
#endif
#endif // GAME_VERSION_H
