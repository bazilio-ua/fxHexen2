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
// sys.h -- non-portable functions

//
// file IO
//
void Sys_mkdir (char *path);
void Sys_rmdir (char *path);
void Sys_unlink (char *path);
void Sys_rename (char *oldp, char *newp);

void Sys_ScanDirList (char *path, filelist_t **list);
void Sys_ScanDirFileList(char *path, char *subdir, char *ext, qboolean stripext, filelist_t **list);

//
// system IO
//
void Sys_Error (char *error, ...);
// an error will cause the entire program to exit

void Sys_Printf (char *fmt, ...);
// send text to the console

extern qboolean has_smp;

void Sys_Init (void);
void Sys_Shutdown (void);
void Sys_Quit (int code);

double Sys_DoubleTime (void);

char *Sys_ConsoleInput (void);

void Sys_Sleep (void);
// called to yield for a little bit so as
// not to hog cpu when paused or debugging

char *Sys_GetClipboardData (void);

void Sys_InitDoubleTime (void);
