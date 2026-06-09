/**
 * @file conf.h
 * @brief ASCII Patrol 游戏配置管理
 */

#include "game_en.h"
#if GAME_ENABLE_AP

#ifndef CONF_H
#define CONF_H

struct ConfCampaign
{
	int passed;
	int course;
	int level;
};

struct ConfPlayer
{
	char name[16];
	unsigned int avatar;
};

extern ConfCampaign conf_campaign;
extern ConfPlayer   conf_player;

extern "C"
{
	void LoadConf();
}

char ConfMapInput(char c);

#endif

#endif /* GAME_ENABLE_AP */
