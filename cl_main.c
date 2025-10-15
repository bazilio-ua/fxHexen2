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
// cl_main.c  -- client main loop

#include "quakedef.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

// these two are not intended to be set directly
cvar_t	cl_name = {"_cl_name", "player", CVAR_ARCHIVE};
cvar_t	cl_color = {"_cl_color", "0", CVAR_ARCHIVE};
cvar_t	cl_playerclass = {"_cl_playerclass", "1", CVAR_ARCHIVE};

cvar_t	cl_shownet = {"cl_shownet","0", CVAR_NONE};	// can be 0, 1, or 2
cvar_t	cl_nolerp = {"cl_nolerp","0", CVAR_NONE};
cvar_t	cl_lerpmuzzleflash = {"cl_lerpmuzzleflash","0", CVAR_NONE};

cvar_t	cl_coloredlight = {"cl_coloredlight","0", CVAR_ARCHIVE};
cvar_t	cl_extradlight = {"cl_extradlight","0", CVAR_ARCHIVE};
cvar_t	cl_extradlightstatic = {"cl_extradlightstatic","0", CVAR_ARCHIVE};

cvar_t	lookspring = {"lookspring","0", CVAR_ARCHIVE};
cvar_t	lookstrafe = {"lookstrafe","0", CVAR_ARCHIVE};
cvar_t	sensitivity = {"sensitivity","3", CVAR_ARCHIVE};
//static float save_sensitivity;

cvar_t	m_pitch = {"m_pitch","0.022", CVAR_ARCHIVE};
cvar_t	m_yaw = {"m_yaw","0.022", CVAR_ARCHIVE};
cvar_t	m_forward = {"m_forward","1", CVAR_ARCHIVE};
cvar_t	m_side = {"m_side","0.8", CVAR_ARCHIVE};


client_static_t	cls;
client_state_t	cl;
// FIXME: put these on hunk?
efrag_t			cl_efrags[MAX_EFRAGS];
entity_t		cl_entities[MAX_EDICTS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

int				cl_numvisedicts;
entity_t		*cl_visedicts[MAX_VISEDICTS];

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	int			i;

	if (!sv.active)
		Host_ClearMemory ();

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));

	// If disconnect was missing, stop sounds here
	if (cls.state == ca_connected)
		S_StopAllSounds (true);

	SZ_Clear (&cls.message);

// clear other arrays	
	memset (cl_efrags, 0, sizeof(cl_efrags));
	memset (cl_entities, 0, sizeof(cl_entities));
	memset (cl_dlights, 0, sizeof(cl_dlights));
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));
	CL_ClearTEnts();
	CL_ClearEffects();

//
// allocate the efrags and chain together into a free list
//
	cl.free_efrags = cl_efrags;
	for (i=0 ; i<MAX_EFRAGS-1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];
	cl.free_efrags[i].entnext = NULL;

	cl.current_frame = cl.current_sequence = 99;
	cl.reference_frame = cl.last_frame = cl.last_sequence = 199;
	cl.need_build = 2;

	plaquemessage = "";

	Sbar_InvReset();
}

void CL_RemoveGIPFiles (char *path)
{
	char	*name;
	char	tempdir[MAX_OSPATH];
	char	*p;
	size_t	len;

	if (path)
	{
		sprintf(tempdir,"%s",path);
	}
	else
	{
		sprintf(tempdir,"%s",com_gamedir);
	}

	len = strlen(tempdir);
	p = tempdir + len;
	len = sizeof(tempdir) - len;

	name = Sys_FindFirstFile (tempdir, "*.gip");
	while (name)
	{
		snprintf (p, len, "/%s", name);
		Sys_unlink (tempdir);
		*p = '\0';
		name = Sys_FindNextFile();
	}

	Sys_FindClose();
}

qboolean CL_CopyFiles(char *source, char *pat, char *dest)
{
	char	*name;
	char	tempdir[MAX_OSPATH], tempdir2[MAX_OSPATH];
	qboolean	error;

	name = Sys_FindFirstFile(source, pat);
	error = false;

	while (name)
	{
		if (snprintf(tempdir, sizeof(tempdir),"%s/%s", source, name) >= (int)sizeof(tempdir) ||
			snprintf(tempdir2, sizeof(tempdir2),"%s/%s", dest, name) >= (int)sizeof(tempdir2))
		{
			Sys_FindClose();
			Host_Error("CL_CopyFiles: string buffer overflow!");
			return true;
		}

		COM_CopyFile (tempdir, tempdir2);

		name = Sys_FindNextFile();
	}

	Sys_FindClose();

	return error;
}

/*
=====================
CL_Reconnect

Wait for the signon messages again.
=====================
*/
void CL_Reconnect (void)
{
	SCR_BeginLoadingPlaque ();
	cls.signon = 0;		// need new connection messages
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
// stop sounds (especially looping!)
	S_StopAllSounds (true);

	CDAudio_Stop(); // Stop the CD music

	R_ClearParticles ();	//jfm: need to clear parts because some now check world
	loading_stage = 0;

// if running a local server, shut it down
	cl.worldmodel = NULL; // This makes sure ambient sounds remain silent

// stop all intermissions 
// (the intermission screen prints the map name so this is important for preventing a crash)
	cl.intermission = 0; // Baker: So critical.  SCR_UpdateScreen uses this.

// remove all center prints
	con_lastcenterstring[0] = 0;
	scr_centerstring[0] = 0;
	scr_centertime_off = 0;

	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (cls.state == ca_connected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		Con_DPrintf ("Sending clc_disconnect\n");
		SZ_Clear (&cls.message);
		MSG_WriteByte (&cls.message, clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &cls.message);
		SZ_Clear (&cls.message);
		NET_Close (cls.netcon);

		cls.state = ca_disconnected;
		if (sv.active)
			Host_ShutdownServer(false);

		CL_RemoveGIPFiles(NULL);
	}

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;

// If we failed to load the requested level, we don't stuck with loading plaque...
    SCR_EndLoadingPlaque ();
}

/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (char *host)
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demoplayback)
		return;

	CL_Disconnect ();

	cls.netcon = NET_Connect (host);
	if (!cls.netcon)
		Host_Error ("CL_EstablishConnection: connect failed");

	Con_DPrintf ("Connected to server %s\n", host);
	
	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing

	MSG_WriteByte (&cls.message, clc_nop); // ProQuake NAT fix
}

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply (void)
{
	Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "prespawn");
		break;
		
	case 2:		
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("name \"%s\"\n", cl_name.string));
	
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("playerclass %i\n", (int)cl_playerclass.value));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("color %i %i\n", ((int)cl_color.value)>>4, ((int)cl_color.value)&15));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("spawn %s", cls.spawnparms));
		break;
		
	case 3:	
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "begin");
		Cache_Report ();		// print remaining memory
		break;
		
	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f (void)
{
	entity_t	*ent;
	int			i;
	
	for (i=0,ent=cl_entities ; i<cl.num_entities ; i++,ent++)
	{
		Con_Printf ("%3i:",i);
		if (!ent->model)
		{
			Con_Printf ("EMPTY\n");
			continue;
		}
		Con_Printf ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n"
		,ent->model->name,ent->frame, ent->origin[0], ent->origin[1], ent->origin[2], ent->angles[0], ent->angles[1], ent->angles[2]);
	}
}


/*
===============
CL_AllocDlight

===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		for (i=0, dl = cl_dlights ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
			// reuse this light
				memset (dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

// then look for anything else
	for (i=0, dl = cl_dlights ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset (dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

// otherwise grab first dlight
	dl = &cl_dlights[0];
	memset (dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

void CL_WhiteDlight (dlight_t *dl)
{
	float average;
	
	// make average white/grey dlight value from all colors
	average = (dl->color[0] + dl->color[1] + dl->color[2]) / 3.0;
	dl->color[0] = average;
	dl->color[1] = average;
	dl->color[2] = average;
}

void CL_ColorDlight (dlight_t *dl, float r, float g, float b)
{
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;

	if (!cl_coloredlight.value)
		CL_WhiteDlight (dl);
}

void CL_ColorDlightPalette (dlight_t *dl, int i)
{
	byte	*rgb;
	
	rgb = (byte *)&d_8to24table[i];
	dl->color[0] = rgb[0] * (1.0 / 255.0);
	dl->color[1] = rgb[1] * (1.0 / 255.0);
	dl->color[2] = rgb[2] * (1.0 / 255.0);
	
	if (!cl_coloredlight.value)
		CL_WhiteDlight (dl);
}

void CL_ColorDlightPaletteLength (dlight_t *dl, int start, int length)
{
	int 	i;
	byte	*rgb;
	
	i = (start + (rand() % length));
	rgb = (byte *)&d_8to24table[i];
	dl->color[0] = rgb[0] * (1.0 / 255.0);
	dl->color[1] = rgb[1] * (1.0 / 255.0);
	dl->color[2] = rgb[2] * (1.0 / 255.0);
	
	if (!cl_coloredlight.value)
		CL_WhiteDlight (dl);
}

/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;
	float		time;
	
	time = cl.time - cl.oldtime;

	for (i=0, dl = cl_dlights ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;
		
		if (dl->radius > 0)
		{
			dl->radius -= time*dl->decay;
			if (dl->radius < 0)
				dl->radius = 0;
		}
		else
		{
			dl->radius += time*dl->decay;
			if (dl->radius > 0)
				dl->radius = 0;
		}
	}
}


/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float	CL_LerpPoint (void)
{
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];

	if (!f || cls.timedemo || (sv.active && !host_netinterval))
	{
		cl.time = cl.mtime[0];
		return 1;
	}

	if (f > 0.1)
	{	// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - 0.1;
		f = 0.1;
	}
	frac = (cl.time - cl.mtime[1]) / f;
	//Con_Printf ("frac: %f\n",frac);
	if (frac < 0)
	{
		if (frac < -0.01)
		{
			cl.time = cl.mtime[1];
			//Con_Printf ("low frac\n");
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			cl.time = cl.mtime[0];
			//Con_Printf ("high frac\n");
		}
		frac = 1;
	}

	//johnfitz -- better nolerp behavior
	if (cl_nolerp.value)
		return 1;
	//johnfitz

	return frac;
}

void CL_UpdateStatic (void)
{
	entity_t *ent;
	int		i;
	dlight_t	*dl;
	int		key;
	
	if (!cl_extradlightstatic.value)
		return;
	
	for (i=0,ent=cl_static_entities ; i<cl.num_statics ; i++,ent++)
	{
		if (!ent->model)
			continue;
		
		key = i + 1;
		
		if (!strcmp (ent->model->name, "models/cflmtrch.mdl") || // blackmarsh castle
			!strcmp (ent->model->name, "models/mflmtrch.mdl") || // meso
			!strcmp (ent->model->name, "models/eflmtrch.mdl") || // egypt
			!strcmp (ent->model->name, "models/rflmtrch.mdl"))   // romeric
		{
			dl = CL_AllocDlight (key);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 100;
			dl->die = cl.time + 0.1;
			
			CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME);
		}
		else if (!strcmp (ent->model->name, "models/flame1.mdl"))
		{
			dl = CL_AllocDlight (key);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 125;
			dl->die = cl.time + 0.1;
			
			CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME);
		}
		else if (!strcmp (ent->model->name, "models/flame2.mdl"))
		{
			dl = CL_AllocDlight (key);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 100;
			dl->die = cl.time + 0.1;
			
			CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME2);
		}
		else if (!strcmp (ent->model->name, "models/flame.mdl"))
		{
			dl = CL_AllocDlight (key);
			VectorCopy (ent->origin, dl->origin);
			dl->origin[2] += 12;
			dl->radius = 100;
			dl->die = cl.time + 0.1;
			
			CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME2);
		}
		else if (!strcmp (ent->model->name, "models/gemlight.mdl"))
		{
			dl = CL_AllocDlight (key);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 85;
			dl->die = cl.time + 0.1;
			
			CL_ColorDlightPalette (dl, DL_COLOR_175);
		}
		else if (!strcmp (ent->model->name, "models/lantern.mdl")) // portals
		{
			dl = CL_AllocDlight (key);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 100;
			dl->die = cl.time + 0.1;
			
			CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME);
		}
		else if (!strcmp (ent->model->name, "models/candle.mdl")) // portals
		{
			dl = CL_AllocDlight (key);
			VectorCopy (ent->origin, dl->origin);
			dl->origin[2] += 8;
			dl->radius = 55;
			dl->die = cl.time + 0.1;

			CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME);
		}
	}
}

/*
===============
CL_RelinkEntities
===============
*/
void CL_RelinkEntities (void)
{
	entity_t	*ent;
	int			i, j;
	float		frac, f, d;
	vec3_t		delta;
	float		objrotate;
	vec3_t		oldorg;
	dlight_t	*dl;
	static float	lastmsg = 0;
	int		key;

// determine partial update time
	frac = CL_LerpPoint ();

	cl_numvisedicts = 0;

//
// interpolate player info
//
	for (i=0 ; i<3 ; i++)
		cl.velocity[i] = cl.mvelocity[1][i] + frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);

//	if ((cls.demoplayback && !intro_playing) || (cl.last_angle_time > cl.time && !(in_attack.state & 3))) // JPG - check for last_angle_time for smooth chasecam!
	if ((cls.demoplayback && !intro_playing) || (cl.last_angle_time > cl.time)) // host time replaced
	{
	// interpolate the angles	
		for (j=0 ; j<3 ; j++)
		{
			d = cl.mviewangles[0][j] - cl.mviewangles[1][j];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;

			// JPG - I can't set cl.viewangles anymore since that messes up the demorecording.
			// So instead, I'll set lerpangles (new variable), and view.c/chase.c will use that instead.
			cl.lerpangles[j] = cl.mviewangles[1][j] + frac*d;
		}
	}
	else
		VectorCopy(cl.viewangles, cl.lerpangles);

	//objrotate = anglemod(100*cl.time); //q1
	//objrotate = anglemod(100*(cl.time+ent->origin[0]+ent->origin[1]));
	//objrotate = anglemod((ent->origin[0]+ent->origin[1])*0.8+(108*cl.time)); // from R_RotateForEntity2
	
// start on the entity after the world
	for (i=1,ent=cl_entities+1 ; i<cl.num_entities ; i++,ent++)
	{
		if (!ent->model)
		{	// empty slot
            
			// ericw -- efrags are only used for static entities in GLQuake
			// ent can't be static, so this is a no-op.
//			if (ent->forcelink)
//				R_RemoveEfrags (ent);	// just became empty
			continue;
		}

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0] && !(ent->baseline.flags & BE_ON))
		{
			ent->model = NULL;
			ent->lerpflags |= LERP_RESETMOVE|LERP_RESETANIM; //johnfitz -- next time this entity slot is reused, the lerp will need to be reset

			continue;
		}

		VectorCopy (ent->origin, oldorg);

		if (ent->forcelink || ent->msgtime != cl.mtime[0])
		{	// the entity was not updated in the last message
			// so move to the final spot
			VectorCopy (ent->msg_origins[0], ent->origin);
			VectorCopy (ent->msg_angles[0], ent->angles);
		}
		else
		{	// if the delta is large, assume a teleport and don't lerp
			f = frac;
			for (j=0 ; j<3 ; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];
				if (delta[j] > 100 || delta[j] < -100)
				{
					f = 1;		// assume a teleportation, not a motion
					ent->lerpflags |= LERP_RESETMOVE; //johnfitz -- don't lerp teleports
				}
			}

			//johnfitz -- don't cl_lerp entities that will be r_lerped
			if (ent->lerpflags & LERP_MOVESTEP)
				f = 1;

		// interpolate the origin and angles
			for (j=0 ; j<3 ; j++)
			{
				ent->origin[j] = ent->msg_origins[1][j] + f*delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;
				ent->angles[j] = ent->msg_angles[1][j] + f*d;
			}
		}

		key = i + cl.num_statics + 1;

		//objrotate = anglemod(100*cl.time);
		//objrotate = anglemod(100*(cl.time+ent->origin[0]+ent->origin[1]));
		objrotate = anglemod((ent->origin[0]+ent->origin[1])*0.8+(108*cl.time)); // from R_RotateForEntity2

// rotate binary objects locally
		if (ent->model->flags & EF_ROTATE)
			ent->angles[1] = objrotate;

		if (!strcmp (ent->model->name, "models/flame1.mdl")) // in the 'portals' mission pack, this is not a static model
		{
			if (cl_extradlight.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 125;
				dl->die = cl.time + 0.1;
				
				CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME);
			}
		}
		else if (!strcmp (ent->model->name, "models/flame2.mdl")) // in the 'portals' mission pack, this is not a static model
		{
			if (cl_extradlight.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 100;
				dl->die = cl.time + 0.1;
				
				CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME2);
			}
		}
		else if (!strcmp (ent->model->name, "models/newfire.mdl")) // in the 'portals' mission pack, this is not a static model
		{
			if (cl_extradlight.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 300;
				dl->die = cl.time + 0.1;
				
				CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME);
			}
		}
		else if (!strcmp (ent->model->name, "models/gemlight.mdl")) // in the 'portals' mission pack, this is not a static model
		{
			if (cl_extradlight.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 85;
				dl->die = cl.time + 0.1;
				
				CL_ColorDlightPalette (dl, DL_COLOR_175);
			}
		}
		else if (!strcmp (ent->model->name, "models/a_torch.mdl")) // torch powerup item
		{
			if (cl_extradlight.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->origin[2] += 16;
				dl->radius = 30;
				dl->die = cl.time + 0.1;
				
				CL_ColorDlightPalette (dl, DL_COLOR_254);
			}
		}

//		if (ent->effects & EF_BRIGHTFIELD);
//			R_EntityParticles (ent);

		if (ent->effects & EF_DARKFIELD)
			R_DarkFieldParticles (ent);

		if (ent->effects & EF_BRIGHTLIGHT)
		{			
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin,  dl->origin);
				dl->origin[2] += 16;
				dl->radius = 400 + (rand()&31);
				dl->die = cl.time + 0.001;
				
				
				if (i == cl.viewentity)
				{
					// white
				}
				else
				{
					
				}

				
				if (!strcmp (ent->model->name, "models/drgnball.mdl"))
					CL_ColorDlightPaletteLength (dl, DL_COLOR_FLAME);
				else
					CL_ColorDlightPalette (dl, DL_COLOR_255); // uncoloured (full white)
			}
		}
		if (ent->effects & EF_DIMLIGHT) // powerup(s) glows
		{
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = 200 + (rand()&31);
				dl->die = cl.time + 0.001;

				// powerup dynamic lights
				if (i == cl.viewentity)
				{
					CL_ColorDlightPalette (dl, DL_COLOR_29); // uncoloured (dim white)
				}
				else
				{
					CL_ColorDlightPalette (dl, DL_COLOR_29); // uncoloured (dim white)
				}

			}
		}
		if (ent->effects & EF_DARKLIGHT)
		{			
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = 200.0 + (rand()&31);
				dl->die = cl.time + 0.001;
				dl->dark = true;
			}
		}
		if (ent->effects & EF_LIGHT)
		{			
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = 200;
				dl->die = cl.time + 0.001;
				
				// orange
			}
		}
		if (ent->effects & EF_MUZZLEFLASH)
		{
			vec3_t		fv, rv, uv;
			
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin,  dl->origin);
				dl->origin[2] += 16;
				AngleVectors (ent->angles, fv, rv, uv);
				
				VectorMA (dl->origin, 18, fv, dl->origin);
//				dl->radius = 200 + (rand()&31);
				dl->radius = ((ent->effects & EF_DIMLIGHT) ? 300 : 200) + (rand()&31);
				dl->minlight = 32;
				dl->die = cl.time + 0.1;
				
				if (i == cl.viewentity)
				{
					// switch the flash colour for the current weapon
					switch ((int)cl.v.weapon)
					{
						case IT_WEAPON1:
							break;
						case IT_WEAPON2:
							if (!strcmp(ent->model->name, "models/necro.mdl"))
								CL_ColorDlightPaletteLength (dl, DL_COLOR_SPELL_M);
							break;
						case IT_WEAPON3:
							if (!strcmp(ent->model->name, "models/necro.mdl"))
								CL_ColorDlightPaletteLength (dl, DL_COLOR_SPELL);
							else if (!strcmp(ent->model->name, "models/crusader.mdl"))
								CL_ColorDlightPalette (dl, DL_COLOR_252);
							break;
						case IT_WEAPON4:
							break;
					}
				}
				else
				{
					// some entities have different attacks resulting in a different flash colour
					if (!strcmp (ent->model->name, "models/ball.mdl"))
						CL_ColorDlightPaletteLength (dl, DL_COLOR_BALL);
					else if (!strcmp (ent->model->name, "models/tornato.mdl"))
						CL_ColorDlightPaletteLength (dl, DL_COLOR_TORNATO);
					else
						CL_ColorDlightPalette (dl, DL_COLOR_15);
				}
			}
			
			//johnfitz -- assume muzzle flash accompanied by muzzle flare, which looks bad when lerped
			if (!cl_lerpmuzzleflash.value)
			{
				if (i == cl.viewentity)
					cl.viewent.lerpflags |= LERP_RESETANIM|LERP_RESETANIM2; // no lerping for two frames
				else
					ent->lerpflags |= LERP_RESETANIM|LERP_RESETANIM2; // no lerping for two frames
			}
		}

		if (ent->model->flags & EF_GIB)
			R_RocketTrail (oldorg, ent->origin, 2);
		else if (ent->model->flags & EF_ZOMGIB)
			R_RocketTrail (oldorg, ent->origin, 4);
		else if (ent->model->flags & EF_BLOODSHOT)
			R_RocketTrail (oldorg, ent->origin, 17);
		else if (ent->model->flags & EF_TRACER)
			R_RocketTrail (oldorg, ent->origin, 3);
		else if (ent->model->flags & EF_TRACER2)
			R_RocketTrail (oldorg, ent->origin, 5);
		else if (ent->model->flags & EF_ROCKET)
		{
			R_RocketTrail (oldorg, ent->origin, 0);
/*			dl = CL_AllocDlight (key);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 200;
			dl->die = cl.time + 0.01;*/
			
			// todo color
		}
		else if (ent->model->flags & EF_FIREBALL)
		{
			R_RocketTrail (oldorg, ent->origin, rt_fireball);
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 120 - (rand() % 20);
				dl->die = cl.time + 0.01;
				
				// red
			}
		}
		else if (ent->model->flags & EF_ACIDBALL)
		{
			R_RocketTrail (oldorg, ent->origin, rt_acidball);
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 120 - (rand() % 20);
				dl->die = cl.time + 0.01;
				
				// green
			}
		}
		else if (ent->model->flags & EF_ICE)
		{
			R_RocketTrail (oldorg, ent->origin, rt_ice);
		}
		else if (ent->model->flags & EF_SPIT)
		{
			R_RocketTrail (oldorg, ent->origin, rt_spit);
			if (cl_prettylights.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = -120 - (rand() % 20);
				dl->die = cl.time + 0.05;
				
				// green
			}
		}
		else if (ent->model->flags & EF_SPELL)
		{
			R_RocketTrail (oldorg, ent->origin, rt_spell);
		}
		else if (ent->model->flags & EF_GRENADE)
			R_RocketTrail (oldorg, ent->origin, 1);
		else if (ent->model->flags & EF_TRACER3)
			R_RocketTrail (oldorg, ent->origin, 6);
		else if (ent->model->flags & EF_VORP_MISSILE)
		{
			R_RocketTrail (oldorg, ent->origin, rt_vorpal);
			
			if (cl_extradlight.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 240 - (rand() % 20);
				dl->die = cl.time + 0.01;
				
				CL_ColorDlightPaletteLength (dl, DL_COLOR_V_SHOT);
			}
		}
		else if (ent->model->flags & EF_SET_STAFF)
		{
			R_RocketTrail (oldorg, ent->origin,rt_setstaff);
		}
		else if (ent->model->flags & EF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
				R_RocketTrail (oldorg, ent->origin, rt_magicmissile);
			
			if (cl_extradlight.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 240 - (rand() % 20);
				dl->die = cl.time + 0.01;
				
				CL_ColorDlightPaletteLength (dl, DL_COLOR_BALL);
			}
		}
		else if (ent->model->flags & EF_BONESHARD)
			R_RocketTrail (oldorg, ent->origin, rt_boneshard);
		else if (ent->model->flags & EF_SCARAB)
		{
			R_RocketTrail (oldorg, ent->origin, rt_scarab);
			
			if (cl_extradlight.value)
			{
				dl = CL_AllocDlight (key);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 240 - (rand() % 20);
				dl->die = cl.time + 0.01;
				
				// orange
			}
		}

		ent->forcelink = false;

		if (i == cl.viewentity && !chase_active.value)
			continue;

		if (ent->effects & EF_NODRAW)
			continue;

		if (cl_numvisedicts < MAX_VISEDICTS)
		{
			cl_visedicts[cl_numvisedicts] = ent;
			cl_numvisedicts++;
		}
		else if (IsTimeout (&lastmsg, 2))
			Con_DWarning ("CL_RelinkEntities: too many visedicts (max = %d)\n", MAX_VISEDICTS);
	}
}


/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer (void)
{
	int		ret;

	cl.oldtime = cl.time;
	cl.time += host_frametime;
	
	do
	{
		ret = CL_GetMessage ();
		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");
		if (!ret)
			break;
		
		cl.last_received_message = realtime;
		CL_ParseServerMessage ();
	} while (ret && cls.state == ca_connected);
	
	if (cl_shownet.value)
		Con_Printf ("\n");

	CL_UpdateStatic ();
	CL_RelinkEntities ();
	CL_UpdateTEnts ();

//
// bring the links up to date
//
	return 0;
}

/*
=================
CL_UpdateViewAngles

Spike: split from CL_SendCmd, to do clientside viewangle changes separately from outgoing packets.
=================
*/
void CL_AccumulateCmd (void)
{
	if (cls.signon == SIGNONS)
	{
	// basic keyboard looking
		CL_AdjustAngles ();

	// accumulate movement from other devices
		IN_Move (&cl.pendingcmd);
	}
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	usercmd_t		cmd;

	if (cls.state != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
	// get basic movement from keyboard
		CL_BaseMove (&cmd);
	
	// allow mice or other external controllers to add to the move
		cmd.forwardmove += cl.pendingcmd.forwardmove;
		cmd.sidemove += cl.pendingcmd.sidemove;
		cmd.upmove += cl.pendingcmd.upmove;
	
	// send the unreliable message
		CL_SendMove (&cmd);
	}
	else
		CL_SendMove (NULL);
	
	memset(&cl.pendingcmd, 0, sizeof(cl.pendingcmd));

	if (cls.demoplayback)
	{
		SZ_Clear (&cls.message);
		return;
	}
	
// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all
	
	if (!NET_CanSendMessage (cls.netcon))
	{
		Con_DPrintf ("CL_SendCmd: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.netcon, &cls.message) == -1)
		Host_Error ("CL_SendCmd: lost server connection");

	SZ_Clear (&cls.message);
}

//void CL_Sensitivity_save_f (void)
//{
//	if (Cmd_Argc() != 2)
//	{
//		Con_Printf ("sensitivity_save <save/restore>\n");
//		return;
//	}
//
//	if (strcasecmp(Cmd_Argv(1),"save") == 0)
//		save_sensitivity = sensitivity.value;
//	else if (strcasecmp(Cmd_Argv(1),"restore") == 0)
//		Cvar_SetValue ("sensitivity", save_sensitivity);
//}

/*
=============
CL_Tracepos_f

display impact point of trace along VPN
=============
*/
void CL_Tracepos_f (void)
{
	vec3_t	v, w;

	VectorScale(vpn, 8192.0, v);
	TraceLine(r_refdef.vieworg, v, w);

	if (VectorLength(w) == 0)
		Con_Printf ("Tracepos: trace didn't hit anything\n");
	else
		Con_Printf ("Tracepos: (%i %i %i)\n", (int)w[0], (int)w[1], (int)w[2]);
}

/*
=============
CL_Viewpos_f

Display client's position and mangle
=============
*/
void CL_Viewpos_f (void)
{
	// Player position
	Con_Printf ("Viewpos: (%i %i %i) %i %i %i\n",
		(int)cl_entities[cl.viewentity].origin[0],
		(int)cl_entities[cl.viewentity].origin[1],
		(int)cl_entities[cl.viewentity].origin[2],
		(int)cl.viewangles[PITCH],
		(int)cl.viewangles[YAW],
		(int)cl.viewangles[ROLL]);
		
	if (sv_player && sv_player->v.movetype == MOVETYPE_NOCLIP && Cmd_Argc() == 4)
	{
		// Major hack ...
		sv_player->v.origin[0] = atoi (Cmd_Argv(1));
		sv_player->v.origin[1] = atoi (Cmd_Argv(2));
		sv_player->v.origin[2] = atoi (Cmd_Argv(3));
	}
}

/*
=============
CL_StaticEnts_f
=============
*/
void CL_StaticEnts_f (void)
{
	Con_Printf ("%d static entities\n", cl.num_statics);
}

/*
=================
CL_Init
=================
*/
void CL_Init (void)
{	
	SZ_Alloc (&cls.message, 8192); //1024, possibly dependant on CMD_TEXTSIZE

	CL_InitInput ();
	CL_InitTEnts ();
	CL_InitEffects();

	
//
// register our commands
//
	Cvar_RegisterVariable (&cl_name);
	Cvar_RegisterVariable (&cl_color);
	Cvar_RegisterVariable (&cl_playerclass);
	Cvar_RegisterVariable (&cl_run);
	Cvar_RegisterVariable (&cl_upspeed);
	Cvar_RegisterVariable (&cl_forwardspeed);
	Cvar_RegisterVariable (&cl_sidespeed);
	Cvar_RegisterVariable (&cl_backspeed); // keep for compatibility
	Cvar_RegisterVariable (&cl_movespeedkey);
	Cvar_RegisterVariable (&cl_yawspeed);
	Cvar_RegisterVariable (&cl_pitchspeed);
	Cvar_RegisterVariable (&cl_maxpitch); // variable pitch clamping
	Cvar_RegisterVariable (&cl_minpitch); // variable pitch clamping
	Cvar_RegisterVariable (&cl_anglespeedkey);
	Cvar_RegisterVariable (&cl_shownet);
	Cvar_RegisterVariable (&cl_nolerp);
	Cvar_RegisterVariable (&cl_lerpmuzzleflash);

	Cvar_RegisterVariable (&cl_coloredlight);
	Cvar_RegisterVariable (&cl_extradlight);
	Cvar_RegisterVariable (&cl_extradlightstatic);

	Cvar_RegisterVariable (&lookspring);
	Cvar_RegisterVariable (&lookstrafe);
	Cvar_RegisterVariable (&sensitivity);

	Cvar_RegisterVariable (&m_pitch);
	Cvar_RegisterVariable (&m_yaw);
	Cvar_RegisterVariable (&m_forward);
	Cvar_RegisterVariable (&m_side);
	Cvar_RegisterVariable (&cl_prettylights);

	
	Cmd_AddCommand ("entities", CL_PrintEntities_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
//	Cmd_AddCommand ("sensitivity_save", CL_Sensitivity_save_f);
	Cmd_AddCommand ("tracepos", CL_Tracepos_f); // fitz
	Cmd_AddCommand ("viewpos", CL_Viewpos_f);
	Cmd_AddCommand ("staticents", CL_StaticEnts_f);
}

