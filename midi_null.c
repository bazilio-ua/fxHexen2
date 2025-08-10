

#include "quakedef.h"


qboolean MIDI_Init(void)
{
	Con_Printf("MIDI_DRV: disabled at compile time.\n");
	return false;
}


void MIDI_Play(char *Name)
{
}

void MIDI_Pause(void)
{
}

void MIDI_Loop(int NewValue)
{
}

void MIDI_Stop(void)
{
}

void MIDI_Cleanup(void)
{
}

