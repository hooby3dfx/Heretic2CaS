// r_main.cpp
//

#include <windows.h>
#include <string>
#include "MinHook.h"
#include "MinHook.h"

extern "C"
{
#include "../game/q_shared.h"
#include "../client/ref.h"
};

#include "glew.h"
#include "launcher.h"

//
// GetRefAPI_t
//
refexport_t GetRefAPI(refimport_t imp) {
	imp.Vid_GetModeInfo = Vid_GetModeInfo;

	gl_ref = GetRefAPIEngine_t(imp);
	gl_imp = imp;

	R_RenderFrameEngine = (void(__cdecl *)(void *))gl_ref.RenderFrame;
	gl_ref.RenderFrame = (int(__cdecl *)(refdef_t *))R_RenderFrame;

	return gl_ref;
}