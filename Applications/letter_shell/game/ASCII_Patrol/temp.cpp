/**
 * @file temp.cpp
 * @brief ASCII Patrol 游戏入口与模态框管理
 *
 * 实现游戏主入口、战役模式、菜单模态框和开场动画
 */

#include "game_en.h"
#if GAME_ENABLE_AP


#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "shell.h"
#include "cmsis_os.h"
#include "Events.h"
#include "memory.h"
#include "twister.h"

#include "inter.h"
#include "gameover.h"
#include "game_ap.h"
#include "spec.h"
#include "menu.h"
#include "conf.h"
#include "manual.h"

MODAL* modal = 0;
SCREEN global_screen(110, 25, ' ');

/**
 * @brief 战役模式模态框
 */
struct CAMPAIGN_MODAL : MODAL {
	MODAL* level_modal;		/**< 当前关卡模态框 */
	MODAL* inter_modal;		/**< 关卡间界面模态框 */

	int iCourse;			/**< 当前课程索引 */
	int iLevel;				/**< 当前关卡索引 */
	int iSubLev;			/**< 当前子关卡索引 */

	SCREEN* s;				/**< 屏幕指针 */
	int w;					/**< 屏幕宽度 */
	int h;					/**< 屏幕高度 */

	int lives;				/**< 剩余生命 */
	int score;				/**< 当前得分 */
	const COURSE* current_course;	/**< 当前课程 */
	const LEVEL* current_level;		/**< 当前关卡 */

	int level_time;			/**< 关卡时间 */

	int startlives;			/**< 进入关卡时的生命数 */
	char hitbin[1024];		/**< 命中统计（最大关卡大小8192） */

	/**
	 * @brief 析构函数
	 */
	virtual ~CAMPAIGN_MODAL()
	{
		if (level_modal) {
			delete level_modal;
		}
		if (inter_modal) {
			delete inter_modal;
		}
	}

	/**
	 * @brief 构造函数
	 *
	 * @param scr 屏幕对象
	 * @param ic  课程索引
	 * @param il  关卡索引
	 */
	CAMPAIGN_MODAL(SCREEN* scr, int ic, int il)
	{
		s = scr;
		w = scr->w;
		h = scr->h;

		iCourse = ic;
		iLevel	= il;
		iSubLev = 0;

		lives = 3;
		score = 0;
		current_course = campaign + iCourse;
		current_level = current_course->level + iLevel;

		startlives = lives;

		memset(hitbin, 0, 1024);
		level_time = 0;

		level_modal = new LEVEL_MODAL(
			&global_screen,
			lives, startlives,
			&score,
			&level_time,
			conf_player.name,
			current_course->name,
			current_level,
			iSubLev, hitbin);

		inter_modal = 0;
	}

	/**
	 * @brief 运行战役模式
	 *
	 * @return int -2表示临时退出到菜单，-3表示永久结束
	 */
	virtual int Run()
	{
		if (level_modal) {
			int ret = level_modal->Run();

			w = s->w;
			h = s->h;

			if (ret >= 0) {
				lives--;

				if (lives) {
					iSubLev = ret;

					delete level_modal;

					level_modal = new LEVEL_MODAL(
						&global_screen,
						lives, startlives,
						&score,
						&level_time,
						conf_player.name,
						current_course->name,
						current_level,
						iSubLev, hitbin);
				} else {
					delete level_modal;
					level_modal = 0;

					inter_modal = new GAMEOVER_MODAL(
						&global_screen,
						&score,
						current_level);
				}

				return 0;
			}

			if (ret == -2) {
				return -2;
			}

			if (ret == -3) {
				return -1;
			}

			int hitacc = 0;
			for (int i = 0; i < 1024; i++) {
				unsigned char b = hitbin[i];
				while (b) {
					if (b & 1) {
						hitacc++;
					}
					b >>= 1;
				}
			}

			int hitmax = 0;
			for (int i = 0; current_level->sprite[i]; i++) {
				char spr = current_level->sprite[i];
				switch (spr) {
					case 'U':
					case 'D':
					case 'B':
					case 'r':
					case 't':
					case 'c':
					case 'h':
					case 'b':
						hitmax++;
				}
			}

			delete level_modal;
			level_modal = 0;

			inter_modal = new INTER_MODAL(
				&global_screen,
				lives, startlives,
				&score,
				current_course->name,
				current_level,
				level_time,
				hitacc, hitmax);

			level_time = 0;
			hitacc = 0;
			startlives = lives;
			memset(hitbin, 0, 1024);

			iSubLev = 0;
			current_level++;
			iLevel++;
			if (!current_level->height) {
				current_course++;
				iCourse++;

				if (iCourse > conf_campaign.passed) {
					conf_campaign.passed = iCourse;

				}

				if (!current_course->level || (current_course->flags & 0x1)) {
					current_course = campaign + 0;
					iCourse = 0;
				}
				current_level = current_course->level + 0;
				iLevel = 0;
			}

			return -1;
		} else if (inter_modal) {
			int ret = inter_modal->Run();

			w = s->w;
			h = s->h;

			if (!lives) {
				if (ret) {
					delete inter_modal;
					inter_modal = 0;
					return -3;
				}
				return -1;
			}

			if (ret == -1) {
				delete inter_modal;
				inter_modal = 0;

				level_modal = new LEVEL_MODAL(
					&global_screen,
					lives, startlives,
					&score,
					&level_time,
					conf_player.name,
					current_course->name,
					current_level,
					iSubLev, hitbin);
				return -1;
			}

			if (ret == -2) {
				return -2;
			}
		}

		return -1;
	}
};

/**
 * @brief 菜单模态框
 */
struct MENU_MODAL : MODAL {
	MODAL* sub_modal;	/**< 当前子模态框（战役模式） */
	MODAL* hold_modal;	/**< 暂停的游戏模态框 */

	/**
	 * @brief 构造函数
	 */
	MENU_MODAL()
	{
		hold_modal = 0;
		sub_modal = 0;
	}

	/**
	 * @brief 析构函数
	 */
	virtual ~MENU_MODAL()
	{
		if (sub_modal) {
			delete sub_modal;
		}
		if (hold_modal) {
			delete hold_modal;
		}

		hold_modal = 0;
		sub_modal = 0;
		FreeMenu();
	}

	/**
	 * @brief 运行菜单
	 *
	 * @return int 0表示继续
	 */
	virtual int Run()
	{
		if (sub_modal) {
			int ret = sub_modal->Run();

			if (ret == -3) {
				if (hold_modal) {
					delete hold_modal;
					hold_modal = 0;
				}
				if (sub_modal) {
					delete sub_modal;
					sub_modal = 0;
				}

				return 0;
			} else if (ret == -2) {
				if (hold_modal) {
					delete hold_modal;
					hold_modal = 0;
				}

				hold_modal = sub_modal;
				sub_modal = 0;

				// 清除屏幕，避免显示之前的游戏画面
				global_screen.Clear();

				return 0;
			}
		} else {
			// 清除屏幕，避免显示之前的游戏画面
			global_screen.Clear();
			int ret = RunMenu(&global_screen);

			if (ret == -3) {
				if (sub_modal) {
					delete sub_modal;
				}

				sub_modal = 0;

				if (hold_modal) {
					sub_modal = hold_modal;
					hold_modal = 0;

					// 恢复游戏：设置 freeze_fr = -1 让游戏继续运行
					CAMPAIGN_MODAL* cm = (CAMPAIGN_MODAL*)sub_modal;
					if (cm->level_modal) {
						LEVEL_MODAL* lm = (LEVEL_MODAL*)cm->level_modal;
						// 保存 freeze_fr 的值用于调整 start_tm
						int saved_freeze_fr = lm->freeze_fr;
						lm->freeze_fr = -1;
						// 调整 start_tm 保持时间连续性
						unsigned long current_tm = get_time();
						lm->start_tm = current_tm - 10 * saved_freeze_fr;
					}
				} else {
					sub_modal = new CAMPAIGN_MODAL(&global_screen,
						conf_campaign.course,
						conf_campaign.level >= 0 ? conf_campaign.level : 0);
				}
			}
		}

		return 0;
	}
};

MENU_MODAL menu_modal;

/**
 * @brief 清除暂停的游戏
 */
void ClearOnHold()
{
	if (menu_modal.hold_modal) {
		CAMPAIGN_MODAL* cm = (CAMPAIGN_MODAL*)menu_modal.hold_modal;
		delete cm;
		menu_modal.hold_modal = 0;
	}
}

/**
 * @brief 获取暂停游戏状态
 *
 * @param course	返回课程索引
 * @param level		返回关卡索引
 * @param sublevel	返回子关卡索引
 * @param percent	返回完成百分比
 * @param score		返回得分
 * @param lives		返回生命数
 * @return bool		true表示游戏处于暂停状态
 */
bool GameOnHold(int* course, int* level, int* sublevel, int* percent, int* score, int* lives)
{
	if (menu_modal.hold_modal) {
		CAMPAIGN_MODAL* cm = (CAMPAIGN_MODAL*)menu_modal.hold_modal;

		if (course) {
			*course = cm->iCourse;
		}
		if (level) {
			*level	= cm->iLevel;
		}
		if (score) {
			*score	= cm->score;
		}

		if (cm->level_modal) {
			LEVEL_MODAL* lm = (LEVEL_MODAL*)cm->level_modal;
			if (lives) {
				*lives	= lm->lives;
			}

			if (lm->t.check_passed + 1 >= lm->t.check_points) {
				if (sublevel) {
					*sublevel = lm->t.check_passed - 1;
				}
				if (percent) {
					*percent = 100;
				}
			} else {
				if (sublevel) {
					*sublevel = lm->t.check_passed;
				}

				int from = 0;
				from = lm->t.check_point[lm->t.check_passed];
				if (from < 0) {
					from = 0;
				}

				int end = lm->t.check_point[lm->t.check_passed + 1];

				if (percent) {
					*percent  = MIN(100, 100 * (lm->t.scroll - lm->t.w - from) / (end - from));
				}
			}
		} else {
			if (lives) {
				*lives	= cm->lives;
			}

			if (sublevel) {
				*sublevel = 0;
			}

			if (percent) {
				*percent = 0;
			}
		}

		return true;
	}

	return false;
}

/**
 * @brief 开场动画模态框
 */
struct INTRO_MODAL : MODAL {
	int ret;

	SPRITE chassis;
	SPRITE fr_wheel;
	SPRITE bk_wheel;
	SPRITE ascii;
	SPRITE patrol;
	SPRITE bkgnd;
	SPRITE bkcut;

	SPRITE prompt;

	int x;
	int w;
	int h;

	unsigned long bt;
	unsigned long fade_tm;

	int terrain_pos;
	signed char fr_terrain[336];
	signed char bk_terrain[336];

	SCREEN* s;

	/**
	 * @brief 初始化开场动画
	 *
	 * @param scr 屏幕对象
	 */
	void Init(SCREEN* scr)
	{
		s = scr;
		w = s->w;
		h = s->h;

		x = -64;

		terrain_pos = 0;

		for (int i = 0; i < 336; i++) {
			if ((i / 16) % 3 == 0) {
				fr_terrain[i] = (signed char)(-1.7f * sinf(3.131592f * i / 8));
			}
			if ((i / 16) % 3 == 1) {
				bk_terrain[i] = (signed char)(1.7f * sinf(3.131592f * (i + 63) / 8));
			}
		}

		bt = get_time();
		fade_tm = 0;
	}

	/**
	 * @brief 构造函数
	 */
	INTRO_MODAL() :
		chassis(&scene_chassis),
		fr_wheel(&scene_fr_wheel),
		bk_wheel(&scene_bk_wheel),
		ascii(&scene_ascii),
		patrol(&scene_patrol),
		bkgnd(&mountains),
		bkcut(&dunes),
		prompt("PRESS ANY KEY TO CONTINUE")
	{
		ret = 0;

	}

	/**
	 * @brief 运行开场动画
	 *
	 * @return int 0表示完成
	 */
	virtual int Run()
	{
		const static char ascii_str[] = {1, 2, 3, 4, 4};
		const static int ascii_krn[]  = {0, 0, 0, 1, 2};
		const static int ascii_frm[]  = {0, 10, 5, 20, 0};

		const static int bk_wheel_x[2] = {20, 38};
		const static int fr_wheel_x[3] = {3, 11, 26};
		const static int wheel_y = 8;
		const static int suspension[4] = {4, 8, 35, 10};

		while (1) {
			unsigned long ct = get_time();

			int fr = (ct - bt) / 15;
			int wfr = (ct - bt) / 32;

			int ffr = (ct - fade_tm) / 10;
			if (!fade_tm) {
				ffr = 0;
			}

			if (ffr > 100) {
				modal = &menu_modal;
				break;
			}

			int dw = 0;
			int dh = 0;
			get_terminal_wh(&dw, &dh);

			int nw = dw;
			int nh = dh;

			if (nw > 160) {
				nw = 160;
			}
			if (nw < 80) {
				nw = 80;
			}
			if (nh > 50) {
				nh = 50;
			}
			if (nh < 25) {
				nh = 25;
			}

			if (w != nw || h != nh) {
				s->Resize(nw, nh);

				w = nw;
				h = nh;
			}

			s->Clear();

			int dx = terrain_pos - chassis.width;

			int y = 3 + (s->h - chassis.height - fr_wheel.height / 2) / 2;

			int mx = (s->w - bkgnd.width) / 2;

			int mt = terrain_pos * 2 - 60;
			mt = MAX(0, mt);
			mt = MIN(43, mt);

			int sfr = fr;
			int scroll1 = (sfr / 2) % 160;
			int scroll2 = (sfr / 4) % 160;

			int my = (int)floorf(-1.0f + 20.0f * sinf(3.141592f * (mt - 0.5f) / 64.0f));
			int ca = MIN(fr, 32);
			ca = (int)(32 * (0.5f - 0.5f * cosf(3.141592f * ca / 32)));
			int cy = (s->h * (32 - ca) + (y + patrol.height - 10) * ca + 16) / 32;

			int mh = bkgnd.height;
			if (y + patrol.height - my - ffr / 4 + mh > cy - ffr / 2 + bkcut.height) {
				mh = cy - ffr / 2 + bkcut.height - (y + patrol.height - my - ffr / 4);
			}

			if (mh > 0 && mt >= 10) {
				bkgnd.Paint(s, mx - scroll2, y + patrol.height - my - ffr / 4, 0, 0, -1, mh);
				bkgnd.Paint(s, mx - scroll2 + 160, y + patrol.height - my - ffr / 4, 0, 0, -1, mh);
			}

			bkcut.Paint(s, mx - scroll1, cy - ffr / 2, 0, 0, -1, -1);
			bkcut.Paint(s, mx - scroll1 + 160, cy - ffr / 2, 0, 0, -1, -1);

			int left = (s->w - patrol.width) / 2;
			int right = dx + x + 4;
			if (right > left) {
				patrol.Paint(s, left, y + 3 + ffr, 0, 0, right - left, -1, 0, false);
			}

			bk_wheel.SetFrame(wfr + 0);
			bk_wheel.Paint(s, dx + x + bk_wheel_x[0], y + wheel_y + bk_terrain[(terrain_pos + bk_wheel_x[0]) % 256], 0, 0);
			bk_wheel.SetFrame(wfr + 2);
			bk_wheel.Paint(s, dx + x + bk_wheel_x[1], y + wheel_y + bk_terrain[(terrain_pos + bk_wheel_x[1]) % 256], 0, 0);

			for (int sy = y + suspension[1]; sy < y + suspension[3]; sy++) {
				if (sy >= 0 && sy < s->h) {
					for (int sx = dx + x + suspension[0]; sx < dx + x + suspension[2]; sx++) {
						if (sx >= 0 && sx < s->w) {
							s->buf[(s->w + 1) * sy + sx] = ' ';
						}
					}
				}
			}

			chassis.Paint(s, dx + x, y, 0, 0);

			fr_wheel.SetFrame(wfr + 1);
			fr_wheel.Paint(s, dx + x + fr_wheel_x[0], y + wheel_y + fr_terrain[(terrain_pos + fr_wheel_x[0]) % 256], 0, 0);
			fr_wheel.SetFrame(wfr + 3);
			fr_wheel.Paint(s, dx + x + fr_wheel_x[1], y + wheel_y + fr_terrain[(terrain_pos + fr_wheel_x[1]) % 256], 0, 0);
			fr_wheel.SetFrame(wfr + 5);
			fr_wheel.Paint(s, dx + x + fr_wheel_x[2], y + wheel_y + fr_terrain[(terrain_pos + fr_wheel_x[2]) % 256], 0, 0);

			int half = (s->w + chassis.width - x) + 50;
			int afr = (fr - half) / 30;
			afr = MAX(-1, afr);
			afr = MIN(5, afr);

			int ax = (s->w - (5 * 10 - ascii_krn[4])) / 2;

			int i = 0;
			for (; i < afr; i++) {
				if (i == afr - 1 && (fr - half) - afr * 30 < ascii_frm[i]) {
					break;
				}
				ascii.SetFrame(ascii_str[i]);
				ascii.Paint(s, ax + i * 10 - ascii_krn[i], -ffr + y - ascii.height + 1, 0, 0, -1, -1, 0, false);
			}

			if (afr < 0) {
				i = afr;
			}
			if (i < 5 && i >= 0 && (fr & 7) < 4) {
				ascii.SetFrame(0);
				ascii.Paint(s, ax + i * 10 - ascii_krn[i], -ffr + y - ascii.height + 1, 0, 0, -1, -1, 0, false);
			}

			if (fr > half + 150 + 100 && !fade_tm) {
				prompt.Paint(s, (s->w - prompt.width) / 2, y + chassis.height + (s->h - (y + chassis.height) + 2) / 2, 0, 0);
			}

			s->Write(dw, dh, 0, 0, -1, -1);

			terrain_pos = fr;
			if (terrain_pos > s->w + chassis.width - 2 * x) {
				terrain_pos = s->w + chassis.width - 2 * x;
			}

			int _ret = InterScreenInput();

			if (_ret == -2) {
				modal = &menu_modal;
				break;
			}

			if (_ret && !fade_tm && fr > half + 150 + 100) {
				fade_tm = get_time();
			}

			break;
		}

	return 0;
}
};

INTRO_MODAL intro_modal;

Shell *ascii_patrol_shell = NULL;

/**
 * @brief ASCII Patrol 游戏主入口
 *
 * @param argc 参数个数
 * @param argv 参数数组
 * @return int 0表示成功
 */
extern "C" int main_ascii_patrol(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	ascii_patrol_shell = shellGetCurrent();
	if (!ascii_patrol_shell) {
		return -1;
	}

	osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

	InitMenu();
	GamePreAlloc();

	int cols, rows;
	terminal_init(0, 0, &cols, &rows);

	// 隐藏光标，避免干扰游戏画面
	ascii_patrol_shell->write("\033[?25l", 6);

	modal = &intro_modal;

	intro_modal.Init(&global_screen);

	terminal_loop();

	CleanupSpriteGarbage();
	FreeMenu();
	terminal_done();

	// 恢复光标显示
	ascii_patrol_shell->write("\033[?25h", 6);

	osEventFlagsClear(System_StatusHandle, APP_NEED_USART);

	return 0;
}

/**
 * @brief 设置屏幕颜色模式
 *
 * @param s	 屏幕对象
 * @param cl 颜色值
 */
void SetColorMode(SCREEN* s, unsigned char cl)
{
	(void)cl;

	if (s->arr) {
		free_con_output(s);
		s->arr = 0;
	}
}

/**
 * @brief 设置全局颜色模式
 *
 * @param cl 颜色值
 */
void SetColorMode(unsigned char cl)
{
	(void)cl;
}

#endif /* GAME_ENABLE_AP */
