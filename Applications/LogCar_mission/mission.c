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
#include "nav_tracker.h"
#include "nav_map.h"
#include "ops.h"
#include "step_port.h"

static uint8_t current_point = 0;

/**
 * @brief 等待导航跟踪完成
 */
static void wait_tracker(void) {
    while (1) {
        TrackState_t state = Nav_Track_GetState();
        if (state == TRACK_STATE_ERROR) {
            logError("Mission Error at point: %d", current_point);
            return;
        } else if (state != TRACK_STATE_RUNNING) {
            return;
        }
        osDelay(1);
    }
}

/**
 * @brief 总任务执行函数
 */
void mission_run(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, MISSION_RUN, osFlagsWaitAny, osWaitForever);
    OPS_Zero();
    Motor_zero(0);
    logInfo("Mission Start");

    for(;;) {
        osDelay(10);

        Nav_Track_GoTo(current_point++);
        wait_tracker();

        Nav_Track_GoTo(current_point++);
        wait_tracker();

        Nav_Track_GoTo(current_point++);
        wait_tracker();

        Nav_Track_GoTo(current_point++);
        wait_tracker();

        current_point = 0;
    }
}

/**
 * @brief 总任务执行shell导出
 */
static void mission_shell(int argc, char *argv[]) {
    if(argc != 2) {
        logPrintln(MISSION_HELP); return;
    }

    if (strcmp(argv[1], "run") == 0) {
        osEventFlagsSet(System_StatusHandle, MISSION_RUN);
    } else if (strcmp(argv[1], "stop") == 0) {
        osEventFlagsClear(System_StatusHandle, MISSION_RUN);
    } else {
        logPrintln("invalid command: %s\r\n%s", argv[1], MISSION_HELP);
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
mission, mission_shell, Start car mission);
