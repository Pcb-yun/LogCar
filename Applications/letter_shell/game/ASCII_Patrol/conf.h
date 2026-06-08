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

struct ConfKeyboard
{
	char map[6];
};

struct ConfPlayer
{
	char name[16];
	unsigned int avatar;
};

extern ConfCampaign conf_campaign;
extern ConfKeyboard conf_keyboard;
extern ConfPlayer   conf_player;

extern "C"
{
	void LoadConf();
	void SaveConf();
}

char ConfMapInput(char c);

#endif

#endif /* GAME_ENABLE_AP */
