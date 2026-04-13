/**
 * @file Emm_V5_Driver.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机驱动层源文件
 */

#include "Emm_V5_Driver.h"

/******************** 触发动作指令 **********************/

/**
  * @brief 触发编码器校准
  * @param addr 电机地址
  */
void Emm_V5_Trig_Encoder_Cal(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  //       地址           功能码         辅助码         校验字节
  cmd[0] = addr; cmd[1] = 0x06; cmd[2] = 0x45; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 重启电机（Y42）
  * @param addr 电机地址
  */
void Emm_V5_Reset_Motor(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  //       地址           功能码         辅助码         校验字节
  cmd[0] = addr; cmd[1] = 0x08; cmd[2] = 0x97; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}
/**
  * @brief 将当前位置清零
  * @param addr 电机地址
  */
void Emm_V5_Reset_CurPos_To_Zero(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  //       地址           功能码         辅助码         校验字节
  cmd[0] = addr; cmd[1] = 0x0A; cmd[2] = 0x6D; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 解除堵转保护
  * @param addr 电机地址
  */
void Emm_V5_Reset_Clog_Pro(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  //       地址           功能码         辅助码         校验字节
  cmd[0] = addr; cmd[1] = 0x0E; cmd[2] = 0x52; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 恢复出厂设置
  * @param addr 电机地址
  */
void Emm_V5_Restore_Motor(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  //       地址           功能码         辅助码         校验字节
  cmd[0] = addr; cmd[1] = 0x0F; cmd[2] = 0x5F; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/******************** 运动控制指令 **********************/

/**
  * @brief 多电机命令（Y42）
  * @param addr 电机地址
  */
void Emm_V5_Multi_Motor_Cmd(uint8_t addr) {
    __IO static uint8_t cmd[MMCL_LEN] = {0};
    uint16_t i = 0, j = 0, len = 0;

	if(MMCL_count > 0) {
		len = MMCL_count + 5;
        //       地址           功能码              总字节高8位                  总字节低8位
        cmd[0] = addr; cmd[1] = 0xAA; cmd[2] = (uint8_t)(len >> 8); cmd[3] = (uint8_t)(len);
		for(i=0,j=4; i < MMCL_count; i++,j++) { cmd[j] = MMCL_cmd[i]; }
		cmd[j] = 0x6B; ++j;                  // 校验字节
        EMM_V5_SEND_CMD(cmd, j);
		MMCL_count = 0;
	} else {
		MMCL_count = 0;
	}
}

/**
  * @brief 使能信号控制
  * @param addr 电机地址
  * @param state 使能状态,true为使能电机,false为关闭电机
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void Emm_V5_En_Control(uint8_t addr, bool state, bool snF) {
  __IO static uint8_t cmd[16] = {0};
  //       地址           功能码         辅助码              使能状态      多机同步标志          校验字节
  cmd[0] = addr; cmd[1] = 0xF3; cmd[2] = 0xAB; cmd[3] = (uint8_t)state; cmd[4] = snF; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 速度模式
  * @param addr 电机地址
  * @param dir 方向,0为CW，其余值为CCW
  * @param vel 速度(RPM) 0-5000
  * @param acc 加速度 0-255, 0为直接启动
  * @param snF 多机同步标志,false为不启用,true为启用
  */
void Emm_V5_Vel_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, bool snF) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xF6; cmd[2] = dir; cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0); cmd[5] = acc; cmd[6] = snF; cmd[7] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 8);
}

/**
  * @brief 位置模式
  * @param addr 电机地址
  * @param dir 方向        ，0为CW，其余值为CCW
  * @param vel 速度(RPM)   ，范围0 - 5000RPM
  * @param acc 加速度      ，范围0 - 255，注意 0是直接启动
  * @param clk 脉冲数      ，范围0- (2^32 - 1)个
  * @param raF 相位/绝对标志，false为相对运动，true为绝对值运动
  * @param snF 多机同步标志 ，false为不启用，true为启用
  */
void Emm_V5_Pos_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, uint32_t clk, bool raF, bool snF) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xFD; cmd[2] = dir; cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0); cmd[5] = acc; cmd[6] = (uint8_t)(clk >> 24);
  cmd[7] = (uint8_t)(clk >> 16); cmd[8] = (uint8_t)(clk >> 8); cmd[9] = (uint8_t)(clk >> 0); cmd[10] = raF; cmd[11] = snF; cmd[12] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 13);
}

/**
  * @brief 立即停止
  * @param addr 电机地址
  * @param snF 多机同步标志，false为不启用，true为启用
 */
void Emm_V5_Stop_Now(uint8_t addr, bool snF) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xFE; cmd[2] = 0x98; cmd[3] = snF; cmd[4] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 5);
}

/**
  * @brief 多机同步运动
  * @param addr 电机地址
  */
void Emm_V5_Synchronous_motion(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xFF; cmd[2] = 0x66; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/******************** 回零点指令 **********************/

/**
  * @brief 设置单圈回零的零点位置
  * @param addr 电机地址
  * @param svF 存储标志，false为不存储，true为存储
  */
void Emm_V5_Origin_Set_O(uint8_t addr, bool svF) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x93; cmd[2] = 0x88; cmd[3] = svF; cmd[4] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 5);
}

/**
  * @brief 触发回零
  * @param addr 电机地址
  * @param o_mode 回零模式，0为单圈就近回零，1为单圈方向回零，2为多圈无限位碰撞回零，3为多圈有限位开关回零
  * @param snF 多机同步标志，false为不启用，true为启用
  */
void Emm_V5_Origin_Trigger_Return(uint8_t addr, uint8_t o_mode, bool snF) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x9A; cmd[2] = o_mode; cmd[3] = snF; cmd[4] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 5);
}

/**
  * @brief 强制中断并退出回零
  * @param addr 电机地址
  */
void Emm_V5_Origin_Interrupt(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x9C; cmd[2] = 0x48; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 读取回零参数
  * @param addr 电机地址
  */
void Emm_V5_Origin_Read_Params(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x22; cmd[2] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改回零参数
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param o_mode 回零模式，0为单圈就近回零，1为单圈方向回零，2为多圈无限位碰撞回零，3为多圈有限位开关回零
  * @param o_dir 回零方向，0为CW，其余值为CCW
  * @param o_vel 回零速度，单位：RPM（转/分钟）
  * @param o_tm 回零超时时间，单位：毫秒
  * @param sl_vel 无限位碰撞回零检测转速，单位：RPM（转/分钟）
  * @param sl_ma 无限位碰撞回零检测电流，单位：Ma（毫安）
  * @param sl_ms 无限位碰撞回零检测时间，单位：Ms（毫秒）
  * @param potF 上电自动触发回零，false为不使能，true为使能
  */
void Emm_V5_Origin_Modify_Params(uint8_t addr, bool svF, uint8_t o_mode, uint8_t o_dir, uint16_t o_vel, uint32_t o_tm, uint16_t sl_vel, uint16_t sl_ma, uint16_t sl_ms, bool potF) {
  __IO static uint8_t cmd[32] = {0};
  cmd[0] = addr; cmd[1] = 0x4C; cmd[2] = 0xAE; cmd[3] = svF; cmd[4] = o_mode; cmd[5] = o_dir;
  cmd[6] = (uint8_t)(o_vel >> 8); cmd[7] = (uint8_t)(o_vel >> 0);
  cmd[8] = (uint8_t)(o_tm >> 24); cmd[9] = (uint8_t)(o_tm >> 16); cmd[10] = (uint8_t)(o_tm >> 8); cmd[11] = (uint8_t)(o_tm >> 0);
  cmd[12] = (uint8_t)(sl_vel >> 8); cmd[13] = (uint8_t)(sl_vel >> 0);
  cmd[14] = (uint8_t)(sl_ma >> 8); cmd[15] = (uint8_t)(sl_ma >> 0);
  cmd[16] = (uint8_t)(sl_ms >> 8); cmd[17] = (uint8_t)(sl_ms >> 0);
  cmd[18] = potF; cmd[19] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 20);
}

/**********************************************************
*** 读取系统参数命令
**********************************************************/
/**
  * @brief 定时返回信息命令（Y42）
  * @param addr 电机地址
  * @param s 系统参数类型
  * @param time_ms 定时时间
  */
void Emm_V5_Auto_Return_Sys_Params_Timed(uint8_t addr, SysParams_t s, uint16_t time_ms) {
  uint8_t i = 0; __IO static uint8_t cmd[16] = {0};
  cmd[i] = addr; ++i;
  cmd[i] = 0x11; ++i;
  cmd[i] = 0x18; ++i;
  switch(s) {
    case S_VBUS : cmd[i] = 0x24; ++i; break;
    case S_CBUS : cmd[i] = 0x26; ++i; break;
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
    case S_VBAT : cmd[i] = 0x38; ++i; break;
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
  EMM_V5_SEND_CMD(cmd, i);
}

/**
  * @brief 读取系统参数
  * @param addr 电机地址
  * @param s 系统参数类型
  */
void Emm_V5_Read_Sys_Params(uint8_t addr, SysParams_t s) {
  uint8_t i = 0; __IO static uint8_t cmd[16] = {0};
  cmd[i] = addr; ++i;
  switch(s) {
    case S_VBUS : cmd[i] = 0x24; ++i; break;
    case S_CBUS : cmd[i] = 0x26; ++i; break;
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
    case S_VBAT : cmd[i] = 0x38; ++i; break;
    case S_TEMP : cmd[i] = 0x39; ++i; break;
    case S_FLAG : cmd[i] = 0x3A; ++i; break;
    case S_OFLAG: cmd[i] = 0x3B; ++i; break;
    case S_OAF  : cmd[i] = 0x3C; ++i; break;
    case S_PIN  : cmd[i] = 0x3D; ++i; break;
    default: break;
  }
  cmd[i] = 0x6B; ++i;
  EMM_V5_SEND_CMD(cmd, i);
}

/**********************************************************
*** 读写驱动参数命令
**********************************************************/
/**
  * @brief 修改电机ID地址
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param id 默认电机ID为1，可修改为1-255，0为广播地址
  */
void Emm_V5_Modify_Motor_ID(uint8_t addr, bool svF, uint8_t id) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xAE; cmd[2] = 0x4B; cmd[3] = svF; cmd[4] = id; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改细分值
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param mstep 默认细分为16，可修改为1-2556，0为256细分
  */
void Emm_V5_Modify_MicroStep(uint8_t addr, bool svF, uint8_t mstep) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x84; cmd[2] = 0x8A; cmd[3] = svF; cmd[4] = mstep; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改掉电标志
  * @param addr 电机地址
  * @param pdf 掉电标志
  */
void Emm_V5_Modify_PDFlag(uint8_t addr, bool pdf) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x50; cmd[2] = pdf; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 读取选项参数状态（Y42）
  * @param addr 电机地址
  */
void Emm_V5_Read_Opt_Param_Sta(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x1A; cmd[2] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改电机类型（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param mottype 电机类型，默认为0，0表示1.8°步进电机，1表示0.9°步进电机
  */
void Emm_V5_Modify_Motor_Type(uint8_t addr, bool svF, bool mottype) {
  __IO static uint8_t cmd[16] = {0}; uint8_t MotType = 0;
  if(mottype) { MotType = 25; } else { MotType = 50; }
  cmd[0] = addr; cmd[1] = 0xD7; cmd[2] = 0x35; cmd[3] = svF; cmd[4] = MotType; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改固件类型（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param fwtype 固件类型，默认为0，0为X固件，1为Emm固件
  */
void Emm_V5_Modify_Firmware_Type(uint8_t addr, bool svF, bool fwtype) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xD5; cmd[2] = 0x69; cmd[3] = svF; cmd[4] = fwtype; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改开环/闭环控制模式（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param ctrl_mode 控制模式，默认为1,0为开环模式，1为闭环FOC模式
  */
void Emm_V5_Modify_Ctrl_Mode(uint8_t addr, bool svF, bool ctrl_mode) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x46; cmd[2] = 0x69; cmd[3] = svF; cmd[4] = ctrl_mode; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改电机运动正方向（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param dir 电机运动正方向，默认为CW，0为CW（顺时针方向），1为CCW
  */
void Emm_V5_Modify_Motor_Dir(uint8_t addr, bool svF, bool dir) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xD4; cmd[2] = 0x60; cmd[3] = svF; cmd[4] = dir; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改锁定按键功能（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param lock 锁定按键功能，默认为Disable，0为Disable，1为Enable
  */
void Emm_V5_Modify_Lock_Btn(uint8_t addr, bool svF, bool lock) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xD0; cmd[2] = 0xB3; cmd[3] = svF; cmd[4] = lock; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改命令速度值是否缩小10倍输入（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param s_vel 命令速度值是否缩小10倍输入，默认为Disable，0为Disable，1为Enable
  */
void Emm_V5_Modify_S_Vel(uint8_t addr, bool svF, bool s_vel) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x4F; cmd[2] = 0x71; cmd[3] = svF; cmd[4] = s_vel; cmd[5] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 6);
}

/**
  * @brief 修改开环模式工作电流
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param om_ma 开环模式工作电流，单位mA
  */
void Emm_V5_Modify_OM_mA(uint8_t addr, bool svF, uint16_t om_ma) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x44; cmd[2] = 0x33; cmd[3] = svF; cmd[4] = (uint8_t)(om_ma >> 8); cmd[5] = (uint8_t)(om_ma >> 0); cmd[6] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 7);
}

/**
  * @brief 修改闭环模式最大电流
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param foc_mA 闭环模式最大电流，单位mA
  */
void Emm_V5_Modify_FOC_mA(uint8_t addr, bool svF, uint16_t foc_mA) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x45; cmd[2] = 0x66; cmd[3] = svF; cmd[4] = (uint8_t)(foc_mA >> 8); cmd[5] = (uint8_t)(foc_mA >> 0); cmd[6] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 7);
}

/**
  * @brief 读取PID参数
  * @param addr 电机地址
  */
void Emm_V5_Read_PID_Params(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x21; cmd[2] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改PID参数
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param kp 比例系数，默认为Y42/18000
  * @param ki 积分系数，默认为Y42/10
  * @param kd 微分系数，默认为Y42/18000
  */
void Emm_V5_Modify_PID_Params(uint8_t addr, bool svF, uint32_t kp, uint32_t ki, uint32_t kd) {
  __IO static uint8_t cmd[20] = {0};
  cmd[0] = addr; cmd[1] = 0x4A; cmd[2] = 0xC3; cmd[3] = svF;
  cmd[4] = (uint8_t)(kp >> 24); cmd[5] = (uint8_t)(kp >> 16); cmd[6] = (uint8_t)(kp >> 8); cmd[7] = (uint8_t)(kp >> 0);
  cmd[8] = (uint8_t)(ki >> 24); cmd[9] = (uint8_t)(ki >> 16); cmd[10] = (uint8_t)(ki >> 8); cmd[11] = (uint8_t)(ki >> 0);
  cmd[12] = (uint8_t)(kd >> 24); cmd[13] = (uint8_t)(kd >> 16); cmd[14] = (uint8_t)(kd >> 8); cmd[15] = (uint8_t)(kd >> 0);
  cmd[16] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 17);
}

/**
  * @brief 读取DMX512协议参数（Y42）
  * @param addr 电机地址
  */
void Emm_V5_Read_DMX512_Params(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x49; cmd[2] = 0x78; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 修改DMX512协议参数（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param tch 总通道数，默认为192，该值要与自身 DMX512 控制器的总通道数一样
  * @param nch 每个电机占用的通道数，默认为1，1为单通道模式,2为双通道模式
  * @param mode 运动模式，默认为1，0表示相对位置模式运动，1表示绝对坐标式位置运动
  * @param vel 单通道模式的运动速度，默认值为1000， 单位RPM， 即1000RPM
  * @param acc 加速度，acc=加速数值/8=125，加速时间见说明书“5.3.12 位置模式控制（Emm）”
  * @param vel_step 双通道模式速度步长，默认值为 10， 即电机运动速度为(通道值 * 10)RPM
  * @param pos_step 双通道模式运动步长，默认值为 100， 即电机转动角度为(通道值 * 10.0)°
  */
void Emm_V5_Modify_DMX512_Params(uint8_t addr, bool svF, uint16_t tch, uint8_t nch, uint8_t mode, uint16_t vel, uint16_t acc, uint16_t vel_step, uint32_t pos_step) {
  __IO static uint8_t cmd[32] = {0};
  cmd[0] = addr; cmd[1] = 0xD9; cmd[2] = 0x90; cmd[3] = svF;
  cmd[4] = (uint8_t)(tch >> 8); cmd[5] = (uint8_t)(tch >> 0);
  cmd[6] = nch; cmd[7] = mode;
  cmd[8] = (uint8_t)(vel >> 8); cmd[9] = (uint8_t)(vel >> 0);
  cmd[10] = (uint8_t)(acc >> 8); cmd[11] = (uint8_t)(acc >> 0);
  cmd[12] = (uint8_t)(vel_step >> 8); cmd[13] = (uint8_t)(vel_step >> 0);
  cmd[14] = (uint8_t)(pos_step >> 24); cmd[15] = (uint8_t)(pos_step >> 16);
  cmd[16] = (uint8_t)(pos_step >> 8); cmd[17] = (uint8_t)(pos_step >> 0);
  cmd[18] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 19);
}

/**
  * @brief 读取位置到达窗口（Y42）
  * @param addr 电机地址
  */
void Emm_V5_Read_Pos_Window(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x41; cmd[2] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改位置到达窗口（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param prw 位置到达窗口，默认值为8，即0.8°
  */
void Emm_V5_Modify_Pos_Window(uint8_t addr, bool svF, uint16_t prw) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xD1; cmd[2] = 0x07; cmd[3] = svF; cmd[4] = (uint8_t)(prw >> 8); cmd[5] = (uint8_t)(prw >> 0); cmd[6] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 7);
}

/**
  * @brief 读取过热过流保护检测阈值（Y42）
  * @param addr 电机地址
  */
void Emm_V5_Read_Otocp(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x13; cmd[2] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改过热过流保护检测阈值（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param otp 过热保护检测阈值，默认100℃
  * @param ocp 过流保护检测阈值，默认6600mA
  * @param time_ms 过热过流检测时间，默认1000ms
  */
void Emm_V5_Modify_Otocp(uint8_t addr, bool svF, uint16_t otp, uint16_t ocp, uint16_t time_ms) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0xD3; cmd[2] = 0x56; cmd[3] = svF;
  cmd[4] = (uint8_t)(otp >> 8); cmd[5] = (uint8_t)(otp >> 0);
  cmd[6] = (uint8_t)(ocp >> 8); cmd[7] = (uint8_t)(ocp >> 0);
  cmd[8] = (uint8_t)(time_ms >> 8); cmd[9] = (uint8_t)(time_ms >> 0);
  cmd[10] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 11);
}

/**
  * @brief 读取心跳保护功能时间（Y42）
  * @param addr 电机地址
  */
void Emm_V5_Read_Heart_Protect(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x16; cmd[2] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改心跳保护功能时间（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param hp 心跳保护时间，单位：ms
  */
void Emm_V5_Modify_Heart_Protect(uint8_t addr, bool svF, uint32_t hp) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x68; cmd[2] = 0x38; cmd[3] = svF;
  cmd[4] = (uint8_t)(hp >> 24); cmd[5] = (uint8_t)(hp >> 16); cmd[6] = (uint8_t)(hp >> 8); cmd[7] = (uint8_t)(hp >> 0);
  cmd[8] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 9);
}

/**
  * @brief 读取积分限幅/刚性系数（Y42）
  * @param addr 电机地址
  */
void Emm_V5_Read_Integral_Limit(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x23; cmd[2] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 3);
}

/**
  * @brief 修改积分限幅/刚性系数（Y42）
  * @param addr 电机地址
  * @param svF 是否存储标志，false为不存储，true为存储
  * @param il 积分限幅，默认值为65535
  */
void Emm_V5_Modify_Integral_Limit(uint8_t addr, bool svF, uint32_t il) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x4B; cmd[2] = 0x57; cmd[3] = svF;
  cmd[4] = (uint8_t)(il >> 24); cmd[5] = (uint8_t)(il >> 16); cmd[6] = (uint8_t)(il >> 8); cmd[7] = (uint8_t)(il >> 0);
  cmd[8] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 9);
}

/**********************************************************
*** 读取所有驱动参数命令
**********************************************************/
/**
  * @brief 读取系统状态参数
  * @param addr 电机地址
  */
void Emm_V5_Read_System_State_Params(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x43; cmd[2] = 0x7A; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}

/**
  * @brief 读取驱动配置参数
  * @param addr 电机地址
  */
void Emm_V5_Read_Motor_Conf_Params(uint8_t addr) {
  __IO static uint8_t cmd[16] = {0};
  cmd[0] = addr; cmd[1] = 0x42; cmd[2] = 0x6C; cmd[3] = 0x6B;
  EMM_V5_SEND_CMD(cmd, 4);
}
