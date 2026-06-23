/**
 * @file nav_local.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航定位头文件
 */

#ifndef __NAV_LOCAL_H__
#define __NAV_LOCAL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdbool.h"
#include "nav_math.h"

/**
 * @brief 位姿时间戳结构体
 */
typedef struct {
    Pose2D_t pose;       // 位姿数据
    uint32_t timestamp;  // 时间戳
} PoseTimestamp_t;

bool Loc_Get(PoseTimestamp_t *pose);



#ifdef __cplusplus
extern "C" }
#endif /* __cplusplus */

#endif /* __NAV_LOCAL_H__ */
