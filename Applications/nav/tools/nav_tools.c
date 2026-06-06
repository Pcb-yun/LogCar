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
#include "nav_map.h"
#include "step_port.h"
#include "stdlib.h"
#include "string.h"

/**
 * @brief 显示当前位姿
 */
static void NavTools_Pose(int argc, char *argv[]) {
    Pose_t pose;
    if (Loc_Get(&pose)) {
        logPrintln("Current Pose:");
        logPrintln("  x:     %.2f cm", pose.x);
        logPrintln("  y:     %.2f cm", pose.y);
        logPrintln("  yaw:   %.2f deg", pose.yaw);
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
        logPrintln("Pose set to: x=%.2f cm, y=%.2f cm, yaw=%.2f deg",
               pose.x, pose.y, pose.yaw);
    } else {
        logWarning("Failed to set pose");
    }
}

/**
 * @brief 获取目标点类型字符串
 * @param type 目标点类型
 * @return 目标点类型字符串
 */
static const char *NavTools_GetTypeName(TargetPointType_t type) {
    switch (type) {
        case TARGET_POINT_NORMAL:  return "Normal";
        case TARGET_POINT_PICKUP: return "Pickup";
        case TARGET_POINT_DELIVERY: return "Delivery";
        case TARGET_POINT_PAUSE:   return "Pause";
        case TARGET_POINT_WAIT:    return "Wait";
        default:                   return "Unknown";
    }
}

/**
 * @brief 打印目标点信息
 * @param point 目标点指针
 */
static void NavTools_PrintPoint(TargetPoint_t *point) {
    logPrintln("ID: %u\r\n"
               "  Name:   %s\r\n"
               "  Type:   %s\r\n"
               "  Pose:   x=%.2f y=%.2f yaw=%.2f\r\n"
               "  Motion: speed=%.2f accel=%.2f\r\n"
               "  Arrive: dist=%.2f yaw=%.2f timeout=%dms\r\n"
               "  Enable: %s",
               point->id,
               point->name,
               NavTools_GetTypeName(point->type),
               point->pose.x, point->pose.y, point->pose.yaw,
               point->motion.target_speed, point->motion.acceleration,
               point->arrive.distance_threshold, point->arrive.yaw_threshold, point->arrive.timeout_ms,
               point->enable ? "true" : "false");
}

/**
 * @brief 查看地图信息
 */
static void NavTools_MapInfo(void) {
    NavMapInfo_t *info = Map_GetInfo();
    if (info == NULL) {
        logWarning("Map not initialized"); return;
    }
    logPrintln("Map Info:\r\n"
               "  Max Points: %u\r\n"
               "  Point Count: %u",
               info->max_points,
               info->point_count);
}

/**
 * @brief 列出所有目标点
 */
static void NavTools_MapList(void) {
    NavMapInfo_t *info = Map_GetInfo();
    if (info == NULL) {
        logWarning("Map not initialized"); return;
    }

    logPrintln("Map Points (%u/%u):", info->point_count, info->max_points);
    for (uint8_t i = 0; i < info->point_count; i++) {
        TargetPoint_t *point = Map_GetPoint(i);
        if (point == NULL) {
            logError("Failed to get point ID:%u", i);
            return;
        }
        NavTools_PrintPoint(point);
    }
}

/**
 * @brief 查看指定ID的目标点
 */
static void NavTools_MapGet(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: map get [id]");
        return;
    }

    uint8_t id = (uint8_t)atoi(argv[1]);
    TargetPoint_t *point = Map_GetPoint(id);

    if (point == NULL) {
        logPrintln("Point ID:%u not found", id);
        return;
    }

    NavTools_PrintPoint(point);
}

/**
 * @brief 删除指定ID的目标点
 */
static void NavTools_MapRm(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: map rm [id]");
        return;
    }

    uint8_t id = (uint8_t)atoi(argv[1]);

    if (Map_RemovePoint(id)) {
        logPrintln("Point ID:%u removed", id);
    } else {
        logWarning("Failed to remove point ID:%u", id);
    }
}

/**
 * @brief 读取一行输入
 * @param buffer 输入缓冲区指针
 * @param maxLen 输入缓冲区最大长度
 */
static void shellReadLine(char *buffer, int maxLen) {
    Shell *shell = shellGetCurrent();
    int index = 0;
    char ch;
    while (true) {
        if (shell->read(&ch, 1) > 0) {
            // 处理回车
            if (ch == '\r' || ch == '\n') {
                break;
            }
            // 处理退格
            if (ch == '\b' || ch == 0x7F) {
                if (index > 0) {
                    index--;
                    shell->write("\b \b", 3);
                }
                continue;
            }
            // 处理 Ctrl+C
            if (ch == 0x03) {
                buffer[0] = ch;
                buffer[1] = '\0';
                return;
            }
            // 添加字符
            if (index < maxLen - 1) {
                buffer[index++] = ch;
                shell->write(&ch, 1);
            }
        } else {
            osDelay(10);
        }
    }

    buffer[index] = '\0';
    shell->write("\r\n", 2);
}

/**
 * @brief 添加目标点
 */
static void NavTools_MapAdd(void) {
    Shell *shell = shellGetCurrent();
    TargetPoint_t point = {0};
    char buffer[64];

    logPrintln("=== Add Target Point (press Ctrl+C to cancel) ===");

    // 1. ID
    while (true) {
        shellPrint(shell, "  id: ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        // 解析 ID
        int id = atoi(buffer);
        if (id < 0 || id > 255) {
            logPrintln("Invalid id (0-255), try again");
            continue;
        }
        point.id = (uint8_t)id;
        break;
    }

    // 2. Name
    while (true) {
        shellPrint(shell, "  name: ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        if (strlen(buffer) > 15) {
            logPrintln("Name too long (max 15 chars), try again");
            continue;
        }
        strncpy(point.name, buffer, 15);
        point.name[15] = '\0';
        break;
    }

    // 3. Type
    while (true) {
        shellPrint(shell, "  type (0:Normal, 1:Pickup, 2:Delivery, 3:Pause, 4:Wait): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        int type = atoi(buffer);
        if (type < 0 || type > 4) {
            logPrintln("Invalid type (0-4), try again");
            continue;
        }
        point.type = (TargetPointType_t)type;
        break;
    }

    // 4. X
    while (true) {
        shellPrint(shell, "  x (cm): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float x = atof(buffer);
        point.pose.x = x;
        break;
    }

    // 5. Y
    while (true) {
        shellPrint(shell, "  y (cm): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float y = atof(buffer);
        point.pose.y = y;
        break;
    }

    // 6. Yaw
    while (true) {
        shellPrint(shell, "  yaw (deg): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float yaw = atof(buffer);
        point.pose.yaw = yaw;
        break;
    }

    // 7. Speed
    while (true) {
        shellPrint(shell, "  target speed (cm/s): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float speed = atof(buffer);
        point.motion.target_speed = speed;
        break;
    }

    // 8. Acceleration
    while (true) {
        shellPrint(shell, "  acceleration (cm/s²): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float accel = atof(buffer);
        point.motion.acceleration = accel;
        break;
    }

    // 9. Deceleration
    while (true) {
        shellPrint(shell, "  deceleration (cm/s²): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float decel = atof(buffer);
        point.motion.deceleration = decel;
        break;
    }

    // 10. Target Angular Speed
    while (true) {
        shellPrint(shell, "  target angular speed (rad/s): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float angSpeed = atof(buffer);
        point.motion.target_angular_speed = angSpeed;
        break;
    }

    // 11. Angular Acceleration
    while (true) {
        shellPrint(shell, "  angular acceleration (rad/s²): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float angAccel = atof(buffer);
        point.motion.angular_acceleration = angAccel;
        break;
    }

    // 12. Arrive Check Mode
    while (true) {
        shellPrint(shell, "  arrive check mode (0:Distance, 1:Yaw, 2:Both): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        int mode = atoi(buffer);
        if (mode < 0 || mode > 2) {
            logPrintln("Invalid mode (0-2), try again");
            continue;
        }
        point.arrive.check_mode = (ArriveCheckMode_t)mode;
        break;
    }

    // 13. Distance Threshold
    while (true) {
        shellPrint(shell, "  distance threshold (cm): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float dist = atof(buffer);
        point.arrive.distance_threshold = dist;
        break;
    }

    // 14. Yaw Threshold
    while (true) {
        shellPrint(shell, "  yaw threshold (deg): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float yawTh = atof(buffer);
        point.arrive.yaw_threshold = yawTh;
        break;
    }

    // 15. Timeout
    while (true) {
        shellPrint(shell, "  timeout (ms): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        int timeout = atoi(buffer);
        point.arrive.timeout_ms = (uint16_t)timeout;
        break;
    }

    // 16. Enable
    while (true) {
        shellPrint(shell, "  enable (0:false, 1:true, default=1): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        int enable = atoi(buffer);
        if (enable != 0 && enable != 1) {
            logPrintln("Invalid value (0 or 1), try again");
            continue;
        }
        point.enable = (enable == 1);
        break;
    }

    // 确认
    logPrintln("\n=== Confirm ===");
    NavTools_PrintPoint(&point);

    // 确认循环
    while (true) {
        shellPrint(shell, "\nAdd? (y/n): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        if (buffer[0] == 'y' || buffer[0] == 'Y') {
            if (Map_AddPoint(&point)) {
                logPrintln("\nPoint ID:%u added successfully", point.id);
            } else {
                logPrintln("\nFailed to add point ID:%u", point.id);
            }
            return;
        }

        if (buffer[0] == 'n' || buffer[0] == 'N') {
            return;
        }
    }
}

ShellCommand NavToolsGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, pose, NavTools_Pose, Show Current Pose),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, setpose, NavTools_SetPose, Set Pose),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
nav, NavToolsGroup, Navigation Tools);

ShellCommand MapToolsGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, info, NavTools_MapInfo, Map Info),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, list, NavTools_MapList, List Map Points),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, get, NavTools_MapGet, Get Point by ID),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, rm, NavTools_MapRm, Remove Point),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, add, NavTools_MapAdd, Add Point),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
map, MapToolsGroup, Map Tools);
