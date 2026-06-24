/**
 * @file conf.cpp
 * @brief ASCII Patrol 游戏配置管理
 */

#include "game_en.h"
#if GAME_ENABLE_AP


#include "conf.h"
#include "menu.h"
#include "spec.h"

/** 默认战役配置 */
ConfCampaign conf_campaign=
{
	0,0,0
};

ConfPlayer conf_player=
{
	"Player 1",
	0x00000000
};

void LoadConf()
{
	// 使用默认配置
	conf_campaign.course = 0;
	conf_campaign.level = 0;
	conf_campaign.passed = 99;	// 全解锁，允许自由选择所有关卡

	LoadMenu();
}

char ConfMapInput(char c)
{
	static const char keys[6] = {'A','D','W','S','L','Q'};
	static const char m[6] = {KBD_LT,KBD_RT,KBD_UP,KBD_DN,13,27};

	while (1)
	{
		for (int i=0; i<6; i++)
		{
			if (keys[i] == c)
				return m[i];
		}

		if (c>='a' && c<='z')
			c+='A'-'a';
		else
			break;
	}

	return c;
}

#endif /* GAME_ENABLE_AP */
