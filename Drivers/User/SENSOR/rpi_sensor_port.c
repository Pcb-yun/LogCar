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
    uint8_t color[1];

    HAL_UART_Transmit_IT(&huart2, &cmd, 1);
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&color, 1);

    RPI_Detect_IDE();
    return (SENSOR_Color_t)color[0];
}

/**
 * @brief 树莓派偏移校准
 * @param err_x 校准误差x轴
 * @param err_y 校准误差y轴
 * @return 校准状态
 */
bool RPI_Calibrate(int32_t *err_x, int32_t *err_y) {
    if (err_x == NULL || err_y == NULL) {
        return false;
    }

    uint8_t cmd = 0x0A;
    uint8_t rsp[6];
    uint16_t abs_dx, abs_dy;

    HAL_UART_Transmit_IT(&huart2, &cmd, 1);
    HAL_UART_Receive_IT(&huart2, rsp, 6);

    /* 解析6字节响应：[dir-x][dir-y][abs-dx高][abs-dx低][abs-dy高][abs-dy低] */
    abs_dx = ((uint16_t)rsp[2] << 8) | rsp[3];
    abs_dy = ((uint16_t)rsp[4] << 8) | rsp[5];

    /* 根据方向确定符号：1为正轴，0为负轴 */
    *err_x = (rsp[0] == 1) ? (int32_t)abs_dx : -(int32_t)abs_dx;
    *err_y = (rsp[1] == 1) ? (int32_t)abs_dy : -(int32_t)abs_dy;

    RPI_Calibrate_IDE();
    return true;
}

/**
 * @brief 树莓派颜色检测空闲
 */
void RPI_Detect_IDE(void) {
    uint8_t cmd = 0x0C;
    HAL_UART_Transmit_IT(&huart2, &cmd, 1);
}

/**
 * @brief 树莓派偏移校准空闲
 */
void RPI_Calibrate_IDE(void) {
    uint8_t cmd = 0x0E;
    HAL_UART_Transmit_IT(&huart2, &cmd, 1);
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
        int32_t err_x, err_y;
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
