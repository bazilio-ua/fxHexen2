/*
	sv_effect.c
	Client side effects.
*/

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

#define MAX_EFFECT_ENTITIES		256

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern cvar_t	sv_ce_scale;
extern cvar_t	sv_ce_max_size;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------


void SV_ClearEffects(void)
{
	memset(sv.Effects,0,sizeof(sv.Effects));
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void SV_SendEffect(sizebuf_t *sb, int index)
{
	qboolean	DoTest;
	vec3_t		TestO1,Diff;
	float		Size,TestDistance;
	int			i,count;

	if (sb == &sv.reliable_datagram && sv_ce_scale.value > 0)
		DoTest = true;
	else
		DoTest = false;

	VectorCopy(vec3_origin, TestO1);
	TestDistance = 0;

	switch(sv.Effects[index].type)
	{
		case CE_RAIN:
		case CE_SNOW:
			DoTest = false;
			break;

		case CE_FOUNTAIN:
			DoTest = false;
			break;

		case CE_QUAKE:
			VectorCopy(sv.Effects[index].effect.Quake.origin, TestO1);
			TestDistance = 700;
			break;

		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
		case CE_FLAMESTREAM:
		case CE_ACID_MUZZFL:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
			VectorCopy(sv.Effects[index].effect.Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_LG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_ACID_HIT:
		case CE_LBALL_EXPL:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_FBOOM:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
		case CE_BOMB:
		case CE_FLOOR_EXPLOSION3:
			VectorCopy(sv.Effects[index].effect.Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			VectorCopy(sv.Effects[index].effect.Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CE_RIDER_DEATH:
			DoTest = false;
			break;

		case CE_GRAVITYWELL:
			DoTest = false;
			break;

		case CE_TELEPORTERPUFFS:
			VectorCopy(sv.Effects[index].effect.Teleporter.origin, TestO1);
			TestDistance = 350;
			break;

		case CE_TELEPORTERBODY:
			VectorCopy(sv.Effects[index].effect.Teleporter.origin, TestO1);
			TestDistance = 350;
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			VectorCopy(sv.Effects[index].effect.Missile.origin, TestO1);
			TestDistance = 600;
			break;

		case CE_CHUNK:
			VectorCopy(sv.Effects[index].effect.Chunk.origin, TestO1);
			TestDistance = 600;
			break;

		default:
//			Sys_Error ("SV_SendEffect: bad type");
			PR_RunError ("SV_SendEffect: bad type");
			break;
	}

	if (!DoTest) 
		count = 1;
	else 
	{
		count = svs.maxclients;
		TestDistance = (float)TestDistance * sv_ce_scale.value;
		TestDistance *= TestDistance;
	}
	
	for(i=0;i<count;i++)
	{
		if (DoTest)
		{	
			if (svs.clients[i].active)
			{
				sb = &svs.clients[i].datagram;
				VectorSubtract(svs.clients[i].edict->v.origin,TestO1,Diff);
				Size = (Diff[0]*Diff[0]) + (Diff[1]*Diff[1]) + (Diff[2]*Diff[2]);

				if (Size > TestDistance)
					continue;
				
				if (sv_ce_max_size.value > 0 && sb->cursize > sv_ce_max_size.value)
					continue;
			}
			else
				continue;
		}
		
		MSG_WriteByte (sb, svc_start_effect);
		MSG_WriteByte (sb, index);
		MSG_WriteByte (sb, sv.Effects[index].type);
		
		switch(sv.Effects[index].type)
		{
			case CE_RAIN:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.min_org[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.min_org[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.min_org[2]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.max_org[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.max_org[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.max_org[2]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.e_size[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.e_size[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.e_size[2]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.dir[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.dir[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.dir[2]);
				MSG_WriteShort(sb, sv.Effects[index].effect.Rain.color);
				MSG_WriteShort(sb, sv.Effects[index].effect.Rain.count);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Rain.wait);
				break;
				
			case CE_SNOW:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.min_org[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.min_org[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.min_org[2]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.max_org[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.max_org[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.max_org[2]);
				MSG_WriteByte(sb, sv.Effects[index].effect.Rain.flags);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.dir[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.dir[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Rain.dir[2]);
				MSG_WriteByte(sb, sv.Effects[index].effect.Rain.count);
				//MSG_WriteShort(sb, sv.Effects[index].effect.Rain.veer);
				break;
				
			case CE_FOUNTAIN:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Fountain.pos[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Fountain.pos[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Fountain.pos[2]);
				MSG_WriteAngle(sb, sv.Effects[index].effect.Fountain.angle[0]);
				MSG_WriteAngle(sb, sv.Effects[index].effect.Fountain.angle[1]);
				MSG_WriteAngle(sb, sv.Effects[index].effect.Fountain.angle[2]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Fountain.movedir[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Fountain.movedir[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Fountain.movedir[2]);
				MSG_WriteShort(sb, sv.Effects[index].effect.Fountain.color);
				MSG_WriteByte(sb, sv.Effects[index].effect.Fountain.cnt);
				break;
				
			case CE_QUAKE:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Quake.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Quake.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Quake.origin[2]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Quake.radius);
				break;
				
			case CE_WHITE_SMOKE:
			case CE_GREEN_SMOKE:
			case CE_GREY_SMOKE:
			case CE_RED_SMOKE:
			case CE_SLOW_WHITE_SMOKE:
			case CE_TELESMK1:
			case CE_TELESMK2:
			case CE_GHOST:
			case CE_REDCLOUD:
			case CE_FLAMESTREAM:
			case CE_ACID_MUZZFL:
			case CE_FLAMEWALL:
			case CE_FLAMEWALL2:
			case CE_ONFIRE:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[2]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Smoke.velocity[0]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Smoke.velocity[1]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Smoke.velocity[2]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Smoke.framelength);
				/* smoke frame is a mission pack thing only. */
				if (sv.protocol > PROTOCOL_RAVEN_111)
					MSG_WriteFloat(sb, sv.Effects[index].effect.Smoke.frame);
				break;
				
			case CE_SM_WHITE_FLASH:
			case CE_YELLOWRED_FLASH:
			case CE_BLUESPARK:
			case CE_YELLOWSPARK:
			case CE_SM_CIRCLE_EXP:
			case CE_BG_CIRCLE_EXP:
			case CE_SM_EXPLOSION:
			case CE_LG_EXPLOSION:
			case CE_FLOOR_EXPLOSION:
			case CE_FLOOR_EXPLOSION3:
			case CE_BLUE_EXPLOSION:
			case CE_REDSPARK:
			case CE_GREENSPARK:
			case CE_ICEHIT:
			case CE_MEDUSA_HIT:
			case CE_MEZZO_REFLECT:
			case CE_FLOOR_EXPLOSION2:
			case CE_XBOW_EXPLOSION:
			case CE_NEW_EXPLOSION:
			case CE_MAGIC_MISSILE_EXPLOSION:
			case CE_BONE_EXPLOSION:
			case CE_BLDRN_EXPL:
			case CE_ACID_HIT:
			case CE_ACID_SPLAT:
			case CE_ACID_EXPL:
			case CE_LBALL_EXPL:	
			case CE_FIREWALL_SMALL:
			case CE_FIREWALL_MEDIUM:
			case CE_FIREWALL_LARGE:
			case CE_FBOOM:
			case CE_BOMB:
			case CE_BRN_BOUNCE:
			case CE_LSHOCK:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[2]);
				break;
				
			case CE_WHITE_FLASH:
			case CE_BLUE_FLASH:
			case CE_SM_BLUE_FLASH:
			case CE_RED_FLASH:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Smoke.origin[2]);
				break;
								
			case CE_RIDER_DEATH:
				MSG_WriteCoord(sb, sv.Effects[index].effect.RD.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.RD.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.RD.origin[2]);
				break;
				
			case CE_TELEPORTERPUFFS:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Teleporter.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Teleporter.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Teleporter.origin[2]);
				break;
				
			case CE_TELEPORTERBODY:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Teleporter.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Teleporter.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Teleporter.origin[2]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Teleporter.velocity[0][0]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Teleporter.velocity[0][1]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Teleporter.velocity[0][2]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Teleporter.skinnum);
				break;

			case CE_BONESHARD:
			case CE_BONESHRAPNEL:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Missile.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Missile.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Missile.origin[2]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.velocity[0]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.velocity[1]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.velocity[2]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.angle[0]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.angle[1]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.angle[2]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.avelocity[0]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.avelocity[1]);
				MSG_WriteFloat(sb, sv.Effects[index].effect.Missile.avelocity[2]);
				
				break;

			case CE_GRAVITYWELL:
				MSG_WriteCoord(sb, sv.Effects[index].effect.RD.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.RD.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.RD.origin[2]);
				MSG_WriteShort(sb, sv.Effects[index].effect.RD.color);
				MSG_WriteFloat(sb, sv.Effects[index].effect.RD.lifetime);
				break;

			case CE_CHUNK:
				MSG_WriteCoord(sb, sv.Effects[index].effect.Chunk.origin[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Chunk.origin[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Chunk.origin[2]);
				MSG_WriteByte (sb, sv.Effects[index].effect.Chunk.type);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Chunk.srcVel[0]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Chunk.srcVel[1]);
				MSG_WriteCoord(sb, sv.Effects[index].effect.Chunk.srcVel[2]);
				MSG_WriteByte (sb, sv.Effects[index].effect.Chunk.numChunks);

				//Con_Printf("Adding %d chunks on server...\n",sv.Effects[index].effect.Chunk.numChunks);
				break;

			default:
	//			Sys_Error ("SV_SendEffect: bad type");
				PR_RunError ("SV_SendEffect: bad type");
				break;
		}
	}
}

void SV_UpdateEffects(sizebuf_t *sb)
{
	int index;

	for(index=0;index<MAX_EFFECTS;index++)
		if (sv.Effects[index].type)
			SV_SendEffect(sb,index);
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void SV_ParseEffect(sizebuf_t *sb)
{
	int index;
	byte effect;

	effect = G_FLOAT(OFS_PARM0);

	for(index=0;index<MAX_EFFECTS;index++)
		if (!sv.Effects[index].type || 
			(sv.Effects[index].expire_time && sv.Effects[index].expire_time <= sv.time)) 
			break;
		
	if (index >= MAX_EFFECTS)
	{
		PR_RunError ("MAX_EFFECTS reached");
		return;
	}

//	Con_Printf("Effect #%d\n",index);

	memset(&sv.Effects[index],0,sizeof(struct EffectT));

	sv.Effects[index].type = effect;
	G_FLOAT(OFS_RETURN) = index;

	switch(effect)
	{
		case CE_RAIN:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].effect.Rain.min_org);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].effect.Rain.max_org);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].effect.Rain.e_size);
			VectorCopy(G_VECTOR(OFS_PARM4),sv.Effects[index].effect.Rain.dir);
			sv.Effects[index].effect.Rain.color = G_FLOAT(OFS_PARM5);
			sv.Effects[index].effect.Rain.count = G_FLOAT(OFS_PARM6);
			sv.Effects[index].effect.Rain.wait = G_FLOAT(OFS_PARM7);

			sv.Effects[index].effect.Rain.next_time = 0;
			break;

		case CE_SNOW:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].effect.Rain.min_org);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].effect.Rain.max_org);
			sv.Effects[index].effect.Rain.flags = G_FLOAT(OFS_PARM3);
			VectorCopy(G_VECTOR(OFS_PARM4),sv.Effects[index].effect.Rain.dir);
			sv.Effects[index].effect.Rain.count = G_FLOAT(OFS_PARM5);
			//sv.Effects[index].effect.Rain.veer = G_FLOAT(OFS_PARM6);
			//sv.Effects[index].effect.Rain.wait = 0.10;

			sv.Effects[index].effect.Rain.next_time = 0;
			break;

		case CE_FOUNTAIN:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].effect.Fountain.pos);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].effect.Fountain.angle);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].effect.Fountain.movedir);
			sv.Effects[index].effect.Fountain.color = G_FLOAT(OFS_PARM4);
			sv.Effects[index].effect.Fountain.cnt = G_FLOAT(OFS_PARM5);
			break;

		case CE_QUAKE:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.Quake.origin);
			sv.Effects[index].effect.Quake.radius = G_FLOAT(OFS_PARM2);
			break;

		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.Smoke.origin);
			VectorCopy(G_VECTOR(OFS_PARM2), sv.Effects[index].effect.Smoke.velocity);
			sv.Effects[index].effect.Smoke.framelength = G_FLOAT(OFS_PARM3);
			sv.Effects[index].effect.Smoke.frame = 0;
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_ACID_MUZZFL:
		case CE_FLAMESTREAM:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.Smoke.origin);
			VectorCopy(G_VECTOR(OFS_PARM2), sv.Effects[index].effect.Smoke.velocity);
			sv.Effects[index].effect.Smoke.framelength = 0.05;
			sv.Effects[index].effect.Smoke.frame = G_FLOAT(OFS_PARM3);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_LG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_FLOOR_EXPLOSION3:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_ACID_HIT:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_LBALL_EXPL:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
		case CE_FBOOM:
		case CE_BOMB:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.Smoke.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.Flash.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_RIDER_DEATH:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.RD.origin);
			break;

		case CE_GRAVITYWELL:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.RD.origin);
			sv.Effects[index].effect.RD.color = G_FLOAT(OFS_PARM2);
			sv.Effects[index].effect.RD.lifetime = G_FLOAT(OFS_PARM3);
			break;

		case CE_TELEPORTERPUFFS:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.Teleporter.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_TELEPORTERBODY:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.Teleporter.origin);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].effect.Teleporter.velocity[0]);
			sv.Effects[index].effect.Teleporter.skinnum = G_FLOAT(OFS_PARM3);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].effect.Missile.origin);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].effect.Missile.velocity);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].effect.Missile.angle);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].effect.Missile.avelocity);

			sv.Effects[index].expire_time = sv.time + 10;
			break;

		case CE_CHUNK:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].effect.Chunk.origin);
			sv.Effects[index].effect.Chunk.type = G_FLOAT(OFS_PARM2);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].effect.Chunk.srcVel);
			sv.Effects[index].effect.Chunk.numChunks = G_FLOAT(OFS_PARM4);

			sv.Effects[index].expire_time = sv.time + 3;
			break;


		default:
//			Sys_Error ("SV_ParseEffect: bad type");
			PR_RunError ("SV_SendEffect: bad type");
	}

	SV_SendEffect(sb,index);
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void SV_SaveEffects(FILE *FH)
{
	int index,count;

	for(index=count=0;index<MAX_EFFECTS;index++)
		if (sv.Effects[index].type)
			count++;

	fprintf(FH,"Effects: %d\n",count);

	for(index=count=0;index<MAX_EFFECTS;index++)
		if (sv.Effects[index].type)
		{
			fprintf(FH,"Effect: %d %d %f: ",index,sv.Effects[index].type,sv.Effects[index].expire_time);

			switch(sv.Effects[index].type)
			{
				case CE_RAIN:
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.min_org[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.min_org[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.min_org[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.max_org[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.max_org[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.max_org[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.e_size[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.e_size[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.e_size[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.dir[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.dir[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.dir[2]);
					fprintf(FH, "%d ", sv.Effects[index].effect.Rain.color);
					fprintf(FH, "%d ", sv.Effects[index].effect.Rain.count);
					fprintf(FH, "%f\n", sv.Effects[index].effect.Rain.wait);
					break;

				case CE_SNOW:
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.min_org[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.min_org[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.min_org[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.max_org[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.max_org[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.max_org[2]);
					fprintf(FH, "%d ", sv.Effects[index].effect.Rain.flags);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.dir[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.dir[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Rain.dir[2]);
					fprintf(FH, "%d ", sv.Effects[index].effect.Rain.count);
					//fprintf(FH, "%d ", sv.Effects[index].effect.Rain.veer);
					break;

				case CE_FOUNTAIN:
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.pos[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.pos[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.pos[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.angle[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.angle[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.angle[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.movedir[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.movedir[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Fountain.movedir[2]);
					fprintf(FH, "%d ", sv.Effects[index].effect.Fountain.color);
					fprintf(FH, "%d\n", sv.Effects[index].effect.Fountain.cnt);
					break;

				case CE_QUAKE:
					fprintf(FH, "%f ", sv.Effects[index].effect.Quake.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Quake.origin[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Quake.origin[2]);
					fprintf(FH, "%f\n", sv.Effects[index].effect.Quake.radius);
					break;

				case CE_WHITE_SMOKE:
				case CE_GREEN_SMOKE:
				case CE_GREY_SMOKE:
				case CE_RED_SMOKE:
				case CE_SLOW_WHITE_SMOKE:
				case CE_TELESMK1:
				case CE_TELESMK2:
				case CE_GHOST:
				case CE_REDCLOUD:
				case CE_ACID_MUZZFL:
				case CE_FLAMESTREAM:
				case CE_FLAMEWALL:
				case CE_FLAMEWALL2:
				case CE_ONFIRE:
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.origin[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.origin[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.velocity[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.velocity[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.velocity[2]);
			/* smoke frame is a mission pack thing only. */
			if (sv.protocol > PROTOCOL_RAVEN_111)
			{
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.framelength);
					fprintf(FH, "%f\n", sv.Effects[index].effect.Smoke.frame);
			}
			else
			{	/* save it in 1.11 style */
					fprintf(FH, "%f\n", sv.Effects[index].effect.Smoke.framelength);
			}
					break;

				case CE_SM_WHITE_FLASH:
				case CE_YELLOWRED_FLASH:
				case CE_BLUESPARK:
				case CE_YELLOWSPARK:
				case CE_SM_CIRCLE_EXP:
				case CE_BG_CIRCLE_EXP:
				case CE_SM_EXPLOSION:
				case CE_LG_EXPLOSION:
				case CE_FLOOR_EXPLOSION:
				case CE_FLOOR_EXPLOSION3:
				case CE_BLUE_EXPLOSION:
				case CE_REDSPARK:
				case CE_GREENSPARK:
				case CE_ICEHIT:
				case CE_MEDUSA_HIT:
				case CE_MEZZO_REFLECT:
				case CE_FLOOR_EXPLOSION2:
				case CE_XBOW_EXPLOSION:
				case CE_NEW_EXPLOSION:
				case CE_MAGIC_MISSILE_EXPLOSION:
				case CE_BONE_EXPLOSION:
				case CE_BLDRN_EXPL:
				case CE_BRN_BOUNCE:
				case CE_LSHOCK:
				case CE_ACID_HIT:
				case CE_ACID_SPLAT:
				case CE_ACID_EXPL:
				case CE_LBALL_EXPL:
				case CE_FIREWALL_SMALL:
				case CE_FIREWALL_MEDIUM:
				case CE_FIREWALL_LARGE:
				case CE_FBOOM:
				case CE_BOMB:
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Smoke.origin[1]);
					fprintf(FH, "%f\n", sv.Effects[index].effect.Smoke.origin[2]);
					break;

				case CE_WHITE_FLASH:
				case CE_BLUE_FLASH:
				case CE_SM_BLUE_FLASH:
				case CE_RED_FLASH:
					fprintf(FH, "%f ", sv.Effects[index].effect.Flash.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Flash.origin[1]);
					fprintf(FH, "%f\n", sv.Effects[index].effect.Flash.origin[2]);
					break;

				case CE_RIDER_DEATH:
					fprintf(FH, "%f ", sv.Effects[index].effect.RD.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.RD.origin[1]);
					fprintf(FH, "%f\n", sv.Effects[index].effect.RD.origin[2]);
					break;

				case CE_GRAVITYWELL:
					fprintf(FH, "%f ", sv.Effects[index].effect.RD.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.RD.origin[1]);
					fprintf(FH, "%f", sv.Effects[index].effect.RD.origin[2]);
					fprintf(FH, "%d", sv.Effects[index].effect.RD.color);
					fprintf(FH, "%f\n", sv.Effects[index].effect.RD.lifetime);
					break;
				case CE_TELEPORTERPUFFS:
					fprintf(FH, "%f ", sv.Effects[index].effect.Teleporter.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Teleporter.origin[1]);
					fprintf(FH, "%f\n", sv.Effects[index].effect.Teleporter.origin[2]);
					break;

				case CE_TELEPORTERBODY:
					fprintf(FH, "%f ", sv.Effects[index].effect.Teleporter.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Teleporter.origin[1]);
					fprintf(FH, "%f\n", sv.Effects[index].effect.Teleporter.origin[2]);
					break;

				case CE_BONESHARD:
				case CE_BONESHRAPNEL:
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.origin[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.origin[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.velocity[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.velocity[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.velocity[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.angle[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.angle[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.angle[2]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.avelocity[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.avelocity[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Missile.avelocity[2]);
					break;

				case CE_CHUNK:
					fprintf(FH, "%f ", sv.Effects[index].effect.Chunk.origin[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Chunk.origin[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Chunk.origin[2]);
					fprintf(FH, "%d ", sv.Effects[index].effect.Chunk.type);
					fprintf(FH, "%f ", sv.Effects[index].effect.Chunk.srcVel[0]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Chunk.srcVel[1]);
					fprintf(FH, "%f ", sv.Effects[index].effect.Chunk.srcVel[2]);
					fprintf(FH, "%d ", sv.Effects[index].effect.Chunk.numChunks);
					break;

				default:
					PR_RunError ("SV_SaveEffect: bad type");
					break;
			}

		}
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void SV_LoadEffects(FILE *FH)
{
	int index,Total,count;

	// Since the map is freshly loaded, clear out any effects as a result of
	// the loading
	SV_ClearEffects();

	fscanf(FH,"Effects: %d\n",&Total);

	for(count=0;count<Total;count++)
	{
		fscanf(FH,"Effect: %d ",&index);
		fscanf(FH,"%d %f: ",&sv.Effects[index].type,&sv.Effects[index].expire_time);

		switch(sv.Effects[index].type)
		{
			case CE_RAIN:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.min_org[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.min_org[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.min_org[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.max_org[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.max_org[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.max_org[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.e_size[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.e_size[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.e_size[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.dir[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.dir[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.dir[2]);
				fscanf(FH, "%d ", &sv.Effects[index].effect.Rain.color);
				fscanf(FH, "%d ", &sv.Effects[index].effect.Rain.count);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.Rain.wait);
				break;

			case CE_SNOW:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.min_org[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.min_org[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.min_org[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.max_org[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.max_org[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.max_org[2]);
				fscanf(FH, "%d ", &sv.Effects[index].effect.Rain.flags);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.dir[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.dir[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Rain.dir[2]);
				fscanf(FH, "%d ", &sv.Effects[index].effect.Rain.count);
				//fscanf(FH, "%d ", &sv.Effects[index].effect.Rain.veer);
				break;

			case CE_FOUNTAIN:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.pos[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.pos[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.pos[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.angle[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.angle[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.angle[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.movedir[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.movedir[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Fountain.movedir[2]);
				fscanf(FH, "%d ", &sv.Effects[index].effect.Fountain.color);
				fscanf(FH, "%d\n", &sv.Effects[index].effect.Fountain.cnt);
				break;

			case CE_QUAKE:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Quake.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Quake.origin[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Quake.origin[2]);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.Quake.radius);
				break;

			case CE_WHITE_SMOKE:
			case CE_GREEN_SMOKE:
			case CE_GREY_SMOKE:
			case CE_RED_SMOKE:
			case CE_SLOW_WHITE_SMOKE:
			case CE_TELESMK1:
			case CE_TELESMK2:
			case CE_GHOST:
			case CE_REDCLOUD:
			case CE_ACID_MUZZFL:
			case CE_FLAMESTREAM:
			case CE_FLAMEWALL:
			case CE_FLAMEWALL2:
			case CE_ONFIRE:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.origin[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.origin[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.velocity[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.velocity[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.velocity[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.framelength);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.Smoke.frame);
				break;

			case CE_SM_WHITE_FLASH:
			case CE_YELLOWRED_FLASH:
			case CE_BLUESPARK:
			case CE_YELLOWSPARK:
			case CE_SM_CIRCLE_EXP:
			case CE_BG_CIRCLE_EXP:
			case CE_SM_EXPLOSION:
			case CE_LG_EXPLOSION:
			case CE_FLOOR_EXPLOSION:
			case CE_FLOOR_EXPLOSION3:
			case CE_BLUE_EXPLOSION:
			case CE_REDSPARK:
			case CE_GREENSPARK:
			case CE_ICEHIT:
			case CE_MEDUSA_HIT:
			case CE_MEZZO_REFLECT:
			case CE_FLOOR_EXPLOSION2:
			case CE_XBOW_EXPLOSION:
			case CE_NEW_EXPLOSION:
			case CE_MAGIC_MISSILE_EXPLOSION:
			case CE_BONE_EXPLOSION:
			case CE_BLDRN_EXPL:
			case CE_BRN_BOUNCE:
			case CE_LSHOCK:
			case CE_ACID_HIT:
			case CE_ACID_SPLAT:
			case CE_ACID_EXPL:
			case CE_LBALL_EXPL:
			case CE_FBOOM:
			case CE_FIREWALL_SMALL:
			case CE_FIREWALL_MEDIUM:
			case CE_FIREWALL_LARGE:
			case CE_BOMB:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Smoke.origin[1]);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.Smoke.origin[2]);
				break;

			case CE_WHITE_FLASH:
			case CE_BLUE_FLASH:
			case CE_SM_BLUE_FLASH:
			case CE_RED_FLASH:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Flash.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Flash.origin[1]);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.Flash.origin[2]);
				break;

			case CE_RIDER_DEATH:
				fscanf(FH, "%f ", &sv.Effects[index].effect.RD.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.RD.origin[1]);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.RD.origin[2]);
				break;

			case CE_GRAVITYWELL:
				fscanf(FH, "%f ", &sv.Effects[index].effect.RD.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.RD.origin[1]);
				fscanf(FH, "%f", &sv.Effects[index].effect.RD.origin[2]);
				fscanf(FH, "%d", &sv.Effects[index].effect.RD.color);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.RD.lifetime);
				break;

			case CE_TELEPORTERPUFFS:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Teleporter.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Teleporter.origin[1]);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.Teleporter.origin[2]);
				break;

			case CE_TELEPORTERBODY:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Teleporter.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Teleporter.origin[1]);
				fscanf(FH, "%f\n", &sv.Effects[index].effect.Teleporter.origin[2]);
				break;

			case CE_BONESHARD:
			case CE_BONESHRAPNEL:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.origin[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.origin[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.velocity[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.velocity[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.velocity[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.angle[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.angle[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.angle[2]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.avelocity[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.avelocity[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Missile.avelocity[2]);
				break;

			case CE_CHUNK:
				fscanf(FH, "%f ", &sv.Effects[index].effect.Chunk.origin[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Chunk.origin[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Chunk.origin[2]);
				fscanf(FH, "%d ", &sv.Effects[index].effect.Chunk.type);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Chunk.srcVel[0]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Chunk.srcVel[1]);
				fscanf(FH, "%f ", &sv.Effects[index].effect.Chunk.srcVel[2]);
				fscanf(FH, "%d ", &sv.Effects[index].effect.Chunk.numChunks);
				break;

			default:
				PR_RunError ("SV_SaveEffect: bad type");
				break;
		}
	}
}
