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
// gl_main.c

#include "quakedef.h"

entity_t	r_worldentity;


vec3_t		modelorg, r_entorigin;
entity_t	*currententity;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];

int			rs_c_brush_polys, rs_c_brush_passes, rs_c_alias_polys, rs_c_alias_passes, rs_c_sky_polys, rs_c_sky_passes;
int			rs_c_dynamic_lightmaps, rs_c_particles;
qboolean	envmap;				// true during envmap command capture 

gltexture_t *playertextures[MAX_SCOREBOARD]; // changed to an array of pointers
gltexture_t			*gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

//mplane_t	*mirror_plane;

float		model_constant_alpha;

float		r_time1;
float		r_lasttime1 = 0;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

// mirror unused stuff
float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

int		d_lightstyle[256];	// 8.8 fraction of base light value

float r_fovx, r_fovy;

void R_MarkLeaves (void);
// interpolation
void R_SetupAliasFrame (entity_t *e, aliashdr_t *paliashdr, lerpdata_t *lerpdata);
void R_SetupEntityTransform (entity_t *e, lerpdata_t *lerpdata);
void GL_DrawEntityTransform (lerpdata_t lerpdata);
void GL_DrawAliasFrame (aliashdr_t *paliashdr, lerpdata_t lerpdata);


cvar_t	r_norefresh = {"r_norefresh","0"};
cvar_t	r_drawentities = {"r_drawentities","1"};
cvar_t	r_drawworld = {"r_drawworld","1"};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1"};
cvar_t	r_speeds = {"r_speeds","0"};
cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_ambient = { "r_ambient","0", CVAR_NONE};
cvar_t	r_wateralpha = {"r_wateralpha","1", true};
cvar_t	r_lockalpha = {"r_lockalpha","0", true};
cvar_t	r_lavafog = {"r_lavafog","0.5", true};
cvar_t	r_slimefog = {"r_slimefog","0.8", true};
cvar_t	r_lavaalpha = {"r_lavaalpha","1", true};
cvar_t	r_slimealpha = {"r_slimealpha","1", true};
cvar_t	r_telealpha = {"r_telealpha","1", true};
cvar_t	r_dynamic = {"r_dynamic","1"};
cvar_t	r_novis = {"r_novis","0"};
cvar_t	r_lockfrustum =	{"r_lockfrustum","0"};
cvar_t	r_lockpvs = {"r_lockpvs","0"};
cvar_t	r_waterwarp = {"r_waterwarp", "1", true};
cvar_t	r_clearcolor = {"r_clearcolor", "2", true}; // Closest to the original

cvar_t	gl_finish = {"gl_finish","0"};
cvar_t	gl_clear = {"gl_clear","0"};
cvar_t	gl_cull = {"gl_cull","1"};
cvar_t	gl_farclip = {"gl_farclip","16384", CVAR_ARCHIVE};
cvar_t	gl_texsort = {"gl_texsort","1"};
cvar_t	gl_smoothmodels = {"gl_smoothmodels","1"};
cvar_t	gl_affinemodels = {"gl_affinemodels","0"};
cvar_t	gl_polyblend = {"gl_polyblend","1"};
cvar_t	gl_flashblend = {"gl_flashblend","0"};
//cvar_t	gl_playermip = {"gl_playermip","0"};
cvar_t	gl_nocolors = {"gl_nocolors","0"};
//cvar_t	gl_keeptjunctions = {"gl_keeptjunctions","1",true};
//cvar_t	gl_reporttjunctions = {"gl_reporttjunctions","0"};

//cvar_t	gl_zfix = {"gl_zfix","0"}; // z-fighting fix


//extern	cvar_t	gl_ztrick;
static qboolean AlwaysDrawModel;

static void R_RotateForEntity2(entity_t *e);
void R_RotateForEntity (entity_t *e);

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, mplane_t *p)
{
	float	dist1, dist2;
	int		sides;
	
	// this is done by the BOX_ON_PLANE_SIDE macro before calling this function
/*
	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}
*/
	// general case
	switch (p->signbits)
	{
		case 0:
			dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
			dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
			break;
		case 1:
			dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
			dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
			break;
		case 2:
			dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
			dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
			break;
		case 3:
			dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
			dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
			break;
		case 4:
			dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
			dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
			break;
		case 5:
			dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
			dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
			break;
		case 6:
			dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
			dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
			break;
		case 7:
			dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
			dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
			break;
		default:
			dist1 = dist2 = 0;		// make compiler happy
			Host_Error ("BoxOnPlaneSide: Bad signbits");
			break;
	}
	
	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;
	
	return sides;
}


/*
=================
R_CullBox
replaced with new function from lordhavoc

Returns true if the box is completely outside the frustum
=================
*/
qboolean R_CullBox (vec3_t emins, vec3_t emaxs)
{
	int i;
	mplane_t *p;
	for (i = 0;i < 4;i++)
	{
		p = frustum + i;
		switch(p->signbits)
		{
			default:
			case 0:
				if (p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2] < p->dist)
					return true;
				break;
			case 1:
				if (p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2] < p->dist)
					return true;
				break;
			case 2:
				if (p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2] < p->dist)
					return true;
				break;
			case 3:
				if (p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2] < p->dist)
					return true;
				break;
			case 4:
				if (p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2] < p->dist)
					return true;
				break;
			case 5:
				if (p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2] < p->dist)
					return true;
				break;
			case 6:
				if (p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2] < p->dist)
					return true;
				break;
			case 7:
				if (p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2] < p->dist)
					return true;
				break;
		}
	}
	return false;
}

/*
===============
R_CullModelForEntity

uses correct bounds based on rotation
===============
*/
qboolean R_CullModelForEntity (entity_t *e)
{
	vec3_t mins, maxs;

	if (e->angles[0] || e->angles[2]) // pitch or roll
	{
		VectorAdd (e->origin, e->model->rmins, mins);
		VectorAdd (e->origin, e->model->rmaxs, maxs);
	}
	else if (e->angles[1]) // yaw
	{
		VectorAdd (e->origin, e->model->ymins, mins);
		VectorAdd (e->origin, e->model->ymaxs, maxs);
	}
	else // no rotation
	{
		VectorAdd (e->origin, e->model->mins, mins);
		VectorAdd (e->origin, e->model->maxs, maxs);
	}

	return R_CullBox (mins, maxs);
}


/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *e)
{
	msprite_t *psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;
	static float	lastmsg = 0;

	psprite = e->model->cache.data;
	frame = e->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		if (IsTimeout (&lastmsg, 2))
		{
			Con_DPrintf ("R_GetSpriteFrame: no such frame %d (%d frames) in %s\n", frame, psprite->numframes, e->model->name);
		}
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + e->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}

/*
=================
R_DrawSpriteModel

now supports all orientations
=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	vec3_t			point, v_forward, v_right, v_up;
	msprite_t		*psprite;
	mspriteframe_t	*frame;
	float			*s_up, *s_right;
	float			angle, sr, cr;

	//TODO: frustum cull it?
	frame = R_GetSpriteFrame (e);
	psprite = e->model->cache.data;

	switch(psprite->type)
	{
		case SPR_VP_PARALLEL_UPRIGHT: //faces view plane, up is towards the heavens
			v_up[0] = 0;
			v_up[1] = 0;
			v_up[2] = 1;
			s_up = v_up;
			s_right = vright;
			break;
		case SPR_FACING_UPRIGHT: //faces camera origin, up is towards the heavens
			VectorSubtract(e->origin, r_origin, v_forward);
			v_forward[2] = 0;
			VectorNormalizeFast(v_forward);
			v_right[0] = v_forward[1];
			v_right[1] = -v_forward[0];
			v_right[2] = 0;
			v_up[0] = 0;
			v_up[1] = 0;
			v_up[2] = 1;
			s_up = v_up;
			s_right = v_right;
			break;
		case SPR_VP_PARALLEL: //faces view plane, up is towards the top of the screen
			s_up = vup;
			s_right = vright;
			break;
		case SPR_ORIENTED: //pitch yaw roll are independent of camera (bullet marks on walls)
			AngleVectors (e->angles, v_forward, v_right, v_up);
			s_up = v_up;
			s_right = v_right;
			break;
		case SPR_VP_PARALLEL_ORIENTED: //faces view plane, but obeys roll value
			angle = e->angles[ROLL] * M_PI_DIV_180;
			sr = sin(angle);
			cr = cos(angle);
			v_right[0] = vright[0] * cr + vup[0] * sr;
			v_right[1] = vright[1] * cr + vup[1] * sr;
			v_right[2] = vright[2] * cr + vup[2] * sr;
			v_up[0] = vright[0] * -sr + vup[0] * cr;
			v_up[1] = vright[1] * -sr + vup[1] * cr;
			v_up[2] = vright[2] * -sr + vup[2] * cr;
			s_up = v_up;
			s_right = v_right;
			break;
		default:
			return;
	}

	GL_SelectTMU0 (); // selects TEXTURE0
	GL_BindTexture (frame->gltexture);

	// offset decals
	if (psprite->type == SPR_ORIENTED)
	{
		glPolygonOffset (-1, -10000);
		glEnable (GL_POLYGON_OFFSET_LINE);
		glEnable (GL_POLYGON_OFFSET_FILL);
	}
/*
	// H2
	if ((e->drawflags & DRF_TRANSLUCENT) || (e->model->flags & EF_TRANSPARENT))
	{
		if (psprite->type == SPR_ORIENTED)
			glDepthMask (GL_FALSE); // don't bother writing Z
		glEnable (GL_BLEND);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f(1.0f, 1.0f, 1.0f, r_wateralpha.value);

		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable (GL_ALPHA_TEST);
//		glAlphaFunc (GL_GEQUAL, 0.5);
		glAlphaFunc (GL_GREATER, 0.7);
	}
	else
	{
		if (psprite->type == SPR_ORIENTED)
			glDepthMask (GL_FALSE); // don't bother writing Z
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable (GL_ALPHA_TEST);
		glAlphaFunc (GL_GEQUAL, 0.5);
	}
*/
	// Q1
	glColor3f (1,1,1); //FX
	// Q1
	glEnable (GL_ALPHA_TEST); //FX

//	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (e->origin, frame->down, s_up, point);
	VectorMA (point, frame->left, s_right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (e->origin, frame->up, s_up, point);
	VectorMA (point, frame->left, s_right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (e->origin, frame->up, s_up, point);
	VectorMA (point, frame->right, s_right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (e->origin, frame->down, s_up, point);
	VectorMA (point, frame->right, s_right, point);
	glVertex3fv (point);

	glEnd ();

//	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	// Q1
	glDisable (GL_ALPHA_TEST); //FX
/*
	// H2
	if ((e->drawflags & DRF_TRANSLUCENT) || (e->model->flags & EF_TRANSPARENT))
	{
		if (psprite->type == SPR_ORIENTED)
			glDepthMask (GL_TRUE); // back to normal Z buffering
		glDisable (GL_BLEND);

		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		glDisable (GL_ALPHA_TEST);
		glAlphaFunc (GL_GREATER, 0.666);
	}
	else
	{
		if (psprite->type == SPR_ORIENTED)
			glDepthMask (GL_TRUE); // back to normal Z buffering
		glDisable (GL_BLEND);
		glDisable (GL_ALPHA_TEST);
		glAlphaFunc (GL_GREATER, 0.666);
	}
*/
	// offset decals
	if (psprite->type == SPR_ORIENTED)
	{
		glPolygonOffset (0, 0);
		glDisable (GL_POLYGON_OFFSET_LINE);
		glDisable (GL_POLYGON_OFFSET_FILL);
	}
}


/*
=================
R_DrawSpriteModel

hexen2 version
=================
*/
/*
typedef struct
{
	vec3_t			vup, vright, vpn;	// in worldspace
} spritedesc_t;

void R_DrawSpriteModel (entity_t *e)
{
	vec3_t	point;
	mspriteframe_t	*frame;
	msprite_t		*psprite;
	
	vec3_t			tvec;
	float			dot, angle, sr, cr;
	spritedesc_t			r_spritedesc;
	int i;

	frame = R_GetSpriteFrame (e);
	psprite = e->model->cache.data;

	if (psprite->type == SPR_FACING_UPRIGHT)
	{
	// generate the sprite's axes, with vup straight up in worldspace, and
	// r_spritedesc.vright perpendicular to modelorg.
	// This will not work if the view direction is very close to straight up or
	// down, because the cross product will be between two nearly parallel
	// vectors and starts to approach an undefined state, so we don't draw if
	// the two vectors are less than 1 degree apart
		tvec[0] = -modelorg[0];
		tvec[1] = -modelorg[1];
		tvec[2] = -modelorg[2];
		VectorNormalize (tvec);
		dot = tvec[2];	// same as DotProduct (tvec, r_spritedesc.vup) because
						//  r_spritedesc.vup is 0, 0, 1
		if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree) = 0.999848
			return;
		r_spritedesc.vup[0] = 0;
		r_spritedesc.vup[1] = 0;
		r_spritedesc.vup[2] = 1;
		r_spritedesc.vright[0] = tvec[1];
								// CrossProduct(r_spritedesc.vup, -modelorg,
		r_spritedesc.vright[1] = -tvec[0];
								//              r_spritedesc.vright)
		r_spritedesc.vright[2] = 0;
		VectorNormalize (r_spritedesc.vright);
		r_spritedesc.vpn[0] = -r_spritedesc.vright[1];
		r_spritedesc.vpn[1] = r_spritedesc.vright[0];
		r_spritedesc.vpn[2] = 0;
					// CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
					//  r_spritedesc.vpn)
	}
	else if (psprite->type == SPR_VP_PARALLEL)
	{
	// generate the sprite's axes, completely parallel to the viewplane. There
	// are no problem situations, because the sprite is always in the same
	// position relative to the viewer
		for (i=0 ; i<3 ; i++)
		{
			r_spritedesc.vup[i] = vup[i];
			r_spritedesc.vright[i] = vright[i];
			r_spritedesc.vpn[i] = vpn[i];
		}
	}
	else if (psprite->type == SPR_VP_PARALLEL_UPRIGHT)
	{
	// generate the sprite's axes, with vup straight up in worldspace, and
	// r_spritedesc.vright parallel to the viewplane.
	// This will not work if the view direction is very close to straight up or
	// down, because the cross product will be between two nearly parallel
	// vectors and starts to approach an undefined state, so we don't draw if
	// the two vectors are less than 1 degree apart
		dot = vpn[2];	// same as DotProduct (vpn, r_spritedesc.vup) because
						//  r_spritedesc.vup is 0, 0, 1
		if ((dot > 0.999848) || (dot < -0.999848))	// cos(1 degree) = 0.999848 //EER1 todo  COS1DEG == 0.999848
			return;
		r_spritedesc.vup[0] = 0;
		r_spritedesc.vup[1] = 0;
		r_spritedesc.vup[2] = 1;
		r_spritedesc.vright[0] = vpn[1];
										// CrossProduct (r_spritedesc.vup, vpn,
		r_spritedesc.vright[1] = -vpn[0];	//  r_spritedesc.vright)
		r_spritedesc.vright[2] = 0;
		VectorNormalize (r_spritedesc.vright);
		r_spritedesc.vpn[0] = -r_spritedesc.vright[1];
		r_spritedesc.vpn[1] = r_spritedesc.vright[0];
		r_spritedesc.vpn[2] = 0;
					// CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
					//  r_spritedesc.vpn)
	}
	else if (psprite->type == SPR_ORIENTED)
	{
	// generate the sprite's axes, according to the sprite's world orientation
		AngleVectors (e->angles, r_spritedesc.vpn,
					  r_spritedesc.vright, r_spritedesc.vup);
	}
	else if (psprite->type == SPR_VP_PARALLEL_ORIENTED)
	{
	// generate the sprite's axes, parallel to the viewplane, but rotated in
	// that plane around the center according to the sprite entity's roll
	// angle. So vpn stays the same, but vright and vup rotate
		angle = e->angles[ROLL] * (M_PI*2 / 360);
		sr = sin(angle);
		cr = cos(angle);

		for (i=0 ; i<3 ; i++)
		{
			r_spritedesc.vpn[i] = vpn[i];
			r_spritedesc.vright[i] = vright[i] * cr + vup[i] * sr;
			r_spritedesc.vup[i] = vright[i] * -sr + vup[i] * cr;
		}
	}
	else
	{
		Sys_Error ("R_DrawSprite: Bad sprite type %d", psprite->type);
	}

	if ((e->drawflags & DRF_TRANSLUCENT) || (e->model->flags & EF_TRANSPARENT))
	{
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f(1.0f, 1.0f, 1.0f, r_wateralpha.value);
	}
	else
	{
		glEnable(GL_BLEND);
		glColor3f(1,1,1);
	}

	GL_DisableMultitexture (); // selects TEXTURE0
	GL_Bind(frame->gltexture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (e->origin, frame->down, r_spritedesc.vup, point);
	VectorMA (point, frame->left, r_spritedesc.vright, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (e->origin, frame->up, r_spritedesc.vup, point);
	VectorMA (point, frame->left, r_spritedesc.vright, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (e->origin, frame->up, r_spritedesc.vup, point);
	VectorMA (point, frame->right, r_spritedesc.vright, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (e->origin, frame->down, r_spritedesc.vup, point);
	VectorMA (point, frame->right, r_spritedesc.vright, point);
	glVertex3fv (point);

	glEnd ();

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if ((e->drawflags & DRF_TRANSLUCENT) || (e->model->flags & EF_TRANSPARENT))
	{
		glDisable(GL_BLEND);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}
	else
	{
		glDisable(GL_BLEND);
	}
}
*/


/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162
float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

//vec3_t	shadevector;
//float	shadelight, ambientlight;
extern vec3_t	lightcolor; // replaces "float shadelight" for lit support

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float	*shadedots = r_avertexnormal_dots[0];

//int	lastposenum;

qboolean shading = true; // if false, disable vertex shading for various reasons (fullbright, etc)

float	entalpha;



/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i;
	int			lnum;
	vec3_t		dist;
	float		add;
	model_t		*clmodel;
//	vec3_t		mins, maxs;
	aliashdr_t	*paliashdr;
	int			anim;
	float		an;
	static float	tmatrix[3][4];
	float entScale;
	float xyfact;
	float zfact;
	qpic_t		*stonepic;
	glpic_t			*gl;
	char temp[40];
	int mls;
	vec3_t		adjust_origin;
	qboolean	isclient = false;
	int			skinnum, client_no;
	gltexture_t	*base, *glow;
	static float	lastmsg = 0;
	lerpdata_t	lerpdata;
	float		scale;

	//
	// locate the proper data
	//
	paliashdr = (aliashdr_t *)Mod_Extradata (e->model);

	//
	// setup pose/lerp data -- do it first so we don't miss updates due to culling
	//
	R_SetupAliasFrame (e, paliashdr, &lerpdata);
	R_SetupEntityTransform (e, &lerpdata);

	//
	// cull it
	//
	if (R_CullModelForEntity(e))
		return;

	VectorCopy (e->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);

	clmodel = e->model;
	skinnum = e->skinnum;
	client_no = e - cl_entities;

	// r_speeds
	rs_c_alias_polys += paliashdr->numtris;

	// check skin bounds
	if (skinnum >= paliashdr->numskins || skinnum < 0)
	{
		if (IsTimeout (&lastmsg, 2))
		{
			Con_DPrintf ("R_DrawAliasModel: no such skin %d (%d skins) in %s\n", skinnum, paliashdr->numskins, clmodel->name);
		}
		skinnum = 0;
	}

	//
	// transform it
	//
	glPushMatrix ();

	GL_DrawEntityTransform (lerpdata); // FX
//	R_RotateForEntity2(e);
//	R_RotateForEntity(e);

	if(e->scale != 0 && e->scale != 100)
	{
		entScale = (float)e->scale/100.0;
		switch(e->drawflags & SCALE_TYPE_MASKIN)
		{
			case SCALE_TYPE_UNIFORM:
				tmatrix[0][0] = paliashdr->scale[0]*entScale;
				tmatrix[1][1] = paliashdr->scale[1]*entScale;
				tmatrix[2][2] = paliashdr->scale[2]*entScale;
				xyfact = zfact = (entScale-1.0)*127.95;
				break;
			case SCALE_TYPE_XYONLY:
				tmatrix[0][0] = paliashdr->scale[0]*entScale;
				tmatrix[1][1] = paliashdr->scale[1]*entScale;
				tmatrix[2][2] = paliashdr->scale[2];
				xyfact = (entScale-1.0)*127.95;
				zfact = 1.0;
				break;
			case SCALE_TYPE_ZONLY:
				tmatrix[0][0] = paliashdr->scale[0];
				tmatrix[1][1] = paliashdr->scale[1];
				tmatrix[2][2] = paliashdr->scale[2]*entScale;
				xyfact = 1.0;
				zfact = (entScale-1.0)*127.95;
				break;
		}
		switch(e->drawflags & SCALE_ORIGIN_MASKIN)
		{
			case SCALE_ORIGIN_CENTER:
				tmatrix[0][3] = paliashdr->scale_origin[0]-paliashdr->scale[0]*xyfact;
				tmatrix[1][3] = paliashdr->scale_origin[1]-paliashdr->scale[1]*xyfact;
				tmatrix[2][3] = paliashdr->scale_origin[2]-paliashdr->scale[2]*zfact;
				break;
			case SCALE_ORIGIN_BOTTOM:
				tmatrix[0][3] = paliashdr->scale_origin[0]-paliashdr->scale[0]*xyfact;
				tmatrix[1][3] = paliashdr->scale_origin[1]-paliashdr->scale[1]*xyfact;
				tmatrix[2][3] = paliashdr->scale_origin[2];
				break;
			case SCALE_ORIGIN_TOP:
				tmatrix[0][3] = paliashdr->scale_origin[0]-paliashdr->scale[0]*xyfact;
				tmatrix[1][3] = paliashdr->scale_origin[1]-paliashdr->scale[1]*xyfact;
				tmatrix[2][3] = paliashdr->scale_origin[2]-paliashdr->scale[2]*zfact*2.0;
				break;
		}
	}
	else
	{
		tmatrix[0][0] = paliashdr->scale[0];
		tmatrix[1][1] = paliashdr->scale[1];
		tmatrix[2][2] = paliashdr->scale[2];
		tmatrix[0][3] = paliashdr->scale_origin[0];
		tmatrix[1][3] = paliashdr->scale_origin[1];
		tmatrix[2][3] = paliashdr->scale_origin[2];
	}

	if(clmodel->flags & EF_ROTATE)
	{ // Floating motion
//		tmatrix[2][3] += sin(currententity->origin[0]
//			+currententity->origin[1]+(cl.time*3))*5.5; // fixme: should use lerpdata->origin

		tmatrix[2][3] += sin(lerpdata.origin[0]+lerpdata.origin[1]+(cl.time*3))*5.5; // fixme: should use lerpdata->origin
	}

// [0][3] [1][3] [2][3]
//	glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	glTranslatef (tmatrix[0][3],tmatrix[1][3],tmatrix[2][3]);
// [0][0] [1][1] [2][2]
//	glScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
	glScalef (tmatrix[0][0],tmatrix[1][1],tmatrix[2][2]);


	// special handling of view model to keep FOV from altering look.
//FX
/*	if (e == &cl.viewent)
		scale = 1.0f / tan( DEG2RAD (r_fovx / 2.0f) ) * scr_weaponfov.value / 90.0f; // reverse out fov and do fov we want
	else
		scale = 1.0f;

	glTranslatef (paliashdr->scale_origin[0] * scale, paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	glScalef (paliashdr->scale[0] * scale, paliashdr->scale[1], paliashdr->scale[2]);
*/

/*
//orig.
	glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
	glScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
*/
	//
	// model rendering stuff
	//
	if (gl_smoothmodels.value)
		glShadeModel (GL_SMOOTH);
	if (gl_affinemodels.value)
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	shading = true;

	//
	// set up for alpha blending
	//
	entalpha = ENTALPHA_DECODE(e->alpha);

	if (entalpha == 0)
		goto cleanup;

	if (entalpha < 1.0)
	{
		glDepthMask (GL_FALSE);
		glEnable (GL_BLEND);
	}

	//
	// set up lighting
	//
	R_LightPoint (e->origin, lightcolor);

	// add dlights
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			VectorSubtract (e->origin, cl_dlights[lnum].origin, dist);
			add = cl_dlights[lnum].radius - VectorLength(dist);

			if (add > 0)
				VectorMA (lightcolor, add, cl_dlights[lnum].color, lightcolor);
		}
	}

	// minimum light value on gun (24)
	if (e == &cl.viewent)
	{
		add = 72.0f - (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add > 0.0f)
		{
			lightcolor[0] += add / 3.0f;
			lightcolor[1] += add / 3.0f;
			lightcolor[2] += add / 3.0f;
		}
	}

	if (client_no >= 1 && client_no <= cl.maxclients)
	{
		isclient = true;

		// minimum light value on players (8)
		add = 24.0f - (lightcolor[0] + lightcolor[1] + lightcolor[2]);
		if (add > 0.0f)
		{
			lightcolor[0] += add / 3.0f;
			lightcolor[1] += add / 3.0f;
			lightcolor[2] += add / 3.0f;
		}
	}

	// clamp lighting so it doesn't overbright as much (96)
	add = 288.0f / (lightcolor[0] + lightcolor[1] + lightcolor[2]);
	if (add < 1.0f)
		VectorScale (lightcolor, add, lightcolor);

	shadedots = r_avertexnormal_dots[((int)(e->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	//VectorScale (lightcolor, 1.0f / 192.0f, lightcolor);//orig.
	VectorScale (lightcolor, 1.0f / 160.0f, lightcolor); //FX, new value

	//
	// set up textures
	//
//	GL_DisableMultitexture (); // selects TEXTURE0

	anim = (int)(cl.time*10) & 3;
	base = paliashdr->base[skinnum][anim];
	glow = paliashdr->glow[skinnum][anim];

	if (e->skinnum >= 100)
	{
		if (e->skinnum > 255) 
		{
			Sys_Error ("skinnum > 255");
		}

		if (gl_extra_textures[e->skinnum-100] == NULL)  // Need to load it in
		{
			sprintf(temp,"gfx/skin%d.lmp",e->skinnum);
			stonepic = Draw_CachePic(temp);
			gl = (glpic_t *)stonepic->data;
			gl_extra_textures[e->skinnum-100] = gl->gltexture;
		}

		base = gl_extra_textures[e->skinnum-100];
	}
	else
	{

		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
	
		if (e->colormap /* != vid.colormap */ && !gl_nocolors.value)
		{
			if (e->model == player_models[0] ||
			    e->model == player_models[1] ||
			    e->model == player_models[2] ||
			    e->model == player_models[3] ||
			    e->model == player_models[4])
			{
				if (isclient)
					base = playertextures[client_no - 1];
			}
		}
	}

	//
	// draw it
	//
//	if (gl_mtexable && gl_texture_env_combine && gl_texture_env_add && glow) // case 1: everything in one pass
	{
		// Binds normal skin to texture env 0
		GL_SelectTMU0 (); // selects TEXTURE0
		GL_BindTexture (base);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR_ARB);
		glTexEnvf (GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2.0f);

		// Binds fullbright skin to texture env 1
		GL_SelectTMU2 (); // selects TEXTURE2
		GL_BindTexture (glow);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		glEnable (GL_BLEND);
		GL_DrawAliasFrame (paliashdr, lerpdata); // FX
		glDisable (GL_BLEND);
		GL_SelectTMU0 (); // selects TEXTURE0
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}
//	else if (gl_texture_env_combine) // case 2: overbright in one pass, then fullbright pass
//	{
//		// first pass
//		GL_Bind (base);
//		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
//		glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
//		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
//		glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT);
//		glTexEnvf (GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0f);
//		GL_DrawAliasFrame (paliashdr, lerpdata); // FX
//		glTexEnvf (GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 1.0f);
//		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//
//		// second pass
//		if (glow)
//		{
//			GL_Bind (glow);
//			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//			glEnable (GL_BLEND);
//			glBlendFunc (GL_ONE, GL_ONE);
//			glDepthMask (GL_FALSE);
//			shading = false;
//			glColor3f (entalpha, entalpha, entalpha);
//			R_FogStartAdditive ();
//			GL_DrawAliasFrame (paliashdr, lerpdata); // FX
//			R_FogStopAdditive ();
//			glDepthMask (GL_TRUE);
//			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//			glDisable (GL_BLEND);
//			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//		}
//	}
//	else // case 3: overbright in two passes, then fullbright pass
//	{
//		// first pass
//		GL_Bind (base);
//		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//		GL_DrawAliasFrame (paliashdr, lerpdata); // FX
//
//		// second pass -- additive with black fog, to double the object colors but not the fog color
//		glEnable (GL_BLEND);
//		glBlendFunc (GL_ONE, GL_ONE);
//		glDepthMask (GL_FALSE);
//		R_FogStartAdditive ();
//		GL_DrawAliasFrame (paliashdr, lerpdata); // FX
//		R_FogStopAdditive ();
//		glDepthMask (GL_TRUE);
//		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		glDisable (GL_BLEND);
//
//		// third pass
//		if (glow)
//		{
//			GL_Bind (glow);
//			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//			glEnable (GL_BLEND);
//			glBlendFunc (GL_ONE, GL_ONE);
//			glDepthMask (GL_FALSE);
//			shading = false;
//			glColor3f (entalpha, entalpha, entalpha);
//			R_FogStartAdditive ();
//			GL_DrawAliasFrame (paliashdr, lerpdata); // FX
//			R_FogStopAdditive ();
//			glDepthMask (GL_TRUE);
//			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//			glDisable (GL_BLEND);
//			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//		}
//	}

cleanup:
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // gl_affinemodels
	glShadeModel (GL_FLAT); // gl_smoothmodels
	glDepthMask (GL_TRUE);
	glDisable (GL_BLEND);
	glColor3f (1, 1, 1);
	glPopMatrix ();

}

//==================================================================================

typedef struct sortedent_s {
	entity_t *ent;
	vec_t len;
} sortedent_t;

sortedent_t     cl_transvisedicts[MAX_VISEDICTS];
sortedent_t		cl_transwateredicts[MAX_VISEDICTS];

int				cl_numtransvisedicts;
int				cl_numtranswateredicts;

/*
=============
R_DrawEntities
=============
*/
void R_DrawEntities (qboolean alphapass)
{
	int		i;

	if (!r_drawentities.value)
		return;

	// sprites are a special case
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		if ((i + 1) % 100 == 0)
			S_ExtraUpdateTime (); // don't let sound get messed up if going slow

		currententity = cl_visedicts[i];

//DBG
//		Con_Printf("[2]R_DrawTransEntities: model: %s, e->origin[0]: %f, e->origin[1]: %f, e->origin[2]: %f\n---*---\n", currententity->model->name, 
//			currententity->origin[0], currententity->origin[1], currententity->origin[2]);
//
//		Con_Printf("ent: %s mins: mins[0]: %f, mins[1]: %f, mins[2]: %f\n", currententity->model->name, 
//			currententity->model->mins[0], currententity->model->mins[1], currententity->model->mins[2]);
//		Con_Printf("ent: %s maxs: maxs[0]: %f, maxs[1]: %f, maxs[2]: %f\n---*---\n", currententity->model->name, 
//			currententity->model->maxs[0], currententity->model->maxs[1], currententity->model->maxs[2]);
//DBG			
		
		// if alphapass is true, draw only alpha entites this time
		// if alphapass is false, draw only nonalpha entities this time
		if ((ENTALPHA_DECODE(currententity->alpha) < 1 && !alphapass) ||
			(ENTALPHA_DECODE(currententity->alpha) == 1 && alphapass))
			continue;

		// chase_active
		if (currententity == &cl_entities[cl.viewentity])
			currententity->angles[0] *= 0.3;

		switch (currententity->model->type)
		{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;

			case mod_brush:
				R_DrawBrushModel (currententity);
				break;

			default:
				break;
		}
	}
}

/*
=============
R_DrawWaterEntities
=============
*/
void R_DrawWaterEntities (qboolean alphapass)
{
	int		i;

	if (!r_drawentities.value)
		return;

	// special case to draw water entities
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		if ((i + 1) % 100 == 0)
			S_ExtraUpdateTime (); // don't let sound get messed up if going slow

		currententity = cl_visedicts[i];

		// if alphapass is true, draw only alpha entites this time
		// if alphapass is false, draw only nonalpha entities this time
		if ((ENTALPHA_DECODE(currententity->alpha) < 1 && !alphapass) ||
			(ENTALPHA_DECODE(currententity->alpha) == 1 && alphapass))
			continue;

		switch (currententity->model->type)
		{
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;

			default:
				break;
		}
	}
}

/*
=============
R_DrawSprites
=============
*/
void R_DrawSprites (void)
{
	int		i;

	if (!r_drawentities.value)
		return;

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		if ((i + 1) % 100 == 0)
			S_ExtraUpdateTime (); // don't let sound get messed up if going slow

		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;

			default:
				break;
		}
	}
}


/*
================
R_DrawTransEntitiesOnList
Implemented by: jack
================
*/

int transCompare(const void *arg1, const void *arg2 ) 
{
	const sortedent_t *a1, *a2;
	a1 = (sortedent_t *) arg1;
	a2 = (sortedent_t *) arg2;
	return (a2->len - a1->len); // Sorted in reverse order.  Neat, huh?
}

void R_DrawTransEntitiesOnList ( qboolean inwater ) 
{
	int i;
	int numents;
	sortedent_t *theents;
	int depthMaskWrite = 0;
	vec3_t result;

	theents = (inwater) ? cl_transwateredicts : cl_transvisedicts;
	numents = (inwater) ? cl_numtranswateredicts : cl_numtransvisedicts;

	for (i=0; i<numents; i++)
	{
		VectorSubtract(theents[i].ent->origin, r_origin, result);
//		theents[i].len = VectorLength(result);
		theents[i].len = (result[0] * result[0]) + (result[1] * result[1]) + (result[2] * result[2]);
	}

	qsort((void *) theents, numents, sizeof(sortedent_t), transCompare);
	// Add in BETTER sorting here

	glDepthMask(0);
	for (i=0 ; i<numents ; i++) 
	{
		currententity = theents[i].ent;

		switch (currententity->model->type)
		{
		case mod_alias:
			if (!depthMaskWrite) 
			{
				depthMaskWrite = 1;
				glDepthMask(1);
			}
			R_DrawAliasModel (currententity);
			break;
		case mod_brush:
			if (!depthMaskWrite) 
			{
				depthMaskWrite = 1;
				glDepthMask(1);
			}
			R_DrawBrushModel (currententity);
			break;
		case mod_sprite:
			if (depthMaskWrite) 
			{
				depthMaskWrite = 0;
				glDepthMask(0);
			}
			R_DrawSpriteModel (currententity);
			break;
		}
	}
	if (!depthMaskWrite) 
		glDepthMask(1);
}

/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	if (!r_drawviewmodel.value || chase_active.value)
		return;

	if (cl.items & IT_INVISIBILITY || cl.v.health <= 0)
		return;

	currententity = &cl.viewent;

	if (!currententity->model)
		return;

	// this fixes a crash
	if (currententity->model->type != mod_alias)
		return;

	// Prevent weapon model error
	if (currententity->model->name[0] == '*')
	{
		Con_Warning ("R_DrawViewModel: viewmodel %s invalid\n", currententity->model->name);
		Cvar_Set ("r_drawviewmodel", "0");
		return;
	}

	// hack the depth range to prevent view model from poking into walls
	glDepthRange (0, 0.3);

	R_DrawAliasModel (currententity);

	glDepthRange (0, 1);
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend.value)
		return;

	if (!v_blend[3])
		return;

	GL_SelectTMU0 (); // selects TEXTURE0

//	glDisable (GL_ALPHA_TEST);
	glDisable (GL_TEXTURE_2D);
	glDisable (GL_DEPTH_TEST);
	glEnable (GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();

	glOrtho (0, 1, 1, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity ();

	glColor4fv (v_blend);

	glBegin (GL_QUADS);
	glVertex2f (0, 0);
	glVertex2f (1, 0);
	glVertex2f (1, 1);
	glVertex2f (0, 1);
	glEnd ();

	glDisable (GL_BLEND);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_TEXTURE_2D);
//	glEnable (GL_ALPHA_TEST);
}


//==================================================================================

/*
===============
SignbitsForPlane
===============
*/
int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test - identify which corner(s) of the box to text against the plane

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}

/*
===============
R_SetFrustum
===============
*/
void R_SetFrustum (float fovx, float fovy)
{
	int		i;

	TurnVector(frustum[0].normal, vpn, vright, fovx/2 - 90); //left plane
	TurnVector(frustum[1].normal, vpn, vright, 90 - fovx/2); //right plane
	TurnVector(frustum[2].normal, vpn, vup, 90 - fovy/2); //bottom plane
	TurnVector(frustum[3].normal, vpn, vup, fovy/2 - 90); //top plane

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal); //FIXME: shouldn't this always be zero?
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
		if (gl_clear.value)
			glClear (GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
}

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
	{
		Cvar_Set ("r_drawentities", "1");
		Cvar_Set ("r_drawworld", "1");
		Cvar_Set ("r_fullbright", "0");
		Cvar_Set ("r_ambient", "0");
	}

	R_PushDlights ();
	R_AnimateLight ();

	r_framecount++;

	R_FogSetupFrame ();

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	// calculate r_fovx and r_fovy here
	r_fovx = r_refdef.fov_x;
	r_fovy = r_refdef.fov_y;
	if (r_waterwarp.value && (r_viewleaf->contents == CONTENTS_WATER || r_viewleaf->contents == CONTENTS_SLIME || r_viewleaf->contents == CONTENTS_LAVA))
	{
		r_fovx = r_refdef.fov_x + sin(cl.time);
		r_fovy = r_refdef.fov_y - cos(cl.time);
	}

	R_SetFrustum (r_fovx, r_fovy); // use r_fov* vars
//	R_MarkSurfaces ();	// create texture chains from PVS (done here so we know if we're in water)
//	R_CullSurfaces ();	// do after R_SetFrustum and R_MarkSurfaces
	R_UpdateWarpTextures ();	// do this before R_Clear
	R_Clear ();
}

void GL_SetFrustum (float fovx, float fovy)
{
	float xmin, xmax, ymin, ymax;

	xmax = NEARCLIP * tan( fovx * M_PI / 360.0 );
	ymax = NEARCLIP * tan( fovy * M_PI / 360.0 );

	xmin = -xmax;
	ymin = -ymax;

	glFrustum (xmin, xmax, ymin, ymax, NEARCLIP, FARCLIP);
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	//
	// set up viewpoint
	//
	glViewport (glx + r_refdef.vrect.x,
				gly + glheight - r_refdef.vrect.y - r_refdef.vrect.height,
				r_refdef.vrect.width,
				r_refdef.vrect.height);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	GL_SetFrustum (r_fovx, r_fovy);

//	glCullFace (GL_FRONT);
//	glCullFace (GL_BACK); // glquake used CCW with backwards culling -- let's do it right

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	
	glRotatef (-90,  1, 0, 0);	    // put Z going up
	glRotatef (90,  0, 0, 1);	    // put Z going up
	glRotatef (-r_refdef.viewangles[2],  1, 0, 0);
	glRotatef (-r_refdef.viewangles[0],  0, 1, 0);
	glRotatef (-r_refdef.viewangles[1],  0, 0, 1);
	glTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);
	
	glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull.value)
		glEnable (GL_CULL_FACE);
	else
		glDisable (GL_CULL_FACE);

	glDisable (GL_BLEND);
	glDisable (GL_ALPHA_TEST);
	glEnable (GL_DEPTH_TEST);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	float	time1 = 0, time2;
	float	ms;
	char str[256]; 

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
//		glFinish ();
		time1 = Sys_DoubleTime ();
	}

	// clear rendering statistics (r_speed)
	rs_c_brush_polys = 
	rs_c_brush_passes = 
	rs_c_alias_polys = 
	rs_c_alias_passes = 
	rs_c_sky_polys = 
	rs_c_sky_passes = 
	rs_c_dynamic_lightmaps = 
	rs_c_particles = 0;

	if (gl_finish.value /* || r_speeds.value */)
		glFinish ();

	// render normal view
	// r_refdef must be set before the first call
	R_SetupFrame ();
	R_SetupGL ();

	S_ExtraUpdateTime ();	// don't let sound get messed up if going slow

	R_FogEnableGFog ();
	R_DrawSky (); // handle worldspawn and bmodels
	R_DrawWorld (); // adds static entities to the list
	R_DrawEntities (false); // false means this is the pass for nonalpha entities
	R_DrawWaterEntities (false); // special case to draw water nonalpha entities
//	R_DrawTextureChainsWater (); // drawn here since they might have transparency
	R_DrawEntities (true); // true means this is the pass for alpha entities
	R_DrawWaterEntities (true); // special case to draw water alpha entities
//	R_RenderDlights ();
	R_DrawSprites ();
//	R_DrawParticles ();
	R_FogDisableGFog ();
	R_DrawViewModel ();
	R_PolyBlend ();
	R_BloomBlend (); // bloom on each frame. 

	S_ExtraUpdateTime ();	// don't let sound get messed up if going slow

	if (r_speeds.value)
	{
//		glFinish ();
		time2 = Sys_DoubleTime ();
		ms = 1000 * (time2 - time1);

		if (r_speeds.value == 2)
			sprintf (str, "%5.1f ms - %4i/%4i wpoly * %4i/%4i epoly * %4i/%4i sky * %4i lmaps * %4i part\n", ms,
				rs_c_brush_polys,
				rs_c_brush_passes,
				rs_c_alias_polys,
				rs_c_alias_passes,
				rs_c_sky_polys,
				rs_c_sky_passes,
				rs_c_dynamic_lightmaps,
				rs_c_particles);
		else
			sprintf (str, "%5.1f ms - %4i wpoly * %4i epoly * %4i lmaps\n", ms, rs_c_brush_polys, rs_c_alias_polys, rs_c_dynamic_lightmaps);

		Con_Printf (str);
	}
}

void R_RotateForEntity (entity_t *e)
{
    glTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    glRotatef (e->angles[1],  0, 0, 1);
    glRotatef (-e->angles[0],  0, 1, 0);
    glRotatef (-e->angles[2],  1, 0, 0);
}
//==========================================================================
//
// R_RotateForEntity2
//
// Same as R_RotateForEntity(), but checks for EF_ROTATE and modifies
// yaw appropriately.
//
//==========================================================================
static void R_RotateForEntity2(entity_t *e)
{
	float	forward;
	float	yaw, pitch;
	vec3_t			angles;

	glTranslatef(e->origin[0], e->origin[1], e->origin[2]);

	if (e->model->flags & EF_FACE_VIEW)
	{
		VectorSubtract(e->origin,r_origin,angles);
		VectorSubtract(r_origin,e->origin,angles);
		VectorNormalize(angles);

		if (angles[1] == 0 && angles[0] == 0)
		{
			yaw = 0;
			if (angles[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int) (atan2(angles[1], angles[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;

			forward = sqrt (angles[0]*angles[0] + angles[1]*angles[1]);
			pitch = (int) (atan2(angles[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}

		angles[0] = pitch;
		angles[1] = yaw;
		angles[2] = 0;

		glRotatef(-angles[0], 0, 1, 0);
		glRotatef(angles[1], 0, 0, 1);
//		glRotatef(-angles[2], 1, 0, 0);
		glRotatef(-e->angles[2], 1, 0, 0);
	}
	else 
	{
		if (e->model->flags & EF_ROTATE)
		{
			glRotatef(anglemod((e->origin[0]+e->origin[1])*0.8
				+(108*cl.time)), 0, 0, 1);
		}
		else
		{
			glRotatef(e->angles[1], 0, 0, 1);
		}
		glRotatef(-e->angles[0], 0, 1, 0);
		glRotatef(-e->angles[2], 1, 0, 0);
	}
}


/*
===============
GL_DrawEntityTransform -- model transform interpolation

R_RotateForEntity renamed and modified to take lerpdata instead of pointer to entity
===============
*/
void GL_DrawEntityTransform (lerpdata_t lerpdata)
{
	glTranslatef (lerpdata.origin[0], lerpdata.origin[1], lerpdata.origin[2]);

	glRotatef ( lerpdata.angles[1],  0, 0, 1);
	glRotatef (-lerpdata.angles[0],  0, 1, 0);
	glRotatef ( lerpdata.angles[2],  1, 0, 0);
}

/*
=============
GL_DrawAliasFrame -- model animation interpolation (lerping)

support colored light, lerping, entalpha, multitexture
=============
*/
void GL_DrawAliasFrame (aliashdr_t *paliashdr, lerpdata_t lerpdata)
{
	float		vertcolor[4]; // replaces "float l" for lit support
	trivertx_t	*verts1, *verts2;
	int			*commands;
	int			count;
	float		u,v;
	float		blend, iblend;
	qboolean	lerping;

	if (lerpdata.pose1 != lerpdata.pose2)
	{
		lerping = true;
		verts1  = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
		verts2  = verts1;

		verts1 += lerpdata.pose1 * paliashdr->poseverts;
		verts2 += lerpdata.pose2 * paliashdr->poseverts;
		blend = lerpdata.blend;
		iblend = 1.0f - blend;
	}
	else // poses the same means that the entity has paused its animation
	{
		lerping = false;
		verts1  = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
		verts2  = verts1; // avoid bogus compiler warning

		verts1 += lerpdata.pose1 * paliashdr->poseverts;
		blend = iblend = 0; // avoid bogus compiler warning
	}

	commands = (int *)((byte *)paliashdr + paliashdr->commands);

	vertcolor[3] = entalpha; // never changes, so there's no need to put this inside the loop

	while (1)
	{
		// get the vertex count and primitive type
		count = *commands++;

		if (!count) 
			break; // done

		if (count < 0)
		{
			count = -count;
			glBegin (GL_TRIANGLE_FAN);
		}
		else
			glBegin (GL_TRIANGLE_STRIP);

		do
		{
			// texture coordinates come from the draw list
			u = ((float *)commands)[0];
			v = ((float *)commands)[1];

//			if (mtexenabled)
			{
				qglMultiTexCoord2f (GL_TEXTURE0_ARB, u, v);
				qglMultiTexCoord2f (GL_TEXTURE2_ARB, u, v);
			}
//			else
//			{
//				glTexCoord2f (u, v);
//			}

			commands += 2;

			// normals and vertexes come from the frame list
			// blend the light intensity from the two frames together
			if (shading)
			{
				// lit support
				if (r_fullbright.value || !cl.worldmodel->lightdata)
				{
					vertcolor[0] = vertcolor[1] = vertcolor[2] = 1.0;
				}
				else if (lerping)
				{
					vertcolor[0] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[0];
					vertcolor[1] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[1];
					vertcolor[2] = (shadedots[verts1->lightnormalindex]*iblend + shadedots[verts2->lightnormalindex]*blend) * lightcolor[2];
				}
				else
				{
					vertcolor[0] = shadedots[verts1->lightnormalindex] * lightcolor[0];
					vertcolor[1] = shadedots[verts1->lightnormalindex] * lightcolor[1];
					vertcolor[2] = shadedots[verts1->lightnormalindex] * lightcolor[2];
				}
				glColor4fv (vertcolor);
			}

			// blend the vertex positions from each frame together
			if (lerping)
			{
				glVertex3f (verts1->v[0]*iblend + verts2->v[0]*blend,
							verts1->v[1]*iblend + verts2->v[1]*blend,
							verts1->v[2]*iblend + verts2->v[2]*blend);
				verts1++;
				verts2++;
			}
			else
			{
				glVertex3f (verts1->v[0], verts1->v[1], verts1->v[2]);
				verts1++;
			}
		}
		while (--count);

		glEnd ();
	}

	// r_speeds
	rs_c_alias_passes += paliashdr->numtris;
}

/*
=================
R_SetupAliasFrame -- model animation interpolation (lerping)

set up animation part of lerpdata
=================
*/
void R_SetupAliasFrame (entity_t *e, aliashdr_t *paliashdr, lerpdata_t *lerpdata)
{
	int		frame = e->frame;
	int		posenum, numposes;
	static float	lastmsg = 0;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		if (IsTimeout (&lastmsg, 2))
		{
			Con_DPrintf ("R_SetupAliasFrame: no such frame ");
			// Single frame?
			if (paliashdr->frames[0].name[0])
				Con_DPrintf ("%d ('%s', %d frames)", frame, paliashdr->frames[0].name, paliashdr->numframes);
			else
				Con_DPrintf ("group %d (%d groups)", frame, paliashdr->numframes);
			Con_DPrintf (" in %s\n", e->model->name);
		}
		frame = 0;
	}

	posenum = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		int firstpose = posenum;

		posenum = numposes * e->syncbase; // Hack to make flames unsynchronized
		e->lerptime = paliashdr->frames[frame].interval;
		posenum += (int)(cl.time / e->lerptime) % numposes;
		posenum = firstpose + posenum % numposes;
	}
	else
	{
		e->lerptime = 0.1; // One tenth of a second is a good for most Quake animations.
	}

	if (e->lerpflags & LERP_RESETANIM) // kill any lerp in progress
	{
		e->lerpstart = 0;
		e->previouspose = posenum;
		e->currentpose = posenum;
		e->lerpflags -= LERP_RESETANIM;
	}
	else if (e->currentpose != posenum)  // pose changed, start new lerp
	{
		e->lerpstart = cl.time;
		e->previouspose = e->currentpose;
		e->currentpose = posenum;
	}

	// set up values
	// always lerp
	{
		if (e->lerpflags & LERP_FINISH && numposes == 1)
			lerpdata->blend = CLAMP (0, (cl.time - e->lerpstart) / (e->lerpfinish - e->lerpstart), 1);
		else
			lerpdata->blend = CLAMP (0, (cl.time - e->lerpstart) / e->lerptime, 1);
		lerpdata->pose1 = e->previouspose;
		lerpdata->pose2 = e->currentpose;
	}
	// don't lerp
/*	{
		lerpdata->blend = 1;
		lerpdata->pose1 = posenum;
		lerpdata->pose2 = posenum;
	}	*/
}

/*
=============
R_SetupEntityTransform -- model transform interpolation

set up transform part of lerpdata
=============
*/
void R_SetupEntityTransform (entity_t *e, lerpdata_t *lerpdata)
{
	float	blend;
	vec3_t	d;
	int	i;

	if (e->lerpflags & LERP_RESETMOVE) // kill any lerps in progress
	{
		e->movelerpstart = 0;
		VectorCopy (e->origin, e->previousorigin);
		VectorCopy (e->origin, e->currentorigin);
		VectorCopy (e->angles, e->previousangles);
		VectorCopy (e->angles, e->currentangles);
		e->lerpflags -= LERP_RESETMOVE;
	}
	else if (!VectorCompare (e->origin, e->currentorigin) || !VectorCompare (e->angles, e->currentangles)) // origin/angles changed, start new lerp
	{
		e->movelerpstart = cl.time;
		VectorCopy (e->currentorigin, e->previousorigin);
		VectorCopy (e->origin,  e->currentorigin);
		VectorCopy (e->currentangles, e->previousangles);
		VectorCopy (e->angles,  e->currentangles);
	}

	// set up values
	if (e != &cl.viewent && e->lerpflags & LERP_MOVESTEP)
	{
		if (e->lerpflags & LERP_FINISH)
			blend = CLAMP (0, (cl.time - e->movelerpstart) / (e->lerpfinish - e->movelerpstart), 1);
		else
			blend = CLAMP (0, (cl.time - e->movelerpstart) / 0.1, 1);

		// positional interpolation (translation)
		VectorSubtract (e->currentorigin, e->previousorigin, d);
		lerpdata->origin[0] = e->previousorigin[0] + d[0] * blend;
		lerpdata->origin[1] = e->previousorigin[1] + d[1] * blend;
		lerpdata->origin[2] = e->previousorigin[2] + d[2] * blend;

		// orientation interpolation (rotation)
		VectorSubtract (e->currentangles, e->previousangles, d);

		// always interpolate along the shortest path
		for (i = 0; i < 3; i++)
		{
			if (d[i] > 180)
				d[i] -= 360;
			else if (d[i] < -180)
				d[i] += 360;
		}
		lerpdata->angles[0] = e->previousangles[0] + d[0] * blend;
		lerpdata->angles[1] = e->previousangles[1] + d[1] * blend;
		lerpdata->angles[2] = e->previousangles[2] + d[2] * blend;
	}
	else // don't lerp
	{
		VectorCopy (e->origin, lerpdata->origin);
		VectorCopy (e->angles, lerpdata->angles);
	}
}
