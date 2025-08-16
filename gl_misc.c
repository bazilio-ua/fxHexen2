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
// gl_misc.c

#include "quakedef.h"

texture_t	*notexture_mip; // moved here from gl_main.c 
texture_t	*notexture_mip2; // used for non-lightmapped surfs with a missing texture

gltexture_t *particletexture;	// particle texture (little dot for particles)
gltexture_t *particletexture1;	// circle
gltexture_t *particletexture2;	// square

byte *playerTranslation;

/*
==================
R_InitTextures
==================
*/
void	R_InitTextures (void)
{
	// create a simple checkerboard texture for the default (notexture miptex)
	notexture_mip = Hunk_AllocName (sizeof(texture_t), "notexture_mip");
	strcpy (notexture_mip->name, "notexture");
	notexture_mip->height = notexture_mip->width = 16;

	notexture_mip2 = Hunk_AllocName (sizeof(texture_t), "notexture_mip2");
	strcpy (notexture_mip2->name, "notexture2");
	notexture_mip2->height = notexture_mip2->width = 16;
}



/*
====================
R_FullBright
====================
*/
void R_FullBright (void)
{
	// Refresh lightmaps
	R_BuildLightmaps ();
}

/*
====================
R_Ambient
====================
*/
void R_Ambient (void)
{
	// Refresh lightmaps
//	R_RebuildAllLightmaps ();
	R_BuildLightmaps ();
}

/*
====================
R_ClearColor
====================
*/
void R_ClearColor (void)
{
//	byte *rgb;

	// Refresh clearcolor
//	rgb = (byte *)(d_8to24table + ((int)r_clearcolor.value & 0xFF));
//	glClearColor (rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0, 0);
}

float globalwateralpha = 0.0;

/*
===============
R_Init
===============
*/
void R_Init (void)
{	
//	extern byte *hunk_base;
//	int counter;

	Cmd_AddCommand ("timerefresh", R_TimeRefresh_f);	
	Cmd_AddCommand ("pointfile", R_ReadPointFile_f);	

	Cmd_AddCommand ("sky", R_Sky_f);
	Cmd_AddCommand ("loadsky", R_Sky_f); // Nehahra
	Cmd_AddCommand ("fog", R_Fog_f);

	Cvar_RegisterVariable (&r_norefresh);
	Cvar_RegisterVariableCallback (&r_fullbright, R_FullBright);
	Cvar_RegisterVariableCallback (&r_ambient, R_Ambient);
	Cvar_RegisterVariable (&r_drawentities);
	Cvar_RegisterVariable (&r_drawworld);
	Cvar_RegisterVariable (&r_drawviewmodel);
	Cvar_RegisterVariable (&r_waterquality);
	Cvar_RegisterVariable (&r_wateralpha);
	Cvar_RegisterVariable (&r_lockalpha);
//	Cvar_RegisterVariable (&r_lavafog, NULL);
//	Cvar_RegisterVariable (&r_slimefog, NULL);
	Cvar_RegisterVariable (&r_lavaalpha);
	Cvar_RegisterVariable (&r_slimealpha);
	Cvar_RegisterVariable (&r_telealpha);
	Cvar_RegisterVariable (&r_dynamic);
	Cvar_RegisterVariable (&r_novis);
	Cvar_RegisterVariable (&r_lockfrustum);
	Cvar_RegisterVariable (&r_lockpvs);
	Cvar_RegisterVariable (&r_speeds);
	Cvar_RegisterVariable (&r_waterwarp);
	Cvar_RegisterVariableCallback (&r_clearcolor, R_ClearColor);
	Cvar_RegisterVariable (&r_fastsky);
	Cvar_RegisterVariable (&r_skyquality);
	Cvar_RegisterVariable (&r_skyalpha);
	Cvar_RegisterVariable (&r_skyfog);
	Cvar_RegisterVariable (&r_oldsky);

	Cvar_RegisterVariable (&gl_finish);
	Cvar_RegisterVariable (&gl_clear);

	Cvar_RegisterVariable (&gl_cull);
	Cvar_RegisterVariable (&gl_farclip);
	Cvar_RegisterVariable (&gl_smoothmodels);
	Cvar_RegisterVariable (&gl_affinemodels);
	Cvar_RegisterVariable (&gl_polyblend);
	Cvar_RegisterVariable (&gl_flashblend);
//	Cvar_RegisterVariable (&gl_playermip, NULL);
	Cvar_RegisterVariable (&gl_nocolors);

//	Cvar_RegisterVariable (&gl_keeptjunctions, NULL);
//	Cvar_RegisterVariable (&gl_reporttjunctions, NULL);
//
//	Cvar_RegisterVariable (&gl_zfix, NULL); // z-fighting fix

	// Nehahra
	Cvar_RegisterVariable (&gl_fogenable);
	Cvar_RegisterVariable (&gl_fogdensity);
	Cvar_RegisterVariable (&gl_fogred);
	Cvar_RegisterVariable (&gl_foggreen);
	Cvar_RegisterVariable (&gl_fogblue);

	Cvar_RegisterVariable (&gl_bloom);
	Cvar_RegisterVariable (&gl_bloomdarken);
	Cvar_RegisterVariable (&gl_bloomalpha);
	Cvar_RegisterVariable (&gl_bloomintensity);
	Cvar_RegisterVariable (&gl_bloomdiamondsize);
	Cvar_RegisterVariableCallback (&gl_bloomsamplesize, R_InitBloomTextures);
	Cvar_RegisterVariable (&gl_bloomfastsample);


	R_InitParticles ();

//	R_InitTranslatePlayerTextures ();


	playerTranslation = (byte *)COM_LoadHunkFile ("gfx/player.lmp", NULL);
	if (!playerTranslation)
		Sys_Error ("Couldn't load gfx/player.lmp");


//	R_InitMapGlobals ();

	R_InitBloomTextures();
}


/*
===============
R_InitTranslatePlayerTextures
===============
*/
static int oldtop[MAX_SCOREBOARD]; 
static int oldbottom[MAX_SCOREBOARD];
static int oldskinnum[MAX_SCOREBOARD];
static int oldplayerclass[MAX_SCOREBOARD];
//void R_InitTranslatePlayerTextures (void)
//{
//	int i;
//
//	for (i = 0; i < MAX_SCOREBOARD; i++)
//	{
//		oldtop[i] = -1;
//		oldbottom[i] = -1;
//		oldskinnum[i] = -1;
//		oldplayerclass[i] = -1;
//
//		playertextures[i] = NULL; //clear playertexture pointers
//	}
//}


/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
	extern	byte		player_texels[MAX_PLAYER_CLASS][620*245];

	int		top, bottom;
	byte	translate[256];
	int		i, size;
	model_t	*model;
	aliashdr_t *paliashdr;
	byte	*original;
	byte	*src, *dst; 
	byte	*pixels = NULL;
	byte	*sourceA, *sourceB, *colorA, *colorB;
	int		playerclass;
	char		name[64];

	//
	// locate the original skin pixels
	//
	currententity = &cl_entities[1+playernum];
	model = currententity->model;

	if (!model)
		return;		// player doesn't have a model yet
	if (model->type != mod_alias)
		return; // only translate skins on alias models

	paliashdr = (aliashdr_t *)Mod_Extradata (model);

	top = (cl.scores[playernum].colors & 0xf0) >> 4;
	bottom = (cl.scores[playernum].colors & 15);
	playerclass = (int)cl.scores[playernum].playerclass;

/*	if (!strcmp (currententity->model->name, "models/paladin.mdl") ||
		!strcmp (currententity->model->name, "models/crusader.mdl") ||
		!strcmp (currententity->model->name, "models/necro.mdl") ||
		!strcmp (currententity->model->name, "models/assassin.mdl") ||
		!strcmp (currententity->model->name, "models/succubus.mdl"))	*/
	if (currententity->model == player_models[0] ||
	    currententity->model == player_models[1] ||
	    currententity->model == player_models[2] ||
	    currententity->model == player_models[3] ||
	    currententity->model == player_models[4])
	{
		if (top == oldtop[playernum] && bottom == oldbottom[playernum] && 
			currententity->skinnum == oldskinnum[playernum] && playerclass == oldplayerclass[playernum])
			return; // translate if only player change his color
	}
	else
	{
		oldtop[playernum] = -1;
		oldbottom[playernum] = -1;
		oldskinnum[playernum] = -1;
		oldplayerclass[playernum] = -1;
		goto skip;
	}

	oldtop[playernum] = top;
	oldbottom[playernum] = bottom;
	oldskinnum[playernum] = currententity->skinnum;
	oldplayerclass[playernum] = playerclass;

skip:
	for (i=0 ; i<256 ; i++)
		translate[i] = i;

	top -= 1;
	bottom -= 1;

	colorA = playerTranslation + 256 + color_offsets[playerclass-1];
	colorB = colorA + 256;
	sourceA = colorB + 256 + (top * 256);
	sourceB = colorB + 256 + (bottom * 256);
	for(i=0;i<256;i++,colorA++,colorB++,sourceA++,sourceB++)
	{
		if (top >= 0 && (*colorA != 255)) 
			translate[i] = *sourceA;
		if (bottom >= 0 && (*colorB != 255)) 
			translate[i] = *sourceB;
	}

	// class limit is mission pack dependant
	i = portals ? MAX_PLAYER_CLASS : MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES;
	if (playerclass >= 1 && playerclass <= i)
		original = player_texels[playerclass-1];
	else
		original = player_texels[0];

	//
	// translate texture
	//
	sprintf (name, "%s_%i_%i", currententity->model->name, currententity->skinnum, playernum);
	size = paliashdr->skinwidth * paliashdr->skinheight;

	// allocate dynamic memory
	pixels = malloc (size);

	dst = pixels;
	src = original;

	for (i=0; i<size; i++)
		*dst++ = translate[*src++];

	original = pixels;

	// do it fast and correct.
//	GL_Bind(playertextures + playernum);
//	GL_Upload8 (name, original, paliashdr->skinwidth, paliashdr->skinheight, false, false, 0); 

	//upload new image
	playertextures[playernum] = TexMgr_LoadTexture (currententity->model, name, paliashdr->skinwidth, paliashdr->skinheight, SRC_INDEXED, original, "", (uintptr_t)original, TEXPREF_PAD | TEXPREF_OVERWRITE);


	// free allocated memory
	free (pixels);

}


/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		d_lightstyle[i] = 264;		// normal light value

	memset (&r_worldentity, 0, sizeof(r_worldentity));
	r_worldentity.model = cl.worldmodel;

// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;
		 	
	r_viewleaf = NULL;

	R_ClearParticles ();

	R_BuildLightmaps ();

//	R_ParseWorldspawnNewMap ();
}


/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int			i;
	float		start, stop, time;

	if (cls.state != ca_connected)
	{
		Con_Printf ("Not connected to a server\n");
		return;
	}

	start = Sys_DoubleTime ();
	for (i=0 ; i<128 ; i++)
	{
		GL_BeginRendering (&glx, &gly, &glwidth, &glheight);

		r_refdef.viewangles[1] = i/128.0*360.0;
		R_RenderView ();
//		glFinish ();

// workaround to avoid flickering uncovered by 3d refresh 2d areas when bloom enabled
		GL_Set2D ();  
		if (/*scr_sbar.value || */scr_viewsize.value < 120)
		{
//			SCR_TileClear ();
			Sbar_Changed ();
			Sbar_Draw ();
		}

		GL_EndRendering ();
	}

	stop = Sys_DoubleTime ();
	time = stop-start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128/time);
}

