/**
 * @brief 机械臂模块控制
 */

#include "arm_ctrl.h"
#include "shell.h"
#include "string.h"
#include "log.h"
#include "cmsis_os2.h"
#include <stdlib.h>

/* 当前升降角度(度), 内部维护 */
static float arm_lift_angle_current = ARM_LIFT_INIT_ANGLE;

/* 当前翻转角度(度), 内部维护 */
static float arm_flip_angle_current = ARM_FLIP_INIT_ANGLE;

/**
 * @brief 机械臂升降舵机移动到指定角度(带限幅)
 * @param angle 目标角度
 */
void arm_lift_move_to(float angle){
    if (angle < ARM_LIFT_MIN_ANGLE){
        angle = ARM_LIFT_MIN_ANGLE;
    }else if (angle > ARM_LIFT_MAX_ANGLE){
        angle = ARM_LIFT_MAX_ANGLE;
    }
    arm_lift_angle_current = angle;
    Servo_MTURN(ARM_LIFT_SERVO_ID, arm_lift_angle_current, 0, 0);
}

/**
 * @brief 机械臂升降舵机回到初始角度
 */
void arm_lift_move_to_init(void){
    arm_lift_move_to(ARM_LIFT_INIT_ANGLE);
}

/**
 * @brief 机械臂翻转舵机移动到指定角度(带限幅)
 * @param angle 目标角度
 */
void arm_flip_move_to(float angle){
    if (angle < ARM_FLIP_MIN_ANGLE){
        angle = ARM_FLIP_MIN_ANGLE;
    }else if (angle > ARM_FLIP_MAX_ANGLE){
        angle = ARM_FLIP_MAX_ANGLE;
    }
    arm_flip_angle_current = angle;
    Servo_MTURN(ARM_FLIP_SERVO_ID, arm_flip_angle_current, 0, 0);
}

/**
 * @brief 机械臂翻转舵机回到初始角度
 */
void arm_flip_move_to_init(void){
    arm_flip_move_to(ARM_FLIP_INIT_ANGLE);
}

/**
 * @brief 机械臂整体回到初始位
 */
void arm_move_to_init(void){
    arm_lift_move_to_init();
    arm_flip_move_to_init();
}

/**
 * @brief 机械臂模块Shell命令
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
static void arm_Shell(int argc, char *argv[]){
    if (argc != 2 && argc != 3){
        logPrintln("Usage: arm <lift|flip> <angle> | init | help");
        return;
    }

    if (strcmp(argv[1], "help") == 0){
        logPrintln("arm: control the mechanical arm lift and flip");
        logPrintln("  arm lift <angle> | flip <angle> | init | help");
        return;
    }else if (strcmp(argv[1], "init") == 0){
        arm_move_to_init();
        return;
    }else if (argc == 3 && (strcmp(argv[1], "lift") == 0 || strcmp(argv[1], "flip") == 0)){
        float val = (float)atof(argv[2]);
        if (strcmp(argv[1], "lift") == 0){
            arm_lift_move_to(val);
        }else{
            arm_flip_move_to(val);
        }
        return;
    }else{
        logPrintln("arm: invalid arguments");
        return;
    }
}

/**
 * @brief 导出机械臂模块Shell命令
 */
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    arm, arm_Shell, arm commands);
