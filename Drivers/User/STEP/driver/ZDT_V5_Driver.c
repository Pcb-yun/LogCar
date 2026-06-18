/**
 * @file ZDT_V5_Driver.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机驱动层源文件
 */

#include "ZDT_V5_Driver.h"

/******************** 触发动作指令 **********************/

/**
  * @brief 触发编码器校准
  * @param addr 电机地址
  */
void ZDT_V5_Trig_Encoder_Cal(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x06; cmd[2] = 0x45; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 重启电机（Y42）
  * @param addr 电机地址
  */
void ZDT_V5_Reset_Motor(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x08; cmd[2] = 0x97; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 将当前位置清零
  * @param addr 电机地址
  */
void ZDT_V5_Reset_CurPos_To_Zero(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x0A; cmd[2] = 0x6D; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 解除堵转保护
  * @param addr 电机地址
  */
void ZDT_V5_Reset_Clog_Pro(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x0E; cmd[2] = 0x52; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 恢复出厂设置
  * @param addr 电机地址
  */
void ZDT_V5_Restore_Motor(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x0F; cmd[2] = 0x5F; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

/******************** 运动控制指令 **********************/

#if MOTOR_CMD_ENABLE
/**
  * @brief 使能信号控制
  * @param addr 电机地址
  * @param state 使能状态,true为使能电机,false为关闭电机
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void ZDT_V5_En_Control(uint8_t addr, bool state, bool snF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xF3; cmd[2] = 0xAB; cmd[3] = (uint8_t)state; cmd[4] = snF; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_CMD_VELOCITY
/**
  * @brief 速度模式
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param vel 速度(RPM) 0-5000
  * @param acc 加速度(RPM/S或档位)
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void ZDT_V5_Vel_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, bool snF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xF6;
#if CURRENT_FIRMWARE == FIRMWARE_X
    cmd[2] = dir;
    cmd[3] = (uint8_t)(acc >> 8); cmd[4] = (uint8_t)(acc >> 0);
    cmd[5] = (uint8_t)(vel >> 8); cmd[6] = (uint8_t)(vel >> 0);
    cmd[7] = snF; cmd[8] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 9);
#else
    cmd[2] = dir; cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0);
    cmd[5] = (uint8_t)acc; cmd[6] = snF; cmd[7] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 8);
#endif
}
#endif

#if MOTOR_CMD_POSITION
/**
  * @brief 位置模式
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param vel 速度(RPM) 0-5000
  * @param acc 加速度(RPM/S或档位)
  * @param dec 减速度(仅X固件)
  * @param clk 脉冲数/角度 0- (2^32 - 1)
  * @param raF 相位/绝对标志,false为相对运动,true为绝对值运动
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void ZDT_V5_Pos_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, uint16_t dec, uint32_t clk, bool raF, bool snF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xFD;
#if CURRENT_FIRMWARE == FIRMWARE_X
    cmd[2] = dir;
    cmd[3] = (uint8_t)(acc >> 8); cmd[4] = (uint8_t)(acc >> 0);
    cmd[5] = (uint8_t)(dec >> 8); cmd[6] = (uint8_t)(dec >> 0);
    cmd[7] = (uint8_t)(vel >> 8); cmd[8] = (uint8_t)(vel >> 0);
    cmd[9] = (uint8_t)(clk >> 24); cmd[10] = (uint8_t)(clk >> 16); cmd[11] = (uint8_t)(clk >> 8); cmd[12] = (uint8_t)(clk >> 0);
    cmd[13] = (uint8_t)raF; cmd[14] = snF; cmd[15] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 16);
#else
    cmd[2] = dir; cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0);
    cmd[5] = (uint8_t)acc;
    cmd[6] = (uint8_t)(clk >> 24); cmd[7] = (uint8_t)(clk >> 16); cmd[8] = (uint8_t)(clk >> 8); cmd[9] = (uint8_t)(clk >> 0);
    cmd[10] = raF; cmd[11] = snF; cmd[12] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 13);
#endif
}
#endif

#if MOTOR_CMD_TORQUE
/**
  * @brief 力矩模式控制
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param slope 电流斜率(mA/S) 0-65535
  * @param current 目标电流(mA) 0-5000
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void ZDT_V5_Torque_Control(uint8_t addr, uint8_t dir, uint16_t slope, uint16_t current, bool snF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xF5; cmd[2] = dir;
    cmd[3] = (uint8_t)(slope >> 8); cmd[4] = (uint8_t)(slope >> 0);
    cmd[5] = (uint8_t)(current >> 8); cmd[6] = (uint8_t)(current >> 0);
    cmd[7] = snF; cmd[8] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 9);
}
#endif

#if MOTOR_CMD_TORQUE && CURRENT_FIRMWARE == FIRMWARE_X
/**
  * @brief 力矩模式限速控制
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param slope 电流斜率(mA/S) 0-65535
  * @param current 目标电流(mA) 0-5000
  * @param max_vel 最大速度(0.1RPM) 0-30000
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void ZDT_V5_Torque_Control_With_Limit(uint8_t addr, uint8_t dir, uint16_t slope, uint16_t current, uint16_t max_vel, bool snF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xC5; cmd[2] = dir;
    cmd[3] = (uint8_t)(slope >> 8); cmd[4] = (uint8_t)(slope >> 0);
    cmd[5] = (uint8_t)(current >> 8); cmd[6] = (uint8_t)(current >> 0);
    cmd[7] = snF;
    cmd[8] = (uint8_t)(max_vel >> 8); cmd[9] = (uint8_t)(max_vel >> 0);
    cmd[10] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 11);
}
#endif

#if MOTOR_CMD_STOP
/**
  * @brief 立即停止
  * @param addr 电机地址
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void ZDT_V5_Stop_Now(uint8_t addr, bool snF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xFE; cmd[2] = 0x98; cmd[3] = snF; cmd[4] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 5);
}
#endif

/**
  * @brief 多机同步运动
  * @param addr 电机地址
  */
void ZDT_V5_Synchronous_motion(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xFF; cmd[2] = 0x66; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

#if MOTOR_CMD_VELOCITY && CURRENT_FIRMWARE == FIRMWARE_X
/**
  * @brief 速度模式限电流控制（X固件）
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param vel 速度(RPM) 0-3000.0
  * @param acc 加速度(RPM/S) 0-65535
  * @param snF 多机同步标志,false为不启用,true为启用
  * @param max_current 最大电流(mA) 0-5000
  */
void ZDT_V5_Vel_Control_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, bool snF, uint16_t max_current) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xC6; cmd[2] = dir;
    cmd[3] = (uint8_t)(acc >> 8); cmd[4] = (uint8_t)(acc >> 0);
    cmd[5] = (uint8_t)(vel >> 8); cmd[6] = (uint8_t)(vel >> 0);
    cmd[7] = snF;
    cmd[8] = (uint8_t)(max_current >> 8); cmd[9] = (uint8_t)(max_current >> 0);
    cmd[10] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 11);
}
#endif

#if MOTOR_CMD_POSITION && CURRENT_FIRMWARE == FIRMWARE_X
/**
  * @brief 直通限速位置模式控制（X固件）
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param vel 速度(RPM) 0-3000.0
  * @param pos 位置角度(0.1°) 0- (2^32 - 1)
  * @param mode 运动模式:0-相对上一目标,1-绝对位置,2-相对当前位置
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void ZDT_V5_Pos_Control_Direct(uint8_t addr, uint8_t dir, uint16_t vel, uint32_t pos, uint8_t mode, bool snF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xFB; cmd[2] = dir;
    cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0);
    cmd[5] = (uint8_t)(pos >> 24); cmd[6] = (uint8_t)(pos >> 16); cmd[7] = (uint8_t)(pos >> 8); cmd[8] = (uint8_t)(pos >> 0);
    cmd[9] = mode; cmd[10] = snF; cmd[11] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 12);
}

/**
  * @brief 直通限速位置模式限电流控制（X固件）
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param vel 速度(RPM) 0-3000.0
  * @param pos 位置角度(0.1°) 0- (2^32 - 1)
  * @param mode 运动模式:0-相对上一目标,1-绝对位置,2-相对当前位置
  * @param snF 多机同步标志,false为不启用,true为启用
  * @param max_current 最大电流(mA) 0-5000
  */
void ZDT_V5_Pos_Control_Direct_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint32_t pos, uint8_t mode, bool snF, uint16_t max_current) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xCB; cmd[2] = dir;
    cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0);
    cmd[5] = (uint8_t)(pos >> 24); cmd[6] = (uint8_t)(pos >> 16); cmd[7] = (uint8_t)(pos >> 8); cmd[8] = (uint8_t)(pos >> 0);
    cmd[9] = mode; cmd[10] = snF;
    cmd[11] = (uint8_t)(max_current >> 8); cmd[12] = (uint8_t)(max_current >> 0);
    cmd[13] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 14);
}

/**
  * @brief 梯形曲线位置模式限电流控制（X固件）
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param vel 速度(RPM) 0-3000.0
  * @param acc 加速度(RPM/S) 0-65535
  * @param dec 减速度(RPM/S) 0-65535
  * @param pos 位置角度(0.1°) 0- (2^32 - 1)
  * @param mode 运动模式:0-相对上一目标,1-绝对位置,2-相对当前位置
  * @param snF 多机同步标志,false为不启用,true为启用
  * @param max_current 最大电流(mA) 0-5000
  */
void ZDT_V5_Pos_Control_Trapezoidal_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, uint16_t dec, uint32_t pos, uint8_t mode, bool snF, uint16_t max_current) {
    uint8_t cmd[24] = {0};
    cmd[0] = addr; cmd[1] = 0xCD; cmd[2] = dir;
    cmd[3] = (uint8_t)(acc >> 8); cmd[4] = (uint8_t)(acc >> 0);
    cmd[5] = (uint8_t)(dec >> 8); cmd[6] = (uint8_t)(dec >> 0);
    cmd[7] = (uint8_t)(vel >> 8); cmd[8] = (uint8_t)(vel >> 0);
    cmd[9] = (uint8_t)(pos >> 24); cmd[10] = (uint8_t)(pos >> 16); cmd[11] = (uint8_t)(pos >> 8); cmd[12] = (uint8_t)(pos >> 0);
    cmd[13] = mode; cmd[14] = snF;
    cmd[15] = (uint8_t)(max_current >> 8); cmd[16] = (uint8_t)(max_current >> 0);
    cmd[17] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 18);
}
#endif

#if MOTOR_CMD_FAST
/******************** 快速位置模式(User) **********************/

/**
  * @brief 快速位置模式 - 设定参数
  * @param addr 电机地址
  * @param vel 速度（RPM）
  * @param acc 加速度（Emm：档位，X：加速加速度）
  * @param dec 减速度（仅X固件）
  * @param max_current 最大电流（仅X固件）
  * @param mode 运动模式：0-相对上一目标，1-绝对位置，2-相对当前位置
  * @param sync 同步标志：0-立即执行，1-等待同步触发
  */
void ZDT_V5_Fast_Set_Param(uint8_t addr, uint16_t vel, uint16_t acc, uint16_t dec, uint16_t max_current, uint8_t mode, uint8_t sync) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xF1;
#if CURRENT_FIRMWARE == FIRMWARE_EMM
    // Emm固件：速度(2) + 加速度(1) + 运动模式(1) + 同步标志(1)
    cmd[2] = (uint8_t)(vel >> 8); cmd[3] = (uint8_t)(vel >> 0);
    cmd[4] = (uint8_t)acc;
    cmd[5] = mode;
    cmd[6] = sync;
    cmd[7] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 8);
#else
    // X固件：加速加速度(2) + 减速加速度(2) + 最大速度(2) + 运动模式(1) + 同步标志(1) + 最大电流(2)
    cmd[2] = (uint8_t)(acc >> 8); cmd[3] = (uint8_t)(acc >> 0);
    cmd[4] = (uint8_t)(dec >> 8); cmd[5] = (uint8_t)(dec >> 0);
    cmd[6] = (uint8_t)(vel >> 8); cmd[7] = (uint8_t)(vel >> 0);
    cmd[8] = mode;
    cmd[9] = sync;
    cmd[10] = (uint8_t)(max_current >> 8); cmd[11] = (uint8_t)(max_current >> 0);
    cmd[12] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 13);
#endif
}

/**
  * @brief 快速位置模式 - 发送位置
  * @param addr 电机地址
  * @param pos 目标位置（Emm：脉冲数，X：角度）
  */
void ZDT_V5_Fast_Send_Pos(uint8_t addr, int32_t pos) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xFC;
    cmd[2] = (uint8_t)(pos >> 24); cmd[3] = (uint8_t)(pos >> 16);
    cmd[4] = (uint8_t)(pos >> 8); cmd[5] = (uint8_t)(pos >> 0);
    cmd[6] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 7);
}
#endif

#if MOTOR_CMD_HOME
/******************** 回零点指令 **********************/

/**
  * @brief 设置单圈回零的零点位置
  * @param addr 电机地址
  * @param svF 存储标志,false为不存储,true为存储
  */
void ZDT_V5_Origin_Set_O(uint8_t addr, bool svF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x93; cmd[2] = 0x88; cmd[3] = svF; cmd[4] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 5);
}

/**
  * @brief 触发回零
  * @param addr 电机地址
  * @param o_mode 回零模式,0为单圈就近回零,1为单圈方向回零,2为多圈无限位碰撞回零,3为多圈有限位开关回零
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void ZDT_V5_Origin_Trigger_Return(uint8_t addr, uint8_t o_mode, bool snF) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x9A; cmd[2] = o_mode; cmd[3] = snF; cmd[4] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 5);
}

/**
  * @brief 强制中断并退出回零
  * @param addr 电机地址
  */
void ZDT_V5_Origin_Interrupt(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x9C; cmd[2] = 0x48; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 读取回零参数
  * @param addr 电机地址
  */
void ZDT_V5_Origin_Read_Params(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x22; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改回零参数
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param o_mode 回零模式,0为单圈就近回零,1为单圈方向回零,2为多圈无限位碰撞回零,3为多圈有限位开关回零
  * @param o_dir 回零方向,0为CW，其余值为CCW
  * @param o_vel 回零速度,单位：RPM（转/分钟）
  * @param o_tm 回零超时时间,单位：毫秒
  * @param sl_vel 无限位碰撞回零检测转速,单位：RPM（转/分钟）
  * @param sl_ma 无限位碰撞回零检测电流,单位：Ma（毫安）
  * @param sl_ms 无限位碰撞回零检测时间,单位：Ms（毫秒）
  * @param potF 上电自动触发回零,false为不使能,true为使能
  */
void ZDT_V5_Origin_Modify_Params(uint8_t addr, bool svF, uint8_t o_mode, uint8_t o_dir, uint16_t o_vel, uint32_t o_tm, uint16_t sl_vel, uint16_t sl_ma, uint16_t sl_ms, bool potF) {
    uint8_t cmd[32] = {0};
    cmd[0] = addr; cmd[1] = 0x4C; cmd[2] = 0xAE; cmd[3] = svF; cmd[4] = o_mode; cmd[5] = o_dir;
    cmd[6] = (uint8_t)(o_vel >> 8); cmd[7] = (uint8_t)(o_vel >> 0);
    cmd[8] = (uint8_t)(o_tm >> 24); cmd[9] = (uint8_t)(o_tm >> 16); cmd[10] = (uint8_t)(o_tm >> 8); cmd[11] = (uint8_t)(o_tm >> 0);
    cmd[12] = (uint8_t)(sl_vel >> 8); cmd[13] = (uint8_t)(sl_vel >> 0);
    cmd[14] = (uint8_t)(sl_ma >> 8); cmd[15] = (uint8_t)(sl_ma >> 0);
    cmd[16] = (uint8_t)(sl_ms >> 8); cmd[17] = (uint8_t)(sl_ms >> 0);
    cmd[18] = potF; cmd[19] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 20);
}
#endif

/******************** 读取系统参数命令 **********************/

/**
  * @brief 定时返回信息命令（Y42）
  * @param addr 电机地址
  * @param s 系统参数类型
  * @param time_ms 定时时间
  */
void ZDT_V5_Auto_Return_Sys_Params_Timed(uint8_t addr, SysParams_t s, uint16_t time_ms) {
    uint8_t i = 0; uint8_t cmd[16] = {0};
    cmd[i] = addr; ++i;
    cmd[i] = 0x11; ++i;
    cmd[i] = 0x18; ++i;
    switch(s) {
        case S_VBUS : cmd[i] = 0x24; ++i; break;
#if CURRENT_FIRMWARE == FIRMWARE_X
        case S_CBUS : cmd[i] = 0x26; ++i; break;
#endif
        case S_CPHA : cmd[i] = 0x27; ++i; break;
        case S_ENCO : cmd[i] = 0x29; ++i; break;
        case S_CLKC : cmd[i] = 0x30; ++i; break;
        case S_ENCL : cmd[i] = 0x31; ++i; break;
        case S_CLKI : cmd[i] = 0x32; ++i; break;
        case S_TPOS : cmd[i] = 0x33; ++i; break;
        case S_SPOS : cmd[i] = 0x34; ++i; break;
        case S_VEL  : cmd[i] = 0x35; ++i; break;
        case S_CPOS : cmd[i] = 0x36; ++i; break;
        case S_PERR : cmd[i] = 0x37; ++i; break;
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case S_VBAT : cmd[i] = 0x38; ++i; break;
#endif
        case S_TEMP : cmd[i] = 0x39; ++i; break;
        case S_FLAG : cmd[i] = 0x3A; ++i; break;
        case S_OFLAG: cmd[i] = 0x3B; ++i; break;
        case S_OAF  : cmd[i] = 0x3C; ++i; break;
        case S_PIN  : cmd[i] = 0x3D; ++i; break;
        default: break;
    }
    cmd[i] = (uint8_t)(time_ms >> 8); ++i;
    cmd[i] = (uint8_t)(time_ms >> 0); ++i;
    cmd[i] = 0x6B; ++i;
    ZDT_V5_SEND_CMD(cmd, i);
}

/**
  * @brief 读取系统参数
  * @param addr 电机地址
  * @param s 系统参数类型
  */
void ZDT_V5_Read_Sys_Params(uint8_t addr, SysParams_t s) {
    uint8_t i = 0; uint8_t cmd[16] = {0};
    cmd[i] = addr; ++i;
    switch(s) {
        case S_VBUS : cmd[i] = 0x24; ++i; break;
#if CURRENT_FIRMWARE == FIRMWARE_X
        case S_CBUS : cmd[i] = 0x26; ++i; break;
#endif
        case S_CPHA : cmd[i] = 0x27; ++i; break;
        case S_ENCO : cmd[i] = 0x29; ++i; break;
        case S_CLKC : cmd[i] = 0x30; ++i; break;
        case S_ENCL : cmd[i] = 0x31; ++i; break;
        case S_CLKI : cmd[i] = 0x32; ++i; break;
        case S_TPOS : cmd[i] = 0x33; ++i; break;
        case S_SPOS : cmd[i] = 0x34; ++i; break;
        case S_VEL  : cmd[i] = 0x35; ++i; break;
        case S_CPOS : cmd[i] = 0x36; ++i; break;
        case S_PERR : cmd[i] = 0x37; ++i; break;
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case S_VBAT : cmd[i] = 0x38; ++i; break;
#endif
        case S_TEMP : cmd[i] = 0x39; ++i; break;
        case S_FLAG : cmd[i] = 0x3A; ++i; break;
        case S_OFLAG: cmd[i] = 0x3B; ++i; break;
        case S_OAF  : cmd[i] = 0x3C; ++i; break;
        case S_PIN  : cmd[i] = 0x3D; ++i; break;
        default: break;
    }
    cmd[i] = 0x6B; ++i;
    ZDT_V5_SEND_CMD(cmd, i);
}

// /**
//   * @brief 批量读取系统状态参数
//   * @param addr 电机地址
//   */
// void ZDT_V5_Read_Batch_Status(uint8_t addr) {
//     uint8_t cmd[16] = {0};
//     cmd[0] = addr; cmd[1] = 0x43; cmd[2] = 0x7A; cmd[3] = 0x6B;
//     ZDT_V5_SEND_CMD(cmd, 4);
// }

/**
  * @brief 批量读取驱动配置参数
  * @param addr 电机地址
  */
void ZDT_V5_Read_Batch_Config(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x42; cmd[2] = 0x6C; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 读取选项参数状态
  * @param addr 电机地址
  */
void ZDT_V5_Read_Option_Params(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x1A; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 读取电机ID/地址
  * @param addr 电机地址
  */
void ZDT_V5_Read_Motor_ID(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x15; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/******************** 读写驱动参数命令 **********************/

#if MOTOR_DRIVER
/**
  * @brief 修改电机ID地址
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param id 默认电机ID为1,可修改为1-255,0为广播地址
  */
void ZDT_V5_Modify_Motor_ID(uint8_t addr, bool svF, uint8_t id) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xAE; cmd[2] = 0x4B; cmd[3] = svF; cmd[4] = id; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改电机细分值
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param mstep 细分值，如：1、2、4、8、16、32、64、128、256
  */
void ZDT_V5_Modify_MicroStep(uint8_t addr, bool svF, uint8_t mstep) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x84; cmd[2] = 0x8A; cmd[3] = svF; cmd[4] = mstep; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改断电标志位
  * @param addr 电机地址
  * @param pdf 是否掉电存储位置功能,false为禁止,true为使能
  */
void ZDT_V5_Modify_PDFlag(uint8_t addr, bool pdf) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x50; cmd[2] = 0x5E; cmd[3] = pdf; cmd[4] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 5);
}

/**
  * @brief 修改电机类型
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param mottype 电机类型,false为1.8°,true为0.9°
  */
void ZDT_V5_Modify_Motor_Type(uint8_t addr, bool svF, bool mottype) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xD7; cmd[2] = 0x23; cmd[3] = svF; cmd[4] = mottype; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改固件类型
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param fwtype 固件类型,false为X固件,true为Emm固件
  */
void ZDT_V5_Modify_Firmware_Type(uint8_t addr, bool svF, bool fwtype) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xD5; cmd[2] = 0x69; cmd[3] = svF; cmd[4] = fwtype; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改控制模式
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param ctrl_mode 控制模式,false为开环,true为闭环FOC
  */
void ZDT_V5_Modify_Ctrl_Mode(uint8_t addr, bool svF, bool ctrl_mode) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xD8; cmd[2] = 0xA6; cmd[3] = svF; cmd[4] = ctrl_mode; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改电机正方向
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param dir 电机正方向,0为CW，其他为CCW
  */
void ZDT_V5_Modify_Motor_Dir(uint8_t addr, bool svF, bool dir) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xD4; cmd[2] = 0x60; cmd[3] = svF; cmd[4] = dir; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改锁定按钮功能
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param lockbtn 锁定按钮状态
  */
void ZDT_V5_Modify_Lock_Btn(uint8_t addr, bool svF, bool lockbtn) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xD6; cmd[2] = 0x9E; cmd[3] = svF; cmd[4] = lockbtn; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改锁定参数
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param lock_level 锁定等级
  */
void ZDT_V5_Modify_Lock_Params(uint8_t addr, bool svF, uint8_t lock_level) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xD6; cmd[2] = 0x4B; cmd[3] = svF; cmd[4] = lock_level; cmd[5] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_CURRENT
/**
  * @brief 修改开环工作电流
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param om_ma 开环工作电流(mA) 0-5000
  */
void ZDT_V5_Modify_OM_mA(uint8_t addr, bool svF, uint16_t om_ma) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x44; cmd[2] = 0x48; cmd[3] = svF;
    cmd[4] = (uint8_t)(om_ma >> 8); cmd[5] = (uint8_t)(om_ma >> 0);
    cmd[6] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 7);
}

/**
  * @brief 修改闭环最大电流
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param foc_mA 闭环最大电流(mA) 0-5000
  */
void ZDT_V5_Modify_FOC_mA(uint8_t addr, bool svF, uint16_t foc_mA) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x45; cmd[2] = 0x66; cmd[3] = svF;
    cmd[4] = (uint8_t)(foc_mA >> 8); cmd[5] = (uint8_t)(foc_mA >> 0);
    cmd[6] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 7);
}
#endif

#if MOTOR_CONTROL
/**
  * @brief 读取PID参数
  * @param addr 电机地址
  */
void ZDT_V5_Read_PID_Params(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x21; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改PID参数
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param kp PID比例系数
  * @param ki PID积分系数
  * @param kd PID微分系数
  */
void ZDT_V5_Modify_PID_Params(uint8_t addr, bool svF, uint32_t kp, uint32_t ki, uint32_t kd) {
    uint8_t cmd[24] = {0};
    cmd[0] = addr; cmd[1] = 0x4A; cmd[2] = 0xC3; cmd[3] = svF;
    cmd[4] = (uint8_t)(kp >> 24); cmd[5] = (uint8_t)(kp >> 16); cmd[6] = (uint8_t)(kp >> 8); cmd[7] = (uint8_t)(kp >> 0);
    cmd[8] = (uint8_t)(ki >> 24); cmd[9] = (uint8_t)(ki >> 16); cmd[10] = (uint8_t)(ki >> 8); cmd[11] = (uint8_t)(ki >> 0);
    cmd[12] = (uint8_t)(kd >> 24); cmd[13] = (uint8_t)(kd >> 16); cmd[14] = (uint8_t)(kd >> 8); cmd[15] = (uint8_t)(kd >> 0);
    cmd[16] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 17);
}

/**
  * @brief 修改积分限幅/刚性系数
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param il 积分限幅值
  */
void ZDT_V5_Modify_Integral_Limit(uint8_t addr, bool svF, uint32_t il) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x4B; cmd[2] = 0x57; cmd[3] = svF;
    cmd[4] = (uint8_t)(il >> 24); cmd[5] = (uint8_t)(il >> 16); cmd[6] = (uint8_t)(il >> 8); cmd[7] = (uint8_t)(il >> 0);
    cmd[8] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 9);
}

/**
  * @brief 修改位置到达窗口
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param prw 位置到达窗口值
  */
void ZDT_V5_Modify_Pos_Window(uint8_t addr, bool svF, uint16_t prw) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xD1; cmd[2] = 0x07; cmd[3] = svF;
    cmd[4] = (uint8_t)(prw >> 8); cmd[5] = (uint8_t)(prw >> 0);
    cmd[6] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 7);
}

/**
  * @brief 读取位置到达窗口
  * @param addr 电机地址
  */
void ZDT_V5_Read_Pos_Window(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x41; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 读取积分限幅/刚性系数
  * @param addr 电机地址
  */
void ZDT_V5_Read_Integral_Limit(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x23; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}
#endif

#if MOTOR_PROTECTION
/**
  * @brief 读取过热过流保护检测阈值
  * @param addr 电机地址
  */
void ZDT_V5_Read_Otocp(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x13; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改过热过流保护阈值
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param otp 温度保护阈值(℃)
  * @param ocp 电流保护阈值(mA)
  * @param time_ms 保护触发时间(ms)
  */
void ZDT_V5_Modify_Otocp(uint8_t addr, bool svF, uint16_t otp, uint16_t ocp, uint16_t time_ms) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0xD3; cmd[2] = 0x56; cmd[3] = svF;
    cmd[4] = (uint8_t)(otp >> 8); cmd[5] = (uint8_t)(otp >> 0);
    cmd[6] = (uint8_t)(ocp >> 8); cmd[7] = (uint8_t)(ocp >> 0);
    cmd[8] = (uint8_t)(time_ms >> 8); cmd[9] = (uint8_t)(time_ms >> 0);
    cmd[10] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 11);
}

/**
  * @brief 读取心跳保护功能时间
  * @param addr 电机地址
  */
void ZDT_V5_Read_Heart_Protect(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x16; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改心跳保护时间
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param hp 心跳保护时间(ms)
  */
void ZDT_V5_Modify_Heart_Protect(uint8_t addr, bool svF, uint32_t hp) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x68; cmd[2] = 0x38; cmd[3] = svF;
    cmd[4] = (uint8_t)(hp >> 24); cmd[5] = (uint8_t)(hp >> 16); cmd[6] = (uint8_t)(hp >> 8); cmd[7] = (uint8_t)(hp >> 0);
    cmd[8] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 9);
}

/**
  * @brief 读取碰撞回零返回角度
  * @param addr 电机地址
  */
void ZDT_V5_Read_Collision_Angle(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x3F; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改碰撞回零返回角度
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param angle 碰撞回零返回角度
  */
void ZDT_V5_Modify_Collision_Angle(uint8_t addr, bool svF, uint16_t angle) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x5C; cmd[2] = 0xAC; cmd[3] = svF;
    cmd[4] = (uint8_t)(angle >> 8); cmd[5] = (uint8_t)(angle >> 0);
    cmd[6] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 7);
}
#elif USE_HEARTBEAT
/**
  * @brief 修改心跳保护时间
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param hp 心跳保护时间(ms)
  */
void ZDT_V5_Modify_Heart_Protect(uint8_t addr, bool svF, uint32_t hp) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x68; cmd[2] = 0x38; cmd[3] = svF;
    cmd[4] = (uint8_t)(hp >> 24); cmd[5] = (uint8_t)(hp >> 16); cmd[6] = (uint8_t)(hp >> 8); cmd[7] = (uint8_t)(hp >> 0);
    cmd[8] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 9);
}
#endif

#if MOTOR_COMM
/**
  * @brief 修改通讯参数
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param uart_baudrate 串口波特率编码 0-8
  * @param can_baudrate CAN波特率编码 0-8
  * @param verify_mode 校验方式 0-4
  * @param response_mode 应答方式 0-4
  */
void ZDT_V5_Modify_Comm_Params(uint8_t addr, bool svF, uint8_t uart_baudrate, uint8_t can_baudrate, uint8_t verify_mode, uint8_t response_mode) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x48; cmd[2] = 0xD1; cmd[3] = svF;
    cmd[4] = uart_baudrate; cmd[5] = can_baudrate; cmd[6] = verify_mode; cmd[7] = response_mode;
    cmd[8] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 9);
}
#endif

#if MOTOR_SYSTEM
/**
  * @brief 读取固件版本和硬件版本
  * @param addr 电机地址
  */
void ZDT_V5_Read_Version_Info(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x1F; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 读取相电阻和相电感
  * @param addr 电机地址
  */
void ZDT_V5_Read_Phase_Params(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x20; cmd[2] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 3);
}
#endif

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
/**
  * @brief 读取DMX512协议参数（Y42）
  * @param addr 电机地址
  */
void ZDT_V5_Read_DMX512_Params(uint8_t addr) {
    uint8_t cmd[16] = {0};
    cmd[0] = addr; cmd[1] = 0x49; cmd[2] = 0x78; cmd[3] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 修改DMX512协议参数（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志,false为不存储,true为存储
  * @param tch 目标通道
  * @param nch 通道数量
  * @param mode 控制模式 0-位置, 1-速度, 2-力矩
  * @param vel 速度(RPM)
  * @param acc 加速度
  * @param vel_step 速度步进
  * @param pos_step 位置步进
  */
void ZDT_V5_Modify_DMX512_Params(uint8_t addr, bool svF, uint16_t tch, uint8_t nch, uint8_t mode, uint16_t vel, uint16_t acc, uint16_t vel_step, uint32_t pos_step) {
    uint8_t cmd[32] = {0};
    cmd[0] = addr; cmd[1] = 0xD9; cmd[2] = svF;
    cmd[3] = (uint8_t)(tch >> 8); cmd[4] = (uint8_t)(tch >> 0);
    cmd[5] = nch; cmd[6] = mode;
    cmd[7] = (uint8_t)(vel >> 8); cmd[8] = (uint8_t)(vel >> 0);
    cmd[9] = (uint8_t)(acc >> 8); cmd[10] = (uint8_t)(acc >> 0);
    cmd[11] = (uint8_t)(vel_step >> 8); cmd[12] = (uint8_t)(vel_step >> 0);
    cmd[13] = (uint8_t)(pos_step >> 24); cmd[14] = (uint8_t)(pos_step >> 16);
    cmd[15] = (uint8_t)(pos_step >> 8); cmd[16] = (uint8_t)(pos_step >> 0);
    cmd[17] = 0x6B;
    ZDT_V5_SEND_CMD(cmd, 18);
}
#endif
