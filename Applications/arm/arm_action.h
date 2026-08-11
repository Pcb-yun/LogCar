#ifndef __ARM_ACTION_H__
#define __ARM_ACTION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_cfg.h"
#include "arm_ctrl.h"

typedef enum{
    ARM_ACTION_INIT = 0,     // 初始位
    ARM_ACTION_PULL_DOWN,    // 下拉机械臂
    ARM_ACTION_STAGE_1_PULL_UP, // 冠军上拉机械臂
    ARM_ACTION_STAGE_1_PULL_DOWN, // 冠军下拉机械臂
    ARM_ACTION_STAGE_2_PULL_UP, // 季军上拉机械臂
    ARM_ACTION_STAGE_2_PULL_DOWN, // 季军下拉机械臂
   }arm_action_t;

void arm_action_init(void);
void arm_action(arm_action_t action);
void arm_action_Shell(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif  /* __ARM_ACTION_H__ */