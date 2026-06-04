/**
 * @file nav_tools.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航调试工具实现
 */

#include "shell.h"
#include "shell_cmd_group.h"
#include "log.h"
#include "cmsis_os.h"
#include "nav_common.h"
#include "nav_local.h"
#include "step_port.h"
#include "stdlib.h"

/**
 * @brief 显示当前位姿
 */
static void NavTools_Pose(int argc, char *argv[]) {
    Pose_t pose;
    if (Loc_Get(&pose)) {
        logPrintln("Current Pose:");
        logPrintln("  x:     %.2f cm", pose.x);
        logPrintln("  y:     %.2f cm", pose.y);
        logPrintln("  yaw:   %.2f rad (%.2f deg)", pose.yaw, pose.yaw * 57.3f);
        logPrintln("  time:  %lu ms", pose.timestamp);
    } else {
        logWarning("Failed to get pose");
    }
}

/**
 * @brief 设置位姿
 */
static void NavTools_SetPose(int argc, char *argv[]) {
    Pose_t pose = {0};

    if (argc != 4) {
        logPrintln("Usage: nav setpose [x] [y] [yaw]");
        return;
    }

    pose.x = atof(argv[2]);
    pose.y = atof(argv[3]);
    pose.yaw = atof(argv[4]);

    if (Loc_Set(&pose)) {
        logPrintln("Pose set to: x=%.2f cm, y=%.2f cm, yaw=%.2f rad",
               pose.x, pose.y, pose.yaw);
    } else {
        logWarning("Failed to set pose");
    }
}

ShellCommand NavToolsGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, pose, NavTools_Pose, Show Current Pose),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, setpose, NavTools_SetPose, Set Pose),
    SHELL_CMD_GROUP_END()
};

SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
nav, NavToolsGroup, Navigation Tools);
