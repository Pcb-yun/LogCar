#ifndef __ARM_CTRL_H__
#define __ARM_CTRL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_cfg.h"
#include "servo_port.h"

/* 机械臂升降控制接口 */
void arm_lift_move_to(float angle);
void arm_lift_move_to_init(void);

/* 机械臂翻转控制接口 */
void arm_flip_move_to(float angle);
void arm_flip_move_to_init(void);

/* 机械臂整体回到初始位 */
void arm_move_to_init(void);

#ifdef __cplusplus
}
#endif

#endif  /* __ARM_CTRL_H__ */
