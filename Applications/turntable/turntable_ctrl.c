/**
 * @brief 转盘模块控制
 */

#include "turntable_ctrl.h"
#include "shell.h"
#include "stdio.h"
#include "string.h"
#include "log.h"
#include "cmsis_os2.h"
#include <stdlib.h>

float turntable_angle_current = TURNABLE_INIT;

float turntable_id[TURNABLE_ID_MAX] = {TURNABLE_ID_0_ANGLE,
     TURNABLE_ID_0_ANGLE + TURNABLE_ANGLE*2,
     TURNABLE_ID_0_ANGLE + TURNABLE_ANGLE*4,
     TURNABLE_ID_0_ANGLE + TURNABLE_ANGLE*6,
     TURNABLE_ID_0_ANGLE + TURNABLE_ANGLE*8};

/**
 * @brief 计算最短路径的目标角度（基于当前位置，在±180°内选择最近路径）
 * @param current 当前角度
 * @param target 目标角度（0-360范围）
 * @return 最短路径对应的实际目标角度
 */
static float turntable_calc_shortest_target(float current, float target) {
    float diff = target - current;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return current + diff;
}

/**
 * @brief 转盘模块移动到指定角度（基于当前位置走最短路径）
 * @param angle 角度值
 */
 
void turntable_move_to(float angle){
    turntable_angle_current = turntable_calc_shortest_target(turntable_angle_current, angle);
    Servo_MTURN(1, turntable_angle_current, 0, 0);
}

/**
 * @brief 转盘模块移动到关闭位置（基于当前位置走最短路径）
 */
void turntable_move_to_close(void){
    turntable_move_to(turntable_angle_current - TURNABLE_CLOSE_OFFSET);

}

/**
 * @brief 转盘模块移动到指定ID
 * @param id ID值
 */
 
void turntable_move_to_id(uint8_t id){
    turntable_move_to(turntable_id[id]);
}

/**
 * @brief 转盘模块初始化到初始角度
 */
 
void turntable_move_to_init(void){
    turntable_move_to(TURNABLE_INIT);
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
    }else if (strcmp(argv[1], "init") == 0){
        turntable_move_to_init();
        return;
    }else if (strcmp(argv[1], "id") == 0){
        if(atoi(argv[2]) >= TURNABLE_ID_MAX){
            logPrintln("turntable: invalid id");
            return;
        }
        turntable_move_to_id(atoi(argv[2]));
        return;
    }else if (strcmp(argv[1], "close") == 0){
        turntable_move_to_close();
        return;
    }else{
        logPrintln("turntable: invalid arguments");
        return;
    }
}

/**
 * @brief 导出转盘模块Shell命令
 */
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    turntable, turntable_Shell, turntable commands);
