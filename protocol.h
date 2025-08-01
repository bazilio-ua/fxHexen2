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
// protocol.h -- communications protocols

#define	PROTOCOL_RAVEN_107		15	/* cd version, aka 1.03 (not supported) */
#define	PROTOCOL_RAVEN_109		17	/* official 1.09 update (not supported) */
#define	PROTOCOL_RAVEN_111		18	// official 1.11 update, without mission pack
#define	PROTOCOL_RAVEN_112		19	// official 1.12, with mission pack
#define	PROTOCOL_UQE_113		20	// Korax UQE patch 1.13

// the default protocol
#define	PROTOCOL_VERSION		(PROTOCOL_RAVEN_112) // Standard Hexen II protocol

// if the high bit of the servercmd is set, the low bits are fast update flags:
#define	U_MOREBITS	(1<<0)
#define	U_ORIGIN1	(1<<1)
#define	U_ORIGIN2	(1<<2)
#define	U_ORIGIN3	(1<<3)
#define	U_ANGLE2	(1<<4)
#define	U_STEP		(1<<5)		// was U_NOLERP, renamed since it's only used for MOVETYPE_STEP
#define	U_FRAME		(1<<6)
#define U_SIGNAL	(1<<7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define	U_ANGLE1		(1<<8)
#define	U_ANGLE3		(1<<9)
#define	U_MODEL			(1<<10)
#define	U_CLEAR_ENT		(1<<11)
#define U_ENT_OFF       (1<<13)
#define	U_LONGENTITY	(1<<14)
#define U_MOREBITS2     (1<<15)

#define	U_SKIN			(1<<16)
#define	U_EFFECTS		(1<<17)
#define U_SCALE			(1<<18)
#define	U_COLORMAP		(1<<19)

#define BE_ON			(1<<0)

#define	SU_VIEWHEIGHT	(1<<0)
#define	SU_IDEALPITCH	(1<<1)
#define	SU_PUNCH1		(1<<2)
#define	SU_PUNCH2		(1<<3)
#define	SU_PUNCH3		(1<<4)
#define	SU_VELOCITY1	(1<<5)
#define	SU_VELOCITY2	(1<<6)
#define	SU_VELOCITY3	(1<<7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define	SU_IDEALROLL	(1<<8)  // I'll take that available bit
#define	SU_SC1			(1<<9)
#define	SU_ONGROUND		(1<<10)		// no data follows, the bit is it
#define	SU_INWATER		(1<<11)		// no data follows, the bit is it
#define	SU_WEAPONFRAME	(1<<12)
#define	SU_ARMOR		(1<<13)
#define	SU_WEAPON		(1<<14)
#define	SU_SC2			(1<<15)

// a sound with no channel is a local only sound
#define	SND_VOLUME		(1<<0)		// a byte
#define	SND_ATTENUATION	(1<<1)		// a byte
#define	SND_OVERFLOW	(1<<2)		// add 255 to snd num
//gonna use the rest of the bits to pack the ent+channel


// johnfitz -- PROTOCOL_FITZQUAKE -- alpha encoding
#define ENTALPHA_DEFAULT	0	//entity's alpha is "default" (i.e. water obeys r_wateralpha) -- must be zero so zeroed out memory works
#define ENTALPHA_ZERO		1	//entity is invisible (lowest possible alpha)
#define ENTALPHA_ONE		255 //entity is fully opaque (highest possible alpha)
#define ENTALPHA_ENCODE(a)	(((a)==0)?ENTALPHA_DEFAULT:Q_rint(CLAMP(1,(a)*254.0f+1,255))) //server convert to byte to send to client
#define ENTALPHA_DECODE(a)	(((a)==ENTALPHA_DEFAULT)?1.0f:((float)(a)-1)/(254)) //client convert to float for rendering
#define ENTALPHA_TOSAVE(a)	(((a)==ENTALPHA_DEFAULT)?0.0f:(((a)==ENTALPHA_ZERO)?-1.0f:((float)(a)-1)/(254))) //server convert to float for savegame
// johnfitz


// Bits to help send server info about the client's edict variables
#define SC1_HEALTH				(1<<0)		// changes stat bar
#define SC1_LEVEL				(1<<1)		// changes stat bar
#define SC1_INTELLIGENCE		(1<<2)		// changes stat bar
#define SC1_WISDOM				(1<<3)		// changes stat bar
#define SC1_STRENGTH			(1<<4)		// changes stat bar
#define SC1_DEXTERITY			(1<<5)		// changes stat bar
#define SC1_WEAPON				(1<<6)		// changes stat bar
#define SC1_BLUEMANA			(1<<7)		// changes stat bar
#define SC1_GREENMANA			(1<<8)		// changes stat bar
#define SC1_EXPERIENCE			(1<<9)		// changes stat bar
#define SC1_CNT_TORCH			(1<<10)		// changes stat bar
#define SC1_CNT_H_BOOST			(1<<11)		// changes stat bar
#define SC1_CNT_SH_BOOST		(1<<12)		// changes stat bar
#define SC1_CNT_MANA_BOOST		(1<<13)		// changes stat bar
#define SC1_CNT_TELEPORT		(1<<14)		// changes stat bar
#define SC1_CNT_TOME			(1<<15)		// changes stat bar
#define SC1_CNT_SUMMON			(1<<16)		// changes stat bar
#define SC1_CNT_INVISIBILITY	(1<<17)		// changes stat bar
#define SC1_CNT_GLYPH			(1<<18)		// changes stat bar
#define SC1_CNT_HASTE			(1<<19)		// changes stat bar
#define SC1_CNT_BLAST			(1<<20)		// changes stat bar
#define SC1_CNT_POLYMORPH		(1<<21)		// changes stat bar
#define SC1_CNT_FLIGHT			(1<<22)		// changes stat bar
#define SC1_CNT_CUBEOFFORCE		(1<<23)		// changes stat bar
#define SC1_CNT_INVINCIBILITY	(1<<24)		// changes stat bar
#define SC1_ARTIFACT_ACTIVE		(1<<25)
#define SC1_ARTIFACT_LOW		(1<<26)
#define SC1_MOVETYPE			(1<<27)
#define SC1_CAMERAMODE			(1<<28)
#define SC1_HASTED				(1<<29)
#define SC1_INVENTORY			(1<<30)
#define SC1_RINGS_ACTIVE		(1<<31)

#define SC2_RINGS_LOW			(1<<0)
#define SC2_AMULET				(1<<1)
#define SC2_BRACER				(1<<2)
#define SC2_BREASTPLATE			(1<<3)
#define SC2_HELMET				(1<<4)
#define SC2_FLIGHT_T			(1<<5)
#define SC2_WATER_T				(1<<6)
#define SC2_TURNING_T			(1<<7)
#define SC2_REGEN_T				(1<<8)
#define SC2_HASTE_T				(1<<9)
#define SC2_TOME_T				(1<<10)
#define SC2_PUZZLE1				(1<<11)
#define SC2_PUZZLE2				(1<<12)
#define SC2_PUZZLE3				(1<<13)
#define SC2_PUZZLE4				(1<<14)
#define SC2_PUZZLE5				(1<<15)
#define SC2_PUZZLE6				(1<<16)
#define SC2_PUZZLE7				(1<<17)
#define SC2_PUZZLE8				(1<<18)
#define SC2_MAXHEALTH			(1<<19)
#define SC2_MAXMANA				(1<<20)
#define SC2_FLAGS				(1<<21)
#define SC2_OBJ					(1<<22)
#define SC2_OBJ2				(1<<23)


// This is to mask out those items that need to generate a stat bar change
#define SC1_STAT_BAR   0x01ffffff
#define SC2_STAT_BAR   0x0

// This is to mask out those items in the inventory (for inventory changes)
#define SC1_INV 0x01fffc00
#define SC2_INV 0x00000000

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	22
#define	DEFAULT_ITEMS		16385


// game types sent by serverinfo
// these determine which intermission screen plays
#define	GAME_COOP			0
#define	GAME_DEATHMATCH		1

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
#define	svc_bad						0
#define	svc_nop						1
#define	svc_disconnect				2
#define	svc_updatestat				3	// [byte] [long]
#define	svc_version					4	// [long] server version
#define	svc_setview					5	// [short] entity number
#define	svc_sound					6	// <see code>
#define	svc_time					7	// [float] server time
#define	svc_print					8	// [string] null terminated string
#define	svc_stufftext				9	// [string] stuffed into client's console buffer
										// the string should be \n terminated
#define	svc_setangle				10	// [angle3] set the view angle to this absolute value
	
#define	svc_serverinfo				11	// [long] version
										// [string] signon string
										// [string]..[0]model cache
										// [string]...[0]sounds cache
#define	svc_lightstyle				12	// [byte] [string]
#define	svc_updatename				13	// [byte] [string]
#define	svc_updatefrags				14	// [byte] [short]
#define	svc_clientdata				15	// <shortbits + data>
#define	svc_stopsound				16	// <see code>
#define	svc_updatecolors			17	// [byte] [byte]
#define	svc_particle				18	// [vec3] <variable>
#define	svc_damage					19
	
#define	svc_spawnstatic				20

#define	svc_raineffect				21

#define	svc_spawnbaseline			22
	
#define	svc_temp_entity				23

#define	svc_setpause				24	// [byte] on / off
#define	svc_signonnum				25	// [byte]  used for the signon sequence

#define	svc_centerprint				26	// [string] to put in center of the screen

#define	svc_killedmonster			27
#define	svc_foundsecret				28

#define	svc_spawnstaticsound		29	// [coord3] [byte] samp [byte] vol [byte] aten

#define	svc_intermission			30		// [string] music
#define	svc_finale					31		// [string] music [string] text

#define	svc_cdtrack					32		// [byte] track [byte] looptrack
#define svc_sellscreen				33
#define	svc_particle2				34	// [vec3] <variable>
#define svc_cutscene				35
#define	svc_midi_name				36		// [string] name
#define	svc_updateclass				37		// [byte] [byte]
#define	svc_particle3				38	
#define	svc_particle4				39	
#define svc_set_view_flags			40
#define svc_clear_view_flags		41
#define svc_start_effect			42
#define svc_end_effect				43
#define svc_plaque					44
#define svc_particle_explosion		45
#define svc_set_view_tint			46
#define svc_reference				47
#define svc_clear_edicts			48
#define svc_update_inv				49

#define	svc_setangle_interpolate	50
#define svc_update_kingofhill		51
#define svc_toggle_statbar			52
#define svc_sound_update_pos		53	//[short] ent+channel [coord3] pos
#define	svc_mod_name		54	// [string] name (UQE v1.13 by Korax, music file name)
#define	svc_skybox		55	// [string] name (UQE v1.13 by Korax, skybox name)


//
// client to server
//
#define	clc_bad			0
#define	clc_nop 		1
#define	clc_disconnect	2
#define	clc_move		3			// [usercmd_t]
#define	clc_stringcmd	4		// [string] message
#define clc_inv_select  5
#define clc_frame		6


//
// temp entity events
//


//#define BASE_ENT_ON		1
//#define BASE_ENT_SENT	2

// moved from quakedef.h
typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;

	byte	ClearCount[32];

} entity_state_t;

typedef struct
{
	byte	flags;
	short	index;

	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;
} entity_state2_t;

typedef struct
{
	byte	flags;

	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;
} entity_state3_t;

#define MAX_CLIENT_STATES 150
#define MAX_FRAMES 5
#define MAX_CLIENTS 8
#define CLEAR_LIMIT 2

#define ENT_STATE_ON		1
#define ENT_CLEARED			2

typedef struct
{
	entity_state2_t states[MAX_CLIENT_STATES];
//	unsigned long frame;
//	unsigned long flags;
	int count;
} client_frames_t;

typedef struct
{
	entity_state2_t states[MAX_CLIENT_STATES*2];
	int count;
} client_frames2_t;

typedef struct
{
	client_frames_t frames[MAX_FRAMES+2]; // 0 = base, 1-max = proposed, max+1 = too late
} client_state2_t;

