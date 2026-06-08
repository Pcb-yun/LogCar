/**
 * @file Art_Data.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief ASCII Art图片数据定义
 */

#include <string.h>
#include <stdint.h>
#include "Art_show.h"
#include "Art_Data.h"

/**
 * @brief ASCII Art图片数据数组
 */
static const Artdata_t Art_cfg[] = {
    {
        .name = "NINA",
        .data = NINA_DATA,
    },
	{
		.name = "HARUHI",
		.data = HARUHI_DATA,
	},
	{
		.name = "IZUMI",
		.data = IZUMI_DATA,
	}
};

/**
 * @brief 查找ASCII Art图片数据
 * @param name 图片名称
 * @param data 图片数据
 * @return 是否找到
 */
bool find_art(const char *name, const char **data) {
	// 遍历数组查找匹配的名称
	for (uint8_t i = 0; i < sizeof(Art_cfg) / sizeof(Art_cfg[0]); i++) {
		if (strcmp(Art_cfg[i].name, name) == 0) {
			*data = Art_cfg[i].data;
			return true;
		}
	}

	return false;
}

/**
 * @brief 列出所有ASCII Art图片名称
 */
void Art_list(void) {
	Shell *shell = shellGetCurrent();
	for (uint8_t i = 0; i < sizeof(Art_cfg) / sizeof(Art_cfg[0]); i++) {
		ART_OUT(shell, Art_cfg[i].name, strlen(Art_cfg[i].name));
		ART_OUT(shell, "\r\n", 2);
	}
}
