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

// gl_screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net 
turn off messages option

the refresh is always rendered, unless the console is full screen


console is:
	notify lines
	half
	full

*/


int			glx, gly, glwidth, glheight;

float		scr_con_current;
float		scr_conlines;		// lines of console to display

cvar_t		scr_viewsize = {"viewsize","100", CVAR_ARCHIVE};
cvar_t		scr_weaponsize = {"weaponsize","100", CVAR_ARCHIVE};
cvar_t		scr_fov = {"fov","90", CVAR_NONE};	// 10 - 170
cvar_t		scr_weaponfov = {"weaponfov","90", CVAR_NONE};	// 10 - 170
cvar_t		scr_conspeed = {"scr_conspeed","5000", CVAR_NONE}; //300
cvar_t		scr_centertime = {"scr_centertime","4", CVAR_NONE};
cvar_t		scr_showfps = {"scr_showfps", "0", CVAR_NONE};
cvar_t		scr_showstats = {"scr_showstats", "0", CVAR_NONE};
cvar_t		scr_showspeed = {"scr_showspeed", "0", CVAR_ARCHIVE};
cvar_t		scr_speedx = {"scr_speedx", "64", CVAR_ARCHIVE};
cvar_t		scr_speedy = {"scr_speedy", "8", CVAR_ARCHIVE};
cvar_t		scr_showram = {"showram","1", CVAR_NONE};
cvar_t		scr_showturtle = {"showturtle","0", CVAR_NONE};
cvar_t		scr_showpause = {"showpause","1", CVAR_NONE};
cvar_t		scr_printspeed = {"scr_printspeed","8", CVAR_NONE};
cvar_t		gl_triplebuffer = {"gl_triplebuffer", "1", CVAR_ARCHIVE};
cvar_t		scr_showloading = {"scr_showloading", "1", CVAR_ARCHIVE}; // loading plaque

qboolean	scr_initialized;		// ready to draw

qpic_t		*scr_ram;
qpic_t		*scr_net;
qpic_t		*scr_turtle;


int			clearconsole;

int			sb_lines;

viddef_t	vid;				// global video state

vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;

int			total_loading_size, current_loading_size, loading_stage;

#define SCR_DEFTIMEOUT 60

static float	scr_timeout;

qboolean	block_drawing;

void SCR_ScreenShot_f (void);
void Plaque_Draw (char *message, qboolean AlwaysDraw);
void Info_Plaque_Draw (char *message);
void Bottom_Plaque_Draw (char *message);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
//int			scr_erase_lines;
//int			scr_erase_center;

int StartC[MAXLINES],EndC[MAXLINES];

#define MAX_INFO 1024
char infomessage[MAX_INFO];
qboolean info_up = false;

extern int	*pr_info_string_index;
extern char	*pr_global_info_strings;
extern int	 pr_info_string_count;

void UpdateInfoMessage(void)
{
	unsigned int i;
	unsigned int check;
	char *newmessage;

	strcpy(infomessage, "Objectives:");

	if (!pr_global_info_strings)
		return;

	for (i = 0; i < 32; i++)
	{
		check = (1 << i);
		
		if (cl.info_mask & check)
		{
			newmessage = &pr_global_info_strings[pr_info_string_index[i]];
			strcat(infomessage, "@@");
			strcat(infomessage, newmessage);
		}
	}

	for (i = 0; i < 32; i++)
	{
		check = (1 << i);
		
		if (cl.info_mask2 & check)
		{
			newmessage = &pr_global_info_strings[pr_info_string_index[i+32]];
			strcat(infomessage, "@@");
			strcat(infomessage, newmessage);
		}
	}

	if (strlen(infomessage)>11)
		Con_LogCenterPrint (infomessage);
}

void SCR_FindTextBreaks(char *message, int Width)
{
	int length,pos,start,lastspace,oldlast;

	length = strlen(message);
	scr_center_lines = pos = start = 0;
	lastspace = -1;

	while(1)
	{
		if (pos-start >= Width || message[pos] == '@' ||
			message[pos] == 0)
		{
			oldlast = lastspace;
			if (message[pos] == '@' || lastspace == -1 || message[pos] == 0)
				lastspace = pos;

			StartC[scr_center_lines] = start;
			EndC[scr_center_lines] = lastspace;
			scr_center_lines++;
			if (scr_center_lines == MAXLINES)
				return;
			if (message[pos] == '@')
				start = pos + 1;
			else if (oldlast == -1)
				start = lastspace;
			else
				start = lastspace + 1;

			lastspace = -1;
		}

		if (message[pos] == 32) lastspace = pos;
		else if (message[pos] == 0)
		{
			break;
		}

		pos++;
	}
}

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	// the server sends a blank centerstring in some places.
	if (!str[0])
	{
		// an empty print is sometimes used to explicitly clear the previous centerprint
		con_lastcenterstring[0] = 0;
		scr_centertime_off = 0;
		return;
	}

	// only log if the previous centerprint has already been cleared
	Con_LogCenterPrint (str);

	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

//	FindTextBreaks(scr_centerstring, 38);
//	scr_center_lines = lines;
}


void SCR_DrawCenterString (void)
{
	int		i;
	int		bx, by;
	int		remaining;
	char	temp[80];

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

//	scr_erase_center = 0;

	SCR_FindTextBreaks(scr_centerstring, 38);

	by = ((25-scr_center_lines) * 8) / 2;
	for(i=0;i<scr_center_lines;i++,by+=8)
	{
		strncpy(temp,&scr_centerstring[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-strlen(temp)) * 8) / 2;
	  	M_Print2 (bx, by, temp);
	}
}

void SCR_CheckDrawCenterString (void)
{
//	if (scr_center_lines > scr_erase_lines)
//		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;
	
	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (key_dest != key_game)
		return;
	if (cl.paused) //johnfitz -- don't show centerprint during a pause
		return;

	if(intro_playing)
		Bottom_Plaque_Draw(scr_centerstring);
	else
		SCR_DrawCenterString ();
}

//=============================================================================

/*
====================
AdaptFovx
Adapt a 4:3 horizontal FOV to the current screen size using the "Hor+" scaling:
2.0 * atan(width / height * 3.0 / 4.0 * tan(fov_x / 2.0))
====================
*/
#define FOV_ASPECT 0.75
float AdaptFovx (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		Host_Error ("Bad fov: %f", fov_x);

	if ((x = height / width) == FOV_ASPECT)
		return fov_x;
	a = atan(FOV_ASPECT / x * tan(fov_x / 360 * M_PI));
	a = a * 360 / M_PI;

	return a;
}

/*
====================
CalcFovy
====================
*/
float CalcFovy (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		Host_Error ("Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);
	a = atan(height / x);
	a = a * 360 / M_PI;

	return a;
}

/*
=================
SCR_RefdefChanged
=================
*/
void SCR_RefdefChanged (void)
{
	vid.recalc_refdef = true;
}

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	float		size;
	int		h;

	vid.recalc_refdef = false;

// force the status bar to redraw
	Sbar_Changed ();

//========================================
	
// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_SetNoCallback ("viewsize","30");
	if (scr_viewsize.value > 120)
		Cvar_SetNoCallback ("viewsize","120");

// bound weaponsize
	if (scr_weaponsize.value < 60)
		Cvar_SetNoCallback ("weaponsize","60");
	if (scr_weaponsize.value > 100)
		Cvar_SetNoCallback ("weaponsize","100");

// bound field of view
	if (scr_fov.value < 10)
		Cvar_SetNoCallback ("fov","10");
	if (scr_fov.value > 170)
		Cvar_SetNoCallback ("fov","170");

// bound weapon field of view
	if (scr_weaponfov.value < 10)
		Cvar_SetNoCallback ("weaponfov","10");
	if (scr_weaponfov.value > 170)
		Cvar_SetNoCallback ("weaponfov","170");

// intermission is always full screen	
	if (cl.intermission)
		size = 110;
	else
		size = scr_viewsize.value;

/*	if (size >= 120)
		sb_lines = 0;		// no status bar at all
	else if (size >= 110)
		sb_lines = 24;		// no inventory
	else
		sb_lines = 24+16+8;*/

	if (size >= 110)
	{ // No status bar
		sb_lines = 0;
	}
	else
	{
		sb_lines = 36;
	}

//	if (scr_overdrawsbar.value)
//		sb_lines = 0;

	if (scr_viewsize.value > 100.0)
	{
		size = 100.0;
	}
	else
	{
		size = scr_viewsize.value;
	}
	
	if (cl.intermission)
	{
		size = 100;
		sb_lines = 0;
	}
	
	size /= 100.0;

	h = vid.height - sb_lines;

	r_refdef.vrect.width = vid.width * size;

	if (r_refdef.vrect.width < 96)
	{
		size = 96.0 / r_refdef.vrect.width; // was vid.width (H2)
		r_refdef.vrect.width = 96;	// min for icons
	}

	r_refdef.vrect.height = vid.height * size;
	if (r_refdef.vrect.height > (int)vid.height - sb_lines)
		r_refdef.vrect.height = vid.height - sb_lines;

	r_refdef.vrect.x = (vid.width - r_refdef.vrect.width)/2;
	r_refdef.vrect.y = (h - r_refdef.vrect.height)/2;

	r_refdef.fov_x = AdaptFovx (scr_fov.value, vid.width, vid.height);
	r_refdef.fov_y = CalcFovy (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);
	
	r_refdef.weaponfov_x = AdaptFovx (scr_weaponfov.value, vid.width, vid.height);

	scr_vrect = r_refdef.vrect;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize.value+10);
	Cvar_SetValue ("weaponsize",scr_weaponsize.value+10);
	Sbar_ViewSizeChanged();
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize.value-10);
	Cvar_SetValue ("weaponsize",scr_weaponsize.value-10);
	Sbar_ViewSizeChanged();
}

//============================================================================

/*
==================
SCR_LoadPics -- johnfitz
==================
*/
void SCR_LoadPics (void)
{
	scr_ram = Draw_PicFromWad ("ram");
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");
}

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	Cvar_RegisterVariableCallback (&scr_fov, SCR_RefdefChanged);
	Cvar_RegisterVariableCallback (&scr_viewsize, SCR_RefdefChanged);
	Cvar_RegisterVariableCallback (&scr_weaponsize, SCR_RefdefChanged);
	Cvar_RegisterVariableCallback (&scr_weaponfov, SCR_RefdefChanged);
	Cvar_RegisterVariable (&scr_conspeed);
	Cvar_RegisterVariable (&scr_showfps); 
	Cvar_RegisterVariable (&scr_showstats); 
	Cvar_RegisterVariable (&scr_showspeed);
	Cvar_RegisterVariable (&scr_speedx);
	Cvar_RegisterVariable (&scr_speedy);
	Cvar_RegisterVariable (&scr_showram);
	Cvar_RegisterVariable (&scr_showturtle);
	Cvar_RegisterVariable (&scr_showpause);
	Cvar_RegisterVariable (&scr_centertime);
	Cvar_RegisterVariable (&scr_printspeed);
	Cvar_RegisterVariable (&gl_triplebuffer);
	Cvar_RegisterVariable (&scr_showloading);

	scr_initialized = true;
	Con_Printf ("Screen initialized\n");

//
// register our commands
//
	Cmd_AddCommand ("screenshot",SCR_ScreenShot_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

	SCR_LoadPics ();
}

/*
==============
SCR_DrawFPS
==============
*/
void SCR_DrawFPS (void)
{
	static double	oldtime = 0, fps = 0;
	static int		oldframecount = 0;
	double			time;
	int				frames;
	char			str[12];

	if (!scr_showfps.value)
		return;

	time = realtime - oldtime;
	frames = r_framecount - oldframecount;

	if (time < 0 || frames < 0)
	{
		oldtime = realtime;
		oldframecount = r_framecount;
		return;
	}

	if (time > 0.75) //update value every 3/4 second
	{
		fps = frames / time;
		oldtime = realtime;
		oldframecount = r_framecount;
	}

	sprintf (str, "%4.0f fps", fps);
	Draw_String (vid.width - (strlen(str)<<3), 0, str);
}

/*
===============
SCR_DrawStats
===============
*/
void SCR_DrawStats (void)
{
	int		mins, secs, tens;
	int		y;
	char    str[100];

	if (!scr_showstats.value)
		return;

	y = scr_showfps.value ? 8 : 0;

	mins = cl.time / 60;
	secs = cl.time - 60 * mins;
	tens = (int)(cl.time * 10) % 10;

	sprintf (str,"%i:%i%i:%i", mins, secs/10, secs%10, tens);
	Draw_String (vid.width - (strlen(str)<<3), y, str);

	if (scr_showstats.value > 1)
	{
		sprintf (str,"s: %3i/%3i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
		Draw_String (vid.width - (strlen(str)<<3), y + 8, str);

		sprintf (str,"m: %3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
		Draw_String (vid.width - (strlen(str)<<3), y + 16, str);
	}
} 

/*
==============
SCR_DrawSpeed
==============
*/
void SCR_DrawSpeed (void)
{
	const float show_speed_interval_value = 0.05f;
	static float maxspeed = 0, display_speed = -1;
	static double lastrealtime = 0;
	float speed;
	vec3_t vel;
	char str[12];

	if (!scr_showspeed.value)
		return;

	if (!cl.viewent.model) // supposed cutscene, no speed for NULL model
		return;

	if (lastrealtime > realtime)
	{
		lastrealtime = 0;
		display_speed = -1;
		maxspeed = 0;
	}

	VectorCopy (cl.velocity, vel);
	vel[2] = 0;
	speed = VectorLength (vel);

	if (speed > maxspeed)
		maxspeed = speed;

	if (display_speed >= 0)
	{
		sprintf (str, "%d ups", (int) display_speed);
		Draw_String (scr_vrect.x + scr_vrect.width/2 + scr_speedx.value - (strlen(str)<<3), scr_vrect.y + scr_vrect.height/2 + scr_speedy.value, str);
	}

	if (realtime - lastrealtime >= show_speed_interval_value)
	{
		lastrealtime = realtime;
		display_speed = maxspeed;
		maxspeed = 0;
	}
}

/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam (void)
{
	if (!scr_showram.value)
		return;

	if (!r_cache_thrash)
		return;

	Draw_Pic (scr_vrect.x+32, scr_vrect.y, scr_ram);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;
	
	if (!scr_showturtle.value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (realtime - cl.last_received_message < 0.3)
		return;
	if (cls.demoplayback)
		return;

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	qpic_t	*pic;
	float delta;
	static qboolean newdraw = false;
	int finaly;
	static float LogoPercent,LogoTargetPercent;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

//	if (!cl.paused)
//		return;

//	pic = Draw_CachePic ("gfx/pause.lmp");
//	Draw_Pic ( (vid.width - pic->width)/2, 
//		(vid.height - 48 - pic->height)/2, pic);


	if (!cl.paused)
	{
		newdraw = false;
		return;
	}

	if (!newdraw)
	{
		newdraw = true;
		LogoTargetPercent= 1;
		LogoPercent = 0;
	}

	pic = Draw_CachePic ("gfx/menu/paused.lmp");
//	Draw_Pic ( (vid.width - pic->width)/2, 
//		(vid.height - 48 - pic->height)/2, pic);

	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent-LogoPercent)/.5)*host_frametime;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	finaly = ((float)pic->height * LogoPercent) - pic->height;
	Draw_TransPicCropped ( (vid.width - pic->width)/2, finaly, pic);
}



/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	int		size, count, offset;
	qpic_t	*pic;
	
	if (!scr_showloading.value)
		return;
	
	if (!scr_drawloading && loading_stage == 0)
		return;
	
	pic = Draw_CachePic ("gfx/menu/loading.lmp");
	offset = (vid.width - pic->width)/2;
	Draw_TransPic (offset , 0, pic);
	
	if (loading_stage == 0)
		return;
	
	if (total_loading_size)
		size = current_loading_size * 106 / total_loading_size;
	else
		size = 0;
	
	offset += 42; //HoT
	
	if (loading_stage == 1)
		count = size;
	else
		count = 106;
	
	if (count)
	{
		Draw_Fill (offset, 87+0, count, 1, 136);
		Draw_Fill (offset, 87+1, count, 4, 138);
		Draw_Fill (offset, 87+5, count, 1, 136);
	}
	if (loading_stage == 2)
		count = size;
	else
		count = 0;
	
	if (count)
	{
		Draw_Fill (offset, 97+0, count, 1, 168);
		Draw_Fill (offset, 97+1, count, 4, 170);
		Draw_Fill (offset, 97+5, count, 1, 168);
	}
}

//=============================================================================

/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	//johnfitz -- let's hack away the problem of slow console when host_timescale is <0
	float timescale;

	Con_CheckResize ();
	
	if (scr_drawloading)
		return;		// never a console with loading plaque
		
// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = vid.height;		// full screen
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console)
		scr_conlines = vid.height/2;	// half screen
	else
		scr_conlines = 0;				// none visible
	
	timescale = (host_timescale.value > 0) ? host_timescale.value : 1; //johnfitz -- timescale

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed.value*host_frametime/timescale; // timescale
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed.value*host_frametime/timescale; // timescale
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
	{
		Sbar_Changed ();
	}
}
	
/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current, true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}

/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 

/* 
================== 
SCR_ScreenShot_f
================== 
*/  
void SCR_ScreenShot_f (void) 
{
	byte		*buffer;
	char		tganame[16]; 
	char		checkname[MAX_OSPATH];
	int			i;
	int			mark;

// 
// find a file name to save it to 
// 
	for (i=0; i<10000; i++)
	{ 
		sprintf (tganame, "hexen%04i.tga", i);
		sprintf (checkname, "%s/%s", com_gamedir, tganame);
		if (Sys_FileTime(checkname) == -1)
			break;	// file doesn't exist
	} 
	if (i == 10000)
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't find an unused filename\n"); 
		return;
	}

//
// get data
//
	// Pa3PyX: now using hunk instead
	mark = Hunk_LowMark ();
	
	buffer = Hunk_AllocName(glwidth * glheight * 4, "buffer_sshot");
	
	glReadPixels (glx, gly, glwidth, glheight, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

//
// now write the file
//
	if (Image_WriteTGA (tganame, buffer, glwidth, glheight, 32, false))
		Con_Printf ("Wrote %s\n", tganame);
	else
		Con_Printf ("SCR_ScreenShot_f: Couldn't create a TGA file\n");

	// Pa3PyX: now using hunk instead
	Hunk_FreeToLowMark (mark);
} 

//=============================================================================

/*
===============
SCR_SetTimeout
================
*/
void SCR_SetTimeout (float timeout)
{
	scr_timeout = timeout;
}

/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	CDAudio_Stop (); // Stop the CD music

	if (cls.state != ca_connected)
		return;
	if (cls.signon != SIGNONS)
		return;
	
// redraw with no console and the loading plaque
	Con_ClearNotify ();
	// remove all center prints
	con_lastcenterstring[0] = 0;
	scr_centerstring[0] = 0;
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = true;
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
	SCR_SetTimeout (SCR_DEFTIMEOUT);
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	Con_ClearNotify ();
}

//=============================================================================

char	*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	Plaque_Draw(scr_notifystring,1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.  
==================
*/
#define MODAL_TIMEOUT 0.0f

int SCR_ModalMessage (char *text)
{
	return SCR_ModalMessageTimeout(text, MODAL_TIMEOUT);
}

int SCR_ModalMessageTimeout (char *text, float timeout) //johnfitz -- timeout
{
	double time1, time2; //johnfitz -- timeout

	if (cls.state == ca_dedicated)
		return true;

	scr_notifystring = text;
 
// draw a fresh screen
	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;
	
	S_ClearBuffer ();		// so dma doesn't loop current sound

	time1 = Sys_DoubleTime () + timeout; //johnfitz -- timeout
	time2 = 0.0f; //johnfitz -- timeout

	do
	{
		key_count = -1;		// wait for a key down and up
		IN_ProcessEvents ();
		if (timeout)
			time2 = Sys_DoubleTime (); //johnfitz -- zero timeout means wait forever.
	} while (key_lastpress != 'y' &&
		key_lastpress != 'n' &&
		key_lastpress != K_ESCAPE &&
		time2 <= time1);

	SCR_UpdateScreen ();

	//johnfitz -- timeout
	if (time2 > time1)
		return false;
	//johnfitz

	return key_lastpress == 'y';
}


//=============================================================================

/*
===============
SCR_BringDownConsole

Brings the console down and fades the palettes back to normal
================
*/
void SCR_BringDownConsole (void)
{
	int		i;
	
	scr_centertime_off = 0;
	
	for (i=0 ; i<20 && scr_conlines != scr_con_current ; i++)
		SCR_UpdateScreen ();

	cl.cshifts[0].percent = 0;		// no area contents palette on next frame
}

/*
==================
SCR_TileClear

fixed the dimentions of right, top and bottom panels
==================
*/
void SCR_TileClear (void)
{
	if (r_refdef.vrect.x > 0)
	{
		Draw_TileClear (0,0,r_refdef.vrect.x,vid.height);
		Draw_TileClear (r_refdef.vrect.x + r_refdef.vrect.width, 0
			, vid.width - r_refdef.vrect.x + r_refdef.vrect.width,vid.height);
	}
//	if (r_refdef.vrect.height < vid.height-44)
	{
		Draw_TileClear (r_refdef.vrect.x, 0, r_refdef.vrect.width, r_refdef.vrect.y);
		Draw_TileClear (r_refdef.vrect.x, r_refdef.vrect.y + r_refdef.vrect.height,
			r_refdef.vrect.width,
			vid.height - (r_refdef.vrect.y + r_refdef.vrect.height) );
	}
}

// This is also located in screen.c
#define PLAQUE_WIDTH 26

void Plaque_Draw (char *message, qboolean AlwaysDraw)
{
	int i;
	char temp[80];
	int bx,by;

	if (scr_con_current == vid.height && !AlwaysDraw)
		return;		// console is full screen

	if (!*message) 
		return;

	SCR_FindTextBreaks(message, PLAQUE_WIDTH);

	by = ((25-scr_center_lines) * 8) / 2;
	M_DrawTextBox2 (32, by-16, 30, scr_center_lines+2,false);

	for(i=0;i<scr_center_lines;i++,by+=8)
	{
		strncpy(temp,&message[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-strlen(temp)) * 8) / 2;
	  	M_Print2 (bx, by, temp);
	}
}

void Info_Plaque_Draw (char *message)
{
	int i;
	char temp[80];
	int bx,by;

	if (scr_con_current == vid.height)
		return;		// console is full screen

	if (!*message) 
		return;


	SCR_FindTextBreaks(message, PLAQUE_WIDTH+4);

	if (scr_center_lines == MAXLINES) 
	{
		Con_DPrintf("Info_Plaque_Draw: line overflow error\n");
		scr_center_lines = MAXLINES-1;
	}

	by = ((25-scr_center_lines) * 8) / 2;
	M_DrawTextBox2 (15, by-16, PLAQUE_WIDTH+4+4, scr_center_lines+2,false);

	for(i=0;i<scr_center_lines;i++,by+=8)
	{
		strncpy(temp,&message[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-strlen(temp)) * 8) / 2;
	  	M_Print2 (bx, by, temp);
	}
}
void I_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line + ((vid.height - 200)>>1), num);
}

void I_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		I_DrawCharacter (cx, cy, ((byte)(*str))+256);
		str++;
		cx += 8;
	}
}

void Bottom_Plaque_Draw (char *message)
{
	int i;
	char temp[80];
	int bx,by;

	if (!*message) 
		return;


	SCR_FindTextBreaks(message, PLAQUE_WIDTH);

	by = (((vid.height)/8)-scr_center_lines-2) * 8;

	M_DrawTextBox2 (32, by-16, 30, scr_center_lines+2,true);

	for(i=0;i<scr_center_lines;i++,by+=8)
	{
		strncpy(temp,&message[StartC[i]],EndC[i]-StartC[i]);
		temp[EndC[i]-StartC[i]] = 0;
		bx = ((40-strlen(temp)) * 8) / 2;
	  	M_Print(bx, by, temp);
	}
}

#define	DEMO_MSG_INDEX	408
// in Hammer of Thyrion, the demo version isn't allowed in combination
// with the mission pack. therefore, the formula below isn't necessary
//#define	DEMO_MSG_INDEX	(ABILITIES_STR_INDEX+MAX_PLAYER_CLASS*2)
//			408 for H2, 410 for H2MP strings.txt

/*
===============
Sbar_IntermissionOverlay
===============
*/
void Sbar_IntermissionOverlay(void)
{
	qpic_t	*pic;
	int		elapsed, size, bx, by, i;
	char	*message,temp[80];


	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}
	
	switch(cl.intermission)
	{
	case 1:
			pic = Draw_CachePicNoTrans ("gfx/meso.lmp");
			break;
		case 2:
			pic = Draw_CachePicNoTrans ("gfx/egypt.lmp");
			break;
		case 3:
			pic = Draw_CachePicNoTrans ("gfx/roman.lmp");
			break;
		case 4:
			pic = Draw_CachePicNoTrans ("gfx/castle.lmp");
			break;
		case 5:
			pic = Draw_CachePicNoTrans ("gfx/castle.lmp");
			break;
		case 6:
			pic = Draw_CachePicNoTrans ("gfx/end-1.lmp");
			break;
		case 7:
			pic = Draw_CachePicNoTrans ("gfx/end-2.lmp");
			break;
		case 8:
			pic = Draw_CachePicNoTrans ("gfx/end-3.lmp");
			break;
		case 9:
			pic = Draw_CachePicNoTrans ("gfx/castle.lmp");
			break;
		// mission pack
		case 10:
			pic = Draw_CachePicNoTrans ("gfx/mpend.lmp");
			break;
		case 11:
			pic = Draw_CachePicNoTrans ("gfx/mpmid.lmp");
			break;
		case 12:
			pic = Draw_CachePicNoTrans ("gfx/end-3.lmp");
			break;

		default:
			Sys_Error ("Sbar_IntermissionOverlay: Bad episode");
			break;
	}

	// Pa3PyX: intermissions now drawn fullscreen
	Draw_IntermissionPic (pic);
//	Draw_Pic (((vid.width - 320)>>1),((vid.height - 200)>>1), pic);

	if (cl.intermission >= 6 && cl.intermission <= 8)
	{
		elapsed = (cl.time - cl.completed_time) * 20;
		elapsed -= 50;
		if (elapsed < 0)
			elapsed = 0;
	}
	else if (cl.intermission == 12)	// mission pack entry screen
	{
	// this intermission is NOT triggered by a server message, but
	// by starting a new game through the menu system. therefore,
	// you cannot use cl.time, and cl.completed_time must be set by
	// the menu sytem, as well.
		elapsed = (realtime - cl.completed_time) * 20;
	}
	else
	{
		elapsed = (cl.time - cl.completed_time) * 20;
	}

	if (cl.intermission <= 4 && cl.intermission + 394 <= pr_string_count)
		message = &pr_global_strings[pr_string_index[cl.intermission + 394]];
	else if (cl.intermission == 5)	// finale for the demo
		message = &pr_global_strings[pr_string_index[DEMO_MSG_INDEX]];
	else if (cl.intermission >= 6 && cl.intermission <= 8 && cl.intermission + 386 <= pr_string_count)
		message = &pr_global_strings[pr_string_index[cl.intermission + 386]];
	else if (cl.intermission == 9)	// finale for the bundle (oem) version
		message = &pr_global_strings[pr_string_index[391]];
	// mission pack
	else if (cl.intermission == 10)
		message = &pr_global_strings[pr_string_index[538]];
	else if (cl.intermission == 11)
		message = &pr_global_strings[pr_string_index[545]];
	else if (cl.intermission == 12)
		message = &pr_global_strings[pr_string_index[561]];
	else
		message = "";

	if (message[0])
		Con_LogCenterPrint (message);

	SCR_FindTextBreaks(message, 38);

	// hacks to print the final messages centered: "by" is the y offset
	// in pixels to begin printing at. each line is 8 pixels - S.A
	//if (cl.intermission == 8)
	//	by = 16;
	if (cl.intermission >= 6 && cl.intermission <= 8)
		// eidolon, endings. num == 6,7,8
		by = (vid.height/2 - scr_center_lines*4);
	else if (cl.intermission == 10)
		// mission pack: tibet10. num == 10
		by = 33;
	else
		by = ((25-scr_center_lines) * 8) / 2;

	for (i = 0; i < scr_center_lines; i++, by += 8)
	{
		size = EndC[i] - StartC[i];
		strncpy (temp, &message[StartC[i]], size);

		if (size > elapsed)
			size = elapsed;
		temp[size] = 0;

		bx = ((40-strlen(temp)) * 8) / 2;
	  	if (cl.intermission < 6 || cl.intermission > 9)
			I_Print (bx, by, temp);
		else
			M_PrintWhite (bx, by, temp);

		elapsed -= size;
		if (elapsed <= 0)
			break;
	}

	if (i == scr_center_lines && elapsed >= 300 && cl.intermission >= 6 && cl.intermission <= 7)
	{
		cl.intermission++;
		cl.completed_time = cl.time;
	}
}

//==========================================================================
//
// Sbar_FinaleOverlay
//
//==========================================================================

void Sbar_FinaleOverlay(void)
{
	qpic_t	*pic;


	pic = Draw_CachePic("gfx/finale.lmp");
	Draw_TransPic((vid.width-pic->width)/2, 16, pic);
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	static float	oldscr_viewsize;

	if (cls.state == ca_dedicated)
		return;				// stdout only

	if (vid_hiddenwindow || block_drawing)
		return;				// don't suck up any cpu if minimized or blocked for drawing

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

	vid.numpages = (gl_triplebuffer.value) ? 3 : 2; // in case gl_triplebuffer is not 0 or 1

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > scr_timeout)
		{
			scr_disabled_for_loading = false;
			total_loading_size = 0;
			loading_stage = 0;

			if (scr_timeout == SCR_DEFTIMEOUT)
				Con_Printf ("screen update timeout -- load failed.\n");
		}
		else
			return;
	}

//
// check for vid changes
//
	if (vid.recalc_refdef)
	{
		// something changed, so reorder the screen
		SCR_CalcRefdef ();
	}

	if (/*scr_overdrawsbar.value || */gl_clear.value || isIntel) // intel video workaround
		Sbar_Changed ();

//
// do 3D refresh drawing, and then update the screen
//
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);

	SCR_SetUpToDrawConsole ();

	if (cl.intermission > 1 || cl.intermission <= 12)
	{
		V_RenderView ();
	}

	GL_Set2D ();

	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear ();

	if (scr_drawdialog) //new game confirm
	{
		Sbar_Draw();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
	}
	else if (scr_drawloading) //loading
	{
		Sbar_Draw();
		Draw_FadeScreen ();
		SCR_DrawLoading ();
	}
	else if (cl.intermission >= 1 && cl.intermission <= 12)
	{
		Sbar_IntermissionOverlay ();
		if (cl.intermission < 12)
		{
			SCR_DrawConsole ();
			M_Draw ();
		}
	}
/*	else if (cl.intermission == 2 && key_dest == key_game)
	{
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
	}*/
	else
	{
		Draw_Crosshair ();
		SCR_DrawSpeed ();
		SCR_DrawRam ();
		SCR_DrawNet ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		SCR_CheckDrawCenterString ();
		SCR_DrawFPS ();
		SCR_DrawStats ();
		Sbar_Draw ();
		Plaque_Draw (plaquemessage,0);
		SCR_DrawConsole ();
		M_Draw ();
		if (errormessage)
			Plaque_Draw (errormessage,1);

		if (info_up)
		{
			UpdateInfoMessage ();
			Info_Plaque_Draw (infomessage);
		}
	}

	if (loading_stage)
		SCR_DrawLoading ();

	V_UpdateBlend ();

	GL_EndRendering ();
}

