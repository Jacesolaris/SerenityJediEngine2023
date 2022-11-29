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

// tr_image.c
#include "tr_local.h"

#include <map>

bool gServerSkinHack = false;

shader_t* R_FindServerShader(const char* name, const int* lightmapIndex, const byte* styles, qboolean mipRawImage);
static char* CommaParse(char** data_p);
/*
===============
RE_SplitSkins
input = skinname, possibly being a macro for three skins
return= true if three part skins found
output= qualified names to three skins if return is true, undefined if false
===============
*/
bool RE_SplitSkins(const char* INname, char* skinhead, char* skintorso, char* skinlower)
{	//INname= "models/players/jedi_tf/|head01_skin1|torso01|lower01";
	if (strchr(INname, '|'))
	{
		char name[MAX_QPATH];
		strcpy(name, INname);
		char* p = strchr(name, '|');
		*p = 0;
		p++;
		//fill in the base path
		strcpy(skinhead, name);
		strcpy(skintorso, name);
		strcpy(skinlower, name);

		//now get the the individual files

		//advance to second
		char* p2 = strchr(p, '|');
		assert(p2);
		if (!p2)
		{
			return false;
		}
		*p2 = 0;
		p2++;
		strcat(skinhead, p);
		strcat(skinhead, ".skin");

		//advance to third
		p = strchr(p2, '|');
		assert(p);
		if (!p)
		{
			return false;
		}
		*p = 0;
		p++;
		strcat(skintorso, p2);
		strcat(skintorso, ".skin");

		strcat(skinlower, p);
		strcat(skinlower, ".skin");

		return true;
	}
	return false;
}

// given a name, go get the skin we want and return
qhandle_t RE_RegisterIndividualSkin(const char* name, qhandle_t hSkin)
{
	char* text, * text_p;
	char			surfName[MAX_QPATH];

	// load and parse the skin file
	ri->FS_ReadFile(name, (void**)&text);
	if (!text) {
#ifndef FINAL_BUILD
		Com_Printf("WARNING: RE_RegisterSkin( '%s' ) failed to load!\n", name);
#endif
		return 0;
	}

	assert(tr.skins[hSkin]);	//should already be setup, but might be an 3part append

	skin_t* skin = tr.skins[hSkin];

	text_p = text;
	while (text_p && *text_p) {
		// get surface name
		const char* token = CommaParse(&text_p);
		Q_strncpyz(surfName, token, sizeof surfName);

		if (!token[0]) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr(surfName);

		if (*text_p == ',') {
			text_p++;
		}

		if (strncmp(token, "tag_", 4) == 0) {	//these aren't in there, but just in case you load an id style one...
			continue;
		}

		// parse the shader name
		token = CommaParse(&text_p);

		if (strcmp(&surfName[strlen(surfName) - 4], "_off") == 0)
		{
			if (strcmp(token, "*off") == 0)
			{
				continue;	//don't need these double offs
			}
			surfName[strlen(surfName) - 4] = 0;	//remove the "_off"
		}
		if (static_cast<int>(std::size(skin->surfaces)) <= skin->numSurfaces)
		{
			assert(static_cast<int>(sizeof skin->surfaces / sizeof skin->surfaces[0]) > skin->numSurfaces);
			Com_Printf("WARNING: RE_RegisterSkin( '%s' ) more than %u surfaces!\n", name, std::size(skin->surfaces));
			break;
		}
		skinSurface_t* surf = static_cast<skinSurface_t*>(Hunk_Alloc(sizeof * skin->surfaces[0], h_low));
		skin->surfaces[skin->numSurfaces] = (_skinSurface_t*)surf;

		Q_strncpyz(surf->name, surfName, sizeof surf->name);

		if (gServerSkinHack)	surf->shader = R_FindServerShader(token, lightmapsNone, stylesDefault, qtrue);
		else					surf->shader = R_FindShader(token, lightmapsNone, stylesDefault, qtrue);
		skin->numSurfaces++;
	}

	ri->FS_FreeFile(text);

	// never let a skin have 0 shaders
	if (skin->numSurfaces == 0) {
		return 0;		// use default skin
	}

	return hSkin;
}

qhandle_t RE_RegisterSkin(const char* name) {
	qhandle_t	hSkin;
	skin_t* skin;

	if (!name || !name[0]) {
		Com_Printf("Empty name passed to RE_RegisterSkin\n");
		return 0;
	}

	if (strlen(name) >= MAX_QPATH) {
		Com_Printf("Skin name exceeds MAX_QPATH\n");
		return 0;
	}

	// see if the skin is already loaded
	for (hSkin = 1; hSkin < tr.numSkins; hSkin++) {
		skin = tr.skins[hSkin];
		if (!Q_stricmp(skin->name, name)) {
			if (skin->numSurfaces == 0) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if (tr.numSkins == MAX_SKINS) {
		Com_Printf("WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name);
		return 0;
	}
	tr.numSkins++;
	skin = static_cast<skin_s*>(Hunk_Alloc(sizeof(skin_t), h_low));
	tr.skins[hSkin] = skin;
	Q_strncpyz(skin->name, name, sizeof skin->name);
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	// If not a .skin file, load as a single shader
	if (strcmp(name + strlen(name) - 5, ".skin") != 0) {
		/*		skin->numSurfaces = 1;
				skin->surfaces[0] = (skinSurface_t *)Hunk_Alloc( sizeof(skin->surfaces[0]), h_low );
				skin->surfaces[0]->shader = R_FindShader( name, lightmapsNone, stylesDefault, qtrue );
				return hSkin;
		*/
	}

	char skinhead[MAX_QPATH] = { 0 };
	char skintorso[MAX_QPATH] = { 0 };
	char skinlower[MAX_QPATH] = { 0 };
	if (RE_SplitSkins(name, (char*)&skinhead, (char*)&skintorso, (char*)&skinlower))
	{//three part
		hSkin = RE_RegisterIndividualSkin(skinhead, hSkin);
		if (hSkin)
		{
			hSkin = RE_RegisterIndividualSkin(skintorso, hSkin);
			if (hSkin)
			{
				hSkin = RE_RegisterIndividualSkin(skinlower, hSkin);
			}
		}
	}
	else
	{//single skin
		hSkin = RE_RegisterIndividualSkin(name, hSkin);
	}
	return hSkin;
}

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatible with our normal parsing rules.
==================
*/
static char* CommaParse(char** data_p) {
	int c = 0;
	static	char	com_token[MAX_TOKEN_CHARS];

	char* data = *data_p;
	int len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if (!data) {
		*data_p = nullptr;
		return com_token;
	}

	while (true) {
		// skip whitespace
		while ((c = *(const unsigned char* /*eurofix*/)data) <= ' ') {
			if (!c) {
				break;
			}
			data++;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			while (*data && (*data != '*' || data[1] != '/'))
			{
				data++;
			}
			if (*data)
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if (c == 0) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (true)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c > 32 && c != ',');

	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

/*
===============
RE_RegisterServerSkin

Mangled version of the above function to load .skin files on the server.
===============
*/
qhandle_t RE_RegisterServerSkin(const char* name) {
	if (ri->Cvar_VariableIntegerValue("cl_running") &&
		ri->Com_TheHunkMarkHasBeenMade() &&
		ShaderHashTableExists())
	{ //If the client is running then we can go straight into the normal registerskin func
		return RE_RegisterSkin(name);
	}

	gServerSkinHack = true;
	const qhandle_t r = RE_RegisterSkin(name);
	gServerSkinHack = false;

	return r;
}

/*
===============
R_InitSkins
===============
*/
void	R_InitSkins(void) {
	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin_t* skin = tr.skins[0] = static_cast<skin_s*>(ri->Hunk_Alloc(sizeof(skin_t), h_low));
	Q_strncpyz(skin->name, "<default skin>", sizeof skin->name);
	skin->numSurfaces = 1;
	skin->surfaces[0] = static_cast<_skinSurface_t*>(ri->Hunk_Alloc(sizeof(skinSurface_t), h_low));
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t* R_GetSkinByHandle(qhandle_t hSkin) {
	if (hSkin < 1 || hSkin >= tr.numSkins) {
		return tr.skins[0];
	}
	return tr.skins[hSkin];
}