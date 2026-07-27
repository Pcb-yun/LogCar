/**
 * @file image_data.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 图片数据文件
 */

#include <string.h>
#include "log.h"
#include "image_data.h"
#include "image_data_include.h"

/**
 * @brief 图片数据数组
 */
static const Image_t Images[] = {
#if IMG2C_PICACG
	{
		.name = "PicAcg",
		.data = *picacg_data,
		.width = PICACG_WIDTH,
		.height = PICACG_HEIGHT,
		.background = 0,
	},
#endif
#if IMG2C_TXWZ
	{
		.name = "Txwz",
		.data = *txwz_data,
		.width = TXWZ_WIDTH,
		.height = TXWZ_HEIGHT,
		.background = 0,
	},
#endif
#if IMG2C_LOGO
	{
		.name = "Logo",
		.data = *logo_data,
		.width = LOGO_WIDTH,
		.height = LOGO_HEIGHT,
		.background = 0,
	},
#endif
    {
		.name = "__EMPTY__",
		.data = NULL,
		.width = 0,
		.height = 0,
		.background = 0,
	}
};

/**
 * @brief 查找图片数据
 * @param name 图片名称
 * @param data 图片结构体指针
 * @return 是否找到
 */
bool find_image(const char *name, const Image_t **data) {
	for (uint8_t i = 0; i < sizeof(Images) / sizeof(Image_t); i++) {
		if (strcmp(Images[i].name, name) == 0) {
			*data = &Images[i];
			return true;
		}
	}
	return false;
}

/**
 * @brief 列出所有图片名称
 */
void Image_list(void) {
	uint8_t list = sizeof(Images) / sizeof(Image_t);
	if (list == 1) return;

	for (uint8_t i = 0; i < list - 1; i++) {
		logPrintln("%s", Images[i].name);
	}
}
