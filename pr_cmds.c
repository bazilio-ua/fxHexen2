
/*
 * $Header: /H2 Mission Pack/Pr_cmds.c 26    3/23/98 7:24p Jmonroe $
 */

#include "quakedef.h"

#define	RETURN_EDICT(e) (((int *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))

extern unsigned int info_mask, info_mask2;

extern int	*pr_info_string_index;
extern char	*pr_global_info_strings;
extern int	 pr_info_string_count;

/*
===============================================================================

						BUILT-IN FUNCTIONS

===============================================================================
*/

char *PF_VarString (int	first)
{
	int		i;
	static char out[256];
	
	out[0] = 0;
	for (i=first ; i<pr_argc ; i++)
	{
		strcat (out, G_STRING((OFS_PARM0+i*3)));
	}
	return out;
}


/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/
void PF_error (void)
{
	char	*s;
	edict_t	*ed;
	
	s = PF_VarString(0);
	Con_Printf ("======SERVER ERROR in %s:\n%s\n"
	,pr_strings + pr_xfunction->s_name,s);
	ed = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));
	ED_Print (ed);

	Host_Error ("Program error");
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void PF_objerror (void)
{
	char	*s;
	edict_t	*ed;
	
	s = PF_VarString(0);
	Con_Printf ("======OBJECT ERROR in %s:\n%s\n"
	,pr_strings + pr_xfunction->s_name,s);
	ed = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));
	ED_Print (ed);
	ED_Free (ed);
	
	Host_Error ("Program error");
}



/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors (void)
{
	AngleVectors (G_VECTOR(OFS_PARM0), PR_GLOBAL_STRUCT(v_forward), PR_GLOBAL_STRUCT(v_right), PR_GLOBAL_STRUCT(v_up));
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void PF_setorigin (void)
{
	edict_t	*e;
	float	*org;
	
	e = G_EDICT(OFS_PARM0);
	org = G_VECTOR(OFS_PARM1);
	VectorCopy (org, e->v.origin);
	SV_LinkEdict (e, false);
}


void SetMinMaxSize (edict_t *e, float *min, float *max, qboolean rotate)
{
	float	*angles;
	vec3_t	rmin, rmax;
	float	bounds[2][3];
	float	xvector[2], yvector[2];
	float	a;
	vec3_t	base, transformed;
	int		i, j, k, l;
	
	for (i=0 ; i<3 ; i++)
		if (min[i] > max[i])
			PR_RunError ("backwards mins/maxs");

	rotate = false;		// FIXME: implement rotation properly again

	if (!rotate)
	{
		VectorCopy (min, rmin);
		VectorCopy (max, rmax);
	}
	else
	{
	// find min / max for rotations
		angles = e->v.angles;
		
		a = angles[1]/180 * M_PI;
		
		xvector[0] = cos(a);
		xvector[1] = sin(a);
		yvector[0] = -sin(a);
		yvector[1] = cos(a);
		
		VectorCopy (min, bounds[0]);
		VectorCopy (max, bounds[1]);
		
		rmin[0] = rmin[1] = rmin[2] = 9999;
		rmax[0] = rmax[1] = rmax[2] = -9999;
		
		for (i=0 ; i<= 1 ; i++)
		{
			base[0] = bounds[i][0];
			for (j=0 ; j<= 1 ; j++)
			{
				base[1] = bounds[j][1];
				for (k=0 ; k<= 1 ; k++)
				{
					base[2] = bounds[k][2];
					
				// transform the point
					transformed[0] = xvector[0]*base[0] + yvector[0]*base[1];
					transformed[1] = xvector[1]*base[0] + yvector[1]*base[1];
					transformed[2] = base[2];
					
					for (l=0 ; l<3 ; l++)
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
	}
	
// set derived values
	VectorCopy (rmin, e->v.mins);
	VectorCopy (rmax, e->v.maxs);
	VectorSubtract (max, min, e->v.size);
	
	SV_LinkEdict (e, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF_setsize (void)
{
	edict_t	*e;
	float	*min, *max;
	
	e = G_EDICT(OFS_PARM0);
	min = G_VECTOR(OFS_PARM1);
	max = G_VECTOR(OFS_PARM2);
	SetMinMaxSize (e, min, max, false);
}


/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
void PF_setmodel (void)
{
	edict_t	*e;
	char	*m, **check;
	model_t	*mod;
	int		i;

	e = G_EDICT(OFS_PARM0);
	m = G_STRING(OFS_PARM1);

// check to see if model was properly precached
	for (i=0, check = sv.model_precache ; *check ; i++, check++)
		if (!strcmp(*check, m))
			break;
			
	if (!*check)
		PR_RunError ("no precache: %s\n", m);
		

	e->v.model = m - pr_strings;
	e->v.modelindex = i; //SV_ModelIndex (m);

	mod = sv.models[ (int)e->v.modelindex];  // Mod_ForName (m, true);
	
	if (mod)
		SetMinMaxSize (e, mod->mins, mod->maxs, true);
	else
		SetMinMaxSize (e, vec3_origin, vec3_origin, true);
}

void PF_setpuzzlemodel (void)
{
	edict_t	*e;
	char	*m, **check;
	model_t	*mod;
	int		i;
	char	NewName[256];

	e = G_EDICT(OFS_PARM0);
	m = G_STRING(OFS_PARM1);

	sprintf(NewName,"models/puzzle/%s.mdl",m);
// check to see if model was properly precached
	for (i=0, check = sv.model_precache ; *check ; i++, check++)
		if (!strcmp(*check, NewName))
			break;
			
	e->v.model = ED_NewString (NewName) - pr_strings;

	if (!*check)
	{
//		PR_RunError ("no precache: %s\n", NewName);
		Con_Printf("**** NO PRECACHE FOR PUZZLE PIECE:");
		Con_Printf("**** %s\n",NewName);

		sv.model_precache[i] = e->v.model + pr_strings;
		sv.models[i] = Mod_ForName (NewName, true);
	}
		
	e->v.modelindex = i; //SV_ModelIndex (m);

	mod = sv.models[ (int)e->v.modelindex];  // Mod_ForName (m, true);
	
	if (mod)
		SetMinMaxSize (e, mod->mins, mod->maxs, true);
	else
		SetMinMaxSize (e, vec3_origin, vec3_origin, true);
}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint (void)
{
	char		*s;

	s = PF_VarString(0);
	SV_BroadcastPrintf ("%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF_sprint (void)
{
	char		*s;
	client_t	*client;
	int			entnum;
	
	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}
		
	client = &svs.clients[entnum-1];
		
	MSG_WriteChar (&client->message,svc_print);
	MSG_WriteString (&client->message, s );
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF_centerprint (void)
{
	char		*s;
	client_t	*client;
	int			entnum;
	
	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}
		
	client = &svs.clients[entnum-1];
		
	MSG_WriteChar (&client->message,svc_centerprint);
	MSG_WriteString (&client->message, s );
}


/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize (void)
{
	float	*value1;
	vec3_t	newvalue;
	float	new;
	
	value1 = G_VECTOR(OFS_PARM0);

	new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	new = sqrt(new);
	
	if (new == 0)
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	else
	{
		new = 1/new;
		newvalue[0] = value1[0] * new;
		newvalue[1] = value1[1] * new;
		newvalue[2] = value1[2] * new;
	}
	
	VectorCopy (newvalue, G_VECTOR(OFS_RETURN));	
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
void PF_vlen (void)
{
	float	*value1;
	float	new;
	
	value1 = G_VECTOR(OFS_PARM0);

	new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	new = sqrt(new);
	
	G_FLOAT(OFS_RETURN) = new;
}

/*
=================
PF_vhlen

scalar vhlen(vector)
=================
*/
void PF_vhlen (void)
{
	float	*value1;
	float	new;
	
	value1 = G_VECTOR(OFS_PARM0);

	new = value1[0] * value1[0] + value1[1] * value1[1];
	new = sqrt(new);
	
	G_FLOAT(OFS_RETURN) = new;
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw (void)
{
	float	*value1;
	float	yaw;
	
	value1 = G_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	G_FLOAT(OFS_RETURN) = yaw;
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
qboolean stupidquakebugfix = false; 
cvar_t	sv_stupidquakebugfix = {"sv_stupidquakebugfix", "0", CVAR_NONE};

void PF_vectoangles (void)
{
	float	*value1;
	float	forward;
	float	yaw, pitch;
	
	value1 = G_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = stupidquakebugfix ? 270 : 90;
		else
			pitch = stupidquakebugfix ? 90 : 270;
	}
	else
	{
		yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		forward = sqrt(value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (atan2(stupidquakebugfix ? -value1[2] : value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	G_FLOAT(OFS_RETURN+0) = pitch;
	G_FLOAT(OFS_RETURN+1) = yaw;
	G_FLOAT(OFS_RETURN+2) = 0;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void PF_random (void)
{
	float		num;
		
	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
	
	G_FLOAT(OFS_RETURN) = num;
}

/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void PF_particle (void)
{
	float		*org, *dir;
	float		color;
	float		count;
			
	org = G_VECTOR(OFS_PARM0);
	dir = G_VECTOR(OFS_PARM1);
	color = G_FLOAT(OFS_PARM2);
	count = G_FLOAT(OFS_PARM3);
	SV_StartParticle (org, dir, color, count);
}


/*
=================
PF_particle2

particle2(origin, dmin, dmax, color, effect, count)
=================
*/
void PF_particle2 (void)
{
	float		*org, *dmin, *dmax;
	float		color;
	float		count;
	float    effect;

	org = G_VECTOR(OFS_PARM0);
	dmin = G_VECTOR(OFS_PARM1);
	dmax = G_VECTOR(OFS_PARM2);
	color = G_FLOAT(OFS_PARM3);
	effect = G_FLOAT(OFS_PARM4);
	count = G_FLOAT(OFS_PARM5);
	SV_StartParticle2 (org, dmin, dmax, color, effect, count);
}


/*
=================
PF_particle3

particle3(origin, box, color, effect, count)
=================
*/
void PF_particle3 (void)
{
	float		*org, *box;
	float		color;
	float		count;
	float    effect;

	org = G_VECTOR(OFS_PARM0);
	box = G_VECTOR(OFS_PARM1);
	color = G_FLOAT(OFS_PARM2);
	effect = G_FLOAT(OFS_PARM3);
	count = G_FLOAT(OFS_PARM4);
	SV_StartParticle3 (org, box, color, effect, count);
}

/*
=================
PF_particle4

particle4(origin, radius, color, effect, count)
=================
*/
void PF_particle4 (void)
{
	float		*org;
	float		radius;
	float		color;
	float		count;
	float    effect;

	org = G_VECTOR(OFS_PARM0);
	radius = G_FLOAT(OFS_PARM1);
	color = G_FLOAT(OFS_PARM2);
	effect = G_FLOAT(OFS_PARM3);
	count = G_FLOAT(OFS_PARM4);
	SV_StartParticle4 (org, radius, color, effect, count);
}


/*
=================
PF_ambientsound

=================
*/
void PF_ambientsound (void)
{
	char		**check;
	char		*samp;
	float		*pos;
	float 		vol, attenuation;
	int			i, soundnum;
	int		SOUNDS_MAX; /* just to be on the safe side */

	pos = G_VECTOR (OFS_PARM0);			
	samp = G_STRING(OFS_PARM1);
	vol = G_FLOAT(OFS_PARM2);
	attenuation = G_FLOAT(OFS_PARM3);
	
// check to see if samp was properly precached
	SOUNDS_MAX = (sv.Protocol == PROTOCOL_RAVEN_111) ? MAX_SOUNDS_OLD : MAX_SOUNDS;
	for (soundnum=0, check = sv.sound_precache ; 
		soundnum < SOUNDS_MAX && *check ; soundnum++, check++)
	{
		if (!strcmp(*check,samp))
			break;
	}
		
	if (soundnum == SOUNDS_MAX || !*check)
	{
		Con_Printf ("no precache: %s\n", samp);
		return;
	}

// add an svc_spawnambient command to the level signon packet

	MSG_WriteByte (&sv.signon,svc_spawnstaticsound);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord(&sv.signon, pos[i]);

	if (sv.Protocol == PROTOCOL_RAVEN_111)
		MSG_WriteByte (&sv.signon, soundnum);
	else
		MSG_WriteShort (&sv.signon, soundnum);

	MSG_WriteByte (&sv.signon, vol*255);
	MSG_WriteByte (&sv.signon, attenuation*64);
}

/*
=================
PF_StopSound
	stop ent's sound on this chan
=================
*/
void PF_StopSound(void)
{
	int			channel;
	edict_t		*entity;
		
	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);
	
	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	SV_StopSound (entity, channel);
}

/*
=================
PF_UpdateSoundPos
	sends cur pos to client to update this ent/chan pair
=================
*/
void PF_UpdateSoundPos(void)
{
	int			channel;
	edict_t		*entity;
		
	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);
	
	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	SV_UpdateSoundPos (entity, channel);
}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
already running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound (void)
{
	char		*sample;
	int			channel;
	edict_t		*entity;
	int 		volume;
	float attenuation;
		
	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);
	sample = G_STRING(OFS_PARM2);
	volume = G_FLOAT(OFS_PARM3) * 255;
	attenuation = G_FLOAT(OFS_PARM4);
	
	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	SV_StartSound (entity, channel, sample, volume, attenuation);
}

/*
=================
PF_break

break()
=================
*/
void PF_break (void)
{
	static qboolean DidIt = false;

	if (!DidIt)
	{	
		DidIt = true;

		Con_Printf ("break statement\n");

//		DebugBreak();//WIN32

		//*(int *)-4 = 0;	// dump to debugger
	}
//	PR_RunError ("break statement");
}

void PR_SetTrace (trace_t trace)
{
	if (is_progdefs111)
	{
		pr_global_struct_v111->trace_allsolid = trace.allsolid;
		pr_global_struct_v111->trace_startsolid = trace.startsolid;
		pr_global_struct_v111->trace_fraction = trace.fraction;
		pr_global_struct_v111->trace_inwater = trace.inwater;
		pr_global_struct_v111->trace_inopen = trace.inopen;
		VectorCopy (trace.endpos, pr_global_struct_v111->trace_endpos);
		VectorCopy (trace.plane.normal, pr_global_struct_v111->trace_plane_normal);
		pr_global_struct_v111->trace_plane_dist =  trace.plane.dist;
		if (trace.ent)
			pr_global_struct_v111->trace_ent = EDICT_TO_PROG(trace.ent);
		else
			pr_global_struct_v111->trace_ent = EDICT_TO_PROG(sv.edicts);

		return;
	}

	pr_global_struct->trace_allsolid = trace.allsolid;
	pr_global_struct->trace_startsolid = trace.startsolid;
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = trace.inwater;
	pr_global_struct->trace_inopen = trace.inopen;
	VectorCopy (trace.endpos, pr_global_struct->trace_endpos);
	VectorCopy (trace.plane.normal, pr_global_struct->trace_plane_normal);
	pr_global_struct->trace_plane_dist =  trace.plane.dist;
	if (trace.ent)
		pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
	else
		pr_global_struct->trace_ent = EDICT_TO_PROG(sv.edicts);
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF_traceline (void)
{
	float	*v1, *v2;
	trace_t	trace;
	int		nomonsters;
	edict_t	*ent;
	float save_hull;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	nomonsters = G_FLOAT(OFS_PARM2);
	ent = G_EDICT(OFS_PARM3);

	save_hull = ent->v.hull;
	ent->v.hull = 0;
	trace = SV_Move (v1, vec3_origin, vec3_origin, v2, nomonsters, ent);
	ent->v.hull = save_hull;

	PR_SetTrace (trace);
}

/*
=================
PF_tracearea

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

tracearea (vector1, vector2, mins, maxs, tryents)
=================
*/
void PF_tracearea (void)
{
	float	*v1, *v2, *mins, *maxs;
	trace_t	trace;
	int		nomonsters;
	edict_t	*ent;
	float save_hull;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	mins = G_VECTOR(OFS_PARM2);
	maxs = G_VECTOR(OFS_PARM3);
	nomonsters = G_FLOAT(OFS_PARM4);
	ent = G_EDICT(OFS_PARM5);

	save_hull = ent->v.hull;
	ent->v.hull = 0;
	trace = SV_Move (v1, mins, maxs, v2, nomonsters, ent);
	ent->v.hull = save_hull;

	PR_SetTrace (trace);
}



struct PointInfo_t
{
   char Found,NumFound,MarkedWhen;
   struct PointInfo_t *FromPos, *Next;
};

#define MAX_POINT_X 21
#define MAX_POINT_Y 21
#define MAX_POINT_Z 11
#define MAX_POINT (MAX_POINT_X * MAX_POINT_Y * MAX_POINT_Z)

#define POINT_POS(x,y,z) ((z*ZOffset)+(y*YOffset)+(x))

#define POINT_X_SIZE 160
#define POINT_Y_SIZE 160
#define POINT_Z_SIZE 50

#define POINT_MAX_DEPTH 5

struct PointInfo_t PI[MAX_POINT];
int ZOffset,YOffset;

extern particle_t	*active_particles, *free_particles;

void AddParticle(float *Org, float color)
{
	particle_t	*p;

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;

	p->die = 99999;
	p->color = color;
	p->type = pt_static;
	VectorCopy (vec3_origin, p->vel);
	VectorCopy (Org, p->org);
}


/*
=================
PF_checkpos

Returns true if the given entity can move to the given position from it's
current position by walking or rolling.
FIXME: make work...
scalar checkpos (entity, vector)
=================
*/
void PF_checkpos (void)
{
}

//============================================================================

byte	checkpvs[MAX_MAP_LEAFS/8];

int PF_newcheckclient (int check)
{
	int		i;
	byte	*pvs;
	edict_t	*ent;
	mleaf_t	*leaf;
	vec3_t	org;

// cycle to the next one

	if (check < 1)
		check = 1;
	if (check > svs.maxclients)
		check = svs.maxclients;

	if (check == svs.maxclients)
		i = 1;
	else
		i = check + 1;

	for ( ;  ; i++)
	{
		if (i == svs.maxclients+1)
			i = 1;

		ent = EDICT_NUM(i);

		if (i == check)
			break;	// didn't find anything else

		if (ent->free)
			continue;
		if (ent->v.health <= 0)
			continue;
		if ((int)ent->v.flags & FL_NOTARGET)
			continue;

	// anything that is a client, or has a client as an enemy
		break;
	}

// get the PVS for the entity
	VectorAdd (ent->v.origin, ent->v.view_ofs, org);
	leaf = Mod_PointInLeaf (org, sv.worldmodel);
	pvs = Mod_LeafPVS (leaf, sv.worldmodel);
	memcpy (checkpvs, pvs, (sv.worldmodel->numleafs+7)>>3 );

	return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define	MAX_CHECK	16
int c_invis, c_notvis;
void PF_checkclient (void)
{
	edict_t	*ent, *self;
	mleaf_t	*leaf;
	int		l;
	vec3_t	view;
	
// find a new check if on a new frame
	if (sv.time - sv.lastchecktime >= 0.1)
	{
		sv.lastcheck = PF_newcheckclient (sv.lastcheck);
		sv.lastchecktime = sv.time;
	}

// return check if it might be visible	
	ent = EDICT_NUM(sv.lastcheck);
	if (ent->free || ent->v.health <= 0)
	{
		RETURN_EDICT(sv.edicts);
		return;
	}

// if current entity can't possibly see the check entity, return 0
	self = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));
	VectorAdd (self->v.origin, self->v.view_ofs, view);
	leaf = Mod_PointInLeaf (view, sv.worldmodel);
	l = (leaf - sv.worldmodel->leafs) - 1;
	if ( (l<0) || !(checkpvs[l>>3] & (1<<(l&7)) ) )
	{
c_notvis++;
		RETURN_EDICT(sv.edicts);
		return;
	}

// might be able to see it
c_invis++;
	RETURN_EDICT(ent);
}

//============================================================================


/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd (void)
{
	int		entnum;
	char	*str;
	client_t	*old;
	
	entnum = G_EDICTNUM(OFS_PARM0);
	if (entnum < 1 || entnum > svs.maxclients)
		PR_RunError ("Parm 0 not a client");
	str = G_STRING(OFS_PARM1);	
	
	old = host_client;
	host_client = &svs.clients[entnum-1];
	Host_ClientCommands ("%s", str);
	host_client = old;
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
void PF_localcmd (void)
{
	char	*str;
	
	str = G_STRING(OFS_PARM0);	
	Cbuf_AddText (str);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
void PF_cvar (void)
{
	char	*str;
	
	str = G_STRING(OFS_PARM0);
	
	G_FLOAT(OFS_RETURN) = Cvar_VariableValue (str);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
void PF_cvar_set (void)
{
	char	*var, *val;
	
	var = G_STRING(OFS_PARM0);
	val = G_STRING(OFS_PARM1);
	
	Cvar_Set (var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void PF_findradius (void)
{
	edict_t	*ent, *chain;
	float	rad;
	float	*org;
	vec3_t	eorg;
	int		i, j;

	chain = (edict_t *)sv.edicts;
	
	org = G_VECTOR(OFS_PARM0);
	rad = G_FLOAT(OFS_PARM1);

	ent = NEXT_EDICT(sv.edicts);
	for (i=1 ; i<sv.num_edicts ; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;
		if (ent->v.solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ent->v.origin[j] + (ent->v.mins[j] + ent->v.maxs[j])*0.5);			
		if (VectorLength(eorg) > rad)
			continue;
			
		ent->v.chain = EDICT_TO_PROG(chain);
		chain = ent;
	}

	RETURN_EDICT(chain);
}


/*
=========
PF_dprint
=========
*/
void PF_dprint (void)
{
	Con_DPrintf ("%s",PF_VarString(0));
}

void PF_dprintf (void)
{
	char temp[256];
	float	v;

	v = G_FLOAT(OFS_PARM1);

	if (v == (int)v)
		sprintf (temp, "%d",(int)v);
	else
		sprintf (temp, "%5.1f",v);

	Con_DPrintf (G_STRING(OFS_PARM0),temp);
}

void PF_dprintv (void)
{
	char temp[256];

	sprintf (temp, "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM1)[0], G_VECTOR(OFS_PARM1)[1], G_VECTOR(OFS_PARM1)[2]);

	Con_DPrintf (G_STRING(OFS_PARM0),temp);
}

char	pr_string_temp[1024];

void PF_ftos (void)
{
	float	v;
	v = G_FLOAT(OFS_PARM0);
	
	if (v == (int)v)
		sprintf (pr_string_temp, "%d",(int)v);
	else
		sprintf (pr_string_temp, "%5.1f",v);
	G_INT(OFS_RETURN) = pr_string_temp - pr_strings;
}

void PF_fabs (void)
{
	float	v;
	v = G_FLOAT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = fabs(v);
}

void PF_vtos (void)
{
	sprintf (pr_string_temp, "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
	G_INT(OFS_RETURN) = pr_string_temp - pr_strings;
}

void PF_Spawn (void)
{
	edict_t	*ed;

	ed = ED_Alloc();

	RETURN_EDICT(ed);
}

void PF_SpawnTemp (void)
{
	edict_t	*ed;

	ed = ED_Alloc_Temp();

	RETURN_EDICT(ed);
}

void PF_Remove (void)
{
	edict_t	*ed;
	int i;
	
	ed = G_EDICT(OFS_PARM0);
	if (ed == sv.edicts)
	{
		Con_DPrintf("Tried to remove the world at %s in %s!\n",
			pr_xfunction->s_name + pr_strings, pr_xfunction->s_file + pr_strings);
		return;
	}

	i = NUM_FOR_EDICT(ed);
	if (i <= svs.maxclients)
	{
		Con_DPrintf("Tried to remove a client at %s in %s!\n",
			pr_xfunction->s_name + pr_strings, pr_xfunction->s_file + pr_strings);
		return;
	}
	ED_Free (ed);
}


// entity (entity start, .string field, string match) find = #5;
void PF_Find (void)
{
	int		e;	
	int		f;
	char	*s, *t;
	edict_t	*ed;

	e = G_EDICTNUM(OFS_PARM0);
	f = G_INT(OFS_PARM1);
	s = G_STRING(OFS_PARM2);
	if (!s)
		PR_RunError ("PF_Find: bad search string");
		
	for (e++ ; e < sv.num_edicts ; e++)
	{
		ed = EDICT_NUM(e);
		if (ed->free)
			continue;
		t = E_STRING(ed,f);
		if (!t)
			continue;
		if (!strcmp(t,s))
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv.edicts);
}

void PF_FindFloat (void)
{
	int		e;	
	int		f;
	float	s, t;
	edict_t	*ed;

	e = G_EDICTNUM(OFS_PARM0);
	f = G_INT(OFS_PARM1);
	s = G_FLOAT(OFS_PARM2);
	if (!s)
		PR_RunError ("PF_Find: bad search string");
		
	for (e++ ; e < sv.num_edicts ; e++)
	{
		ed = EDICT_NUM(e);
		if (ed->free)
			continue;
		t = E_FLOAT(ed,f);
		if (t == s)
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv.edicts);
}

void PR_CheckEmptyString (char *s)
{
	if (s[0] <= ' ')
		PR_RunError ("Bad string");
}

void PF_precache_file (void)
{	// precache_file is only used to copy files with qcc, it does nothing
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

void PF_precache_sound (void)
{
	char	*s;
	int		SOUNDS_MAX; /* just to be on the safe side */
	int		i;
	
	if (sv.state != ss_loading && !ignore_precache)
		PR_RunError ("PF_Precache_*: Precache can only be done in spawn functions");
		
	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString (s);
	
	SOUNDS_MAX = (sv.Protocol == PROTOCOL_RAVEN_111) ? MAX_SOUNDS_OLD : MAX_SOUNDS;
	for (i=0 ; i<SOUNDS_MAX ; i++)
	{
		if (!sv.sound_precache[i])
		{
			sv.sound_precache[i] = s;
			return;
		}
		if (!strcmp(sv.sound_precache[i], s))
			return;
	}
	PR_RunError ("PF_precache_sound: overflow");
}

void PF_precache_sound2 (void)
{
	if (!registered.value)
		return;

	PF_precache_sound();
}

void PF_precache_sound3 (void)
{
	if (!registered.value && !oem.value)
		return;

	PF_precache_sound();
}

void PF_precache_sound4 (void)
{//mission pack only
	if (!registered.value)
		return;

	PF_precache_sound();
}

void PF_precache_model (void)
{
	char	*s;
	int		i;
	
	if (sv.state != ss_loading && !ignore_precache)
		PR_RunError ("PF_Precache_*: Precache can only be done in spawn functions");
		
	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString (s);

	for (i=0 ; i<MAX_MODELS ; i++)
	{
		if (!sv.model_precache[i])
		{
			sv.model_precache[i] = s;
			sv.models[i] = Mod_ForName (s, true);
			return;
		}
		if (!strcmp(sv.model_precache[i], s))
		{
//			Con_DPrintf("duplicate precache: %s!\n",s);
			return;
		}
	}
	PR_RunError ("PF_precache_model: overflow");
}

void PF_precache_model2 (void)
{
	if (!registered.value)
		return;

	PF_precache_model();
}

void PF_precache_model3 (void)
{
	if (!registered.value && !oem.value)
		return;

	PF_precache_model();
}

void PF_precache_model4 (void)
{
	if (!registered.value)
		return;
	PF_precache_model();
}

void PF_precache_puzzle_model (void)
{
	int		i;
	char	*s,temp[256],*m;
	
	if (sv.state != ss_loading && !ignore_precache)
		PR_RunError ("PF_Precache_*: Precache can only be done in spawn functions");
		
	m = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);

	sprintf(temp,"models/puzzle/%s.mdl",m);
	s = ED_NewString (temp);

	PR_CheckEmptyString (s);

	for (i=0 ; i<MAX_MODELS ; i++)
	{
		if (!sv.model_precache[i])
		{
			sv.model_precache[i] = s;
			sv.models[i] = Mod_ForName (s, true);
			return;
		}
		if (!strcmp(sv.model_precache[i], s))
			return;
	}
	PR_RunError ("PF_precache_puzzle_model: overflow");
}


void PF_coredump (void)
{
	ED_PrintEdicts ();
}

void PF_traceon (void)
{
	pr_trace = true;
}

void PF_traceoff (void)
{
	pr_trace = false;
}

void PF_eprint (void)
{
	ED_PrintNum (G_EDICTNUM(OFS_PARM0));
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void PF_walkmove (void)
{
	edict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
	dfunction_t	*oldf;
	int 	oldself;
	qboolean set_trace;
	
	ent = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));
	yaw = G_FLOAT(OFS_PARM0);
	dist = G_FLOAT(OFS_PARM1);
	set_trace = G_FLOAT(OFS_PARM2);
	
	if ( !( (int)ent->v.flags & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	yaw = yaw*M_PI*2 / 360;
	
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

// save program state, because SV_movestep may call other progs
	oldf = pr_xfunction;
	oldself = PR_GLOBAL_STRUCT(self);
	
	G_FLOAT(OFS_RETURN) = SV_movestep(ent, move, true, true, set_trace);
	
	
// restore program state
	pr_xfunction = oldf;
	if (is_progdefs111)
		pr_global_struct_v111->self = oldself;
	else
		pr_global_struct->self = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void PF_droptofloor (void)
{
	edict_t		*ent;
	vec3_t		end;
	trace_t		trace;
	
	ent = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));

	VectorCopy (ent->v.origin, end);
	end[2] -= 256;
	
	trace = SV_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

	if (trace.fraction == 1 || trace.allsolid)
		G_FLOAT(OFS_RETURN) = 0;
	else
	{
		VectorCopy (trace.endpos, ent->v.origin);
		SV_LinkEdict (ent, false);
		ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
		ent->v.groundentity = EDICT_TO_PROG(trace.ent);
		G_FLOAT(OFS_RETURN) = 1;
	}
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle (void)
{
	int		style;
	char	*val;
	client_t	*client;
	int			j;
	
	style = G_FLOAT(OFS_PARM0);
	val = G_STRING(OFS_PARM1);

// change the string in sv
	sv.lightstyles[style] = val;
	
// send message to all clients on this server
	if (sv.state != ss_active)
		return;
	
	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
		if (client->active || client->spawned)
		{
			MSG_WriteChar (&client->message, svc_lightstyle);
			MSG_WriteChar (&client->message,style);
			MSG_WriteString (&client->message, val);
		}
}

//==========================================================================
//
// PF_lightstylevalue
//
// void lightstylevalue(float style);
//
//==========================================================================

extern int d_lightstyle[256];

void PF_lightstylevalue(void)
{
	int style;

	style = G_FLOAT(OFS_PARM0);
	if(style < 0 || style >= MAX_LIGHTSTYLES)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	G_FLOAT(OFS_RETURN) = (int)((float)d_lightstyle[style]/22.0);
}

//==========================================================================
//
// PF_lightstylestatic
//
// void lightstylestatic(float style, float value);
//
//==========================================================================

void PF_lightstylestatic(void)
{
	int i;
	int value;
	int styleNumber;
	char *styleString;
	client_t *client;
	static char *styleDefs[] =
	{
		"a", "b", "c", "d", "e", "f", "g",
		"h", "i", "j", "k", "l", "m", "n",
		"o", "p", "q", "r", "s", "t", "u",
		"v", "w", "x", "y", "z"
	};

	styleNumber = G_FLOAT(OFS_PARM0);
	value = G_FLOAT(OFS_PARM1);
	if(value < 0)
	{
		value = 0;
	}
	else if(value > 'z'-'a')
	{
		value = 'z'-'a';
	}
	styleString = styleDefs[value];

	// Change the string in sv
	sv.lightstyles[styleNumber] = styleString;

	if(sv.state != ss_active)
	{
		return;
	}

	// Send message to all clients on this server
	for(i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if(client->active || client->spawned)
		{
			MSG_WriteChar(&client->message, svc_lightstyle);
			MSG_WriteChar(&client->message, styleNumber);
			MSG_WriteString(&client->message, styleString);
		}
	}
}

void PF_rint (void)
{
	float	f;
	f = G_FLOAT(OFS_PARM0);
	if (f > 0)
		G_FLOAT(OFS_RETURN) = (int)(f + 0.1);
	else
		G_FLOAT(OFS_RETURN) = (int)(f - 0.1);
}
void PF_floor (void)
{
	G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}
void PF_ceil (void)
{
	G_FLOAT(OFS_RETURN) = ceil(G_FLOAT(OFS_PARM0));
}


/*
=============
PF_checkbottom
=============
*/
void PF_checkbottom (void)
{
	edict_t	*ent;
	
	ent = G_EDICT(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = SV_CheckBottom (ent);
}

/*
=============
PF_pointcontents
=============
*/
void PF_pointcontents (void)
{
	float	*v;
	
	v = G_VECTOR(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = SV_PointContents (v);	
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void PF_nextent (void)
{
	int		i;
	edict_t	*ent;
	
	i = G_EDICTNUM(OFS_PARM0);
	while (1)
	{
		i++;
		if (i == sv.num_edicts)
		{
			RETURN_EDICT(sv.edicts);
			return;
		}
		ent = EDICT_NUM(i);
		if (!ent->free)
		{
			RETURN_EDICT(ent);
			return;
		}
	}
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
cvar_t	sv_aim = {"sv_aim", "0.93"};
void PF_aim (void)
{
	edict_t	*ent, *check, *bestent;
	vec3_t	start, dir, end, bestdir,hold_org;
	int		i, j;
	trace_t	tr;
	float	dist, bestdist;
	float	speed;
	float	*shot_org;
	float save_hull;

	ent = G_EDICT(OFS_PARM0);
	shot_org = G_VECTOR(OFS_PARM1);
	speed = G_FLOAT(OFS_PARM2);

//	VectorCopy (ent->v.origin, start);
	VectorCopy (shot_org, start);
	start[2] += 20;

// try sending a trace straight
	VectorCopy (PR_GLOBAL_STRUCT(v_forward), dir);
	VectorMA (start, 2048, dir, end);

	save_hull = ent->v.hull;
	ent->v.hull = 0;
	tr = SV_Move (start, vec3_origin, vec3_origin, end, false, ent);
	ent->v.hull = save_hull;

	if (tr.ent && tr.ent->v.takedamage == DAMAGE_YES
	&& (!teamplay.value || ent->v.team <=0 || ent->v.team != tr.ent->v.team) )
	{
		VectorCopy (PR_GLOBAL_STRUCT(v_forward), G_VECTOR(OFS_RETURN));
		return;
	}

// try all possible entities
	VectorCopy (dir, bestdir);
	bestdist = sv_aim.value;
	bestent = NULL;
	
	check = NEXT_EDICT(sv.edicts);
	for (i=1 ; i<sv.num_edicts ; i++, check = NEXT_EDICT(check) )
	{
		if (check->v.takedamage != DAMAGE_YES)
			continue;
		if (check == ent)
			continue;
		if (teamplay.value && ent->v.team > 0 && ent->v.team == check->v.team)
			continue;	// don't aim at teammate
		for (j=0 ; j<3 ; j++)
			end[j] = check->v.origin[j]
			+ 0.5*(check->v.mins[j] + check->v.maxs[j]);
		VectorSubtract (end, start, dir);
		VectorNormalize (dir);
		dist = DotProduct (dir, PR_GLOBAL_STRUCT(v_forward));
		if (dist < bestdist)
			continue;	// to far to turn
	save_hull = ent->v.hull;
	ent->v.hull = 0;
		tr = SV_Move (start, vec3_origin, vec3_origin, end, false, ent);
	ent->v.hull = save_hull;
		if (tr.ent == check)
		{	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}
	
	if (bestent)
	{	// Since all origins are at the base, move the point to the middle of the victim model
		hold_org[0] =bestent->v.origin[0]; 
		hold_org[1] =bestent->v.origin[1]; 
		hold_org[2] =bestent->v.origin[2] + (0.5 * bestent->v.maxs[2]);

		VectorSubtract (hold_org,shot_org,dir);
		dist = DotProduct (dir, PR_GLOBAL_STRUCT(v_forward));
		VectorScale (PR_GLOBAL_STRUCT(v_forward), dist, end);
		end[2] = dir[2];
		VectorNormalize (end);
		VectorCopy (end, G_VECTOR(OFS_RETURN));	
	}
	else
	{
		VectorCopy (bestdir, G_VECTOR(OFS_RETURN));
	}
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw (void)
{
	edict_t		*ent;
	float		ideal, current, move, speed;
	
	ent = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));
	current = anglemod( ent->v.angles[1] );
	ideal = ent->v.ideal_yaw;
	speed = ent->v.yaw_speed;
	
	if (current == ideal)
	{
	   G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	move = ideal - current;
	
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}

   G_FLOAT(OFS_RETURN) = move;

	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	
	ent->v.angles[1] = anglemod (current + move);
}


/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define	MSG_BROADCAST	0		// unreliable to all
#define	MSG_ONE			1		// reliable to one (msg_entity)
#define	MSG_ALL			2		// reliable to all
#define	MSG_INIT		3		// write to the init string

sizebuf_t *WriteDest (void)
{
	int		entnum;
	int		dest;
	edict_t	*ent;

	dest = G_FLOAT(OFS_PARM0);
	switch (dest)
	{
	case MSG_BROADCAST:
		return &sv.datagram;
	
	case MSG_ONE:
		ent = PROG_TO_EDICT(PR_GLOBAL_STRUCT(msg_entity));
		entnum = NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > svs.maxclients)
			PR_RunError ("WriteDest: not a client");
		return &svs.clients[entnum-1].message;
		
	case MSG_ALL:
		return &sv.reliable_datagram;
	
	case MSG_INIT:
		return &sv.signon;

	default:
		PR_RunError ("WriteDest: bad destination");
		break;
	}
	
	return NULL;
}

void PF_WriteByte (void)
{
	MSG_WriteByte (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteChar (void)
{
	MSG_WriteChar (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteShort (void)
{
	MSG_WriteShort (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteLong (void)
{
	MSG_WriteLong (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteAngle (void)
{
	MSG_WriteAngle (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteCoord (void)
{
	MSG_WriteCoord (WriteDest(), G_FLOAT(OFS_PARM1));
}

void PF_WriteString (void)
{
	MSG_WriteString (WriteDest(), G_STRING(OFS_PARM1));
}


void PF_WriteEntity (void)
{
	MSG_WriteShort (WriteDest(), G_EDICTNUM(OFS_PARM1));
}

//=============================================================================

int SV_ModelIndex (char *name);

void PF_makestatic (void)
{
	edict_t	*ent;
	int		i;
	
	ent = G_EDICT(OFS_PARM0);

	MSG_WriteByte (&sv.signon,svc_spawnstatic);

	MSG_WriteShort (&sv.signon, SV_ModelIndex(pr_strings + ent->v.model));

	MSG_WriteByte (&sv.signon, ent->v.frame);
	MSG_WriteByte (&sv.signon, ent->v.colormap);
	MSG_WriteByte (&sv.signon, ent->v.skin);
	MSG_WriteByte (&sv.signon, (int)(ent->v.scale*100.0)&255);
	MSG_WriteByte (&sv.signon, ent->v.drawflags);
	MSG_WriteByte (&sv.signon, (int)(ent->v.abslight*255.0)&255);

	for (i=0 ; i<3 ; i++)
	{
		MSG_WriteCoord(&sv.signon, ent->v.origin[i]);
		MSG_WriteAngle(&sv.signon, ent->v.angles[i]);
	}

// throw the entity away now
	ED_Free (ent);
}

//=============================================================================

/*
==============
PF_setspawnparms
==============
*/
void PF_setspawnparms (void)
{
	edict_t	*ent;
	int		i;
	client_t	*client;

	ent = G_EDICT(OFS_PARM0);
	i = NUM_FOR_EDICT(ent);
	if (i < 1 || i > svs.maxclients)
		PR_RunError ("Entity is not a client");

	// copy spawn parms out of the client_t
	client = svs.clients + (i-1);

	for (i=0 ; i< NUM_SPAWN_PARMS ; i++)
	{
	    if (is_progdefs111)
		(&pr_global_struct_v111->parm1)[i] = client->spawn_parms[i];
	    else
		(&pr_global_struct->parm1)[i] = client->spawn_parms[i];
	}
}

/*
==============
PF_changelevel
==============
*/
void PF_changelevel (void)
{
	char	*s1, *s2;

	if (svs.changelevel_issued)
		return;
	svs.changelevel_issued = true;

	s1 = G_STRING(OFS_PARM0);
	s2 = G_STRING(OFS_PARM1);

	if ((int)PR_GLOBAL_STRUCT(serverflags) & (SFL_NEW_UNIT | SFL_NEW_EPISODE))
		Cbuf_AddText (va("changelevel %s %s\n",s1, s2));
	else
		Cbuf_AddText (va("changelevel2 %s %s\n",s1, s2));
}


void PF_sin (void)
{
	G_FLOAT(OFS_RETURN) = sin(G_FLOAT(OFS_PARM0));
}

void PF_cos (void)
{
	G_FLOAT(OFS_RETURN) = cos(G_FLOAT(OFS_PARM0));
}

void PF_sqrt (void)
{
	G_FLOAT(OFS_RETURN) = sqrt(G_FLOAT(OFS_PARM0));
}

void PF_Fixme (void)
{
	PR_RunError ("unimplemented builtin");
}


void PF_plaque_draw (void)
{
	int Index;

//	plaquemessage = G_STRING(OFS_PARM0);

	Index = ((int)G_FLOAT(OFS_PARM1));

	if (Index < 0)
		PR_RunError ("PF_plaque_draw: index(%d) < 1",Index);

	if (Index > pr_string_count)
		PR_RunError ("PF_plaque_draw: index(%d) >= pr_string_count(%d)",Index,pr_string_count);

	MSG_WriteByte (WriteDest(), svc_plaque);
	MSG_WriteShort (WriteDest(), Index);
}

void PF_rain_go (void)
{
	float *min_org,*max_org,*e_size;
	float *dir;
	vec3_t	org,org2;
	int color,count,x_dir,y_dir;

	min_org = G_VECTOR (OFS_PARM0);
	max_org = G_VECTOR (OFS_PARM1);
	e_size  = G_VECTOR (OFS_PARM2);
	dir		= G_VECTOR (OFS_PARM3);
	color	= G_FLOAT (OFS_PARM4);
	count = G_FLOAT (OFS_PARM5);

	org[0] = min_org[0];
	org[1] = min_org[1];
	org[2] = max_org[2];

	org2[0] = e_size[0];
	org2[1] = e_size[1];
	org2[2] = e_size[2];

	x_dir = dir[0];
	y_dir = dir[1];
	
//	R_RainEffect(org,org2,x_dir,y_dir,color,count);	//DUH!
//void SV_StartRainEffect (vec3_t org, vec3_t e_size, int x_dir, int y_dir, int color, int count)
{
	MSG_WriteByte (&sv.datagram, svc_raineffect);
	MSG_WriteCoord (&sv.datagram, org[0]);
	MSG_WriteCoord (&sv.datagram, org[1]);
	MSG_WriteCoord (&sv.datagram, org[2]);
	MSG_WriteCoord (&sv.datagram, e_size[0]);
	MSG_WriteCoord (&sv.datagram, e_size[1]);
	MSG_WriteCoord (&sv.datagram, e_size[2]);
	MSG_WriteAngle (&sv.datagram, x_dir);	
	MSG_WriteAngle (&sv.datagram, y_dir);	
	MSG_WriteShort (&sv.datagram, color);
	MSG_WriteShort (&sv.datagram, count);

//	SV_Multicast (org, MULTICAST_PVS);
}

}

void PF_particleexplosion (void)
{
	float *org;
	int color,radius,counter;

	org = G_VECTOR(OFS_PARM0);
	color = G_FLOAT(OFS_PARM1);
	radius = G_FLOAT(OFS_PARM2);
	counter = G_FLOAT(OFS_PARM3);

	MSG_WriteByte(&sv.datagram, svc_particle_explosion);
	MSG_WriteCoord(&sv.datagram, org[0]);
	MSG_WriteCoord(&sv.datagram, org[1]);
	MSG_WriteCoord(&sv.datagram, org[2]);
	MSG_WriteShort(&sv.datagram, color);
	MSG_WriteShort(&sv.datagram, radius);
	MSG_WriteShort(&sv.datagram, counter);
}

void PF_movestep (void)
{
	vec3_t v;
	edict_t	*ent;
	dfunction_t	*oldf;
	int 	oldself;
	qboolean set_trace;

	ent = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));

	v[0] = G_FLOAT(OFS_PARM0);
	v[1] = G_FLOAT(OFS_PARM1);
	v[2] = G_FLOAT(OFS_PARM2);
	set_trace = G_FLOAT(OFS_PARM3);

// save program state, because SV_movestep may call other progs
	oldf = pr_xfunction;
	oldself = PR_GLOBAL_STRUCT(self);

	G_INT(OFS_RETURN) = SV_movestep (ent, v, false, true, set_trace);

// restore program state
	pr_xfunction = oldf;
	if (is_progdefs111)
		pr_global_struct_v111->self = oldself;
	else
		pr_global_struct->self = oldself;
}

//

void PF_Cos(void)
{
	float angle;

	angle = G_FLOAT(OFS_PARM0);

	angle = angle*M_PI*2 / 360;
	
	G_FLOAT(OFS_RETURN) = cos(angle);
}

void PF_Sin(void)
{
	float angle;

	angle = G_FLOAT(OFS_PARM0);

	angle = angle*M_PI*2 / 360;
	
	G_FLOAT(OFS_RETURN) = sin(angle);
}

void PF_AdvanceFrame(void)
{
	edict_t *Ent;
	float Start,End,Result;
	
	Ent = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));
	Start = G_FLOAT(OFS_PARM0);
	End = G_FLOAT(OFS_PARM1);

	if (Ent->v.frame < Start || Ent->v.frame > End)
	{ // Didn't start in the range
		Ent->v.frame = Start;
		Result = 0;
	}
	else if(Ent->v.frame == End)
	{  // Wrapping
		Ent->v.frame = Start;
		Result = 1;
	}
	else
	{  // Regular Advance
		Ent->v.frame++;
		if (Ent->v.frame == End)
			Result = 2;
		else 
			Result = 0;
	}

	G_FLOAT(OFS_RETURN) = Result;
}

void PF_RewindFrame(void)
{
	edict_t *Ent;
	float Start,End,Result;
	
	Ent = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));
	Start = G_FLOAT(OFS_PARM0);
	End = G_FLOAT(OFS_PARM1);

	if (Ent->v.frame > Start || Ent->v.frame < End)
	{ // Didn't start in the range
		Ent->v.frame = Start;
		Result = 0;
	}
	else if (Ent->v.frame == End)
	{  // Wrapping
		Ent->v.frame = Start;
		Result = 1;
	}
	else
	{  // Regular Advance
		Ent->v.frame--;
		if (Ent->v.frame == End)
			Result = 2;
		else
			Result = 0;
	}

	G_FLOAT(OFS_RETURN) = Result;
}

#define WF_NORMAL_ADVANCE 0
#define WF_CYCLE_STARTED 1
#define WF_CYCLE_WRAPPED 2
#define WF_LAST_FRAME 3

void PF_advanceweaponframe (void)
{
	edict_t *ent;
	float startframe,endframe;
	float state;

	ent = PROG_TO_EDICT(PR_GLOBAL_STRUCT(self));
	startframe = G_FLOAT(OFS_PARM0);
	endframe = G_FLOAT(OFS_PARM1);

	if ((endframe > startframe && (ent->v.weaponframe > endframe || ent->v.weaponframe < startframe)) ||
	(endframe < startframe && (ent->v.weaponframe < endframe || ent->v.weaponframe > startframe)) )
	{
		ent->v.weaponframe=startframe;
		state = WF_CYCLE_STARTED;
	}
	else if(ent->v.weaponframe==endframe)
	{			  
		ent->v.weaponframe=startframe;
		state = WF_CYCLE_WRAPPED;
	}
	else
	{
		if (startframe > endframe)
			ent->v.weaponframe = ent->v.weaponframe - 1;
		else if (startframe < endframe)
			ent->v.weaponframe = ent->v.weaponframe + 1;

		if (ent->v.weaponframe==endframe)
			state = WF_LAST_FRAME;
		else 
			state = WF_NORMAL_ADVANCE;
	}

	G_FLOAT(OFS_RETURN) = state;
}

void PF_setclass (void)
{
	float		NewClass;
	int			entnum;
	edict_t	*e;
	client_t	*client,*old;
	
	entnum = G_EDICTNUM(OFS_PARM0);
	e = G_EDICT(OFS_PARM0);
	NewClass = G_FLOAT(OFS_PARM1);
	
	if (entnum < 1 || entnum > svs.maxclients)
	{
		Con_Printf ("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum-1];

	old = host_client;
	host_client = client;
	Host_ClientCommands ("playerclass %i\n", (int)NewClass);
	host_client = old;

	// These will get set again after the message has filtered its way
	// but it wouldn't take affect right away
	e->v.playerclass = NewClass;
	client->playerclass = NewClass;
}

void PF_starteffect (void)
{
	SV_ParseEffect(&sv.reliable_datagram);
}

void PF_endeffect (void)
{
	int index;

	index = G_FLOAT(OFS_PARM0);
	index = G_FLOAT(OFS_PARM1);

	if (!sv.Effects[index].type) return;

	sv.Effects[index].type = 0;
	MSG_WriteByte (&sv.reliable_datagram, svc_end_effect);
	MSG_WriteByte (&sv.reliable_datagram, index);
}

void PF_randomrange(void)
{
	float num,minv,maxv;

	minv = G_FLOAT(OFS_PARM0);
	maxv = G_FLOAT(OFS_PARM1);

	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);

	G_FLOAT(OFS_RETURN) = ((maxv-minv) * num) + minv;
}

void PF_randomvalue(void)
{
	float num,range;

	range = G_FLOAT(OFS_PARM0);

	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);

	G_FLOAT(OFS_RETURN) = range * num;
}

void PF_randomvrange(void)
{
	float num,*minv,*maxv;
	vec3_t result;

	minv = G_VECTOR(OFS_PARM0);
	maxv = G_VECTOR(OFS_PARM1);

	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
	result[0] = ((maxv[0]-minv[0]) * num) + minv[0];
	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
	result[1] = ((maxv[1]-minv[1]) * num) + minv[1];
	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
	result[2] = ((maxv[2]-minv[2]) * num) + minv[2];

	VectorCopy (result, G_VECTOR(OFS_RETURN));
}

void PF_randomvvalue(void)
{
	float num,*range;
	vec3_t result;

	range = G_VECTOR(OFS_PARM0);

	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
	result[0] = range[0] * num;
	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
	result[1] = range[1] * num;
	num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
	result[2] = range[2] * num;

	VectorCopy (result, G_VECTOR(OFS_RETURN));
}

void PF_concatv(void)
{
	float *in,*range;
	vec3_t result;

	in = G_VECTOR(OFS_PARM0);
	range = G_VECTOR(OFS_PARM1);

	VectorCopy (in, result);
	if (result[0] < -range[0]) result[0] = -range[0];
	if (result[0] > range[0]) result[0] = range[0];
	if (result[1] < -range[1]) result[1] = -range[1];
	if (result[1] > range[1]) result[1] = range[1];
	if (result[2] < -range[2]) result[2] = -range[2];
	if (result[2] > range[2]) result[2] = range[2];

	VectorCopy (result, G_VECTOR(OFS_RETURN));
}

void PF_GetString(void)
{
	int Index;

	Index = (int)G_FLOAT(OFS_PARM0) - 1;

	if (Index < 0)
		PR_RunError ("PF_GetString: index(%d) < 1",Index+1);

	if (Index >= pr_string_count)
		PR_RunError ("PF_GetString: index(%d) >= pr_string_count(%d)",Index,pr_string_count);

	G_INT(OFS_RETURN) = (&pr_global_strings[pr_string_index[Index]]) - pr_strings;
}


void PF_v_factor(void)
// returns (v_right * factor_x) + (v_forward * factor_y) + (v_up * factor_z)
{
	float *range;
	vec3_t result;

	range = G_VECTOR(OFS_PARM0);

	result[0] = (PR_GLOBAL_STRUCT(v_right[0]) * range[0]) +
				(PR_GLOBAL_STRUCT(v_forward[0]) * range[1]) +
				(PR_GLOBAL_STRUCT(v_up[0]) * range[2]);

	result[1] = (PR_GLOBAL_STRUCT(v_right[1]) * range[0]) +
				(PR_GLOBAL_STRUCT(v_forward[1]) * range[1]) +
				(PR_GLOBAL_STRUCT(v_up[1]) * range[2]);

	result[2] = (PR_GLOBAL_STRUCT(v_right[2]) * range[0]) +
				(PR_GLOBAL_STRUCT(v_forward[2]) * range[1]) +
				(PR_GLOBAL_STRUCT(v_up[2]) * range[2]);

	VectorCopy (result, G_VECTOR(OFS_RETURN));
}

void PF_v_factorrange(void)
// returns (v_right * factor_x) + (v_forward * factor_y) + (v_up * factor_z)
{
	float num,*minv,*maxv;
	vec3_t result,r2;

	minv = G_VECTOR(OFS_PARM0);
	maxv = G_VECTOR(OFS_PARM1);

//	num = (rand() & 0x7fff) / ((float)0x7fff);
	num = rand() * (1.0 / RAND_MAX);
	result[0] = ((maxv[0] - minv[0]) * num) + minv[0];
//	num = (rand() & 0x7fff) / ((float)0x7fff);
	num = rand() * (1.0 / RAND_MAX);
	result[1] = ((maxv[1] - minv[1]) * num) + minv[1];
//	num = (rand() & 0x7fff) / ((float)0x7fff);
	num = rand() * (1.0 / RAND_MAX);
	result[2] = ((maxv[2] - minv[2]) * num) + minv[2];

	r2[0] = (PR_GLOBAL_STRUCT(v_right[0]) * result[0]) +
			(PR_GLOBAL_STRUCT(v_forward[0]) * result[1]) +
			(PR_GLOBAL_STRUCT(v_up[0]) * result[2]);

	r2[1] = (PR_GLOBAL_STRUCT(v_right[1]) * result[0]) +
			(PR_GLOBAL_STRUCT(v_forward[1]) * result[1]) +
			(PR_GLOBAL_STRUCT(v_up[1]) * result[2]);

	r2[2] = (PR_GLOBAL_STRUCT(v_right[2]) * result[0]) +
			(PR_GLOBAL_STRUCT(v_forward[2]) * result[1]) +
			(PR_GLOBAL_STRUCT(v_up[2]) * result[2]);

	VectorCopy (r2, G_VECTOR(OFS_RETURN));
}

void PF_matchAngleToSlope(void)
{
	edict_t	*actor;
	vec3_t v_forward, old_forward, old_right, new_angles2 = { 0, 0, 0 };
	float pitch, mod, dot;

	// OFS_PARM0 is used by PF_vectoangles below
	actor = G_EDICT(OFS_PARM1);

	AngleVectors(actor->v.angles, old_forward, old_right, PR_GLOBAL_STRUCT(v_up));

	PF_vectoangles();

	pitch = G_FLOAT(OFS_RETURN) - 90;

	new_angles2[1] = G_FLOAT(OFS_RETURN+1);

	AngleVectors(new_angles2, v_forward, PR_GLOBAL_STRUCT(v_right), PR_GLOBAL_STRUCT(v_up));

	mod = DotProduct(v_forward, old_right);

	if(mod<0)
		mod=1;
	else
		mod=-1;

	dot = DotProduct(v_forward, old_forward);

	actor->v.angles[0] = dot*pitch;
	actor->v.angles[2] = (1-fabs(dot))*pitch*mod;
}

void PF_updateInfoPlaque (void)
{
	unsigned int check;
	unsigned int index, mode;
	long *use;
	int	ofs = 0;

	index = G_FLOAT(OFS_PARM0);
	mode = G_FLOAT(OFS_PARM1);

	if (index > 31) 
	{
		use = &info_mask2;
		ofs = 32;
	}
	else
	{
		use = &info_mask;
	}

	check = (long) (1 << (index - ofs));

	if (((mode & 1) && ((*use) & check)) || ((mode & 2) && !((*use) & check)))
		;
	else
	{
		(*use) ^= check;
	}
}

extern void V_WhiteFlash_f(void);

void PF_doWhiteFlash(void)
{
	V_WhiteFlash_f();
}

builtin_t pr_builtin[] =
{
PF_Fixme,

PF_makevectors,			// 1
PF_setorigin,			// 2
PF_setmodel,			// 3
PF_setsize,				// 4
PF_lightstylestatic,	// 5

PF_break,									// void() break														= #6
PF_random,									// float() random														= #7
PF_sound,									// void(entity e, float chan, string samp) sound			= #8
PF_normalize,								// vector(vector v) normalize										= #9
PF_error,									// void(string e) error												= #10
PF_objerror,								// void(string e) objerror											= #11
PF_vlen,										// float(vector v) vlen												= #12
PF_vectoyaw,								// float(vector v) vectoyaw										= #13
PF_Spawn,									// entity() spawn														= #14
PF_Remove,									// void(entity e) remove											= #15
PF_traceline,								// float(vector v1, vector v2, float tryents) traceline	= #16
PF_checkclient,							// entity() clientlist												= #17
PF_Find,										// entity(entity start, .string fld, string match) find	= #18
PF_precache_sound,						// void(string s) precache_sound									= #19
PF_precache_model,						// void(string s) precache_model									= #20
PF_stuffcmd,								// void(entity client, string s)stuffcmd						= #21
PF_findradius,								// entity(vector org, float rad) findradius					= #22
PF_bprint,									// void(string s) bprint											= #23
PF_sprint,									// void(entity client, string s) sprint						= #24
PF_dprint,									// void(string s) dprint											= #25
PF_ftos,										// void(string s) ftos												= #26
PF_vtos,										// void(string s) vto				s								= #27
PF_coredump,								//	PF_coredump															= #28
PF_traceon,									// PF_traceon															= #29
PF_traceoff,								// PF_traceoff															= #30
PF_eprint,									// void(entity e) debug print an entire entity				= #31
PF_walkmove,								// float(float yaw, float dist) walkmove						= #32
PF_tracearea,								// float(vector v1, vector v2, vector mins, vector maxs, 
												//       float tryents) traceline								= #33
PF_droptofloor,							// PF_droptofloor														= #34
PF_lightstyle,								//																			= #35
PF_rint,										//																			= #36
PF_floor,									//																			= #37
PF_ceil,										//																			= #38
PF_Fixme,
PF_checkbottom,							//																			= #40
PF_pointcontents,							//																			= #41
PF_particle2,
PF_fabs,										//																			= #43
PF_aim,										//																			= #44
PF_cvar,										//																			= #45
PF_localcmd,								//																			= #46
PF_nextent,									//																			= #47
PF_particle,								//																			= #48
PF_changeyaw,								//																			= #49
PF_vhlen,									// float(vector v) vhlen											= #50
PF_vectoangles,							//																			= #51

PF_WriteByte,								//																			= #52
PF_WriteChar,								//																			= #53
PF_WriteShort,								//																			= #54
PF_WriteLong,								//																			= #55
PF_WriteCoord,								//																			= #56
PF_WriteAngle,								//																			= #57
PF_WriteString,							//																			= #58
PF_WriteEntity,							//																			= #59

//PF_FindPath,								//																			= #60
PF_dprintf,									// void(string s1, string s2) dprint										= #60
PF_Cos,										//																			= #61
PF_Sin,										//																			= #62
PF_AdvanceFrame,							//																			= #63
PF_dprintv,									// void(string s1, string s2) dprint										= #64
PF_RewindFrame,								//																			= #65
PF_setclass,

SV_MoveToGoal,
PF_precache_file,
PF_makestatic,

PF_changelevel,

PF_lightstylevalue,		// 71

PF_cvar_set,
PF_centerprint,

PF_ambientsound,

PF_precache_model2,
PF_precache_sound2,							// precache_sound2 is different only for qcc
PF_precache_file,

PF_setspawnparms,
PF_plaque_draw,
PF_rain_go,										//																				=	#80
PF_particleexplosion,							//																				=	#81
PF_movestep,
PF_advanceweaponframe,
PF_sqrt,

PF_particle3,								// 85
PF_particle4,								// 86
PF_setpuzzlemodel,							// 87

PF_starteffect,								// 88
PF_endeffect,								// 89

PF_precache_puzzle_model,					// 90
PF_concatv,									// 91
PF_GetString,								// 92
PF_SpawnTemp,								// 93
PF_v_factor,								// 94
PF_v_factorrange,							// 95

PF_precache_sound3,							// 96
PF_precache_model3,							// 97
PF_precache_file,							// 98
PF_matchAngleToSlope,						// 99
PF_updateInfoPlaque,						//100

PF_precache_sound4,							// 101
PF_precache_model4,							// 102
PF_precache_file,							// 103

PF_doWhiteFlash,							//104
PF_UpdateSoundPos,							//105
PF_StopSound,								//106

PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme

};

builtin_t *pr_builtins = pr_builtin;
int pr_numbuiltins = sizeof(pr_builtin)/sizeof(pr_builtin[0]);

