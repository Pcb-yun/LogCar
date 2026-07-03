/**
 * @file zdt_v5_port.h
 * @brief 张大头V5步进电机移植接口
 *
 * zdt-v5-driver - 张大头V5步进电机通用驱动
 * Copyright (C) 2024-2026  Pcb-yun
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * 将本文件复制到工程中，并改名为 zdt_v5_port.h
 */

#ifndef __ZDT_V5_PORT_H__
#define __ZDT_V5_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "zdt_v5_cmd.h"
#include "log.h"


#define USE_VIEW 0

void zdt_v5_port_send(uint8_t *cmd, uint8_t len);
bool Motor_Send_Cmd(MotorCmd_t *cmd);

/**
 * @brief 日志输出宏（可选）
 */
#define ZDT_V5_LOG(fmt, ...) logPrintln(fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ZDT_V5_PORT_H__ */
