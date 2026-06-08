/**
 * @file twister.cpp
 * @brief ASCII Patrol 随机数生成器
 *
 * 实现基于 XORShift 的伪随机数生成器
 * @version 1.0.0
 * @date 2026-06-08
 *
 * @copyright (c) 2026
 */

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

void dbg_get_twister_state(unsigned int s[6])
{
	s[0]=state[0].x; s[1]=state[0].y; s[2]=state[0].z; 
	s[3]=state[1].x; s[4]=state[1].y; s[5]=state[1].z; 
}

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

#if 0
static const unsigned long SIZE   = 624;
static const unsigned long PERIOD = 397;
static const unsigned long DIFF   = SIZE-PERIOD;

static unsigned long MT[SIZE];
static unsigned long index = 0;

#define M32(x) (0x80000000 & x) // 32nd Most Significant Bit
#define L31(x) (0x7FFFFFFF & x) // 31 Least Significant Bits
#define ODD(x) (x & 1) // Check if number is odd

#define UNROLL(expr) y = M32(MT[i]) | L31(MT[i+1]); MT[i] = MT[expr] ^ (y>>1) ^ MATRIX[ODD(y)]; ++i;

void twister_seed(unsigned long s)
{
  MT[0] = s;
  index = 0;

  for ( unsigned i=1; i<SIZE; ++i )
    MT[i] = 0x6c078965*(MT[i-1] ^ MT[i-1]>>30) + i;
}

int twister_rand()
{
  if ( !index )
  {
	  static const unsigned long MATRIX[2] = {0, 0x9908b0df};
	  unsigned long y, i=0;

	  while ( i<(DIFF-1) ) 
	  {
		UNROLL(i+PERIOD);
		UNROLL(i+PERIOD);
	  }

	  UNROLL((i+PERIOD) % SIZE);

	  while ( i<(SIZE-1) ) 
	  {
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
		UNROLL(i-DIFF);
	  }

	  y = M32(MT[SIZE-1]) | L31(MT[0]);
	  MT[SIZE-1] = MT[PERIOD-1] ^ (y>>1) ^ MATRIX[ODD(y)];  
  }

  unsigned long y = MT[index];

  y ^= y>>11;
  y ^= y<< 7 & 0x9d2c5680;
  y ^= y<<15 & 0xefc60000;
  y ^= y>>18;

  if ( ++index == SIZE )
    index = 0;

  return (int)(y&~(1<<31));
}
#endif
