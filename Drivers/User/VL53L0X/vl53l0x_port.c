/**
 * @file vl53l0x_port.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief VL53L0X模块用户层源文件
 */

#include "vl53l0x_port.h"
#include "i2c.h"
#include "Events.h"
#include "freeRTOS.h"
#include "task.h"

static bool is_init = false;
VL53L0X_Dev_t *g_vl53l0x_dev = NULL;
VL53L0X_RangingMeasurementData_t *g_ranging_data = NULL;


/**
 * @brief 初始化VL53L0X模块
 * @return 初始化状态
 */
bool VL53L0X_Init(void) {
    g_vl53l0x_dev = pvPortMalloc(sizeof(VL53L0X_Dev_t));
    if (g_vl53l0x_dev == NULL) {
        return false;
    }
    g_ranging_data = pvPortMalloc(sizeof(VL53L0X_RangingMeasurementData_t));
    if (g_ranging_data == NULL) {
        vPortFree(g_vl53l0x_dev);
        return false;
    }
    memset(g_vl53l0x_dev, 0, sizeof(VL53L0X_Dev_t));
    memset(g_ranging_data, 0, sizeof(VL53L0X_RangingMeasurementData_t));
    g_vl53l0x_dev->I2cHandle = &hi2c1;
    g_vl53l0x_dev->I2cDevAddr = 0x52;

    // 1. 数据初始化（加载NVM校准数据）
    VL53L0X_Error status = VL53L0X_DataInit(g_vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 2. 静态初始化（应用默认设置）
    status = VL53L0X_StaticInit(g_vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 3. 执行参考校准
    uint8_t vhv_settings, phase_cal;
    status = VL53L0X_PerformRefCalibration(g_vl53l0x_dev, &vhv_settings, &phase_cal);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 4. 参考SPAD管理
    uint32_t ref_spad_count;
    uint8_t is_aperture_spads;
    status = VL53L0X_PerformRefSpadManagement(g_vl53l0x_dev, &ref_spad_count, &is_aperture_spads);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 5. 设置设备模式为单次测距
    status = VL53L0X_SetDeviceMode(g_vl53l0x_dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    is_init = true;
    return true;

clean_up:
    vPortFree(g_vl53l0x_dev);
    return false;
}


void Dist_Get_Task(void *argument) {
    (void) argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!is_init)  vTaskDelete(NULL);

    for (;;) {
        osDelay(1000);




    }
}
