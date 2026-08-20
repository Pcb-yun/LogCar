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

#define MISSION_MATL_NAV 1      // 物料导航方式： 0：地图定位 1：巡线
#define MISSION_TROP_NAV 0      // 奖杯导航方式： 0：地图定位 1：巡线
#define MISSION_COLOR_SENSOR 0  // 颜色识别方式： 0：颜色传感器 1：树莓派通讯

#define MISSION_USE_RPI_CAL 1   // 是否使用树莓派校准
#define RPI_FRAME_HEAD 0xFE // 帧头
#define MISSION_CAL2OPS 0       // 是否将校准数据回写码盘
#define MISSION_RPI_TIMEOUT 2000 // 树莓派响应超时 ms

#define MISSION_BACK_DIST 25.0f // 放置后回退的距离 cm
#define MISSION_BACK_TIME 400    // 放置后回退等待时间 ms

#define MISSION_POP_OFFSET 98.01f   // 转盘放点偏移校准 mm
#define MISSION_QR_TIMEOUT 2000 // 二维码识别超时时间 ms
#define MISSION_TROP_DOWN_WAIT 150 // 奖杯下降等待时间 ms
#define MISSION_TROP_BACK_WAIT 350 // 奖杯放置转盘等待时间
#define MISSION_TROP_YOFFSET 28.0f // 高台y方向提前偏移
#define MISSION_TROP_NEXT_TIME 450 // 奖杯下一个点等待时间

#define MISSION_TROP1_OFFSET_X 0 // 奖杯1x方向偏移校准 mm
#define MISSION_TROP1_OFFSET_Y 0 // 奖杯1y方向偏移校准 mm

#define MISSION_TROP2_OFFSET_X 0 // 奖杯2x方向偏移校准 mm
#define MISSION_TROP2_OFFSET_Y 0 // 奖杯2y方向偏移校准 mm

#define MISSION_TROP3_OFFSET_X 0 // 奖杯3x方向偏移校准 mm
#define MISSION_TROP3_OFFSET_Y 0 // 奖杯3y方向偏移校准 mm


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
