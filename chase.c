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
// chase.c -- chase camera code

#include "quakedef.h"

cvar_t	chase_back = {"chase_back", "100", CVAR_NONE};
cvar_t	chase_up = {"chase_up", "16", CVAR_NONE};
cvar_t	chase_right = {"chase_right", "0", CVAR_NONE};
cvar_t	chase_active = {"chase_active", "0", CVAR_NONE};

void Chase_Init (void)
{
	Cvar_RegisterVariable (&chase_back);
	Cvar_RegisterVariable (&chase_up);
	Cvar_RegisterVariable (&chase_right);
	Cvar_RegisterVariable (&chase_active);
}

void Chase_Reset (void)
{
	// for respawning and teleporting
//	start position 12 units behind head
}

void TraceLine (vec3_t start, vec3_t end, vec3_t impact)
{
	trace_t	trace;
	qboolean    result;

	memset (&trace, 0, sizeof(trace));
	result = SV_RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, start, end, &trace);

	// If result, don't use trace.endpos, otherwise view might suddenly point straight down
	VectorCopy ((result ? end : trace.endpos), impact);
}

void Chase_Update (void)
{
	int		i;
	float	dist;
	vec3_t	forward, up, right;
	vec3_t	dest, stop;
	vec3_t	chase_dest;

	// if can't see player, reset
	AngleVectors (cl.viewangles, forward, right, up);

	// calc exact destination
	for (i=0 ; i<3 ; i++)
		chase_dest[i] = r_refdef.vieworg[i]
 		- forward[i]*chase_back.value
 		- right[i]*chase_right.value;
	chase_dest[2] = r_refdef.vieworg[2] + chase_up.value;

	// find the spot the player is looking at
	VectorMA (r_refdef.vieworg, 4096, forward, dest);
	TraceLine (r_refdef.vieworg, dest, stop);

	// calculate pitch to look at the same spot from camera
	VectorSubtract (stop, r_refdef.vieworg, stop);

	dist = DotProduct (stop, forward);
	if (dist < 1)
		dist = 1;

	r_refdef.viewangles[PITCH] = -atan(stop[2] / dist) / M_PI * 180;

	// chase_active 1 : keep view inside map except in noclip
	// chase_active 2 : maintain chase_back distance even if view is outside map
	if (chase_active.value == 1 && !(sv_player && sv_player->v.movetype == MOVETYPE_NOCLIP))
	{
		// Make sure view is inside map
		TraceLine (r_refdef.vieworg, chase_dest, stop);
		if (VectorLength(stop) != 0)
		{
			// update the camera destination to where we hit the wall
			VectorCopy (stop, chase_dest);

			// this prevents the camera from poking into the wall by rounding off the traceline...
			LerpVector (r_refdef.vieworg, chase_dest, 0.8f, chase_dest);
		}
	}

	// move towards destination
	VectorCopy (chase_dest, r_refdef.vieworg);
}

