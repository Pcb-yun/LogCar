/**
 * @brief 机械臂模块控制
 */

#include "arm_ctrl.h"
#include "shell.h"
#include "string.h"
#include "log.h"
#include "cmsis_os2.h"
#include <stdlib.h>

/**
 * @brief 角度限幅(自动按数值大小排序边界, 不依赖传入顺序)
 * @param angle  目标角度
 * @param bound1 边界1
 * @param bound2 边界2
 * @return 限幅后的角度
 */
static float arm_clamp_angle(float angle, float bound1, float bound2){
    float lo = (bound1 < bound2) ? bound1 : bound2;
    float hi = (bound1 < bound2) ? bound2 : bound1;
    if (angle < lo){
        angle = lo;
    }else if (angle > hi){
        angle = hi;
    }
    return angle;
}

/**
 * @brief 机械臂升降舵机移动到指定角度(带限幅)
 * @param angle 目标角度
 */
void arm_lift_move_to(float angle){
    angle = arm_clamp_angle(angle, ARM_LIFT_MIN_ANGLE, ARM_LIFT_MAX_ANGLE);
    Servo_MTURN(ARM_LIFT_SERVO_ID, angle,ARM_ACTION_INTERVAL_MS, 0);
}

/**
 * @brief 机械臂升降舵机移动到指定角度(带限幅)
 * @param angle 目标角度
 * @param velocity 运动速度，单位°/s
 */
void arm_lift_move_by_velocity(float angle, float velocity){
    angle = arm_clamp_angle(angle, ARM_LIFT_MIN_ANGLE, ARM_LIFT_MAX_ANGLE);

    Servo_MTurnByVelocity(ARM_LIFT_SERVO_ID, angle, velocity, ARM_LIFT_ACC, ARM_LIFT_DEC, 0);
}

/**
 * @brief 机械臂升降舵机回到初始角度
 */
void arm_lift_move_to_init(void){
    arm_lift_move_by_velocity(ARM_LIFT_INIT_ANGLE, 200);
}

/**
 * @brief 机械臂翻转舵机移动到指定角度(带限幅)
 * @param angle 目标角度
 */
void arm_flip_move_to(float angle){
    angle = arm_clamp_angle(angle, ARM_FLIP_MIN_ANGLE, ARM_FLIP_MAX_ANGLE);
    Servo_MTURN(ARM_FLIP_SERVO_ID, angle, ARM_ACTION_INTERVAL_MS, 0);
}

/**
 * @brief 机械臂翻转舵机移动到指定角度(带限幅)
 * @param angle 目标角度
 * @param velocity 运动速度，单位°/s
 */
void arm_flip_move_by_velocity(float angle, float velocity){
    angle = arm_clamp_angle(angle, ARM_FLIP_MIN_ANGLE, ARM_FLIP_MAX_ANGLE);

    Servo_MTurnByVelocity(ARM_FLIP_SERVO_ID, angle, velocity, ARM_FLIP_ACC, ARM_FLIP_DEC, 0);
}

/**
 * @brief 机械臂翻转舵机回到初始角度
 */
void arm_flip_move_to_init(void){
    arm_flip_move_by_velocity(ARM_FLIP_INIT_ANGLE, 100);
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
        logPrintln("  arm lift <angle> <velocity> | flip <angle> <velocity> | init | help");
        return;
    }else if (strcmp(argv[1], "init") == 0){
        arm_move_to_init();
        return;
    }else if (argc == 4 && (strcmp(argv[1], "lift") == 0 || strcmp(argv[1], "flip") == 0)){
        float angle = (float)atof(argv[2]);
        float velocity = (float)atof(argv[3]);
        if (strcmp(argv[1], "lift") == 0){
            arm_lift_move_by_velocity(angle, velocity);
        }else{
            arm_flip_move_by_velocity(angle, velocity);
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
