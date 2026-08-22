/**
 * @brief 机械臂动作执行
 */
#include "arm_action.h"
#include "log.h"
#include "shell.h"
#include "string.h"

/**
 * @brief 机械臂动作初始化
 */
void arm_action_init(void){
    arm_move_to_init();
}

/**
 * @brief 机械臂动作执行
 * @param action 动作类型
 */
void arm_action(arm_action_t action){
    switch (action){
        case ARM_ACTION_INIT:
            arm_action_init();
            break;
        case ARM_ACTION_PULL_DOWN:
            arm_lift_move_by_velocity(ARM_LIFT_MIN_ANGLE,100);
            arm_flip_move_to(ARM_FLIP_MAX_ANGLE);
            break;
        case ARM_ACTION_STAGE_1_PULL_UP:
            arm_lift_move_by_velocity(ARM_ACTION_STAGE1_DOWN_LIFT - ARM_ACTION_UP_OFFSET,200);
            arm_flip_move_by_velocity(ARM_ACTION_STAGE1_UP_FLIP,200);
            break;
        case ARM_ACTION_STAGE_1_PULL_DOWN:
            arm_lift_move_by_velocity(ARM_ACTION_STAGE1_DOWN_LIFT,200);
            arm_flip_move_by_velocity(ARM_ACTION_STAGE1_DOWN_FLIP,100);
            break;
        case ARM_ACTION_STAGE_2_PULL_UP:
            arm_lift_move_by_velocity(ARM_ACTION_STAGE2_DOWN_LIFT - ARM_ACTION_UP_OFFSET,200);
            arm_flip_move_by_velocity(ARM_ACTION_STAGE2_DOWN_FLIP,200);
            break;
        case ARM_ACTION_STAGE_2_PULL_DOWN:
            arm_lift_move_by_velocity(ARM_ACTION_STAGE2_DOWN_LIFT,200);
            arm_flip_move_by_velocity(ARM_ACTION_STAGE2_DOWN_FLIP,200);
            break;
        default:
            break;
    }
}

/**
 * @brief 机械臂动作Shell命令
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
void arm_action_Shell(int argc, char *argv[]){
    if (argc < 2){
        logPrintln("Usage: arm_action <init|pull_down|stage1_up|stage1_down|stage2_up|stage2_down>");
        return;
    }

    arm_action_t action;
    if (strcmp(argv[1], "init") == 0){
        action = ARM_ACTION_INIT;
    }else if (strcmp(argv[1], "pull_down") == 0){
        action = ARM_ACTION_PULL_DOWN;
    }else if (strcmp(argv[1], "stage1_up") == 0){
        action = ARM_ACTION_STAGE_1_PULL_UP;
    }else if (strcmp(argv[1], "stage1_down") == 0){
        action = ARM_ACTION_STAGE_1_PULL_DOWN;
    }else if (strcmp(argv[1], "stage2_up") == 0){
        action = ARM_ACTION_STAGE_2_PULL_UP;
    }else if (strcmp(argv[1], "stage2_down") == 0){
        action = ARM_ACTION_STAGE_2_PULL_DOWN;
    }else{
        logPrintln("Unknown action: %s", argv[1]);
        logPrintln("Usage: arm_action <init|pull_down|stage1_up|stage1_down|stage2_up|stage2_down>");
        return;
    }

    arm_action(action);
    logPrintln("Action %s executed", argv[1]);
}

SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    arm_action, arm_action_Shell, arm_action commands);
