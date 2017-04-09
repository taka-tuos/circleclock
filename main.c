#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>

#include "fontstash.h"

#include "tsprintf.h"

typedef struct {
	float m_LEDSegmentScaleNow[7];
	float m_LEDSegmentScaleTgt[7];
} LEDSegments_t;

typedef int (*gfxFuncDrawParticle)(int, int, int);
typedef int (*gfxFuncMoveParticle)(int*, int*, int);

const int c_gfxWidth = 800;
const int c_gfxHeight = 480;

int g_LEDSegmentPosition[7][3][2] = {
	{ // A
		{ 1,0 },
		{ 3,0 },
		{ 5,0 },
	},
	{ // B
		{ 
	6,1 },
		{ 6,3 },
		{ 6,5 },
	},
	{ // C
		{ 6,7 },
		{ 6,9 },
		{ 6,11 },
	},
	{ // D
		{ 1,12 },
		{ 3,12 },
		{ 5,12 },
	},
	{ // E
		{ 0,7 },
		{ 0,9 },
		{ 0,11 },
	},
	{ // F
		{ 0,1 },
		{ 0,3 },
		{ 0,5 },
	},
	{ // G
		{ 1,6 },
		{ 3,6 },
		{ 5,6 },
	},
};

int g_LEDSegmentFont[10][7] = {
	{ 1,1,1,1,1,1,0 }, //0
	{ 0,1,1,0,0,0,0 }, //1
	{ 1,1,0,1,1,0,1 }, //2
	{ 1,1,1,1,0,0,1 }, //3
	{ 0,1,1,0,0,1,1 }, //4
	{ 1,0,1,1,0,1,1 }, //5
	{ 1,0,1,1,1,1,1 }, //6
	{ 1,1,1,0,0,1,0 }, //7
	{ 1,1,1,1,1,1,1 }, //8
	{ 1,1,1,1,0,1,1 }, //9
};

char *g_strWhatDayArrayJp[] = {
	"日","月","火","水","木","金","土"
};

char *g_strWhatDayArrayUs[] = {
	"Sun","Mon","Tue","Wed","Thr","Fri","Sat"
};

char *g_strMonthNameArray[] = {
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

enum {
	FMT_YEAR_INT=0,	// %d
	FMT_MON_INT,	// %d
	FMT_MON_NAME,	// %s
	FMT_DAY_INT,	// %d
	FMT_DAY_TH,		// %d%s
	FMT_WDAY_JP,	// %s
	FMT_WDAY_US,	// %s
	
	FMT_LANG_JP=0x100,
	FMT_LANG_US,
};

int g_FormatDateJp[] = {
	FMT_YEAR_INT,
	FMT_MON_INT,
	FMT_DAY_INT,
	FMT_WDAY_JP,
	-1
};

int g_FormatDateUs[] = {
	FMT_MON_NAME,
	FMT_DAY_TH,
	FMT_YEAR_INT,
	FMT_WDAY_US,
	-1
};

char *g_strFormatDateJp = "%d年 %d月%d日 %s曜日";
char *g_strFormatDateUs = "%s %d%s %d(%s)";

int g_confLanguageSelect = FMT_LANG_JP;

LEDSegments_t g_ClockLED[6];
struct sth_stash* g_ttfStash;
int g_ttfFont;

float g_ParticlePositionList[512][3];
int g_ParticleEnableList[512];

float mathLinerStep(float v, float t, float s)
{
	float r = v;
	if(v < t) r = v + s;
	if(v > t) r = v - s;
	
	if(r > t && v < t) r = t;
	if(r < t && v > t) r = t;
	
	return r;
}

void gfxDrawCircle(float x, float y, float s, float r, float g, float b)
{
	int i, n = 16;
	glBegin(GL_POLYGON);
	glColor4f(r, g, b, 1.0);
	for (i = 0; i < n; i++) {
		float gx = x + s * cos(2.0f * 3.14f * ((float)i/n));
		float gy = y + s * sin(2.0f * 3.14f * ((float)i/n));
		glVertex2f(gx, gy);
	}
	glEnd();
}

void gfxDrawSegment(LEDSegments_t *t, float x, float y, float h)
{
	for(int s = 0; s < 2; s++) {
		for(int i = 0; i < 7; i++) {
			for(int j = 0; j < 3; j++) {
				if(t->m_LEDSegmentScaleNow[i]) {
					float px = (float)g_LEDSegmentPosition[i][j][0] / 12.0f;
					float py = (float)g_LEDSegmentPosition[i][j][1] / 12.0f;
					int c = (!s ? 0 : 1);
					gfxDrawCircle(x + px * h, y + py * h, t->m_LEDSegmentScaleNow[i] * h / 12.0f + (!s ? 2 : 0), c, c, c);
				}
			}
		}
	}
}

void gfxLEDSegmentMove(LEDSegments_t *t, int ms)
{
	for(int i = 0; i < ms; i++) {
		for(int j = 0; j < 7; j++) {
			t->m_LEDSegmentScaleNow[j] = 
				mathLinerStep(
					t->m_LEDSegmentScaleNow[j],
					t->m_LEDSegmentScaleTgt[j],
					1.0f / 800.0f
				);
		}
	}
}

void gfxLEDSegmentSetNumber(LEDSegments_t *t, int n)
{
	n %= 10;
	
	for(int i = 0; i < 7; i++) t->m_LEDSegmentScaleTgt[i] = g_LEDSegmentFont[n][i];
}

void sysQuitProgram(int code)
{
	SDL_Quit();

	exit(code);
}

void sysHandleKeyDown(SDL_keysym* keysym)
{
	switch(keysym->sym) {
	case SDLK_ESCAPE:
		sysQuitProgram(0);
		break;
	case SDLK_SPACE:
		g_confLanguageSelect ^= 1;
		break;
	default:
		break;
	}

}

void sysProcessEvents(void)
{
	SDL_Event event;
	
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
		case SDL_KEYDOWN:
			sysHandleKeyDown(&event.key.keysym);
			break;
		case SDL_QUIT:
			sysQuitProgram(0);
			break;
		}

	}

}

void strGetDateString(char *sz, int *fmt_list, char *fmt_str, struct tm *tmt)
{
	int p = 0, q = 0;
	long arg[16];
	
	while(fmt_list[p] != -1) {
		switch(fmt_list[p]) {
		case FMT_YEAR_INT:
			arg[q] = tmt->tm_year + 1900;
			break;
		case FMT_MON_INT:
			arg[q] = tmt->tm_mon + 1;
			break;
		case FMT_MON_NAME:
			arg[q] = (long)g_strMonthNameArray[tmt->tm_mon];
			break;
		case FMT_DAY_INT:
			arg[q] = tmt->tm_mday;
			break;
		case FMT_DAY_TH:
			arg[q] = tmt->tm_mday;
			q++;
			arg[q] = (long)"th";
			if(tmt->tm_mday == 1) arg[q] = (long)"st";
			if(tmt->tm_mday == 2) arg[q] = (long)"nd";
			if(tmt->tm_mday == 3) arg[q] = (long)"rd";
			if(tmt->tm_mday >= 21) {
				if((tmt->tm_mday % 10) == 1) arg[q] = (long)"st";
				if((tmt->tm_mday % 10) == 2) arg[q] = (long)"nd";
				if((tmt->tm_mday % 10) == 3) arg[q] = (long)"rd";
			}
			break;
		case FMT_WDAY_JP:
			arg[q] = (long)g_strWhatDayArrayJp[tmt->tm_wday];
			break;
		case FMT_WDAY_US:
			arg[q] = (long)g_strWhatDayArrayUs[tmt->tm_wday];
			break;
		}
		q++;
		p++;
	}
	
	tsprintf(sz,fmt_str,arg);
}

void gfxSetView2D(int width, int height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void gfxDrawString(float x, float y, float h, char *s)
{
	sth_begin_draw(g_ttfStash);
	sth_draw_text(g_ttfStash, g_ttfFont, h + 8, x, y + h, s, NULL);
	sth_end_draw(g_ttfStash);
}

void gfxCreateParticle(int x, int y, float s)
{
	for(int i = 0; i < 512; i++) {
		if(g_ParticleEnableList[i] == 0)
		{
			g_ParticleEnableList[i] = 1;
			g_ParticlePositionList[i][0] = x;
			g_ParticlePositionList[i][1] = y;
			g_ParticlePositionList[i][2] = s;
			break;
		}
	}
}

void gfxDrawParticle(float ms)
{
	for(int i = 0; i < 512; i++) {
		if(g_ParticleEnableList[i] != 0)
		{
			float x, y;
			x = g_ParticlePositionList[i][0];
			y = g_ParticlePositionList[i][1];
			gfxDrawCircle(x, y, 40.0f - g_ParticlePositionList[i][2] * 10.0f, 1.0f, 0.98f, 0.8f);
			g_ParticlePositionList[i][1] -= (480.0f + 40.0f + 40.0f) / (1800.0f - g_ParticlePositionList[i][2] * 500.0f) * ms;
			if(g_ParticlePositionList[i][1] < -40) g_ParticleEnableList[i] = 0;
		}
	}
}

void gfxDrawScreen(void)
{
	static int bms = 0;
	if(!bms) bms = SDL_GetTicks();
	
	static int bms_pt = 0;
	if(!bms_pt) bms_pt = SDL_GetTicks();
	
	int bfms = SDL_GetTicks() - bms;
	
	for(int i = 0; i < 6; i++) gfxLEDSegmentMove(&g_ClockLED[i], bfms);
	
	bms = SDL_GetTicks();
	
	if(SDL_GetTicks() - bms_pt > 100) {
		int x = (rand() % (800 + 20 + 20)) - 20;
		int y = (rand() % 80) + 480 + 20;
		float s = (float)rand() / (float)RAND_MAX;
		gfxCreateParticle(x, y, s);
		bms_pt = SDL_GetTicks();
	}
	
	glClearColor(1.0f, 0.4f, 0.8f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	time_t t;
	
	time(&t);
	
	struct tm *tmt = localtime(&t);
	
	int clocks[3];
	clocks[0] = tmt->tm_hour;
	clocks[1] = tmt->tm_min;
	clocks[2] = tmt->tm_sec;
	
	for(int i = 0; i < 6; i++) {
		int x = i / 2;
		int y = (i % 2);
		int n = clocks[x];
		if(!y) n /= 10;
		n %= 10;
		gfxLEDSegmentSetNumber(&g_ClockLED[i], n);
	}
	
	gfxDrawParticle(bfms);
	
	for(int i = 0; i < 6; i++) gfxDrawSegment(&g_ClockLED[i], 732.0f/6.0f*(float)i + 20.0f + (i / 2) * 24.0f, 176, 128);
	for(int i = 0; i < 2; i++) {
		gfxDrawCircle(732.0f/6.0f*(i+1)*2+7.0f + i * 24.0f, 176 + 32, 128.0f/12.0f+2, 0,0,0);
		gfxDrawCircle(732.0f/6.0f*(i+1)*2+7.0f + i * 24.0f, 176 + 96, 128.0f/12.0f+2, 0,0,0);
		gfxDrawCircle(732.0f/6.0f*(i+1)*2+7.0f + i * 24.0f, 176 + 32, 128.0f/12.0f, 1,1,1);
		gfxDrawCircle(732.0f/6.0f*(i+1)*2+7.0f + i * 24.0f, 176 + 96, 128.0f/12.0f, 1,1,1);
	}
	
	char sz[256];
	
	int *fmt_list = g_confLanguageSelect == FMT_LANG_JP ? g_FormatDateJp : g_FormatDateUs;
	char *fmt_str = g_confLanguageSelect == FMT_LANG_JP ? g_strFormatDateJp : g_strFormatDateUs;
	
	strGetDateString(sz,fmt_list, fmt_str, tmt);
	
	gfxDrawString(2,2,24,sz);
	
	SDL_GL_SwapBuffers();
}

void gfxInitialize(int width, int height)
{
	gfxSetView2D(width, height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int main(int argc, char* argv[])
{
	const SDL_VideoInfo* info = NULL;
	int width = 0;
	int height = 0;
	int bpp = 0;
	int flags = 0;

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Video initialization failed: %s\n",
			 SDL_GetError());
		sysQuitProgram(1);
	}
	
	info = SDL_GetVideoInfo();

	if(!info) {
		fprintf(stderr, "Video query failed: %s\n",
			 SDL_GetError());
		sysQuitProgram(1);
	}

	width = c_gfxWidth;
	height = c_gfxHeight;
	bpp = info->vfmt->BitsPerPixel;

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	flags = SDL_OPENGL;

	if( SDL_SetVideoMode(width, height, bpp, flags) == 0 ) {
		fprintf(stderr, "Video mode set failed: %s\n",
			 SDL_GetError());
		sysQuitProgram(1);
	}
	
	gfxInitialize(width, height);
	
	g_ttfStash = sth_create(512, 512);
	g_ttfFont = sth_add_font(g_ttfStash, "circle-mplus-1p-regular.ttf");
	
	while(1) {
		sysProcessEvents();
		gfxDrawScreen();
	}

	return 0;
}
