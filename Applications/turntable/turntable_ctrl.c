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

/**
 * @brief 转盘模块移动到指定角度
 * @param angle 角度值
 */
 
static void turntable_move_to(float angle){
    Servo_MTURN(1, angle, 0.5f, 0.0f);
}

static void turntable_move_to_close(void){
    ServoData servodata = {0};
    Servo_MONITOR(1, &servodata);
    turntable_move_to(servodata.angle - 18.0f);
}

/**
 * @brief 转盘模块移动到指定ID
 * @param id ID值
 */
 
static void turntable_move_to_id(uint8_t id){
    turntable_move_to(turntable_id[id]);
}

/**
 * @brief 转盘模块初始化到初始角度
 */
 
static void turntable_move_to_init(void){
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
