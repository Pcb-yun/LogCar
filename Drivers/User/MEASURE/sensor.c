/**
 * @file sensor.c
 * @brief PE1引脚电平读取
 */

#include "sensor.h"
#include "gpio.h"
#include "shell.h"
#include "log.h"
#include "shell_cmd_group.h"
#include <string.h>
#include "Events.h"



bool SENSOR_Read(void) {
    return !(bool)HAL_GPIO_ReadPin(sensor_GPIO_Port, sensor_Pin);
}

static void SENSOR_Read_Shell(void) {
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    uint8_t byte;

    logPrintln("Sensor Reader - Press ^C to exit\r\n"
               "sensor: %d", SENSOR_Read());

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    for (;;) {
        osDelay(100);
        logPrintln("\033[1A\033[2K\rsensor: %d", SENSOR_Read());

        if (osMessageQueueGet(Usart1_Rx_DataHandle, &byte, NULL, 0) == osOK) {
            if (byte == 0x03) break;
        }
    }

    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    logPrintln("\033[1A\033[2K\r");
}

ShellCommand SensorGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, read, SENSOR_Read_Shell, read PE1 pin level continuously),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                       sensor, SensorGroup, Sensor Tool Group);
