/**
 * @file ops.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 平面定位模块源文件
 */

#include "ops.h"
#include "usart.h"
#include "Events.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dsp_tool.h"


static OPSData_t *g_ops = NULL;
static bool is_init = false;
static void Data_Analyse(const uint8_t rec);
static void OPS_Send_Cmd(const uint8_t *cmd, uint16_t len);
extern osMutexId_t OPS_MutexHandle;

/**
 * @brief 初始化平面定位模块
 * */
bool OPS_Init(void) {
    g_ops = pvPortMalloc(sizeof(OPSData_t));
    if(g_ops == NULL) {
        return false;
    }
    memset(g_ops, 0, sizeof(OPSData_t));
    is_init = true;

    return true;
}

/**
 * @brief 获取平面定位数据
 * @param pose 定位数据指针
 * */
bool OPS_Get(OPSData_t *pose) {
    if(!is_init) return false;
    if (osMutexAcquire(OPS_MutexHandle, osWaitForever) == osOK) {
        memcpy(pose, g_ops, sizeof(OPSData_t));
        osMutexRelease(OPS_MutexHandle);
        return true;
    }

    return false;
}

/**
 * @brief 归零定位模块
 * */
void OPS_Zero(void) {
    OPS_Send_Cmd((const uint8_t *)"ACT0", 4);
}

/**
 * @brief 数据更新任务
 * */
void OPS_Update_Task(void *argument) {
    (void)argument;
    extern osMessageQueueId_t Uart4_Rx_DataHandle;
    Uart4_RxBuf_t rx_buf;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);
    if (!is_init) {
        osMessageQueueDelete(Uart4_Rx_DataHandle);
        vTaskDelete(NULL);
    }
    MX_UART4_Init();

    for (;;) {
        osMessageQueueGet(Uart4_Rx_DataHandle, &rx_buf, NULL, osWaitForever);

        if (osMutexAcquire(OPS_MutexHandle, osWaitForever) == osOK) {
            g_ops->timestamp = xTaskGetTickCount();
            for (uint8_t j = 0; j < rx_buf.len; j++) {
                Data_Analyse(rx_buf.data[j]);
            }
            osMutexRelease(OPS_MutexHandle);
        }
    }
}

/**
 * @brief 检查定位模块是否就绪
 * @return true 如果就绪
 * */
bool OPS_Is_Ready(void) {
    return (g_ops->timestamp != 0 || is_init);
}

#if OPS_CAL
/**
 * @brief 校准定位模块
 */
static void OPS_Cal_Shell(int argc, char *argv[]) {
    extern osThreadId_t OPS_UpdateHandle;
    extern osMessageQueueId_t Uart4_Rx_DataHandle;
    Uart4_RxBuf_t rx_buf;
    uint32_t timeout = 16 * 60 * 1000;
    char spinner[] = {'|', '/', '-', '\\'};
    uint8_t spinner_idx = 0;

    if (!is_init) {
        logWarning("OPS module not initialized"); return;
    }

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);
    logPrintln("The calibration takes about 15 minutes and the error is absolutely stationary.\r\n"
        "Calibration is not recommended in general.\r\n"
        "Would you like to proceed? (y/n)\r\n");


    Shell *shell;
    uint8_t byte;
    shell = shellGetCurrent();
    if (shell == NULL) {osEventFlagsClear(System_StatusHandle, APP_NEED_USART); return;}

    while(1) {
        if (shell->read((char*)&byte, 1)){
            if (byte == 'y') break;
            else {osEventFlagsClear(System_StatusHandle, APP_NEED_USART); return;}
				}
        osDelay(33);
    }

    osThreadSuspend(OPS_UpdateHandle);
    OPS_Send_Cmd((const uint8_t *)"ACTR", 4);
    osMessageQueueReset(Uart4_Rx_DataHandle);

    if (osMessageQueueGet(Uart4_Rx_DataHandle, &rx_buf, NULL, 500) == osOK) {
        logPrintln("Calibration start: %s\r\nPress ^C to stop\r\n", rx_buf.data);
    } else {
        logPrintln("Calibration start timeout");
        osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
        osThreadResume(OPS_UpdateHandle); return;
    }

    uint32_t start_time = HAL_GetTick();
    uint32_t elapsed = 0;
    while (elapsed < timeout) {
        if (osMessageQueueGet(Uart4_Rx_DataHandle, &rx_buf, NULL, 150) == osOK) {
            if (strcmp((char *)rx_buf.data, "check") == 0) {
                logPrintln("\033[1A\033[2K\rCalibration completed"); break;
            }
        }

        elapsed = HAL_GetTick() - start_time;
        uint8_t min = (uint8_t)(elapsed / 60000);
        uint8_t sec = (uint8_t)((elapsed % 60000) / 1000);
        logPrintln("\033[1A\033[2K\rCalibrating...  %c          Usage: %02u:%02u", spinner[spinner_idx], min, sec);
        spinner_idx = (spinner_idx + 1) % 4;

        if (shell->read((char*)&byte, 1)) {
            if (byte == 0x03) {
                logPrintln("Keyboard interruption"); break;
            }
        }
    }

    if (elapsed >= timeout) {
        logPrintln("\033[1A\033[2K\rCalibration timeout");
    }

    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    osThreadResume(OPS_UpdateHandle);
}
#endif /* OPS_CAL */

#if OPS_ZERO
/**
 * @brief 位置重置
 */
static void OPS_Zero_Shell(int argc, char *argv[]) {
    if (!is_init) {
        logWarning("OPS module not initialized"); return;
    }

    OPS_Zero();
    logPrintln("Position reset");
}
#endif /* OPS_ZERO */

#if OPS_SET
/**
 * @brief 设置参数命令处理函数
 */
static void OPS_Set_Shell(int argc, char *argv[]) {
    if (!is_init) {
        logWarning("OPS module not initialized"); return;
    }

    if (argc != 3) {
        logPrintln(OPS_SET_HELP); return;
    }

    char *endptr;
    float value = strtof(argv[2], &endptr);
    if (*endptr != '\0') {
        logPrintln("Invalid value: %s", argv[2]); return;
    }

    union {
        float f;
        uint8_t bytes[4];
    } data;

    uint8_t cmd[8] = {'A', 'C', 'T', 0, 0, 0, 0, 0};

    if (strcmp(argv[1], "j") == 0) {
        cmd[3] = 'J';
        logPrintln("Set yaw to %.2f", value);
    // } else if (strcmp(argv[1], "x") == 0) {
    //     cmd[3] = 'X';
    //     logPrintln("Set X to %.2f", value);
    // } else if (strcmp(argv[1], "y") == 0) {
    //     cmd[3] = 'Y';
    //     logPrintln("Set Y to %.2f", value);
    // } else {
    // 由于实际安装而进行的修改
    } else if (strcmp(argv[1], "y") == 0) {
        data.f = -value;
        cmd[3] = 'X';
        logPrintln("Set Y to %.2f", -value);
    } else if (strcmp(argv[1], "x") == 0) {
        data.f = value;
        cmd[3] = 'Y';
        logPrintln("Set X to %.2f", value);
    } else {
        logPrintln("Invalid command: %s\r\n"
                OPS_SET_HELP, argv[1]); return;
    }

    memcpy(&cmd[4], data.bytes, 4);
    OPS_Send_Cmd(cmd, 8);
}
#endif /* OPS_SET */

/**
 * @brief 实时查看位置数据
 */
static void OPS_View_Shell(void) {
    if (!is_init) {
        logWarning("OPS module not initialized"); return;
    }

    Shell *shell;
    uint8_t byte;
    shell = shellGetCurrent();
    if (shell == NULL) return;

    logPrintln("\033[?25l\rPosition Data Viewer - Press ^C to exit\r\n"
               "X: ------.--  Y: ------.--\r\n"
               "Yaw: ---.--  Pitch: ---.--  Roll: ---.--\r\n"
               "Wz: ---.-- dps  Timestamp:"
    );

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    for (;;) {
        char buf[128];
        int len = 0;
#if OPS_USE_POS
        len += sprintf(buf + len, "\033[3A\033[2K\rX: %.2f  Y: %.2f\r\n", g_ops->x, g_ops->y);
#else
        len += sprintf(buf + len, "X: ------.--  Y: ------.--\r\n");
#endif
#if OPS_USE_YAW
        len += sprintf(buf + len, "Yaw: %.2f  ", g_ops->yaw);
#else
        len += sprintf(buf + len, "Yaw: ---.--  ");
#endif
#if OPS_USE_PITCH
        len += sprintf(buf + len, "Pitch: %.2f  ", g_ops->pitch);
#else
        len += sprintf(buf + len, "Pitch: ---.--  ");
#endif
#if OPS_USE_ROLL
        len += sprintf(buf + len, "Roll: %.2f\r\n", g_ops->roll);
#else
        len += sprintf(buf + len, "Roll: ---.--\r\n");
#endif
#if OPS_USE_ANG_VEL
        len += sprintf(buf + len, "Wz: %.2f dps", g_ops->w_z);
#else
        len += sprintf(buf + len, "Wz: ---.-- dps  ");
#endif
        len += sprintf(buf + len, "Timestamp: %u", g_ops->timestamp);
        logPrintln("%s", buf);

        if (shell->read((char*)&byte, 1)) {
            if (byte == 0x03) break;
        }
        osDelay(10);
    }

    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    logPrintln("\033[4A\033[J\033[2A\033[?25h");
}

ShellCommand OPSGroup[] = {
#if OPS_CAL
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, cal, OPS_Cal_Shell, calibration module),
#endif
#if OPS_ZERO
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, zero, OPS_Zero_Shell, zero all data),
#endif
#if OPS_SET
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, set, OPS_Set_Shell, set yaw/x/y),
#endif
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, view, OPS_View_Shell, view position data),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
ops, OPSGroup, OPS Tool Group);

/**
 * @brief 发送命令到定位模块
 * @param cmd 命令字符串
 * @param len 命令长度
 */
static void OPS_Send_Cmd(const uint8_t *cmd, uint16_t len) {
    HAL_UART_Transmit(&huart4, cmd, len, 100);
}

/**
 * @brief 数据解析函数
 * @param rec 串口接收到的字节数据
 * */
static void Data_Analyse(const uint8_t rec) {
    static uint8_t count, i;
	static union {
		uint8_t date[24];
		float ActVal[6];
	} posture;

	switch(count) {
		case 0: if (rec == 0x0d) count++; else count = 0; break;
		case 1:
			if (rec == 0x0a) { i = 0; count++; }
            else if(rec == 0x0d); else count = 0; break;
		case 2:
			posture.date[i] = rec; i++;
			if (i >= 24) { i = 0; count++; } break;
		case 3:
			if (rec == 0x0a) count++;
			else count = 0; break;
		case 4:
			if(rec == 0x0d) {
#if OPS_USE_POS
				// g_ops->x = posture.ActVal[3];
				// g_ops->y = posture.ActVal[4];
                // 由于实际安装而进行的修改
                g_ops->x = posture.ActVal[4];
				g_ops->y = -posture.ActVal[3];

#endif
#if OPS_USE_YAW
				g_ops->yaw = posture.ActVal[0];
#endif
#if OPS_USE_PITCH
				g_ops->pitch = posture.ActVal[1];
#endif
#if OPS_USE_ROLL
				g_ops->roll = posture.ActVal[2];
#endif
#if OPS_USE_ANG_VEL
				g_ops->w_z = posture.ActVal[5];
#endif
			} count = 0; break;
		default: count = 0; break;
	}
}
