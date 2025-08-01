/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// gl_draw.c -- this is the only file outside the refresh that touches the vid buffer

#include "quakedef.h"

extern int ColorIndex[16];
extern unsigned ColorPercent[16];

#define MAX_DISC 18

cvar_t		scr_conalpha = {"scr_conalpha", "1", true};
//cvar_t		gl_nobind = {"gl_nobind", "0"};
//cvar_t		gl_max_size = {"gl_max_size", "2048"};
//cvar_t		gl_round_down = {"gl_round_down", "0"};
//cvar_t		gl_picmip = {"gl_picmip", "0"};
//cvar_t		gl_swapinterval = {"gl_swapinterval", "0", true};
//cvar_t		gl_warp_image_size = {"gl_warp_image_size", "256", true}; // was 512, for water warp

byte		*draw_chars;				// 8*8 graphic characters
byte		*draw_smallchars;			// Small characters for status bar
byte		*draw_menufont; 			// Big Menu Font
qpic_t		*draw_disc[MAX_DISC] = { NULL }; // make the first one null for sure
qpic_t		*draw_backtile;

gltexture_t *notexture;
gltexture_t *nulltexture;
gltexture_t			*translate_texture[MAX_PLAYER_CLASS];
gltexture_t			*char_texture;
gltexture_t			*char_smalltexture;
gltexture_t			*char_menufonttexture;


int		indexed_bytes = 1;
int		rgba_bytes = 4;
int		bgra_bytes = 4;

int		gl_lightmap_format = 4;
int		gl_solid_format = GL_RGB; // was 3
int		gl_alpha_format = GL_RGBA; // was 4

#define MAXGLMODES 6

typedef struct
{
	int magfilter;
	int minfilter;
	char *name;
} glmode_t;

glmode_t modes[MAXGLMODES] = {
	{GL_NEAREST, GL_NEAREST,				"GL_NEAREST"},
	{GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST,	"GL_NEAREST_MIPMAP_NEAREST"},
	{GL_NEAREST, GL_NEAREST_MIPMAP_LINEAR,	"GL_NEAREST_MIPMAP_LINEAR"},
	{GL_LINEAR,  GL_LINEAR,					"GL_LINEAR"},
	{GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST,	"GL_LINEAR_MIPMAP_NEAREST"},
	{GL_LINEAR,  GL_LINEAR_MIPMAP_LINEAR,	"GL_LINEAR_MIPMAP_LINEAR"},
};

int		gl_texturemode = 3; // linear
int		gl_filter_min = GL_LINEAR; // was GL_NEAREST
int		gl_filter_mag = GL_LINEAR;

float	gl_hardware_max_anisotropy = 1; // just in case
float 	gl_texture_anisotropy = 1;

int		gl_hardware_max_size = 1024; // just in case
int		gl_texture_max_size = 1024;

int		gl_warpimage_size = 256; // fitzquake has 512, for water warp

#define MAX_GLTEXTURES	4096 // orig was 2048
gltexture_t	*active_gltextures, *free_gltextures;
int			numgltextures;


GLuint currenttexture = (GLuint)-1; // to avoid unnecessary texture sets
GLenum TEXTURE0, TEXTURE1;
qboolean mtexenabled = false;

unsigned int d_8to24table[256]; // for GL renderer
unsigned int d_8to24table_fbright[256];
unsigned int d_8to24table_nobright[256];
unsigned int d_8to24table_conchars[256];
unsigned int d_8to24TranslucentTable[256];

//const char *gl_vendor;
//const char *gl_renderer;
//const char *gl_version;
//const char *gl_extensions;

//qboolean fullsbardraw = false;
//qboolean isIntel = false; // intel video workaround
//qboolean gl_mtexable = false;
//qboolean gl_texture_env_combine = false;
//qboolean gl_texture_env_add = false;
//qboolean gl_swap_control = false;


//int		texture_mode = GL_LINEAR;

//int		texture_extension_number = 1;
/*
================================================

			OpenGL Stuff

================================================
*/
 
/*
================
GL_CheckSize

return smallest power of two greater than or equal to size
================
*/
//int GL_CheckSize (int size)
//{
//	int checksize;
//
//	for (checksize = 1; checksize < size; checksize <<= 1)
//		;
//
//	return checksize;
//}

/*
===============
GL_UploadWarpImage

called during init,
choose correct warpimage size and reload existing warpimage textures if needed
===============
*/
//void GL_UploadWarpImage (void)
//{
//	int	oldsize;
//	int mark;
//	gltexture_t *glt;
//	byte *dummy;
//
//	//
//	// find the new correct size
//	//
//	oldsize = gl_warpimage_size;
//
//	if ((int)gl_warp_image_size.value < 32)
//		Cvar_SetValue ("gl_warp_image_size", 32);
//
//	//
//	// make sure warpimage size is a power of two
//	//
//	gl_warpimage_size = GL_CheckSize((int)gl_warp_image_size.value);
//
//	while (gl_warpimage_size > vid.width)
//		gl_warpimage_size >>= 1;
//	while (gl_warpimage_size > vid.height)
//		gl_warpimage_size >>= 1;
//
//	if (gl_warpimage_size != gl_warp_image_size.value)
//		Cvar_SetValue ("gl_warp_image_size", gl_warpimage_size);
//
//	if (gl_warpimage_size == oldsize)
//		return;
//
//	//
//	// resize the textures in opengl
//	//
////	dummy = malloc (gl_warpimage_size*gl_warpimage_size*4);
//	mark = Hunk_LowMark();
//	dummy = Hunk_Alloc (gl_warpimage_size*gl_warpimage_size*4);
//
//	for (glt=active_gltextures; glt; glt=glt->next)
//	{
//		if (glt->flags & TEXPREF_WARPIMAGE)
//		{
//			GL_Bind (glt);
//			glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, gl_warpimage_size, gl_warpimage_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, dummy);
//			glt->width = glt->height = gl_warpimage_size;
//		}
//	}
//
////	free (dummy);
//	Hunk_FreeToLowMark (mark);
//}

/*
===============
GL_CheckExtensions
===============
*/
//void GL_CheckExtensions (void) 
//{
//	qboolean ARB = false;
//	qboolean EXTcombine, ARBcombine;
//	qboolean EXTadd, ARBadd;
//	qboolean SWAPcontrol;
//	qboolean anisotropy;
//	int units;
//
//	//
//	// poll max size from hardware
//	//
//	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &gl_hardware_max_size);
//	Con_Printf ("Maximum texture size %i\n", gl_hardware_max_size);
//
//	// by default we sets maxsize as hardware maxsize
//	gl_texture_max_size = gl_hardware_max_size; 
//
//	//
//	// Multitexture
//	//
//	ARB = strstr (gl_extensions, "GL_ARB_multitexture") != NULL;
//
//	if (COM_CheckParm("-nomtex"))
//	{
//		Con_Warning ("Multitexture disabled at command line\n");
//	}
//	else if (ARB)
//	{
//		// Check how many texture units there actually are
//		glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &units);
//
//		qglMultiTexCoord2f = (void *) qglGetProcAddress ("glMultiTexCoord2fARB");
//		qglActiveTexture = (void *) qglGetProcAddress ("glActiveTextureARB");
//
//		if (units < 2)
//		{
//			Con_Warning ("Only %i TMU available, multitexture not supported\n", units);
//		}
//		else if (!qglMultiTexCoord2f || !qglActiveTexture)
//		{
//			Con_Warning ("Multitexture not supported (qglGetProcAddress failed)\n");
//		}
//		else
//		{
//			TEXTURE0 = GL_TEXTURE0_ARB;
//			TEXTURE1 = GL_TEXTURE1_ARB;
//
//			Con_Printf ("GL_ARB_multitexture extension found\n");
//			Con_Printf ("   %i TMUs on hardware\n", units);
//
//			gl_mtexable = true;
//		} 
//	}
//	else
//	{
//		Con_Warning ("Multitexture not supported (extension not found)\n");
//	}
//
//	//
//	// Texture combine environment mode
//	//
//	ARBcombine = strstr (gl_extensions, "GL_ARB_texture_env_combine") != NULL;
//	EXTcombine = strstr (gl_extensions, "GL_EXT_texture_env_combine") != NULL;
//
//	if (COM_CheckParm("-nocombine"))
//	{
//		Con_Warning ("Texture combine environment disabled at command line\n");
//	}
//	else if (ARBcombine || EXTcombine)
//	{
//		Con_Printf ("GL_%s_texture_env_combine extension found\n", ARBcombine ? "ARB" : "EXT");
//		gl_texture_env_combine = true;
//	}
//	else
//	{
//		Con_Warning ("Texture combine environment not supported (extension not found)\n");
//	}
//
//	//
//	// Texture add environment mode
//	//
//	ARBadd = strstr (gl_extensions, "GL_ARB_texture_env_add") != NULL;
//	EXTadd = strstr (gl_extensions, "GL_EXT_texture_env_add") != NULL;
//
//	if (COM_CheckParm("-noadd"))
//	{
//		Con_Warning ("Texture add environment disabled at command line\n");
//	}
//	else if (ARBadd || EXTadd)
//	{
//		Con_Printf ("GL_%s_texture_env_add extension found\n", ARBadd ? "ARB" : "EXT");
//		gl_texture_env_add = true;
//	}
//	else
//	{
//		Con_Warning ("Texture add environment not supported (extension not found)\n");
//	}
//
//	//
//	// Swap control
//	//
//#ifdef _WIN32
//	SWAPcontrol = strstr (gl_extensions, SWAPCONTROLSTRING) != NULL;
//#else
//	SWAPcontrol = strstr (glx_extensions, SWAPCONTROLSTRING) != NULL;
//#endif
//
//	if (COM_CheckParm("-novsync"))
//	{
//		Con_Warning ("Vertical sync disabled at command line\n");
//	}
//	else if (SWAPcontrol)
//	{
//		qglSwapInterval = (void *) qglGetProcAddress (SWAPINTERVALFUNC);
//
//		if (qglSwapInterval)
//		{
//			if (!qglSwapInterval(0))
//				Con_Warning ("Vertical sync not supported (%s failed)\n", SWAPINTERVALFUNC);
//			else
//			{
//				Con_Printf ("%s extension found\n", SWAPCONTROLSTRING);
//				gl_swap_control = true;
//			}
//		}
//		else
//			Con_Warning ("Vertical sync not supported (qglGetProcAddress failed)\n");
//	}
//	else
//		Con_Warning ("Vertical sync not supported (extension not found)\n");
//
//	//
//	// Anisotropic filtering
//	//
//	anisotropy = strstr (gl_extensions, "GL_EXT_texture_filter_anisotropic") != NULL;
//
//	if (COM_CheckParm("-noanisotropy"))
//	{
//		Con_Warning ("Anisotropic filtering disabled at command line\n");
//	}
//	else if (anisotropy)
//	{
//		float test1, test2;
//		GLuint tex;
//
//		// test to make sure we really have control over it
//		// 1.0 and 2.0 should always be legal values
//		glGenTextures (1, &tex);
//		glBindTexture (GL_TEXTURE_2D, tex);
//		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
//		glGetTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &test1);
//		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f);
//		glGetTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &test2);
//		glDeleteTextures (1, &tex);
//
//		if (test1 == 1 && test2 == 2)
//			Con_Printf ("GL_EXT_texture_filter_anisotropic extension found\n");
//		else
//			Con_Warning ("Anisotropic filtering locked by driver. Current driver setting is %f\n", test1);
//
//		// get max value either way, so the menu and stuff know it
//		glGetFloatv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_hardware_max_anisotropy);
//	}
//	else
//	{
//		Con_Warning ("Anisotropic filtering not supported (extension not found)\n");
//	}
//}

/*
===============
GL_MakeExtensionsList
===============
*/
//char *GL_MakeExtensionsList (const char *in)
//{
//	char *copy, *token, *out;
//	int i, count;
//
//	// each space will be replaced by 4 chars, so count the spaces before we malloc
//	for (i = 0, count = 1; i < (int)strlen(in); i++)
//		if (in[i] == ' ')
//			count++;
//	out = Z_Malloc (strlen(in) + count*3 + 1); // usually about 1-2k
//	out[0] = 0;
//
//	copy = Z_Malloc(strlen(in) + 1);
//	strcpy(copy, in);
//
//	for (token = strtok(copy, " "); token; token = strtok(NULL, " "))
//	{
//		strcat(out, "\n   ");
//		strcat(out, token);
//	}
//
//	Z_Free (copy);
//	return out;
//}

/*
===============
GL_Info_f
===============
*/
//void GL_Info_f (void)
//{
//	static char *gl_extensions_nice = NULL;
//#ifndef _WIN32
//	static char *glx_extensions_nice = NULL;
//#endif
//
//	if (!gl_extensions_nice)
//		gl_extensions_nice = GL_MakeExtensionsList (gl_extensions);
//#ifndef _WIN32
//	if (!glx_extensions_nice)
//		glx_extensions_nice = GL_MakeExtensionsList (glx_extensions);
//#endif
//
//	Con_SafePrintf ("GL_VENDOR: %s\n", gl_vendor);
//	Con_SafePrintf ("GL_RENDERER: %s\n", gl_renderer);
//	Con_SafePrintf ("GL_VERSION: %s\n", gl_version);
//	Con_SafePrintf ("GL_EXTENSIONS: %s\n", gl_extensions_nice);
//#ifndef _WIN32
//	Con_SafePrintf ("GLX_EXTENSIONS: %s\n", glx_extensions_nice);
//#endif
//}

/*
===============
GL_SwapInterval
===============
*/
//void GL_SwapInterval (void)
//{
//	if (gl_swap_control)
//	{
//		if (gl_swapinterval.value)
//		{
//			if (!qglSwapInterval(1))
//				Con_Printf ("GL_SwapInterval: failed on %s\n", SWAPINTERVALFUNC);
//		}
//		else
//		{
//			if (!qglSwapInterval(0))
//				Con_Printf ("GL_SwapInterval: failed on %s\n", SWAPINTERVALFUNC);
//		}
//	}
//}

/*
===============
GL_SetupState

does all the stuff from GL_Init that needs to be done every time a new GL render context is created
GL_Init will still do the stuff that only needs to be done once
===============
*/
//void GL_SetupState (void)
//{
//	glClearColor (0.15,0.15,0.15,0); // originally was 1,0,0,0
//
//	glCullFace (GL_BACK); // glquake used CCW with backwards culling -- let's do it right
//	glFrontFace (GL_CW); // glquake used CCW with backwards culling -- let's do it right
//
//	glEnable (GL_TEXTURE_2D);
//
//	glEnable (GL_ALPHA_TEST);
//	glAlphaFunc (GL_GREATER, 0.666);
//
//	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
//	glShadeModel (GL_FLAT);
//
//	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
//	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 
//
//	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // was GL_NEAREST
//	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // was GL_NEAREST
//	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//	glDepthRange (0, 1); // moved here because gl_ztrick is gone.
//	glDepthFunc (GL_LEQUAL); // moved here because gl_ztrick is gone.
//}

/*
===============
GL_Init
===============
*/
//void GL_Init (void)
//{
//	// gl_info
//	gl_vendor = (const char *)glGetString (GL_VENDOR);
////	Con_SafePrintf ("GL_VENDOR: %s\n", gl_vendor);
//	gl_renderer = (const char *)glGetString (GL_RENDERER);
////	Con_SafePrintf ("GL_RENDERER: %s\n", gl_renderer);
//	gl_version = (const char *)glGetString (GL_VERSION);
////	Con_SafePrintf ("GL_VERSION: %s\n", gl_version);
//	gl_extensions = (const char *)glGetString (GL_EXTENSIONS);
////	Con_SafePrintf ("GL_EXTENSIONS: %s\n", gl_extensions);
//#ifndef _WIN32
////	Con_SafePrintf ("GLX_EXTENSIONS: %s\n", glx_extensions);
//#endif
//
//	Cmd_AddCommand ("gl_info", GL_Info_f);
//	Cmd_AddCommand ("gl_reloadtextures", GL_ReloadTextures_f);
//
//	Cvar_RegisterVariable (&gl_swapinterval, GL_SwapInterval);
//
//	Con_Printf ("OpenGL initialized\n");
//
//	GL_CheckExtensions ();
//
//	if (!strcmp(gl_vendor, "Intel")) // intel video workaround
//	{
//		Con_Printf ("Intel Display Adapter detected\n");
//		isIntel = true;
//	}
//
//	GL_SetupState ();
//}

/*
================
GL_Bind
================
*/
//void GL_Bind (gltexture_t *texture)
//{
//	if (!texture)
//		texture = nulltexture;
//
//	if (texture->texnum != currenttexture)
//	{
//		currenttexture = texture->texnum;
//		glBindTexture (GL_TEXTURE_2D, currenttexture);
//	}
//}

/*
================
GL_SelectTexture
================
*/
//void GL_SelectTexture (GLenum target)
//{
//	static GLenum currenttarget;
//	static int ct0, ct1;
//
//	if (target == currenttarget)
//		return;
//
//	qglActiveTexture (target);
//
//	if (target == TEXTURE0)
//	{
//		ct1 = currenttexture;
//		currenttexture = ct0;
//	}
//	else //target == TEXTURE1
//	{
//		ct0 = currenttexture;
//		currenttexture = ct1;
//	}
//
//	currenttarget = target;
//}

/*
================
GL_DisableMultitexture -- selects texture unit 0
================
*/
//void GL_DisableMultitexture (void) 
//{
//	if (mtexenabled) 
//	{
//		glDisable (GL_TEXTURE_2D);
//		GL_SelectTexture (TEXTURE0);
//		mtexenabled = false;
//	}
//}

/*
================
GL_EnableMultitexture -- selects texture unit 1
================
*/
//void GL_EnableMultitexture (void) 
//{
//	if (gl_mtexable) 
//	{
//		GL_SelectTexture (TEXTURE1);
//		glEnable (GL_TEXTURE_2D);
//		mtexenabled = true;
//	}
//}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar objects into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

#define MAX_SCRAPS		1
#define BLOCK_WIDTH		256
#define BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT];
qboolean	scrap_dirty;
gltexture_t			*scrap_textures[MAX_SCRAPS]; // changed to array


// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
}

void Scrap_Upload (void)
{
	int		i;
	char name[64];

//	GL_Bind(scrap_textures);
//	sprintf (name, "scrap");
//	GL_Upload8 (name, scrap_texels[0], BLOCK_WIDTH, BLOCK_HEIGHT, false, true, 0);
	for (i = 0; i < MAX_SCRAPS; i++) 
	{
		sprintf (name, "scrap%i", i);
		scrap_textures[i] = GL_LoadTexture (NULL, name, BLOCK_WIDTH, BLOCK_HEIGHT, SRC_INDEXED, scrap_texels[i], "", (unsigned)scrap_texels[i], TEXPREF_ALPHA | TEXPREF_OVERWRITE | TEXPREF_NOPICMIP);
	}

	scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char		name[MAX_QPATH];
	qpic_t		pic;
	byte		padding[32];	// for appended glpic
} cachepic_t;

#define MAX_CACHED_PICS 	256
cachepic_t	menu_cachepics[MAX_CACHED_PICS];
int			menu_numcachepics;

/*
 * Geometry for the player/skin selection screen image.
 */
#define PLAYER_PIC_WIDTH 68
#define PLAYER_PIC_HEIGHT 114
#define PLAYER_DEST_WIDTH 128
#define PLAYER_DEST_HEIGHT 128

byte		menuplyr_pixels[MAX_PLAYER_CLASS][PLAYER_PIC_WIDTH*PLAYER_PIC_HEIGHT];


qpic_t *Draw_PicFromWad (char *name)
{
	qpic_t	*p;
	glpic_t *gl;
	unsigned offset; //johnfitz
	char texturename[64]; //johnfitz

	p = W_GetLumpName (name);

	// Sanity ...
	if (p->width & 0xC0000000 || p->height & 0xC0000000)
		Sys_Error ("Draw_PicFromWad: invalid dimensions (%dx%d) for '%s'", p->width, p->height, name);

	gl = (glpic_t *)p->data;

	// load little ones into the scrap
	if (p->width < 64 && p->height < 64)
	{
		int		x, y;
		int		i, j, k;
		int		texnum;

		texnum = Scrap_AllocBlock (p->width, p->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;

		scrap_dirty = true;
		k = 0;
		for (i=0 ; i<p->height ; i++)
			for (j=0 ; j<p->width ; j++, k++)
				scrap_texels[texnum][(y+i)*BLOCK_WIDTH + x + j] = p->data[k];

//		texnum += scrap_textures;
//		gl->gltexture = texnum;
		gl->gltexture = scrap_textures[texnum]; // changed to an array

		// no longer go from 0.01 to 0.99
		gl->sl = x/(float)BLOCK_WIDTH;
		gl->sh = (x+p->width)/(float)BLOCK_WIDTH;
		gl->tl = y/(float)BLOCK_WIDTH;
		gl->th = (y+p->height)/(float)BLOCK_WIDTH;

	}
	else
	{
nonscrap:
//		gl->gltexture = GL_LoadTexture (name, p->width, p->height, p->data, false, true, 0);
		sprintf (texturename, "%s:%s", WADFILE, name); //johnfitz
		offset = (unsigned)p - (unsigned)wad_base + sizeof(int)*2; //johnfitz
		gl->gltexture = GL_LoadTexture (NULL, texturename, p->width, p->height, SRC_INDEXED, p->data, WADFILE, offset, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
		gl->sl = 0;
		gl->sh = 1;
		gl->tl = 0;
		gl->th = 1;
	}
	return p;
}


qpic_t *Draw_PicFromFile (char *name)
{
	qpic_t	*p;
	glpic_t *gl;

	p = (qpic_t *)COM_LoadHunkFile (name, NULL);
	if (!p)
	{
		return NULL;
	}

	gl = (glpic_t *)p->data;

	{
//		gl->gltexture = GL_LoadTexture (name, p->width, p->height, p->data, false, true, 0);
		gl->gltexture = GL_LoadTexture (NULL, name, p->width, p->height, SRC_INDEXED, p->data, name, sizeof(int)*2, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
		gl->sl = 0;
		gl->sh = 1;
		gl->tl = 0;
		gl->th = 1;
	}
	return p;
}

/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	cachepic_t	*pic;
	int			i;
	qpic_t		*dat;
	glpic_t 	*gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS (%d)", MAX_CACHED_PICS);
	menu_numcachepics++;
	strcpy (pic->name, path);

//
// load the pic from disk
//
	dat = (qpic_t *)COM_LoadTempFile (path, NULL);
	if (!dat)
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	SwapPic (dat);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	/* garymct */
	if (!strcmp (path, "gfx/menu/netp1.lmp"))
		memcpy (menuplyr_pixels[0], dat->data, dat->width*dat->height);
	else if (!strcmp (path, "gfx/menu/netp2.lmp"))
		memcpy (menuplyr_pixels[1], dat->data, dat->width*dat->height);
	else if (!strcmp (path, "gfx/menu/netp3.lmp"))
		memcpy (menuplyr_pixels[2], dat->data, dat->width*dat->height);
	else if (!strcmp (path, "gfx/menu/netp4.lmp"))
		memcpy (menuplyr_pixels[3], dat->data, dat->width*dat->height);
	else if (!strcmp (path, "gfx/menu/netp5.lmp"))
		memcpy (menuplyr_pixels[4], dat->data, dat->width*dat->height);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	gl = (glpic_t *)pic->pic.data;
//	gl->gltexture = GL_LoadTexture (path, dat->width, dat->height, dat->data, false, true, 0);
	gl->gltexture = GL_LoadTexture (NULL, path, dat->width, dat->height, SRC_INDEXED, dat->data, path, sizeof(int)*2, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	return &pic->pic;
}

/*
================
Draw_CachePicNoTrans

Pa3PyX: Function added to cache pics ignoring transparent colors
(e.g. in intermission screens)
================
*/
qpic_t *Draw_CachePicNoTrans(char *path)
{
	cachepic_t      *pic;
	int                     i;
	qpic_t          *dat;
	glpic_t         *gl;

	for (pic=menu_cachepics, i=0 ; i<menu_numcachepics ; pic++, i++)
		if (!strcmp (path, pic->name))
			return &pic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS (%d)", MAX_CACHED_PICS);
	menu_numcachepics++;
	strcpy (pic->name, path);

//
// load the pic from disk
//
	dat = (qpic_t *)COM_LoadTempFile (path, NULL);
	if (!dat)
		Sys_Error ("Draw_CachePicNoTrans: failed to load %s", path);
	SwapPic (dat);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	// Get rid of transparencies
	for (i = 0; i < dat->width * dat->height; i++)
	{
		if (dat->data[i] == 255)
			dat->data[i] = 31; // pal(31) == pal(255) == FCFCFC (white)
	}

	gl = (glpic_t *)pic->pic.data;
//	gl->gltexture = GL_LoadTexture (path, dat->width, dat->height, dat->data, false, true, 0);
	gl->gltexture = GL_LoadTexture (NULL, path, dat->width, dat->height, SRC_INDEXED, dat->data, path, sizeof(int)*2, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	gl->sl = 0;
	gl->sh = 1;
	gl->tl = 0;
	gl->th = 1;

	return &pic->pic;
}


/*
===============
GL_SetFilterModes
===============
*/
//void GL_SetFilterModes (gltexture_t *glt)
//{
//	GL_Bind (glt);
//
//	if (glt->flags & TEXPREF_NEAREST)
//	{
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	}
//	else if (glt->flags & TEXPREF_LINEAR)
//	{
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	}
//	else if (glt->flags & TEXPREF_MIPMAP)
//	{
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, modes[gl_texturemode].magfilter);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, modes[gl_texturemode].minfilter);
//	}
//	else
//	{
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, modes[gl_texturemode].magfilter);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, modes[gl_texturemode].magfilter);
//	}
//
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_texture_anisotropy);
//}

/*
===============
GL_TextureMode_f
===============
*/
//void GL_TextureMode_f (void)
//{
//	int		i;
//	char *arg;
//	gltexture_t	*glt;
//
//	if (Cmd_Argc() == 1)
//	{
//		for (i=0 ; i< MAXGLMODES ; i++)
//		{
//			if (gl_filter_min == modes[i].minfilter)
//			{
//				Con_Printf ("gl_texturemode is %s (%d)\n", modes[i].name, i + 1);
//				return;
//			}
//		}
//	}
//
//	for (i=0 ; i< MAXGLMODES ; i++)
//	{
//		arg = Cmd_Argv(1);
//		if (!strcasecmp (modes[i].name, arg ) || (isdigit(*arg) && atoi(arg) - 1 == i))
//			break;
//	}
//	if (i == MAXGLMODES)
//	{
//		Con_Printf ("bad filter name, available are:\n");
//		for (i=0 ; i< MAXGLMODES ; i++)
//			Con_Printf ("%s (%d)\n", modes[i].name, i + 1);
//		return;
//	}
//
//	gl_texturemode = i;
//	gl_filter_min = modes[gl_texturemode].minfilter;
//	gl_filter_mag = modes[gl_texturemode].magfilter;
//
//	// change all the existing texture objects
//	for (glt=active_gltextures; glt; glt=glt->next)
//		GL_SetFilterModes (glt);
//
//	Sbar_Changed (); // sbar graphics need to be redrawn with new filter mode
//}

/*
===============
GL_Texture_Anisotropy_f
===============
*/
//void GL_Texture_Anisotropy_f (void)
//{
//	gltexture_t	*glt;
//
//	if (Cmd_Argc() == 1)
//	{
//		Con_Printf ("gl_texture_anisotropy is %f, hardware limit is %f\n", gl_texture_anisotropy, gl_hardware_max_anisotropy);
//		return;
//	}
//
//	gl_texture_anisotropy = CLAMP(1.0f, atof (Cmd_Argv(1)), gl_hardware_max_anisotropy);
//
//	for (glt=active_gltextures; glt; glt=glt->next)
//		GL_SetFilterModes (glt);
//}


/*
===============
Pics_Upload
===============
*/
void Pics_Upload (void)
{
	int		i;
	qpic_t	*mf;
	char texturepath[MAX_QPATH];
	unsigned	offset; // johnfitz
	char		texturename[64]; //johnfitz


	//
	// load console charset
	//
	//draw_chars = W_GetLumpName ("conchars");
	sprintf (texturepath, "%s", "gfx/menu/conchars.lmp"); // EER1
	draw_chars = COM_LoadHunkFile (texturepath, NULL);
/*	for (i=0 ; i<256*128 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color
*/
	if (!draw_chars)
		Sys_Error ("Pics_Upload: couldn't load conchars");

	// now turn them into textures
//	char_texture = GL_LoadTexture ("charset", 256, 128, draw_chars, false, true, 0);
	char_texture = GL_LoadTexture (NULL, texturepath, 256, 128, SRC_INDEXED, draw_chars, texturepath, sizeof(int)*2, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);


	//
	// load sbar small font
	//
	draw_smallchars = W_GetLumpName("tinyfont");
/*	for (i=0 ; i<128*32 ; i++)
		if (draw_smallchars[i] == 0)
			draw_smallchars[i] = 255;	// proper transparent color
*/
	if (!draw_smallchars)
		Sys_Error ("Pics_Upload: couldn't load tinyfont");

	// now turn them into textures
//	char_smalltexture = GL_LoadTexture ("smallcharset", 128, 32, draw_smallchars, false, true, 0);
	sprintf (texturename, "%s:%s", WADFILE, "smallcharset"); // EER1
	offset = (unsigned)draw_smallchars - (unsigned)wad_base;
	char_smalltexture = GL_LoadTexture (NULL, texturename, 128, 32, SRC_INDEXED, draw_smallchars, WADFILE, offset, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);


	//
	// load menu big font
	//
	sprintf (texturepath, "%s", "gfx/menu/bigfont2.lmp"); // EER1
//	mf = (qpic_t *)COM_LoadTempFile("gfx/menu/bigfont.lmp", NULL);
	mf = (qpic_t *)COM_LoadTempFile(texturepath, NULL);
/*	for (i=0 ; i<160*80 ; i++)
		if (mf->data[i] == 0)
			mf->data[i] = 255;	// proper transparent color
*/
	if (!mf)
		Sys_Error ("Pics_Upload: couldn't load menufont");

	// now turn them into textures
//	char_menufonttexture = GL_LoadTexture ("menufont", 160, 80, mf->data, false, true, 0);
	char_menufonttexture = GL_LoadTexture (NULL, texturepath, 160, 80, SRC_INDEXED, mf->data, texturepath, sizeof(int)*2, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);
	
	//
	// get the other pics we need
	//
//	draw_disc = Draw_PicFromWad ("disc");
	// Do this backwards so we don't try and draw the 
	// skull as we are loading
	for(i=MAX_DISC-1 ; i>=0 ; i--)
	{
		sprintf(texturepath, "gfx/menu/skull%d.lmp", i);
		draw_disc[i] = Draw_PicFromFile (texturepath);
	}

//	draw_backtile = Draw_PicFromWad ("backtile");
	draw_backtile = Draw_PicFromFile ("gfx/menu/backtile.lmp");

}

/*
===============
Draw_Init

rewritten
===============
*/
void Draw_Init (void)
{
//	int i;
//	static byte notexture_data[16] = {159,91,83,255,0,0,0,255,0,0,0,255,159,91,83,255}; // black and pink checker
//	static byte nulltexture_data[16] = {127,191,255,255,0,0,0,255,0,0,0,255,127,191,255,255}; // black and blue checker

//	// init texture list
//	free_gltextures = (gltexture_t *) Hunk_AllocName (MAX_GLTEXTURES * sizeof(gltexture_t), "gltextures");
//	active_gltextures = NULL;
//	for (i=0; i<MAX_GLTEXTURES-1; i++)
//		free_gltextures[i].next = &free_gltextures[i+1];
//	free_gltextures[i].next = NULL;
//	numgltextures = 0;
//
//	for(i=0;i<MAX_EXTRA_TEXTURES;i++)
//		gl_extra_textures[i] = NULL;


	Cvar_RegisterVariable (&scr_conalpha);
//	Cvar_RegisterVariable (&gl_picmip, NULL);
//	Cvar_RegisterVariable (&gl_warp_image_size, GL_UploadWarpImage);

//	Cmd_AddCommand ("gl_texturemode", &GL_TextureMode_f);
//	Cmd_AddCommand ("gl_texture_anisotropy", &GL_Texture_Anisotropy_f);

	// load notexture images
//	notexture = GL_LoadTexture (NULL, "notexture", 2, 2, SRC_RGBA, notexture_data, "", (unsigned)notexture_data, TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP);
//	nulltexture = GL_LoadTexture (NULL, "nulltexture", 2, 2, SRC_RGBA, nulltexture_data, "", (unsigned)nulltexture_data, TEXPREF_NEAREST | TEXPREF_PERSIST | TEXPREF_NOPICMIP);

	// have to assign these here because R_InitTextures is called before Draw_Init
//	notexture_mip->gltexture = notexture_mip2->gltexture = notexture;

	// upload warpimage
//	GL_UploadWarpImage ();

	// clear scrap and allocate gltextures
	memset(&scrap_allocated, 0, sizeof(scrap_allocated));
	memset(&scrap_texels, 255, sizeof(scrap_texels));
	Scrap_Upload (); // creates 2 empty textures

	// load pics
	Pics_Upload ();
}

/*
===============
Draw_Crosshair
===============
*/
void Draw_Crosshair (void)
{
	if (!crosshair.value) 
		return;

//	Draw_Character (scr_vrect.x + scr_vrect.width/2 + cl_crossx.value, scr_vrect.y + scr_vrect.height/2 + cl_crossy.value, '+');
}

/*
================
Draw_CharacterQuad

seperate function to spit out verts
================
*/
void Draw_CharacterQuad (int x, int y, short num)
{
	int				row, col;
	float			frow, fcol, xsize, ysize;

	row = num>>5;
	col = num&31;

	xsize = 0.03125;
	ysize = 0.0625;
	fcol = col*xsize;
	frow = row*ysize;

	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + xsize, frow);
	glVertex2f (x+8, y);
	glTexCoord2f (fcol + xsize, frow + ysize);
	glVertex2f (x+8, y+8);
	glTexCoord2f (fcol, frow + ysize);
	glVertex2f (x, y+8);

}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.

modified to call Draw_CharacterQuad
================
*/
void Draw_Character (int x, int y, unsigned int num)
{
	if (y <= -8)
		return; 		// totally off screen

	num &= 511;

	if (num == 32)
		return; 	// don't waste verts on spaces

	GL_Bind (char_texture);
	glBegin (GL_QUADS);

	Draw_CharacterQuad (x, y, (short) num);

	glEnd ();
}

/*
================
Draw_String

modified to call Draw_CharacterQuad
================
*/
void Draw_String (int x, int y, char *str)
{
	if (y <= -8)
		return;			// totally off screen

	GL_Bind (char_texture);
	glBegin (GL_QUADS);

	while (*str)
	{
		if (*str != 32) // don't waste verts on spaces
			Draw_CharacterQuad (x, y, *str);
		str++;
		x += 8;
	}

	glEnd ();
}


/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x, int y, qpic_t *pic, float alpha)
{
	glpic_t			*gl;

	if (scrap_dirty)
		Scrap_Upload ();

	gl = (glpic_t *)pic->data;

	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glColor4f (1,1,1,alpha);
	GL_Bind (gl->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, gl->tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, gl->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (gl->sh, gl->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (gl->sl, gl->th);
	glVertex2f (x, y+pic->height);
	glEnd ();
	glColor4f (1,1,1,1);
	glEnable (GL_ALPHA_TEST);
	glDisable (GL_BLEND);
}


/*
================
Draw_SmallCharacterQuad

seperate function to spit out verts
================
*/
void Draw_SmallCharacterQuad (int x, int y, char num)
{
	int				row, col;
	float			frow, fcol, xsize, ysize;

	row = num>>4;
	col = num&15;

	xsize = 0.0625;
	ysize = 0.25;
	fcol = col*xsize;
	frow = row*ysize;

	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + xsize, frow);
	glVertex2f (x+8, y);
	glTexCoord2f (fcol + xsize, frow + ysize);
	glVertex2f (x+8, y+8);
	glTexCoord2f (fcol, frow + ysize);
	glVertex2f (x, y+8);
}

/*
================
Draw_SmallCharacter

Draws a small character that is clipped at the bottom edge of the
screen.

modified to call Draw_SmallCharacterQuad
================
*/
void Draw_SmallCharacter (int x, int y, int num)
{
	if (y <= -8)
		return; 		// totally off screen

	if (y >= vid.height)
		return;			// totally off screen

	if (num < 32)
		num = 0;
	else if (num >= 'a' && num <= 'z')
		num -= 64;
	else if (num > '_')
		num = 0;
	else
		num -= 32;

	if (num == 0)
		return;

	GL_Bind (char_smalltexture);
	glBegin (GL_QUADS);

	Draw_SmallCharacterQuad (x, y, (char) num);

	glEnd ();
}

/*
================
Draw_SmallString

modified to call Draw_SmallCharacterQuad
================
*/
void Draw_SmallString (int x, int y, char *str)
{
	int num;

	if (y <= -8)
		return; 		// totally off screen

	if (y >= vid.height)
		return;			// totally off screen

	GL_Bind (char_smalltexture);
	glBegin (GL_QUADS);

	while (*str)
	{
		num = *str;

		if (num < 32)
			num = 0;
		else if (num >= 'a' && num <= 'z')
			num -= 64;
		else if (num > '_')
			num = 0;
		else
			num -= 32;

		if (num != 0)
			Draw_SmallCharacterQuad (x, y, (char) num);

		str++;
		x += 6;
	}

	glEnd ();
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	glpic_t 		*gl;

	if (scrap_dirty)
		Scrap_Upload ();

	gl = (glpic_t *)pic->data;

	glDisable (GL_ALPHA_TEST); //FX new
	glEnable (GL_BLEND); //FX
	glColor4f (1,1,1,1);
	GL_Bind (gl->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, gl->tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, gl->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (gl->sh, gl->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (gl->sl, gl->th);
	glVertex2f (x, y+pic->height);
	glEnd ();
	glEnable (GL_ALPHA_TEST); //FX new
	glDisable (GL_BLEND); //FX
}

/*
=============
Draw_IntermissionPic

Pa3PyX: this new function introduced to draw the intermission screen only
=============
*/
void Draw_IntermissionPic (qpic_t *pic)
{
	glpic_t                 *gl;
	
	gl = (glpic_t *)pic->data;

	glDisable (GL_ALPHA_TEST); //FX new
	glEnable (GL_BLEND); //FX

	glColor4f (1,1,1,1);
	GL_Bind (gl->gltexture);
	
	
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(vid.width, 0.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(vid.width, vid.height);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(0.0f, vid.height);
	glEnd();

	glEnable (GL_ALPHA_TEST); //FX new
	glDisable (GL_BLEND); //FX

}

void Draw_PicCropped(int x, int y, qpic_t *pic)
{
	int height;
	glpic_t 		*gl;
	float th,tl;

	if((x < 0) || (x+pic->width > (int)vid.width))
	{
		Sys_Error("Draw_PicCropped: bad coordinates");
	}

	if (y >= (int)vid.height || y+pic->height < 0)
	{ // Totally off screen
		return;
	}

	if (scrap_dirty)
		Scrap_Upload ();

	gl = (glpic_t *)pic->data;

	// rjr tl/th need to be computed based upon pic->tl and pic->th
	//     cuz the piece may come from the scrap
	if(y+pic->height > (int)vid.height)
	{
		height = vid.height-y;
		tl = 0;
		th = (height-0.01)/pic->height;
	}
	else if (y < 0)
	{
		height = pic->height+y;
		y = -y;
		tl = (y-0.01)/pic->height;
		th = (pic->height-0.01)/pic->height;
		y = 0;
	}
	else
	{
		height = pic->height;
		tl = gl->tl;
		th = gl->th;//(height-0.01)/pic->height;
	}

	glDisable (GL_ALPHA_TEST); //FX new
	glEnable (GL_BLEND); //FX

	glColor4f (1,1,1,1);
	GL_Bind (gl->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (gl->sl, tl);
	glVertex2f (x, y);
	glTexCoord2f (gl->sh, tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (gl->sh, th);
	glVertex2f (x+pic->width, y+height);
	glTexCoord2f (gl->sl, th);
	glVertex2f (x, y+height);
	glEnd ();

	glEnable (GL_ALPHA_TEST); //FX new
	glDisable (GL_BLEND); //FX
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, qpic_t *pic)
{

	if (x < 0 || (x + pic->width) > vid.width || y < 0 || (y + pic->height) > vid.height)
	{
		Sys_Error ("Draw_TransPic: bad coordinates (%d, %d)", x, y);
	}

	Draw_Pic (x, y, pic);
}

void Draw_TransPicCropped(int x, int y, qpic_t *pic)
{
	Draw_PicCropped (x, y, pic);
}

/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
{
	byte		trans[PLAYER_DEST_WIDTH * PLAYER_DEST_HEIGHT];
	byte			*src, *dst;
	byte	*data;
	char	name[64];
	int i, j;


	data = menuplyr_pixels[setup_class-1];
	sprintf (name, "gfx/menu/netp%d.lmp", setup_class);

	dst = trans;
	src = data;

	for( i = 0; i < PLAYER_PIC_WIDTH; i++ )
	{
		for( j = 0; j < PLAYER_PIC_HEIGHT; j++ )
		{
			dst[j * PLAYER_DEST_WIDTH + i] = translation[src[j * PLAYER_PIC_WIDTH + i]];
		}
	}

	data = trans;

//	GL_Bind (translate_texture[setup_class-1]);
//	GL_Upload8 (name, data, PLAYER_DEST_WIDTH, PLAYER_DEST_HEIGHT, false, true, 0); 
	translate_texture[setup_class-1] = GL_LoadTexture (NULL, name, PLAYER_DEST_WIDTH, PLAYER_DEST_HEIGHT, SRC_INDEXED, data, "", (unsigned)data, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);



	glColor3f (1,1,1);
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex2f (x, y);
	glTexCoord2f (( float )PLAYER_PIC_WIDTH / PLAYER_DEST_WIDTH, 0);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (( float )PLAYER_PIC_WIDTH / PLAYER_DEST_WIDTH, ( float )PLAYER_PIC_HEIGHT / PLAYER_DEST_HEIGHT);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (0, ( float )PLAYER_PIC_HEIGHT / PLAYER_DEST_HEIGHT);
	glVertex2f (x, y+pic->height);
	glEnd ();
}



/*
================
Draw_BigCharacterQuad

seperate function to spit out verts
================
*/
void Draw_BigCharacterQuad (int x, int y, char num)
{
	int				row, col;
	float			frow, fcol, xsize, ysize;

	row = num/8;
	col = num%8;

	xsize = 0.125;
	ysize = 0.25;
	fcol = col*xsize;
	frow = row*ysize;

	glTexCoord2f (fcol, frow);
	glVertex2f (x, y);
	glTexCoord2f (fcol + xsize, frow);
	glVertex2f (x+20, y);
	glTexCoord2f (fcol + xsize, frow + ysize);
	glVertex2f (x+20, y+20);
	glTexCoord2f (fcol, frow + ysize);
	glVertex2f (x, y+20);
}

int M_DrawBigCharacter (int x, int y, int num, int numnext)
{
	int				add;

	if (num == ' ')
		return 32;

	if (num == '/')
		num = 26;
	else
		num -= 65;

	if (num < 0 || num >= 27)  // only a-z and /
		return 0;

	if (numnext == '/')
		numnext = 26;
	else
		numnext -= 65;

	GL_Bind (char_menufonttexture);
	glBegin (GL_QUADS);

	Draw_BigCharacterQuad (x, y, (char) num);

	glEnd ();

	if (numnext < 0 || numnext >= 27)
		return 0;

	add = 0;
	if (num == (int)'C'-65 && numnext == (int)'P'-65)
		add = 3;

	return BigCharWidth[num][numnext] + add;
}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground (int lines)
{
	qpic_t *pic;
	int y;
	float alpha;

	pic = Draw_CachePic ("gfx/menu/conback.lmp");
	pic->width = vid.width;
	pic->height = vid.height;

//	alpha = (con_forcedup) ? 1.0 : CLAMP(0.0, scr_conalpha.value, 1.0);
	alpha = 1.0;

	y = (vid.height * 3) >> 2;

	if (lines > y)
		Draw_Pic(0, lines - vid.height, pic);
	else
//		Draw_AlphaPic (0, lines - vid.height, pic, (float)(2 * alpha * lines)/y); //alpha depend on height console
		Draw_AlphaPic (0, lines - vid.height, pic, alpha);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	glpic_t	*gl;

	gl = (glpic_t *)draw_backtile->data;

	glDisable (GL_ALPHA_TEST); //FX new
	glEnable (GL_BLEND); //FX
//	glColor3f (1,1,1);
	glColor4f (1,1,1,1); //FX new
	GL_Bind (gl->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (x/64.0, y/64.0);
	glVertex2f (x, y);
	glTexCoord2f ( (x+w)/64.0, y/64.0);
	glVertex2f (x+w, y);
	glTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	glVertex2f (x+w, y+h);
	glTexCoord2f ( x/64.0, (y+h)/64.0 );
	glVertex2f (x, y+h);
	glEnd ();
	glEnable (GL_ALPHA_TEST); //FX new
	glDisable (GL_BLEND); //FX
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	byte *pal = (byte *)d_8to24table; // use d_8to24table instead of host_basepal
	float alpha = 1.0;

	glDisable (GL_TEXTURE_2D);
	glEnable (GL_BLEND); // for alpha
	glDisable (GL_ALPHA_TEST); // for alpha
	glColor4f (pal[c*4]/255.0, pal[c*4+1]/255.0, pal[c*4+2]/255.0, alpha); // added alpha

	glBegin (GL_QUADS);
	glVertex2f (x, y);
	glVertex2f (x+w, y);
	glVertex2f (x+w, y+h);
	glVertex2f (x, y+h);
	glEnd ();

	glColor3f (1,1,1);
	glDisable (GL_BLEND); // for alpha
	glEnable (GL_ALPHA_TEST); // for alpha
	glEnable (GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	int bx,by,ex,ey;
	int c;

	glAlphaFunc(GL_ALWAYS, 0);

	glEnable (GL_BLEND);
	glDisable (GL_TEXTURE_2D);

//	glColor4f (248.0/255.0, 220.0/255.0, 120.0/255.0, 0.2);
	glColor4f (208.0/255.0, 180.0/255.0, 80.0/255.0, 0.2);
	glBegin (GL_QUADS);
	glVertex2f (0,0);
	glVertex2f (vid.width, 0);
	glVertex2f (vid.width, vid.height);
	glVertex2f (0, vid.height);
	glEnd ();

	glColor4f (208.0/255.0, 180.0/255.0, 80.0/255.0, 0.035);
	for(c=0;c<40;c++)
	{
		bx = rand() % vid.width-20;
		by = rand() % vid.height-20;
		ex = bx + (rand() % 40) + 20;
		ey = by + (rand() % 40) + 20;
		if (bx < 0) bx = 0;
		if (by < 0) by = 0;
		if (ex > (int)vid.width) ex = (int)vid.width;
		if (ey > (int)vid.height) ey = (int)vid.height;

		glBegin (GL_QUADS);
		glVertex2f (bx,by);
		glVertex2f (ex, by);
		glVertex2f (ex, ey);
		glVertex2f (bx, ey);
		glEnd ();
	}

	glColor4f (1,1,1,1);
	glEnable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);

	glAlphaFunc(GL_GREATER, 0.666);

	Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	static int index = 0;

	if (!draw_disc[index] || loading_stage)
		return;

	index++;
	if (index >= MAX_DISC)
		index = 0;

//	glDrawBuffer  (GL_FRONT);

	Draw_Pic (vid.width - 28, 0, draw_disc[index]);

//	glDrawBuffer  (GL_BACK);
}

/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
//void GL_Set2D (void)
//{
//	//
//	// set up viewpoint
//	//
//	glViewport (glx, gly, glwidth, glheight);
//
//	glMatrixMode (GL_PROJECTION);
//	glLoadIdentity ();
//
//	glOrtho  (0, glwidth, glheight, 0, -99999, 99999);
//
//	glMatrixMode (GL_MODELVIEW);
//	glLoadIdentity ();
//
//	//
//	// set drawing parms
//	//
//	glDisable (GL_DEPTH_TEST);
//	glDisable (GL_CULL_FACE);
//	glDisable (GL_BLEND);
//	glEnable (GL_ALPHA_TEST);
//
//	glColor4f (1,1,1,1);
//}

//====================================================================


/*
================
GL_ResampleTextureQuality

bilinear resample
================
*/
//void GL_ResampleTextureQuality (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight, qboolean alpha)
//{
//	byte	 *nwpx, *nepx, *swpx, *sepx, *dest, *inlimit;
//	unsigned xfrac, yfrac, x, y, modx, mody, imodx, imody, injump, outjump;
//	int	 i, j;
//
//	// Sanity ...
//	if (inwidth <= 0 || inheight <= 0 || outwidth <= 0 || outheight <= 0 ||
//		inwidth * 0x10000 & 0xC0000000 || inheight * outheight & 0xC0000000 ||
//		inwidth * inheight & 0xC0000000)
//		Sys_Error ("GL_ResampleTextureQuality: invalid parameters (in:%dx%d, out:%dx%d)", inwidth, inheight, outwidth, outheight);
//
//	inlimit = (byte *)(in + inwidth * inheight);
//
//	// Make sure "out" size is at least 2x2!
//	xfrac = ((inwidth-1) << 16) / (outwidth-1);
//	yfrac = ((inheight-1) << 16) / (outheight-1);
//	y = outjump = 0;
//
//	// Better resampling, less blurring of all texes, requires a lot of memory
//	for (i=0; i<outheight; i++)
//	{
//		mody = (y>>8) & 0xFF;
//		imody = 256 - mody;
//		injump = (y>>16) * inwidth;
//		x = 0;
//
//		for (j=0; j<outwidth; j++)
//		{
//			modx = (x>>8) & 0xFF;
//			imodx = 256 - modx;
//
//			nwpx = (byte *)(in + (x>>16) + injump);
//			nepx = nwpx + sizeof(int);
//			swpx = nwpx + inwidth * sizeof(int); // Next line
//
//			// Don't exceed "in" size
//			if (swpx + sizeof(int) >= inlimit)
//			{
////				Con_Error ("GL_ResampleTextureQuality: %4d\n", swpx + sizeof(int) - inlimit);
//				swpx = nwpx; // There's no next line
//			}
//
//			sepx = swpx + sizeof(int);
//
//			dest = (byte *)(out + outjump + j);
//
//			dest[0] = (nwpx[0]*imodx*imody + nepx[0]*modx*imody + swpx[0]*imodx*mody + sepx[0]*modx*mody)>>16;
//			dest[1] = (nwpx[1]*imodx*imody + nepx[1]*modx*imody + swpx[1]*imodx*mody + sepx[1]*modx*mody)>>16;
//			dest[2] = (nwpx[2]*imodx*imody + nepx[2]*modx*imody + swpx[2]*imodx*mody + sepx[2]*modx*mody)>>16;
//			if (alpha)
//				dest[3] = (nwpx[3]*imodx*imody + nepx[3]*modx*imody + swpx[3]*imodx*mody + sepx[3]*modx*mody)>>16;
//			else
//				dest[3] = 255;
//
//			x += xfrac;
//		}
//		outjump += outwidth;
//		y += yfrac;
//	}
//}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
//void GL_MipMap (byte *in, int width, int height)
//{
//	int		i, j;
//	byte	*out;
//
//	width <<=2;
//	height >>= 1;
//	out = in;
//
//	for (i=0 ; i<height ; i++, in+=width)
//	{
//		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
//		{
//			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
//			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
//			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
//			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
//		}
//	}
//}

/*
===============
GL_AlphaEdgeFix

eliminate pink edges on sprites, etc.
operates in place on 32bit data
===============
*/
//void GL_AlphaEdgeFix (byte *data, int width, int height)
//{
//	int i,j,n=0,b,c[3]={0,0,0},lastrow,thisrow,nextrow,lastpix,thispix,nextpix;
//	byte *dest = data;
//
//	for (i=0; i<height; i++)
//	{
//		lastrow = width * 4 * ((i == 0) ? height-1 : i-1);
//		thisrow = width * 4 * i;
//		nextrow = width * 4 * ((i == height-1) ? 0 : i+1);
//
//		for (j=0; j<width; j++, dest+=4)
//		{
//			if (dest[3]) // not transparent
//				continue;
//
//			lastpix = 4 * ((j == 0) ? width-1 : j-1);
//			thispix = 4 * j;
//			nextpix = 4 * ((j == width-1) ? 0 : j+1);
//
//			b = lastrow + lastpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
//			b = thisrow + lastpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
//			b = nextrow + lastpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
//			b = lastrow + thispix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
//			b = nextrow + thispix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
//			b = lastrow + nextpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
//			b = thisrow + nextpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
//			b = nextrow + nextpix; if (data[b+3]) {c[0] += data[b]; c[1] += data[b+1]; c[2] += data[b+2]; n++;}
//
//			// average all non-transparent neighbors
//			if (n)
//			{
//				dest[0] = (byte)(c[0]/n);
//				dest[1] = (byte)(c[1]/n);
//				dest[2] = (byte)(c[2]/n);
//
//				n = c[0] = c[1] = c[2] = 0;
//			}
//		}
//	}
//}

/*
================
GL_ScaleSize
================
*/
//int GL_ScaleSize (int oldsize, qboolean force)
//{
//	int newsize, nextsize;
//
//	if (force)
//		nextsize = oldsize;
//	else
//		nextsize = 3 * oldsize / 2; // Avoid unfortunate resampling
//
//	for (newsize = 1; newsize < nextsize && newsize != oldsize; newsize <<= 1)
//		;
//
//	return newsize;
//}

/*
===============
GL_Upload32

handles 32bit source data
===============
*/
void GL_Upload32 (gltexture_t *glt, unsigned *data)
{
	int			internalformat;
	int			scaled_width, scaled_height;
	int			picmip;
	unsigned	*scaled = NULL;

	scaled_width = GL_ScaleSize (glt->width, false);
	scaled_height = GL_ScaleSize (glt->height, false);

	if (glt->width && glt->height)
	{
		// Preserve proportions
		while (glt->width > glt->height && scaled_width < scaled_height)
			scaled_width *= 2;

		while (glt->width < glt->height && scaled_width > scaled_height)
			scaled_height *= 2;
	}

	// Note: Can't use Con_Printf here!
	if (developer.value > 1 && (scaled_width != GL_ScaleSize (glt->width, true) || scaled_height != GL_ScaleSize (glt->height, true)))
		Con_DPrintf ("GL_Upload32: in:%dx%d, out:%dx%d, '%s'\n", glt->width, glt->height, scaled_width, scaled_height, glt->name);

	// Prevent too large or too small images (might otherwise crash resampling)
	scaled_width = CLAMP(2, scaled_width, gl_texture_max_size);
	scaled_height = CLAMP(2, scaled_height, gl_texture_max_size);

	// allocate dynamic memory
	scaled = malloc (scaled_width * scaled_height * 4); // was sizeof(unsigned)
//	scaled = Hunk_Alloc (scaled_width * scaled_height * 4);

	// Resample up
	if (glt->width && glt->height) // Don't resample 0-sized images
		GL_ResampleTextureQuality (data, glt->width, glt->height, scaled, scaled_width, scaled_height, (glt->flags & TEXPREF_ALPHA));
	else
		memcpy (scaled, data, scaled_width * scaled_height * rgba_bytes); // FIXME: 0-sized texture, so we just copy it

	// mipmap down
	picmip = (glt->flags & TEXPREF_NOPICMIP) ? 0 : max((int)gl_picmip.value, 0);
	if (glt->flags & TEXPREF_MIPMAP)
	{
		int i;

		// Only affect mipmapped texes, typically not console graphics
		for (i = 0; i < picmip && (scaled_width > 1 || scaled_height > 1); ++i)
		{
			GL_MipMap ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			scaled_width = max(scaled_width, 1);
			scaled_height = max(scaled_height, 1);

			if (glt->flags & TEXPREF_ALPHA)
				GL_AlphaEdgeFix ((byte *)scaled, scaled_width, scaled_height);
		}
	}

	// upload
	GL_Bind (glt);
	internalformat = (glt->flags & TEXPREF_ALPHA) ? gl_alpha_format : gl_solid_format;
	glTexImage2D (GL_TEXTURE_2D, 0, internalformat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

	// upload mipmaps
	if (glt->flags & TEXPREF_MIPMAP)
	{
		int miplevel = 0;

		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			scaled_width = max(scaled_width, 1);
			scaled_height = max(scaled_height, 1);

			miplevel++;
			glTexImage2D (GL_TEXTURE_2D, miplevel, internalformat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		}
	}

	// set filter modes
	GL_SetFilterModes (glt);

	// free allocated memory
	free (scaled);
}

/*
================
GL_UploadBloom

handles bloom data
================
*/
void GL_UploadBloom (gltexture_t *glt, unsigned *data)
{
	// upload it
	GL_Bind (glt);
	glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, glt->width, glt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	// set filter modes
	GL_SetFilterModes (glt);
}

/*
================
GL_UploadLightmap

handles lightmap data
================
*/
void GL_UploadLightmap (gltexture_t *glt, byte *data)
{
	// upload it
	GL_Bind (glt);
	glTexImage2D (GL_TEXTURE_2D, 0, gl_lightmap_format, glt->width, glt->height, 0, gl_lightmap_format, GL_UNSIGNED_BYTE, data);

	// set filter modes
	GL_SetFilterModes (glt);
}


/*
* mode:
* 0 - standard
* 1 - color 0 transparent, odd - translucent, even - full value
* 2 - color 0 transparent
* 3 - special (particle translucency table)
*/
/*
===============
GL_Upload8

handles 8bit source data, then passes it to GL_Upload32
===============
*/
void GL_Upload8 (gltexture_t *glt, byte *data)
{
	int			i, size;
	int			p;
	unsigned	*trans = NULL;

	size = glt->width * glt->height;
//	if (size & 3)
//		Con_Warning ("GL_Upload8: size %d is not a multiple of 4 in '%s'\n", size, glt->name);

	// allocate dynamic memory
	trans = malloc (size * 4); // was sizeof(unsigned)
//	trans = Hunk_Alloc (size * 4);

	// detect false alpha cases
	if (glt->flags & TEXPREF_ALPHA && !(glt->flags & TEXPREF_CONCHARS))
	{
		for (i=0 ; i<size ; i++)
		{
			p = data[i];
			if (p == 255) //transparent index
				break;
		}
		if (i == size)
//			glt->flags -= TEXPREF_ALPHA;
			glt->flags &= ~TEXPREF_ALPHA;
	}


	// set alpha if we have trans mode
	if (glt->flags & (TEXPREF_TRANSPARENT | TEXPREF_HOLEY | TEXPREF_SPECIAL_TRANS))
		glt->flags |= TEXPREF_ALPHA;


	// choose palette and convert to 32bit
	if (glt->flags & TEXPREF_FULLBRIGHT)
	{
		for (i=0 ; i<size ; ++i)
		{
			p = data[i];
			trans[i] = d_8to24table_fbright[p];
		}
	}
	else if (glt->flags & TEXPREF_NOBRIGHT)
	{
		for (i=0 ; i<size ; ++i)
		{
			p = data[i];
			trans[i] = d_8to24table_nobright[p];
		}
	}
	else if (glt->flags & TEXPREF_CONCHARS)
	{
		for (i=0 ; i<size ; ++i)
		{
			p = data[i];
			trans[i] = d_8to24table_conchars[p];
		}
	}
	else
	{
		for (i=0 ; i<size ; ++i)
		{
			p = data[i];
			trans[i] = d_8to24table[p];
		}
	}



//if (glt->flags & (TEXPREF_ALPHA | TEXPREF_TRANSPARENT | TEXPREF_HOLEY | TEXPREF_SPECIAL_TRANS))
//if (glt->flags & TEXPREF_ALPHA)
{
	for (i = 0; i < size; i++)
	{
		p = data[i];
		if (p == 255)
		{

			/* transparent, so scan around for another color
			 * to avoid alpha fringes */
			/* this is a replacement from Quake II for Raven's
			 * "neighboring colors" code */
			if (i > (int)glt->width && data[i-glt->width] != 255)
				p = data[i-glt->width];
			else if (i < size-(int)glt->width && data[i+glt->width] != 255)
				p = data[i+glt->width];
			else if (i > 0 && data[i-1] != 255)
				p = data[i-1];
			else if (i < size-1 && data[i+1] != 255)
				p = data[i+1];
			else
				p = 0;
			/* copy rgb components */
			((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
			((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
			((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
		} 
	}
}



	// transmode
	if (glt->flags & TEXPREF_TRANSPARENT)
	{
//		glt->flags |= TEXPREF_ALPHA;
		for (i=0 ; i<size ; i++)
		{
			p = data[i];
			if (p == 0)
				trans[i] &= 0x00ffffff; // transparent
			else if( p & 1 )
			{
				trans[i] &= 0x00ffffff; // translucent
				trans[i] |= ( ( int )( 255 * r_wateralpha.value ) ) << 24; //todo r_wateralpha -> r_modelalpha with clamping
			}
			else
			{
				trans[i] |= 0xff000000; // full value
			}
		}
	}
	else if (glt->flags & TEXPREF_HOLEY)
	{
//		glt->flags |= TEXPREF_ALPHA;
		for (i=0 ; i<size ; i++)
		{
			p = data[i];
			if (p == 0)
				trans[i] &= 0x00ffffff;
		}
	}
	else if (glt->flags & TEXPREF_SPECIAL_TRANS)
	{
//		glt->flags |= TEXPREF_ALPHA;
		for (i=0 ; i<size ; i++)
		{
			p = data[i];
			trans[i] = d_8to24table[ColorIndex[p>>4]] & 0x00ffffff;
			trans[i] |= ( int )ColorPercent[p&15] << 24;
		}
	}




	// fix edges
	if (glt->flags & TEXPREF_ALPHA)
		GL_AlphaEdgeFix ((byte *)trans, glt->width, glt->height);

	// upload it
	GL_Upload32 (glt, trans);

	// free allocated memory
	free (trans);
}


/*
================
GL_FindTexture
================
*/
gltexture_t *GL_FindTexture (model_t *owner, char *name)
{
	gltexture_t	*glt;

	if (name)
	{
		for (glt = active_gltextures; glt; glt = glt->next)
			if (glt->owner == owner && !strcmp (glt->name, name))
				return glt;
	}

	return NULL;
}


/*
================
GL_NewTexture
================
*/
gltexture_t *GL_NewTexture (void)
{
	gltexture_t *glt;

	if (numgltextures >= MAX_GLTEXTURES)
		Sys_Error ("GL_NewTexture: cache full, max is %i textures", MAX_GLTEXTURES);

	glt = free_gltextures;
	free_gltextures = glt->next;
	glt->next = active_gltextures;
	active_gltextures = glt;

	glGenTextures(1, &glt->texnum);
	numgltextures++;
	return glt;
}


/*
================
GL_FreeTexture
================
*/
void GL_FreeTexture (gltexture_t *free)
{
	gltexture_t *glt;

	if (free == NULL)
	{
		Con_SafePrintf ("GL_FreeTexture: NULL texture\n");
		return;
	}

	if (active_gltextures == free)
	{
		active_gltextures = free->next;
		free->next = free_gltextures;
		free_gltextures = free;

		glDeleteTextures(1, &free->texnum);
		numgltextures--;
		return;
	}

	for (glt = active_gltextures; glt; glt = glt->next)
	{
		if (glt->next == free)
		{
			glt->next = free->next;
			free->next = free_gltextures;
			free_gltextures = free;

			glDeleteTextures(1, &free->texnum);
			numgltextures--;
			return;
		}
	}

	Con_SafePrintf ("GL_FreeTexture: not found\n");
}

/*
================
GL_FreeTextures
================
*/
void GL_FreeTextures (model_t *owner)
{
	gltexture_t *glt, *next;

	for (glt = active_gltextures; glt; glt = next)
	{
		next = glt->next;
		if (glt && glt->owner == owner)
			GL_FreeTexture (glt);
	}
}

/*
================
GL_LoadTexture

the one entry point for loading all textures
================
*/
gltexture_t *GL_LoadTexture (model_t *owner, char *name, int width, int height, enum srcformat format, byte *data, char *source_file, unsigned source_offset, unsigned flags)
{
	int		size = 0; // keep compiler happy
	gltexture_t	*glt;
	unsigned short crc;
//	int mark;

	if (cls.state == ca_dedicated)
		return NULL; // No textures in dedicated mode

	// check format size
	size = width * height;
	switch (format)
	{
		case SRC_INDEXED:
			size *= indexed_bytes;
			break;
		case SRC_LIGHTMAP:
			size *= lightmap_bytes;
			break;
		case SRC_RGBA:
			size *= rgba_bytes;
			break;
		case SRC_BLOOM:
			size *= rgba_bytes;
			break;
	}

	if (size == 0)
		Con_DPrintf ("GL_LoadTexture: texture '%s' has size 0\n", name);

	// Sanity check, max = 32kx32k
	if (width < 0 || height < 0 || size > 0x40000000)
		Sys_Error ("GL_LoadTexture: texture '%s' has invalid size (%dM, max = %dM)", name, size / (1024 * 1024), 0x40000000 / (1024 * 1024));

	// cache check
	crc = CRC_Block(data, size);

	if ((flags ^ TEXPREF_OVERWRITE) && (glt = GL_FindTexture (owner, name)))
//	if ((flags & TEXPREF_OVERWRITE) && (glt = GL_FindTexture (owner, name))) 
	{
		if (glt->source_crc == crc)
			return glt;
	}
	else
		glt = GL_NewTexture ();

	// copy data
	glt->owner = owner;
	strncpy (glt->name, name, sizeof(glt->name));
	glt->width = width;
	glt->height = height;
	glt->flags = flags;
	strncpy (glt->source_file, source_file, sizeof(glt->source_file));
	glt->source_offset = source_offset;
	glt->source_format = format;
	glt->source_width = width;
	glt->source_height = height;
	glt->source_crc = crc;

	//upload it
//	mark = Hunk_LowMark();

	switch (glt->source_format)
	{
		case SRC_INDEXED:
			GL_Upload8 (glt, data);
			break;
		case SRC_LIGHTMAP:
			GL_UploadLightmap (glt, data);
			break;
		case SRC_RGBA:
			GL_Upload32 (glt, (unsigned *)data);
			break;
		case SRC_BLOOM:
			GL_UploadBloom (glt, (unsigned *)data);
			break;
	}

//	Hunk_FreeToLowMark(mark);

	return glt;
}


/*
================
GL_ReloadTexture

reloads a texture
================
*/
void GL_ReloadTexture (gltexture_t *glt)
{
	byte	*data = NULL;
//	int		mark;
//
// get source data
//
//	mark = Hunk_LowMark ();

	if (glt->source_file[0] && glt->source_offset)
	{
		// lump inside file
		FILE *f;
		int size;

		COM_FOpenFile(glt->source_file, &f, NULL);
		if (f)
		{
			fseek (f, glt->source_offset, SEEK_CUR);
	
			// check format size
			size = (glt->source_width * glt->source_height);
			switch (glt->source_format)
			{
				case SRC_INDEXED:
					size *= indexed_bytes;
					break;
				case SRC_LIGHTMAP:
					size *= lightmap_bytes;
					break;
				case SRC_RGBA:
					size *= rgba_bytes;
					break;
				case SRC_BLOOM:
					size *= rgba_bytes;
					break;
			}
	
			data = Hunk_Alloc (size);
			fread (data, 1, size, f);
			fclose (f);
		}
	}
	else if (glt->source_file[0] && !glt->source_offset)
	{
//		data = GL_LoadImage (glt->source_file, (int *)&glt->source_width, (int *)&glt->source_height); // simple file
		data =NULL;
	}
	else if (!glt->source_file[0] && glt->source_offset)
		data = (byte *)glt->source_offset; // image in memory

	if (!data)
	{
		Con_Printf ("GL_ReloadTexture: invalid source for %s\n", glt->name);
//		Hunk_FreeToLowMark(mark);
		return;
	}

	glt->width = glt->source_width;
	glt->height = glt->source_height;
//
// upload it
//
	switch (glt->source_format)
	{
		case SRC_INDEXED:
			GL_Upload8 (glt, data);
			break;
		case SRC_LIGHTMAP:
			GL_UploadLightmap (glt, data);
			break;
		case SRC_RGBA:
			GL_Upload32 (glt, (unsigned *)data);
			break;
		case SRC_BLOOM:
			GL_UploadBloom (glt, (unsigned *)data);
			break;
	}

//	Hunk_FreeToLowMark(mark);
}

/*
================
GL_ReloadTextures_f

reloads all texture images.
================
*/
//void GL_ReloadTextures_f (void)
//{
//	gltexture_t *glt;
//
//	for (glt=active_gltextures; glt; glt=glt->next)
//	{
//		glGenTextures(1, &glt->texnum);
//		GL_ReloadTexture (glt);
//	}
//}



void D_ShowLoadingSize (void)
{
#ifdef RELEASE
	int cur_perc;
	static int prev_perc; 
	
	if (cls.state == ca_dedicated)
		return;				// stdout only

	//HoT speedup loading progress
	cur_perc = loading_stage * 100;
	if (total_loading_size)
		cur_perc += current_loading_size * 100 / total_loading_size;
	if (cur_perc == prev_perc)
		return;
	prev_perc = cur_perc; 

	glDrawBuffer  (GL_FRONT);

	SCR_DrawLoading();

	glFlush(); //EER1 this is cause very slow map loading, but show loading plaque (updating server/client progress) correctly
//	glFinish();

	glDrawBuffer  (GL_BACK);
#endif
}



/*
============================================================================================================

		MATRIX OPS by MH

	These happen in pace on the matrix and update it's current values

	These are D3D style matrix functions; sorry OpenGL-lovers but they're more sensible, usable
	and intuitive this way...

============================================================================================================
*/
// Baker change (RMQ Engine)
//glmatrix_t *GL_LoadMatrix (glmatrix_t *dst, glmatrix_t *src)
//{
//	memcpy (dst, src, sizeof (glmatrix_t));
//
//	return dst;
//}
//
//glmatrix_t *GL_IdentityMatrix (glmatrix_t *m)
//{
//	m->m16[0] = m->m16[5] = m->m16[10] = m->m16[15] = 1;
//	m->m16[1] = m->m16[2] = m->m16[3] = m->m16[4] = m->m16[6] = m->m16[7] = m->m16[8] = m->m16[9] = m->m16[11] = m->m16[12] = m->m16[13] = m->m16[14] = 0;
//
//	return m;
//}
//
//glmatrix_t *GL_MultiplyMatrix (glmatrix_t *out, glmatrix_t *m1, glmatrix_t *m2)
//{
//	int i, j;
//	glmatrix_t tmp;
//
//	// do it this way because either of m1 or m2 might be the same as out...
//	for (i = 0; i < 4; i++)
//	{
//		for (j = 0; j < 4; j++)
//		{
//			tmp.m4x4[i][j] = m1->m4x4[i][0] * m2->m4x4[0][j] +
//							 m1->m4x4[i][1] * m2->m4x4[1][j] +
//							 m1->m4x4[i][2] * m2->m4x4[2][j] +
//							 m1->m4x4[i][3] * m2->m4x4[3][j];
//		}
//	}
//
//	memcpy (out, &tmp, sizeof (glmatrix_t));
//
//	return out;
//}
//
//glmatrix_t *GL_TranslateMatrix (glmatrix_t *m, float x, float y, float z)
//{
//	glmatrix_t tmp;
//	GL_IdentityMatrix (&tmp);
//
//	tmp.m16[12] = x;
//	tmp.m16[13] = y;
//	tmp.m16[14] = z;
//
//	GL_MultiplyMatrix (m, &tmp, m);
//
//	return m;
//}
//
//glmatrix_t *GL_ScaleMatrix (glmatrix_t *m, float x, float y, float z)
//{
//	glmatrix_t tmp;
//	GL_IdentityMatrix (&tmp);
//
//	tmp.m16[0] = x;
//	tmp.m16[5] = y;
//	tmp.m16[10] = z;
//
//	GL_MultiplyMatrix (m, &tmp, m);
//
//	return m;
//}
//
//glmatrix_t *GL_RotateMatrix (glmatrix_t *m, float a, float x, float y, float z)
//{
//	// i prefer spaces around my operators because it makes stuff like a = b * -c clearer and easier on the eye. ;)
//	glmatrix_t tmp;
//	float c = cos (a * M_PI / 180.0);
//	float s = sin (a * M_PI / 180.0);
//
//	// http://www.opengl.org/sdk/docs/man/xhtml/glRotate.xml
//	// this should normalize the vector before rotating
//	VectorNormalize3f (&x, &y, &z);
//
//	tmp.m16[0] = x * x * (1 - c) + c;
//	tmp.m16[4] = x * y * (1 - c) - z * s;
//	tmp.m16[8] = x * z * (1 - c) + y * s;
//	tmp.m16[12] = 0;
//
//	tmp.m16[1] = y * x * (1 - c) + z * s;
//	tmp.m16[5] = y * y * (1 - c) + c;
//	tmp.m16[9] = y * z * (1 - c) - x * s;
//	tmp.m16[13] = 0;
//
//	tmp.m16[2] = x * z * (1 - c) - y * s;
//	tmp.m16[6] = y * z * (1 - c) + x * s;
//	tmp.m16[10] = z * z * (1 - c) + c;
//	tmp.m16[14] = 0;
//
//	tmp.m16[3] = 0;
//	tmp.m16[7] = 0;
//	tmp.m16[11] = 0;
//	tmp.m16[15] = 1;
//
//	GL_MultiplyMatrix (m, &tmp, m);
//
//	return m;
//}

