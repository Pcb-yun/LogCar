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
#include "step_port.h"

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
            logError("Mission Error at point: %d", current_point);
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

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    for(;;) {
        osDelay(1);
        if (!mission_running) {
            continue;
        }

        while (1) {
            if (!mission_running) break;
            for (uint8_t i = 0; i < Map_GetDataPointCount(); i++) {
                TargetPoint_t *target = Map_GetPoint(i);
                if (!target->enable) continue;
                if (!mission_running) break;

                Nav_GoTo(i);
                if (!wait_tracker()) {
                    break;
                }
            }
        }
    }
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
        OPS_Zero();
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
