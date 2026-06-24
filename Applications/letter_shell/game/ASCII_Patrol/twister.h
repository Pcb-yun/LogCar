
#include "game_en.h"
#if GAME_ENABLE_AP

#ifndef TWISTER_H
#define TWISTER_H

void twister_switch(int i);

void twister_seed(unsigned int s);
int  twister_rand();

#endif


#endif /* GAME_ENABLE_AP */
