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
// pr_exec.c

#include "quakedef.h"

typedef struct
{
	int s;
	dfunction_t *f;
} prstack_t;

#define MAX_STACK_DEPTH 	256 //32
prstack_t pr_stack[MAX_STACK_DEPTH];
int pr_depth;
int	pr_peakdepth;

#define LOCALSTACK_SIZE 2048
int localstack[LOCALSTACK_SIZE];
int localstack_used;


qboolean pr_trace;
dfunction_t	*pr_xfunction;
int pr_xstatement;


int pr_argc;

char *pr_opnames[] =
{
	"DONE",
	"MUL_F", "MUL_V",  "MUL_FV", "MUL_VF",
	"DIV",
	"ADD_F", "ADD_V",
  	"SUB_F", "SUB_V",
	"EQ_F", "EQ_V", "EQ_S", "EQ_E", "EQ_FNC",
 	"NE_F", "NE_V", "NE_S", "NE_E", "NE_FNC",
 	"LE", "GE", "LT", "GT",
	"INDIRECT", "INDIRECT", "INDIRECT",
	"INDIRECT", "INDIRECT", "INDIRECT",
	"ADDRESS",
	"STORE_F", "STORE_V", "STORE_S",
	"STORE_ENT", "STORE_FLD", "STORE_FNC",
	"STOREP_F", "STOREP_V", "STOREP_S",
	"STOREP_ENT", "STOREP_FLD", "STOREP_FNC",
	"RETURN",
	"NOT_F", "NOT_V", "NOT_S", "NOT_ENT", "NOT_FNC",
	"IF", "IFNOT",
	"CALL0", "CALL1", "CALL2", "CALL3", "CALL4",
	"CALL5", "CALL6", "CALL7", "CALL8",
	"STATE",
	"GOTO",
	"AND", "OR", 
	"BITAND", "BITOR",
	"OP_MULSTORE_F", "OP_MULSTORE_V", "OP_MULSTOREP_F", "OP_MULSTOREP_V",
	"OP_DIVSTORE_F", "OP_DIVSTOREP_F",
	"OP_ADDSTORE_F", "OP_ADDSTORE_V", "OP_ADDSTOREP_F", "OP_ADDSTOREP_V",
	"OP_SUBSTORE_F", "OP_SUBSTORE_V", "OP_SUBSTOREP_F", "OP_SUBSTOREP_V",
	"OP_FETCH_GBL_F",
	"OP_FETCH_GBL_V",
	"OP_FETCH_GBL_S",
	"OP_FETCH_GBL_E",
	"OP_FETCH_GBL_FNC",
	"OP_CSTATE", "OP_CWSTATE",
	
	"OP_THINKTIME",

	"OP_BITSET", "OP_BITSETP", "OP_BITCLR",	"OP_BITCLRP",

	"OP_RAND0", "OP_RAND1",	"OP_RAND2",	"OP_RANDV0", "OP_RANDV1", "OP_RANDV2",

	"OP_SWITCH_F", "OP_SWITCH_V", "OP_SWITCH_S", "OP_SWITCH_E", "OP_SWITCH_FNC",

	"OP_CASE",
	"OP_CASERANGE"

};

char *PR_GlobalString (int ofs);
char *PR_GlobalStringNoContents (int ofs);


//=============================================================================

/*
=================
PR_PrintStatement
=================
*/
void PR_PrintStatement (dstatement_t *s)
{
	int i;

	if ( (unsigned)s->op < sizeof(pr_opnames)/sizeof(pr_opnames[0]))
	{
		Con_SafePrintf ("%s ", pr_opnames[s->op]);
		i = strlen(pr_opnames[s->op]);
		for ( ; i<10 ; i++)
			Con_Printf (" ");
	}

	if (s->op == OP_IF || s->op == OP_IFNOT)
	{
		Con_SafePrintf ("%sbranch %i", PR_GlobalString(s->a), s->b);
	}
	else if (s->op == OP_GOTO)
	{
		Con_SafePrintf ("branch %i", s->a);
	}
	else if ( (unsigned)(s->op - OP_STORE_F) < 6)
	{
		Con_SafePrintf ("%s", PR_GlobalString(s->a));
		Con_SafePrintf ("%s", PR_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
			Con_SafePrintf ("%s", PR_GlobalString(s->a));
		if (s->b)
			Con_SafePrintf ("%s", PR_GlobalString(s->b));
		if (s->c)
			Con_SafePrintf ("%s", PR_GlobalStringNoContents(s->c));
	}
	Con_SafePrintf ("\n");
}

/*
============
PR_StackTrace
============
*/
void PR_StackTrace (void)
{
	dfunction_t	*f;
	int i;

	if (pr_depth <= 0)
	{
		Con_SafePrintf ("<NO STACK>\n");
		return;
	}

	if (pr_depth > MAX_STACK_DEPTH)
		pr_depth = MAX_STACK_DEPTH;

	pr_stack[pr_depth].s = pr_xstatement;
	pr_stack[pr_depth].f = pr_xfunction;
	for (i=pr_depth ; i>0 ; i--)
	{
		f = pr_stack[i].f;
		if (!f)
			Con_SafePrintf ("<NO FUNCTION>\n");
		else
			Con_SafePrintf ("%12s : %s\n", PR_GetString(f->s_file), PR_GetString(f->s_name));
	}
}

/*
============
PR_Profile_f

============
*/
void PR_Profile_f (void)
{
	int i, j;
	int max;
	dfunction_t *f, *bestFunc;
	int total;
	int funcCount;
	qboolean byHC;
	char saveName[128];
	FILE *saveFile;
	int currentFile;
	int bestFile;
	int tally;
	char *s;

	if (!progs)
	{
		Con_SafePrintf ("No progs loaded, can't profile\n");
		return;
	}

	byHC = false;
	funcCount = 10;
	*saveName = 0;
	for (i = 1; i < Cmd_Argc(); i++)
	{
		s = Cmd_Argv(i);
		if (tolower(*s) == 'h')
		{ // Sort by HC source file
			byHC = true;
		}
		else if (tolower(*s) == 's')
		{ // Save to file
			if (i+1 < Cmd_Argc() && !isdigit(*Cmd_Argv(i+1)))
			{
				i++;
				sprintf(saveName, "%s/%s", com_gamedir, Cmd_Argv(i));
			}
			else
			{
				sprintf(saveName, "%s/profile.txt", com_gamedir);
			}
		}
		else if (isdigit(*s))
		{ // Specify function count
			funcCount = atoi(Cmd_Argv(i));
			if (funcCount < 1)
			{
				funcCount = 1;
			}
		}
	}

	total = 0;
	for (i = 0; i < progs->numfunctions; i++)
	{
		total += pr_functions[i].profile;
	}

	if (*saveName)
	{ // Create the output file
		if ((saveFile = fopen(saveName, "w")) == NULL)
		{
			Con_SafePrintf("Could not open %s\n", saveName);
			return;
		}
	}


	if (byHC == false)
	{
		j = 0;
		do
		{
			max = 0;
			bestFunc = NULL;
			for (i = 0; i < progs->numfunctions; i++)
			{
				f = &pr_functions[i];
				if (f->profile > max)
				{
					max = f->profile;
					bestFunc = f;
				}
			}
			if (bestFunc)
			{
				if (j < funcCount)
				{
					if (*saveName)
					{
						fprintf(saveFile, "%05.2f %s\n",
							((float)bestFunc->profile/(float)total)*100.0,
							PR_GetString(bestFunc->s_name));
					}
					else
					{
						Con_SafePrintf("%05.2f %s\n",
							((float)bestFunc->profile/(float)total)*100.0,
							PR_GetString(bestFunc->s_name));
					}
				}
				j++;
				bestFunc->profile = 0;
			}
		} while (bestFunc);
		if (*saveName)
		{
			fclose(saveFile);
		}
		return;
	}

	currentFile = -1;
	do
	{
		tally = 0;
		bestFile = INT_MAX;
		for (i = 0; i < progs->numfunctions; i++)
		{
			if (pr_functions[i].s_file > currentFile
				&& pr_functions[i].s_file < bestFile)
			{
				bestFile = pr_functions[i].s_file;
				tally = pr_functions[i].profile;
				continue;
			}
			if (pr_functions[i].s_file == bestFile)
			{
				tally += pr_functions[i].profile;
			}
		}
		currentFile = bestFile;
		if (tally && currentFile != INT_MAX)
		{
			if (*saveName)
			{
				fprintf(saveFile, "\"%s\"\n", PR_GetString(currentFile));
			}
			else
			{
				Con_SafePrintf("\"%s\"\n", PR_GetString(currentFile));
			}
			j = 0;
			do
			{
				max = 0;
				bestFunc = NULL;
				for (i = 0; i < progs->numfunctions; i++)
				{
					f = &pr_functions[i];
					if (f->s_file == currentFile && f->profile > max)
					{
						max = f->profile;
						bestFunc = f;
					}
				}
				if (bestFunc)
				{
					if (j < funcCount)
					{
						if (*saveName)
						{
							fprintf(saveFile, "   %05.2f %s\n",
								((float)bestFunc->profile
								/(float)total)*100.0,
								PR_GetString(bestFunc->s_name));
						}
						else
						{
							Con_SafePrintf("   %05.2f %s\n",
								((float)bestFunc->profile
								/(float)total)*100.0,
								PR_GetString(bestFunc->s_name));
						}
					}
					j++;
					bestFunc->profile = 0;
				}
			} while (bestFunc);
		}
	} while (currentFile != INT_MAX);
	if (*saveName)
	{
		fclose(saveFile);
	}
}

/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError (char *error, ...)
{
	va_list argptr;
	char string[MAX_PRINTMSG]; //1024

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);

	PR_PrintStatement (pr_statements + pr_xstatement);
	PR_StackTrace ();
	Con_SafePrintf ("%s\n", string);

	pr_depth = 0; // dump the stack so host_error can shutdown functions

	Host_Error ("Program error");
}

/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
int PR_EnterFunction (dfunction_t *f)
{
	int i, j, c, o;

	pr_stack[pr_depth].s = pr_xstatement;
	pr_stack[pr_depth].f = pr_xfunction;
	pr_depth++;
	if (pr_depth >= MAX_STACK_DEPTH)
		PR_RunError ("PR_EnterFunction: stack overflow (%d, max = %d)", pr_depth, MAX_STACK_DEPTH - 1);

	if (pr_depth > pr_peakdepth)
		pr_peakdepth = pr_depth; // Remember peak depth

	// save off any locals that the new function steps on
	c = f->locals;
	if (localstack_used + c > LOCALSTACK_SIZE)
		PR_RunError ("PR_EnterFunction: locals stack overflow (%d, max = %d)", localstack_used + c, LOCALSTACK_SIZE);

	for (i=0 ; i < c ; i++)
		localstack[localstack_used+i] = ((int *)pr_globals)[f->parm_start + i];
	localstack_used += c;

	// copy parameters
	o = f->parm_start;
	for (i=0 ; i<f->numparms ; i++)
	{
		for (j=0 ; j<f->parm_size[i] ; j++)
		{
			((int *)pr_globals)[o] = ((int *)pr_globals)[OFS_PARM0+i*3+j];
			o++;
		}
	}

	pr_xfunction = f;
	return f->first_statement - 1;	// -1 to offset the st++
}

/*
====================
PR_LeaveFunction
====================
*/
int PR_LeaveFunction (void)
{
	int i, c;

	if (pr_depth <= 0)
		PR_RunError ("PR_LeaveFunction: prog stack underflow (%d)", pr_depth);

	// Restore locals from the stack
	c = pr_xfunction->locals;
	localstack_used -= c;
	if (localstack_used < 0)
		PR_RunError ("PR_LeaveFunction: locals stack underflow (%d)", localstack_used);

	for (i=0 ; i < c ; i++)
		((int *)pr_globals)[pr_xfunction->parm_start + i] = localstack[localstack_used+i];

	// up stack
	pr_depth--;
	pr_xfunction = pr_stack[pr_depth].f;
	return pr_stack[pr_depth].s;
}

//switch types
enum {SWITCH_F,SWITCH_V,SWITCH_S,SWITCH_E,SWITCH_FNC};

#define RUNAWAY	     100000

/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram (func_t fnum)
{
	eval_t *a, *b, *c;
	dstatement_t *st;
	dfunction_t *f, *newf;
	int		profile, startprofile;
	int i;
	edict_t *ed = NULL;
	int exitdepth;
	eval_t *ptr;
	int startFrame;
	int endFrame;
	float val;
	int case_type=-1;
	float switch_float = 0;	// make compiler happy

	if (!fnum || fnum < 0 || fnum >= progs->numfunctions)
	{
		if (*pr_global_struct.self)
			ED_Print (PROG_TO_EDICT(*pr_global_struct.self));

		if (!fnum)
			Host_Error ("PR_ExecuteProgram: NULL function");
		else
			Host_Error ("PR_ExecuteProgram: invalid function (%d, max = %d)", fnum, progs->numfunctions);
	}

	f = &pr_functions[fnum];

	pr_trace = false;

// make a stack frame
	exitdepth = pr_depth;
	pr_peakdepth = 0;

	st = &pr_statements[PR_EnterFunction (f)];
	startprofile = profile = 0;

while (1)
{
	st++; // Next statement

	a = (eval_t *)&pr_globals[(unsigned short)st->a];
	b = (eval_t *)&pr_globals[(unsigned short)st->b];
	c = (eval_t *)&pr_globals[(unsigned short)st->c];

	if (++profile > RUNAWAY)
	{
		pr_xstatement = st - pr_statements;
		PR_RunError ("PR_ExecuteProgram: runaway loop error %d", RUNAWAY);
	}

	if (pr_trace)
		PR_PrintStatement (st);

	switch (st->op)
	{
	case OP_ADD_F:
		c->_float = a->_float + b->_float;
		break;
	case OP_ADD_V:
		c->vector[0] = a->vector[0] + b->vector[0];
		c->vector[1] = a->vector[1] + b->vector[1];
		c->vector[2] = a->vector[2] + b->vector[2];
		break;
		
	case OP_SUB_F:
		c->_float = a->_float - b->_float;
		break;
	case OP_SUB_V:
		c->vector[0] = a->vector[0] - b->vector[0];
		c->vector[1] = a->vector[1] - b->vector[1];
		c->vector[2] = a->vector[2] - b->vector[2];
		break;

	case OP_MUL_F:
		c->_float = a->_float * b->_float;
		break;
	case OP_MUL_V:
		c->_float = a->vector[0]*b->vector[0]
			+ a->vector[1]*b->vector[1]
			+ a->vector[2]*b->vector[2];
		break;
	case OP_MUL_FV:
		c->vector[0] = a->_float * b->vector[0];
		c->vector[1] = a->_float * b->vector[1];
		c->vector[2] = a->_float * b->vector[2];
		break;
	case OP_MUL_VF:
		c->vector[0] = b->_float * a->vector[0];
		c->vector[1] = b->_float * a->vector[1];
		c->vector[2] = b->_float * a->vector[2];
		break;

	case OP_DIV_F:
		c->_float = a->_float / b->_float;
		break;
	
	case OP_BITAND:
		c->_float = (int)a->_float & (int)b->_float;
		break;
	
	case OP_BITOR:
		c->_float = (int)a->_float | (int)b->_float;
		break;


	case OP_GE:
		c->_float = a->_float >= b->_float;
		break;
	case OP_LE:
		c->_float = a->_float <= b->_float;
		break;
	case OP_GT:
		c->_float = a->_float > b->_float;
		break;
	case OP_LT:
		c->_float = a->_float < b->_float;
		break;
	case OP_AND:
		c->_float = a->_float && b->_float;
		break;
	case OP_OR:
		c->_float = a->_float || b->_float;
		break;

	case OP_NOT_F:
		c->_float = !a->_float;
		break;
	case OP_NOT_V:
		c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];
		break;
	case OP_NOT_S:
		c->_float = !a->string || !*PR_GetString(a->string);
		break;
	case OP_NOT_FNC:
		c->_float = !a->function;
		break;
	case OP_NOT_ENT:
		c->_float = (PROG_TO_EDICT(a->edict) == sv.edicts);
		break;

	case OP_EQ_F:
		c->_float = a->_float == b->_float;
		break;
	case OP_EQ_V:
		c->_float = (a->vector[0] == b->vector[0]) &&
			(a->vector[1] == b->vector[1]) &&
			(a->vector[2] == b->vector[2]);
		break;
	case OP_EQ_S:
		c->_float = !strcmp(PR_GetString(a->string), PR_GetString(b->string));
		break;
	case OP_EQ_E:
		c->_float = a->_int == b->_int;
		break;
	case OP_EQ_FNC:
		c->_float = a->function == b->function;
		break;


	case OP_NE_F:
		c->_float = a->_float != b->_float;
		break;
	case OP_NE_V:
		c->_float = (a->vector[0] != b->vector[0]) ||
			(a->vector[1] != b->vector[1]) ||
			(a->vector[2] != b->vector[2]);
		break;
	case OP_NE_S:
		c->_float = strcmp(PR_GetString(a->string), PR_GetString(b->string));
		break;
	case OP_NE_E:
		c->_float = a->_int != b->_int;
		break;
	case OP_NE_FNC:
		c->_float = a->function != b->function;
		break;

//==================
	case OP_STORE_F:
	case OP_STORE_ENT:
	case OP_STORE_FLD:		// integers
	case OP_STORE_S:
	case OP_STORE_FNC:		// pointers
		b->_int = a->_int;
		break;
	case OP_STORE_V:
		b->vector[0] = a->vector[0];
		b->vector[1] = a->vector[1];
		b->vector[2] = a->vector[2];
		break;
		
	case OP_STOREP_F:
	case OP_STOREP_ENT:
	case OP_STOREP_FLD:		// integers
	case OP_STOREP_S:
	case OP_STOREP_FNC:		// pointers
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		ptr->_int = a->_int;
		break;
	case OP_STOREP_V:
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		ptr->vector[0] = a->vector[0];
		ptr->vector[1] = a->vector[1];
		ptr->vector[2] = a->vector[2];
		break;

	case OP_MULSTORE_F: // f *= f
		b->_float *= a->_float;
		break;
	case OP_MULSTORE_V: // v *= f
		b->vector[0] *= a->_float;
		b->vector[1] *= a->_float;
		b->vector[2] *= a->_float;
		break;
	case OP_MULSTOREP_F: // e.f *= f
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		c->_float = (ptr->_float *= a->_float);
		break;
	case OP_MULSTOREP_V: // e.v *= f
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		c->vector[0] = (ptr->vector[0] *= a->_float);
		c->vector[0] = (ptr->vector[1] *= a->_float);
		c->vector[0] = (ptr->vector[2] *= a->_float);
		break;

	case OP_DIVSTORE_F: // f /= f
		b->_float /= a->_float;
		break;
	case OP_DIVSTOREP_F: // e.f /= f
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		c->_float = (ptr->_float /= a->_float);
		break;

	case OP_ADDSTORE_F: // f += f
		b->_float += a->_float;
		break;
	case OP_ADDSTORE_V: // v += v
		b->vector[0] += a->vector[0];
		b->vector[1] += a->vector[1];
		b->vector[2] += a->vector[2];
		break;
	case OP_ADDSTOREP_F: // e.f += f
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		c->_float = (ptr->_float += a->_float);
		break;
	case OP_ADDSTOREP_V: // e.v += v
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		c->vector[0] = (ptr->vector[0] += a->vector[0]);
		c->vector[1] = (ptr->vector[1] += a->vector[1]);
		c->vector[2] = (ptr->vector[2] += a->vector[2]);
		break;

	case OP_SUBSTORE_F: // f -= f
		b->_float -= a->_float;
		break;
	case OP_SUBSTORE_V: // v -= v
		b->vector[0] -= a->vector[0];
		b->vector[1] -= a->vector[1];
		b->vector[2] -= a->vector[2];
		break;
	case OP_SUBSTOREP_F: // e.f -= f
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		c->_float = (ptr->_float -= a->_float);
		break;
	case OP_SUBSTOREP_V: // e.v -= v
		ptr = (eval_t *)((byte *)sv.edicts + b->_int);
		c->vector[0] = (ptr->vector[0] -= a->vector[0]);
		c->vector[1] = (ptr->vector[1] -= a->vector[1]);
		c->vector[2] = (ptr->vector[2] -= a->vector[2]);
		break;

	case OP_ADDRESS:
		ed = PROG_TO_EDICT(a->edict);

		if (ed == (edict_t *)sv.edicts && sv.state == ss_active)
		{
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: assignment to world entity");
		}

		c->_int = (byte *)((int *)&ed->v + b->_int) - (byte *)sv.edicts;
		break;
		
	case OP_LOAD_F:
	case OP_LOAD_FLD:
	case OP_LOAD_ENT:
	case OP_LOAD_S:
	case OP_LOAD_FNC:
		ed = PROG_TO_EDICT(a->edict);

		ptr = (eval_t *)((int *)&ed->v + b->_int);
		c->_int = ptr->_int;
		break;

	case OP_LOAD_V:
		ed = PROG_TO_EDICT(a->edict);

		ptr = (eval_t *)((int *)&ed->v + b->_int);
		c->vector[0] = ptr->vector[0];
		c->vector[1] = ptr->vector[1];
		c->vector[2] = ptr->vector[2];
		break;

	case OP_FETCH_GBL_F:
	case OP_FETCH_GBL_S:
	case OP_FETCH_GBL_E:
	case OP_FETCH_GBL_FNC:
		i = (int)b->_float;
		if (i < 0 || i > G_INT((unsigned short)st->a - 1))
		{
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: array index out of bounds: %d", i);
		}
		ptr = (eval_t *)&pr_globals[(unsigned short)st->a + i];
		c->_int = ptr->_int;
		break;
	case OP_FETCH_GBL_V:
		i = (int)b->_float;
		if (i < 0 || i > G_INT((unsigned short)st->a - 1))
		{
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: array index out of bounds: %d", i);
		}
		ptr = (eval_t *)&pr_globals[(unsigned short)st->a + ((int)b->_float)*3];
		c->vector[0] = ptr->vector[0];
		c->vector[1] = ptr->vector[1];
		c->vector[2] = ptr->vector[2];
		break;

	case OP_IFNOT:
		if (!a->_int)
			st += st->b - 1; // -1 to offset the st++
		break;

	case OP_IF:
		if (a->_int)
			st += st->b - 1; // -1 to offset the st++
		break;

	case OP_GOTO:
		st += st->a - 1; // -1 to offset the st++
		break;

	case OP_CALL8:
	case OP_CALL7:
	case OP_CALL6:
	case OP_CALL5:
	case OP_CALL4:
	case OP_CALL3:
	case OP_CALL2: // Copy second arg to shared space
		VectorCopy(c->vector, G_VECTOR(OFS_PARM1));
	case OP_CALL1: // Copy first arg to shared space
		VectorCopy(b->vector, G_VECTOR(OFS_PARM0));
	case OP_CALL0:
		pr_xfunction->profile += profile - startprofile;
		startprofile = profile;
		pr_xstatement = st - pr_statements;
		pr_argc = st->op - OP_CALL0;
		if (!a->function)
			PR_RunError ("PR_ExecuteProgram: NULL function");

		newf = &pr_functions[a->function];
		// negative statements are built in functions
		if (newf->first_statement < 0)
		{ // Built-in function
			i = -newf->first_statement;
			if (i >= pr_numbuiltins)
				PR_RunError ("PR_ExecuteProgram: bad builtin call number (%d, max = %d)", i, pr_numbuiltins);
			pr_builtins[i] ();
			break;
		}
		// Normal function
		st = &pr_statements[PR_EnterFunction (newf)];
		break;

	case OP_DONE:
	case OP_RETURN:
		pr_xfunction->profile += profile - startprofile;
		startprofile = profile;
		pr_xstatement = st - pr_statements;
		pr_globals[OFS_RETURN] = pr_globals[(unsigned short)st->a];
		pr_globals[OFS_RETURN+1] = pr_globals[(unsigned short)st->a+1];
		pr_globals[OFS_RETURN+2] = pr_globals[(unsigned short)st->a+2];

		st = &pr_statements[PR_LeaveFunction ()];
		if (pr_depth == exitdepth)
		{ // Done
			// Check old limit
			if (pr_peakdepth >= 32)
				Con_DWarning ("PR_ExecuteProgram: stack depth exceeds standard limit (%d, normal max = %d)\n", pr_peakdepth, 32 - 1);

			return;		// all done
		}
		break;

	case OP_STATE:
		ed = PROG_TO_EDICT(*pr_global_struct.self);
		ed->v.nextthink = *pr_global_struct.time + HX_FRAME_TIME;
		ed->v.frame = a->_float;
		ed->v.think = b->function;
		break;

	case OP_CSTATE: // Cycle state
		ed = PROG_TO_EDICT(*pr_global_struct.self);
		ed->v.nextthink = *pr_global_struct.time + HX_FRAME_TIME;
		ed->v.think = pr_xfunction-pr_functions;
		*pr_global_struct.cycle_wrapped = false;
		startFrame = (int)a->_float;
		endFrame = (int)b->_float;
		if (startFrame <= endFrame)
		{ // Increment
			if (ed->v.frame < startFrame || ed->v.frame > endFrame)
			{
				ed->v.frame = startFrame;
				break;
			}
			ed->v.frame++;
			if (ed->v.frame > endFrame)
			{
				*pr_global_struct.cycle_wrapped = true;
				ed->v.frame = startFrame;
			}
			break;
		}
		// Decrement
		if (ed->v.frame > startFrame || ed->v.frame < endFrame)
		{
			ed->v.frame = startFrame;
			break;
		}
		ed->v.frame--;
		if (ed->v.frame < endFrame)
		{
			*pr_global_struct.cycle_wrapped = true;
			ed->v.frame = startFrame;
		}
		break;

	case OP_CWSTATE: // Cycle weapon state
		ed = PROG_TO_EDICT(*pr_global_struct.self);
		ed->v.nextthink = *pr_global_struct.time + HX_FRAME_TIME;
		ed->v.think = pr_xfunction-pr_functions;
		*pr_global_struct.cycle_wrapped = false;
		startFrame = (int)a->_float;
		endFrame = (int)b->_float;
		if (startFrame <= endFrame)
		{ // Increment
			if (ed->v.weaponframe < startFrame
				|| ed->v.weaponframe > endFrame)
			{
				ed->v.weaponframe = startFrame;
				break;
			}
			ed->v.weaponframe++;
			if (ed->v.weaponframe > endFrame)
			{
				*pr_global_struct.cycle_wrapped = true;
				ed->v.weaponframe = startFrame;
			}
			break;
		}
		// Decrement
		if (ed->v.weaponframe > startFrame
			|| ed->v.weaponframe < endFrame)
		{
			ed->v.weaponframe = startFrame;
			break;
		}
		ed->v.weaponframe--;
		if (ed->v.weaponframe < endFrame)
		{
			*pr_global_struct.cycle_wrapped = true;
			ed->v.weaponframe = startFrame;
		}
		break;

	case OP_THINKTIME:
		ed = PROG_TO_EDICT(a->edict);
		if (ed == (edict_t *)sv.edicts && sv.state == ss_active)
		{
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: assignment to world entity");
		}
		ed->v.nextthink = *pr_global_struct.time + b->_float;
		break;

	case OP_BITSET: // f (+) f
		b->_float = (int)b->_float | (int)a->_float;
		break;
	case OP_BITSETP: // e.f (+) f
		ptr = (eval_t *)((byte *)sv.edicts+b->_int);
		ptr->_float = (int)ptr->_float | (int)a->_float;
		break;
	case OP_BITCLR: // f (-) f
		b->_float = (int)b->_float & ~((int)a->_float);
		break;
	case OP_BITCLRP: // e.f (-) f
		ptr = (eval_t *)((byte *)sv.edicts+b->_int);
		ptr->_float = (int)ptr->_float & ~((int)a->_float);
		break;

	case OP_RAND0:
//		val = (rand() & 0x7fff) / ((float)0x7fff);
		val = rand() * (1.0 / RAND_MAX);
		G_FLOAT(OFS_RETURN) = val;
		break;
	case OP_RAND1:
		val = rand()*(1.0/RAND_MAX)*a->_float;
		G_FLOAT(OFS_RETURN) = val;
		break;
	case OP_RAND2:
		if (a->_float < b->_float)
		{
			val = a->_float+(rand()*(1.0/RAND_MAX)*(b->_float-a->_float));
		}
		else
		{
			val = b->_float+(rand()*(1.0/RAND_MAX)*(a->_float-b->_float));
		}
		G_FLOAT(OFS_RETURN) = val;
		break;
	case OP_RANDV0:
		val = rand()*(1.0/RAND_MAX);
		G_FLOAT(OFS_RETURN+0) = val;
		val = rand()*(1.0/RAND_MAX);
		G_FLOAT(OFS_RETURN+1) = val;
		val = rand()*(1.0/RAND_MAX);
		G_FLOAT(OFS_RETURN+2) = val;
		break;
	case OP_RANDV1:
		val = rand()*(1.0/RAND_MAX)*a->vector[0];
		G_FLOAT(OFS_RETURN+0) = val;
		val = rand()*(1.0/RAND_MAX)*a->vector[1];
		G_FLOAT(OFS_RETURN+1) = val;
		val = rand()*(1.0/RAND_MAX)*a->vector[2];
		G_FLOAT(OFS_RETURN+2) = val;
		break;
	case OP_RANDV2:
		for (i = 0; i < 3; i++)
		{
			if (a->vector[i] < b->vector[i])
			{
				val = a->vector[i]+(rand()*(1.0/RAND_MAX)*(b->vector[i]-a->vector[i]));
			}
			else
			{
				val = b->vector[i]+(rand()*(1.0/RAND_MAX)*(a->vector[i]-b->vector[i]));
			}
			G_FLOAT(OFS_RETURN+i) = val;
		}
		break;
	case OP_SWITCH_F:
		case_type = SWITCH_F;
		switch_float = a->_float;
		st += st->b-1; // -1 to offset the st++
		break;
	case OP_SWITCH_V:
		pr_xstatement = st - pr_statements;
		PR_RunError ("PR_ExecuteProgram: switch v not done yet!");
		break;
	case OP_SWITCH_S:
		pr_xstatement = st - pr_statements;
		PR_RunError ("PR_ExecuteProgram: switch s not done yet!");
		break;
	case OP_SWITCH_E:
		pr_xstatement = st - pr_statements;
		PR_RunError ("PR_ExecuteProgram: switch e not done yet!");
		break;
	case OP_SWITCH_FNC:
		pr_xstatement = st - pr_statements;
		PR_RunError ("PR_ExecuteProgram: switch fnc not done yet!");
		break;

	case OP_CASERANGE:
		if (case_type != SWITCH_F)
		{
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: caserange fucked!");
		}
		if ((switch_float >= a->_float) && (switch_float <= b->_float))
		{
			st += st->c-1; // -1 to offset the st++
		}
		break;
	case OP_CASE:
		switch (case_type)
		{
		case SWITCH_F:
			if (switch_float == a->_float)
			{
				st += st->b-1; // -1 to offset the st++
			}
			break;
		case SWITCH_V:
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: case switch v not done yet!");
			break;
		case SWITCH_S:
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: case switch s not done yet!");
			break;
		case SWITCH_E:
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: case switch e not done yet!");
			break;
		case SWITCH_FNC:
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: case switch fnc not done yet!");
			break;
		default:
			pr_xstatement = st - pr_statements;
			PR_RunError ("PR_ExecuteProgram: fucked case!");

		}
		break;

	default:
		pr_xstatement = st - pr_statements;
		PR_RunError ("PR_ExecuteProgram: bad opcode %i", st->op);
	}
}

}

/*----------------------*/

#define PR_STRTBL_CHUNK 256
char **pr_strtbl = NULL;
int pr_strtbl_size;
int num_prstr;

void PR_InitStringTable(void)
{
	if (pr_strtbl) {
		Z_Free (pr_strtbl);
		pr_strtbl = NULL;
	}
	pr_strtbl_size = 0;
	num_prstr = 0;
}

char *PR_GetString(int num)
{
	if (num >= 0 && num < pr_strings_size - 1)
		return pr_strings + num;
	else if (num < 0 && num >= -num_prstr)
		return pr_strtbl[-num - 1];
	else
	{
		Con_Error ("PR_GetString: invalid string offset %d (%d to %d valid)\n", num, -num_prstr, pr_strings_size - 2);
		return pr_strings;
//		Host_Error ("PR_GetString: invalid string offset %d (%d to %d valid)", num, -num_prstr, pr_strings_size - 2);
	}
	
	return "";
}

int PR_SetString(char *s)
{
	int i;
	
	if (s - pr_strings < 0 || s - pr_strings > pr_strings_size - 2) {
		for (i = 0; i < num_prstr; i++)
			if (pr_strtbl[i] == s)
				break;
		if (i < num_prstr)
			return -i - 1;
		if (num_prstr == pr_strtbl_size) {
			pr_strtbl_size += PR_STRTBL_CHUNK;
			pr_strtbl = Z_Realloc(pr_strtbl, pr_strtbl_size * sizeof(char *));
		}
		pr_strtbl[num_prstr] = s;
		num_prstr++;
		return -num_prstr;
	}
	return (int)(s - pr_strings);
}
