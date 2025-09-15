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


#define MAX_DISC 18

cvar_t		scr_conalpha = {"scr_conalpha", "1", CVAR_ARCHIVE};

byte		*draw_chars;				// 8*8 graphic characters
byte		*draw_smallchars;			// Small characters for status bar
qpic_t		*draw_menufont; 			// Big Menu Font
qpic_t		*draw_disc[MAX_DISC] = { NULL }; // make the first one null for sure
qpic_t		*draw_backtile;

//gltexture_t			*translate_texture[MAX_PLAYER_CLASS];
gltexture_t			*char_texture;
gltexture_t			*char_smalltexture;
gltexture_t			*char_menufonttexture;

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar objects into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

#define MAX_SCRAPS		1
#define SCRAP_WIDTH		256
#define SCRAP_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][SCRAP_WIDTH];
byte		scrap_texels[MAX_SCRAPS][SCRAP_WIDTH*SCRAP_HEIGHT];
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
		best = SCRAP_HEIGHT;

		for (i=0 ; i<SCRAP_WIDTH-w ; i++)
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

		if (best + h > SCRAP_HEIGHT)
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

	for (i = 0; i < MAX_SCRAPS; i++) 
	{
		sprintf (name, "scrap%i", i);
		scrap_textures[i] = TexMgr_LoadTexture (NULL, name, SCRAP_WIDTH, SCRAP_HEIGHT, SRC_INDEXED, scrap_texels[i], "", (uintptr_t)scrap_texels[i], TEXPREF_ALPHA | TEXPREF_OVERWRITE | TEXPREF_NOPICMIP);
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
//#define PLAYER_PIC_WIDTH 68
//#define PLAYER_PIC_HEIGHT 114
//#define PLAYER_DEST_WIDTH 128
//#define PLAYER_DEST_HEIGHT 128

//byte		menuplyr_pixels[MAX_PLAYER_CLASS][PLAYER_PIC_WIDTH*PLAYER_PIC_HEIGHT];


qpic_t *Draw_PicFromWad (char *name)
{
	qpic_t	*p;
	glpic_t *glp;
	uintptr_t offset; //johnfitz
	char texturename[64]; //johnfitz

	p = W_GetLumpName (name);

	// Sanity ...
	if (p->width & 0xC0000000 || p->height & 0xC0000000)
		Sys_Error ("Draw_PicFromWad: invalid dimensions (%dx%d) for '%s'", p->width, p->height, name);

	glp = (glpic_t *)p->data;

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
				scrap_texels[texnum][(y+i)*SCRAP_WIDTH + x + j] = p->data[k];

		glp->gltexture = scrap_textures[texnum]; // changed to an array
		// no longer go from 0.01 to 0.99
		glp->sl = x/(float)SCRAP_WIDTH;
		glp->sh = (x+p->width)/(float)SCRAP_WIDTH;
		glp->tl = y/(float)SCRAP_HEIGHT;
		glp->th = (y+p->height)/(float)SCRAP_HEIGHT;
	}
	else
	{
nonscrap:
		sprintf (texturename, "%s:%s", WADFILE, name); //johnfitz
		offset = (uintptr_t)p - (uintptr_t)wad_base + sizeof(int)*2; //johnfitz
		glp->gltexture = TexMgr_LoadTexture (NULL, texturename, p->width, p->height, SRC_INDEXED, p->data, WADFILE, offset, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
		glp->sl = 0;
		glp->sh = (float)p->width/(float)TexMgr_PadConditional(p->width); //johnfitz
		glp->tl = 0;
		glp->th = (float)p->height/(float)TexMgr_PadConditional(p->height); //johnfitz
	}

	return p;
}


qpic_t *Draw_PicFromFile (char *name, void *buf)
{
	qpic_t	*p;
	glpic_t *glp;

	//johnfitz -- dynamic gamedir loading
	//johnfitz -- modified to use zone alloc
	p = (qpic_t *)COM_LoadZoneFile (name, buf, NULL);

	if (!p)
		return NULL;
	SwapPic (p);

	glp = (glpic_t *)p->data;
	glp->gltexture = TexMgr_LoadTexture (NULL, name, p->width, p->height, SRC_INDEXED, p->data, name, sizeof(int)*2, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	glp->sl = 0;
	glp->sh = (float)p->width/(float)TexMgr_PadConditional(p->width); //johnfitz
	glp->tl = 0;
	glp->th = (float)p->height/(float)TexMgr_PadConditional(p->height); //johnfitz
	
	return p;
}

/*
================
Draw_CachePic
================
*/
qpic_t	*Draw_CachePic (char *path)
{
	cachepic_t	*cpic;
	int			i;
	qpic_t		*p;
	glpic_t 	*glp;

	for (cpic=menu_cachepics, i=0 ; i<menu_numcachepics ; cpic++, i++)
		if (!strcmp (path, cpic->name))
			return &cpic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("Draw_CachePic: menu_numcachepics == MAX_CACHED_PICS (%d)", MAX_CACHED_PICS);
	menu_numcachepics++;
	strcpy (cpic->name, path);

//
// load the pic from disk
//
	p = (qpic_t *)COM_LoadTempFile (path, NULL);
	if (!p)
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	SwapPic (p);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	/* garymct */
//	if (!strcmp (path, "gfx/menu/netp1.lmp"))
//		memcpy (menuplyr_pixels[0], p->data, p->width*p->height);
//	else if (!strcmp (path, "gfx/menu/netp2.lmp"))
//		memcpy (menuplyr_pixels[1], p->data, p->width*p->height);
//	else if (!strcmp (path, "gfx/menu/netp3.lmp"))
//		memcpy (menuplyr_pixels[2], p->data, p->width*p->height);
//	else if (!strcmp (path, "gfx/menu/netp4.lmp"))
//		memcpy (menuplyr_pixels[3], p->data, p->width*p->height);
//	else if (!strcmp (path, "gfx/menu/netp5.lmp"))
//		memcpy (menuplyr_pixels[4], p->data, p->width*p->height);

	cpic->pic.width = p->width;
	cpic->pic.height = p->height;

	glp = (glpic_t *)cpic->pic.data;
	glp->gltexture = TexMgr_LoadTexture (NULL, path, p->width, p->height, SRC_INDEXED, p->data, path, sizeof(int)*2, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	glp->sl = 0;
	glp->sh = (float)p->width/(float)TexMgr_PadConditional(p->width); //johnfitz
	glp->tl = 0;
	glp->th = (float)p->height/(float)TexMgr_PadConditional(p->height); //johnfitz

	return &cpic->pic;
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
	cachepic_t      *cpic;
	int                     i;
	qpic_t          *p;
	glpic_t         *glp;

	for (cpic=menu_cachepics, i=0 ; i<menu_numcachepics ; cpic++, i++)
		if (!strcmp (path, cpic->name))
			return &cpic->pic;

	if (menu_numcachepics == MAX_CACHED_PICS)
		Sys_Error ("menu_numcachepics == MAX_CACHED_PICS (%d)", MAX_CACHED_PICS);
	menu_numcachepics++;
	strcpy (cpic->name, path);

//
// load the pic from disk
//
	p = (qpic_t *)COM_LoadTempFile (path, NULL);
	if (!p)
		Sys_Error ("Draw_CachePicNoTrans: failed to load %s", path);
	SwapPic (p);

	cpic->pic.width = p->width;
	cpic->pic.height = p->height;

	// Get rid of transparencies
	for (i = 0; i < p->width * p->height; i++)
	{
		if (p->data[i] == 255)
			p->data[i] = 31; // pal(31) == pal(255) == FCFCFC (white)
	}

	glp = (glpic_t *)cpic->pic.data;
	glp->gltexture = TexMgr_LoadTexture (NULL, path, p->width, p->height, SRC_INDEXED, p->data, path, sizeof(int)*2, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
	glp->sl = 0;
	glp->sh = 1;
	glp->tl = 0;
	glp->th = 1;

	return &cpic->pic;
}



/*
===============
Draw_LoadPics
===============
*/
void Draw_LoadPics (void)
{
	int		i;
	char texturepath[MAX_QPATH];
	uintptr_t	offset; // johnfitz
	char		texturename[64]; //johnfitz

	//
	// load console charset
	//
	//draw_chars = W_GetLumpName ("conchars");
	sprintf (texturepath, "%s", "gfx/menu/conchars.lmp"); // EER1
	draw_chars = COM_LoadZoneFile (texturepath, draw_chars, NULL);

	if (!draw_chars)
		Sys_Error ("Draw_LoadPics: couldn't load conchars");

	// now turn them into textures
	offset = (uintptr_t)0; // was sizeof(int)*2, because "gfx/menu/conchars.lmp" it's just a data, so we don't need an offset
	char_texture = TexMgr_LoadTexture (NULL, texturepath, 256, 128, SRC_INDEXED, draw_chars, texturepath, offset, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);


	//
	// load sbar small font
	//
	draw_smallchars = W_GetLumpName("tinyfont");

	if (!draw_smallchars)
		Sys_Error ("Draw_LoadPics: couldn't load tinyfont");

	// now turn them into textures
	sprintf (texturename, "%s:%s", WADFILE, "smallcharset"); // EER1
	offset = (uintptr_t)draw_smallchars - (uintptr_t)wad_base;
	char_smalltexture = TexMgr_LoadTexture (NULL, texturename, 128, 32, SRC_INDEXED, draw_smallchars, WADFILE, offset, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);


	//
	// load menu big font
	//
	sprintf (texturepath, "%s", "gfx/menu/bigfont2.lmp"); // EER1 "menufont"
	draw_menufont = (qpic_t *)COM_LoadZoneFile (texturepath, draw_menufont, NULL);
	if (!draw_menufont)
	{	// old version of demo has bigfont.lmp, not bigfont2.lmp
		sprintf (texturepath, "%s", "gfx/menu/bigfont.lmp"); // EER1 "menufont"
		draw_menufont = (qpic_t *)COM_LoadZoneFile (texturepath, draw_menufont, NULL);
	}

	if (!draw_menufont)
		Sys_Error ("Draw_LoadPics: couldn't load menufont");
	SwapPic (draw_menufont);

	// now turn them into textures
	offset = (uintptr_t)sizeof(int)*2; // offset to data in qpic_t
	// w*h size MUST be 160*80
	char_menufonttexture = TexMgr_LoadTexture (NULL, texturepath, draw_menufont->width, draw_menufont->height, SRC_INDEXED, draw_menufont->data, texturepath, offset, TEXPREF_ALPHA | TEXPREF_NEAREST | TEXPREF_NOPICMIP | TEXPREF_CONCHARS);

	//
	// get the other pics we need
	//
//	draw_disc = Draw_PicFromWad ("disc");
	block_drawing = true;
	for(i=MAX_DISC-1 ; i>=0 ; i--)
	{	// Do this backwards so we don't try and draw the skull as we are loading
		sprintf(texturepath, "gfx/menu/skull%d.lmp", i);
		draw_disc[i] = Draw_PicFromFile (texturepath, draw_disc[i]);
	}
	block_drawing = false;

//	draw_backtile = Draw_PicFromWad ("backtile");
	draw_backtile = Draw_PicFromFile ("gfx/menu/backtile.lmp", draw_backtile);

}


/*
===============
Draw_NewGame -- johnfitz
===============
*/
void Draw_NewGame (void)
{
	cachepic_t	*cpic;
	int			i;

	// empty scrap and reallocate gltextures
	memset(&scrap_allocated, 0, sizeof(scrap_allocated));
	memset(&scrap_texels, 255, sizeof(scrap_texels));
	Scrap_Upload (); // creates 2 empty gltextures

	// reload wad pics
	W_LoadWadFile (); //johnfitz -- filename is now hard-coded for honesty
	Draw_LoadPics ();
	SCR_LoadPics ();
	Sbar_LoadPics ();

	// empty lmp cache
	for (cpic=menu_cachepics, i=0 ; i<menu_numcachepics ; cpic++, i++)
		cpic->name[0] = 0;
	menu_numcachepics = 0;
}


/*
===============
Draw_Init

rewritten
===============
*/
void Draw_Init (void)
{
	Cvar_RegisterVariable (&scr_conalpha);

	// clear scrap and allocate gltextures
	memset(&scrap_allocated, 0, sizeof(scrap_allocated));
	memset(&scrap_texels, 255, sizeof(scrap_texels));
	Scrap_Upload (); // creates 2 empty textures

	// load pics
	Draw_LoadPics ();
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

	Draw_Character (scr_vrect.x + scr_vrect.width/2 + cl_crossx.value, scr_vrect.y + scr_vrect.height/2 + cl_crossy.value, '+');
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

	GL_BindTexture (char_texture);
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

	GL_BindTexture (char_texture);
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
	glpic_t			*glp;

	if (scrap_dirty)
		Scrap_Upload ();

	glp = (glpic_t *)pic->data;

	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glColor4f (1,1,1,alpha);
	GL_BindTexture (glp->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (glp->sl, glp->tl);
	glVertex2f (x, y);
	glTexCoord2f (glp->sh, glp->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (glp->sh, glp->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (glp->sl, glp->th);
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

	GL_BindTexture (char_smalltexture);
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

	GL_BindTexture (char_smalltexture);
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

	GL_BindTexture (char_menufonttexture);
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
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, qpic_t *pic)
{
	glpic_t 		*glp;

	if (scrap_dirty)
		Scrap_Upload ();

	glp = (glpic_t *)pic->data;

	glDisable (GL_ALPHA_TEST); //FX new
	glEnable (GL_BLEND); //FX
	glColor4f (1,1,1,1);
	GL_BindTexture (glp->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (glp->sl, glp->tl);
	glVertex2f (x, y);
	glTexCoord2f (glp->sh, glp->tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (glp->sh, glp->th);
	glVertex2f (x+pic->width, y+pic->height);
	glTexCoord2f (glp->sl, glp->th);
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
	glpic_t                 *glp;
	
	glp = (glpic_t *)pic->data;

	glDisable (GL_ALPHA_TEST); //FX new
	glEnable (GL_BLEND); //FX

	glColor4f (1,1,1,1);
	GL_BindTexture (glp->gltexture);
	
	
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
	glpic_t 		*glp;
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

	glp = (glpic_t *)pic->data;

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
		tl = glp->tl;
		th = glp->th;//(height-0.01)/pic->height;
	}

	glDisable (GL_ALPHA_TEST); //FX new
	glEnable (GL_BLEND); //FX

	glColor4f (1,1,1,1);
	GL_BindTexture (glp->gltexture);
	glBegin (GL_QUADS);
	glTexCoord2f (glp->sl, tl);
	glVertex2f (x, y);
	glTexCoord2f (glp->sh, tl);
	glVertex2f (x+pic->width, y);
	glTexCoord2f (glp->sh, th);
	glVertex2f (x+pic->width, y+height);
	glTexCoord2f (glp->sl, th);
	glVertex2f (x, y+height);
	glEnd ();

	glEnable (GL_ALPHA_TEST); //FX new
	glDisable (GL_BLEND); //FX
}

void Draw_TransPicCropped(int x, int y, qpic_t *pic)
{
	Draw_PicCropped (x, y, pic);
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

/*
=============
Draw_TransPicTranslate

-- johnfitz -- rewritten to use texmgr to do translation
Only used for the player color selection menu
=============
*/
//void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom, int playerclass)
{
	static int oldtop = -2;
	static int oldbottom = -2;
	static int oldplayerclass = -2;
	
	if (top != oldtop || bottom != oldbottom || playerclass != oldplayerclass)
	{
		glpic_t *glp = (glpic_t *)pic->data;
		gltexture_t *glt = glp->gltexture;
		oldtop = top;
		oldbottom = bottom;
		oldplayerclass = playerclass;
		TexMgr_ReloadTextureTranslation (glt, top, bottom, playerclass);
	}
	Draw_Pic (x, y, pic);

//	byte		trans[PLAYER_DEST_WIDTH * PLAYER_DEST_HEIGHT];
//	byte			*src, *dst;
//	byte	*data;
//	char	name[64];
//	int i, j;
//
//
//	data = menuplyr_pixels[setup_class-1];
//	sprintf (name, "gfx/menu/netp%d.lmp", setup_class);
//
//	dst = trans;
//	src = data;
//
//	for( i = 0; i < PLAYER_PIC_WIDTH; i++ )
//	{
//		for( j = 0; j < PLAYER_PIC_HEIGHT; j++ )
//		{
//			dst[j * PLAYER_DEST_WIDTH + i] = translation[src[j * PLAYER_PIC_WIDTH + i]];
//		}
//	}
//
//	data = trans;
//
//
//	translate_texture[setup_class-1] = TexMgr_LoadTexture (NULL, name, PLAYER_DEST_WIDTH, PLAYER_DEST_HEIGHT, SRC_INDEXED, data, "", (uintptr_t)data, TEXPREF_ALPHA | TEXPREF_PAD | TEXPREF_NOPICMIP);
//
//
//	glColor3f (1,1,1);
//	glBegin (GL_QUADS);
//	glTexCoord2f (0, 0);
//	glVertex2f (x, y);
//	glTexCoord2f (( float )PLAYER_PIC_WIDTH / PLAYER_DEST_WIDTH, 0);
//	glVertex2f (x+pic->width, y);
//	glTexCoord2f (( float )PLAYER_PIC_WIDTH / PLAYER_DEST_WIDTH, ( float )PLAYER_PIC_HEIGHT / PLAYER_DEST_HEIGHT);
//	glVertex2f (x+pic->width, y+pic->height);
//	glTexCoord2f (0, ( float )PLAYER_PIC_HEIGHT / PLAYER_DEST_HEIGHT);
//	glVertex2f (x, y+pic->height);
//	glEnd ();
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

	alpha = (con_forcedup) ? 1.0 : CLAMP(0.0, scr_conalpha.value, 1.0);
//	alpha = 1.0;

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
	glpic_t	*glp;

	glp = (glpic_t *)draw_backtile->data;

	glDisable (GL_ALPHA_TEST); //FX new
	glEnable (GL_BLEND); //FX
	glColor4f (1,1,1,1); //FX new
	GL_BindTexture (glp->gltexture);
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

	if (!draw_disc[index] || loading_stage || block_drawing || isIntel) // intel video workaround
		return;

	index++;
	if (index >= MAX_DISC)
		index = 0;

	Draw_Pic (vid.width - 28, 0, draw_disc[index]);
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


//====================================================================


void D_ShowLoadingSize (void)
{
	int cur_perc;
	static int prev_perc;
	
	if (!scr_showloading.value)
		return;
	
	if (cls.state == ca_dedicated)
		return;				// stdout only
	
	//HoT speedup loading progress
	cur_perc = loading_stage * 100;
	if (total_loading_size)
		cur_perc += current_loading_size * 100 / total_loading_size;
	if (cur_perc == prev_perc)
		return;
	prev_perc = cur_perc;
	
	glDrawBuffer (GL_FRONT);
	
	SCR_DrawLoading();
	
	glFlush (); //EER1 this is cause very slow map loading, but show loading plaque (updating server/client progress) correctly
	
	glDrawBuffer (GL_BACK);
}

