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
#include "nav_core.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "Events.h"

static void shellReadLine(char *buffer, int maxLen);
static void shellReadLineWithPrompt(char *buffer, int maxLen, const char *default_val, const char *field,const char *name);
static void NavTools_Pose_View(void);

/**
 * @brief 获取导航状态字符串
 */
static const char *NavTools_GetStateName(NavState_t state) {
    switch (state) {
        case NAV_STATE_IDLE:     return "Idle";
        case NAV_STATE_RUNNING:  return "Running";
        case NAV_STATE_COMPLETE: return "Complete";
        case NAV_STATE_ERROR:    return "Error";
        default:                 return "Unknown";
    }
}

/**
 * @brief 导航到指定目标点
 */
static void NavTools_GoTo(int argc, char *argv[]) {
    if (argc != 3) {
        logPrintln("Usage: nav goto [id] [view]");
        return;
    }

    uint8_t id = (uint8_t)atoi(argv[1]);
    bool view = strcmp(argv[2], "1") == 0;
    TargetPoint_t *point = Map_GetPoint(id);

    if (point == NULL) {
        logPrintln("Point ID:%u not found", id);
        return;
    }

    if (!point->enable) {
        logPrintln("Point ID:%u is disabled", id);
        return;
    }

    if (Nav_GoTo(id)) {
        if (strcmp(argv[2], "1") == 0) {
            NavTools_Pose_View();
        }
    } else {
        logWarning("Failed to start navigation");
    }
}

/**
 * @brief 导航到指定坐标
 */
static void NavTools_GotoDirect(int argc, char *argv[]) {
    if (argc != 4) {
        logPrintln("Usage: goto [x] [y] [yaw]"); return;
    }

    float x = atof(argv[1]);
    float y = atof(argv[2]);
    float yaw = atof(argv[3]);

    static TargetPoint_t debug_target = {0};
    debug_target.id = 0xFF;
    strncpy(debug_target.name, "Manual Point", 15);
    debug_target.name[15] = '\0';
    debug_target.type = TARGET_POINT_NORMAL;
    debug_target.pose.x = x;
    debug_target.pose.y = y;
    debug_target.pose.yaw = yaw;

    debug_target.motion.target_speed = 60.0f;
    debug_target.motion.target_angular_speed = 100.0f;
    debug_target.motion.acceleration = 150.0f;
    debug_target.motion.deceleration = 350.0f;
    debug_target.motion.angular_acceleration = 350.0f;

    debug_target.arrive.check_mode = ARRIVE_CHECK_BOTH;
    debug_target.arrive.distance_threshold = 2.0f;
    debug_target.arrive.yaw_threshold = 3.0f;
    debug_target.arrive.timeout_ms = 5000;

    debug_target.enable = true;

    if (!Nav_GoToDirect(&debug_target)) {
        logWarning("Failed to start navigation");
    }
}

/**
 * @brief 停止导航
 */
static void NavTools_Stop(void) {
    Nav_Stop();
    logPrintln("Navigation stopped");
}

/**
 * @brief 查询导航状态
 */
static void NavTools_State(void) {
    NavState_t state = Nav_GetState();
    logPrintln("Navigation State: %s", NavTools_GetStateName(state));
}

/**
 * @brief 实时显示当前位姿
 */
static void NavTools_Pose_View(void) {
    PoseTimestamp_t pose;
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    uint8_t byte;

    logPrintln("\033[?25l\r"
        "Current Pose:\r\n"
        "  x:    0.00 cm\r\n"
        "  y:    0.00 cm\r\n"
        "  yaw:  0.00 deg\r\n"
        "  tick: 0");

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    while (1) {
        if (!Loc_Get(&pose)) {
            logWarning("Failed to get pose");
            break;
        }

        logPrintln("\033[5A\033[2K\rCurrent Pose:\r\n"
                   "\033[2K\r  x:    %.2f cm\r\n"
                   "\033[2K\r  y:    %.2f cm\r\n"
                   "\033[2K\r  yaw:  %.2f deg\r\n"
                   "\033[2K\r  tick: %lu",
            pose.pose.x, pose.pose.y, pose.pose.yaw, pose.timestamp);

        osMessageQueueGet(Usart1_Rx_DataHandle, &byte, NULL, 10);
        if (byte == 0x03) break;
    }
    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    logPrintln("\033[5A\033[J\033[2A\033[?25h");
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
               "  Motion: target_speed=%.2f angular_speed=%.2f\r\n"
               "          accel=%.2f decel=%.2f angular_accel=%.2f\r\n"
               "  Arrive: mode=%d dist=%.2f yaw=%.2f timeout=%dms\r\n"
               "  Enable: %s",
               point->id,
               point->name,
               NavTools_GetTypeName(point->type),
               point->pose.x, point->pose.y, point->pose.yaw,
               point->motion.target_speed, point->motion.target_angular_speed,
               point->motion.acceleration, point->motion.deceleration,
               point->motion.angular_acceleration,
               point->arrive.check_mode,
               point->arrive.distance_threshold, point->arrive.yaw_threshold, point->arrive.timeout_ms,
               point->enable ? "true" : "false");
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
            logWarning("Failed to get point ID:%u", i);
            continue;
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
 * @brief 修改目标点
 */
static void NavTools_MapModify(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: map set [id]");
        return;
    }

    uint8_t id = (uint8_t)atoi(argv[1]);
    TargetPoint_t *point = Map_GetPoint(id);

    if (point == NULL) {
        logPrintln("Point ID:%u not found", id);
        return;
    }

    Shell *shell = shellGetCurrent();
    TargetPoint_t new_point;
    memcpy(&new_point, point, sizeof(TargetPoint_t));
    char buffer[64];
    char default_str[64];

    logPrintln("=== Edit Point ID:%u (press Ctrl+C to cancel, Enter to skip) ===", id);
    NavTools_PrintPoint(point);

    // Name
    shellReadLineWithPrompt(buffer, sizeof(buffer), point->name, "Point name", "Name");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0 && strlen(buffer) <= 15) {
        strncpy(new_point.name, buffer, 15);
        new_point.name[15] = '\0';
    }

    // Type
    snprintf(default_str, sizeof(default_str), "%d", point->type);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "Point type (0:Normal, 1:Pickup, 2:Delivery, 3:Pause, 4:Wait)", "Type");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        int type = atoi(buffer);
        if (type >= 0 && type <= 4) {
            new_point.type = (TargetPointType_t)type;
        }
    }

    // X
    snprintf(default_str, sizeof(default_str), "%.2f", point->pose.x);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "Pose x (cm)", "x");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.pose.x = atof(buffer);
    }

    // Y
    snprintf(default_str, sizeof(default_str), "%.2f", point->pose.y);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "Pose y (cm)", "y");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.pose.y = atof(buffer);
    }

    // Yaw
    snprintf(default_str, sizeof(default_str), "%.2f", point->pose.yaw);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "Pose yaw (deg)", "yaw");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.pose.yaw = atof(buffer);
    }

    // Target Speed
    snprintf(default_str, sizeof(default_str), "%.2f", point->motion.target_speed);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "target speed (cm/s)", "target_speed");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.motion.target_speed = atof(buffer);
    }

    // Angular Speed
    snprintf(default_str, sizeof(default_str), "%.2f", point->motion.target_angular_speed);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "target angular speed (deg/s)", "angular_speed");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.motion.target_angular_speed = atof(buffer);
    }

    // Acceleration
    snprintf(default_str, sizeof(default_str), "%.2f", point->motion.acceleration);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "acceleration (cm/s²)", "accel");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.motion.acceleration = atof(buffer);
    }

    // Deceleration
    snprintf(default_str, sizeof(default_str), "%.2f", point->motion.deceleration);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "deceleration (cm/s²)", "decel");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.motion.deceleration = atof(buffer);
    }

    // Angular Acceleration
    snprintf(default_str, sizeof(default_str), "%.2f", point->motion.angular_acceleration);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "angular acceleration (deg/s²)", "angular_accel");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.motion.angular_acceleration = atof(buffer);
    }

    // Check Mode
    snprintf(default_str, sizeof(default_str), "%d", point->arrive.check_mode);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "arrive check mode (0:Distance, 1:Yaw, 2:Both)", "mode");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        int mode = atoi(buffer);
        if (mode >= 0 && mode <= 2) {
            new_point.arrive.check_mode = (ArriveCheckMode_t)mode;
        }
    }

    // Distance Threshold
    snprintf(default_str, sizeof(default_str), "%.2f", point->arrive.distance_threshold);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "distance threshold (cm)", "dist");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.arrive.distance_threshold = atof(buffer);
    }

    // Yaw Threshold
    snprintf(default_str, sizeof(default_str), "%.2f", point->arrive.yaw_threshold);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "yaw threshold (deg)", "yaw");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.arrive.yaw_threshold = atof(buffer);
    }

    // Timeout
    snprintf(default_str, sizeof(default_str), "%d", point->arrive.timeout_ms);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "timeout (ms)", "timeout");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        new_point.arrive.timeout_ms = (uint16_t)atoi(buffer);
    }

    // Enable
    snprintf(default_str, sizeof(default_str), "%d", point->enable ? 1 : 0);
    shellReadLineWithPrompt(buffer, sizeof(buffer), default_str, "enable (0:false, 1:true)", "Enable");
    if (buffer[0] == 0x03) return;
    if (strlen(buffer) > 0) {
        int enable = atoi(buffer);
        if (enable == 0 || enable == 1) {
            new_point.enable = (enable == 1);
        }
    }

    // 确认
    logPrintln("\n=== New Values ===");
    NavTools_PrintPoint(&new_point);

    while (true) {
        shellPrint(shell, "\nSave? (y/n): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        if (buffer[0] == 'y' || buffer[0] == 'Y') {
            if (Map_UpdatePoint(id, &new_point)) {
                logPrintln("\nPoint ID:%u updated", id);
            } else {
                logPrintln("\nFailed to update point ID:%u", id);
            }
            return;
        }

        if (buffer[0] == 'n' || buffer[0] == 'N') {
            return;
        }
    }
}

/**
 * @brief 读取一行输入（覆盖模式）
 * @param buffer 输入缓冲区指针
 * @param maxLen 输入缓冲区最大长度
 * @param default_val 默认值字符串
 * @param name 字段名
 */
static void shellReadLineWithPrompt(char *buffer, int maxLen, const char *default_val, const char *field,const char *name) {
    Shell *shell = shellGetCurrent();
    int index = 0;
    char ch;
    char prompt_line[128];

    // 第一行：字段名
    shellPrint(shell, "%s", field);

    // 第二行：提示符
    snprintf(prompt_line, sizeof(prompt_line), "%s [%s]: ", name, default_val);
    shellPrint(shell, "\r\n%s", prompt_line);

    // 输入循环
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

    shellPrint(shell, "\033[1A\r\033[J");
    // // 覆盖模式：将结果显示在第一行
    // if (strlen(buffer) > 0) {
    //     // 构建结果行
    //     snprintf(label_line, sizeof(label_line), "%s: %s", name, buffer);
    //     // 清屏两行
    //     shell->write("\033[2K\033[1A\033[2K\r", 10);
    //     // 显示结果
    //     shellPrint(shell, "%s", label_line);
    // } else {
    //     // 使用默认值
    //     snprintf(label_line, sizeof(label_line), "%s: %s", name, default_val);
    //     shell->write("\033[2K\033[1A\033[2K\r", 10);
    //     shellPrint(shell, "%s", label_line);
    // }
    // shell->write("\r\n", 2);
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
        shellPrint(shell, "  target angular speed (deg/s): ");
        shellReadLine(buffer, sizeof(buffer));

        if (strlen(buffer) == 0) continue;
        if (buffer[0] == 0x03) return;

        float angSpeed = atof(buffer);
        point.motion.target_angular_speed = angSpeed;
        break;
    }

    // 11. Angular Acceleration
    while (true) {
        shellPrint(shell, "  angular acceleration (deg/s²): ");
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
        shellPrint(shell, "  enable (0:false, 1:true): ");
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
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, pose, NavTools_Pose_View, View Current Pose),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, go, NavTools_GoTo, Navigate to Point),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, to, NavTools_GotoDirect, Navigate to specified point),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, stop, NavTools_Stop, Stop Navigation),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, state, NavTools_State, Show Navigation State),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
nav, NavToolsGroup, Navigation Tools);

ShellCommand MapToolsGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, list, NavTools_MapList, List Map Points),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, get, NavTools_MapGet, Get Point by ID),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, set, NavTools_MapModify, Modify Point),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, rm, NavTools_MapRm, Remove Point),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, add, NavTools_MapAdd, Add Point),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
map, MapToolsGroup, Map Tools);
