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
// quakedef.h -- primary header for client and server


// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All rights reserved.
// Hexen II is a trademark of Raven Software, Inc., (c) 1997 Raven Software, Inc. All rights reserved.


// disable data conversion warnings for MSVC++ 
#ifdef _MSC_VER
#pragma warning(disable : 4244)	// double|int to float truncation
#pragma warning(disable : 4305)	// const double to float truncation
//#pragma warning(disable : 4018)	// signed/unsigned mismatch
//#pragma warning(disable : 4996)	// deprecated
#endif  


#define VERSION				1.12

#define	GAMENAME	"data1"		// directory to look in by default

#include <math.h>
#include <string.h>
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <errno.h>


// MS Visual Studio provides stdint.h only starting with version 2010
#if defined(_MSC_VER) && (_MSC_VER < 1600)
#include "msstdint.h"
#else
#include <stdint.h>
#endif 

// MSVC++ has a different name for several standard functions
#ifdef _MSC_VER
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define inline __inline
#endif 


#define	MINIMUM_MEMORY			0x1800000 // 24 Mb, was 0x550000 (5.6 Mb)
#define	MINIMUM_MEMORY_LEVELPAK	(MINIMUM_MEMORY + 0x400000) // +4 Mb, was 0x100000 (+1 Mb) 

#define DEFAULT_MEMORY_SIZE		64	// default memory size in Mb

#define MAX_NUM_ARGVS	50

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


// Timing macros
#define HX_FRAME_TIME		0.05
#define HX_FPS				20


#define	MAX_QPATH		64			// max length of a quake game pathname
#define	MAX_OSPATH		1024			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon
#define	DIST_EPSILON		(0.03125)	// 1/32 epsilon to keep floating point happy (moved from world.c)

//#define	MAX_MSGLEN		8000		// max length of a reliable message
//#define	MAX_MSGLEN		16000		// max length of a reliable message
#define	MAX_MSGLEN		20000		// for mission pack tibet2

#define	MAX_DATAGRAM	1024		// max length of unreliable message
//#define	MAX_DATAGRAM	2048		// max length of unreliable message  TEMP: This only for E3

#define	MAX_PRINTMSG	4096

//
// per-level limits
//
#define MAX_EDICTS      600	// More than 8192 requires protocol change
#define	MAX_LIGHTSTYLES	64

#define	MAX_MODELS	512		/* Sent over the net as a word */
#define	MAX_SOUNDS_OLD	256		/* Hexen2 v1.11 (protocol 18) and older: sent as a byte	*/
#define	MAX_SOUNDS_H2MP	512		/* Mission Pack (protocol 19), messy thing:		*/
					/* SV_StartSound sends it as a byte, but PF_ambientsound
					   sends it as a word.					*/
#define	MAX_SOUNDS	(MAX_SOUNDS_H2MP)


#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		32
//#define	STAT_HEALTH			0
#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
//#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
//#define	STAT_SHELLS			6
//#define	STAT_NAILS			7
//#define	STAT_ROCKETS		8
//#define	STAT_CELLS			9
//#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster
//#define	STAT_BLUEMANA			15
//#define	STAT_GREENMANA			16
//#define	STAT_EXPERIENCE		17


#define	MAX_INVENTORY			15		// Max inventory array size

// stock defines

#define	IT_SHOTGUN				1
#define	IT_SUPER_SHOTGUN		2
#define	IT_NAILGUN				4
#define	IT_SUPER_NAILGUN		8
#define	IT_GRENADE_LAUNCHER		16
#define	IT_ROCKET_LAUNCHER		32
#define	IT_LIGHTNING			64
#define IT_EXTRA_WEAPON         128 // was IT_SUPER_LIGHTNING, unused at id
#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048
#define IT_AXE                  4096
#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768
#define IT_SUPERHEALTH          65536
#define IT_KEY1                 131072
#define IT_KEY2                 262144
#define	IT_INVISIBILITY			524288
#define	IT_INVULNERABILITY		1048576
#define	IT_SUIT					2097152
#define	IT_QUAD					4194304
#define IT_SIGIL1               (1<<28)
#define IT_SIGIL2               (1<<29)
#define IT_SIGIL3               (1<<30)
#define IT_SIGIL4               (1<<31)

#define ART_HASTE					1
#define ART_INVINCIBILITY			2
#define ART_TOMEOFPOWER				4
#define ART_INVISIBILITY			8
#define ARTFLAG_FROZEN				128
#define ARTFLAG_STONED				256
#define ARTFLAG_DIVINE_INTERVENTION 512

//===========================================

#define MAX_PLAYER_CLASS			5 // total number of available player classes
#define PORTALS_EXTRA_CLASSES		1 // number of player classes only available in the mission pack
#define ABILITIES_STR_INDEX			400 // starting number of class ability lines in strings.txt - 1


//===========================================

#define	MAX_SCOREBOARD		64 //16
#define	MAX_SCOREBOARDNAME	32

#define	SOUND_CHANNELS		8


#include "common.h"
#include "mathlib.h"
#include "cvar.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"

#include "bspfile.h"
#include "wad.h"
#include "draw.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "render.h"
#include "effect.h"
#include "progs.h"
#include "client.h"
#include "server.h"

#include "model.h"

#include "input.h"
#include "world.h"
#include "keys.h"
#include "console.h"
#include "config.h"
#include "view.h"
#include "menu.h"
#include "crc.h"
#include "cdaudio.h"

#include "glquake.h"

#include "midi.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char	*basedir;
	char	*cachedir;		// for development over ISDN lines
	int		argc;
	char	**argv;
	void	*membase;
	int		memsize;
	int		numcpus;
	int		errstate;
} quakeparms_t;


//=============================================================================


//
// host
//
extern	quakeparms_t *host_parms;

extern	cvar_t		sys_ticrate;
extern	cvar_t		sys_throttle;

extern	cvar_t		developer;
extern	cvar_t		host_timescale;

extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	byte		*host_basepal;
extern	byte		*host_colormap;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset
extern	float		host_netinterval;	// for renderer/server isolation

void Host_ClearMemory (void);
void Host_ServerFrame (void);
void Host_InitFileList (void);
void Host_InitCommands (void);
void Host_Init (void);
void Host_Shutdown(void);
void Host_Error (char *error, ...);
void Host_EndGame (char *message, ...);
void Host_Frame (double time);
void Host_Quit_f (void);
void Host_ClientCommands (char *fmt, ...);
void Host_ShutdownServer (qboolean crash);
void Host_LoadPalettes (void);
void Host_WriteConfiguration (char *configname);
void Host_Resetdemos (void);
void Host_MapListRebuild (void);
void Host_DemoListRebuild (void);
void Host_SaveListRebuild (void);
void Host_ConfigListRebuild (void);

void SaveGamestate(qboolean ClientsOnly);
int LoadGamestate(char *level, char *startspot, int ClientsMode);

extern int			current_skill;		// skill level for currently loaded level (in case
										//  the user changes the cvar while the level is
										//  running, this reflects the level actually in use)

extern int			sv_kingofhill;
extern qboolean		intro_playing;
//extern qboolean		skip_start;
//extern int			num_intro_msg;

extern filelist_t	*gamelist;
extern filelist_t	*maplist;
extern filelist_t	*demolist;
extern filelist_t	*savelist;
extern filelist_t	*configlist;

//
// chase
//
extern	cvar_t	chase_active;

void Chase_Init (void);
void Chase_Reset (void);
void Chase_Update (void);

//
// vid
//
