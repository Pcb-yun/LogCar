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
#include "nav_common.h"
#include "chassis.h"

#define MAX_SENSOR_SOURCES 4    // 最大传感器数据来源数量

bool Loc_Set(Pose_t *pose);
bool Loc_Get(Pose_t *pose);



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#endif /* __NAV_LOCAL_H__ */
