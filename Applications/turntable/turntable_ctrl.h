#ifndef __TURNTABLE_CTRL_H__
#define __TURNTABLE_CTRL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "turntable_cfg.h"
#include "servo_port.h"


/* 转盘运动控制接口 */
void turntable_move_to(float angle);
void turntable_move_to_id(uint8_t id);
void turntable_move_to_close(void);
void turntable_move_to_init(void);

#ifdef __cplusplus
}
#endif

#endif  /* __TURNTABLE_CTRL_H__ */
