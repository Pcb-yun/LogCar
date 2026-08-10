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
#include "shell.h"
#include "shell_cmd_group.h"
#include "log.h"
#include "i2c.h"
#include <stdlib.h>


static bool is_init = false;
static uint16_t update_time = 50;    // 测距间隔(ms)
static VL53L0X_Dev_t *g_vl53l0x_dev = NULL;
static VL53L0X_RangingMeasurementData_t *g_ranging_data = NULL;


/**
 * @brief 校准前准备：关闭可能干扰校准的特性，清除中断
 */
static VL53L0X_Error cal_prepare(void) {
	VL53L0X_Error status = VL53L0X_SetXTalkCompensationEnable(g_vl53l0x_dev, 0);
	if (status != VL53L0X_ERROR_NONE) return status;
	VL53L0X_ClearInterruptMask(g_vl53l0x_dev, 0);
	return VL53L0X_ERROR_NONE;
}

static void restart_continuous(void) {
	VL53L0X_SetDeviceMode(g_vl53l0x_dev, VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING);
	VL53L0X_SetInterMeasurementPeriodMilliSeconds(g_vl53l0x_dev, update_time);
	VL53L0X_SetXTalkCompensationEnable(g_vl53l0x_dev, 0);
	VL53L0X_StartMeasurement(g_vl53l0x_dev);
}

/**
 * @brief 等待用户按键辅助函数
 */
static void wait_for_enter(void) {
    Shell *shell = shellGetCurrent();
    if (shell == NULL) return;
    uint8_t byte;
    logPrintln("Press Enter to continue...");
    while (1) {
        if (shell->read((char*)&byte, 1)) {
            if (byte == '\r' || byte == '\n') break;
        }
        osDelay(10);
    }
}

/**
 * @brief 初始化VL53L0X模块
 * @return 初始化状态
 */
bool VL53L0X_Init(void) {
    MX_I2C2_Init();

    g_vl53l0x_dev = pvPortMalloc(sizeof(VL53L0X_Dev_t));
    if (g_vl53l0x_dev == NULL) return false;

    g_ranging_data = pvPortMalloc(sizeof(VL53L0X_RangingMeasurementData_t));
    if (g_ranging_data == NULL) {
        vPortFree(g_vl53l0x_dev);
        return false;
    }
    memset(g_vl53l0x_dev, 0, sizeof(VL53L0X_Dev_t));
    memset(g_ranging_data, 0, sizeof(VL53L0X_RangingMeasurementData_t));
    g_vl53l0x_dev->I2cHandle = &hi2c2;
    g_vl53l0x_dev->I2cDevAddr = 0x52;

    // 1. 数据初始化（加载NVM校准数据）
    VL53L0X_Error status = VL53L0X_DataInit(g_vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 2. 静态初始化（应用默认设置）
    status = VL53L0X_StaticInit(g_vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 3. 恢复手动校准值（运行dist cal all后将CAL_宏的值填入）
#if CAL_REF_SPAD_COUNT > 0
    VL53L0X_SetReferenceSpads(g_vl53l0x_dev, CAL_REF_SPAD_COUNT, CAL_IS_APERTURE_SPADS);
#endif
#if CAL_VHV_SETTINGS > 0 || CAL_PHASE_CAL > 0
    VL53L0X_SetRefCalibration(g_vl53l0x_dev, CAL_VHV_SETTINGS, CAL_PHASE_CAL);
#endif
#if CAL_OFFSET_UM != 0
    VL53L0X_SetOffsetCalibrationDataMicroMeter(g_vl53l0x_dev, CAL_OFFSET_UM);
#endif

    // 4. 设置设备模式为定时连续测距
    status = VL53L0X_SetDeviceMode(g_vl53l0x_dev, VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 4. 设置测量时间预算（必须小于测距间隔）
    status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(g_vl53l0x_dev, DIST_TimeBudget);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 5. 设置测距间隔
    status = VL53L0X_SetInterMeasurementPeriodMilliSeconds(g_vl53l0x_dev, update_time);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    // 6. 启动连续测距
    status = VL53L0X_StartMeasurement(g_vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) goto clean_up;

    is_init = true;
    return true;

clean_up:
    vPortFree(g_vl53l0x_dev);
    vPortFree(g_ranging_data);
    return false;
}

/**
 * @brief 测距任务
 */
void Dist_Get_Task(void *argument) {
    (void) argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);
    if (!is_init)  vTaskDelete(NULL);

    for (;;) {
        osDelay(update_time);
        VL53L0X_GetRangingMeasurementData(g_vl53l0x_dev, g_ranging_data);
        VL53L0X_ClearInterruptMask(g_vl53l0x_dev, 0);
    }
}

/**
 * @brief 获取测距数据
 * @return 测距数据（单位：mm）
 */
uint16_t Dist_Get(void) {
    if (!is_init) return 0;
    if (g_ranging_data->RangeStatus != 0) return 0;
    return g_ranging_data->RangeMilliMeter;
}

/**
 * @brief 设置测距间隔
 */
static void Dist_Time(int argc, char *argv[]) {
    if (!is_init) {
        logPrintln("VL53L0X Module not initialized"); return;
    }

    if (argc != 2) {
        logPrintln("Usage: time [ms]"); return;
    }

    char *endptr;
    long val = strtol(argv[1], &endptr, 10);
    if(*endptr != '\0') {
        logPrintln("invalid time value: %s", argv[1]); return;
    } else {
        if (val < DIST_TimeBudget / 1000) {
            logPrintln("time value must be greater than %d ms", DIST_TimeBudget / 1000); return;
        }
        update_time = (uint16_t)val;
        restart_continuous();
    }
}

/**
 * @brief 实时刷新测距数据
 */
static void Dist_View(void) {
    if (!is_init) {
        logWarning("VL53L0X Module not initialized"); return;
    }
    logPrintln("VL53L0X Distance Viewer - Press ^C to exit\r\n"
               "Distance: ---- mm\r\n"
               "Status:");

    Shell *shell;
    uint8_t byte;
    shell = shellGetCurrent();
    if (shell == NULL) return;

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    for(;;) {
        char status_str[32];
        VL53L0X_GetRangeStatusString(g_ranging_data->RangeStatus, status_str);
        logPrintln("\033[2A\033[2K\rDistance: %d mm\r\n"
                   "Status: %s",
                    g_ranging_data->RangeMilliMeter,
                    status_str);

        if (shell->read((char*)&byte, 1)) {
            if (byte == 0x03) break;
        }
        osDelay(update_time);
    }
    logPrintln("\033[3A\033[J\033[2A");
    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
}

/**
 * @brief 测距校准命令组
 */
static void Dist_Cal(int argc, char *argv[]) {
	if (!is_init) {
		logPrintln("VL53L0X Module not initialized"); return;
	}
	if (argc < 2) {
		logPrintln(DIST_CAL_ALL_HELP); return;
	}

	osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

	VL53L0X_Error status;

	if (strcmp(argv[1], "ref") == 0) {
		status = cal_prepare();
		if (status != VL53L0X_ERROR_NONE) {
			logPrintln("FAILED: %d", status);
		} else {
			uint8_t vhv_settings, phase_cal;
			status = VL53L0X_PerformRefCalibration(g_vl53l0x_dev, &vhv_settings, &phase_cal);
			if (status == VL53L0X_ERROR_NONE) {
				logPrintln("Ref calibration success: VHV=%d, Phase=%d", vhv_settings, phase_cal);
			} else {
				logPrintln("FAILED: %d", status);
			}
		}
	} else if (strcmp(argv[1], "spad") == 0) {
		status = cal_prepare();
		if (status != VL53L0X_ERROR_NONE) {
			logPrintln("FAILED: %d", status);
		} else {
			uint32_t ref_spad_count;
			uint8_t is_aperture_spads;
			status = VL53L0X_PerformRefSpadManagement(g_vl53l0x_dev, &ref_spad_count, &is_aperture_spads);
			if (status == VL53L0X_ERROR_NONE) {
				logPrintln("SPAD management success: count=%lu, aperture=%d", ref_spad_count, is_aperture_spads);
			} else {
				logPrintln("FAILED: %d", status);
			}
		}
	} else if (strcmp(argv[1], "xwalk") == 0) {
		uint16_t xtalk_dist;
		if (argc >= 3) {
			char *endptr;
			long val = strtol(argv[2], &endptr, 10);
			if (*endptr != '\0' || val <= 0) {
				logPrintln("Invalid distance: %s", argv[2]); goto cal_exit;
			}
			xtalk_dist = (uint16_t)val;
		} else {
			xtalk_dist = 400;
		}
		logPrintln("XTalk calibration at %u mm\r\n"
				   "Place grey target (17%% reflectivity) at %u mm", xtalk_dist, xtalk_dist);
		wait_for_enter();
		status = cal_prepare();
		if (status != VL53L0X_ERROR_NONE) {
			logPrintln("FAILED: %d", status);
		} else {
			FixPoint1616_t compensation;
			status = VL53L0X_PerformXTalkCalibration(g_vl53l0x_dev, (FixPoint1616_t)(xtalk_dist * 65536UL), &compensation);
			if (status == VL53L0X_ERROR_NONE) {
				logPrintln("  -> compensation=%d MCPS", (int)(compensation >> 16));
			} else {
				logPrintln("FAILED: %d at %u mm, try a shorter distance", status, xtalk_dist);
			}
		}
	} else if (strcmp(argv[1], "offset") == 0) {
		if (argc != 3) {
			logPrintln("Usage: dist cal offset [distance_mm]\r\n"
					   "Place target at given distance"); goto cal_exit;
		}
		char *endptr;
		long cal_dist = strtol(argv[2], &endptr, 10);
		if (*endptr != '\0' || cal_dist <= 0) {
			logPrintln("Invalid distance: %s", argv[2]); goto cal_exit;
		}
		logPrintln("Offset calibration at %ld mm", cal_dist);
		wait_for_enter();
		status = cal_prepare();
		if (status != VL53L0X_ERROR_NONE) {
			logPrintln("FAILED: %d", status);
		} else {
			FixPoint1616_t cal_dist_fp = (FixPoint1616_t)(cal_dist << 16);
			int32_t offset_micrometer;
			status = VL53L0X_PerformOffsetCalibration(g_vl53l0x_dev, cal_dist_fp, &offset_micrometer);
			if (status == VL53L0X_ERROR_NONE) {
				logPrintln("  -> offset=%ld um", offset_micrometer);
			} else {
				logPrintln("FAILED: %d", status);
			}
		}
	} else if (strcmp(argv[1], "all") == 0) {
		logPrintln("=== Full calibration sequence (UM2039) ===");

		logPrintln("[1/4] SPAD management (auto)...");
		cal_prepare();
		uint32_t ref_spad_count;
		uint8_t is_aperture_spads;
		status = VL53L0X_PerformRefSpadManagement(g_vl53l0x_dev, &ref_spad_count, &is_aperture_spads);
		if (status != VL53L0X_ERROR_NONE) {
			logPrintln("FAILED: %d", status); goto cal_exit;
		}
		logPrintln("  -> count=%lu, aperture=%d", ref_spad_count, is_aperture_spads);

		logPrintln("[2/4] Reference calibration (auto)...");
		cal_prepare();
		uint8_t vhv_settings, phase_cal;
		status = VL53L0X_PerformRefCalibration(g_vl53l0x_dev, &vhv_settings, &phase_cal);
		if (status != VL53L0X_ERROR_NONE) {
			logPrintln("FAILED: %d", status); goto cal_exit;
		}
		logPrintln("  -> VHV=%d, Phase=%d", vhv_settings, phase_cal);

		logPrintln("[3/4] Offset calibration\r\n"
				   "Place white target (88%%) at exactly 100mm\r\n"
				   "Dark environment recommended");
		cal_prepare();
		wait_for_enter();
		int32_t offset_micrometer;
		status = VL53L0X_PerformOffsetCalibration(g_vl53l0x_dev, (FixPoint1616_t)(100*65536), &offset_micrometer);
		if (status != VL53L0X_ERROR_NONE) {
			logPrintln("FAILED: %d", status); goto cal_exit;
		}
		logPrintln("  -> offset=%ld um", offset_micrometer);

		logPrintln("[4/4] XTalk calibration\r\n"
				   "Place grey target (17%%) at ~400mm");
		cal_prepare();
		wait_for_enter();
		FixPoint1616_t compensation;
		status = VL53L0X_PerformXTalkCalibration(g_vl53l0x_dev, (FixPoint1616_t)(400*65536), &compensation);
		if (status != VL53L0X_ERROR_NONE) {
			logPrintln("FAILED: %d", status); goto cal_exit;
		}
		logPrintln("  -> compensation=%d MCPS", (int)(compensation >> 16));

		logPrintln("=== Calibration done ===\r\n"
				   "Copy these into vl53l0x_port.c:\r\n"
				   "#define CAL_REF_SPAD_COUNT      %lu\r\n"
				   "#define CAL_IS_APERTURE_SPADS   %u\r\n"
				   "#define CAL_VHV_SETTINGS        %u\r\n"
				   "#define CAL_PHASE_CAL           %u\r\n"
				   "#define CAL_OFFSET_UM           %ld",
				   ref_spad_count, is_aperture_spads, vhv_settings, phase_cal, offset_micrometer);
	} else if (strcmp(argv[1], "show") == 0) {
		uint32_t spad_count;
		uint8_t is_aperture;
		uint8_t vhv, phase;
		int32_t offset_um;
		FixPoint1616_t xtalk_rate;
		VL53L0X_GetReferenceSpads(g_vl53l0x_dev, &spad_count, &is_aperture);
		VL53L0X_GetRefCalibration(g_vl53l0x_dev, &vhv, &phase);
		VL53L0X_GetOffsetCalibrationDataMicroMeter(g_vl53l0x_dev, &offset_um);
		VL53L0X_GetXTalkCompensationRateMegaCps(g_vl53l0x_dev, &xtalk_rate);
		logPrintln("Current calibration values:\r\n"
			"#define CAL_REF_SPAD_COUNT      %lu\r\n"
			"#define CAL_IS_APERTURE_SPADS   %u\r\n"
			"#define CAL_VHV_SETTINGS        %u\r\n"
			"#define CAL_PHASE_CAL           %u\r\n"
			"#define CAL_OFFSET_UM           %ld",
			spad_count, is_aperture, vhv, phase, offset_um);
	} else {
		logPrintln("Unknown calibration type: %s", argv[1]);
	}

cal_exit:
	restart_continuous();
	osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
}


ShellCommand Vl53l0XGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, time, Dist_Time, Set Update time),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, cal, Dist_Cal, VL53L0X Calibration Group),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, view, Dist_View, view distance),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                       dist, Vl53l0XGroup, VL53L0X Cmd Group);
