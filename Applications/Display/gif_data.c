/**
 * @file gif_data.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief GIF数据源文件
 */

#include <string.h>
#include "log.h"
#include "gif_data.h"
#include "gif_data_include.h"

/**
 * @brief 图片数据数组
 */
static const Gif_t Gifs[] = {
#if IMG2C_EEVEE1
    {
        .name = "Eevee1",
        .data = *Eevee1_data,
        .width = EEVEE1_WIDTH,
        .height = EEVEE1_HEIGHT,
        .frame_count = EEVEE1_FRAME_COUNT,
        .frame_size = EEVEE1_FRAME_SIZE,
        .background = 0,
        .fps = EEVEE1_FPS,
    },
#endif
#if IMG2C_EEVEE2
    {
        .name = "Eevee2",
        .data = *Eevee2_data,
        .width = EEVEE2_WIDTH,
        .height = EEVEE2_HEIGHT,
        .frame_count = EEVEE2_FRAME_COUNT,
        .frame_size = EEVEE2_FRAME_SIZE,
        .background = 0,
        .fps = EEVEE2_FPS,
    },
#endif
#if IMG2C_BADAPPLE
	{
        .name = "badapple",
        .data = *badapple_data,
        .width = BADAPPLE_WIDTH,
        .height = BADAPPLE_HEIGHT,
        .frame_count = BADAPPLE_FRAME_COUNT,
        .frame_size = BADAPPLE_FRAME_SIZE,
        .background = 0,
        .fps = BADAPPLE_FPS,
    },
#endif
    {
		.name = "__EMPTY__",
		.data = NULL,
		.width = 0,
		.height = 0,
		.background = 0,
		.fps = 0,
	}
};


/**
 * @brief 查找GIF数据
 * @param name GIF名称
 * @param data GIF结构体指针
 * @return 是否找到
 */
bool find_gif(const char *name, const Gif_t **data) {
	for (uint8_t i = 0; i < sizeof(Gifs) / sizeof(Gif_t); i++) {
		if (strcmp(Gifs[i].name, name) == 0) {
			*data = &Gifs[i];
			return true;
		}
	}
	return false;
}

/**
 * @brief 列出所有GIF名称
 */
void Gif_list(void) {
	uint8_t list = sizeof(Gifs) / sizeof(Gif_t);
	if (list == 1) return;

	for (uint8_t i = 0; i < list - 1; i++) {
		logPrintln("%s", Gifs[i].name);
	}
}
