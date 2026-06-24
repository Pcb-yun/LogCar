
#include "game_en.h"
#if GAME_ENABLE_AP

#ifndef SPEC_H
#define SPEC_H

struct MODAL
{
	virtual ~MODAL() {}
	virtual int Run() = 0;
};

extern MODAL* modal;

int terminal_init(int argc, char* argv[], int* dw, int* dh);
void terminal_done();

void terminal_loop();

struct CON_OUTPUT
{
	int w,h;
	char* buf;
	void* arr;
};

void free_con_output(CON_OUTPUT* screen);
void resize_con_output(CON_OUTPUT* s, int _w, int _h, char t); // not specialized!

void get_terminal_wh(int* dw, int* dh);

int screen_write(CON_OUTPUT* screen, int dw, int dh, int sx, int sy, int sw, int sh);

#define CON_INPUT_KBD 0x0001
#define CON_INPUT_FOC 0x0002
#define CON_INPUT_UNK 0xFFFF

#define CON_INPUT_TCH_BEGIN 0x0003
#define CON_INPUT_TCH_MOVE  0x0004
#define CON_INPUT_TCH_END   0x0005

// non-ascii key mappings
// BUT: bkspc=8, tab=9, esc=27, enter=13
#define KBD_LT	1
#define KBD_RT	2
#define KBD_UP	3
#define KBD_DN	4
#define KBD_DEL	5
#define KBD_INS 6
#define KBD_HOM 14
#define KBD_END 15
#define KBD_PUP 16
#define KBD_PDN	17

struct CON_INPUT
{
    int EventType;
    union
    {
		struct
		{
			bool bKeyDown;
			struct
			{
				char AsciiChar;
			} uChar;
		} KeyEvent;

		struct
		{
			bool bSetFocus;
		} FocusEvent;

		struct
		{
			int x,y;
			int id;
		} TouchEvent;

    } Event;
};

void vsync_wait();
void sleep_ms(int ms);
unsigned int get_time();

bool get_input_len( int* r);
bool spec_read_input( CON_INPUT* ir, int n, int* r);
bool read_input( CON_INPUT* ir, int n, int* r);
bool has_key_releases();

#ifndef WIN

#define sprintf_s(dst,size,...) sprintf(dst,__VA_ARGS__)

#define sscanf_s(src,fmt,...) sscanf(src,fmt,__VA_ARGS__)

#define _strdup(str) strdup(str)

#define strcpy_s(dst,size,src) strcpy(dst,src)

#endif

#ifdef DOS
float sqrtf(float f);
float logf(float f);
float floorf(float f);
float sinf(float f);
float cosf(float f);
float expf(float f);
float powf(float f, float n);
#else
float sqrtf(float f);
float logf(float f);
float log(float x);
float floorf(float f);
float sinf(float f);
float cosf(float f);
float expf(float f);
float sin(float x);
#endif

#define ABS(a) ((a)<0 ? -(a):(a))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

void app_exit();
#endif

#endif /* GAME_ENABLE_AP */
