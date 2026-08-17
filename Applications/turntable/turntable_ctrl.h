/**
 * @file turntable_ctrl.h
 * @brief 转盘运动控制头文件
 */

#ifndef __TURNTABLE_CTRL_H__
#define __TURNTABLE_CTRL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "turntable_cfg.h"
#include "servo_port.h"


void turntable_move_to(float angle);
void turntable_move_to_id(uint8_t id);
uint8_t turntable_get_id(void);
void turntable_move_to_next(uint8_t direction);
void turntable_move_to_close(void);
void turntable_move_to_init(void);

void turntable_move_to_int(float angle, int interval);
void turntable_move_to_id_int(uint8_t id, int interval);

#ifdef __cplusplus
}
#endif

#endif  /* __TURNTABLE_CTRL_H__ */
