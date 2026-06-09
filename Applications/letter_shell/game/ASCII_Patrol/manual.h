
#include "game_en.h"
#if GAME_ENABLE_AP

#ifndef MANUAL_H
#define MANUAL_H

#include "game_ap.h"

struct MANUAL : SCREEN
{
	virtual ~MANUAL();
	MANUAL(const ASSET* a, char transp = ' ');
	virtual void Paint(SCREEN* scr, int dx, int dy, int sx, int sy, int sw=-1, int sh=-1, bool blend=false);


	SPRITE man;
	SCREEN* paper[10];

	int sheets;
	int page;
	int anim;
	int half_w;

	void ClearPaper(int i);
	void SetPage(int i, int f);
};


#endif

#endif /* GAME_ENABLE_AP */
