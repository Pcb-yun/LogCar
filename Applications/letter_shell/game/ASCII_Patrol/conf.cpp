/**
 * @file conf.cpp
 * @brief ASCII Patrol 游戏配置管理
 *
 * 实现游戏配置数据的定义和持久化
 * @version 1.0.0
 * @date 2026-06-08
 *
 * @copyright (c) 2026
 */

#include "conf.h"
#include "menu.h"
#include "spec.h"

/** 默认战役配置 */
ConfCampaign conf_campaign=
{
	0,0,0
};

ConfKeyboard conf_keyboard=
{
	{'A','D','W','S','L','Q'}
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
	conf_campaign.passed = 2;

	LoadMenu();
}

void SaveConf()
{
	// 配置不保存，使用默认配置
}

char ConfMapInput(char c)
{
	static const char m[6]=
	{
		KBD_LT,KBD_RT,KBD_UP,KBD_DN,13,27
	};

	while (1)
	{
		for (int i=0; i<6; i++)
		{
			if (conf_keyboard.map[i] == c)
				return m[i];
		}

		if (c>='a' && c<='z')
			c+='A'-'a';
		else
			break;
	}

	return c;
}
