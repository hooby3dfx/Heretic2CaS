#pragma once

void __cdecl R_RenderFrame(refdef_t *refdef);
float R_GetFOV(void);

extern void(*R_RenderFrameEngine)(void *refdef);
void Com_Printf(char *format, ...);
bool R_RenderCaS(void);

float R_GetBrightness(void);