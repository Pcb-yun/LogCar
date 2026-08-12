/**
 * @file nav_track.h
 * @brief 灰度条巡线模块头文件
 *
 * 基于8路灰度传感器（TRACK模块）实现沿灰度条巡线。
 * 通过数字量数据计算巡线偏差，PID控制输出转向角速度。
 */

#ifndef __NAV_TRACK_H__
#define __NAV_TRACK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 巡线状态枚举
 */
typedef enum {
    NAV_TRACK_STATE_IDLE = 0,   // 空闲
    NAV_TRACK_STATE_RUNNING,    // 巡线中
} NavTrackState_t;

/**
 * @brief 巡线参数结构体
 */
typedef struct {
    float forward_speed;     // 巡线前进速度 (cm/s)
    float kp;                // 偏差比例系数
    float ki;                // 偏差积分系数
    float kd;                // 偏差微分系数
    float max_yaw_speed;     // 最大转向角速度 (deg/s)
    int8_t line_polarity;    // 传感器极性: 1=bit为1表示检测到黑线; 0=bit为0表示检测到黑线
    int8_t steering_dir;     // 转向方向: 1=线偏右时右转; -1=反向
    uint32_t run_time_ms;    // 巡线运行时长 (ms)，0=持续运行直到调用Stop
} NavTrackParams_t;

bool Nav_Track_Init(void);
void Nav_Track_Task(void *argument);

bool Nav_Track_Start(void);
void Nav_Track_Stop(void);
NavTrackState_t Nav_Track_GetState(void);

void Nav_Track_GetParams(NavTrackParams_t *params);
void Nav_Track_SetParams(const NavTrackParams_t *params);

void ntrack_time_shell(int argc, char *argv[]);
bool Nav_Track_Start(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NAV_TRACK_H__ */
