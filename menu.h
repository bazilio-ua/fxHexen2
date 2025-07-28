/*
 * $Header: /H3/game/MENU.H 5     9/04/97 4:44p Rjohnson $
 */

//
// the net drivers should just set the apropriate bits in m_activenet,
// instead of having the menu code look through their internal tables
//

extern	int	m_activenet;

extern char	*plaquemessage;     // Pointer to current plaque
extern char	*errormessage;     // Pointer to current plaque

extern char BigCharWidth[27][27];

extern int setup_class;

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
