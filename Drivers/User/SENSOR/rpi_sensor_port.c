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
    if (HAL_UART_Receive(&huart2, (uint8_t *)&color, 1, 2000) != HAL_OK) {
        logWarning("receive timeout");
    }

    RPI_Detect_IDE();
    return (SENSOR_Color_t)color[0];
}

/**
 * @brief 树莓派偏移校准
 * @param err_x x轴误差 mm
 * @param err_y y轴误差 mm
 * @return 校准状态
 */
bool RPI_Calibrate(int16_t *err_x, int16_t *err_y) {
    if (err_x == NULL || err_y == NULL) {
        return false;
    }

    uint8_t cmd = 0x0A;
    uint8_t rsp[4] = {0};

    HAL_UART_Transmit(&huart2, &cmd, 1, 200);
    if (HAL_UART_Receive(&huart2, (uint8_t *)&rsp, 4, 2000) != HAL_OK) {
        logWarning("receive timeout");
    }

    *err_x = (rsp[0] == 0) ? (int16_t)rsp[2] : -(int16_t)rsp[2];
    *err_y = (rsp[1] == 0) ? (int16_t)rsp[3] : -(int16_t)rsp[3];

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
        if(RPI_Calibrate(&err_x, &err_y)) {
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
