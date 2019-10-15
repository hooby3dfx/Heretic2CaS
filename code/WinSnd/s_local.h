// s_local.h
//

#pragma once

extern "C"
{
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
};

#include <vector>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

//
// sfx_t
//
struct sfx_t
{
	sfx_t() {
		memset(this, 0, sizeof(sfx_t));
	};

	char soundName[512];
	bool isLoaded;

	ALuint buffer;
};

struct sndGlobals_t {
	ALCdevice *device;
	ALCcontext *context;

	ALuint		voices[32];
	ALuint		music_voice;

	vec3_t		prev_origin;

	std::vector<sfx_t *> sfxList;
};

extern sndGlobals_t sndGlobal;