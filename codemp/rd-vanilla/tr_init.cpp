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

// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

#include <algorithm>
#include "../rd-common/tr_common.h"
#include "tr_WorldEffects.h"
#include "ghoul2/g2_local.h"

glconfig_t	glConfig;
glconfigExt_t glConfigExt;
glstate_t	glState;
window_t	window;

static void GfxInfo_f();

cvar_t* r_verbose;
cvar_t* r_ignore;

cvar_t* r_detailTextures;

cvar_t* r_znear;

cvar_t* r_skipBackEnd;

cvar_t* r_measureOverdraw;

cvar_t* r_inGameVideo;
cvar_t* r_fastsky;
cvar_t* r_drawSun;
cvar_t* r_dynamiclight;
// rjr - removed for hacking cvar_t	*r_dlightBacks;

cvar_t* r_lodbias;
cvar_t* r_lodscale;
cvar_t* r_autolodscalevalue;

cvar_t* r_norefresh;
cvar_t* r_drawentities;
cvar_t* r_drawworld;
cvar_t* r_drawfog;
cvar_t* r_speeds;
cvar_t* r_fullbright;
cvar_t* r_novis;
cvar_t* r_nocull;
cvar_t* r_facePlaneCull;
cvar_t* r_cullRoofFaces; //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
cvar_t* r_roofCullCeilDist; //ceiling distance cull tolerance -rww
cvar_t* r_roofCullFloorDist; //floor distance cull tolerance -rww
cvar_t* r_showcluster;
cvar_t* r_nocurves;

cvar_t* r_autoMap; //automap renderside toggle for debugging -rww
cvar_t* r_autoMapBackAlpha; //alpha of automap bg -rww
cvar_t* r_autoMapDisable; //don't calc it (since it's slow in debug) -rww

cvar_t* r_dlightStyle;
cvar_t* r_surfaceSprites;
cvar_t* r_surfaceWeather;

cvar_t* r_windSpeed;
cvar_t* r_windAngle;
cvar_t* r_windGust;
cvar_t* r_windDampFactor;
cvar_t* r_windPointForce;
cvar_t* r_windPointX;
cvar_t* r_windPointY;

cvar_t* r_allowExtensions;

cvar_t* r_ext_compressed_textures;
cvar_t* r_ext_compressed_lightmaps;
cvar_t* r_ext_preferred_tc_method;
cvar_t* r_ext_gamma_control;
cvar_t* r_ext_multitexture;
cvar_t* r_ext_compiled_vertex_array;
cvar_t* r_ext_texture_env_add;
cvar_t* r_ext_texture_filter_anisotropic;
cvar_t* r_gammaShaders;

cvar_t* r_environmentMapping;

cvar_t* r_DynamicGlow;
cvar_t* r_DynamicGlowPasses;
cvar_t* r_DynamicGlowDelta;
cvar_t* r_DynamicGlowIntensity;
cvar_t* r_DynamicGlowSoft;
cvar_t* r_DynamicGlowWidth;
cvar_t* r_DynamicGlowHeight;

cvar_t* r_ignoreGLErrors;
cvar_t* r_logFile;

cvar_t* r_primitives;
cvar_t* r_texturebits;
cvar_t* r_texturebitslm;

cvar_t* r_lightmap;
cvar_t* r_vertexLight;
cvar_t* r_uiFullScreen;
cvar_t* r_shadows;
cvar_t* r_shadowRange;
cvar_t* r_flares;
cvar_t* r_nobind;
cvar_t* r_singleShader;
cvar_t* r_colorMipLevels;
cvar_t* r_picmip;
cvar_t* r_showtris;
cvar_t* r_showsky;
cvar_t* r_shownormals;
cvar_t* r_finish;
cvar_t* r_clear;
cvar_t* r_markcount;
cvar_t* r_textureMode;
cvar_t* r_offsetFactor;
cvar_t* r_offsetUnits;
cvar_t* r_gamma;
cvar_t* r_intensity;
cvar_t* r_lockpvs;
cvar_t* r_noportals;
cvar_t* r_portalOnly;

cvar_t* r_subdivisions;
cvar_t* r_lodCurveError;

cvar_t* r_overBrightBits;
cvar_t* r_mapOverBrightBits;

cvar_t* r_debugSurface;
cvar_t* r_simpleMipMaps;

cvar_t* r_showImages;

cvar_t* r_ambientScale;
cvar_t* r_directedScale;
cvar_t* r_debugLight;
cvar_t* r_debugSort;

cvar_t* r_marksOnTriangleMeshes;

cvar_t* r_aspectCorrectFonts;

// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define	DEFAULT_MAX_POLYS		600
#define	DEFAULT_MAX_POLYVERTS	3000
cvar_t* r_maxpolys;
cvar_t* r_maxpolyverts;
int		max_polys;
int		max_polyverts;

cvar_t* r_modelpoolmegs;

/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
cvar_t* r_noPrecacheGLA;
#endif

cvar_t* r_noServerGhoul2;
cvar_t* r_Ghoul2AnimSmooth = nullptr;
cvar_t* r_Ghoul2UnSqashAfterSmooth = nullptr;
//cvar_t	*r_Ghoul2UnSqash;
//cvar_t	*r_Ghoul2TimeBase=0; from single player
//cvar_t	*r_Ghoul2NoLerp;
//cvar_t	*r_Ghoul2NoBlend;
//cvar_t	*r_Ghoul2BlendMultiplier=0;

cvar_t* broadsword = nullptr;
cvar_t* broadsword_kickbones = nullptr;
cvar_t* broadsword_kickorigin = nullptr;
cvar_t* broadsword_playflop = nullptr;
cvar_t* broadsword_dontstopanim = nullptr;
cvar_t* broadsword_waitforshot = nullptr;
cvar_t* broadsword_smallbbox = nullptr;
cvar_t* broadsword_extra1 = nullptr;
cvar_t* broadsword_extra2 = nullptr;

cvar_t* broadsword_effcorr = nullptr;
cvar_t* broadsword_ragtobase = nullptr;
cvar_t* broadsword_dircap = nullptr;

/*
Ghoul2 Insert End
*/

cvar_t* se_language;
cvar_t* r_weather;

cvar_t* r_aviMotionJpegQuality;
cvar_t* r_screenshotJpegQuality;

PFNGLACTIVETEXTUREARBPROC qglActiveTextureARB;
PFNGLCLIENTACTIVETEXTUREARBPROC qglClientActiveTextureARB;
PFNGLMULTITEXCOORD2FARBPROC qglMultiTexCoord2fARB;
#if !defined(__APPLE__)
PFNGLTEXIMAGE3DPROC qglTexImage3D;
PFNGLTEXSUBIMAGE3DPROC qglTexSubImage3D;
#endif

PFNGLCOMBINERPARAMETERFVNVPROC qglCombinerParameterfvNV;
PFNGLCOMBINERPARAMETERIVNVPROC qglCombinerParameterivNV;
PFNGLCOMBINERPARAMETERFNVPROC qglCombinerParameterfNV;
PFNGLCOMBINERPARAMETERINVPROC qglCombinerParameteriNV;
PFNGLCOMBINERINPUTNVPROC qglCombinerInputNV;
PFNGLCOMBINEROUTPUTNVPROC qglCombinerOutputNV;

PFNGLFINALCOMBINERINPUTNVPROC qglFinalCombinerInputNV;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC qglGetCombinerInputParameterfvNV;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC qglGetCombinerInputParameterivNV;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC qglGetCombinerOutputParameterfvNV;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC qglGetCombinerOutputParameterivNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC qglGetFinalCombinerInputParameterfvNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC qglGetFinalCombinerInputParameterivNV;

PFNGLPROGRAMSTRINGARBPROC qglProgramStringARB;
PFNGLBINDPROGRAMARBPROC qglBindProgramARB;
PFNGLDELETEPROGRAMSARBPROC qglDeleteProgramsARB;
PFNGLGENPROGRAMSARBPROC qglGenProgramsARB;
PFNGLPROGRAMENVPARAMETER4DARBPROC qglProgramEnvParameter4dARB;
PFNGLPROGRAMENVPARAMETER4DVARBPROC qglProgramEnvParameter4dvARB;
PFNGLPROGRAMENVPARAMETER4FARBPROC qglProgramEnvParameter4fARB;
PFNGLPROGRAMENVPARAMETER4FVARBPROC qglProgramEnvParameter4fvARB;
PFNGLPROGRAMLOCALPARAMETER4DARBPROC qglProgramLocalParameter4dARB;
PFNGLPROGRAMLOCALPARAMETER4DVARBPROC qglProgramLocalParameter4dvARB;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC qglProgramLocalParameter4fvARB;
PFNGLGETPROGRAMENVPARAMETERDVARBPROC qglGetProgramEnvParameterdvARB;
PFNGLGETPROGRAMENVPARAMETERFVARBPROC qglGetProgramEnvParameterfvARB;
PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC qglGetProgramLocalParameterdvARB;
PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC qglGetProgramLocalParameterfvARB;
PFNGLGETPROGRAMIVARBPROC qglGetProgramivARB;
PFNGLGETPROGRAMSTRINGARBPROC qglGetProgramStringARB;
PFNGLISPROGRAMARBPROC qglIsProgramARB;

PFNGLLOCKARRAYSEXTPROC qglLockArraysEXT;
PFNGLUNLOCKARRAYSEXTPROC qglUnlockArraysEXT;

bool g_bTextureRectangleHack = false;

void RE_SetLightStyle(int style, int color);
void RE_GetBModelVerts(int bmodelIndex, vec3_t* verts, vec3_t normal);

void R_Splash()
{
	image_t* p_image;
	const int splash_pick = rand() % 5;

	switch (splash_pick)
	{
	case 0:
		p_image = R_FindImageFile("menu/splash", qfalse, qfalse, qfalse, GL_CLAMP);
		break;
	case 1:
		p_image = R_FindImageFile("menu/splash2", qfalse, qfalse, qfalse, GL_CLAMP);
		break;
	case 2:
		p_image = R_FindImageFile("menu/splash3", qfalse, qfalse, qfalse, GL_CLAMP);
		break;
	case 3:
		p_image = R_FindImageFile("menu/splash4", qfalse, qfalse, qfalse, GL_CLAMP);
		break;
	case 4:
		p_image = R_FindImageFile("menu/splash5", qfalse, qfalse, qfalse, GL_CLAMP);
		break;
	default:
		p_image = R_FindImageFile("menu/splash", qfalse, qfalse, qfalse, GL_CLAMP);
		break;
	}

	extern void	RB_SetGL2D();
	RB_SetGL2D();

	if (p_image)
	{//invalid paths?
		GL_Bind(p_image);
	}
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);

	constexpr int width = 640;
	constexpr int height = 480;
	constexpr float x1 = 320 - width / 2;
	constexpr float x2 = 320 + width / 2;
	constexpr float y1 = 240 - height / 2;
	constexpr float y2 = 240 + height / 2;

	qglBegin(GL_TRIANGLE_STRIP);
	qglTexCoord2f(0, 0);
	qglVertex2f(x1, y1);
	qglTexCoord2f(1, 0);
	qglVertex2f(x2, y1);
	qglTexCoord2f(0, 1);
	qglVertex2f(x1, y2);
	qglTexCoord2f(1, 1);
	qglVertex2f(x2, y2);
	qglEnd();

	ri->WIN_Present(&window);
}

/*
** GLW_CheckForExtension

  Cannot use strstr directly to differentiate between (for eg) reg_combiners and reg_combiners2
*/

static void GLW_InitTextureCompression()
{
	// Check for available tc methods.
	const bool newer_tc = ri->GL_ExtensionSupported("GL_ARB_texture_compression") && ri->GL_ExtensionSupported(
		"GL_EXT_texture_compression_s3tc");
	const bool old_tc = ri->GL_ExtensionSupported("GL_S3_s3tc");

	if (old_tc)
	{
		Com_Printf("...GL_S3_s3tc available\n");
	}

	if (newer_tc)
	{
		Com_Printf("...GL_EXT_texture_compression_s3tc available\n");
	}

	if (!r_ext_compressed_textures->value)
	{
		// Compressed textures are off
		glConfig.textureCompression = TC_NONE;
		Com_Printf("...ignoring texture compression\n");
	}
	else if (!old_tc && !newer_tc)
	{
		// Requesting texture compression, but no method found
		glConfig.textureCompression = TC_NONE;
		Com_Printf("...no supported texture compression method found\n");
		Com_Printf(".....ignoring texture compression\n");
	}
	else
	{
		// some form of supported texture compression is avaiable, so see if the user has a preference
		if (r_ext_preferred_tc_method->integer == TC_NONE)
		{
			// No preference, so pick the best
			if (newer_tc)
			{
				Com_Printf("...no tc preference specified\n");
				Com_Printf(".....using GL_EXT_texture_compression_s3tc\n");
				glConfig.textureCompression = TC_S3TC_DXT;
			}
			else
			{
				Com_Printf("...no tc preference specified\n");
				Com_Printf(".....using GL_S3_s3tc\n");
				glConfig.textureCompression = TC_S3TC;
			}
		}
		else
		{
			// User has specified a preference, now see if this request can be honored
			if (old_tc && newer_tc)
			{
				// both are avaiable, so we can use the desired tc method
				if (r_ext_preferred_tc_method->integer == TC_S3TC)
				{
					Com_Printf("...using preferred tc method, GL_S3_s3tc\n");
					glConfig.textureCompression = TC_S3TC;
				}
				else
				{
					Com_Printf("...using preferred tc method, GL_EXT_texture_compression_s3tc\n");
					glConfig.textureCompression = TC_S3TC_DXT;
				}
			}
			else
			{
				// Both methods are not available, so this gets trickier
				if (r_ext_preferred_tc_method->integer == TC_S3TC)
				{
					// Preferring to user older compression
					if (old_tc)
					{
						Com_Printf("...using GL_S3_s3tc\n");
						glConfig.textureCompression = TC_S3TC;
					}
					else
					{
						// Drat, preference can't be honored
						Com_Printf("...preferred tc method, GL_S3_s3tc not available\n");
						Com_Printf(".....falling back to GL_EXT_texture_compression_s3tc\n");
						glConfig.textureCompression = TC_S3TC_DXT;
					}
				}
				else
				{
					// Preferring to user newer compression
					if (newer_tc)
					{
						Com_Printf("...using GL_EXT_texture_compression_s3tc\n");
						glConfig.textureCompression = TC_S3TC_DXT;
					}
					else
					{
						// Drat, preference can't be honored
						Com_Printf("...preferred tc method, GL_EXT_texture_compression_s3tc not available\n");
						Com_Printf(".....falling back to GL_S3_s3tc\n");
						glConfig.textureCompression = TC_S3TC;
					}
				}
			}
		}
	}
}

/*
===============
GLimp_InitExtensions
===============
*/
extern bool g_bDynamicGlowSupported;
extern bool g_bARBShadersAvailable;
static void GLimp_InitExtensions()
{
	if (!r_allowExtensions->integer)
	{
		Com_Printf("*** IGNORING OPENGL EXTENSIONS ***\n");
		g_bDynamicGlowSupported = false;
		ri->Cvar_Set("r_DynamicGlow", "0");
		return;
	}

	Com_Printf("Initializing OpenGL extensions\n");

	// Select our tc scheme
	GLW_InitTextureCompression();

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if (ri->GL_ExtensionSupported("GL_EXT_texture_env_add"))
	{
		if (r_ext_texture_env_add->integer)
		{
			glConfig.textureEnvAddAvailable = qtrue;
			Com_Printf("...using GL_EXT_texture_env_add\n");
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			Com_Printf("...ignoring GL_EXT_texture_env_add\n");
		}
	}
	else
	{
		Com_Printf("...GL_EXT_texture_env_add not found\n");
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig.maxTextureFilterAnisotropy = 0;
	if (ri->GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic"))
	{
		qglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropy);
		Com_Printf("...GL_EXT_texture_filter_anisotropic available\n");

		if (r_ext_texture_filter_anisotropic->integer > 1)
		{
			Com_Printf("...using GL_EXT_texture_filter_anisotropic\n");
		}
		else
		{
			Com_Printf("...ignoring GL_EXT_texture_filter_anisotropic\n");
		}
		ri->Cvar_SetValue("r_ext_texture_filter_anisotropic_avail", glConfig.maxTextureFilterAnisotropy);
		if (r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy)
		{
			ri->Cvar_SetValue("r_ext_texture_filter_anisotropic_avail", glConfig.maxTextureFilterAnisotropy);
		}
	}
	else
	{
		Com_Printf("...GL_EXT_texture_filter_anisotropic not found\n");
		ri->Cvar_Set("r_ext_texture_filter_anisotropic_avail", "0");
	}

	// GL_EXT_clamp_to_edge
	glConfig.clampToEdgeAvailable = qtrue;
	Com_Printf("...using GL_EXT_texture_edge_clamp\n");

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = nullptr;
	qglActiveTextureARB = nullptr;
	qglClientActiveTextureARB = nullptr;
	if (ri->GL_ExtensionSupported("GL_ARB_multitexture"))
	{
		if (r_ext_multitexture->integer)
		{
			qglMultiTexCoord2fARB = static_cast<PFNGLMULTITEXCOORD2FARBPROC>(ri->GL_GetProcAddress("glMultiTexCoord2fARB"));
			qglActiveTextureARB = static_cast<PFNGLACTIVETEXTUREARBPROC>(ri->GL_GetProcAddress("glActiveTextureARB"));
			qglClientActiveTextureARB = static_cast<PFNGLCLIENTACTIVETEXTUREARBPROC>(ri->GL_GetProcAddress("glClientActiveTextureARB"));

			if (qglActiveTextureARB)
			{
				qglGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glConfig.maxActiveTextures);

				if (glConfig.maxActiveTextures > 1)
				{
					Com_Printf("...using GL_ARB_multitexture\n");
				}
				else
				{
					qglMultiTexCoord2fARB = nullptr;
					qglActiveTextureARB = nullptr;
					qglClientActiveTextureARB = nullptr;
					Com_Printf("...not using GL_ARB_multitexture, < 2 texture units\n");
				}
			}
		}
		else
		{
			Com_Printf("...ignoring GL_ARB_multitexture\n");
		}
	}
	else
	{
		Com_Printf("...GL_ARB_multitexture not found\n");
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = nullptr;
	qglUnlockArraysEXT = nullptr;
	if (ri->GL_ExtensionSupported("GL_EXT_compiled_vertex_array"))
	{
		if (r_ext_compiled_vertex_array->integer)
		{
			Com_Printf("...using GL_EXT_compiled_vertex_array\n");
			qglLockArraysEXT = static_cast<PFNGLLOCKARRAYSEXTPROC>(ri->GL_GetProcAddress("glLockArraysEXT"));
			qglUnlockArraysEXT = static_cast<PFNGLUNLOCKARRAYSEXTPROC>(ri->GL_GetProcAddress("glUnlockArraysEXT"));
			if (!qglLockArraysEXT || !qglUnlockArraysEXT) {
				Com_Error(ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			Com_Printf("...ignoring GL_EXT_compiled_vertex_array\n");
		}
	}
	else
	{
		Com_Printf("...GL_EXT_compiled_vertex_array not found\n");
	}

	bool b_nv_register_combiners;
	// Register Combiners.
	if (ri->GL_ExtensionSupported("GL_NV_register_combiners"))
	{
		// NOTE: This extension requires multitexture support (over 2 units).
		if (glConfig.maxActiveTextures >= 2)
		{
			b_nv_register_combiners = true;
			// Register Combiners function pointer address load.	- AReis
			// NOTE: VV guys will _definetly_ not be able to use regcoms. Pixel Shaders are just as good though :-)
			// NOTE: Also, this is an nVidia specific extension (of course), so fragment shaders would serve the same purpose
			// if we needed some kind of fragment/pixel manipulation support.
			qglCombinerParameterfvNV = static_cast<PFNGLCOMBINERPARAMETERFVNVPROC>(ri->GL_GetProcAddress("glCombinerParameterfvNV"));
			qglCombinerParameterivNV = static_cast<PFNGLCOMBINERPARAMETERIVNVPROC>(ri->GL_GetProcAddress("glCombinerParameterivNV"));
			qglCombinerParameterfNV = static_cast<PFNGLCOMBINERPARAMETERFNVPROC>(ri->GL_GetProcAddress("glCombinerParameterfNV"));
			qglCombinerParameteriNV = static_cast<PFNGLCOMBINERPARAMETERINVPROC>(ri->GL_GetProcAddress("glCombinerParameteriNV"));
			qglCombinerInputNV = static_cast<PFNGLCOMBINERINPUTNVPROC>(ri->GL_GetProcAddress("glCombinerInputNV"));
			qglCombinerOutputNV = static_cast<PFNGLCOMBINEROUTPUTNVPROC>(ri->GL_GetProcAddress("glCombinerOutputNV"));
			qglFinalCombinerInputNV = static_cast<PFNGLFINALCOMBINERINPUTNVPROC>(ri->GL_GetProcAddress("glFinalCombinerInputNV"));
			qglGetCombinerInputParameterfvNV = static_cast<PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC>(ri->GL_GetProcAddress("glGetCombinerInputParameterfvNV"));
			qglGetCombinerInputParameterivNV = static_cast<PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC>(ri->GL_GetProcAddress("glGetCombinerInputParameterivNV"));
			qglGetCombinerOutputParameterfvNV = static_cast<PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC>(ri->GL_GetProcAddress("glGetCombinerOutputParameterfvNV"));
			qglGetCombinerOutputParameterivNV = static_cast<PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC>(ri->GL_GetProcAddress("glGetCombinerOutputParameterivNV"));
			qglGetFinalCombinerInputParameterfvNV = static_cast<PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC>(ri->GL_GetProcAddress("glGetFinalCombinerInputParameterfvNV"));
			qglGetFinalCombinerInputParameterivNV = static_cast<PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC>(ri->GL_GetProcAddress("glGetFinalCombinerInputParameterivNV"));

			// Validate the functions we need.
			if (!qglCombinerParameterfvNV || !qglCombinerParameterivNV || !qglCombinerParameterfNV || !qglCombinerParameteriNV || !qglCombinerInputNV ||
				!qglCombinerOutputNV || !qglFinalCombinerInputNV || !qglGetCombinerInputParameterfvNV || !qglGetCombinerInputParameterivNV ||
				!qglGetCombinerOutputParameterfvNV || !qglGetCombinerOutputParameterivNV || !qglGetFinalCombinerInputParameterfvNV || !qglGetFinalCombinerInputParameterivNV)
			{
				b_nv_register_combiners = false;
				qglCombinerParameterfvNV = nullptr;
				qglCombinerParameteriNV = nullptr;
				Com_Printf("...GL_NV_register_combiners failed\n");
			}
		}
		else
		{
			b_nv_register_combiners = false;
			Com_Printf("...ignoring GL_NV_register_combiners\n");
		}
	}
	else
	{
		b_nv_register_combiners = false;
		Com_Printf("...GL_NV_register_combiners not found\n");
	}

	// NOTE: Vertex and Fragment Programs are very dependant on each other - this is actually a
	// good thing! So, just check to see which we support (one or the other) and load the shared
	// function pointers. ARB rocks!

	// Vertex Programs.
	bool b_arb_vertex_program;
	if (ri->GL_ExtensionSupported("GL_ARB_vertex_program"))
	{
		b_arb_vertex_program = true;
	}
	else
	{
		b_arb_vertex_program = false;
		Com_Printf("...GL_ARB_vertex_program not found\n");
	}

	// Fragment Programs.
	bool b_arb_fragment_program;
	if (ri->GL_ExtensionSupported("GL_ARB_fragment_program"))
	{
		b_arb_fragment_program = true;
	}
	else
	{
		b_arb_fragment_program = false;
		Com_Printf("...GL_ARB_fragment_program not found\n");
	}

	// If we support one or the other, load the shared function pointers.
	if (b_arb_vertex_program || b_arb_fragment_program)
	{
		qglProgramStringARB = static_cast<PFNGLPROGRAMSTRINGARBPROC>(ri->GL_GetProcAddress("glProgramStringARB"));
		qglBindProgramARB = static_cast<PFNGLBINDPROGRAMARBPROC>(ri->GL_GetProcAddress("glBindProgramARB"));
		qglDeleteProgramsARB = static_cast<PFNGLDELETEPROGRAMSARBPROC>(ri->GL_GetProcAddress("glDeleteProgramsARB"));
		qglGenProgramsARB = static_cast<PFNGLGENPROGRAMSARBPROC>(ri->GL_GetProcAddress("glGenProgramsARB"));
		qglProgramEnvParameter4dARB = static_cast<PFNGLPROGRAMENVPARAMETER4DARBPROC>(ri->GL_GetProcAddress("glProgramEnvParameter4dARB"));
		qglProgramEnvParameter4dvARB = static_cast<PFNGLPROGRAMENVPARAMETER4DVARBPROC>(ri->GL_GetProcAddress("glProgramEnvParameter4dvARB"));
		qglProgramEnvParameter4fARB = static_cast<PFNGLPROGRAMENVPARAMETER4FARBPROC>(ri->GL_GetProcAddress("glProgramEnvParameter4fARB"));
		qglProgramEnvParameter4fvARB = static_cast<PFNGLPROGRAMENVPARAMETER4FVARBPROC>(ri->GL_GetProcAddress("glProgramEnvParameter4fvARB"));
		qglProgramLocalParameter4dARB = static_cast<PFNGLPROGRAMLOCALPARAMETER4DARBPROC>(ri->GL_GetProcAddress("glProgramLocalParameter4dARB"));
		qglProgramLocalParameter4dvARB = static_cast<PFNGLPROGRAMLOCALPARAMETER4DVARBPROC>(ri->GL_GetProcAddress("glProgramLocalParameter4dvARB"));
		qglProgramLocalParameter4fARB = static_cast<PFNGLPROGRAMLOCALPARAMETER4FARBPROC>(ri->GL_GetProcAddress("glProgramLocalParameter4fARB"));
		qglProgramLocalParameter4fvARB = static_cast<PFNGLPROGRAMLOCALPARAMETER4FVARBPROC>(ri->GL_GetProcAddress("glProgramLocalParameter4fvARB"));
		qglGetProgramEnvParameterdvARB = static_cast<PFNGLGETPROGRAMENVPARAMETERDVARBPROC>(ri->GL_GetProcAddress("glGetProgramEnvParameterdvARB"));
		qglGetProgramEnvParameterfvARB = static_cast<PFNGLGETPROGRAMENVPARAMETERFVARBPROC>(ri->GL_GetProcAddress("glGetProgramEnvParameterfvARB"));
		qglGetProgramLocalParameterdvARB = static_cast<PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC>(ri->GL_GetProcAddress("glGetProgramLocalParameterdvARB"));
		qglGetProgramLocalParameterfvARB = static_cast<PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC>(ri->GL_GetProcAddress("glGetProgramLocalParameterfvARB"));
		qglGetProgramivARB = static_cast<PFNGLGETPROGRAMIVARBPROC>(ri->GL_GetProcAddress("glGetProgramivARB"));
		qglGetProgramStringARB = static_cast<PFNGLGETPROGRAMSTRINGARBPROC>(ri->GL_GetProcAddress("glGetProgramStringARB"));
		qglIsProgramARB = static_cast<PFNGLISPROGRAMARBPROC>(ri->GL_GetProcAddress("glIsProgramARB"));

		// Validate the functions we need.
		if (!qglProgramStringARB || !qglBindProgramARB || !qglDeleteProgramsARB || !qglGenProgramsARB ||
			!qglProgramEnvParameter4dARB || !qglProgramEnvParameter4dvARB || !qglProgramEnvParameter4fARB ||
			!qglProgramEnvParameter4fvARB || !qglProgramLocalParameter4dARB || !qglProgramLocalParameter4dvARB ||
			!qglProgramLocalParameter4fARB || !qglProgramLocalParameter4fvARB || !qglGetProgramEnvParameterdvARB ||
			!qglGetProgramEnvParameterfvARB || !qglGetProgramLocalParameterdvARB || !qglGetProgramLocalParameterfvARB ||
			!qglGetProgramivARB || !qglGetProgramStringARB || !qglIsProgramARB)
		{
			b_arb_vertex_program = false;
			b_arb_fragment_program = false;
			qglGenProgramsARB = nullptr;	//clear ptrs that get checked
			qglProgramEnvParameter4fARB = nullptr;
			Com_Printf("...ignoring GL_ARB_vertex_program\n");
			Com_Printf("...ignoring GL_ARB_fragment_program\n");
		}
	}

	// Figure out which texture rectangle extension to use.
	bool b_tex_rect_supported = false;
	if (Q_stricmpn(glConfig.vendor_string, "Advanced Micro Devices", 16) == 0
		&& Q_stricmpn(glConfig.version_string, "Year-22,Month-12,Day-13,BuildNum-07", 5) == 0
		&& glConfig.version_string[5] < '9') //1.3.34 and 1.3.37 and 1.3.38 are broken for sure, 1.3.39 is not
	{
		g_bTextureRectangleHack = true;
	}

	if (ri->GL_ExtensionSupported("GL_NV_texture_rectangle") || ri->GL_ExtensionSupported("GL_EXT_texture_rectangle"))
	{
		b_tex_rect_supported = true;
	}

	// Find out how many general combiners they have.
#define GL_MAX_GENERAL_COMBINERS_NV       0x854D
	GLint i_num_general_combiners = 0;
	if (b_nv_register_combiners)
		qglGetIntegerv(GL_MAX_GENERAL_COMBINERS_NV, &i_num_general_combiners);

	glConfigExt.doGammaCorrectionWithShaders = qfalse;
	if (r_gammaShaders->integer && qglActiveTextureARB && b_tex_rect_supported && b_arb_vertex_program && b_arb_fragment_program)
	{
#if !defined(__APPLE__)
		qglTexImage3D = static_cast<PFNGLTEXIMAGE3DPROC>(ri->GL_GetProcAddress("glTexImage3D"));
		qglTexSubImage3D = static_cast<PFNGLTEXSUBIMAGE3DPROC>(ri->GL_GetProcAddress("glTexSubImage3D"));
		if (qglTexImage3D && qglTexSubImage3D)
		{
			glConfigExt.doGammaCorrectionWithShaders = qtrue;
		}
#else
		glConfigExt.doGammaCorrectionWithShaders = qtrue;
#endif
	}

	// Only allow dynamic glows/flares if they have the hardware
	if (b_tex_rect_supported && b_arb_vertex_program && qglActiveTextureARB && glConfig.maxActiveTextures >= 4 &&
		(b_nv_register_combiners && i_num_general_combiners >= 2 || b_arb_fragment_program))
	{
		g_bDynamicGlowSupported = true;
	}
	else
	{
		g_bDynamicGlowSupported = false;
		ri->Cvar_Set("r_DynamicGlow", "0");
	}
}

// Truncates the GL extensions string by only allowing up to 'maxExtensions' extensions in the string.
static const char* TruncateGLExtensionsString(const char* extensions_string, const int max_extensions)
{
	const char* p = extensions_string;
	const char* q;
	int num_extensions = 0;
	size_t extensions_len = strlen(extensions_string);

	while ((q = strchr(p, ' ')) != nullptr && num_extensions <= max_extensions)
	{
		p = q + 1;
		num_extensions++;
	}

	if (q != nullptr)
	{
		// We still have more extensions. We'll call this the end

		extensions_len = p - extensions_string - 1;
	}

	const auto truncated_extensions = static_cast<char*>(Hunk_Alloc(extensions_len + 1, h_low));
	Q_strncpyz(truncated_extensions, extensions_string, extensions_len + 1);

	return truncated_extensions;
}

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL()
{
	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//

	if (glConfig.vidWidth == 0)
	{
		constexpr windowDesc_t window_desc = { GRAPHICS_API_OPENGL };
		memset(&glConfig, 0, sizeof glConfig);
		memset(&glConfigExt, 0, sizeof glConfigExt);

		window = ri->WIN_Init(&window_desc, &glConfig);

		Com_Printf("GL_RENDERER: %s\n", (char*)qglGetString(GL_RENDERER));

		// get our config strings
		glConfig.vendor_string = reinterpret_cast<const char*>(qglGetString(GL_VENDOR));
		glConfig.renderer_string = reinterpret_cast<const char*>(qglGetString(GL_RENDERER));
		glConfig.version_string = reinterpret_cast<const char*>(qglGetString(GL_VERSION));
		glConfig.extensions_string = reinterpret_cast<const char*>(qglGetString(GL_EXTENSIONS));

		glConfigExt.originalExtensionString = glConfig.extensions_string;
		glConfig.extensions_string = TruncateGLExtensionsString(glConfigExt.originalExtensionString, 128);

		// OpenGL driver constants
		qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize);

		// stubbed or broken drivers may have reported 0...
		glConfig.maxTextureSize = Q_max(0, glConfig.maxTextureSize);

		// initialize extensions
		GLimp_InitExtensions();

		// set default state
		GL_SetDefaultState();
		R_Splash();	//get something on screen asap
	}
	else
	{
		// set default state
		GL_SetDefaultState();
	}
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors() {
#if defined(_DEBUG)
	char	s[64];

	const GLenum err = qglGetError();
	if (err == GL_NO_ERROR) {
		return;
	}
	if (r_ignoreGLErrors->integer) {
		return;
	}
	switch (err) {
	case GL_INVALID_ENUM:
		strcpy(s, "GL_INVALID_ENUM");
		break;
	case GL_INVALID_VALUE:
		strcpy(s, "GL_INVALID_VALUE");
		break;
	case GL_INVALID_OPERATION:
		strcpy(s, "GL_INVALID_OPERATION");
		break;
	case GL_STACK_OVERFLOW:
		strcpy(s, "GL_STACK_OVERFLOW");
		break;
	case GL_STACK_UNDERFLOW:
		strcpy(s, "GL_STACK_UNDERFLOW");
		break;
	case GL_OUT_OF_MEMORY:
		strcpy(s, "GL_OUT_OF_MEMORY");
		break;
	default:
		Com_sprintf(s, sizeof s, "%i", err);
		break;
	}

	Com_Error(ERR_FATAL, "GL_CheckErrors: %s", s);
#endif
}

/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with Hunk_FreeTempMemory()
==================
*/

byte* RB_ReadPixels(const int x, const int y, const int width, const int height, size_t* offset, int* padlen)
{
	GLint packAlign;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	const int linelen = width * 3;
	const int padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	auto buffer = static_cast<byte*>(Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1));

	const auto bufstart = static_cast<byte*>(PADP(reinterpret_cast<intptr_t>(buffer) + *offset, packAlign));
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot(const int x, const int y, const int width, const int height, const char* file_name) {
	byte* destptr;

	int padlen;
	size_t offset = 18;

	byte* allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	byte* buffer = allbuf + offset - 18;

	Com_Memset(buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr and remove padding from line endings
	const int linelen = width * 3;

	byte* srcptr = destptr = allbuf + offset;
	const byte* endmem = srcptr + (linelen + padlen) * height;

	while (srcptr < endmem)
	{
		const byte* endline = srcptr + linelen;

		while (srcptr < endline)
		{
			const byte temp = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	const size_t memcount = linelen * height;

	// gamma correct
	if (glConfig.deviceSupportsGamma && !glConfigExt.doGammaCorrectionWithShaders)
		R_GammaCorrect(allbuf + offset, memcount);

	ri->FS_WriteFile(file_name, buffer, memcount + 18);

	ri->Hunk_FreeTempMemory(allbuf);
}

/*
==================
R_TakeScreenshotPNG
==================
*/
void R_TakeScreenshotPNG(const int x, const int y, const int width, const int height, const char* file_name) {
	size_t offset = 0;
	int padlen = 0;

	byte* buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	RE_SavePNG(file_name, buffer, width, height, 3);
	ri->Hunk_FreeTempMemory(buffer);
}

/*
==================
R_TakeScreenshotJPEG
==================
*/
void R_TakeScreenshotJPEG(const int x, const int y, const int width, const int height, const char* file_name) {
	size_t offset = 0;
	int padlen;

	byte* buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	const size_t memcount = (width * 3 + padlen) * height;

	// gamma correct
	if (glConfig.deviceSupportsGamma && !glConfigExt.doGammaCorrectionWithShaders)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SaveJPG(file_name, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri->Hunk_FreeTempMemory(buffer);
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename(char* buf, const int buf_size, const char* ext) {
	time_t rawtime;
	char time_str[32] = { 0 }; // should really only reach ~19 chars

	time(&rawtime);
	strftime(time_str, sizeof time_str, "%Y-%m-%d_%H-%M-%S", localtime(&rawtime)); // or gmtime

	Com_sprintf(buf, buf_size, "screenshots/shot%s%s", time_str, ext);
}

/*
====================
R_LevelShot

levelshots are specialized 256*256 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
#define LEVELSHOTSIZE 256
static void R_LevelShot() {
	char		checkname[MAX_OSPATH];
	size_t		offset = 0;
	int			padlen;
	int g, b;

	Com_sprintf(checkname, sizeof checkname, "levelshots/%s.tga", tr.world->baseName);

	byte* allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen);
	const byte* source = allsource + offset;

	byte* buffer = static_cast<byte*>(ri->Hunk_AllocateTempMemory(LEVELSHOTSIZE * LEVELSHOTSIZE * 3 + 18));
	Com_Memset(buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = LEVELSHOTSIZE & 255;
	buffer[13] = LEVELSHOTSIZE >> 8;
	buffer[14] = LEVELSHOTSIZE & 255;
	buffer[15] = LEVELSHOTSIZE >> 8;
	buffer[16] = 24;	// pixel size

	// resample from source
	const float x_scale = glConfig.vidWidth / (4.0 * LEVELSHOTSIZE);
	const float y_scale = glConfig.vidHeight / (3.0 * LEVELSHOTSIZE);
	for (int y = 0; y < LEVELSHOTSIZE; y++) {
		for (int x = 0; x < LEVELSHOTSIZE; x++) {
			int r = g = b = 0;
			for (int yy = 0; yy < 3; yy++) {
				for (int xx = 0; xx < 4; xx++) {
					const byte* src = source + 3 * (glConfig.vidWidth * static_cast<int>((y * 3 + yy) * y_scale) + static_cast<int>((x * 4 + xx) * x_scale));
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			byte* dst = buffer + 18 + 3 * (y * LEVELSHOTSIZE + x);
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if (tr.overbrightBits > 0 && glConfig.deviceSupportsGamma && !glConfigExt.doGammaCorrectionWithShaders) {
		R_GammaCorrect(buffer + 18, LEVELSHOTSIZE * LEVELSHOTSIZE * 3);
	}

	ri->FS_WriteFile(checkname, buffer, LEVELSHOTSIZE * LEVELSHOTSIZE * 3 + 18);

	ri->Hunk_FreeTempMemory(buffer);
	ri->Hunk_FreeTempMemory(allsource);

	ri->Printf(PRINT_ALL, "[skipnotify]Wrote %s\n", checkname);
}

/*
==================
R_ScreenShotTGA_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShotTGA_f() {
	char checkname[MAX_OSPATH] = { 0 };
	qboolean silent = qfalse;

	if (strcmp(ri->Cmd_Argv(1), "levelshot") == 0) {
		R_LevelShot();
		return;
	}

	if (strcmp(ri->Cmd_Argv(1), "silent") == 0)
		silent = qtrue;

	if (ri->Cmd_Argc() == 2 && !silent) {
		// explicit filename
		Com_sprintf(checkname, sizeof checkname, "screenshots/%s.tga", ri->Cmd_Argv(1));
	}
	else {
		// timestamp the file
		R_ScreenshotFilename(checkname, sizeof checkname, ".tga");

		if (ri->FS_FileExists(checkname)) {
			ri->Printf(PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}
	}

	R_TakeScreenshot(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname);

	if (!silent)
		ri->Printf(PRINT_ALL, "[skipnotify]Wrote %s\n", checkname);
}

/*
==================
R_ScreenShotPNG_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShotPNG_f() {
	char checkname[MAX_OSPATH] = { 0 };
	qboolean silent = qfalse;

	if (strcmp(ri->Cmd_Argv(1), "levelshot") == 0) {
		R_LevelShot();
		return;
	}

	if (strcmp(ri->Cmd_Argv(1), "silent") == 0)
		silent = qtrue;

	if (ri->Cmd_Argc() == 2 && !silent) {
		// explicit filename
		Com_sprintf(checkname, sizeof checkname, "screenshots/%s.png", ri->Cmd_Argv(1));
	}
	else {
		// timestamp the file
		R_ScreenshotFilename(checkname, sizeof checkname, ".png");

		if (ri->FS_FileExists(checkname)) {
			ri->Printf(PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}
	}

	R_TakeScreenshotPNG(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname);

	if (!silent)
		ri->Printf(PRINT_ALL, "[skipnotify]Wrote %s\n", checkname);
}

void R_ScreenShot_f() {
	char checkname[MAX_OSPATH] = { 0 };
	qboolean silent = qfalse;

	if (strcmp(ri->Cmd_Argv(1), "levelshot") == 0) {
		R_LevelShot();
		return;
	}
	if (strcmp(ri->Cmd_Argv(1), "silent") == 0)
		silent = qtrue;

	if (ri->Cmd_Argc() == 2 && !silent) {
		// explicit filename
		Com_sprintf(checkname, sizeof checkname, "screenshots/%s.jpg", ri->Cmd_Argv(1));
	}
	else {
		// timestamp the file
		R_ScreenshotFilename(checkname, sizeof checkname, ".jpg");

		if (ri->FS_FileExists(checkname)) {
			ri->Printf(PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}
	}

	R_TakeScreenshotJPEG(0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname);

	if (!silent)
		ri->Printf(PRINT_ALL, "[skipnotify]Wrote %s\n", checkname);
}

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void* RB_TakeVideoFrameCmd(const void* data)
{
	GLint pack_align;

	const auto cmd = static_cast<const videoFrameCommand_t*>(data);

	qglGetIntegerv(GL_PACK_ALIGNMENT, &pack_align);

	const size_t linelen = cmd->width * 3;

	// Alignment stuff for glReadPixels
	const int padwidth = PAD(linelen, pack_align);
	const int padlen = padwidth - linelen;
	// AVI line padding
	const int avipadwidth = PAD(linelen, AVI_LINE_PADDING);
	const int avipadlen = avipadwidth - linelen;

	const auto c_buf = static_cast<byte*>(PADP(cmd->captureBuffer, pack_align));

	qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
		GL_UNSIGNED_BYTE, c_buf);

	size_t memcount = padwidth * cmd->height;

	// gamma correct
	if (glConfig.deviceSupportsGamma && !glConfigExt.doGammaCorrectionWithShaders)
		R_GammaCorrect(c_buf, memcount);

	if (cmd->motionJpeg)
	{
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
			r_aviMotionJpegQuality->integer,
			cmd->width, cmd->height, c_buf, padlen);
		ri->CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}
	else
	{
		byte* srcptr = c_buf;
		byte* destptr = cmd->encodeBuffer;
		const byte* memend = srcptr + memcount;

		// swap R and B and remove line paddings
		while (srcptr < memend)
		{
			const byte* lineend = srcptr + linelen;
			while (srcptr < lineend)
			{
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr += 3;
			}

			Com_Memset(destptr, '\0', avipadlen);
			destptr += avipadlen;

			srcptr += padlen;
		}

		ri->CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return cmd + 1;
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState()
{
	qglClearDepth(1.0f);

	qglCullFace(GL_FRONT);

	qglColor4f(1, 1, 1, 1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if (qglActiveTextureARB) {
		GL_SelectTexture(1);
		GL_TextureMode(r_textureMode->string);
		GL_TexEnv(GL_MODULATE);
		qglDisable(GL_TEXTURE_2D);
		GL_SelectTexture(0);
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode(r_textureMode->string);
	GL_TexEnv(GL_MODULATE);

	qglShadeModel(GL_SMOOTH);
	qglDepthFunc(GL_LEQUAL);

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState(GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask(GL_TRUE);
	qglDisable(GL_DEPTH_TEST);
	qglEnable(GL_SCISSOR_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_BLEND);
}

/*
================
R_PrintLongString

Workaround for ri->Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(const char* string)
{
	const char* p = string;
	int remaining_length = strlen(string);

	while (remaining_length > 0)
	{
		char buffer[1024];
		// Take as much characters as possible from the string without splitting words between buffers
		// This avoids the client console splitting a word up when one half fits on the current line,
		// but the second half would have to be written on a new line
		int charsToTake = sizeof buffer - 1;
		if (remaining_length > charsToTake) {
			while (p[charsToTake - 1] > ' ' && p[charsToTake] > ' ') {
				charsToTake--;
				if (charsToTake == 0) {
					charsToTake = sizeof buffer - 1;
					break;
				}
			}
		}
		else if (remaining_length < charsToTake) {
			charsToTake = remaining_length;
		}

		Q_strncpyz(buffer, p, charsToTake + 1);
		ri->Printf(PRINT_ALL, "%s", buffer);
		remaining_length -= charsToTake;
		p += charsToTake;
	}
}

/*
================
GfxInfo_f
================
*/
extern bool g_bTextureRectangleHack;
void GfxInfo_f()
{
	const char* enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char* fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};
	const char* noborderstrings[] =
	{
		"",
		"noborder "
	};

	const char* tc_table[] =
	{
		"None",
		"GL_S3_s3tc",
		"GL_EXT_texture_compression_s3tc",
	};

	const int fullscreen = ri->Cvar_VariableIntegerValue("r_fullscreen");
	const int noborder = ri->Cvar_VariableIntegerValue("r_noborder");

	ri->Printf(PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string);
	ri->Printf(PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string);
	ri->Printf(PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string);
	R_PrintLongString(glConfigExt.originalExtensionString);
	ri->Printf(PRINT_ALL, "\n");
	ri->Printf(PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);
	ri->Printf(PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.maxActiveTextures);
	ri->Printf(PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
	ri->Printf(PRINT_ALL, "MODE: %d, %d x %d %s%s hz:",
		ri->Cvar_VariableIntegerValue("r_mode"),
		glConfig.vidWidth, glConfig.vidHeight,
		fullscreen == 0 ? noborderstrings[noborder == 1] : noborderstrings[0],
		fsstrings[fullscreen == 1]);
	if (glConfig.displayFrequency)
	{
		ri->Printf(PRINT_ALL, "%d\n", glConfig.displayFrequency);
	}
	else
	{
		ri->Printf(PRINT_ALL, "N/A\n");
	}
	if (glConfig.deviceSupportsGamma && !glConfigExt.doGammaCorrectionWithShaders)
	{
		ri->Printf(PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits);
	}
	else
	{
		ri->Printf(PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits);
	}

	// rendering primitives
	{
		// default is to use triangles if compiled vertex arrays are present
		ri->Printf(PRINT_ALL, "rendering primitives: ");
		int primitives = r_primitives->integer;
		if (primitives == 0) {
			if (qglLockArraysEXT) {
				primitives = 2;
			}
			else {
				primitives = 1;
			}
		}
		if (primitives == -1) {
			ri->Printf(PRINT_ALL, "none\n");
		}
		else if (primitives == 2) {
			ri->Printf(PRINT_ALL, "single glDrawElements\n");
		}
		else if (primitives == 1) {
			ri->Printf(PRINT_ALL, "multiple glArrayElement\n");
		}
		else if (primitives == 3) {
			ri->Printf(PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n");
		}
	}

	ri->Printf(PRINT_ALL, "texturemode: %s\n", r_textureMode->string);
	ri->Printf(PRINT_ALL, "picmip: %d\n", r_picmip->integer);
	ri->Printf(PRINT_ALL, "texture bits: %d\n", r_texturebits->integer);
	if (r_texturebitslm->integer > 0)
		ri->Printf(PRINT_ALL, "lightmap texture bits: %d\n", r_texturebitslm->integer);
	ri->Printf(PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != nullptr]);
	ri->Printf(PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != nullptr]);
	ri->Printf(PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0]);
	ri->Printf(PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE]);
	ri->Printf(PRINT_ALL, "compressed lightmaps: %s\n", enablestrings[(r_ext_compressed_lightmaps->integer != 0 && glConfig.textureCompression != TC_NONE)]);
	ri->Printf(PRINT_ALL, "texture compression method: %s\n", tc_table[glConfig.textureCompression]);
	ri->Printf(PRINT_ALL, "anisotropic filtering: %s  ", enablestrings[r_ext_texture_filter_anisotropic->integer != 0 && glConfig.maxTextureFilterAnisotropy]);
	if (r_ext_texture_filter_anisotropic->integer != 0 && glConfig.maxTextureFilterAnisotropy)
	{
		if (Q_isintegral(r_ext_texture_filter_anisotropic->value))
			ri->Printf(PRINT_ALL, "(%i of ", static_cast<int>(r_ext_texture_filter_anisotropic->value));
		else
			ri->Printf(PRINT_ALL, "(%f of ", r_ext_texture_filter_anisotropic->value);

		if (Q_isintegral(glConfig.maxTextureFilterAnisotropy))
			ri->Printf(PRINT_ALL, "%i)\n", static_cast<int>(glConfig.maxTextureFilterAnisotropy));
		else
			ri->Printf(PRINT_ALL, "%f)\n", glConfig.maxTextureFilterAnisotropy);
	}
	ri->Printf(PRINT_ALL, "Dynamic Glow: %s\n", enablestrings[r_DynamicGlow->integer ? 1 : 0]);
	if (g_bTextureRectangleHack) ri->Printf(PRINT_ALL, "Dynamic Glow ATI BAD DRIVER HACK %s\n", enablestrings[g_bTextureRectangleHack]);

	if (r_finish->integer) {
		ri->Printf(PRINT_ALL, "Forcing glFinish\n");
	}

	const int display_refresh = ri->Cvar_VariableIntegerValue("r_displayRefresh");
	if (display_refresh) {
		ri->Printf(PRINT_ALL, "Display refresh set to %d\n", display_refresh);
	}
	if (tr.world)
	{
		ri->Printf(PRINT_ALL, "Light Grid size set to (%.2f %.2f %.2f)\n", tr.world->lightGridSize[0], tr.world->lightGridSize[1], tr.world->lightGridSize[2]);
	}
}

void R_AtiHackToggle_f()
{
	g_bTextureRectangleHack = !g_bTextureRectangleHack;
}

using consoleCommand_t = struct consoleCommand_s {
	const char* cmd;
	xcommand_t	func;
};

static consoleCommand_t	commands[] = {
	{ "imagelist",			R_ImageList_f },
	{ "shaderlist",			R_ShaderList_f },
	{ "skinlist",			R_SkinList_f },
	{ "fontlist",			R_FontList_f },
	{ "screenshot",			R_ScreenShot_f },
	{ "screenshot_png",		R_ScreenShotPNG_f },
	{ "screenshot_tga",		R_ScreenShotTGA_f },
	{ "gfxinfo",			GfxInfo_f },
	{ "r_atihack",			R_AtiHackToggle_f },
	{ "r_we",				R_WorldEffect_f },
	{ "imagecacheinfo",		RE_RegisterImages_Info_f },
	{ "modellist",			R_Modellist_f },
	{ "modelcacheinfo",		RE_RegisterModels_Info_f },
	{ "weather",			R_SetWeatherEffect_f },
	{ "r_weather",			R_WeatherEffect_f },
};

#ifdef _DEBUG
#define MIN_PRIMITIVES -1
#else
#define MIN_PRIMITIVES 0
#endif
#define MAX_PRIMITIVES 3

/*
===============
R_Register
===============
*/

void R_Register()
{
	//FIXME: lol badness
	se_language = ri->Cvar_Get("se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART, "");
	//
	// latched and archived variables
	//
	r_allowExtensions = ri->Cvar_Get("r_allowExtensions", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_ext_compressed_textures = ri->Cvar_Get("r_ext_compress_textures", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_ext_compressed_lightmaps = ri->Cvar_Get("r_ext_compress_lightmaps", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_ext_preferred_tc_method = ri->Cvar_Get("r_ext_preferred_tc_method", "0", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_ext_gamma_control = ri->Cvar_Get("r_ext_gamma_control", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_ext_multitexture = ri->Cvar_Get("r_ext_multitexture", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_ext_compiled_vertex_array = ri->Cvar_Get("r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_ext_texture_env_add = ri->Cvar_Get("r_ext_texture_env_add", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_ext_texture_filter_anisotropic = ri->Cvar_Get("r_ext_texture_filter_anisotropic", "16", CVAR_ARCHIVE_ND, "");
	r_gammaShaders = ri->Cvar_Get("r_gammaShaders", "0", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_environmentMapping = ri->Cvar_Get("r_environmentMapping", "1", CVAR_ARCHIVE_ND, "");
	r_DynamicGlow = ri->Cvar_Get("r_DynamicGlow", "1", CVAR_ARCHIVE_ND, "");
	r_DynamicGlowPasses = ri->Cvar_Get("r_DynamicGlowPasses", "5", CVAR_ARCHIVE_ND, "");
	r_DynamicGlowDelta = ri->Cvar_Get("r_DynamicGlowDelta", "0.8f", CVAR_ARCHIVE_ND, "");
	r_DynamicGlowIntensity = ri->Cvar_Get("r_DynamicGlowIntensity", "1.13f", CVAR_ARCHIVE_ND, "");
	r_DynamicGlowSoft = ri->Cvar_Get("r_DynamicGlowSoft", "1", CVAR_ARCHIVE_ND, "");
	r_DynamicGlowWidth = ri->Cvar_Get("r_DynamicGlowWidth", "320", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_DynamicGlowHeight = ri->Cvar_Get("r_DynamicGlowHeight", "240", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_picmip = ri->Cvar_Get("r_picmip", "0", CVAR_ARCHIVE | CVAR_LATCH, "");
	ri->Cvar_CheckRange(r_picmip, 0, 16, qtrue);
	r_colorMipLevels = ri->Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH, "");
	r_detailTextures = ri->Cvar_Get("r_detailtextures", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_texturebits = ri->Cvar_Get("r_texturebits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_texturebitslm = ri->Cvar_Get("r_texturebitslm", "0", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_overBrightBits = ri->Cvar_Get("r_overBrightBits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_mapOverBrightBits = ri->Cvar_Get("r_mapOverBrightBits", "0", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_simpleMipMaps = ri->Cvar_Get("r_simpleMipMaps", "1", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	r_vertexLight = ri->Cvar_Get("r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH, "");
	r_uiFullScreen = ri->Cvar_Get("r_uifullscreen", "0", CVAR_NONE, "");
	r_subdivisions = ri->Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE_ND | CVAR_LATCH, "");
	ri->Cvar_CheckRange(r_subdivisions, 0, 80, qfalse);

	r_fullbright = ri->Cvar_Get("r_fullbright", "0", CVAR_CHEAT, "");
	r_intensity = ri->Cvar_Get("r_intensity", "1", CVAR_LATCH, "");
	r_singleShader = ri->Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH, "");
	r_lodCurveError = ri->Cvar_Get("r_lodCurveError", "250", CVAR_ARCHIVE_ND, "");
	r_lodbias = ri->Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE_ND, "");
	r_autolodscalevalue = ri->Cvar_Get("r_autolodscalevalue", "0", CVAR_ROM, "");
	r_flares = ri->Cvar_Get("r_flares", "1", CVAR_ARCHIVE_ND, "");

	r_znear = ri->Cvar_Get("r_znear", "4", CVAR_ARCHIVE_ND, "");
	ri->Cvar_CheckRange(r_znear, 0.001f, 10, qfalse);
	r_ignoreGLErrors = ri->Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE_ND, "");
	r_fastsky = ri->Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE_ND, "");
	r_inGameVideo = ri->Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE_ND, "");
	r_drawSun = ri->Cvar_Get("r_drawSun", "0", CVAR_ARCHIVE_ND, "");
	r_dynamiclight = ri->Cvar_Get("r_dynamiclight", "1", CVAR_ARCHIVE, "");
	// rjr - removed for hacking
//	r_dlightBacks						= ri->Cvar_Get( "r_dlightBacks",					"1",						CVAR_CHEAT, "" );
	r_finish = ri->Cvar_Get("r_finish", "0", CVAR_ARCHIVE_ND, "");
	r_textureMode = ri->Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE, "");
	r_markcount = ri->Cvar_Get("r_markcount", "100", CVAR_ARCHIVE_ND, "");
	r_gamma = ri->Cvar_Get("r_gamma", "1", CVAR_ARCHIVE_ND, "");
	r_facePlaneCull = ri->Cvar_Get("r_facePlaneCull", "1", CVAR_ARCHIVE_ND, "");
	r_cullRoofFaces = ri->Cvar_Get("r_cullRoofFaces", "0", CVAR_CHEAT, ""); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_roofCullCeilDist = ri->Cvar_Get("r_roofCullCeilDist", "256", CVAR_CHEAT, ""); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_roofCullFloorDist = ri->Cvar_Get("r_roofCeilFloorDist", "128", CVAR_CHEAT, ""); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_primitives = ri->Cvar_Get("r_primitives", "0", CVAR_ARCHIVE_ND, "");
	ri->Cvar_CheckRange(r_primitives, MIN_PRIMITIVES, MAX_PRIMITIVES, qtrue);
	r_ambientScale = ri->Cvar_Get("r_ambientScale", "0.6", CVAR_CHEAT, "");
	r_directedScale = ri->Cvar_Get("r_directedScale", "1", CVAR_CHEAT, "");
	r_autoMap = ri->Cvar_Get("r_autoMap", "0", CVAR_ARCHIVE_ND, ""); //automap renderside toggle for debugging -rww
	r_autoMapBackAlpha = ri->Cvar_Get("r_autoMapBackAlpha", "0", CVAR_NONE, ""); //alpha of automap bg -rww
	r_autoMapDisable = ri->Cvar_Get("r_autoMapDisable", "1", CVAR_NONE, "");
	r_showImages = ri->Cvar_Get("r_showImages", "0", CVAR_CHEAT, "");
	r_debugLight = ri->Cvar_Get("r_debuglight", "0", CVAR_TEMP, "");
	r_debugSort = ri->Cvar_Get("r_debugSort", "0", CVAR_CHEAT, "");
	r_dlightStyle = ri->Cvar_Get("r_dlightStyle", "1", CVAR_TEMP, "");
	r_surfaceSprites = ri->Cvar_Get("r_surfaceSprites", "1", CVAR_ARCHIVE_ND, "");
	r_surfaceWeather = ri->Cvar_Get("r_surfaceWeather", "0", CVAR_TEMP, "");
	r_windSpeed = ri->Cvar_Get("r_windSpeed", "0", CVAR_NONE, "");
	r_windAngle = ri->Cvar_Get("r_windAngle", "0", CVAR_NONE, "");
	r_windGust = ri->Cvar_Get("r_windGust", "0", CVAR_NONE, "");
	r_windDampFactor = ri->Cvar_Get("r_windDampFactor", "0.1", CVAR_NONE, "");
	r_windPointForce = ri->Cvar_Get("r_windPointForce", "0", CVAR_NONE, "");
	r_windPointX = ri->Cvar_Get("r_windPointX", "0", CVAR_NONE, "");
	r_windPointY = ri->Cvar_Get("r_windPointY", "0", CVAR_NONE, "");
	r_nocurves = ri->Cvar_Get("r_nocurves", "0", CVAR_CHEAT, "");
	r_drawworld = ri->Cvar_Get("r_drawworld", "1", CVAR_CHEAT, "");
	r_drawfog = ri->Cvar_Get("r_drawfog", "1", CVAR_NONE, "");
	r_lightmap = ri->Cvar_Get("r_lightmap", "0", CVAR_CHEAT, "");
	r_portalOnly = ri->Cvar_Get("r_portalOnly", "0", CVAR_CHEAT, "");
	r_skipBackEnd = ri->Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT, "");
	r_measureOverdraw = ri->Cvar_Get("r_measureOverdraw", "0", CVAR_CHEAT, "");
	r_lodscale = ri->Cvar_Get("r_lodscale", "5", CVAR_NONE, "");
	r_norefresh = ri->Cvar_Get("r_norefresh", "0", CVAR_CHEAT, "");
	r_drawentities = ri->Cvar_Get("r_drawentities", "1", CVAR_CHEAT, "");
	r_ignore = ri->Cvar_Get("r_ignore", "1", CVAR_CHEAT, "");
	r_nocull = ri->Cvar_Get("r_nocull", "0", CVAR_CHEAT, "");
	r_novis = ri->Cvar_Get("r_novis", "0", CVAR_CHEAT, "");
	r_showcluster = ri->Cvar_Get("r_showcluster", "0", CVAR_CHEAT, "");
	r_speeds = ri->Cvar_Get("r_speeds", "0", CVAR_CHEAT, "");
	r_verbose = ri->Cvar_Get("r_verbose", "0", CVAR_CHEAT, "");
	r_logFile = ri->Cvar_Get("r_logFile", "0", CVAR_CHEAT, "");
	r_debugSurface = ri->Cvar_Get("r_debugSurface", "0", CVAR_CHEAT, "");
	r_nobind = ri->Cvar_Get("r_nobind", "0", CVAR_CHEAT, "");
	r_showtris = ri->Cvar_Get("r_showtris", "0", CVAR_CHEAT, "");
	r_showsky = ri->Cvar_Get("r_showsky", "0", CVAR_CHEAT, "");
	r_shownormals = ri->Cvar_Get("r_shownormals", "0", CVAR_CHEAT, "");
	r_clear = ri->Cvar_Get("r_clear", "0", CVAR_CHEAT, "");
	r_offsetFactor = ri->Cvar_Get("r_offsetfactor", "-1", CVAR_CHEAT, "");
	r_offsetUnits = ri->Cvar_Get("r_offsetunits", "-2", CVAR_CHEAT, "");
	r_lockpvs = ri->Cvar_Get("r_lockpvs", "0", CVAR_CHEAT, "");
	r_noportals = ri->Cvar_Get("r_noportals", "0", CVAR_CHEAT, "");
	r_shadows = ri->Cvar_Get("cg_shadows", "2", CVAR_NONE, "");
	r_shadowRange = ri->Cvar_Get("r_shadowRange", "1000", CVAR_NONE, "");
	r_marksOnTriangleMeshes = ri->Cvar_Get("r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE_ND, "");
	r_aspectCorrectFonts = ri->Cvar_Get("r_aspectCorrectFonts", "0", CVAR_ARCHIVE, "");
	r_maxpolys = ri->Cvar_Get("r_maxpolys", XSTRING(DEFAULT_MAX_POLYS), CVAR_NONE, "");
	r_maxpolyverts = ri->Cvar_Get("r_maxpolyverts", XSTRING(DEFAULT_MAX_POLYVERTS), CVAR_NONE, "");
	/*
	Ghoul2 Insert Start
	*/
#ifdef _DEBUG
	r_noPrecacheGLA = ri->Cvar_Get("r_noPrecacheGLA", "0", CVAR_CHEAT, "");
#endif
	r_noServerGhoul2 = ri->Cvar_Get("r_noserverghoul2", "0", CVAR_CHEAT, "");
	r_Ghoul2AnimSmooth = ri->Cvar_Get("r_ghoul2animsmooth", "0.3", CVAR_NONE, "");
	r_Ghoul2UnSqashAfterSmooth = ri->Cvar_Get("r_ghoul2unsqashaftersmooth", "1", CVAR_NONE, "");
	broadsword = ri->Cvar_Get("broadsword", "1", CVAR_ARCHIVE_ND, "");
	broadsword_kickbones = ri->Cvar_Get("broadsword_kickbones", "1", CVAR_NONE, "");
	broadsword_kickorigin = ri->Cvar_Get("broadsword_kickorigin", "1", CVAR_NONE, "");
	broadsword_dontstopanim = ri->Cvar_Get("broadsword_dontstopanim", "0", CVAR_NONE, "");
	broadsword_waitforshot = ri->Cvar_Get("broadsword_waitforshot", "0", CVAR_NONE, "");
	broadsword_playflop = ri->Cvar_Get("broadsword_playflop", "1", CVAR_NONE, "");
	broadsword_smallbbox = ri->Cvar_Get("broadsword_smallbbox", "0", CVAR_NONE, "");
	broadsword_extra1 = ri->Cvar_Get("broadsword_extra1", "0", CVAR_NONE, "");
	broadsword_extra2 = ri->Cvar_Get("broadsword_extra2", "0", CVAR_NONE, "");
	broadsword_effcorr = ri->Cvar_Get("broadsword_effcorr", "1", CVAR_NONE, "");
	broadsword_ragtobase = ri->Cvar_Get("broadsword_ragtobase", "2", CVAR_NONE, "");
	broadsword_dircap = ri->Cvar_Get("broadsword_dircap", "64", CVAR_NONE, "");

	r_weather = ri->Cvar_Get("r_weather", "0", CVAR_ARCHIVE, "");
	/*
	Ghoul2 Insert End
	*/
	r_modelpoolmegs = ri->Cvar_Get("r_modelpoolmegs", "20", CVAR_ARCHIVE, "");
	if (ri->Sys_LowPhysicalMemory())
		ri->Cvar_Set("r_modelpoolmegs", "0");

	r_aviMotionJpegQuality = ri->Cvar_Get("r_aviMotionJpegQuality", "90", CVAR_ARCHIVE_ND, "");
	r_screenshotJpegQuality = ri->Cvar_Get("r_screenshotJpegQuality", "95", CVAR_ARCHIVE_ND, "");

	ri->Cvar_CheckRange(r_aviMotionJpegQuality, 10, 100, qtrue);
	ri->Cvar_CheckRange(r_screenshotJpegQuality, 10, 100, qtrue);

	for (const auto& command : commands)
		ri->Cmd_AddCommand(command.cmd, command.func, "");
}

/*
===============
R_Init
===============
*/
extern void R_InitWorldEffects(); //tr_WorldEffects.cpp
void R_Init() {
	int i;
	byte* ptr;

	//	ri->Printf( PRINT_ALL, "----- R_Init -----\n" );
		// clear all our internal state
	memset(&tr, 0, sizeof tr);
	memset(&backEnd, 0, sizeof backEnd);
	memset(&tess, 0, sizeof tess);

	//	Swap_Init();

#ifndef FINAL_BUILD
	if ((intptr_t)tess.xyz & 15) {
		ri->Printf(PRINT_ALL, "WARNING: tess.xyz not 16 byte aligned\n");
	}
#endif
	//
	// init function tables
	//
	for (i = 0; i < FUNCTABLE_SIZE; i++)
	{
		tr.sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1))));
		tr.squareTable[i] = i < FUNCTABLE_SIZE / 2 ? 1.0f : -1.0f;
		tr.sawToothTable[i] = static_cast<float>(i) / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if (i < FUNCTABLE_SIZE / 2)
		{
			if (i < FUNCTABLE_SIZE / 4)
			{
				tr.triangleTable[i] = static_cast<float>(i) / (FUNCTABLE_SIZE / 4);
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}
	R_InitFogTable();

	R_ImageLoader_Init();
	R_NoiseInit();
	R_Register();

	max_polys = Q_min(r_maxpolys->integer, DEFAULT_MAX_POLYS);
	max_polyverts = Q_min(r_maxpolyverts->integer, DEFAULT_MAX_POLYVERTS);

	ptr = static_cast<byte*>(Hunk_Alloc(sizeof * backEndData + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts,
		h_low));
	backEndData = (backEndData_t*)ptr;
	backEndData->polys = (srfPoly_t*)((char*)ptr + sizeof * backEndData);
	backEndData->polyVerts = (polyVert_t*)((char*)ptr + sizeof * backEndData + sizeof(srfPoly_t) * max_polys);

	R_InitNextFrame();

	for (i = 0; i < MAX_LIGHT_STYLES; i++)
	{
		RE_SetLightStyle(i, -1);
	}
	InitOpenGL();

	R_InitImages();
	R_InitShaders(qfalse);
	R_InitSkins();

	R_InitFonts();

	R_ModelInit();
	//	re.G2VertSpaceServer = &IHeapAllocator_singleton;
	R_InitDecals();

	R_InitWorldEffects();

#if defined(_DEBUG)
	const int	err = qglGetError();
	if (err != GL_NO_ERROR)
		ri->Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);
#endif

	RestoreGhoul2InfoArray();
	// print info
	GfxInfo_f();

	//	ri->Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown(const qboolean destroy_window, const qboolean restarting) {
	//	ri->Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	for (const auto& command : commands)
		ri->Cmd_RemoveCommand(command.cmd);

	if (r_DynamicGlow && r_DynamicGlow->integer)
	{
		// Release the Glow Vertex Shader.
		if (tr.glowVShader)
		{
			qglDeleteProgramsARB(1, &tr.glowVShader);
		}

		// Release Pixel Shader.
		if (tr.glowPShader)
		{
			if (qglCombinerParameteriNV)
			{
				// Release the Glow Regcom call list.
				qglDeleteLists(tr.glowPShader, 1);
			}
			else if (qglGenProgramsARB)
			{
				// Release the Glow Fragment Shader.
				qglDeleteProgramsARB(1, &tr.glowPShader);
			}
		}

		if (tr.gammaCorrectVtxShader)
		{
			qglDeleteProgramsARB(1, &tr.gammaCorrectVtxShader);
		}

		if (tr.gammaCorrectPxShader)
		{
			qglDeleteProgramsARB(1, &tr.gammaCorrectPxShader);
		}

		// Release the scene glow texture.
		qglDeleteTextures(1, &tr.screenGlow);

		// Release the scene texture.
		qglDeleteTextures(1, &tr.sceneImage);

		qglDeleteTextures(1, &tr.gammaCorrectLUTImage);

		// Release the blur texture.
		qglDeleteTextures(1, &tr.blurImage);
	}

	R_ShutdownWorldEffects();
	R_ShutdownFonts();
	if (tr.registered) {
		R_IssuePendingRenderCommands();
		if (destroy_window)
		{
			R_DeleteTextures();		// only do this for vid_restart now, not during things like map load

			if (restarting)
			{
				SaveGhoul2InfoArray();
			}
		}
	}

	// shut down platform specific OpenGL stuff
	if (destroy_window) {
		ri->WIN_Shutdown();
	}

	tr.registered = qfalse;
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration() {
	R_IssuePendingRenderCommands();
	if (!ri->Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}

void RE_GetLightStyle(const int style, color4ub_t color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
		Com_Error(ERR_FATAL, "RE_GetLightStyle: %d is out of range", style);
	}

	const auto ba_dest = (byteAlias_t*)&color;
	const byteAlias_t* ba_source = (byteAlias_t*)&styleColors[style];
	ba_dest->i = ba_source->i;
}

void RE_SetLightStyle(const int style, const int color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
		Com_Error(ERR_FATAL, "RE_SetLightStyle: %d is out of range", style);
	}

	const auto ba = reinterpret_cast<byteAlias_t*>(&styleColors[style]);
	if (ba->i != color) {
		ba->i = color;
	}
}

static void SetRangedFog(const float range) { tr.rangedFog = range; }

extern qboolean gG2_GBMNoReconstruct;
extern qboolean gG2_GBMUseSPMethod;
static void G2API_BoltMatrixReconstruction(const qboolean reconstruct) { gG2_GBMNoReconstruct = static_cast<qboolean>(!reconstruct); }
static void G2API_BoltMatrixSPMethod(const qboolean spMethod) { gG2_GBMUseSPMethod = spMethod; }

extern float tr_distortionAlpha; //opaque
extern float tr_distortionStretch; //no stretch override
extern qboolean tr_distortionPrePost; //capture before postrender phase?
extern qboolean tr_distortionNegate; //negative blend mode
static void SetRefractionProperties(const float distortionAlpha, const float distortionStretch, const qboolean distortionPrePost, const qboolean distortionNegate) {
	tr_distortionAlpha = distortionAlpha;
	tr_distortionStretch = distortionStretch;
	tr_distortionPrePost = distortionPrePost;
	tr_distortionNegate = distortionNegate;
}

static float GetDistanceCull()
{ return tr.distanceCull; }

static void GetRealRes(int* w, int* h) {
	*w = glConfig.vidWidth;
	*h = glConfig.vidHeight;
}

extern void R_SVModelInit(void); //tr_model.cpp
extern void R_AutomapElevationAdjustment(float newHeight); //tr_world.cpp
extern qboolean R_InitializeWireframeAutomap(void); //tr_world.cpp

extern qhandle_t RE_RegisterServerSkin(const char* name);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
extern "C" {
	Q_EXPORT refexport_t* QDECL GetRefAPI(const int api_version, refimport_t* rimp) {
		static refexport_t re;

		assert(rimp);
		ri = rimp;

		memset(&re, 0, sizeof re);

		if (api_version != REF_API_VERSION) {
			ri->Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, api_version);
			return nullptr;
		}

		// the RE_ functions are Renderer Entry points

		re.Shutdown = RE_Shutdown;
		re.BeginRegistration = RE_BeginRegistration;
		re.RegisterModel = RE_RegisterModel;
		re.RegisterServerModel = RE_RegisterServerModel;
		re.RegisterSkin = RE_RegisterSkin;
		re.RegisterServerSkin = RE_RegisterServerSkin;
		re.RegisterShader = RE_RegisterShader;
		re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
		re.ShaderNameFromIndex = RE_ShaderNameFromIndex;
		re.LoadWorld = RE_LoadWorldMap;
		re.SetWorldVisData = RE_SetWorldVisData;
		re.EndRegistration = RE_EndRegistration;
		re.BeginFrame = RE_BeginFrame;
		re.EndFrame = RE_EndFrame;
		re.MarkFragments = R_MarkFragments;
		re.LerpTag = R_LerpTag;
		re.ModelBounds = R_ModelBounds;
		re.DrawRotatePic = RE_RotatePic;
		re.DrawRotatePic2 = RE_RotatePic2;
		re.ClearScene = RE_ClearScene;
		re.ClearDecals = RE_ClearDecals;
		re.AddRefEntityToScene = RE_AddRefEntityToScene;
		re.AddMiniRefEntityToScene = RE_AddMiniRefEntityToScene;
		re.AddPolyToScene = RE_AddPolyToScene;
		re.AddDecalToScene = RE_AddDecalToScene;
		re.LightForPoint = R_LightForPoint;
		re.AddLightToScene = RE_AddLightToScene;
		re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;

		re.RenderScene = RE_RenderScene;
		re.SetColor = RE_SetColor;
		re.DrawStretchPic = RE_StretchPic;
		re.DrawStretchRaw = RE_StretchRaw;
		re.UploadCinematic = RE_UploadCinematic;

		re.RegisterFont = RE_RegisterFont;
		re.Font_StrLenPixels = RE_Font_StrLenPixels;
		re.Font_StrLenChars = RE_Font_StrLenChars;
		re.Font_HeightPixels = RE_Font_HeightPixels;
		re.Font_DrawString = RE_Font_DrawString;
		re.Language_IsAsian = Language_IsAsian;
		re.Language_UsesSpaces = Language_UsesSpaces;
		re.AnyLanguage_ReadCharFromString = AnyLanguage_ReadCharFromString;

		re.RemapShader = R_RemapShader;
		re.GetEntityToken = R_GetEntityToken;
		re.inPVS = R_inPVS;
		re.GetLightStyle = RE_GetLightStyle;
		re.SetLightStyle = RE_SetLightStyle;
		re.GetBModelVerts = RE_GetBModelVerts;

		// missing from 1.01
		re.SetRangedFog = SetRangedFog;
		re.SetRefractionProperties = SetRefractionProperties;
		re.GetDistanceCull = GetDistanceCull;
		re.GetRealRes = GetRealRes;
		re.AutomapElevationAdjustment = R_AutomapElevationAdjustment; //tr_world.cpp
		re.InitializeWireframeAutomap = R_InitializeWireframeAutomap; //tr_world.cpp
		re.AddWeatherZone = RE_AddWeatherZone;
		re.WorldEffectCommand = RE_WorldEffectCommand;
		re.RegisterMedia_LevelLoadBegin = RE_RegisterMedia_LevelLoadBegin;
		re.RegisterMedia_LevelLoadEnd = RE_RegisterMedia_LevelLoadEnd;
		re.RegisterMedia_GetLevel = RE_RegisterMedia_GetLevel;
		re.RegisterImages_LevelLoadEnd = RE_RegisterImages_LevelLoadEnd;
		re.RegisterModels_LevelLoadEnd = RE_RegisterModels_LevelLoadEnd;

		re.IsOutsideCausingPain = R_IsOutsideCausingPain;

		// AVI recording
		re.TakeVideoFrame = RE_TakeVideoFrame;

		// G2 stuff
		re.InitSkins = R_InitSkins;
		re.InitShaders = R_InitShaders;
		re.SVModelInit = R_SVModelInit;
		re.HunkClearCrap = RE_HunkClearCrap;

		// G2API
		re.G2API_AddBolt = G2API_AddBolt;
		re.G2API_AddBoltSurfNum = G2API_AddBoltSurfNum;
		re.G2API_AddSurface = G2API_AddSurface;
		re.G2API_AnimateG2ModelsRag = G2API_AnimateG2ModelsRag;
		re.G2API_AttachEnt = G2API_AttachEnt;
		re.G2API_AttachG2Model = G2API_AttachG2Model;
		re.G2API_AttachInstanceToEntNum = G2API_AttachInstanceToEntNum;
		re.G2API_AbsurdSmoothing = G2API_AbsurdSmoothing;
		re.G2API_BoltMatrixReconstruction = G2API_BoltMatrixReconstruction;
		re.G2API_BoltMatrixSPMethod = G2API_BoltMatrixSPMethod;
		re.G2API_CleanEntAttachments = G2API_CleanEntAttachments;
		re.G2API_CleanGhoul2Models = G2API_CleanGhoul2Models;
		re.G2API_ClearAttachedInstance = G2API_ClearAttachedInstance;
		re.G2API_CollisionDetect = G2API_CollisionDetect;
		re.G2API_CollisionDetectCache = G2API_CollisionDetectCache;
		re.G2API_CopyGhoul2Instance = G2API_CopyGhoul2Instance;
		re.G2API_CopySpecificG2Model = G2API_CopySpecificG2Model;
		re.G2API_DetachG2Model = G2API_DetachG2Model;
		re.G2API_DoesBoneExist = G2API_DoesBoneExist;
		re.G2API_DuplicateGhoul2Instance = G2API_DuplicateGhoul2Instance;
		re.G2API_FreeSaveBuffer = G2API_FreeSaveBuffer;
		re.G2API_GetAnimFileName = G2API_GetAnimFileName;
		re.G2API_GetAnimFileNameIndex = G2API_GetAnimFileNameIndex;
		re.G2API_GetAnimRange = G2API_GetAnimRange;
		re.G2API_GetBoltMatrix = G2API_GetBoltMatrix;
		re.G2API_GetBoneAnim = G2API_GetBoneAnim;
		re.G2API_GetBoneIndex = G2API_GetBoneIndex;
		re.G2API_GetGhoul2ModelFlags = G2API_GetGhoul2ModelFlags;
		re.G2API_GetGLAName = G2API_GetGLAName;
		re.G2API_GetModelName = G2API_GetModelName;
		re.G2API_GetParentSurface = G2API_GetParentSurface;
		re.G2API_GetRagBonePos = G2API_GetRagBonePos;
		re.G2API_GetSurfaceIndex = G2API_GetSurfaceIndex;
		re.G2API_GetSurfaceName = G2API_GetSurfaceName;
		re.G2API_GetSurfaceOnOff = G2API_GetSurfaceOnOff;
		re.G2API_GetSurfaceRenderStatus = G2API_GetSurfaceRenderStatus;
		re.G2API_GetTime = G2API_GetTime;
		re.G2API_Ghoul2Size = G2API_Ghoul2Size;
		re.G2API_GiveMeVectorFromMatrix = G2API_GiveMeVectorFromMatrix;
		re.G2API_HasGhoul2ModelOnIndex = G2API_HasGhoul2ModelOnIndex;
		re.G2API_HaveWeGhoul2Models = G2API_HaveWeGhoul2Models;
		re.G2API_IKMove = G2API_IKMove;
		re.G2API_InitGhoul2Model = G2API_InitGhoul2Model;
		re.G2API_IsGhoul2InfovValid = G2API_IsGhoul2InfovValid;
		re.G2API_IsPaused = G2API_IsPaused;
		re.G2API_ListBones = G2API_ListBones;
		re.G2API_ListSurfaces = G2API_ListSurfaces;
		re.G2API_LoadGhoul2Models = G2API_LoadGhoul2Models;
		re.G2API_LoadSaveCodeDestructGhoul2Info = G2API_LoadSaveCodeDestructGhoul2Info;
		re.G2API_OverrideServerWithClientData = G2API_OverrideServerWithClientData;
		re.G2API_PauseBoneAnim = G2API_PauseBoneAnim;
		re.G2API_PrecacheGhoul2Model = G2API_PrecacheGhoul2Model;
		re.G2API_RagEffectorGoal = G2API_RagEffectorGoal;
		re.G2API_RagEffectorKick = G2API_RagEffectorKick;
		re.G2API_RagForceSolve = G2API_RagForceSolve;
		re.G2API_RagPCJConstraint = G2API_RagPCJConstraint;
		re.G2API_RagPCJGradientSpeed = G2API_RagPCJGradientSpeed;
		re.G2API_RemoveBolt = G2API_RemoveBolt;
		re.G2API_RemoveBone = G2API_RemoveBone;
		re.G2API_RemoveGhoul2Model = G2API_RemoveGhoul2Model;
		re.G2API_RemoveGhoul2Models = G2API_RemoveGhoul2Models;
		re.G2API_RemoveSurface = G2API_RemoveSurface;
		re.G2API_ResetRagDoll = G2API_ResetRagDoll;
		re.G2API_SaveGhoul2Models = G2API_SaveGhoul2Models;
		re.G2API_SetBoltInfo = G2API_SetBoltInfo;
		re.G2API_SetBoneAngles = G2API_SetBoneAngles;
		re.G2API_SetBoneAnglesIndex = G2API_SetBoneAnglesIndex;
		re.G2API_SetBoneAnglesMatrix = G2API_SetBoneAnglesMatrix;
		re.G2API_SetBoneAnglesMatrixIndex = G2API_SetBoneAnglesMatrixIndex;
		re.G2API_SetBoneAnim = G2API_SetBoneAnim;
		re.G2API_SetBoneAnimIndex = G2API_SetBoneAnimIndex;
		re.G2API_SetBoneIKState = G2API_SetBoneIKState;
		re.G2API_SetGhoul2ModelIndexes = G2API_SetGhoul2ModelIndexes;
		re.G2API_SetGhoul2ModelFlags = G2API_SetGhoul2ModelFlags;
		re.G2API_SetLodBias = G2API_SetLodBias;
		re.G2API_SetNewOrigin = G2API_SetNewOrigin;
		re.G2API_SetRagDoll = G2API_SetRagDoll;
		re.G2API_SetRootSurface = G2API_SetRootSurface;
		re.G2API_SetShader = G2API_SetShader;
		re.G2API_SetSkin = G2API_SetSkin;
		re.G2API_SetSurfaceOnOff = G2API_SetSurfaceOnOff;
		re.G2API_SetTime = G2API_SetTime;
		re.G2API_SkinlessModel = G2API_SkinlessModel;
		re.G2API_StopBoneAngles = G2API_StopBoneAngles;
		re.G2API_StopBoneAnglesIndex = G2API_StopBoneAnglesIndex;
		re.G2API_StopBoneAnim = G2API_StopBoneAnim;
		re.G2API_StopBoneAnimIndex = G2API_StopBoneAnimIndex;

#ifdef _G2_GORE
		re.G2API_GetNumGoreMarks = G2API_GetNumGoreMarks;
		re.G2API_AddSkinGore = G2API_AddSkinGore;
		re.G2API_ClearSkinGore = G2API_ClearSkinGore;
#endif // _SOF2

		// this is set in R_Init
		//re.G2VertSpaceServer	= G2VertSpaceServer;

		re.ext.Font_StrLenPixels = RE_Font_StrLenPixelsNew;

		return &re;
	}
} //extern "C"