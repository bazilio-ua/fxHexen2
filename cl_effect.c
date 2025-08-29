/*
	cl_effect.c
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

static int NewEffectEntity(void);
static void FreeEffectEntity(int index);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static entity_t EffectEntities[MAX_EFFECT_ENTITIES];
static qboolean EntityUsed[MAX_EFFECT_ENTITIES];
static int EffectEntityCount;

// CODE --------------------------------------------------------------------

//==========================================================================
//
// CL_InitTEnts
//
//==========================================================================

void CL_InitEffects(void)
{
}

void CL_ClearEffects(void)
{
	memset(cl.Effects,0,sizeof(cl.Effects));
	memset(EntityUsed,0,sizeof(EntityUsed));
	EffectEntityCount = 0;
}

void CL_FreeEffect(int index)
{	
	int i;

	switch(cl.Effects[index].type)
	{
		case CE_RAIN:
			break;

		case CE_SNOW:
			break;

		case CE_FOUNTAIN:
			break;

		case CE_QUAKE:
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
			FreeEffectEntity(cl.Effects[index].effect.Smoke.entity_index);
			break;

		// Just go through animation and then remove
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
		case CE_BOMB:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
			FreeEffectEntity(cl.Effects[index].effect.Smoke.entity_index);
			break;

		// Go forward then backward through animation then remove
		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			FreeEffectEntity(cl.Effects[index].effect.Flash.entity_index);
			break;

		case CE_RIDER_DEATH:
			break;

		case CE_GRAVITYWELL:
			break;

		case CE_TELEPORTERPUFFS:
			for (i=0;i<8;++i)
				FreeEffectEntity(cl.Effects[index].effect.Teleporter.entity_index[i]);
			break;

		case CE_TELEPORTERBODY:
			FreeEffectEntity(cl.Effects[index].effect.Teleporter.entity_index[0]);
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			FreeEffectEntity(cl.Effects[index].effect.Missile.entity_index);
			break;

		case CE_CHUNK:
			//Con_Printf("Freeing a chunk here\n");
			for (i=0;i < cl.Effects[index].effect.Chunk.numChunks;i++)
			{
				if(cl.Effects[index].effect.Chunk.entity_index[i] != -1)
					FreeEffectEntity(cl.Effects[index].effect.Chunk.entity_index[i]);
			}
			break;
	}

	memset(&cl.Effects[index],0,sizeof(struct EffectT));
}

//==========================================================================
//
// CL_ParseEffect
//
//==========================================================================

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void CL_ParseEffect(void)
{
	int index,i;
	qboolean ImmediateFree;
	entity_t *ent;
	int dir;
	float	angleval, sinval, cosval;
	float skinnum;
	float final;

	ImmediateFree = false;

	index = MSG_ReadByte(net_message);
	if (cl.Effects[index].type)
		CL_FreeEffect(index);

	memset(&cl.Effects[index],0,sizeof(struct EffectT));

	cl.Effects[index].type = MSG_ReadByte(net_message);

	switch(cl.Effects[index].type)
	{
		case CE_RAIN:
			cl.Effects[index].effect.Rain.min_org[0] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.min_org[1] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.min_org[2] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.max_org[0] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.max_org[1] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.max_org[2] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.e_size[0] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.e_size[1] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.e_size[2] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.dir[0] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.dir[1] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.dir[2] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.color = MSG_ReadShort(net_message);
			cl.Effects[index].effect.Rain.count = MSG_ReadShort(net_message);
			cl.Effects[index].effect.Rain.wait = MSG_ReadFloat(net_message);
			break;

		case CE_SNOW:
			cl.Effects[index].effect.Rain.min_org[0] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.min_org[1] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.min_org[2] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.max_org[0] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.max_org[1] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.max_org[2] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.flags = MSG_ReadByte(net_message);
			cl.Effects[index].effect.Rain.dir[0] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.dir[1] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.dir[2] = MSG_ReadCoord(net_message);
			cl.Effects[index].effect.Rain.count = MSG_ReadByte(net_message);
			//cl.Effects[index].effect.Rain.veer = MSG_ReadShort(net_message);
			break;

		case CE_FOUNTAIN:
			cl.Effects[index].effect.Fountain.pos[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Fountain.pos[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Fountain.pos[2] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Fountain.angle[0] = MSG_ReadAngle (net_message);
			cl.Effects[index].effect.Fountain.angle[1] = MSG_ReadAngle (net_message);
			cl.Effects[index].effect.Fountain.angle[2] = MSG_ReadAngle (net_message);
			cl.Effects[index].effect.Fountain.movedir[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Fountain.movedir[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Fountain.movedir[2] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Fountain.color = MSG_ReadShort (net_message);
			cl.Effects[index].effect.Fountain.cnt = MSG_ReadByte (net_message);
			AngleVectors (cl.Effects[index].effect.Fountain.angle, 
						  cl.Effects[index].effect.Fountain.vforward,
						  cl.Effects[index].effect.Fountain.vright,
						  cl.Effects[index].effect.Fountain.vup);
			break;

		case CE_QUAKE:
			cl.Effects[index].effect.Quake.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Quake.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Quake.origin[2] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Quake.radius = MSG_ReadFloat (net_message);
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
			cl.Effects[index].effect.Smoke.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Smoke.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Smoke.origin[2] = MSG_ReadCoord (net_message);

			cl.Effects[index].effect.Smoke.velocity[0] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Smoke.velocity[1] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Smoke.velocity[2] = MSG_ReadFloat (net_message);

			cl.Effects[index].effect.Smoke.framelength = MSG_ReadFloat (net_message);
		/* smoke frame is a mission pack thing only. */
		if (cl.protocol > PROTOCOL_RAVEN_111)
			cl.Effects[index].effect.Smoke.frame = MSG_ReadFloat (net_message);

			if ((cl.Effects[index].effect.Smoke.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].effect.Smoke.entity_index];
				VectorCopy(cl.Effects[index].effect.Smoke.origin, ent->origin);

				if ((cl.Effects[index].type == CE_WHITE_SMOKE) || 
					(cl.Effects[index].type == CE_SLOW_WHITE_SMOKE))
					ent->model = Mod_ForName("models/whtsmk1.spr", true);
				else if (cl.Effects[index].type == CE_GREEN_SMOKE)
					ent->model = Mod_ForName("models/grnsmk1.spr", true);
				else if (cl.Effects[index].type == CE_GREY_SMOKE)
					ent->model = Mod_ForName("models/grysmk1.spr", true);
				else if (cl.Effects[index].type == CE_RED_SMOKE)
					ent->model = Mod_ForName("models/redsmk1.spr", true);
				else if (cl.Effects[index].type == CE_TELESMK1)
					ent->model = Mod_ForName("models/telesmk1.spr", true);
				else if (cl.Effects[index].type == CE_TELESMK2)
					ent->model = Mod_ForName("models/telesmk2.spr", true);
				else if (cl.Effects[index].type == CE_REDCLOUD)
					ent->model = Mod_ForName("models/rcloud.spr", true);
				else if (cl.Effects[index].type == CE_FLAMESTREAM)
					ent->model = Mod_ForName("models/flamestr.spr", true);
				else if (cl.Effects[index].type == CE_ACID_MUZZFL)
				{
					ent->model = Mod_ForName("models/muzzle1.spr", true);
					ent->drawflags=DRF_TRANSLUCENT|MLS_ABSLIGHT;
					ent->abslight=0.2;
				}
				else if (cl.Effects[index].type == CE_FLAMEWALL)
					ent->model = Mod_ForName("models/firewal1.spr", true);
				else if (cl.Effects[index].type == CE_FLAMEWALL2)
					ent->model = Mod_ForName("models/firewal2.spr", true);
				else if (cl.Effects[index].type == CE_ONFIRE)
				{
					float rdm = rand() & 3;

					if (rdm < 1)
						ent->model = Mod_ForName("models/firewal1.spr", true);
					else if (rdm < 2)
						ent->model = Mod_ForName("models/firewal2.spr", true);
					else
						ent->model = Mod_ForName("models/firewal3.spr", true);
					
					ent->drawflags = DRF_TRANSLUCENT;
					ent->abslight = 1;
					ent->frame = cl.Effects[index].effect.Smoke.frame;
				}

				if (cl.Effects[index].type != CE_REDCLOUD&&cl.Effects[index].type != CE_ACID_MUZZFL&&cl.Effects[index].type != CE_FLAMEWALL)
					ent->drawflags = DRF_TRANSLUCENT;

				if (cl.Effects[index].type == CE_FLAMESTREAM)
				{
					ent->drawflags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
					ent->abslight = 1;
					ent->frame = cl.Effects[index].effect.Smoke.frame;
				}

				if (cl.Effects[index].type == CE_GHOST)
				{
					ent->model = Mod_ForName("models/ghost.spr", true);
					ent->drawflags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
					ent->abslight = .5;
				}
			}
			else
				ImmediateFree = true;
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
		case CE_BOMB:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
			cl.Effects[index].effect.Smoke.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Smoke.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Smoke.origin[2] = MSG_ReadCoord (net_message);
			if ((cl.Effects[index].effect.Smoke.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].effect.Smoke.entity_index];
				VectorCopy(cl.Effects[index].effect.Smoke.origin, ent->origin);

				if (cl.Effects[index].type == CE_BLUESPARK)
					ent->model = Mod_ForName("models/bspark.spr", true);
				else if (cl.Effects[index].type == CE_YELLOWSPARK)
					ent->model = Mod_ForName("models/spark.spr", true);
				else if (cl.Effects[index].type == CE_SM_CIRCLE_EXP)
					ent->model = Mod_ForName("models/fcircle.spr", true);
				else if (cl.Effects[index].type == CE_BG_CIRCLE_EXP)
					ent->model = Mod_ForName("models/xplod29.spr", true);
				else if (cl.Effects[index].type == CE_SM_WHITE_FLASH)
					ent->model = Mod_ForName("models/sm_white.spr", true);
				else if (cl.Effects[index].type == CE_YELLOWRED_FLASH)
				{
					ent->model = Mod_ForName("models/yr_flsh.spr", true);
					ent->drawflags = DRF_TRANSLUCENT;
				}
				else if (cl.Effects[index].type == CE_SM_EXPLOSION)
					ent->model = Mod_ForName("models/sm_expld.spr", true);
				else if (cl.Effects[index].type == CE_LG_EXPLOSION)
					ent->model = Mod_ForName("models/bg_expld.spr", true);
				else if (cl.Effects[index].type == CE_FLOOR_EXPLOSION)
					ent->model = Mod_ForName("models/fl_expld.spr", true);
				else if (cl.Effects[index].type == CE_FLOOR_EXPLOSION3)
					ent->model = Mod_ForName("models/biggy.spr", true);
				else if (cl.Effects[index].type == CE_BLUE_EXPLOSION)
					ent->model = Mod_ForName("models/xpspblue.spr", true);
				else if (cl.Effects[index].type == CE_REDSPARK)
					ent->model = Mod_ForName("models/rspark.spr", true);
				else if (cl.Effects[index].type == CE_GREENSPARK)
					ent->model = Mod_ForName("models/gspark.spr", true);
				else if (cl.Effects[index].type == CE_ICEHIT)
					ent->model = Mod_ForName("models/icehit.spr", true);
				else if (cl.Effects[index].type == CE_MEDUSA_HIT)
					ent->model = Mod_ForName("models/medhit.spr", true);
				else if (cl.Effects[index].type == CE_MEZZO_REFLECT)
					ent->model = Mod_ForName("models/mezzoref.spr", true);
				else if (cl.Effects[index].type == CE_FLOOR_EXPLOSION2)
					ent->model = Mod_ForName("models/flrexpl2.spr", true);
				else if (cl.Effects[index].type == CE_XBOW_EXPLOSION)
					ent->model = Mod_ForName("models/xbowexpl.spr", true);
				else if (cl.Effects[index].type == CE_NEW_EXPLOSION)
					ent->model = Mod_ForName("models/gen_expl.spr", true);
				else if (cl.Effects[index].type == CE_MAGIC_MISSILE_EXPLOSION)
					ent->model = Mod_ForName("models/mm_expld.spr", true);
				else if (cl.Effects[index].type == CE_BONE_EXPLOSION)
					ent->model = Mod_ForName("models/bonexpld.spr", true);
				else if (cl.Effects[index].type == CE_BLDRN_EXPL)
					ent->model = Mod_ForName("models/xplsn_1.spr", true);
				else if (cl.Effects[index].type == CE_ACID_HIT)
					ent->model = Mod_ForName("models/axplsn_2.spr", true);
				else if (cl.Effects[index].type == CE_ACID_SPLAT)
					ent->model = Mod_ForName("models/axplsn_1.spr", true);
				else if (cl.Effects[index].type == CE_ACID_EXPL)
				{
					ent->model = Mod_ForName("models/axplsn_5.spr", true);
					ent->drawflags = MLS_ABSLIGHT;
					ent->abslight = 1;
				}
				else if (cl.Effects[index].type == CE_FBOOM)
					ent->model = Mod_ForName("models/fboom.spr", true);
				else if (cl.Effects[index].type == CE_BOMB)
					ent->model = Mod_ForName("models/pow.spr", true);
				else if (cl.Effects[index].type == CE_LBALL_EXPL)
					ent->model = Mod_ForName("models/Bluexp3.spr", true);
				else if (cl.Effects[index].type == CE_FIREWALL_SMALL)
					ent->model = Mod_ForName("models/firewal1.spr", true);
				else if (cl.Effects[index].type == CE_FIREWALL_MEDIUM)
					ent->model = Mod_ForName("models/firewal5.spr", true);
				else if (cl.Effects[index].type == CE_FIREWALL_LARGE)
					ent->model = Mod_ForName("models/firewal4.spr", true);
				else if (cl.Effects[index].type == CE_BRN_BOUNCE)
					ent->model = Mod_ForName("models/spark.spr", true);
				else if (cl.Effects[index].type == CE_LSHOCK)
				{
					ent->model = Mod_ForName("models/vorpshok.mdl", true);
					ent->drawflags=MLS_TORCH;
					ent->angles[2]=90;
					ent->scale=255;
				}
			}
			else
			{
				ImmediateFree = true;
			}
			break;

		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			cl.Effects[index].effect.Flash.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Flash.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Flash.origin[2] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Flash.reverse = 0;
			if ((cl.Effects[index].effect.Flash.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].effect.Flash.entity_index];
				VectorCopy(cl.Effects[index].effect.Flash.origin, ent->origin);

				if (cl.Effects[index].type == CE_WHITE_FLASH)
					ent->model = Mod_ForName("models/gryspt.spr", true);
				else if (cl.Effects[index].type == CE_BLUE_FLASH)
					ent->model = Mod_ForName("models/bluflash.spr", true);
				else if (cl.Effects[index].type == CE_SM_BLUE_FLASH)
					ent->model = Mod_ForName("models/sm_blue.spr", true);
				else if (cl.Effects[index].type == CE_RED_FLASH)
					ent->model = Mod_ForName("models/redspt.spr", true);

				ent->drawflags = DRF_TRANSLUCENT;

			}
			else
			{
				ImmediateFree = true;
			}
			break;

		case CE_RIDER_DEATH:
			cl.Effects[index].effect.RD.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.RD.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.RD.origin[2] = MSG_ReadCoord (net_message);
			break;

		case CE_GRAVITYWELL:
			cl.Effects[index].effect.RD.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.RD.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.RD.origin[2] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.RD.color = MSG_ReadShort (net_message);
			cl.Effects[index].effect.RD.lifetime = MSG_ReadFloat (net_message);
			break;

		case CE_TELEPORTERPUFFS:
			cl.Effects[index].effect.Teleporter.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Teleporter.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Teleporter.origin[2] = MSG_ReadCoord (net_message);
				
			cl.Effects[index].effect.Teleporter.framelength = .05;
			dir = 0;
			for (i=0;i<8;++i)
			{		
				if ((cl.Effects[index].effect.Teleporter.entity_index[i] = NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.Effects[index].effect.Teleporter.entity_index[i]];
					VectorCopy(cl.Effects[index].effect.Teleporter.origin, ent->origin);

					angleval = dir * M_PI*2 / 360;

					sinval = sin(angleval);
					cosval = cos(angleval);

					cl.Effects[index].effect.Teleporter.velocity[i][0] = 10*cosval;
					cl.Effects[index].effect.Teleporter.velocity[i][1] = 10*sinval;
					cl.Effects[index].effect.Teleporter.velocity[i][2] = 0;
					dir += 45;

					ent->model = Mod_ForName("models/telesmk2.spr", true);
					ent->drawflags = DRF_TRANSLUCENT;
				}
			}
			break;

		case CE_TELEPORTERBODY:
			cl.Effects[index].effect.Teleporter.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Teleporter.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Teleporter.origin[2] = MSG_ReadCoord (net_message);

			cl.Effects[index].effect.Teleporter.velocity[0][0] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Teleporter.velocity[0][1] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Teleporter.velocity[0][2] = MSG_ReadFloat (net_message);

			skinnum = MSG_ReadFloat (net_message);
			
			cl.Effects[index].effect.Teleporter.framelength = .05;
			dir = 0;
			if ((cl.Effects[index].effect.Teleporter.entity_index[0] = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].effect.Teleporter.entity_index[0]];
				VectorCopy(cl.Effects[index].effect.Teleporter.origin, ent->origin);

				ent->model = Mod_ForName("models/teleport.mdl", true);
				ent->drawflags = SCALE_TYPE_XYONLY | DRF_TRANSLUCENT;
				ent->scale = 100;
				ent->skinnum = skinnum;
			}
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			cl.Effects[index].effect.Missile.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Missile.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Missile.origin[2] = MSG_ReadCoord (net_message);

			cl.Effects[index].effect.Missile.velocity[0] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Missile.velocity[1] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Missile.velocity[2] = MSG_ReadFloat (net_message);

			cl.Effects[index].effect.Missile.angle[0] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Missile.angle[1] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Missile.angle[2] = MSG_ReadFloat (net_message);

			cl.Effects[index].effect.Missile.avelocity[0] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Missile.avelocity[1] = MSG_ReadFloat (net_message);
			cl.Effects[index].effect.Missile.avelocity[2] = MSG_ReadFloat (net_message);

			if ((cl.Effects[index].effect.Missile.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].effect.Missile.entity_index];
				VectorCopy(cl.Effects[index].effect.Missile.origin, ent->origin);
				if (cl.Effects[index].type == CE_BONESHARD)
					ent->model = Mod_ForName("models/boneshot.mdl", true);
				else if (cl.Effects[index].type == CE_BONESHRAPNEL)
					ent->model = Mod_ForName("models/boneshrd.mdl", true);
			}
			else
				ImmediateFree = true;
			break;

		case CE_CHUNK:
			cl.Effects[index].effect.Chunk.origin[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Chunk.origin[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Chunk.origin[2] = MSG_ReadCoord (net_message);

			cl.Effects[index].effect.Chunk.type = MSG_ReadByte (net_message);

			cl.Effects[index].effect.Chunk.srcVel[0] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Chunk.srcVel[1] = MSG_ReadCoord (net_message);
			cl.Effects[index].effect.Chunk.srcVel[2] = MSG_ReadCoord (net_message);

			cl.Effects[index].effect.Chunk.numChunks = MSG_ReadByte (net_message);

			cl.Effects[index].effect.Chunk.time_amount = 4.0;

			cl.Effects[index].effect.Chunk.aveScale = 30 + 100 * (cl.Effects[index].effect.Chunk.numChunks / 40.0);

			if(cl.Effects[index].effect.Chunk.numChunks > 16)cl.Effects[index].effect.Chunk.numChunks = 16;

			for (i=0;i < cl.Effects[index].effect.Chunk.numChunks;i++)
			{		
				if ((cl.Effects[index].effect.Chunk.entity_index[i] = NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.Effects[index].effect.Chunk.entity_index[i]];
					VectorCopy(cl.Effects[index].effect.Chunk.origin, ent->origin);

					VectorCopy(cl.Effects[index].effect.Chunk.srcVel, cl.Effects[index].effect.Chunk.velocity[i]);
					VectorScale(cl.Effects[index].effect.Chunk.velocity[i], .80 + ((rand()%4)/10.0), cl.Effects[index].effect.Chunk.velocity[i]);
					// temp modify them...
					cl.Effects[index].effect.Chunk.velocity[i][0] += (rand()%140)-70;
					cl.Effects[index].effect.Chunk.velocity[i][1] += (rand()%140)-70;
					cl.Effects[index].effect.Chunk.velocity[i][2] += (rand()%140)-70;

					// are these in degrees or radians?
					ent->angles[0] = rand()%360;
					ent->angles[1] = rand()%360;
					ent->angles[2] = rand()%360;

					ent->scale = cl.Effects[index].effect.Chunk.aveScale + rand()%40;

					// make this overcomplicated
					final = (rand()%100)*.01;
					if ((cl.Effects[index].effect.Chunk.type==THINGTYPE_GLASS) || (cl.Effects[index].effect.Chunk.type==THINGTYPE_REDGLASS) || 
							(cl.Effects[index].effect.Chunk.type==THINGTYPE_CLEARGLASS) || (cl.Effects[index].effect.Chunk.type==THINGTYPE_WEBS))
					{
						if (final<0.20)
							ent->model = Mod_ForName ("models/shard1.mdl", true);
						else if (final<0.40)
							ent->model = Mod_ForName ("models/shard2.mdl", true);
						else if (final<0.60)
							ent->model = Mod_ForName ("models/shard3.mdl", true);
						else if (final<0.80)
							ent->model = Mod_ForName ("models/shard4.mdl", true);
						else 
							ent->model = Mod_ForName ("models/shard5.mdl", true);

						if (cl.Effects[index].effect.Chunk.type==THINGTYPE_CLEARGLASS)
						{
							ent->skinnum=1;
							ent->drawflags |= DRF_TRANSLUCENT;
						}
						else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_REDGLASS)
						{
							ent->skinnum=2;
						}
						else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_WEBS)
						{
							ent->skinnum=3;
							ent->drawflags |= DRF_TRANSLUCENT;
						}
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_WOOD)
					{
						if (final < 0.25)
							ent->model = Mod_ForName ("models/splnter1.mdl", true);
						else if (final < 0.50)
							ent->model = Mod_ForName ("models/splnter2.mdl", true);
						else if (final < 0.75)
							ent->model = Mod_ForName ("models/splnter3.mdl", true);
						else 
							ent->model = Mod_ForName ("models/splnter4.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_METAL)
					{
						if (final < 0.25)
							ent->model = Mod_ForName ("models/metlchk1.mdl", true);
						else if (final < 0.50)
							ent->model = Mod_ForName ("models/metlchk2.mdl", true);
						else if (final < 0.75)
							ent->model = Mod_ForName ("models/metlchk3.mdl", true);
						else 
							ent->model = Mod_ForName ("models/metlchk4.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_FLESH)
					{
						if (final < 0.33)
							ent->model = Mod_ForName ("models/flesh1.mdl", true);
						else if (final < 0.66)
							ent->model = Mod_ForName ("models/flesh2.mdl", true);
						else
							ent->model = Mod_ForName ("models/flesh3.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_BROWNSTONE)
					{
						if (final < 0.25)
							ent->model = Mod_ForName ("models/schunk1.mdl", true);
						else if (final < 0.50)
							ent->model = Mod_ForName ("models/schunk2.mdl", true);
						else if (final < 0.75)
							ent->model = Mod_ForName ("models/schunk3.mdl", true);
						else 
							ent->model = Mod_ForName ("models/schunk4.mdl", true);
						ent->skinnum = 1;
					}
					else if ((cl.Effects[index].effect.Chunk.type==THINGTYPE_CLAY) || (cl.Effects[index].effect.Chunk.type==THINGTYPE_BONE))
					{
						if (final < 0.25)
							ent->model = Mod_ForName ("models/clshard1.mdl", true);
						else if (final < 0.50)
							ent->model = Mod_ForName ("models/clshard2.mdl", true);
						else if (final < 0.75)
							ent->model = Mod_ForName ("models/clshard3.mdl", true);
						else 
							ent->model = Mod_ForName ("models/clshard4.mdl", true);
						if (cl.Effects[index].effect.Chunk.type==THINGTYPE_BONE)
						{
							ent->skinnum=1;//bone skin is second
						}
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_LEAVES)
					{
						if (final < 0.33)
							ent->model = Mod_ForName ("models/leafchk1.mdl", true);
						else if (final < 0.66)
							ent->model = Mod_ForName ("models/leafchk2.mdl", true);
						else 
							ent->model = Mod_ForName ("models/leafchk3.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_HAY)
					{
						if (final < 0.33)
							ent->model = Mod_ForName ("models/hay1.mdl", true);
						else if (final < 0.66)
							ent->model = Mod_ForName ("models/hay2.mdl", true);
						else 
							ent->model = Mod_ForName ("models/hay3.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_CLOTH)
					{
						if (final < 0.33)
							ent->model = Mod_ForName ("models/clthchk1.mdl", true);
						else if (final < 0.66)
							ent->model = Mod_ForName ("models/clthchk2.mdl", true);
						else 
							ent->model = Mod_ForName ("models/clthchk3.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_WOOD_LEAF)
					{
						if (final < 0.14)
							ent->model = Mod_ForName ("models/splnter1.mdl", true);
						else if (final < 0.28)
							ent->model = Mod_ForName ("models/leafchk1.mdl", true);
						else if (final < 0.42)
							ent->model = Mod_ForName ("models/splnter2.mdl", true);
						else if (final < 0.56)
							ent->model = Mod_ForName ("models/leafchk2.mdl", true);
						else if (final < 0.70)
							ent->model = Mod_ForName ("models/splnter3.mdl", true);
						else if (final < 0.84)
							ent->model = Mod_ForName ("models/leafchk3.mdl", true);
						else 
							ent->model = Mod_ForName ("models/splnter4.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_WOOD_METAL)
					{
						if (final < 0.125)
							ent->model = Mod_ForName ("models/splnter1.mdl", true);
						else if (final < 0.25)
							ent->model = Mod_ForName ("models/metlchk1.mdl", true);
						else if (final < 0.375)
							ent->model = Mod_ForName ("models/splnter2.mdl", true);
						else if (final < 0.50)
							ent->model = Mod_ForName ("models/metlchk2.mdl", true);
						else if (final < 0.625)
							ent->model = Mod_ForName ("models/splnter3.mdl", true);
						else if (final < 0.75)
							ent->model = Mod_ForName ("models/metlchk3.mdl", true);
						else if (final < 0.875)
							ent->model = Mod_ForName ("models/splnter4.mdl", true);
						else 
							ent->model = Mod_ForName ("models/metlchk4.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_WOOD_STONE)
					{
						if (final < 0.125)
							ent->model = Mod_ForName ("models/splnter1.mdl", true);
						else if (final < 0.25)
							ent->model = Mod_ForName ("models/schunk1.mdl", true);
						else if (final < 0.375)
							ent->model = Mod_ForName ("models/splnter2.mdl", true);
						else if (final < 0.50)
							ent->model = Mod_ForName ("models/schunk2.mdl", true);
						else if (final < 0.625)
							ent->model = Mod_ForName ("models/splnter3.mdl", true);
						else if (final < 0.75)
							ent->model = Mod_ForName ("models/schunk3.mdl", true);
						else if (final < 0.875)
							ent->model = Mod_ForName ("models/splnter4.mdl", true);
						else 
							ent->model = Mod_ForName ("models/schunk4.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_METAL_STONE)
					{
						if (final < 0.125)
							ent->model = Mod_ForName ("models/metlchk1.mdl", true);
						else if (final < 0.25)
							ent->model = Mod_ForName ("models/schunk1.mdl", true);
						else if (final < 0.375)
							ent->model = Mod_ForName ("models/metlchk2.mdl", true);
						else if (final < 0.50)
							ent->model = Mod_ForName ("models/schunk2.mdl", true);
						else if (final < 0.625)
							ent->model = Mod_ForName ("models/metlchk3.mdl", true);
						else if (final < 0.75)
							ent->model = Mod_ForName ("models/schunk3.mdl", true);
						else if (final < 0.875)
							ent->model = Mod_ForName ("models/metlchk4.mdl", true);
						else 
							ent->model = Mod_ForName ("models/schunk4.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_METAL_CLOTH)
					{
						if (final < 0.14)
							ent->model = Mod_ForName ("models/metlchk1.mdl", true);
						else if (final < 0.28)
							ent->model = Mod_ForName ("models/clthchk1.mdl", true);
						else if (final < 0.42)
							ent->model = Mod_ForName ("models/metlchk2.mdl", true);
						else if (final < 0.56)
							ent->model = Mod_ForName ("models/clthchk2.mdl", true);
						else if (final < 0.70)
							ent->model = Mod_ForName ("models/metlchk3.mdl", true);
						else if (final < 0.84)
							ent->model = Mod_ForName ("models/clthchk3.mdl", true);
						else 
							ent->model = Mod_ForName ("models/metlchk4.mdl", true);
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_ICE)
					{
						ent->model = Mod_ForName("models/shard.mdl", true);
						ent->skinnum=0;
						ent->frame = rand()%2;
						ent->drawflags |= DRF_TRANSLUCENT|MLS_ABSLIGHT;
						ent->abslight = 0.5;
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_METEOR)
					{
						ent->model = Mod_ForName("models/tempmetr.mdl", true);
						ent->skinnum = 0;
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_ACID)
					{	// no spinning if possible...
						ent->model = Mod_ForName("models/sucwp2p.mdl", true);
						ent->skinnum = 0;
					}
					else if (cl.Effects[index].effect.Chunk.type==THINGTYPE_GREENFLESH)
					{	// spider guts
						if (final < 0.33)
							ent->model = Mod_ForName ("models/sflesh1.mdl", true);
						else if (final < 0.66)
							ent->model = Mod_ForName ("models/sflesh2.mdl", true);
						else
							ent->model = Mod_ForName ("models/sflesh3.mdl", true);

						ent->skinnum = 0;
					}
					else// if (cl.Effects[index].effect.Chunk.type==THINGTYPE_GREYSTONE)
					{
						if (final < 0.25)
							ent->model = Mod_ForName ("models/schunk1.mdl", true);
						else if (final < 0.50)
							ent->model = Mod_ForName ("models/schunk2.mdl", true);
						else if (final < 0.75)
							ent->model = Mod_ForName ("models/schunk3.mdl", true);
						else 
							ent->model = Mod_ForName ("models/schunk4.mdl", true);
						ent->skinnum = 0;
					}
				}
			}
			for(i=0; i < 3; i++)
			{
				cl.Effects[index].effect.Chunk.avel[i][0] = rand()%850 - 425;
				cl.Effects[index].effect.Chunk.avel[i][1] = rand()%850 - 425;
				cl.Effects[index].effect.Chunk.avel[i][2] = rand()%850 - 425;
			}

			break;

		default:
			Sys_Error ("CL_ParseEffect: bad type");
	}

	if (ImmediateFree)
	{
		cl.Effects[index].type = CE_NONE;
	}
}

void CL_EndEffect(void)
{
	int index;

	index = MSG_ReadByte(net_message);

	CL_FreeEffect(index);
}

void CL_LinkEntity(entity_t *ent)
{
	if (cl_numvisedicts < MAX_VISEDICTS)
	{
		cl_visedicts[cl_numvisedicts++] = ent;
	}
}

void R_RunQuakeEffect (vec3_t org, float distance);
void RiderParticle(int count, vec3_t origin);
void GravityWellParticle(int count, vec3_t origin, int color);

void CL_UpdateEffects(void)
{
	int index,cur_frame;
	vec3_t mymin,mymax;
	float frametime;
	vec3_t	org,org2,alldir;
	int x_dir,y_dir;
	entity_t *ent;
	float distance,smoketime;
	int i;
	vec3_t snow_org;

	if (cls.state == ca_disconnected)
		return;

	frametime = cl.time - cl.oldtime;
	if (!frametime) return;
//	Con_Printf("Here at %f\n",cl.time);

	for(index=0;index<MAX_EFFECTS;index++)
	{
		if (!cl.Effects[index].type) 
			continue;

		switch(cl.Effects[index].type)
		{
			case CE_RAIN:
				org[0] = cl.Effects[index].effect.Rain.min_org[0];
				org[1] = cl.Effects[index].effect.Rain.min_org[1];
				org[2] = cl.Effects[index].effect.Rain.max_org[2];

				org2[0] = cl.Effects[index].effect.Rain.e_size[0];
				org2[1] = cl.Effects[index].effect.Rain.e_size[1];
				org2[2] = cl.Effects[index].effect.Rain.e_size[2];

				x_dir = cl.Effects[index].effect.Rain.dir[0];
				y_dir = cl.Effects[index].effect.Rain.dir[1];
				
				cl.Effects[index].effect.Rain.next_time += frametime;
				if (cl.Effects[index].effect.Rain.next_time >= cl.Effects[index].effect.Rain.wait)
				{		
					R_RainEffect(org,org2,x_dir,y_dir,cl.Effects[index].effect.Rain.color,
						cl.Effects[index].effect.Rain.count);
					cl.Effects[index].effect.Rain.next_time = 0;
				}
				break;

			case CE_SNOW:
				VectorCopy(cl.Effects[index].effect.Rain.min_org,org);
				VectorCopy(cl.Effects[index].effect.Rain.max_org,org2);
				VectorCopy(cl.Effects[index].effect.Rain.dir,alldir);
								
				VectorAdd(org, org2, snow_org);

				snow_org[0] *= 0.5;
				snow_org[1] *= 0.5;
				snow_org[2] *= 0.5;

				snow_org[2] = r_origin[2];

				VectorSubtract(snow_org, r_origin, snow_org);
				
				distance = VectorNormalize(snow_org);
				
				cl.Effects[index].effect.Rain.next_time += frametime;
				//jfm:  fixme, check distance to player first
				if (cl.Effects[index].effect.Rain.next_time >= 0.10 && distance < 1024)
				{		
					R_SnowEffect(org,org2,cl.Effects[index].effect.Rain.flags,alldir,
								 cl.Effects[index].effect.Rain.count);

					cl.Effects[index].effect.Rain.next_time = 0;
				}
				break;

			case CE_FOUNTAIN:
				mymin[0] = (-3 * cl.Effects[index].effect.Fountain.vright[0] * cl.Effects[index].effect.Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].effect.Fountain.vforward[0] * cl.Effects[index].effect.Fountain.movedir[1]) +
						   (2 * cl.Effects[index].effect.Fountain.vup[0] * cl.Effects[index].effect.Fountain.movedir[2]);
				mymin[1] = (-3 * cl.Effects[index].effect.Fountain.vright[1] * cl.Effects[index].effect.Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].effect.Fountain.vforward[1] * cl.Effects[index].effect.Fountain.movedir[1]) +
						   (2 * cl.Effects[index].effect.Fountain.vup[1] * cl.Effects[index].effect.Fountain.movedir[2]);
				mymin[2] = (-3 * cl.Effects[index].effect.Fountain.vright[2] * cl.Effects[index].effect.Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].effect.Fountain.vforward[2] * cl.Effects[index].effect.Fountain.movedir[1]) +
						   (2 * cl.Effects[index].effect.Fountain.vup[2] * cl.Effects[index].effect.Fountain.movedir[2]);
				mymin[0] *= 15;
				mymin[1] *= 15;
				mymin[2] *= 15;

				mymax[0] = (3 * cl.Effects[index].effect.Fountain.vright[0] * cl.Effects[index].effect.Fountain.movedir[0]) +
						   (3 * cl.Effects[index].effect.Fountain.vforward[0] * cl.Effects[index].effect.Fountain.movedir[1]) +
						   (10 * cl.Effects[index].effect.Fountain.vup[0] * cl.Effects[index].effect.Fountain.movedir[2]);
				mymax[1] = (3 * cl.Effects[index].effect.Fountain.vright[1] * cl.Effects[index].effect.Fountain.movedir[0]) +
						   (3 * cl.Effects[index].effect.Fountain.vforward[1] * cl.Effects[index].effect.Fountain.movedir[1]) +
						   (10 * cl.Effects[index].effect.Fountain.vup[1] * cl.Effects[index].effect.Fountain.movedir[2]);
				mymax[2] = (3 * cl.Effects[index].effect.Fountain.vright[2] * cl.Effects[index].effect.Fountain.movedir[0]) +
						   (3 * cl.Effects[index].effect.Fountain.vforward[2] * cl.Effects[index].effect.Fountain.movedir[1]) +
						   (10 * cl.Effects[index].effect.Fountain.vup[2] * cl.Effects[index].effect.Fountain.movedir[2]);
				mymax[0] *= 15;
				mymax[1] *= 15;
				mymax[2] *= 15;

				R_RunParticleEffect2 (cl.Effects[index].effect.Fountain.pos,mymin,mymax,
					                  cl.Effects[index].effect.Fountain.color,2,cl.Effects[index].effect.Fountain.cnt);

/*				memset(&test,0,sizeof(test));
				trace = SV_Move (cl.Effects[index].effect.Fountain.pos, mymin, mymax, mymin, false, &test);
				Con_Printf("Fraction is %f\n",trace.fraction);*/
				break;

			case CE_QUAKE:
				R_RunQuakeEffect (cl.Effects[index].effect.Quake.origin,cl.Effects[index].effect.Quake.radius);
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
				cl.Effects[index].effect.Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].effect.Smoke.entity_index];

				smoketime = cl.Effects[index].effect.Smoke.framelength;
				if (!smoketime)
					smoketime = HX_FRAME_TIME;

				ent->origin[0] += (frametime/smoketime) * cl.Effects[index].effect.Smoke.velocity[0];
				ent->origin[1] += (frametime/smoketime) * cl.Effects[index].effect.Smoke.velocity[1];
				ent->origin[2] += (frametime/smoketime) * cl.Effects[index].effect.Smoke.velocity[2];

				while(cl.Effects[index].effect.Smoke.time_amount >= smoketime)
				{
					ent->frame++;
					cl.Effects[index].effect.Smoke.time_amount -= smoketime;
				}

				if (ent->frame >= ent->model->numframes)
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);

				break;

			// Just go through animation and then remove
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
			case CE_ACID_HIT:
			case CE_ACID_SPLAT:
			case CE_ACID_EXPL:
			case CE_LBALL_EXPL:
			case CE_FBOOM:
			case CE_BOMB:
			case CE_FIREWALL_SMALL:
			case CE_FIREWALL_MEDIUM:
			case CE_FIREWALL_LARGE:

				cl.Effects[index].effect.Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].effect.Smoke.entity_index];

				if (cl.Effects[index].type != CE_BG_CIRCLE_EXP)
				{
					while(cl.Effects[index].effect.Smoke.time_amount >= HX_FRAME_TIME)
					{
						ent->frame++;
						cl.Effects[index].effect.Smoke.time_amount -= HX_FRAME_TIME;
					}
				}
				else
				{
					while(cl.Effects[index].effect.Smoke.time_amount >= HX_FRAME_TIME * 2)
					{
						ent->frame++;
						cl.Effects[index].effect.Smoke.time_amount -= HX_FRAME_TIME * 2;
					}
				}
				if (ent->frame >= ent->model->numframes)
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;


			case CE_LSHOCK:
				ent = &EffectEntities[cl.Effects[index].effect.Smoke.entity_index];
				if(ent->skinnum==0)
					ent->skinnum=1;
				else if(ent->skinnum==1)
					ent->skinnum=0;
				ent->scale-=10;
				if (ent->scale<=10)
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;

			// Go forward then backward through animation then remove
			case CE_WHITE_FLASH:
			case CE_BLUE_FLASH:
			case CE_SM_BLUE_FLASH:
			case CE_RED_FLASH:
				cl.Effects[index].effect.Flash.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].effect.Flash.entity_index];

				while(cl.Effects[index].effect.Flash.time_amount >= HX_FRAME_TIME)
				{
					if (!cl.Effects[index].effect.Flash.reverse)
					{
						if (ent->frame >= ent->model->numframes-1)  // Ran through forward animation
						{
							cl.Effects[index].effect.Flash.reverse = 1;
							ent->frame--;
						}
						else
							ent->frame++;

					}	
					else
						ent->frame--;

					cl.Effects[index].effect.Flash.time_amount -= HX_FRAME_TIME;
				}

				if ((ent->frame <= 0) && (cl.Effects[index].effect.Flash.reverse))
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;

			case CE_RIDER_DEATH:
				cl.Effects[index].effect.RD.time_amount += frametime;
				if (cl.Effects[index].effect.RD.time_amount >= 1)
				{
					cl.Effects[index].effect.RD.stage++;
					cl.Effects[index].effect.RD.time_amount -= 1;
				}

				VectorCopy(cl.Effects[index].effect.RD.origin,org);
				org[0] += sin(cl.Effects[index].effect.RD.time_amount * 2 * M_PI) * 30;
				org[1] += cos(cl.Effects[index].effect.RD.time_amount * 2 * M_PI) * 30;

				if (cl.Effects[index].effect.RD.stage <= 6)
//					RiderParticle(cl.Effects[index].effect.RD.stage+1,cl.Effects[index].effect.RD.origin);
					RiderParticle(cl.Effects[index].effect.RD.stage+1,org);
				else
				{
					// To set the rider's origin point for the particles
					RiderParticle(0,org);
					if (cl.Effects[index].effect.RD.stage == 7) 
					{
						cl.cshifts[CSHIFT_BONUS].destcolor[0] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[1] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[2] = 255;
						cl.cshifts[CSHIFT_BONUS].percent = 256;
					}
					else if (cl.Effects[index].effect.RD.stage > 13) 
					{
//						cl.Effects[index].effect.RD.stage = 0;
						CL_FreeEffect(index);
					}
				}
				break;

			case CE_GRAVITYWELL:
			
				cl.Effects[index].effect.RD.time_amount += frametime*2;
				if (cl.Effects[index].effect.RD.time_amount >= 1)
					cl.Effects[index].effect.RD.time_amount -= 1;
		
				VectorCopy(cl.Effects[index].effect.RD.origin,org);
				org[0] += sin(cl.Effects[index].effect.RD.time_amount * 2 * M_PI) * 30;
				org[1] += cos(cl.Effects[index].effect.RD.time_amount * 2 * M_PI) * 30;

				if (cl.Effects[index].effect.RD.lifetime < cl.time)
				{
					CL_FreeEffect(index);
				}
				else
					GravityWellParticle(rand()%8,org, cl.Effects[index].effect.RD.color);

				break;

			case CE_TELEPORTERPUFFS:
				cl.Effects[index].effect.Teleporter.time_amount += frametime;
				smoketime = cl.Effects[index].effect.Teleporter.framelength;

				ent = &EffectEntities[cl.Effects[index].effect.Teleporter.entity_index[0]];
				while(cl.Effects[index].effect.Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->frame++;
					cl.Effects[index].effect.Teleporter.time_amount -= HX_FRAME_TIME;
				}
				cur_frame = ent->frame;

				if (cur_frame >= ent->model->numframes)
				{
					CL_FreeEffect(index);
					break;
				}

				for (i=0;i<8;++i)
				{
					ent = &EffectEntities[cl.Effects[index].effect.Teleporter.entity_index[i]];

					ent->origin[0] += (frametime/smoketime) * cl.Effects[index].effect.Teleporter.velocity[i][0];
					ent->origin[1] += (frametime/smoketime) * cl.Effects[index].effect.Teleporter.velocity[i][1];
					ent->origin[2] += (frametime/smoketime) * cl.Effects[index].effect.Teleporter.velocity[i][2];
					ent->frame = cur_frame;

					CL_LinkEntity(ent);
				}
				break;

			case CE_TELEPORTERBODY:
				cl.Effects[index].effect.Teleporter.time_amount += frametime;
				smoketime = cl.Effects[index].effect.Teleporter.framelength;

				ent = &EffectEntities[cl.Effects[index].effect.Teleporter.entity_index[0]];
				while(cl.Effects[index].effect.Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->scale -= 15;
					cl.Effects[index].effect.Teleporter.time_amount -= HX_FRAME_TIME;
				}

				ent = &EffectEntities[cl.Effects[index].effect.Teleporter.entity_index[0]];
				ent->angles[1] += 45;

				if (ent->scale <= 10)
				{
					CL_FreeEffect(index);
				}
				else
				{
					CL_LinkEntity(ent);
				}
				break;

			case CE_BONESHARD:
			case CE_BONESHRAPNEL:
				cl.Effects[index].effect.Missile.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].effect.Missile.entity_index];

//		ent->angles[0] = cl.Effects[index].effect.Missile.angle[0];
//		ent->angles[1] = cl.Effects[index].effect.Missile.angle[1];
//		ent->angles[2] = cl.Effects[index].effect.Missile.angle[2];

				ent->angles[0] += frametime * cl.Effects[index].effect.Missile.avelocity[0];
				ent->angles[1] += frametime * cl.Effects[index].effect.Missile.avelocity[1];
				ent->angles[2] += frametime * cl.Effects[index].effect.Missile.avelocity[2];

				ent->origin[0] += frametime * cl.Effects[index].effect.Missile.velocity[0];
				ent->origin[1] += frametime * cl.Effects[index].effect.Missile.velocity[1];
				ent->origin[2] += frametime * cl.Effects[index].effect.Missile.velocity[2];

				CL_LinkEntity(ent);
				break;

			case CE_CHUNK:
				cl.Effects[index].effect.Chunk.time_amount -= frametime;
				if(cl.Effects[index].effect.Chunk.time_amount < 0)
				{
					CL_FreeEffect(index);	
				}
				else
				{
					for (i=0;i < cl.Effects[index].effect.Chunk.numChunks;i++)
					{
						vec3_t oldorg;
						mleaf_t		*l;
						int			moving = 1;

						ent = &EffectEntities[cl.Effects[index].effect.Chunk.entity_index[i]];

						VectorCopy(ent->origin, oldorg);

						ent->origin[0] += frametime * cl.Effects[index].effect.Chunk.velocity[i][0];
						ent->origin[1] += frametime * cl.Effects[index].effect.Chunk.velocity[i][1];
						ent->origin[2] += frametime * cl.Effects[index].effect.Chunk.velocity[i][2];

						l = Mod_PointInLeaf (ent->origin, cl.worldmodel);
						if(l->contents!=CONTENTS_EMPTY) //||in_solid==true
						{	// bouncing prolly won't work...
							VectorCopy(oldorg, ent->origin);

							cl.Effects[index].effect.Chunk.velocity[i][0] = 0;
							cl.Effects[index].effect.Chunk.velocity[i][1] = 0;
							cl.Effects[index].effect.Chunk.velocity[i][2] = 0;

							moving = 0;
						}
						else
						{
							ent->angles[0] += frametime * cl.Effects[index].effect.Chunk.avel[i%3][0];
							ent->angles[1] += frametime * cl.Effects[index].effect.Chunk.avel[i%3][1];
							ent->angles[2] += frametime * cl.Effects[index].effect.Chunk.avel[i%3][2];
						}

						if(cl.Effects[index].effect.Chunk.time_amount < frametime * 3)
						{	// chunk leaves in 3 frames
							ent->scale *= .7;
						}

						CL_LinkEntity(ent);

						cl.Effects[index].effect.Chunk.velocity[i][2] -= frametime * 500; // apply gravity

						switch(cl.Effects[index].effect.Chunk.type)
						{
						case THINGTYPE_GREYSTONE:
							break;
						case THINGTYPE_WOOD:
							break;
						case THINGTYPE_METAL:
							break;
						case THINGTYPE_FLESH:
							if(moving)R_RocketTrail (oldorg, ent->origin, 17);
							break;
						case THINGTYPE_FIRE:
							break;
						case THINGTYPE_CLAY:
						case THINGTYPE_BONE:
							break;
						case THINGTYPE_LEAVES:
							break;
						case THINGTYPE_HAY:
							break;
						case THINGTYPE_BROWNSTONE:
							break;
						case THINGTYPE_CLOTH:
							break;
						case THINGTYPE_WOOD_LEAF:
							break;
						case THINGTYPE_WOOD_METAL:
							break;
						case THINGTYPE_WOOD_STONE:
							break;
						case THINGTYPE_METAL_STONE:
							break;
						case THINGTYPE_METAL_CLOTH:
							break;
						case THINGTYPE_WEBS:
							break;
						case THINGTYPE_GLASS:
							break;
						case THINGTYPE_ICE:
							if(moving)R_RocketTrail (oldorg, ent->origin, rt_ice);
							break;
						case THINGTYPE_CLEARGLASS:
							break;
						case THINGTYPE_REDGLASS:
							break;
						case THINGTYPE_ACID:
							if(moving)R_RocketTrail (oldorg, ent->origin, rt_acidball);
							break;
						case THINGTYPE_METEOR:
							R_RocketTrail (oldorg, ent->origin, 1);
							break;
						case THINGTYPE_GREENFLESH:
							if(moving)R_RocketTrail (oldorg, ent->origin, rt_acidball);
							break;

						}
					}
				}
				break;
		}
	}
}

//==========================================================================
//
// NewEffectEntity
//
//==========================================================================

static int NewEffectEntity(void)
{
	entity_t	*ent;
	int counter;

	if(cl_numvisedicts == MAX_VISEDICTS)
	{
		return -1;
	}
	if(EffectEntityCount == MAX_EFFECT_ENTITIES)
	{
		return -1;
	}

	for(counter=0;counter<MAX_EFFECT_ENTITIES;counter++)
		if (!EntityUsed[counter]) 
			break;

	EntityUsed[counter] = true;
	EffectEntityCount++;
	ent = &EffectEntities[counter];
	memset(ent, 0, sizeof(*ent));
	ent->colormap = 0; //vid.colormap;

	return counter;
}

static void FreeEffectEntity(int index)
{
	EntityUsed[index] = false;
	EffectEntityCount--;
}

