/*
 * $Header: /H2 Mission Pack/Menu.c 41    3/20/98 2:03p Jmonroe $
 */

#include "quakedef.h"

//#ifdef _WIN32
//#include "winquake.h"
//#endif


void (*vid_menucmdfn)(void); //johnfitz
void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);


void M_Menu_SinglePlayer_f (void);
void M_Menu_Load_f (void);
void M_Menu_Save_f (void);
void M_Menu_MultiPlayer_f (void);
void M_Menu_Setup_f (void);

void M_Menu_Options_f (void);
void M_Menu_Keys_f (void);
void M_Menu_Video_f (void);
void M_Menu_Help_f (void);
void M_Menu_Quit_f (void);


void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_Search_f (void);
void M_Menu_ServerList_f (void);

void M_Main_Draw (void);
void M_SinglePlayer_Draw (void);
void M_Load_Draw (void);
void M_Save_Draw (void);
void M_MultiPlayer_Draw (void);
void M_Setup_Draw (void);

void M_Options_Draw (void);
void M_Keys_Draw (void);
void M_Video_Draw (void);
void M_Help_Draw (void);
void M_Quit_Draw (void);


void M_LanConfig_Draw (void);
void M_GameOptions_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);

void M_Main_Key (int key);
void M_SinglePlayer_Key (int key);
void M_Load_Key (int key);
void M_Save_Key (int key);
void M_MultiPlayer_Key (int key);
void M_Setup_Key (int key);

void M_Options_Key (int key);
void M_Keys_Key (int key);
void M_Video_Key (int key);
void M_Help_Key (int key);
void M_Quit_Key (int key);


void M_LanConfig_Key (int key);
void M_GameOptions_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

int			m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];

static float TitlePercent = 0;
static float TitleTargetPercent = 1;
static float LogoPercent = 0;
static float LogoTargetPercent = 1;

int		setup_class;

static char *message,*message2;
static double message_time;

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)

void M_ConfigureNetSubsystem(void);
void M_Menu_Class_f (void);

extern qboolean introPlaying;

char *ClassNames[MAX_PLAYER_CLASS] = 
{
	"Paladin",
	"Crusader",
	"Necromancer",
	"Assassin",
	"Demoness"
};

char *ClassNamesU[MAX_PLAYER_CLASS] = 
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"DEMONESS"
};

#define NUM_DIFFLEVELS	4

char *DiffNames[MAX_PLAYER_CLASS][NUM_DIFFLEVELS] =
{
	{	// Paladin
		"APPRENTICE",
		"SQUIRE",
		"ADEPT",
		"LORD"
	},
	{	// Crusader
		"GALLANT",
		"HOLY AVENGER",
		"DIVINE HERO",
		"LEGEND"
	},
	{	// Necromancer
		"SORCERER",
		"DARK SERVANT",
		"WARLOCK",
		"LICH KING"
	},
	{	// Assassin
		"ROGUE",
		"CUTTHROAT",
		"EXECUTIONER",
		"WIDOW MAKER"
	},
	{	// Demoness
		"LARVA",
		"SPAWN",
		"FIEND",
		"SHE BITCH"
	}
};


//=============================================================================
/* Support Routines */

/*
================
M_DrawCharacter

Draws one solid graphics character, centered, on line
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line, num);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, ((byte)(*str))+256);
		str++;
		cx += 8;
	}
}

/*
================
M_DrawCharacter2

Draws one solid graphics character, centered H and V
================
*/
void M_DrawCharacter2 (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line + ((vid.height - 200)>>1), num);
}

void M_Print2 (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter2 (cx, cy, ((byte)(*str))+256);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (byte)*str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawTransPic2 (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y + ((vid.height - 200)>>1), pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawTransPicCropped (int x, int y, qpic_t *pic)
{
	Draw_TransPicCropped (x + ((vid.width - 320)>>1), y, pic);
}


void M_DrawTransPicTranslate (int x, int y, qpic_t *pic, int top, int bottom, int playerclass) //johnfitz -- more parameters
{
	Draw_TransPicTranslate (x + ((vid.width - 320)>>1), y, pic, top, bottom, playerclass);
}


void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p,*tm,*bm;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	tm = Draw_CachePic ("gfx/box_tm.lmp");
	bm = Draw_CachePic ("gfx/box_bm.lmp");
	while (width > 0)
	{
		cy = y;
		M_DrawTransPic (cx, cy, tm);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		M_DrawTransPic (cx, cy+8, bm);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}


void M_DrawTextBox2 (int x, int y, int width, int lines, qboolean bottom)
{
	qpic_t	*p,*tm,*bm;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy, p);
	else
		M_DrawTransPic2 (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		if(bottom)
			M_DrawTransPic (cx, cy, p);
		else
			M_DrawTransPic2 (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy+8, p);
	else
		M_DrawTransPic2 (cx, cy+8, p);

	// draw middle
	cx += 8;
	tm = Draw_CachePic ("gfx/box_tm.lmp");
	bm = Draw_CachePic ("gfx/box_bm.lmp");
	while (width > 0)
	{
		cy = y;
		
		if(bottom)
			M_DrawTransPic (cx, cy, tm);
		else
			M_DrawTransPic2 (cx, cy, tm);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			if(bottom)
				M_DrawTransPic (cx, cy, p);
			else
				M_DrawTransPic2 (cx, cy, p);
		}
		if(bottom)
			M_DrawTransPic (cx, cy+8, bm);
		else
			M_DrawTransPic2 (cx, cy+8, bm);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy, p);
	else
		M_DrawTransPic2 (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		if(bottom)
			M_DrawTransPic (cx, cy, p);
		else
			M_DrawTransPic2 (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy+8, p);
	else
		M_DrawTransPic2 (cx, cy+8, p);
}

//=============================================================================

int m_save_demonum;
		
/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			LogoTargetPercent = TitleTargetPercent = 1;
			LogoPercent = TitlePercent = 0;
			M_Menu_Main_f ();
			return;
		}
		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		LogoTargetPercent = TitleTargetPercent = 1;
		LogoPercent = TitlePercent = 0;
		M_Menu_Main_f ();
	}
}

char BigCharWidth[27][27];
static char unused_filler;  // cuz the COM_LoadStackFile puts a 0 at the end of the data

//#define BUILD_BIG_CHAR 1

void M_BuildBigCharWidth (void)
{
#ifdef BUILD_BIG_CHAR
	qpic_t	*p;
	int ypos,xpos;
	byte			*source;
	int biggestX,adjustment;
	char After[20], Before[20];
	int numA,numB;
	FILE *FH;
	char temp[MAX_OSPATH];

	p = Draw_CachePic ("gfx/menu/bigfont.lmp");

	for(numA = 0; numA < 27; numA++)
	{
		memset(After,20,sizeof(After));
		source = p->data + ((numA % 8) * 20) + (numA / 8 * p->width * 20);
		biggestX = 0;

		for(ypos=0;ypos < 19;ypos++)
		{
			for(xpos=0;xpos<19;xpos++,source++)
			{
				if (*source) 
				{
					After[ypos] = xpos;
					if (xpos > biggestX) biggestX = xpos;
				}
			}
			source += (p->width - 19);
		}
		biggestX++;

		for(numB = 0; numB < 27; numB++)
		{
			memset(Before,0,sizeof(Before));
			source = p->data + ((numB % 8) * 20) + (numB / 8 * p->width * 20);
			adjustment = 0;

			for(ypos=0;ypos < 19;ypos++)
			{
				for(xpos=0;xpos<19;xpos++,source++)
				{
					if (!(*source))
					{
						Before[ypos]++;
					}
					else break;
				}
				source += (p->width - xpos);
			}


			while(1)
			{
				for(ypos=0;ypos<19;ypos++)
				{
					if (After[ypos] - Before[ypos] >= 15) break;
					Before[ypos]--;
				}
				if (ypos < 19) break;
				adjustment--;
			}
			BigCharWidth[numA][numB] = adjustment+biggestX;
		}
	}

	sprintf(temp,"%s\\gfx\\menu\\fontsize.lmp",com_gamedir);
	FH = fopen(temp,"wb");
	fwrite(BigCharWidth,1,sizeof(BigCharWidth),FH);
	fclose(FH);
#else
	COM_LoadStackFile ("gfx/menu/fontsize.lmp",BigCharWidth,sizeof(BigCharWidth)+1, NULL);
#endif
}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/


void M_DrawBigString(int x, int y, char *string)
{
	int c,length;

	x += ((vid.width - 320)>>1);

	length = strlen(string);
	for(c=0;c<length;c++)
	{
		x += M_DrawBigCharacter(x,y,string[c],string[c+1]);
	}
}






void ScrollTitle (char *name)
{
	float delta;
	qpic_t	*p;
	static char *LastName = "";
	int finaly;
	static qboolean CanSwitch = true;

	if (TitlePercent < TitleTargetPercent)
	{
		delta = ((TitleTargetPercent-TitlePercent)/0.5)*host_frametime;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		TitlePercent += delta;
		if (TitlePercent > TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
		}
	}
	else if (TitlePercent > TitleTargetPercent)
	{
		delta = ((TitlePercent-TitleTargetPercent)/0.15)*host_frametime;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		TitlePercent -= delta;
		if (TitlePercent <= TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
			CanSwitch = true;
		}
	}

	if (LogoPercent < LogoTargetPercent)
	{
/*		delta = ((LogoTargetPercent-LogoPercent)/1.1)*host_frametime;
		if (delta < 0.0015)
		{
			delta = 0.0015;
		}
*/
		delta = ((LogoTargetPercent-LogoPercent)/.15)*host_frametime;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	if (strcasecmp(LastName,name) != 0 && TitleTargetPercent != 0)
		TitleTargetPercent = 0;

    if (CanSwitch) 
	{
		LastName = name;
		CanSwitch = false;
		TitleTargetPercent = 1;
	}

	p = Draw_CachePic(LastName);
	finaly = ((float)p->height * TitlePercent) - p->height;
	M_DrawTransPicCropped( (320-p->width)/2, finaly , p);

	if (m_state != m_keys)
	{
		p = Draw_CachePic("gfx/menu/hplaque.lmp");
		finaly = ((float)p->height * LogoPercent) - p->height;
		M_DrawTransPicCropped(10, finaly, p);
	}		
}

//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
#define	MAIN_ITEMS	5


void M_Menu_Main_f (void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;
}
				

void M_Main_Draw (void)
{
	int		f;

	ScrollTitle("gfx/menu/title0.lmp");
//	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mainmenu.lmp") );
	M_DrawBigString (72,60+(0*20),"SINGLE PLAYER");
	M_DrawBigString (72,60+(1*20),"MULTIPLAYER");
	M_DrawBigString (72,60+(2*20),"OPTIONS");
	M_DrawBigString (72,60+(3*20),"HELP");
	M_DrawBigString (72,60+(4*20),"QUIT");


	f = (int)(realtime * 10)%8;
	M_DrawTransPic (43, 54 + m_main_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo ();
		break;
		
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_SinglePlayer_f ();
			break;

		case 1:
			M_Menu_MultiPlayer_f ();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Help_f ();
			break;

		case 4:
			M_Menu_Quit_f ();
			break;
		}
	}
}

char	*plaquemessage = NULL;   // Pointer to current plaque message
char    *errormessage = NULL;

//=============================================================================
/* DIFFICULTY MENU */
void M_Menu_Difficulty_f (void)
{
	key_dest = key_menu;
	m_state = m_difficulty;
}

int	m_diff_cursor;
int m_enter_portals = 0;
#define	DIFF_ITEMS	NUM_DIFFLEVELS

void M_Difficulty_Draw (void)
{
	int		f, i;

	ScrollTitle("gfx/menu/title5.lmp");

	setup_class = cl_playerclass.value;

	if (setup_class < 1 || setup_class > MAX_PLAYER_CLASS)
		setup_class = MAX_PLAYER_CLASS;
	if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES && !portals)
		setup_class = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES;
	setup_class--;
	
	for(i = 0; i < NUM_DIFFLEVELS; ++i)
		M_DrawBigString (72,60+(i*20),DiffNames[setup_class][i]);

	f = (int)(realtime * 10)%8;
	M_DrawTransPic (43, 54 + m_diff_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}

void M_NewMissionPackGame (void)
{
/* NOTES:
 - running a new single player mission pack game through
   the menu system starts an "intermission screen" (#12)
   first: some scrolling text giving a story background.
 - this intermission screen #12 is only started by this
   menu system: unlike the other intermissions, it isn't
   progs (server) controlled.
 - there will not be a svc_intermission message for this
   so, we must set cl.completed_time and cl.intermission
   by hand here.
 - since the user may just have started the game, we must
   load strings.txt for the scrolling text before setting
   cl.intermission, if necessary.
 - running the keep1 map is also a client-side thing:  it
   is triggered by Key_Event() when the user hits a key.
 */
	key_dest = key_game;
	if (!pr_string_count || !pr_global_strings || !pr_string_index)
		PR_LoadStrings ();
	cl.intermission = 12;
	cl.completed_time = realtime;
	cls.demonum = m_save_demonum;
}

void M_Difficulty_Key (int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ESCAPE:
		M_Menu_Class_f ();
		break;
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_diff_cursor >= DIFF_ITEMS)
			m_diff_cursor = 0;
		break;
	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_diff_cursor < 0)
			m_diff_cursor = DIFF_ITEMS - 1;
		break;
	case K_ENTER:
		Cvar_SetValue ("skill", m_diff_cursor);
		m_entersound = true;
		m_state = m_none;
		if (m_enter_portals)
		{
			M_NewMissionPackGame ();
			return;
		}
		Cbuf_AddText ("wait\n"); // make m_none to really work
		Cbuf_AddText ("map demo1\n");
		break;
	default:
		key_dest = key_game;
		m_state = m_none;
		break;
	}
}

//=============================================================================
/* CLASS CHOICE MENU */
int class_flag;

void M_Menu_Class_f (void)
{
	class_flag=0;
	key_dest = key_menu;
	m_state = m_class;
}

void M_Menu_Class2_f (void)
{
	key_dest = key_menu;
	m_state = m_class;
	class_flag=1;
}


int	m_class_cursor;

void M_Class_Draw (void)
{
	int	i, f = MAX_PLAYER_CLASS;

	if (!portals && !m_enter_portals)
		f = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES;

	if (m_class_cursor >= f)
		m_class_cursor = 0;

	ScrollTitle("gfx/menu/title2.lmp");

	for (i = 0; i < f; ++i)
		M_DrawBigString (72,60+(i*20),ClassNamesU[i]);

	f = (int)(realtime * 10)%8;
	M_DrawTransPic (43, 54 + m_class_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );

	M_DrawPic (251,54 + 21, Draw_CachePic (va("gfx/cport%d.lmp", m_class_cursor + 1)));
	M_DrawTransPic (242,54, Draw_CachePic ("gfx/menu/frame.lmp"));
}

void M_Class_Key (int key)
{
	int		f = MAX_PLAYER_CLASS;

	if (!portals && !m_enter_portals)
		f = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES;

	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_class_cursor >= f)
			m_class_cursor = 0;
		break;
	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_class_cursor < 0)
			m_class_cursor = f - 1;
		break;
	case K_ENTER:
//		sv_player->v.playerclass=m_class_cursor+1;
		Cbuf_AddText ( va ("playerclass %d\n", m_class_cursor+1) );
		m_entersound = true;
		if (!class_flag)
		{
			M_Menu_Difficulty_f();
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}
		break;
	default:
		key_dest = key_game;
		m_state = m_none;
		break;
	}
}


//=============================================================================
/* SINGLE PLAYER MENU */

#define	SINGLEPLAYER_ITEMS	3
#define	SP_PORTALS_ITEMS	2

int	m_singleplayer_cursor;

void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
	Cvar_SetValue ("timelimit", 0);		//put this here to help play single after dm
}

void M_SinglePlayer_Draw (void)
{
	int		f;

	ScrollTitle("gfx/menu/title1.lmp");

	if (portals)
		M_DrawBigString (72,60+(0*20),"NEW MISSION");
	else
		M_DrawBigString (72,60+(0*20),"NEW GAME");

	M_DrawBigString (72,60+(1*20),"LOAD");
	M_DrawBigString (72,60+(2*20),"SAVE");

	if (portals)
	{
		M_DrawBigString (72,60+(3*20),"OLD MISSION");
		M_DrawBigString (72,60+(4*20),"VIEW INTRO");
	}

	f = (int)(realtime * 10)%8;
	M_DrawTransPic (43, 54 + m_singleplayer_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		m_singleplayer_cursor++;
		if (portals)
		{
			if (m_singleplayer_cursor >= SINGLEPLAYER_ITEMS + SP_PORTALS_ITEMS)
				m_singleplayer_cursor = 0;
		}
		else 
		{
			if (m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
				m_singleplayer_cursor = 0;
		}
		break;
	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_singleplayer_cursor < 0)
		{
			if (portals)
				m_singleplayer_cursor = SINGLEPLAYER_ITEMS + SP_PORTALS_ITEMS - 1;
			else
				m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		}
		break;
	case K_ENTER:
		m_entersound = true;
		m_enter_portals = 0;
		switch (m_singleplayer_cursor)
		{
		case 0:
			if (portals)
				m_enter_portals = 1;
		case 3:
			if (sv.active)
				if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
					break;
			key_dest = key_game;
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			CL_RemoveGIPFiles(NULL);
			Cbuf_AddText ("maxplayers 1\n");
			Cbuf_AddText ("deathmatch 0\n");
			Cbuf_AddText ("coop 0\n");
			M_Menu_Class_f ();
			break;
		case 1:
			M_Menu_Load_f ();
			break;
		case 2:
			M_Menu_Save_f ();
			break;
		case 4:
			if (portals)
			{
				key_dest = key_game;
				Cbuf_AddText("playdemo t9\n");
			}
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "%s/s%i/info.dat", com_gamedir, i);
		f = fopen (name, "r");
		if (!f)
			continue;
		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], name, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		fclose (f);			
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves ();
}
				

void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	
	ScrollTitle("gfx/menu/load.lmp");
	
	for (i=0 ; i< MAX_SAVEGAMES; i++)
		M_Print (16, 60 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 60 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int		i;

	ScrollTitle("gfx/menu/save.lmp");

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print (16, 60 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 60 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Load_Key (int k)
{	
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		S_LocalSound ("raven/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

	// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;	
	}
}



//=============================================================================
/* MULTIPLAYER LOAD/SAVE MENU */

void M_ScanMSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "%s/ms%i/info.dat", com_gamedir, i);
		f = fopen (name, "r");
		if (!f)
			continue;
		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], name, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		fclose (f);			
	}
}

void M_Menu_MLoad_f (void)
{
	m_entersound = true;
	m_state = m_mload;
	key_dest = key_menu;
	M_ScanMSaves ();
}

void M_Menu_MSave_f (void)
{
	if (!sv.active || cl.intermission || svs.maxclients == 1)
	{
		message = "Only a network server";
		message2 = "can save a multiplayer game";
		message_time = realtime;
		return;
	}
	m_entersound = true;
	m_state = m_msave;
	key_dest = key_menu;
	M_ScanMSaves ();
}


void M_MLoad_Key (int k)
{	
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_ENTER:
		S_LocalSound ("raven/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

		if (sv.active)
			Cbuf_AddText ("disconnect\n");
		Cbuf_AddText ("listen 1\n");	// so host_netport will be re-examined

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

	// issue the load command
		Cbuf_AddText (va ("load ms%i\n", load_cursor) );
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_MSave_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va("save ms%i\n", load_cursor));
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;	
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	5

void M_Menu_MultiPlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;

	message = NULL;
}

void M_MultiPlayer_Draw (void)
{
	int		f;

	ScrollTitle("gfx/menu/title4.lmp");
//	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	M_DrawBigString (72,60+(0*20),"JOIN A GAME");
	M_DrawBigString (72,60+(1*20),"NEW GAME");
	M_DrawBigString (72,60+(2*20),"SETUP");
	M_DrawBigString (72,60+(3*20),"LOAD");
	M_DrawBigString (72,60+(4*20),"SAVE");

	f = (int)(realtime * 10)%8;
	M_DrawTransPic (43, 54 + m_multiplayer_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );

	if (message)
	{
		M_PrintWhite ((320/2) - ((27*8)/2), 168, message);
		M_PrintWhite ((320/2) - ((27*8)/2), 176, message2);
		if (realtime - 5 > message_time)
			message = NULL;
	}

	if (tcpipAvailable)
		return;

	M_PrintWhite ((320/2) - ((27*8)/2), 160, "No Communications Available");
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if (tcpipAvailable)
				M_Menu_LanConfig_f();
			break;

		case 1:
			if (tcpipAvailable)
				M_Menu_LanConfig_f();
			break;

		case 2:
			M_Menu_Setup_f ();
			break;

		case 3:
			M_Menu_MLoad_f ();
			break;

		case 4:
			M_Menu_MSave_f ();
			break;
		}
	}
}

//=============================================================================
/* SETUP MENU */

int		setup_cursor = 5;
int		setup_cursor_table[] = {60, 76, 100, 124, 148, 176};

char	setup_hostname[16];
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;

#define	NUM_SETUP_CMDS	6

void M_Menu_Setup_f (void)
{
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	strcpy(setup_myname, cl_name.string);
	strcpy(setup_hostname, hostname.string);
	setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
	setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
	setup_class = cl_playerclass.value;
	if (setup_class < 1 || setup_class > MAX_PLAYER_CLASS)
		setup_class = MAX_PLAYER_CLASS;

	if (!portals)
	{
		if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES)
			setup_class = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES;
	}
}


void M_Setup_Draw (void)
{
	qpic_t	*p;

	ScrollTitle("gfx/menu/title4.lmp");

	M_Print (56, 60, "Hostname");
	M_DrawTextBox (152, 52, 16, 1);
	M_Print (160, 60, setup_hostname);

	M_Print (56, 76, "Your name");
	M_DrawTextBox (152, 68, 16, 1);
	M_Print (160, 76, setup_myname);

	M_Print (56, 100, "Current Class: ");
	M_Print (80, 108, ClassNames[setup_class-1]);

	M_Print (56, 124, "First color patch");
	M_Print (56, 148, "Second color patch");

	M_DrawTextBox (56, 168, 14, 1);
	M_Print (64, 176, "Accept Changes");

	p = Draw_CachePic (va("gfx/menu/netp%i.lmp", setup_class));
	M_DrawTransPicTranslate (212, 92, p, setup_top, setup_bottom, setup_class);

	M_DrawCharacter (48, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

	if (setup_cursor == 0)
		M_DrawCharacter (160 + 8*strlen(setup_hostname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));

	if (setup_cursor == 1)
		M_DrawCharacter (160 + 8*strlen(setup_myname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
}


void M_Setup_Key (int k)
{
	int			l;

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;
		S_LocalSound ("raven/menu3.wav");
		if (setup_cursor == 2)
		{
			setup_class--;
			if (setup_class < 1) 
				setup_class = MAX_PLAYER_CLASS;
			if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES && !portals)
				setup_class = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES;
		}
		else if (setup_cursor == 3)
			setup_top = setup_top - 1;
		else if (setup_cursor == 4)
			setup_bottom = setup_bottom - 1;
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
forward:
		S_LocalSound ("raven/menu3.wav");
		if (setup_cursor == 2)
		{
			setup_class++;
			if (setup_class > MAX_PLAYER_CLASS) 
				setup_class = 1;
			if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES && !portals)
				setup_class = 1;
		}
		else if (setup_cursor == 3)
			setup_top = setup_top + 1;
		else if (setup_cursor == 4)
			setup_bottom = setup_bottom + 1;
		break;

	case K_ENTER:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 2 || setup_cursor == 3 || setup_cursor == 4)
			goto forward;

		if (strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (strcmp(hostname.string, setup_hostname) != 0)
			Cvar_Set("hostname", setup_hostname);
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText( va ("color %i %i\n", setup_top, setup_bottom) );
		Cbuf_AddText ( va ("playerclass %d\n", setup_class) );
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;
	
	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname)-1] = 0;
		}

		if (setup_cursor == 1)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
		break;
		
	default:
		if (k < 32 || k > 127)
			break;
		if (setup_cursor == 0)
		{
			l = strlen(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l+1] = 0;
				setup_hostname[l] = k;
			}
		}
		if (setup_cursor == 1)
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l+1] = 0;
				setup_myname[l] = k;
			}
		}
	}
/*
	if (setup_top > 10)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 10;
	if (setup_bottom > 10)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 10;
*/
	//use CLAMP
	setup_top = CLAMP(0, setup_top, 11);
	setup_bottom = CLAMP(0, setup_bottom, 11);
}

//=============================================================================
/* OPTIONS MENU */

#define	SLIDER_RANGE	10

enum 
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,	//1
	OPT_DEFAULTS,	//2
	OPT_SCRSIZE,	//3
	OPT_GAMMA,		//4
	OPT_MOUSESPEED,	//5
	OPT_MUSICTYPE,	//6
	OPT_MUSICVOL,	//7
	OPT_SNDVOL,		//8
	OPT_ALWAYRUN,	//9
	OPT_INVMOUSE,	//10
	OPT_LOOKSPRING,	//11
	OPT_LOOKSTRAFE,	//12
	OPT_CROSSHAIR,	//13
	OPT_ALWAYSMLOOK,//14
	OPT_VIDEO,		//15
	OPTIONS_ITEMS
};

static int		options_cursor;

void M_Menu_Options_f (void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;
}


void M_AdjustSliders (int dir)
{
	S_LocalSound ("raven/menu3.wav");

	switch (options_cursor)
	{
	case OPT_SCRSIZE:	// screen size
		scr_viewsize.value += dir * 10;
		if (scr_viewsize.value < 30)
			scr_viewsize.value = 30;
		if (scr_viewsize.value > 120)
			scr_viewsize.value = 120;
		Cvar_SetValue ("viewsize", scr_viewsize.value);
		Sbar_ViewSizeChanged();
		vid.recalc_refdef = 1;
		break;
	case OPT_GAMMA: //case 4 // gamma
		v_gamma.value -= dir * 0.05;
		if (v_gamma.value < 0.5)
			v_gamma.value = 0.5;
		else if (v_gamma.value > 1)
			v_gamma.value = 1;
		Cvar_SetValue ("gamma", v_gamma.value);
		break;
	case OPT_MOUSESPEED:	// mouse speed
		sensitivity.value += dir * 0.5;
		if (sensitivity.value < 1)
			sensitivity.value = 1;
		if (sensitivity.value > 11)
			sensitivity.value = 11;
		Cvar_SetValue ("sensitivity", sensitivity.value);
		break;
	case OPT_MUSICTYPE: // bgm type
		if (strcasecmp(bgmtype.string,"midi") == 0)
		{
			if (dir < 0)
				Cvar_Set("bgmtype","none");
			else
				Cvar_Set("bgmtype","cd");
		}
		else if (strcasecmp(bgmtype.string,"cd") == 0)
		{
			if (dir < 0)
				Cvar_Set("bgmtype","midi");
			else
				Cvar_Set("bgmtype","none");
		}
		else
		{
			if (dir < 0)
				Cvar_Set("bgmtype","cd");
			else
				Cvar_Set("bgmtype","midi");
		}
		break;
	case OPT_MUSICVOL:	// music volume
		bgmvolume.value += dir * 0.1;

		if (bgmvolume.value < 0)
			bgmvolume.value = 0;
		if (bgmvolume.value > 1)
			bgmvolume.value = 1;
		Cvar_SetValue ("bgmvolume", bgmvolume.value);
		break;
	case OPT_SNDVOL:	// sfx volume
		volume.value += dir * 0.1;
		if (volume.value < 0)
			volume.value = 0;
		if (volume.value > 1)
			volume.value = 1;
		Cvar_SetValue ("volume", volume.value);
		break;
	case OPT_ALWAYRUN:	// always run
		Cvar_SetValue ("cl_run", !cl_run.value);
		break;
	case OPT_INVMOUSE:	// invert mouse
		Cvar_SetValue ("m_pitch", -m_pitch.value);
		break;
	case OPT_LOOKSPRING:	// lookspring
		Cvar_SetValue ("lookspring", !lookspring.value);
		break;
	case OPT_LOOKSTRAFE:	// lookstrafe
		Cvar_SetValue ("lookstrafe", !lookstrafe.value);
		break;
	case OPT_CROSSHAIR:	
		Cvar_SetValue ("crosshair", !crosshair.value);
		break;
	case OPT_ALWAYSMLOOK:	
		if (in_mlook.state & 1)
			Cbuf_AddText("-mlook");
		else
			Cbuf_AddText("+mlook");
		break;

	default:
		break;
	}
}


void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 256);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 257);
	M_DrawCharacter (x+i*8, y, 258);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 259);
}

void M_DrawCheckbox (int x, int y, int on)
{
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

void M_Options_Draw (void)
{
	float		r;
	
	ScrollTitle("gfx/menu/title3.lmp");
	
	M_Print (16, 60+(0*8), "    Customize controls");
	M_Print (16, 60+(1*8), "         Go to console");
	M_Print (16, 60+(2*8), "     Reset to defaults");

	M_Print (16, 60+(3*8), "           Screen size");
	r = (scr_viewsize.value - 30) / (120 - 30);
	M_DrawSlider (220, 60+(3*8), r);

	M_Print (16, 60+(4*8), "            Brightness");
	r = (1.0 - v_gamma.value) / 0.5;
	M_DrawSlider (220, 60+(4*8), r);

	M_Print (16, 60+(5*8), "           Mouse Speed");
	r = (sensitivity.value - 1)/10;
	M_DrawSlider (220, 60+(5*8), r);

	M_Print (16, 60+(6*8), "            Music Type");
	if (strcasecmp(bgmtype.string,"midi") == 0)
		M_Print (220, 60+(6*8), "MIDI");
	else if (strcasecmp(bgmtype.string,"cd") == 0)
		M_Print (220, 60+(6*8), "CD");
	else
		M_Print (220, 60+(6*8), "None");

	M_Print (16, 60+(7*8), "          Music Volume");
	r = bgmvolume.value;
	M_DrawSlider (220, 60+(7*8), r);

	M_Print (16, 60+(8*8), "          Sound Volume");
	r = volume.value;
	M_DrawSlider (220, 60+(8*8), r);

	M_Print (16, 60+(9*8), "            Always Run");
	M_DrawCheckbox (220, 60+(9*8), cl_run.value);

	M_Print (16, 60+(OPT_INVMOUSE*8), "          Invert Mouse");
	M_DrawCheckbox (220, 60+(OPT_INVMOUSE*8), m_pitch.value < 0);

	M_Print (16, 60+(OPT_LOOKSPRING*8),	"            Lookspring");
	M_DrawCheckbox (220, 60+(OPT_LOOKSPRING*8), lookspring.value);

	M_Print (16, 60+(OPT_LOOKSTRAFE*8), "            Lookstrafe");
	M_DrawCheckbox (220, 60+(OPT_LOOKSTRAFE*8), lookstrafe.value);

	M_Print (16, 60+(OPT_CROSSHAIR*8), "        Show Crosshair");
	M_DrawCheckbox (220, 60+(OPT_CROSSHAIR*8), crosshair.value);

	M_Print (16,60+(OPT_ALWAYSMLOOK*8), "            Mouse Look");
	M_DrawCheckbox (220, 60+(OPT_ALWAYSMLOOK*8), in_mlook.state & 1);

	if (vid_menudrawfn)
		M_Print (16, 60+(OPT_VIDEO*8), "         Video Options");

// cursor
	// doesn't get drawn properly with XFree4.3/MGA200 S.A.
	M_DrawCharacter (200, 60 + options_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Options_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case OPT_CUSTOMIZE:
			M_Menu_Keys_f ();
			break;
		case OPT_CONSOLE:
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		case OPT_DEFAULTS:
			Cbuf_AddText ("exec default.cfg\n");
			break;
		case OPT_VIDEO:
			M_Menu_Video_f ();
			break;
		default:
			M_AdjustSliders (1);
			break;
		}
		return;
	
	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		options_cursor--;
		if (options_cursor < OPT_CUSTOMIZE)
			options_cursor = OPTIONS_ITEMS - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = OPT_CUSTOMIZE;
		break;	

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}

	if (options_cursor == OPT_VIDEO && !vid_menudrawfn)
		if (k == K_UPARROW)
			options_cursor = OPT_VIDEO - 1;
		else
			options_cursor = OPT_CUSTOMIZE;
}

//=============================================================================
/* KEYS MENU */

char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"impulse 10", 		"change weapon"},
{"+jump", 			"jump / swim up"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+crouch",			"crouch"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
{"+moveup",			"swim up"},
{"+movedown",		"swim down"},
{"impulse 13", 		"lift object"},
{"invuse",			"use inv item"},
{"impulse 44",		"drop inv item"},
{"+showinfo",		"full inventory"},
{"+showdm",			"info / frags"},
{"toggle_dm",		"toggle frags"},
{"+infoplaque",		"objectives"},
{"invleft",			"inv move left"},
{"invright",		"inv move right"},
{"impulse 100",		"inv:torch"},
{"impulse 101",		"inv:qrtz flask"},
{"impulse 102",		"inv:mystic urn"},
{"impulse 103",		"inv:krater"},
{"impulse 104",		"inv:chaos devc"},
{"impulse 105",		"inv:tome power"},
{"impulse 106",		"inv:summon stn"},
{"impulse 107",		"inv:invisiblty"},
{"impulse 108",		"inv:glyph"},
{"impulse 109",		"inv:boots"},
{"impulse 110",		"inv:repulsion"},
{"impulse 111",		"inv:bo peep"},
{"impulse 112",		"inv:flight"},
{"impulse 113",		"inv:force cube"},
{"impulse 114",		"inv:icon defn"}
};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

#define KEYS_SIZE 14

int		keys_cursor;
int		bind_grab;
int		keys_top = 0;

void M_Menu_Keys_f (void)
{
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l,l2;
	char	*b;

	twokeys[0] = twokeys[1] = -1; 
	l = strlen(command); 
	count = 0;
	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j]; 
		if (!b)
			continue; 
		if (!strncmp (b, command, l))
		{
			l2= strlen(b); 
			if (l == l2)
			{		
				twokeys[count] = j; 
				count++; 
				if (count == 2)
					break;
			}		
		}
	}
}

void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	char	*name;
	int		x, y;

	ScrollTitle("gfx/menu/title6.lmp");

//	M_DrawTextBox (6,56, 35,16);

//	p = Draw_CachePic("gfx/menu/hback.lmp");
//	M_DrawTransPicCropped(8, 62, p);

	if (bind_grab)
		M_Print (12, 64, "Press a key or button for this action");
	else
		M_Print (18, 64, "Enter to change, backspace to clear");
		
	if (keys_top) 
		M_DrawCharacter (6, 80, 128);
	if (keys_top + KEYS_SIZE < NUMCOMMANDS)
		M_DrawCharacter (6, 80 + ((KEYS_SIZE-1)*8), 129);

// search for known bindings
	for (i=0 ; i<KEYS_SIZE ; i++)
	{
		y = 80 + 8*i;

		M_Print (16, y, bindnames[i+keys_top][1]);

		l = strlen (bindnames[i+keys_top][0]);
		
		M_FindKeysForCommand (bindnames[i+keys_top][0], keys);
		
		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = strlen(name) * 8;
			if (keys[1] != -1)
			{
				M_Print (140 + x + 8, y, "or");
				M_Print (140 + x + 32, y, Key_KeynumToString (keys[1]));
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter (130, 80 + (keys_cursor-keys_top)*8, '=');
	else
		M_DrawCharacter (130, 80 + (keys_cursor-keys_top)*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[2];
	
	if (bind_grab)
	{	// defining a key
		S_LocalSound ("raven/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor][0]);			
			Cbuf_InsertText (cmd);
		}
		
		bind_grab = false;
		return;
	}
	
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		S_LocalSound ("raven/menu2.wav");
		if (keys[1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_LocalSound ("raven/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}

	if (keys_cursor < keys_top) 
		keys_top = keys_cursor;
	else if (keys_cursor >= keys_top+KEYS_SIZE)
		keys_top = keys_cursor - KEYS_SIZE + 1;
}

//=============================================================================
/* VIDEO MENU */

//void M_Menu_Video_f (void)
//{
//	key_dest = key_menu;
//	m_state = m_video;
//	m_entersound = true;
//}
void M_Menu_Video_f (void)
{
	(*vid_menucmdfn) (); //johnfitz
}


void M_Video_Draw (void)
{
	(*vid_menudrawfn) ();
}


void M_Video_Key (int key)
{
	(*vid_menukeyfn) (key);
}

//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	5


void M_Menu_Help_f (void)
{
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}


void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic ( va("gfx/menu/help%02i.lmp", help_page+1)) );
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
int		m_quit_prevstate;
qboolean	wasInMenus;

char *quitMessage [] = 
{
/* .........1.........2.... */
  "   Look! Behind you!    ",
  "  There's a big nasty   ",
  "   thing - shoot it!    ",
  "                        ",
 
  "  You can't go now, I   ",
  "   was just getting     ",
  "    warmed up.          ",
  "                        ",

  "    One more game.      ",
  "      C'mon...          ",
  "   Who's gonna know?    ",
  "                        ",

  "   What's the matter?   ",
  "   Palms too sweaty to  ",
  "     keep playing?      ",
  "                        ",
 
  "  Watch your local store",
  "      for Hexen II      ",
  "    plush toys and      ",
  "    greeting cards!     ",
 
  "  Hexen II...           ",
  "                        ",
  "    Too much is never   ",
  "        enough.         ",
 
  "  Sure go ahead and     ",
  "  leave.  But I know    ",
  "  you'll be back.       ",
  "                        ",
 
  "                        ",
  "  Insert cute phrase    ",
  "        here            ",
  "                        "
};

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand()&7;
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		key_dest = key_console;
		Host_Quit_f ();
		break;

	default:
		break;
	}

}

void M_Quit_Draw (void)
{
	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	M_DrawTextBox (56, 76, 24, 4);
	M_Print (64, 84,  quitMessage[msgNumber*4+0]);
	M_Print (64, 92,  quitMessage[msgNumber*4+1]);
	M_Print (64, 100, quitMessage[msgNumber*4+2]);
	M_Print (64, 108, quitMessage[msgNumber*4+3]);
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
int		lanConfig_cursor_table [] = {100, 120, 140, 172};
#define NUM_LANCONFIG_CMDS	4

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[30];

void M_Menu_LanConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor >= 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;

	setup_class = cl_playerclass.value;
	if (setup_class < 1 || setup_class > MAX_PLAYER_CLASS)
		setup_class = MAX_PLAYER_CLASS;
	if (!portals)
	{
		if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES)
			setup_class = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES;
	}
	setup_class--;
}


void M_LanConfig_Draw (void)
{
	int		basex;
	char	*startJoin;
	char	*protocol;

	ScrollTitle("gfx/menu/title4.lmp");
	basex = 48;

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";

	protocol = "TCP/IP";

	M_Print (basex, 60, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 80, "Address:");
	M_Print (basex+9*8, 80, my_tcpip_address);

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, 6, 1);
	M_Print (basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Class:");
		M_Print (basex+8*7, lanConfig_cursor_table[1], ClassNames[setup_class]);

		M_Print (basex, lanConfig_cursor_table[2], "Search for local games...");
		M_Print (basex, 156, "Join game at:");
		M_DrawTextBox (basex, lanConfig_cursor_table[3]-8, 30, 1);
		M_Print (basex+8, lanConfig_cursor_table[3], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		M_Print (basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [0], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 3)
		M_DrawCharacter (basex+8 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [3], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 192, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		lanConfig_cursor--;

		if (JoiningGame)
		{
			if (lanConfig_cursor < 0)
				lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		}
		else
		{
			if (lanConfig_cursor < 0)
				lanConfig_cursor = NUM_LANCONFIG_CMDS-2;
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
		if ((JoiningGame && lanConfig_cursor <= 1) ||
			(!JoiningGame && lanConfig_cursor == 0))
			break;

		m_entersound = true;
		if (JoiningGame)
			Cbuf_AddText ( va ("playerclass %d\n", setup_class+1) );

		M_ConfigureNetSubsystem ();

		if ((JoiningGame && lanConfig_cursor == 2) ||
			(!JoiningGame && lanConfig_cursor == 1))
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 3)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			key_dest = key_game;
			m_state = m_none;
			Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			break;
		}

		break;
	
	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 3)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;
		
	case K_LEFTARROW:
		if (lanConfig_cursor != 1 || !JoiningGame)
			break;

		S_LocalSound ("raven/menu3.wav");
		setup_class--;
		if (setup_class < 0) 
			setup_class = MAX_PLAYER_CLASS -1;
		if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES - 1 && !portals)
			setup_class = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES -1;
		break;

	case K_RIGHTARROW:
		if (lanConfig_cursor != 1 || !JoiningGame)
			break;

		S_LocalSound ("raven/menu3.wav");
		setup_class++;
		if (setup_class > MAX_PLAYER_CLASS - 1) 
			setup_class = 0;
		if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES - 1 && !portals)
			setup_class = 0;
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 3)
		{
			l = strlen(lanConfig_joinname);
			if (l < 29)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = strlen(lanConfig_portname);
			if (l < 5)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;

	l =  atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	char	*name;
	char	*description;
} level_t;

level_t		levels[] =
{
	{"demo1", "Blackmarsh"},							// 0
	{"demo2", "Barbican"},								// 1

	{"ravdm1", "Deathmatch 1"},							// 2

	{"demo1","Blackmarsh"},								// 3
	{"demo2","Barbican"},								// 4
	{"demo3","The Mill"},								// 5
	{"village1","King's Court"},						// 6
	{"village2","Inner Courtyard"},						// 7
	{"village3","Stables"},								// 8
	{"village4","Palace Entrance"},						// 9
	{"village5","The Forgotten Chapel"},				// 10
	{"rider1a","Famine's Domain"},						// 11

	{"meso2","Plaza of the Sun"},						// 12
	{"meso1","The Palace of Columns"},					// 13
	{"meso3","Square of the Stream"},					// 14
	{"meso4","Tomb of the High Priest"},				// 15
	{"meso5","Obelisk of the Moon"},					// 16
	{"meso6","Court of 1000 Warriors"},					// 17
	{"meso8","Bridge of Stars"},						// 18
	{"meso9","Well of Souls"},							// 19

	{"egypt1","Temple of Horus"},						// 20
	{"egypt2","Ancient Temple of Nefertum"},			// 21
	{"egypt3","Temple of Nefertum"},					// 22
	{"egypt4","Palace of the Pharaoh"},					// 23
	{"egypt5","Pyramid of Anubis"},						// 24
	{"egypt6","Temple of Light"},						// 25
	{"egypt7","Shrine of Naos"},						// 26
	{"rider2c","Pestilence's Lair"},					// 27

	{"romeric1","The Hall of Heroes"},					// 28
	{"romeric2","Gardens of Athena"},					// 29
	{"romeric3","Forum of Zeus"},						// 30
	{"romeric4","Baths of Demetrius"},					// 31
	{"romeric5","Temple of Mars"},						// 32
	{"romeric6","Coliseum of War"},						// 33
	{"romeric7","Reflecting Pool"},						// 34

	{"cath","Cathedral"},								// 35
	{"tower","Tower of the Dark Mage"},					// 36
	{"castle4","The Underhalls"},						// 37
	{"castle5","Eidolon's Ordeal"},						// 38
	{"eidolon","Eidolon's Lair"},						// 39

	{"ravdm1","Atrium of Immolation"},					// 40
	{"ravdm2","Total Carnage"},							// 41
	{"ravdm3","Reckless Abandon"},						// 42
	{"ravdm4","Temple of RA"},							// 43
	{"ravdm5","Tom Foolery"},							// 44

	{"ravdm1", "Deathmatch 1"},							// 45

//OEM
	{"demo1","Blackmarsh"},								// 46
	{"demo2","Barbican"},								// 47
	{"demo3","The Mill"},								// 48
	{"village1","King's Court"},						// 49
	{"village2","Inner Courtyard"},						// 50
	{"village3","Stables"},								// 51
	{"village4","Palace Entrance"},						// 52
	{"village5","The Forgotten Chapel"},				// 53
	{"rider1a","Famine's Domain"},						// 54

//Mission Pack
	{"keep1",	"Eidolon's Lair"},						// 55
	{"keep2",	"Village of Turnabel"},					// 56
	{"keep3",	"Duke's Keep"},							// 57
	{"keep4",	"The Catacombs"},						// 58
	{"keep5",	"Hall of the Dead"},					// 59

	{"tibet1",	"Tulku"},								// 60
	{"tibet2",	"Ice Caverns"},							// 61
	{"tibet3",	"The False Temple"},					// 62
	{"tibet4",	"Courtyards of Tsok"},					// 63
	{"tibet5",	"Temple of Kalachakra"},				// 64
	{"tibet6",	"Temple of Bardo"},						// 65
	{"tibet7",	"Temple of Phurbu"},					// 66
	{"tibet8",	"Palace of Emperor Egg Chen"},			// 67
	{"tibet9",	"Palace Inner Chambers"},				// 68
	{"tibet10",	"The Inner Sanctum of Praevus"},		// 69
};

typedef struct
{
	char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	// Demo
	{"Demo", 0, 2},
	{"Demo Deathmatch", 2, 1},

	// Registered
	{"Village", 3, 9},
	{"Meso", 12, 8},
	{"Egypt", 20, 8},
	{"Romeric", 28, 7},
	{"Cathedral", 35, 5},
	{"MISSION PACK", 55, 15},
	{"Deathmatch", 40, 5},

	// OEM
	{"Village", 46, 9},
	{"Deathmatch", 45, 1},
};

#define OEM_START 9
#define REG_START 2
#define MP_START 7
#define DM_START 8

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 104, 112, 128, 136};
#define	NUM_GAMEOPTIONS	11
int		gameoptions_cursor;

void M_Menu_GameOptions_f (void)
{
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;

	setup_class = cl_playerclass.value;
	if (setup_class < 1 || setup_class > MAX_PLAYER_CLASS)
		setup_class = MAX_PLAYER_CLASS;
	if (!portals)
	{
		if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES)
			setup_class = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES;
	}
	setup_class--;

	if (oem.value)
	{
		if (startepisode < OEM_START || startepisode > OEM_START+1)
			startepisode = OEM_START;
		if (coop.value)
			startepisode = OEM_START;
	}
	else if (registered.value)
	{
		if (startepisode < REG_START || startepisode >= OEM_START)
			startepisode = REG_START;
		else if (startepisode == MP_START && !portals)
			startepisode = REG_START;
		if (coop.value && startepisode == DM_START)
			startepisode = REG_START;
	}
	else	// demo
	{
		if (startepisode < 0 || startepisode > 1)
			startepisode = 0;
		if (coop.value)
			startepisode = 0;
	}

	if (coop.value)
	{
		startlevel = 0;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS-1)
			gameoptions_cursor = 0;
	}
}

void M_GameOptions_Draw (void)
{
	ScrollTitle("gfx/menu/title4.lmp");

	M_DrawTextBox (152+8, 60, 10, 1);
	M_Print (160+8, 68, "begin game");

	M_Print (0+8, 84, "      Max players");
	M_Print (160+8, 84, va("%i", maxplayers) );

	M_Print (0+8, 92, "        Game Type");
	if (coop.value)
		M_Print (160+8, 92, "Cooperative");
	else
		M_Print (160+8, 92, "Deathmatch");

	M_Print (0+8, 100, "        Teamplay");
	{
		char *msg;

		switch((int)teamplay.value)
		{
			case 1:
				msg = "No Friendly Fire";
				break;
			case 2:
				msg = "Friendly Fire";
				break;
			default:
				msg = "Off";
				break;
		}
		M_Print (160+8, 100, msg);
	}

	M_Print (0+8, 108, "            Class");
	M_Print (160+8, 108, ClassNames[setup_class]);

	M_Print (0+8, 116, "       Difficulty");

	M_Print (160+8, 116, DiffNames[setup_class][(int)skill.value]);

	M_Print (0+8, 124, "       Frag Limit");
	if (fraglimit.value == 0)
		M_Print (160+8, 124, "none");
	else
		M_Print (160+8, 124, va("%i frags", (int)fraglimit.value));

	M_Print (0+8, 132, "       Time Limit");
	if (timelimit.value == 0)
		M_Print (160+8, 132, "none");
	else
		M_Print (160+8, 132, va("%i minutes", (int)timelimit.value));

	M_Print (0+8, 140, "     Random Class");
	if (randomclass.value)
		M_Print (160+8, 140, "on");
	else
		M_Print (160+8, 140, "off");

	M_Print (0+8, 156, "         Episode");
	M_Print (160+8, 156, episodes[startepisode].description);

	M_Print (0+8, 164, "           Level");
	M_Print (160+8, 164, levels[episodes[startepisode].firstLevel + startlevel].name);
	M_Print (96, 180, levels[episodes[startepisode].firstLevel + startlevel].description);

// line cursor
	M_DrawCharacter (172-16, gameoptions_cursor_table[gameoptions_cursor]+28, 12+((int)(realtime*4)&1));

/*	rjr
	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			x = (320-26*8)/2;
			M_DrawTextBox (x, 138, 24, 4);
			x += 8;
			M_Print (x, 146, "  More than 4 players   ");
			M_Print (x, 154, " requires using command ");
			M_Print (x, 162, "line parameters; please ");
			M_Print (x, 170, "   see techinfo.txt.    ");
		}
		else
		{
			m_serverInfoMessage = false;
		}
	}*/
}


void M_NetStart_Change (int dir)
{
	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
		//	m_serverInfoMessage = true;
		//	m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_SetValue ("coop", coop.value ? 0 : 1);
		if (coop.value)
		{
			startlevel = 0;
			if (startepisode == 1)
				startepisode = 0;
			else if (startepisode == DM_START)
				startepisode = REG_START;
			else if (startepisode == OEM_START+1)
				startepisode = OEM_START;
		}
		break;

	case 3:
		Cvar_SetValue ("teamplay", teamplay.value + dir);
		if (teamplay.value > 2)
			Cvar_SetValue ("teamplay", 0);
		else if (teamplay.value < 0)
			Cvar_SetValue ("teamplay", 2);
		break;

	case 4:
		setup_class += dir;
		if (setup_class < 0) 
			setup_class = MAX_PLAYER_CLASS - 1;
		if (!portals)
		{
			if (setup_class > MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES - 1)
				setup_class = MAX_PLAYER_CLASS - PORTALS_EXTRA_CLASSES - 1;
		}
		if (setup_class > MAX_PLAYER_CLASS - 1) 
			setup_class = 0;
		break;

	case 5:
		Cvar_SetValue ("skill", skill.value + dir);
		if (skill.value > 3)
			Cvar_SetValue ("skill", 0);
		else if (skill.value < 0)
			Cvar_SetValue ("skill", 3);
		break;

	case 6:
		Cvar_SetValue ("fraglimit", fraglimit.value + dir*10);
		if (fraglimit.value > 100)
			Cvar_SetValue ("fraglimit", 0);
		else if (fraglimit.value < 0)
			Cvar_SetValue ("fraglimit", 100);
		break;

	case 7:
		Cvar_SetValue ("timelimit", timelimit.value + dir*5);
		if (timelimit.value > 60)
			Cvar_SetValue ("timelimit", 0);
		else if (timelimit.value < 0)
			Cvar_SetValue ("timelimit", 60);
		break;

	case 8:
		if (randomclass.value)
			Cvar_SetValue ("randomclass", 0);
		else
			Cvar_SetValue ("randomclass", 1);
		break;

	case 9:
		if (registered.value)
		{
			startepisode += dir;
			startlevel = 0;
			if (startepisode > DM_START)
				startepisode = REG_START;
			else
			{
				if (startepisode == MP_START && !portals)
					startepisode += dir;
				if (coop.value && startepisode == DM_START)
					startepisode = (dir > 0) ? REG_START : (portals ? MP_START : MP_START-1);
				if (startepisode < REG_START)
					startepisode = (coop.value) ? (portals ? MP_START : MP_START-1) : DM_START;
			}
		}
		else if (oem.value)
		{
			if (!coop.value)
			{
				startepisode = (startepisode != OEM_START) ? OEM_START : OEM_START+1;
				startlevel = 0;
			}
		}
		else	// demo version
		{
			if (!coop.value)
			{
				startepisode = (startepisode != 0) ? 0 : 1;
				startlevel = 0;
			}
		}
		break;

	case 10:
		if (coop.value)
		{
			startlevel = 0;
			break;
		}
		startlevel += dir;

		if (startlevel < 0)
			startlevel = episodes[startepisode].levels - 1;
		else if (startlevel >= episodes[startepisode].levels)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
		{
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
			if (coop.value)
				gameoptions_cursor--;
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		gameoptions_cursor++;
		if (coop.value)
		{
			if (gameoptions_cursor >= NUM_GAMEOPTIONS-1)
				gameoptions_cursor = 0;
		}
		else
		{
			if (gameoptions_cursor >= NUM_GAMEOPTIONS)
				gameoptions_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("raven/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("raven/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
		S_LocalSound ("raven/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ( va ("playerclass %d\n", setup_class+1) );
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText ( va ("maxplayers %u\n", maxplayers) );
			SCR_BeginLoadingPlaque ();

			Cbuf_AddText ( va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name) );

			return;
		}
		
		M_NetStart_Change (1);
		break;	
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;

void M_Menu_Search_f (void)
{
	key_dest = key_menu;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw (void)
{
	int x;

	ScrollTitle("gfx/menu/title4.lmp");

	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 60, 12, 1);
	M_Print (x, 68, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 92, "No Hexen II servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

int		slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n;
	char	string[64];

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int	i,j;
			hostcache_t temp;
			for (i = 0; i < hostCacheCount; i++)
				for (j = i+1; j < hostCacheCount; j++)
					if (strcmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}

	ScrollTitle("gfx/menu/title4.lmp");
	for (n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].maxusers)
			sprintf(string, "%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		else
			sprintf(string, "%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
		M_Print (16, 60 + 8*n, string);
	}
	M_DrawCharacter (0, 60 + slist_cursor*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 176, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
		S_LocalSound ("raven/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		key_dest = key_game;
		m_state = m_none;
		Cbuf_AddText ( va ("connect \"%s\"\n", hostcache[slist_cursor].cname) );
		break;

	default:
		break;
	}
}

//=============================================================================
/* Menu Subsystem */

void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	Cmd_AddCommand ("menu_class", M_Menu_Class2_f);

	M_BuildBigCharWidth();
}


void M_Draw (void)
{
	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			S_ExtraUpdate ();
		}
		else
			Draw_FadeScreen ();

	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_difficulty:
		M_Difficulty_Draw ();
		break;

	case m_class:
		M_Class_Draw ();
		break;

	case m_load:
	case m_mload:
		M_Load_Draw ();
		break;

	case m_save:
	case m_msave:
		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		M_Quit_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
	}

	if (m_entersound)
	{
		S_LocalSound ("raven/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate ();
}


void M_Keydown (int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_difficulty:
		M_Difficulty_Key (key);
		return;

	case m_class:
		M_Class_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_mload:
		M_MLoad_Key (key);
		return;

	case m_msave:
		M_MSave_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_video:
		M_Video_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;
	}
}


void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config

	Cbuf_AddText ("stopdemo\n");

	net_hostport = lanConfig_port;
}

