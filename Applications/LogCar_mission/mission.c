/**
 * @file mission.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 总任务函数
 */

#include <stdbool.h>
#include <string.h>
#include "mission.h"
#include "Events.h"
#include "shell.h"
#include "log.h"
#include "nav_core.h"
#include "nav_map.h"
#include "ops.h"
#include "turntable_port.h"
#include "scan_driver.h"
#include "nav_track.h"
#include "arm_action.h"
#include "motion_control.h"
#include "nav_track_cfg.h"
#include "nav_local.h"
#include "rpi_sensor_port.h"
#include "turntable_ctrl.h"
#include <stdlib.h>


static bool matl_grap(void);
static bool matl_pop(void);
static bool trop_grap(void);
static bool trop_pop(void);
static bool Home_Sweet_home(void);
static void pop_to_back(void);
static bool RPI_Cal(TargetPoint_t *point, RPI_CalType_t type);
static bool wait_qr(void);

static bool mission_running = false;

/**
 * @brief 等待导航跟踪完成
 */
static bool wait_tracker(void) {
    while (1) {
        NavState_t state = Nav_GetState();
        if (!mission_running) {
            Nav_Stop();
            return false;
        }
        if (state == NAV_STATE_ERROR) {
            logError("Mission Failed");
            return false;
        } else if (state == NAV_STATE_COMPLETE) {
            return true;
        }
        osDelay(1);
    }
}

/**
 * @brief 总任务执行函数
 */
void mission_run(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);
    uint32_t tick;

    for (;;) {
        osDelay(1);
        if (!mission_running)  {
            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
                arm_action(ARM_ACTION_INIT);
                turntable_move_to_id(0);
            }
            continue;
        }
        osEventFlagsClear(System_StatusHandle, TURNTABLE_CPLT);
        osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);
        tick = osKernelGetTickCount() + MISSION_OPS_TIMEOUT;
        while (1) {
            if (OPS_Is_Ready()) break;
            else if (osKernelGetTickCount() > tick) goto fail;
        }
        tick = osKernelGetTickCount();
        logInfo("Mission Start");
        OPS_Zero();
        turntable_move_to_id(0);

        // // 出发点
        // Nav_GoTo_fromName("HOME");
        // if (!wait_tracker()) goto fail;

        // 出站避让点
        // Nav_GoTo_fromName("OA1");
        // if (!wait_tracker()) goto fail;
        MotionControl_SetPosition(0, -25, 0);
        osDelay(500);

        arm_action(ARM_ACTION_PULL_DOWN);

        // 二维码点1（物料顺序）
        Nav_GoTo_fromName("QrCode_1");
        if (!wait_tracker()) goto fail;

        // 等待二维码识别
        if (!wait_qr()) goto fail;

        // 抓取物料
        if (!matl_grap()) goto fail;

        // 放置物料
        if (!matl_pop()) goto fail;

        // 二维码点2（奖杯顺序）
        MotionControl_SetPosition(0, 0, 90);
        osDelay(600);
        Nav_GoTo_fromName("QrCode_2");
        if (!wait_tracker()) goto fail;

        // 等待二维码识别
        if (!wait_qr()) goto fail;

        // 抓取奖杯
        if (!trop_grap()) goto fail;

        // 放置奖杯
        if (!trop_pop()) goto fail;

        // 回到home点
        if (!Home_Sweet_home()) goto fail;

        mission_running = false;
        logInfo("Mission Complete, cost: %d ms", osKernelGetTickCount() - tick);
        continue;

    fail:
        Nav_Stop();
        mission_running = false;
        logError("Mission Failed, cost: %d ms", osKernelGetTickCount() - tick);
    }
}

/**
 * @brief 物料抓取导航
 * @return 抓取状态
 */
static bool matl_grap(void) {
    Turntable_Port_SetType(TURNTABLE_MATL);
    osEventFlagsSet(System_StatusHandle, TURNTABLE_RUN);

#if MISSION_MATL_NAV // 灰度巡线
    Nav_GoTo_fromName("MATL_TRACK");
    if (!wait_tracker()) goto fail;
    Nav_Track_SetCurve(NAV_TRACK_RIGHT,NAV_TRACK_PATH_RADIUS_CM);
    if (!Nav_Track_Start()) goto fail;
    osEventFlagsWait(System_StatusHandle, TURNTABLE_CPLT, osFlagsWaitAll, osWaitForever);
    Nav_Track_Stop();
#else // 地图定位
    TargetPoint_t *point = Map_GetPointByName("MATL_GRAP1");
    if (point == NULL) goto fail;

    for(uint8_t i = 0; i < 5; i++) {
        Nav_GoTo(point->id + i);
        if (!wait_tracker()) goto fail;
    }
#endif

    osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);
    return true;

fail:
    osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);
    return false;
}

/**
 * @brief 物料放置导航
 * @return 放置状态
 */
static bool matl_pop(void) {
    TargetPoint_t *point = Map_GetPointByName("POP_A");
    if (point == NULL) return false;

    for(uint8_t i = 0; i < 5; i++) {
        Nav_GoTo(point->id + i);
        if (!wait_tracker()) return false;

        if (!Turntable_Pop((TurntablePop_t)i)) {
            logWarning("pop failed");
            return false;
        }

#if MISSION_USE_RPI_CAL // 树莓派校准
        if (!RPI_Cal(Map_GetPoint(point->id + i), RPI_CAL_TYPE_MATL)) return false;
#if MISSION_CAL2OPS // 将校准数据回写到码盘


#endif
#endif

        pop_to_back();
        if (i == 1) osDelay(200);
        if (i == 3) osDelay(100);
    }
    return true;
}

/**
 * @brief 奖杯抓取导航
 * @return 抓取状态
 */
static bool trop_grap(void) {
    Turntable_Port_SetType(TURNTABLE_TROP);
    osEventFlagsSet(System_StatusHandle, TURNTABLE_RUN);

#if MISSION_TROP_NAV // 灰度巡线
    Nav_GoTo_fromName("TROP_TRACK");
    if (!wait_tracker()) goto fail;
    Nav_Track_SetCurve(NAV_TRACK_LIFT,NAV_TRACK_PATH_RADIUS_CM);
    if (!Nav_Track_Start()) goto fail;
    osEventFlagsWait(System_StatusHandle, TURNTABLE_CPLT, osFlagsWaitAll, osWaitForever);
    Nav_Track_Stop();
#else // 地图定位
    TargetPoint_t *point = Map_GetPointByName("TROP_GRAP1");
    if (point == NULL) goto fail;

    for(uint8_t i = 0; i < 3; i++) {
        Nav_GoTo(point->id + i);
        if (!wait_tracker()) goto fail;
    }
#endif
    return true;

fail:
    osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);
    return false;
}

/**
 * @brief 奖杯放置导航
 * @return 放置状态
 */
static bool trop_pop(void) {
    TargetPoint_t *point = Map_GetPointByName("SECOND");
    if (point == NULL) return false;

    arm_action(ARM_ACTION_STAGE_2_PULL_UP);

    for (uint8_t i = 0; i < 3; i++) {
        switch (i) {
            case 1: arm_action(ARM_ACTION_STAGE_1_PULL_UP); break;
            case 2: arm_action(ARM_ACTION_PULL_DOWN); osDelay(MISSION_TROP_DOWN_WAIT); break;
            default: break;
        }

        Nav_GoTo(point->id + i);
        if (!wait_tracker()) return false;

#if MISSION_USE_RPI_CAL // 树莓派校准

        switch (i) {
            case 0: if (!RPI_Cal(Map_GetPoint(point->id + i), RPI_CAL_TYPE_TROP2)) return false; break;
            case 1: if (!RPI_Cal(Map_GetPoint(point->id + i), RPI_CAL_TYPE_TROP1)) return false; break;
            case 2: if (!RPI_Cal(Map_GetPoint(point->id + i), RPI_CAL_TYPE_TROP3)) return false; break;
            default: break;
        }
#if MISSION_CAL2OPS // 将校准数据回写到码盘


#endif
#endif

        switch (i) {
            case 0: arm_action(ARM_ACTION_STAGE_2_PULL_DOWN); osDelay(MISSION_TROP_DOWN_WAIT); break;
            case 1: arm_action(ARM_ACTION_STAGE_1_PULL_DOWN); osDelay(MISSION_TROP_DOWN_WAIT); break;
            default: break;
        }

        if (!Turntable_Pop((TurntablePop_t)(i + 1))) {
            logWarning("pop failed");
            return false;
        }

        osDelay(MISSION_TROP_BACK_WAIT);
        pop_to_back();
        if (i < 2) {
            MotionControl_SetPosition(0.0f, MISSION_TROP_YOFFSET, 0.0f);
            osDelay(MISSION_TROP_NEXT_TIME);
        }
    }
    return true;
}

/**
 * @brief 回到home点
 * @return 返回状态
 */
static bool Home_Sweet_home(void) {
    arm_action(ARM_ACTION_INIT);
    turntable_move_to_id(0);
    // Nav_GoTo_fromName("OA3");
    // if (!wait_tracker()) return false;
    Nav_GoTo_fromName("SWEET_HOME");
    if (!wait_tracker()) return false;
    return true;
}

/**
 * @brief 放置后后退
 */
static void pop_to_back(void) {
    MotionControl_SetMotionParams(500.0f, 250.0f, 550.0f, 550.0f);
    MotionControl_SetPosition(-MISSION_BACK_DIST, 0.0f, 0.0f);
    osDelay(MISSION_BACK_TIME);
}

/**
 * @brief 树莓派校准
 * @param point 校准点
 * @param type 校准类型
 * @return 校准状态
 */
static bool RPI_Cal(TargetPoint_t *point, RPI_CalType_t type) {
    uint32_t start_tick = osKernelGetTickCount();
    int16_t err_x, err_y;
    if (!RPI_Calibrate(&err_x, &err_y, type)) return false;
    TargetPoint_t cal_pose;
    memcpy(&cal_pose, point, sizeof(TargetPoint_t));

    PoseTimestamp_t pose;
    if (!Loc_Get(&pose)) return false;

    // 将放点偏移（车正前方距离 mm → cm）根据当前航向解算到世界坐标系
    float offset_cm = (float)MISSION_POP_OFFSET / 10.0f;
    float yaw_rad = deg2rad(pose.pose.yaw);
    float offset_x = offset_cm * cosf(yaw_rad);
    float offset_y = offset_cm * sinf(yaw_rad);

    if (strcmp(point->name, "FIRST") == 0) {
        err_x += MISSION_TROP1_OFFSET_X;
        err_y += MISSION_TROP1_OFFSET_Y;
    } else if (strcmp(point->name, "SECOND") == 0) {
        err_x += MISSION_TROP2_OFFSET_X;
        err_y += MISSION_TROP2_OFFSET_Y;
    } else if (strcmp(point->name, "THRID") == 0) {
        err_x += MISSION_TROP3_OFFSET_X;
        err_y += MISSION_TROP3_OFFSET_Y;
    }

    // 当前位置 + 放点前向偏移 + 树莓派校准修正 = 实际放置位置
    cal_pose.pose.x = pose.pose.x + offset_x + (float)err_x / 10.0f;
    cal_pose.pose.y = pose.pose.y + offset_y + (float)err_y / 10.0f;
    cal_pose.pose.yaw = pose.pose.yaw;

    logInfo("RPI Cal Time: %d ms\r\n"
            "      err_x: %d err_y: %d perr_x: %f perr_y: %f",
            osKernelGetTickCount() - start_tick, err_x, err_y, cal_pose.pose.x  - pose.pose.x, cal_pose.pose.y - pose.pose.y);

    Nav_GoToDirect(&cal_pose);
    if (!wait_tracker()) return false;
    return true;
}

/**
 * @brief 等待二维码识别
 * @return 二维码识别状态
 */
static bool wait_qr(void) {
    uint8_t barcode[3];
    uint32_t start_tick = osKernelGetTickCount();

    while (1) {
        if (!mission_running) return false;
        if (Scan_GetLatestBarcode(barcode, sizeof(barcode))) {
            int order = atoi((const char *)barcode);
            Turntable_SetOrder((uint8_t)order);
            logInfo("barcode: %d", order);
            return true;
        }
        if (osKernelGetTickCount() - start_tick >= MISSION_QR_TIMEOUT) {
            logWarning("qr timeout");
            return false;
        }
        osDelay(1);
    }
}

/**
 * @brief 设置总任务运行状态
 * @param running 运行状态
 */
void mission_set_running(bool running) {
    mission_running = running;
    osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);
}

/**
 * @brief 总任务执行shell导出
 */
static void mission_shell(int argc, char *argv[]) {
    if(argc != 2) {
        logPrintln(MISSION_HELP); return;
    }

    if (strcmp(argv[1], "run") == 0) {
        logPrintln("Mission Start");
        mission_set_running(true);
    } else if (strcmp(argv[1], "stop") == 0) {
        logPrintln("Mission Stop");
        mission_set_running(false);
    } else {
        logPrintln("invalid command: %s\r\n%s", argv[1], MISSION_HELP);
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
mission, mission_shell, Start car mission);
