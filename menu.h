/*
 * $Header: /H3/game/MENU.H 5     9/04/97 4:44p Rjohnson $
 */

//
// the net drivers should just set the apropriate bits in m_activenet,
// instead of having the menu code look through their internal tables
//

enum {
	m_none,
	m_main,
	m_singleplayer,
	m_load,
	m_save,
	m_multiplayer,
	m_setup,
	m_net,
	m_options,
	m_video,
	m_keys,
	m_help,
	m_quit,
	m_serialconfig,
	m_modemconfig,
	m_lanconfig,
	m_gameoptions,
	m_search,
	m_slist,
	m_class,
	m_difficulty,
	m_mload,
	m_msave
} m_state;

extern	int	m_activenet;

extern char	*plaquemessage;     // Pointer to current plaque
extern char	*errormessage;     // Pointer to current plaque

extern char BigCharWidth[27][27];

extern int setup_class;

extern qboolean m_entersound; // play after drawing a frame, so caching won't disrupt the sound
extern qboolean m_recursiveDraw;
extern int m_return_state;
extern qboolean m_return_onerror;
extern char m_return_reason[32];

//
// menus
//
void M_Init (void);
void M_Keydown (int key);
void M_Draw (void);
void M_ToggleMenu_f (void);
void M_Menu_Main_f (void);
void M_DrawTextBox (int x, int y, int width, int lines);
void M_DrawTextBox2 (int x, int y, int width, int lines, qboolean bottom);
void M_Print (int cx, int cy, char *str);
void M_Print2 (int cx, int cy, char *str);
void M_PrintWhite (int cx, int cy, char *str);
void M_DrawTransPic (int x, int y, qpic_t *pic);
void ScrollTitle (char *name);

void M_Menu_Quit_f (void);
void M_Menu_Options_f (void);

void M_Draw (void);
void M_DrawCharacter (int cx, int line, int num);
void M_DrawPic (int x, int y, qpic_t *pic);
void M_DrawTransPic (int x, int y, qpic_t *pic);
void M_DrawCheckbox (int x, int y, int on);
