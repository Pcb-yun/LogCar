#include "shell.h"
#include "log.h"
#include "oled_port.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "ops.h"
#include "Events.h"
#include <stdio.h>


/**
 * @brief 在屏幕上显示OPS数据
 */
void ops_show(void) {
    extern osMutexId_t OLED_MutexHandle;
    Shell *shell;
    uint8_t byte;
    shell = shellGetCurrent();
	
    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);
    for (;;) {
        osMutexAcquire(OLED_MutexHandle, osWaitForever);
        ssd1306_Fill(Black);
        osMutexRelease(OLED_MutexHandle);

        OPSData_t ops_data;
        if (!OPS_Get(&ops_data)) break;

        char buf[32];
        uint8_t y_offset = 0;

        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("OPS:", Font_7x10, White);

        y_offset += Font_7x10.height + 5;
        ssd1306_SetCursor(0, y_offset);
        snprintf(buf, sizeof(buf), "X: %.2f", ops_data.x);
        ssd1306_WriteString(buf, Font_7x10, White);

        y_offset += Font_7x10.height + 5;
        ssd1306_SetCursor(0, y_offset);
        snprintf(buf, sizeof(buf), "Y: %.2f", ops_data.y);
        ssd1306_WriteString(buf, Font_7x10, White);
        
        y_offset += Font_7x10.height + 5;
        ssd1306_SetCursor(0, y_offset);
        snprintf(buf, sizeof(buf), "Yaw: %.2f", ops_data.yaw);
        ssd1306_WriteString(buf, Font_7x10, White);

        y_offset += Font_7x10.height + 5;
        ssd1306_SetCursor(0, y_offset);
        snprintf(buf, sizeof(buf), "Time: %d", ops_data.timestamp);
        ssd1306_WriteString(buf, Font_7x10, White);

        if (shell->read((char*)&byte, 1)) {
            if (byte == 0x03) break;
        }
        osDelay(10);
    }
    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
}
