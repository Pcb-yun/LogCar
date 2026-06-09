/**
 * @file twister.cpp
 * @brief ASCII Patrol 随机数生成器
 *
 * 实现基于 XORShift 的伪随机数生成器
 */

#include "game_en.h"
#if GAME_ENABLE_AP


#include <stdlib.h>
#include "twister.h"

/**
 * @brief 随机数生成器状态结构
 */
struct TWISTER
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
};

static TWISTER state[2]=
{
	{
		123456789,
		362436069,
		521288629
	},
	{
		123456789,
		362436069,
		521288629
	}
};

static TWISTER* twister = state;

void twister_switch(int i)
{
	twister = state+(i&1);
}

void twister_seed(unsigned int s)
{
	twister->x = s;
	twister->y = 362436069;
	twister->z = 521288629;

	twister_rand();
	twister_rand();
	twister_rand();
}

int twister_rand()
{
	unsigned int t;

	twister->x ^= twister->x << 16;
	twister->x ^= twister->x >> 5;
	twister->x ^= twister->x << 1;

	t = twister->x;
	twister->x = twister->y;
	twister->y = twister->z;
	twister->z = t ^ twister->x ^ twister->y;

	return twister->z & 0x7fffffff;
}

#endif /* GAME_ENABLE_AP */
