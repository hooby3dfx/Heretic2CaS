/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include "../game/q_shared.h"


#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

vec3_t vec3_origin = { 0,0,0 };

#define	EQUAL_EPSILON	0.001

H2COMMON_API qboolean Vec3IsZeroEpsilon(vec3_t in);
H2COMMON_API void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);
H2COMMON_API void PerpendicularVector(vec3_t dst, const vec3_t src);
H2COMMON_API void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
H2COMMON_API vec_t VectorNormalize(vec3_t v);
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p);
H2COMMON_API vec_t DotProduct(vec3_t v1, vec3_t v2);
H2COMMON_API void Swap_Init(void);
H2COMMON_API vec_t VectorNormalize2(vec3_t in, vec3_t out);

#define VectorClear(x) {x[0] = x[1] = x[2] = 0;}

//============================================================================

#ifdef _WIN32
#pragma optimize( "", off )
#endif

H2COMMON_API void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees)
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	CrossProduct(vr, vf, vup);
	VectorNormalize(vup);
	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	zrot[0][0] = cos(DEG2RAD(degrees));
	zrot[0][1] = sin(DEG2RAD(degrees));
	zrot[1][0] = -sin(DEG2RAD(degrees));
	zrot[1][1] = cos(DEG2RAD(degrees));

	R_ConcatRotations(m, zrot, tmpmat);
	R_ConcatRotations(tmpmat, im, rot);

	for (i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif

H2COMMON_API void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI * 2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI * 2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI * 2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1 * sr*sp*cy + -1 * cr*-sy);
		right[1] = (-1 * sr*sp*sy + -1 * cr*cy);
		right[2] = -1 * sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy + -sr*-sy);
		up[1] = (cr*sp*sy + -sr*cy);
		up[2] = cr*cp;
	}
}


H2COMMON_API void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct(normal, normal);

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
H2COMMON_API void PerpendicularVector(vec3_t dst, const vec3_t src)
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabs(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	VectorNormalize(dst);
}



/*
================
R_ConcatRotations
================
*/
H2COMMON_API void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
}


/*
================
R_ConcatTransforms
================
*/
H2COMMON_API void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
		in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
		in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
		in1[2][2] * in2[2][3] + in1[2][3];
}


//============================================================================


#if defined _M_IX86 && !defined C_ONLY
#pragma warning (disable:4035)
__declspec(naked) long Q_ftol(float f)
{
	static int tmp;
	__asm fld dword ptr[esp + 4]
		__asm fistp tmp
	__asm mov eax, tmp
	__asm ret
}
#pragma warning (default:4035)
#endif

/*
===============
LerpAngle

===============
*/
H2COMMON_API float LerpAngle(float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}


H2COMMON_API float	anglemod(float a)
{
	a = (360.0 / 65536) * ((int)(a*(65536 / 360.0)) & 65535);
	return a;
}

H2COMMON_API float	anglemod_old(float a1)
{
	double v2; // st7@1
	char v3 = 0; // c0@1
	double result; // st7@2

	v2 = a1;
	if (v3)
		result = (double)(signed int)(360 - -360 * (unsigned __int64)(signed __int64)(v2 * -0.0027777778)) + a1;
	else
		result = a1 - (double)(signed int)(360 * (unsigned __int64)(signed __int64)(v2 * 0.0027777778));
	return result;
}

H2COMMON_API float Clamp(float src, float min, float max)
{
	float result; // st7@4

	if (src > (float)max)
		src = max;
	if (src >= (float)min)
		result = src;
	else
		result = min;
	return result;
}

H2COMMON_API int ClampI(int src, int min, int max)
{
	int result; // eax@1

	result = src;
	if (src > max)
		result = max;
	if (result < min)
		result = min;
	return result;
}

int		i;
vec3_t	corners[2];


// this is the slow, general version
H2COMMON_API int BoxOnPlaneSide2(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i = 0; i<3; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct(p->normal, corners[0]) - p->dist;
	dist2 = DotProduct(p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		assert(0);
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	assert(sides != 0);

	return sides;
}

H2COMMON_API void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

H2COMMON_API void AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i = 0; i<3; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}


H2COMMON_API int VectorCompare(vec3_t v1, vec3_t v2)
{
	int		i;

	for (i = 0; i < 3; i++)
		if (fabs(v1[i] - v2[i]) > EQUAL_EPSILON)
			return false;

	return true;
}

#pragma optimize("g", off)	// went back to turning optimization off,
// the bug_fix thing stopped working
H2COMMON_API vec_t VectorNormalize(vec3_t v)
{
	return VectorNormalize2(v, v);
}

H2COMMON_API vec_t VectorNormalize2(vec3_t in, vec3_t out)
{
	vec_t	length, ilength;

	length = sqrt(in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);
	if (length == 0)
	{
		VectorClear(out);
		return 0;
	}

	ilength = 1.0 / length;
	out[0] = in[0] * ilength;
	out[1] = in[1] * ilength;
	out[2] = in[2] * ilength;

	return length;

}
#pragma optimize("", on)

H2COMMON_API void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


H2COMMON_API vec_t DotProduct(vec3_t v1, vec3_t v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

H2COMMON_API void VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

H2COMMON_API void VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

H2COMMON_API void VectorCopy(vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

H2COMMON_API void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

double sqrt(double x);

H2COMMON_API vec_t VectorLength(vec3_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i = 0; i< 3; i++)
		length += v[i] * v[i];
	length = sqrt(length);		// FIXME

	return length;
}

H2COMMON_API void VectorInverse(vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

H2COMMON_API void VectorScale(vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}


H2COMMON_API int Q_log2(int val)
{
	int answer = 0;
	while (val >>= 1)
		answer++;
	return answer;
}



//====================================================================================

/*
============
COM_SkipPath
============
*/
H2COMMON_API char *COM_SkipPath(char *pathname)
{
	char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname == '/')
			last = pathname + 1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
H2COMMON_API void COM_StripExtension(char *in, char *out)
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}

/*
============
COM_FileExtension
============
*/
H2COMMON_API char *COM_FileExtension(char *in)
{
	static char exten[8];
	int		i;

	while (*in && *in != '.')
		in++;
	if (!*in)
		return "";
	in++;
	for (i = 0; i<7 && *in; i++, in++)
		exten[i] = *in;
	exten[i] = 0;
	return exten;
}

/*
============
COM_FileBase
============
*/
H2COMMON_API void COM_FileBase(char *in, char *out)
{
	char *s, *s2;

	s = in + strlen(in) - 1;

	while (s != in && *s != '.')
		s--;

	for (s2 = s; s2 != in && *s2 != '/'; s2--)
		;

	if (s - s2 < 2)
		out[0] = 0;
	else
	{
		s--;
		strncpy(out, s2 + 1, s - s2);
		out[s - s2] = 0;
	}
}

/*
============
COM_FilePath

Returns the path up to, but not including the last /
============
*/
H2COMMON_API void COM_FilePath(char *in, char *out)
{
	char *s;

	s = in + strlen(in) - 1;

	while (s != in && *s != '/')
		s--;

	strncpy(out, in, s - in);
	out[s - in] = 0;
}


/*
==================
COM_DefaultExtension
==================
*/
H2COMMON_API void COM_DefaultExtension(char *path, char *extension)
{
	char    *src;
	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat(path, extension);
}

/*
============================================================================

BYTE ORDER FUNCTIONS

============================================================================
*/

qboolean	bigendien;

// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
H2COMMON_API short(*_BigShort) (short l);
H2COMMON_API short(*_LittleShort) (short l);
H2COMMON_API int(*_BigLong) (int l);
H2COMMON_API int(*_LittleLong) (int l);
H2COMMON_API float(*_BigFloat) (float l);
H2COMMON_API float(*_LittleFloat) (float l);

H2COMMON_API short	BigShort(short l) { if (!_BigShort) { Swap_Init(); } _BigShort(l); }
//short	LittleShort(short l) { return _LittleShort(l); }
H2COMMON_API int		BigLong(int l) { if (!_BigShort) { Swap_Init(); } return _BigLong(l); }
//int		LittleLong(int l) { return _LittleLong(l); }
H2COMMON_API float	BigFloat(float l) { if (!_BigShort) { Swap_Init(); } return _BigFloat(l); }
//float	LittleFloat(float l) { return _LittleFloat(l); }

H2COMMON_API short   ShortSwap(short l)
{
	byte    b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return (b1 << 8) + b2;
}

H2COMMON_API short	ShortNoSwap(short l)
{
	return l;
}

H2COMMON_API int    LongSwap(int l)
{
	byte    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

H2COMMON_API int	LongNoSwap(int l)
{
	return l;
}

H2COMMON_API float FloatSwap(float f)
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

H2COMMON_API float FloatNoSwap(float f)
{
	return f;
}

/*
=================
SetPlaneSignbits
=================
*/
H2COMMON_API void SetPlaneSignbits(cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j = 0; j < 3; j++) {
		if (out->normal[j] < 0) {
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}


/*
================
Swap_Init
================
*/
H2COMMON_API void Swap_Init(void)
{
	byte	swaptest[2] = { 1,0 };

	// set the byte swapping variables in a portable manner	
	if (*(short *)swaptest == 1)
	{
		bigendien = false;
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = true;
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}

}



/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
H2COMMON_API char	*va(char *format, ...)
{
	va_list		argptr;
	static char		string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}


char	com_token[MAX_TOKEN_CHARS];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
H2COMMON_API char *COM_Parse(char **data_p)
{
	int		c;
	int		len;
	char	*data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*data_p = NULL;
		return "";
	}

	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return "";
		}
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
		//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}


/*
===============
Com_PageInMemory

===============
*/
int	paged_total;

H2COMMON_API void Com_PageInMemory(byte *buffer, int size)
{
	int		i;

	for (i = size - 1; i>0; i -= 4096)
		paged_total += buffer[i];
}



/*
============================================================================

LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

H2COMMON_API void Com_sprintf(char *dest, int size, char *fmt, ...)
{
	int		len;
	va_list		argptr;
	char	bigbuffer[0x10000];

	va_start(argptr, fmt);
	len = vsprintf(bigbuffer, fmt, argptr);
	va_end(argptr);
	if (len >= size)
		Com_Printf("Com_sprintf: overflow of %i in %i\n", len, size);
	strncpy(dest, bigbuffer, size - 1);
}

/*
=====================================================================

INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
H2COMMON_API char *Info_ValueForKey(char *s, char *key)
{
	char	pkey[512];
	static	char value[2][512];	// use two buffers so compares
								// work without stomping on each other
	static	int	valueindex;
	char	*o;

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp(key, pkey))
			return value[valueindex];

		if (!*s)
			return "";
		s++;
	}
}

H2COMMON_API void Info_RemoveKey(char *s, char *key)
{
	char	*start;
	char	pkey[512];
	char	value[512];
	char	*o;

	if (strstr(key, "\\"))
	{
		//		Com_Printf ("Can't use a key with a \\\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp(key, pkey))
		{
			strcpy(start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
H2COMMON_API qboolean Info_Validate(char *s)
{
	if (strstr(s, "\""))
		return false;
	if (strstr(s, ";"))
		return false;
	return true;
}

H2COMMON_API void Info_SetValueForKey(char *s, char *key, char *value)
{
	char	newi[MAX_INFO_STRING], *v;
	int		c;
	int		maxsize = MAX_INFO_STRING;

	if (strstr(key, "\\") || strstr(value, "\\"))
	{
		Com_Printf("Can't use keys or values with a \\\n");
		return;
	}

	if (strstr(key, ";"))
	{
		Com_Printf("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strstr(key, "\"") || strstr(value, "\""))
	{
		Com_Printf("Can't use keys or values with a \"\n");
		return;
	}

	if (strlen(key) > MAX_INFO_KEY - 1 || strlen(value) > MAX_INFO_KEY - 1)
	{
		Com_Printf("Keys and values must be < 64 characters.\n");
		return;
	}
	Info_RemoveKey(s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > maxsize)
	{
		Com_Printf("Info string length exceeded\n");
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newi;
	while (*v)
	{
		c = *v++;
		c &= 127;		// strip high bits
		if (c >= 32 && c < 127)
			*s++ = c;
	}
	*s = 0;
}

//====================================================================


