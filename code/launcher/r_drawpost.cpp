// r_drawPost.cpp
//

#include <windows.h>
#include <string>
#include "MinHook.h"
#include "MinHook.h"
#include "../game/q_shared.h"
#include "../client/ref.h"

#include "glew.h"
#include "launcher.h"

GLuint casProgram;
GLuint casRenderTexture = 0;

void R_GetWindowDimen(int *width, int *height);

const char *Quake_CasShader_Vertex = {
	"#version 120\n"
	"varying vec4 texcoords;\n"
	"void main() {\n"
	"gl_Position = ftransform();\n" // AWFUL!!!
	"texcoords = gl_MultiTexCoord0;\n"
	"}\n"
};

const char *Quake_PostCaSShader_Pixel = {
"#version 140\n"
"varying vec4 texcoords;\n"
"float Min3(float x, float y, float z)\n"
"{\n"
	"return min(x, min(y, z));\n"
"}\n"
"float Max3(float x, float y, float z)\n"
"{\n"
	"return max(x, max(y, z));\n"
"}\n"
"float rcp(float v)\n"
"{\n"
	"return 1.0 / v;\n"
"}\n"
"vec4 CaSUpSample(sampler2D diffuseImage, vec2 diffuseST)\n"
"{\n"
	"vec2 texcoord = diffuseST.xy;\n"
	"texcoord.y = 1.0 - texcoord.y;\n"
	"float Sharpness = 1;\n"

	"ivec2 bufferSize = textureSize(diffuseImage, 0);\n"
	"float pixelX = (1.0 / bufferSize.x);\n"
	"float pixelY = (1.0 / bufferSize.y);\n"

	"vec3 a = texture2D(diffuseImage, texcoord + vec2(-pixelX, -pixelY)).rgb;\n"
	"vec3 b = texture2D(diffuseImage, texcoord + vec2(0.0, -pixelY)).rgb;\n"
	"vec3 c = texture2D(diffuseImage, texcoord + vec2(pixelX, -pixelY)).rgb;\n"
	"vec3 d = texture2D(diffuseImage, texcoord + vec2(-pixelX, 0.0)).rgb;\n"
	"vec3 e = texture2D(diffuseImage, texcoord).rgb;\n"
	"vec3 f = texture2D(diffuseImage, texcoord + vec2(pixelX, 0.0)).rgb;\n"
	"vec3 g = texture2D(diffuseImage, texcoord + vec2(-pixelX, pixelY)).rgb;\n"
	"vec3 h = texture2D(diffuseImage, texcoord + vec2(0.0, pixelY)).rgb;\n"
	"vec3 i = texture2D(diffuseImage, texcoord + vec2(pixelX, pixelY)).rgb;\n"

	"float mnR = Min3(Min3(d.r, e.r, f.r), b.r, h.r);\n"
	"float mnG = Min3(Min3(d.g, e.g, f.g), b.g, h.g);\n"
	"float mnB = Min3(Min3(d.b, e.b, f.b), b.b, h.b);\n"

	"float mnR2 = Min3(Min3(mnR, a.r, c.r), g.r, i.r);\n"
	"float mnG2 = Min3(Min3(mnG, a.g, c.g), g.g, i.g);\n"
	"float mnB2 = Min3(Min3(mnB, a.b, c.b), g.b, i.b);\n"
	"mnR = mnR + mnR2;\n"
	"mnG = mnG + mnG2;\n"
	"mnB = mnB + mnB2;\n"

	"float mxR = Max3(Max3(d.r, e.r, f.r), b.r, h.r);\n"
	"float mxG = Max3(Max3(d.g, e.g, f.g), b.g, h.g);\n"
	"float mxB = Max3(Max3(d.b, e.b, f.b), b.b, h.b);\n"

	"float mxR2 = Max3(Max3(mxR, a.r, c.r), g.r, i.r);\n"
	"float mxG2 = Max3(Max3(mxG, a.g, c.g), g.g, i.g);\n"
	"float mxB2 = Max3(Max3(mxB, a.b, c.b), g.b, i.b);\n"
	"mxR = mxR + mxR2;\n"
	"mxG = mxG + mxG2;\n"
	"mxB = mxB + mxB2;\n"

	"float rcpMR = rcp(mxR);\n"
	"float rcpMG = rcp(mxG);\n"
	"float rcpMB = rcp(mxB);\n"

	"float ampR = clamp(min(mnR, 2.0 - mxR) * rcpMR, 0, 1);\n"
	"float ampG = clamp(min(mnG, 2.0 - mxG) * rcpMG, 0, 1);\n"
	"float ampB = clamp(min(mnB, 2.0 - mxB) * rcpMB, 0, 1);\n"

	"ampR = sqrt(ampR);\n"
	"ampG = sqrt(ampG);\n"
	"ampB = sqrt(ampB);\n"

	"float peak = -rcp(mix(8.0, 5.0, clamp(Sharpness, 0, 1)));\n"

	"float wR = ampR * peak;\n"
	"float wG = ampG * peak;\n"
	"float wB = ampB * peak;\n"

	"float rcpWeightR = rcp(1.0 + 4.0 * wR);\n"
	"float rcpWeightG = rcp(1.0 + 4.0 * wG);\n"
	"float rcpWeightB = rcp(1.0 + 4.0 * wB);\n"

	"vec3 outColor = vec3(clamp((b.r*wR + d.r*wR + f.r*wR + h.r*wR + e.r)*rcpWeightR, 0, 1),\n"
	"	clamp((b.g*wG + d.g*wG + f.g*wG + h.g*wG + e.g)*rcpWeightG, 0, 1),\n"
	"	clamp((b.b*wB + d.b*wB + f.b*wB + h.b*wB + e.b)*rcpWeightB, 0, 1));\n"

	"return vec4(outColor, 1.0);\n"
	"}\n"

	"uniform sampler2D diffuseTex;"
	"void main() {\n"
	"	gl_FragColor = CaSUpSample(diffuseTex, texcoords.xy);\n"
	"}\n"
};

/*
====================
Quake_CopyFramebuffer
====================
*/
void Quake_CopyFramebuffer(GLuint texId, int x, int y, int imageWidth, int imageHeight) {
	glBindTexture(GL_TEXTURE_2D, texId);

	glReadBuffer(GL_BACK);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, x, y, imageWidth, imageHeight, 0);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static int MakePowerOfTwo(int num) {
	int pot;
	for (pot = 1; pot < num; pot <<= 1) {}
	return pot;
}


void Quake_DrawFullscreenPass(GLuint program, GLuint texId, int width, int height)
{

	glUseProgram(program);

	//match projection to window resolution (could be in reshape callback)
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, texId);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 1); glVertex2f(0, 0);
	glTexCoord2i(1, 1); glVertex2f(width, 0);
	glTexCoord2i(1, 0); glVertex2f(width, height);
	glTexCoord2i(0, 0); glVertex2f(0, height);
	glEnd();

	glUseProgram(0);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void Quake_RenderCaSPass(void)
{

	if (casRenderTexture == 0)
	{
		glGenTextures(1, &casRenderTexture);
	}

	int xdim, ydim;
	R_GetWindowDimen(&xdim, &ydim);

	Quake_CopyFramebuffer(casRenderTexture, 0, 0, xdim, ydim);
	Quake_DrawFullscreenPass(casProgram, casRenderTexture, xdim, ydim);
}

void Quake_CompileShader(GLuint shader)
{
	glCompileShader(shader);

	int infologLength = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1) {
		char *infoLog = new char[infologLength];
		int charsWritten = 0;
		glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);

		if (strstr(infoLog, "error") || strstr(infoLog, "ERROR"))
		{
			Com_Printf("FATAL ERROR: %s\n", infoLog);
			exit(0);
		}
		else
		{
			Com_Printf("While compiling CaS shader...\n");
			Com_Printf("%s\n", infoLog);
		}
		delete infoLog;
	}
}

void Quake_CasInit(void) {
	GLuint casVertexShader;
	GLuint casPixelShader;

	casVertexShader = glCreateShader(GL_VERTEX_SHADER);
	casPixelShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(casVertexShader, 1, &Quake_CasShader_Vertex, NULL);
	glShaderSource(casPixelShader, 1, &Quake_PostCaSShader_Pixel, NULL);

	Quake_CompileShader(casVertexShader);
	Quake_CompileShader(casPixelShader);

	casProgram = glCreateProgram();
	glAttachShader(casProgram, casVertexShader);
	glAttachShader(casProgram, casPixelShader);

	glLinkProgram(casProgram);
}

//
// R_SetFOV
//
void R_SetFOV(refdef_t *refdef)
{
	float fov = R_GetFOV();
	int xdim, ydim;
	R_GetWindowDimen(&xdim, &ydim);
	
	float x = (float)xdim / tan(fov / 360 * M_PI);
	float y = atan2(ydim, x);

	refdef->fov_y = y * 360 / M_PI;

	float ratio_x = 16.0f;
	float ratio_y = 9.0f;

	y = ratio_y / tan(refdef->fov_y / 360.0f * M_PI);
	refdef->fov_x = atan2(ratio_x, y) * 360.0f / M_PI;
}

//
// R_RenderFrame
//
void __cdecl R_RenderFrame(refdef_t *refdef) {
	static bool isVidInit = false;
	
	R_SetFOV(refdef);

	if(!isVidInit)
	{
		glewInit();
		Quake_CasInit();
		isVidInit = true;
	}

	R_RenderFrameEngine(refdef);

	if (R_RenderCaS())
	{
		Quake_RenderCaSPass();
	}
}

