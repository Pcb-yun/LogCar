/**
 * @file mission.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 总任务函数头文件
 */

#ifndef __MISSION_H__
#define __MISSION_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>

#define MISSION_MATL_NAV 0  // 物料导航方式： 0：地图定位 1：巡线



void mission_set_running(bool running);


#define MISSION_HELP \
    "Usage: mission COMMAND\r\n" \
    "commands:\r\n" \
    "  run      Start mission\r\n" \
    "  stop     Stop mission"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MISSION_H__ */
