/**
 * @file nav_core.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航核心控制器头文件
 */

#ifndef __NAV_CORE_H__
#define __NAV_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdbool.h"
#include "nav_common.h"
#include "nav_map.h"

/**
 * @brief 导航状态枚举
 */
typedef enum {
    NAV_STATE_IDLE = 0,
    NAV_STATE_RUNNING,
    NAV_STATE_STOPPING,
    NAV_STATE_COMPLETE,
    NAV_STATE_ERROR,
} NavState_t;

bool Nav_GoTo(uint8_t target_id);
bool Nav_GoToDirect(TargetPoint_t *target);
bool Nav_GoTo_fromName(const char *name);
void Nav_Stop(void);
NavState_t Nav_GetState(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NAV_CORE_H__ */
