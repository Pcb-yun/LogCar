/**
 * @file chassis.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 车体控制模块
 */

#include "chassis.h"
#include "shell.h"
#include "log.h"
#include "shell_cmd_group.h"
#include "motion_control.h"
#include "step_cfg.h"
#include "stdlib.h"
#include "cmsis_os2.h"

static bool is_init = false;

/**
 * @brief 初始化车体控制模块
 * @return 初始化状态
 */
bool Chassis_Init(void) {
	if (!MotionControl_Init()) return false;

	is_init = true;
    return true;
}

#if MOTOR_POS_MODE_TRAPEZOIDAL
/**
 * @brief 将车移动指定距离
 */
static void Car_Move(int argc, char *argv[]) {
	if (!is_init) {
		logWarning("Chassis not initialized"); return;
	}

    if (argc != 4) {
        logPrintln("Usage: car pos [x_offset] [y_offset] [yaw_offset]"); return;
    }

    int32_t x_offset = atoi(argv[1]);
    int32_t y_offset = atoi(argv[2]);
    int32_t yaw_offset = atoi(argv[3]);

    MotionControl_SetPosition(x_offset, y_offset, yaw_offset);
}
#endif /* MOTOR_POS_MODE_TRAPEZOIDAL */

#if MOTOR_VELOCITY_MODE
/**
 * @brief 设置小车速度
 */
static void Car_Vel(int argc, char *argv[]) {
	if (!is_init) {
		logWarning("Chassis not initialized"); return;
	}
    if (argc != 4) {
        logPrintln("Usage: car vel [x] [y] [yaw]"); return;
    }

    int32_t x = atoi(argv[1]);
    int32_t y = atoi(argv[2]);
    int32_t yaw = atoi(argv[3]);

	MotionControl_SetVelocity(x, y, yaw);
}

/**
 * @brief 按键遥控
 */
static void Car_Key(void) {
	if (!is_init) {
		logWarning("Chassis not initialized"); return;
	}

	Shell *shell = shellGetCurrent();
	logPrintln("Key control started. WASD=move, QE=rotate, ^C=exit");

	char prev_key = 0;
	char key;
	uint32_t last_time;
	float linear_speed, yaw_speed, acc, dec;
	MotionControl_GetMotionParams(&linear_speed, &yaw_speed, &acc, &dec);

	for (;;) {
		uint8_t ret = shell->read(&key, 1);

		if (ret > 0) {
			if (key == 0x03) {	// ^C 退出
				MotionControl_Stop(); break;
			}

			if (key == prev_key) {
				last_time = osKernelGetTickCount();
			} else {
				int8_t x_comp = 0, y_comp = 0, yaw_comp = 0;
				bool valid_key = true;

				switch (key) {
				case 'w': case 'W': x_comp = linear_speed;   break;	// 前进
				case 's': case 'S': x_comp = -linear_speed;  break;	// 后退
				case 'a': case 'A': y_comp = linear_speed;   break;	// 左移
				case 'd': case 'D': y_comp = -linear_speed;  break;	// 右移
				case 'q': case 'Q': yaw_comp = yaw_speed; break;	// 逆时针旋转
				case 'e': case 'E': yaw_comp = -yaw_speed; break;	// 顺时针旋转
				default: valid_key = false; break;
				}

				if (valid_key) {
					MotionControl_SetVelocity(x_comp, y_comp, yaw_comp);
					last_time = osKernelGetTickCount();
					prev_key = key;
				}
			}

		}

		if (osKernelGetTickCount() - last_time > 100) {
			prev_key = 0;
			MotionControl_Stop();
		}
		osDelay(1);
	}
    logPrintln("\033[%dA\033[J\033[2A", 1);
}
#endif /* MOTOR_VELOCITY_MODE */

/**
 * @brief 设置车的运动参数
 */
static void Car_Params(int argc, char *argv[]) {
    float linear_speed;
    float yaw_speed;
    float acc;
    float dec;

	if (!is_init) {
		logWarning("Chassis not initialized"); return;
	}

    if (argc == 1) {
        MotionControl_GetMotionParams(&linear_speed, &yaw_speed, &acc, &dec);
        logPrintln("Current params: linear_speed=%.1f, yaw_speed=%.1f, acc=%.1f, dec=%.1f", linear_speed, yaw_speed, acc, dec); return;
    } else if (argc != 5) {
        logPrintln("Usage: car par [linear_speed] [yaw_speed] [acc] [dec]"); return;
    }

    linear_speed = atof(argv[1]);
    yaw_speed = atof(argv[2]);
    acc = atof(argv[3]);
    dec = atof(argv[4]);

    MotionControl_SetMotionParams(linear_speed, yaw_speed, acc, dec);
    logPrintln("Set params: linear_speed=%.1f, yaw_speed=%.1f, acc=%.1f, dec=%.1f", linear_speed, yaw_speed, acc, dec);
}

ShellCommand MoveGroup[] = {
#if MOTOR_POS_MODE_TRAPEZOIDAL
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, pos, Car_Move, Move car),
#endif
#if MOTOR_VELOCITY_MODE
	SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, vel, Car_Vel, Car Key Control),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, key, Car_Key, Car Key Control),
#endif
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, par, Car_Params, Set Car Motion Params),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
car, MoveGroup, Car Control CMD Group);
