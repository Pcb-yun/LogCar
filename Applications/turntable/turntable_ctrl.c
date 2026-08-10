/**
 * @file turntable_ctrl.c
 * @brief 转盘模块控制源文件
 */

#include "turntable_ctrl.h"
#include "shell.h"
#include "string.h"
#include "log.h"
#include "cmsis_os2.h"
#include <stdlib.h>
#include <math.h>

/* 当前转盘角度(度), 内部维护 */
static float turntable_angle_current = 0;

/* 槽位 id 对应的物理角度(度) */
static const float turntable_id[TURNTABLE_ID_MAX] = {
    TURNTABLE_ID_0_ANGLE,
    TURNTABLE_ID_0_ANGLE + TURNTABLE_ANGLE,
    TURNTABLE_ID_0_ANGLE + TURNTABLE_ANGLE * 2,
    TURNTABLE_ID_0_ANGLE + TURNTABLE_ANGLE * 3,
    TURNTABLE_ID_0_ANGLE + TURNTABLE_ANGLE * 4
};

/**
 * @brief 计算最短路径的目标角度
 * @param current 当前角度（可累积超出±180°）
 * @param target 目标角度（0-180范围）
 * @return 最短路径对应的实际目标角度（可能超出±180°）
 */
static float turntable_calc_shortest_target(float current, float target) {
    float diff = fmodf(target - current, 180.0f);
    if (diff > 90.0f) diff -= 180.0f;
    else if (diff < -90.0f) diff += 180.0f;
    return current + diff;
}

/**
 * @brief 转盘模块移动到指定角度（基于当前位置走最短路径）
 * @param angle 角度值
 */
void turntable_move_to(float angle){
    turntable_angle_current = turntable_calc_shortest_target(turntable_angle_current, angle);
    Servo_MTURN(TURNTABLE_SERVO_ID, turntable_angle_current, 0, 0);
}

/**
 * @brief 转盘模块移动到关闭位置（基于当前位置走最短路径）
 */
void turntable_move_to_close(void){
    turntable_move_to(turntable_angle_current - TURNTABLE_CLOSE_OFFSET);
}

/**
 * @brief 转盘模块移动到指定ID
 * @param id ID值
 */
void turntable_move_to_id(uint8_t id){
    if (id >= TURNTABLE_ID_MAX){
        logPrintln("turntable: id %u out of range", id);
        return;
    }
    turntable_move_to(turntable_id[id]);
}

/**
 * @brief 获取与当前角度最近的槽位 id
 * @return 最近的槽位 id (0 ~ TURNTABLE_ID_MAX-1)
 */
uint8_t turntable_get_id(void){
	uint8_t id = 0;
	float min_diff = turntable_calc_shortest_target(turntable_angle_current, turntable_id[0]) - turntable_angle_current;
	if (min_diff < 0) min_diff = -min_diff;
	for (uint8_t i = 1; i < TURNTABLE_ID_MAX; i++){
		float d = turntable_calc_shortest_target(turntable_angle_current, turntable_id[i]) - turntable_angle_current;
		if (d < 0) d = -d;
		if (d < min_diff){
			min_diff = d;
			id = i;
		}
	}
	return id;
}

/**
 * @brief 转盘模块转动到下一个槽位
 * @param direction 旋转方向: 0：顺时针, 非0：逆时针
 */
void turntable_move_to_next(uint8_t direction){
	uint8_t id = turntable_get_id();

	/* 顺时针: 槽位 id 递增(角度增大); 逆时针: 槽位 id 递减 */
	if (direction){
        turntable_move_to_id((id + 1) % TURNTABLE_ID_MAX);
	}else{
		turntable_move_to_id((id + TURNTABLE_ID_MAX - 1) % TURNTABLE_ID_MAX);
	}
}

/**
 * @brief 转盘模块Shell命令
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
static void turntable_Shell(int argc, char *argv[]){
    if (argc != 2 && argc != 3){
        logPrintln("Usage: turntable <id> | init | help");
        return;
    }

    if (strcmp(argv[1], "help") == 0){
        logPrintln("turntable: move the turntable to a specific angle or id");
        logPrintln("  turntable <id> | init | help");
        return;
    }else if (strcmp(argv[1], "id") == 0){
        if (argc != 3){
            logPrintln("Usage: turntable id <N>");
            return;
        }
        int val = atoi(argv[2]);
        if (val < 0 || val >= TURNTABLE_ID_MAX){
            logPrintln("turntable: invalid id");
            return;
        }
        turntable_move_to_id((uint8_t)val);
        return;
    }else if (strcmp(argv[1], "close") == 0){
        turntable_move_to_close();
        return;
    }else{
        logPrintln("turntable: invalid arguments");
        return;
    }
}
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    turntable, turntable_Shell, turntable commands);
