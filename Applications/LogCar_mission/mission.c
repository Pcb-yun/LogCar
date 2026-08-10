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

static bool matl_grap();
static bool matl_pop();
static bool trop_grap();
static bool trop_pop();
static bool Home_Sweet_home();

static uint8_t current_point = 0;
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
            logError("Mission Failed at point: %d", current_point);
            return false;
        } else if (state != NAV_STATE_RUNNING) {
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
    uint8_t barcode[16];

    for (;;) {
        osDelay(1);
        if (!mission_running) continue;

        // 出站
        Nav_GoTo_fromName("START");
        if (!wait_tracker()) goto done;

        // 二维码点1（物料顺序）
        Nav_GoTo_fromName("QrCode_1");
        if (!wait_tracker()) goto done;

        // 读取二维码
        if (!Scan_GetLatestBarcode(barcode, sizeof(barcode))) goto done;

        // 设置转盘为物料抓取
        Turntable_Port_SetType(TURNTABLE_MATL);

        // 抓取物料
        osEventFlagsSet(System_StatusHandle, TURNTABLE_RUN);
        if (!matl_grap()) goto done;
        osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);

        // 放置物料
        if (!matl_pop()) goto done;

        // 读取二维码
        if (!Scan_GetLatestBarcode(barcode, sizeof(barcode))) {
            // 二维码点2（奖杯顺序）
            Nav_GoTo_fromName("QrCode_2");
            if (!wait_tracker()) goto done;

            if (!Scan_GetLatestBarcode(barcode, sizeof(barcode))) goto done;
        }

        // 设置转盘为奖杯抓取
        Turntable_Port_SetType(TURNTABLE_TROP);

        // 抓取奖杯
        osEventFlagsSet(System_StatusHandle, TURNTABLE_RUN);
        if (!trop_grap()) goto done;
        osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);

        // 放置奖杯
        if (!trop_pop()) goto done;

        // 回到home点
        if (!Home_Sweet_home()) goto done;

    done:
        Nav_Stop();
        mission_running = false;
    }
}

/**
 * @brief 物料抓取导航
 * @return 抓取状态
 */
static bool matl_grap() {
#if MISSION_MATL_NAV == 0 // 地图定位
    TargetPoint_t *point = Map_GetPointByName("START");
    if (point == NULL) return false;

    for(uint8_t i = 0; i < 5; i++) {
        Nav_GoTo(point->id + i);
        if (!wait_tracker()) {
            return false;
        }
    }
#else // 灰度巡线




#endif
    return true;
}

/**
 * @brief 物料放置导航
 * @return 放置状态
 */
static bool matl_pop() {
    TargetPoint_t *point = Map_GetPointByName("POP_A");
    if (point == NULL) return false;

    for(uint8_t i = 0; i < 5; i++) {
        Nav_GoTo(point->id + i);
        if (!wait_tracker()) {
            return false;
        }
        Turntable_Pop((TurntablePop_t)i);







    }
    return true;
}

/**
 * @brief 物料抓取导航
 * @return 抓取状态
 */
static bool trop_grap() {


    return true;
}

/**
 * @brief 物料放置导航
 * @return 放置状态
 */
static bool trop_pop() {


    return true;
}

/**
 * @brief 回到home点
 * @return 返回状态
 */
static bool Home_Sweet_home() {
    Nav_GoTo_fromName("HOME");
    if (!wait_tracker()) {
        return false;
    }

    return true;
}

/**
 * @brief 设置总任务运行状态
 * @param running 运行状态
 */
void mission_set_running(bool running) {
    mission_running = running;
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
