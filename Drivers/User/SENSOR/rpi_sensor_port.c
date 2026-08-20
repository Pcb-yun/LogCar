/**
 * @file rpi_sensor_port.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 树莓派通信接口
 */

#include "rpi_sensor_port.h"
#include "turntable_port.h"
#include "usart.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "log.h"
#include <string.h>
#include "cmsis_os2.h"
#include "mission.h"

/**
 * @brief 初始化树莓派通信
 */
void RPI_Init(void) {
    MX_USART2_UART_Init();
}

/**
 * @brief 树莓派颜色检测
 * @return 颜色枚举值
 */
SENSOR_Color_t RPI_DetectColor(void) {
    uint8_t cmd = 0x0B;
    uint8_t color[1] = {0};

    HAL_UART_Transmit(&huart2, &cmd, 1, 200);
    if (HAL_UART_Receive(&huart2, (uint8_t *)&color, 1, MISSION_RPI_TIMEOUT) != HAL_OK) {
        logWarning("receive timeout");
    }

    RPI_Detect_IDE();
    return (SENSOR_Color_t)color[0];
}

/**
 * @brief 树莓派偏移校准
 * @param err_x x轴误差 mm
 * @param err_y y轴误差 mm
 * @param type 校准类型
 * @return 校准状态
 */
bool RPI_Calibrate(int16_t *err_x, int16_t *err_y, RPI_CalType_t type) {
    if (err_x == NULL || err_y == NULL) {
        return false;
    }

    uint8_t cmd;
    uint8_t rsp[7] = {0};
    uint8_t *p = NULL;

    switch (type) {
        case RPI_CAL_TYPE_MATL:
        case RPI_CAL_TYPE_TROP3: cmd = 0x0A; break;
        case RPI_CAL_TYPE_TROP1: cmd = 0x11; break;
        case RPI_CAL_TYPE_TROP2: cmd = 0x10; break;
        default: logWarning("invalid cal type: %d", type); return false;
    }

    HAL_UART_Transmit(&huart2, &cmd, 1, 200);
    if (HAL_UART_Receive(&huart2, (uint8_t *)&rsp, 7, MISSION_RPI_TIMEOUT) != HAL_OK) {
        logWarning("receive timeout");
        return false;
    }

    // 从后向前定位帧头,确保后接完整数据
    for (uint8_t i = 7 - 5; ; i--) {
        if (rsp[i] == RPI_FRAME_HEAD) {
            p = &rsp[i];
            break;
        }
        if (i == 0) break;
    }
    if (p == NULL) {
        logWarning("frame head not found");
        return false;
    }

    *err_x = (p[1] == 0) ? (int16_t)p[3] : -(int16_t)p[3];
    *err_y = (p[2] == 0) ? (int16_t)p[4] : -(int16_t)p[4];

    RPI_Calibrate_IDE();
    return true;
}

/**
 * @brief 树莓派颜色检测空闲
 */
void RPI_Detect_IDE(void) {
    uint8_t cmd = 0x0C;
    HAL_UART_Transmit(&huart2, &cmd, 1, 200);
}

/**
 * @brief 树莓派偏移校准空闲
 */
void RPI_Calibrate_IDE(void) {
    uint8_t cmd = 0x0E;
    HAL_UART_Transmit(&huart2, &cmd, 1, 200);
}

/**
 * @brief 树莓派测试命令
 */
void RPI_Shell(int argc, char *argv[]) {
    if(argc != 2) {
        logPrintln(RPI_HELP); return;
    }

    if(strcmp(argv[1], "det") == 0) {
        SENSOR_Color_t c = RPI_DetectColor();
        logPrintln("Color: %s", (c <= COLOR_BLUE) ? matl_str[c] : "Invalid");
    } else if(strcmp(argv[1], "cal") == 0) {
        int16_t err_x, err_y;
        if(RPI_Calibrate(&err_x, &err_y, RPI_CAL_TYPE_MATL)) {
            logPrintln("Calibrate: x: %d, y: %d", err_x, err_y);
        } else {
            logPrintln("Calibrate failed");
        }
    } else {
        logPrintln("invalid command: %s\r\n%s", argv[1], RPI_HELP);
    }
}
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    rpi, RPI_Shell, Rpi test);
