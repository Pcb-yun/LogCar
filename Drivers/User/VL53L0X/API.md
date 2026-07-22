# VL53L0X 官方API函数详解

## 概述

本文档仅介绍供用户使用的公开API函数，忽略内部实现细节。所有函数均以 `VL53L0X_ERROR` 作为返回值，表示执行状态。

**返回值说明**：

- `VL53L0X_ERROR_NONE`：执行成功
- 其他值：执行失败，具体错误类型见 `vl53l0x_def.h`

***

## 一、设备初始化

### 1.1 VL53L0X\_SetDeviceAddress

**作用**：设置设备I2C地址

**参数**：

| 参数              | 类型            | 说明            |
| --------------- | ------------- | ------------- |
| `Dev`           | `VL53L0X_DEV` | 设备句柄          |
| `DeviceAddress` | `uint8_t`     | 新的I2C地址（8位格式） |

**使用方法**：

```c
// 将设备地址改为0x54（8位格式）
VL53L0X_SetDeviceAddress(&vl53l0x_dev, 0x54);
```

**注意**：多设备场景下，必须通过XSHUT引脚控制逐个上电后调用此函数。

***

### 1.2 VL53L0X\_DataInit

**作用**：数据初始化，加载芯片NVM中的校准数据，完成设备的基础配置

**参数**：

| 参数    | 类型            | 说明   |
| ----- | ------------- | ---- |
| `Dev` | `VL53L0X_DEV` | 设备句柄 |

**使用方法**：

```c
// 上电后必须调用一次
VL53L0X_DataInit(&vl53l0x_dev);
```

**硬件交互原理**：

调用此函数时，芯片内部会发生以下操作：

1. **I2C总线电压配置**：根据编译选项设置I2C SCL/SDA引脚的供电电压（1.8V或2.8V）

2. **芯片解锁序列**：
   - 写入寄存器 `0x80` = `0x01`（进入解锁状态）
   - 写入寄存器 `0xFF` = `0x01`（选择特殊功能页）
   - 写入寄存器 `0x00` = `0x00`（复位特殊功能）
   - 读取寄存器 `0x91`（获取停止变量）
   - 写入寄存器 `0x00` = `0x01`（退出复位）
   - 写入寄存器 `0xFF` = `0x00`（退出特殊功能页）
   - 写入寄存器 `0x80` = `0x00`（退出解锁状态）

3. **默认参数初始化**：
   - 线性校正增益（LinearityCorrectiveGain）设为1000（无校正）
   - Dmax校准参数：默认400mm距离，1.42 MCPS信号率
   - 振荡器频率：默认9.44MHz
   - XTalk补偿率：设为0（禁用）
   - Sigma估算参数：设置参考数组、脉冲宽度、目标参考率

4. **限制检查配置**：
   - 启用所有限制检查（8项）
   - 禁用部分检查：信号参考裁剪、范围忽略阈值、MSRC信号率、预测距信号率
   - 设置默认阈值：Sigma最终范围18mm、信号率0.25 MCPS

5. **序列配置**：
   - 设置系统序列配置寄存器为0xFF（启用所有序列步骤）
   - 将PAL状态设为 `WAIT_STATICINIT`（等待静态初始化）

**注意**：

- 必须在设备复位后调用
- 如果无法执行硬件复位，需手动保存和恢复偏移校准数据
- 此函数不访问NVM，NVM数据在StaticInit中加载

***

### 1.3 VL53L0X\_StaticInit

**作用**：静态初始化，加载NVM数据、应用调谐参数、配置SPAD阵列、设置GPIO中断，完成设备的完全初始化

**参数**：

| 参数    | 类型            | 说明   |
| ----- | ------------- | ---- |
| `Dev` | `VL53L0X_DEV` | 设备句柄 |

**使用方法**：

```c
// 在DataInit之后调用
VL53L0X_StaticInit(&vl53l0x_dev);
```

**硬件交互原理**：

调用此函数时，芯片内部会发生以下操作：

1. **从设备获取信息**：读取设备的NVM数据，获取芯片ID、产品版本等信息

2. **SPAD阵列配置**：
   - 从NVM读取参考SPAD数量和类型（孔径SPAD或标准SPAD）
   - 验证NVM值的有效性：孔径SPAD数量≤32，标准SPAD数量≤12
   - 如果NVM值无效，执行SPAD管理（自动检测最佳SPAD配置）
   - 如果NVM值有效，直接应用参考SPAD配置

3. **加载调谐参数**：
   - 使用内部默认调谐参数（`DefaultTuningSettings`数组）
   - 将调谐参数写入芯片的相关寄存器

4. **GPIO中断配置**：
   - 配置GPIO引脚为"新采样就绪"功能
   - 设置中断极性为低电平有效

5. **读取振荡器频率**：
   - 进入特殊功能页（`0xFF` = `0x01`）
   - 读取寄存器 `0x84` 获取振荡器频率
   - 退出特殊功能页
   - 将频率值保存到PAL内部数据结构

6. **更新设备参数**：
   - 获取当前设备参数（包括分数启用状态）
   - 读取序列配置寄存器并保存

7. **禁用部分序列步骤**：
   - 禁用TCC（Target Centre Check）序列步骤
   - 禁用MSRC（Minimum Signal Rate Check）序列步骤

8. **保存关键参数**：
   - 保存预测距VCSEL脉冲周期
   - 保存最终测距VCSEL脉冲周期
   - 保存预测距超时时间
   - 保存最终测距超时时间

9. **状态转换**：将PAL状态设为 `IDLE`（空闲状态），设备准备就绪可进行测量

**原理说明**：

- **SPAD（Single Photon Avalanche Diode）**：单光子雪崩二极管，是VL53L0X的核心感光元件。SPAD阵列的配置直接影响测量的灵敏度和精度。孔径SPAD（Aperture SPAD）用于长距离测量，标准SPAD用于短距离测量。

- **调谐参数（Tuning Settings）**：包含芯片出厂时校准的各种参数，如VCSEL驱动电流、积分时间、增益等。这些参数对测量精度至关重要。

- **振荡器频率**：芯片内部振荡器的实际频率，用于计算测量时间和距离。不同芯片的振荡器频率可能略有差异，需要校准。

**注意**：此函数会将设备状态从 `WAIT_STATICINIT` 变为 `IDLE`。

***

### 1.4 VL53L0X\_ResetDevice

**作用**：复位设备

**参数**：

| 参数    | 类型            | 说明   |
| ----- | ------------- | ---- |
| `Dev` | `VL53L0X_DEV` | 设备句柄 |

**使用方法**：

```c
// 复位设备到初始状态
VL53L0X_ResetDevice(&vl53l0x_dev);
```

**注意**：复位后需要重新调用 `DataInit` 和 `StaticInit`。

***

## 二、校准函数

### 2.1 VL53L0X\_PerformRefCalibration

**作用**：执行参考校准，包括VHV（垂直高压）校准和相位校准，确保测量精度

**参数**：

| 参数             | 类型            | 说明         |
| -------------- | ------------- | ---------- |
| `Dev`          | `VL53L0X_DEV` | 设备句柄       |
| `pVhvSettings` | `uint8_t*`    | 输出：VHV设置参数 |
| `pPhaseCal`    | `uint8_t*`    | 输出：相位校准参数  |

**使用方法**：

```c
uint8_t vhv_settings, phase_cal;
VL53L0X_PerformRefCalibration(&vl53l0x_dev, &vhv_settings, &phase_cal);
```

**硬件交互原理**：

调用此函数时，芯片内部会发生以下操作：

1. **保存当前序列配置**：将系统序列配置寄存器的值保存到临时变量

2. **执行VHV校准**（Vertical High Voltage Calibration）：
   - VHV校准用于确定VCSEL（垂直腔面发射激光器）的最佳驱动电压
   - 芯片会自动调整VHV电平，使VCSEL发射功率达到最佳状态
   - 通过多次测量找到合适的VHV设置值
   - 将计算得到的VHV设置值写入芯片寄存器

3. **执行相位校准**（Phase Calibration）：
   - 相位校准用于校准内部时钟和测量时序之间的相位关系
   - 确保TOF（飞行时间）测量的时间精度
   - 通过测量参考信号的相位偏移，计算相位校准值
   - 将相位校准值写入芯片寄存器

4. **恢复序列配置**：将之前保存的序列配置恢复到寄存器

**原理说明**：

- **VHV（Vertical High Voltage）**：VCSEL的驱动电压。VL53L0X使用VCSEL发射激光脉冲，VHV电平直接影响激光功率和测量范围。不同环境温度和芯片个体差异会影响最佳VHV值，因此需要定期校准。

- **相位校准**：TOF测量依赖精确的时间测量。芯片内部的时钟信号与实际测量时序之间可能存在相位偏移，相位校准可以消除这种偏移，提高测量精度。

- **校准时机**：建议在以下情况下执行参考校准：
  - 设备首次上电后
  - 环境温度变化较大时（温度每变化10°C建议重新校准）
  - 测量精度下降时

**注意**：

- 此函数是阻塞函数，会等待校准完成
- 会触发一次特殊的测距测量序列
- 校准结果（VHV设置和相位校准值）会自动保存到芯片内部，无需用户手动处理

***

### 2.2 VL53L0X\_PerformRefSpadManagement

**作用**：参考SPAD管理，检测并配置最佳的SPAD阵列，确保测量灵敏度和精度

**参数**：

| 参数                 | 类型            | 说明           |
| ------------------ | ------------- | ------------ |
| `Dev`              | `VL53L0X_DEV` | 设备句柄         |
| `pRefSpadCount`    | `uint32_t*`   | 输出：参考SPAD数量  |
| `pIsApertureSpads` | `uint8_t*`    | 输出：是否为孔径SPAD |

**使用方法**：

```c
uint32_t ref_spad_count;
uint8_t is_aperture_spads;
VL53L0X_PerformRefSpadManagement(&vl53l0x_dev, &ref_spad_count, &is_aperture_spads);
```

**硬件交互原理**：

调用此函数时，芯片内部会发生以下操作：

1. **检测可用SPAD**：扫描芯片内部的SPAD阵列，识别哪些SPAD是可用的（未损坏、性能良好）

2. **启用最小SPAD数量**：
   - 首先启用最小数量的非孔径SPAD（标准SPAD）
   - 执行参考信号测量，检查信号率是否达到目标值

3. **评估是否需要孔径SPAD**：
   - 如果标准SPAD的信号率过高（超过目标值），说明当前环境光太强或反射太强
   - 切换到孔径SPAD（Aperture SPAD），孔径SPAD具有更小的接收角度，能减少环境光影响

4. **逐步增加SPAD数量**：
   - 如果信号率低于目标值，逐步增加SPAD数量
   - 每次增加后执行参考信号测量
   - 直到信号率达到目标值或达到最大SPAD数量

5. **保存配置**：将最终的SPAD配置（数量和类型）保存到设备参数中

**原理说明**：

- **SPAD（Single Photon Avalanche Diode）**：单光子雪崩二极管，是VL53L0X的核心感光元件。当单个光子撞击SPAD时，会触发雪崩效应，产生可检测的电信号。

- **标准SPAD vs 孔径SPAD**：
  - **标准SPAD**：接收角度较大，灵敏度较高，适合短距离测量和低反射率目标
  - **孔径SPAD**：通过物理孔径限制接收角度，减少环境光和串扰，适合长距离测量和高反射率目标

- **SPAD数量的影响**：
  - 更多的SPAD可以提供更强的信号，提高测量灵敏度
  - 但过多的SPAD会增加噪声，降低精度
  - 需要根据实际环境和目标特性找到最佳平衡点

- **目标信号率（Target Ref Rate）**：默认20 MCPS（百万计数/秒）。参考校准的目标是使参考信号率接近这个值。

**注意**：此函数是阻塞函数。

***

### 2.3 VL53L0X\_PerformXTalkMeasurement

**作用**：执行串扰测量

**参数**：

| 参数                | 类型                | 说明                |
| ----------------- | ----------------- | ----------------- |
| `Dev`             | `VL53L0X_DEV`     | 设备句柄              |
| `TimeoutMs`       | `uint32_t`        | 测量超时时间（毫秒）        |
| `pXtalkPerSpad`   | `FixPoint1616_t*` | 输出：串扰值（MCPS/SPAD） |
| `pAmbientTooHigh` | `uint8_t*`        | 输出：环境光是否过高        |

**使用方法**：

```c
FixPoint1616_t xtalk;
uint8_t ambient_too_high;
VL53L0X_PerformXTalkMeasurement(&vl53l0x_dev, 1000, &xtalk, &ambient_too_high);
```

**注意**：

- 传感器前方必须没有目标
- 此函数是阻塞函数

***

### 2.4 VL53L0X\_PerformXTalkCalibration

**作用**：执行串扰校准

**参数**：

| 参数                              | 类型                | 说明     |
| ------------------------------- | ----------------- | ------ |
| `Dev`                           | `VL53L0X_DEV`     | 设备句柄   |
| `XTalkCalDistance`              | `FixPoint1616_t`  | 校准距离   |
| `pXTalkCompensationRateMegaCps` | `FixPoint1616_t*` | 输出：补偿率 |

**使用方法**：

```c
FixPoint1616_t compensation_rate;
VL53L0X_PerformXTalkCalibration(&vl53l0x_dev, (FixPoint1616_t)(100*65536), &compensation_rate);
```

**注意**：此函数会将设备模式改为 `SINGLE_RANGING`。

***

### 2.5 VL53L0X\_PerformOffsetCalibration

**作用**：执行偏移校准

**参数**：

| 参数                      | 类型               | 说明         |
| ----------------------- | ---------------- | ---------- |
| `Dev`                   | `VL53L0X_DEV`    | 设备句柄       |
| `CalDistanceMilliMeter` | `FixPoint1616_t` | 校准距离（毫米）   |
| `pOffsetMicroMeter`     | `int32_t*`       | 输出：偏移值（微米） |

**使用方法**：

```c
int32_t offset;
VL53L0X_PerformOffsetCalibration(&vl53l0x_dev, (FixPoint1616_t)(100*65536), &offset);
```

**注意**：此函数不改变设备模式。

***

### 2.6 VL53L0X\_SetOffsetCalibrationDataMicroMeter

**作用**：设置偏移校准数据

**参数**：

| 参数                                | 类型            | 说明      |
| --------------------------------- | ------------- | ------- |
| `Dev`                             | `VL53L0X_DEV` | 设备句柄    |
| `OffsetCalibrationDataMicroMeter` | `int32_t`     | 偏移值（微米） |

**使用方法**：

```c
// 设置偏移校准数据
VL53L0X_SetOffsetCalibrationDataMicroMeter(&vl53l0x_dev, 500);
```

**注意**：用于恢复之前保存的校准数据。

***

### 2.7 VL53L0X\_GetOffsetCalibrationDataMicroMeter

**作用**：获取偏移校准数据

**参数**：

| 参数                                 | 类型            | 说明         |
| ---------------------------------- | ------------- | ---------- |
| `Dev`                              | `VL53L0X_DEV` | 设备句柄       |
| `pOffsetCalibrationDataMicroMeter` | `int32_t*`    | 输出：偏移值（微米） |

**使用方法**：

```c
int32_t offset;
VL53L0X_GetOffsetCalibrationDataMicroMeter(&vl53l0x_dev, &offset);
```

**注意**：应在 `DataInit` 之后调用以备份NVM值。

***

## 三、参数配置

### 3.1 VL53L0X\_SetDeviceMode

**作用**：设置设备模式

**参数**：

| 参数           | 类型                    | 说明   |
| ------------ | --------------------- | ---- |
| `Dev`        | `VL53L0X_DEV`         | 设备句柄 |
| `DeviceMode` | `VL53L0X_DeviceModes` | 设备模式 |

**支持的模式**：

| 模式                                            | 说明     |
| --------------------------------------------- | ------ |
| `VL53L0X_DEVICEMODE_SINGLE_RANGING`           | 单次测距   |
| `VL53L0X_DEVICEMODE_CONTINUOUS_RANGING`       | 连续测距   |
| `VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING` | 定时连续测距 |

**使用方法**：

```c
// 设置为单次测距模式
VL53L0X_SetDeviceMode(&vl53l0x_dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
```

***

### 3.2 VL53L0X\_SetMeasurementTimingBudgetMicroSeconds

**作用**：设置测量时间预算

**参数**：

| 参数                                    | 类型            | 说明         |
| ------------------------------------- | ------------- | ---------- |
| `Dev`                                 | `VL53L0X_DEV` | 设备句柄       |
| `MeasurementTimingBudgetMicroSeconds` | `uint32_t`    | 测量时间预算（微秒） |

**推荐值**：

| 模式             | 时间预算              |
| -------------- | ----------------- |
| HIGH\_SPEED    | 20000 μs (20ms)   |
| LONG\_RANGE    | 33000 μs (33ms)   |
| HIGH\_ACCURACY | 200000 μs (200ms) |

**使用方法**：

```c
// 设置33ms测量时间预算
VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, 33000);
```

***

### 3.3 VL53L0X\_SetVcselPulsePeriod

**作用**：设置VCSEL脉冲周期

**参数**：

| 参数                 | 类型                    | 说明    |
| ------------------ | --------------------- | ----- |
| `Dev`              | `VL53L0X_DEV`         | 设备句柄  |
| `VcselPeriodType`  | `VL53L0X_VcselPeriod` | 周期类型  |
| `VCSELPulsePeriod` | `uint8_t`             | 脉冲周期值 |

**周期类型**：

| 类型                                 | 说明     |
| ---------------------------------- | ------ |
| `VL53L0X_VCSEL_PERIOD_PRE_RANGE`   | 预测距周期  |
| `VL53L0X_VCSEL_PERIOD_FINAL_RANGE` | 最终测距周期 |

**推荐配置**：

| 模式             | 预测距周期 | 最终测距周期 |
| -------------- | ----- | ------ |
| LONG\_RANGE    | 18    | 14     |
| HIGH\_SPEED    | 14    | 10     |
| HIGH\_ACCURACY | 14    | 10     |

**使用方法**：

```c
// 设置长距离模式的VCSEL周期
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
```

***

### 3.4 VL53L0X\_SetInterMeasurementPeriodMilliSeconds

**作用**：设置连续模式下的测量间隔

**参数**：

| 参数                                   | 类型            | 说明       |
| ------------------------------------ | ------------- | -------- |
| `Dev`                                | `VL53L0X_DEV` | 设备句柄     |
| `InterMeasurementPeriodMilliSeconds` | `uint32_t`    | 测量间隔（毫秒） |

**使用方法**：

```c
// 设置100ms测量间隔
VL53L0X_SetInterMeasurementPeriodMilliSeconds(&vl53l0x_dev, 100);
```

**注意**：仅在连续测距模式下有效。

***

### 3.5 VL53L0X\_SetLimitCheckEnable

**作用**：启用/禁用限制检查

**参数**：

| 参数                 | 类型            | 说明        |
| ------------------ | ------------- | --------- |
| `Dev`              | `VL53L0X_DEV` | 设备句柄      |
| `LimitCheckId`     | `uint16_t`    | 限制检查ID    |
| `LimitCheckEnable` | `uint8_t`     | 0=禁用，1=启用 |

**常用的LimitCheckId**：

| ID                                            | 说明          |
| --------------------------------------------- | ----------- |
| `VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE` | 最终测距信号率检查   |
| `VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE`       | 最终测距Sigma检查 |
| `VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD`  | 范围忽略阈值检查    |

**使用方法**：

```c
// 启用信号率检查
VL53L0X_SetLimitCheckEnable(&vl53l0x_dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
```

***

### 3.6 VL53L0X\_SetLimitCheckValue

**作用**：设置限制检查阈值

**参数**：

| 参数                | 类型               | 说明           |
| ----------------- | ---------------- | ------------ |
| `Dev`             | `VL53L0X_DEV`    | 设备句柄         |
| `LimitCheckId`    | `uint16_t`       | 限制检查ID       |
| `LimitCheckValue` | `FixPoint1616_t` | 阈值（16.16定点数） |

**使用方法**：

```c
// 设置信号率阈值为0.1 MCPS
VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
    VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
    (FixPoint1616_t)(0.1 * 65536));
```

**注意**：`FixPoint1616_t` 是16.16定点数格式，实际值 = 输入值 / 65536。

***

### 3.7 VL53L0X\_SetRangeFractionEnable

**作用**：启用/禁用高分辨率模式

**参数**：

| 参数       | 类型            | 说明                   |
| -------- | ------------- | -------------------- |
| `Dev`    | `VL53L0X_DEV` | 设备句柄                 |
| `Enable` | `uint8_t`     | 0=1mm分辨率，1=0.25mm分辨率 |

**使用方法**：

```c
// 启用高分辨率（0.25mm）
VL53L0X_SetRangeFractionEnable(&vl53l0x_dev, 1);
```

***

## 四、测量函数

### 4.1 VL53L0X\_PerformSingleRangingMeasurement

**作用**：执行单次测距并获取数据（阻塞模式），这是最常用的测量函数

**参数**：

| 参数                        | 类型                                  | 说明      |
| ------------------------- | ----------------------------------- | ------- |
| `Dev`                     | `VL53L0X_DEV`                       | 设备句柄    |
| `pRangingMeasurementData` | `VL53L0X_RangingMeasurementData_t*` | 输出：测距数据 |

**使用方法**：

```c
VL53L0X_RangingMeasurementData_t ranging_data;
VL53L0X_PerformSingleRangingMeasurement(&vl53l0x_dev, &ranging_data);

if (ranging_data.RangeStatus == 0) {
    uint16_t distance_mm = ranging_data.RangeMilliMeter;
    // 距离有效
}
```

**硬件交互原理**：

调用此函数时，芯片内部会执行完整的TOF（飞行时间）测量序列：

1. **设置设备模式**：将设备模式改为 `SINGLE_RANGING`

2. **执行测量序列**（由多个步骤组成）：
   - **预测距（Pre-Range）**：快速测量目标的大致距离，用于设置最终测距的参数
   - **最终测距（Final Range）**：精确测量目标距离
   - 每个步骤都包括：发射激光脉冲 → 等待反射 → 检测SPAD信号 → 计算时间

3. **获取测量数据**：读取芯片内部的测量结果寄存器

4. **清除中断标志**：清除测量完成中断标志

**TOF测量原理**：

VL53L0X使用飞行时间（Time-of-Flight）技术测量距离：

1. **发射激光脉冲**：VCSEL发射短脉冲红外激光（940nm）

2. **接收反射信号**：目标反射的激光被SPAD阵列接收，每个SPAD检测到光子时产生电信号

3. **时间测量**：芯片精确测量激光从发射到返回的时间差（飞行时间）

4. **距离计算**：距离 = 光速 × 飞行时间 / 2

**测量序列详解**：

| 步骤 | 名称 | 作用 |
|------|------|------|
| 1 | TCC（Target Centre Check） | 检测目标是否在视野中心 |
| 2 | DSS（Dynamic SPAD Selection） | 动态选择最佳SPAD |
| 3 | MSRC（Minimum Signal Rate Check） | 检查最小信号率 |
| 4 | PRE-RANGE | 预测距，快速估算距离 |
| 5 | FINAL RANGE | 最终测距，精确测量 |

**关键参数**：

| 参数 | 说明 |
|------|------|
| `RangeMilliMeter` | 测量距离（毫米） |
| `RangeStatus` | 测量状态（0=有效） |
| `SignalRateRtnMegaCps` | 返回信号率（MCPS），反映目标反射强度 |
| `AmbientRateRtnMegaCps` | 环境光强度（MCPS） |
| `SigmaMilliMeter` | 测量精度（毫米），越小越精确 |

**注意**：

- 此函数是阻塞函数，会等待测量完成（时间取决于测量时间预算）
- 自动将设备模式改为 `SINGLE_RANGING`

***

### 4.2 VL53L0X\_PerformSingleMeasurement

**作用**：执行单次测量（通用版本）

**参数**：

| 参数    | 类型            | 说明   |
| ----- | ------------- | ---- |
| `Dev` | `VL53L0X_DEV` | 设备句柄 |

**使用方法**：

```c
VL53L0X_PerformSingleMeasurement(&vl53l0x_dev);
// 测量完成后调用GetRangingMeasurementData获取数据
```

**注意**：此函数不自动获取数据，需手动调用 `GetRangingMeasurementData`。

***

### 4.3 VL53L0X\_StartMeasurement

**作用**：启动测量（非阻塞模式）

**参数**：

| 参数    | 类型            | 说明   |
| ----- | ------------- | ---- |
| `Dev` | `VL53L0X_DEV` | 设备句柄 |

**使用方法**：

```c
// 设置为连续测距模式
VL53L0X_SetDeviceMode(&vl53l0x_dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
// 启动测量（非阻塞）
VL53L0X_StartMeasurement(&vl53l0x_dev);
```

**注意**：此函数是非阻塞的，测量在后台进行。

***

### 4.4 VL53L0X\_StopMeasurement

**作用**：停止测量

**参数**：

| 参数    | 类型            | 说明   |
| ----- | ------------- | ---- |
| `Dev` | `VL53L0X_DEV` | 设备句柄 |

**使用方法**：

```c
VL53L0X_StopMeasurement(&vl53l0x_dev);
```

**注意**：单次模式下不需要调用，设备会自动返回待机状态。

***

## 五、数据获取

### 5.1 VL53L0X\_GetRangingMeasurementData

**作用**：获取测距测量数据

**参数**：

| 参数                        | 类型                                  | 说明      |
| ------------------------- | ----------------------------------- | ------- |
| `Dev`                     | `VL53L0X_DEV`                       | 设备句柄    |
| `pRangingMeasurementData` | `VL53L0X_RangingMeasurementData_t*` | 输出：测距数据 |

**使用方法**：

```c
VL53L0X_RangingMeasurementData_t data;
VL53L0X_GetRangingMeasurementData(&vl53l0x_dev, &data);
```

**测距数据结构**：

| 字段                      | 类型               | 说明           |
| ----------------------- | ---------------- | ------------ |
| `RangeMilliMeter`       | `uint16_t`       | 距离（毫米）       |
| `RangeStatus`           | `uint8_t`        | 测量状态（0=有效）   |
| `SignalRateRtnMegaCps`  | `FixPoint1616_t` | 返回信号强度（MCPS） |
| `AmbientRateRtnMegaCps` | `FixPoint1616_t` | 环境光强度（MCPS）  |
| `SigmaMilliMeter`       | `uint16_t`       | 测量精度（毫米）     |

***

### 5.2 VL53L0X\_GetMeasurementDataReady

**作用**：检查测量数据是否就绪

**参数**：

| 参数                      | 类型            | 说明            |
| ----------------------- | ------------- | ------------- |
| `Dev`                   | `VL53L0X_DEV` | 设备句柄          |
| `pMeasurementDataReady` | `uint8_t*`    | 输出：0=未就绪，1=就绪 |

**使用方法**：

```c
uint8_t ready;
VL53L0X_GetMeasurementDataReady(&vl53l0x_dev, &ready);
if (ready) {
    // 数据就绪，可以读取
}
```

**注意**：用于非阻塞模式下轮询数据。

***

### 5.3 VL53L0X\_GetTotalSignalRate

**作用**：获取总信号率

**参数**：

| 参数                 | 类型                | 说明            |
| ------------------ | ----------------- | ------------- |
| `Dev`              | `VL53L0X_DEV`     | 设备句柄          |
| `pTotalSignalRate` | `FixPoint1616_t*` | 输出：总信号率（MCPS） |

**使用方法**：

```c
FixPoint1616_t signal_rate;
VL53L0X_GetTotalSignalRate(&vl53l0x_dev, &signal_rate);
```

***

## 六、电源管理

### 6.1 VL53L0X\_SetPowerMode

**作用**：设置电源模式

**参数**：

| 参数          | 类型                   | 说明   |
| ----------- | -------------------- | ---- |
| `Dev`       | `VL53L0X_DEV`        | 设备句柄 |
| `PowerMode` | `VL53L0X_PowerModes` | 电源模式 |

**支持的模式**：

| 模式                                 | 说明    |
| ---------------------------------- | ----- |
| `VL53L0X_POWERMODE_STANDBY_LEVEL1` | 待机模式1 |
| `VL53L0X_POWERMODE_IDLE_LEVEL1`    | 空闲模式1 |

**使用方法**：

```c
// 设置为待机模式
VL53L0X_SetPowerMode(&vl53l0x_dev, VL53L0X_POWERMODE_STANDBY_LEVEL1);
```

**注意**：不应在测距状态下调用。

***

### 6.2 VL53L0X\_GetPowerMode

**作用**：获取当前电源模式

**参数**：

| 参数           | 类型                    | 说明        |
| ------------ | --------------------- | --------- |
| `Dev`        | `VL53L0X_DEV`         | 设备句柄      |
| `pPowerMode` | `VL53L0X_PowerModes*` | 输出：当前电源模式 |

**使用方法**：

```c
VL53L0X_PowerModes mode;
VL53L0X_GetPowerMode(&vl53l0x_dev, &mode);
```

***

## 七、中断管理

### 7.1 VL53L0X\_SetGpioConfig

**作用**：配置GPIO引脚功能

**参数**：

| 参数              | 类型                          | 说明             |
| --------------- | --------------------------- | -------------- |
| `Dev`           | `VL53L0X_DEV`               | 设备句柄           |
| `Pin`           | `uint8_t`                   | GPIO引脚ID（仅支持0） |
| `DeviceMode`    | `VL53L0X_DeviceModes`       | 关联的设备模式        |
| `Functionality` | `VL53L0X_GpioFunctionality` | GPIO功能         |
| `Polarity`      | `VL53L0X_InterruptPolarity` | 中断极性           |

**支持的功能**：

| 功能                                                 | 说明     |
| -------------------------------------------------- | ------ |
| `VL53L0X_GPIOFUNCTIONALITY_OFF`                    | 关闭     |
| `VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY`      | 新测量就绪  |
| `VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_LOW`  | 低于阈值   |
| `VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_HIGH` | 高于阈值   |
| `VL53L0X_GPIOFUNCTIONALITY_THRESHOLD_CROSSED_OUT`  | 超出阈值范围 |

**中断极性**：

| 极性                               | 说明    |
| -------------------------------- | ----- |
| `VL53L0X_INTERRUPTPOLARITY_LOW`  | 低电平有效 |
| `VL53L0X_INTERRUPTPOLARITY_HIGH` | 高电平有效 |

**使用方法**：

```c
// 配置GPIO为新测量就绪，高电平有效
VL53L0X_SetGpioConfig(&vl53l0x_dev, 0,
    VL53L0X_DEVICEMODE_CONTINUOUS_RANGING,
    VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY,
    VL53L0X_INTERRUPTPOLARITY_HIGH);
```

***

### 7.2 VL53L0X\_SetInterruptThresholds

**作用**：设置中断阈值

**参数**：

| 参数              | 类型                    | 说明           |
| --------------- | --------------------- | ------------ |
| `Dev`           | `VL53L0X_DEV`         | 设备句柄         |
| `DeviceMode`    | `VL53L0X_DeviceModes` | 设备模式（当前版本忽略） |
| `ThresholdLow`  | `FixPoint1616_t`      | 低阈值          |
| `ThresholdHigh` | `FixPoint1616_t`      | 高阈值          |

**使用方法**：

```c
// 设置距离阈值：小于50mm或大于200mm时触发中断
VL53L0X_SetInterruptThresholds(&vl53l0x_dev,
    VL53L0X_DEVICEMODE_CONTINUOUS_RANGING,
    (FixPoint1616_t)(50 * 65536),
    (FixPoint1616_t)(200 * 65536));
```

***

## 八、错误处理

### 8.1 VL53L0X\_GetDeviceErrorStatus

**作用**：获取设备错误状态

**参数**：

| 参数                   | 类型                     | 说明      |
| -------------------- | ---------------------- | ------- |
| `Dev`                | `VL53L0X_DEV`          | 设备句柄    |
| `pDeviceErrorStatus` | `VL53L0X_DeviceError*` | 输出：错误状态 |

**使用方法**：

```c
VL53L0X_DeviceError error;
VL53L0X_GetDeviceErrorStatus(&vl53l0x_dev, &error);
```

***

### 8.2 VL53L0X\_GetPalErrorString

**作用**：获取PAL错误的人类可读字符串

**参数**：

| 参数                | 类型              | 说明       |
| ----------------- | --------------- | -------- |
| `PalErrorCode`    | `VL53L0X_Error` | PAL错误码   |
| `pPalErrorString` | `char*`         | 输出：错误字符串 |

**使用方法**：

```c
char error_str[64];
VL53L0X_GetPalErrorString(status, error_str);
// 打印错误信息
```

***

### 8.3 VL53L0X\_GetRangeStatusString

**作用**：获取测距状态的人类可读字符串

**参数**：

| 参数                   | 类型        | 说明       |
| -------------------- | --------- | -------- |
| `RangeStatus`        | `uint8_t` | 测距状态码    |
| `pRangeStatusString` | `char*`   | 输出：状态字符串 |

**使用方法**：

```c
char status_str[64];
VL53L0X_GetRangeStatusString(ranging_data.RangeStatus, status_str);
// 打印状态信息
```

***

## 九、设备信息

### 9.1 VL53L0X\_GetDeviceInfo

**作用**：获取设备信息

**参数**：

| 参数                    | 类型                      | 说明      |
| --------------------- | ----------------------- | ------- |
| `Dev`                 | `VL53L0X_DEV`           | 设备句柄    |
| `pVL53L0X_DeviceInfo` | `VL53L0X_DeviceInfo_t*` | 输出：设备信息 |

**使用方法**：

```c
VL53L0X_DeviceInfo_t info;
VL53L0X_GetDeviceInfo(&vl53l0x_dev, &info);
```

***

### 9.2 VL53L0X\_GetProductRevision

**作用**：获取产品版本信息

**参数**：

| 参数                      | 类型            | 说明      |
| ----------------------- | ------------- | ------- |
| `Dev`                   | `VL53L0X_DEV` | 设备句柄    |
| `pProductRevisionMajor` | `uint8_t*`    | 输出：主版本号 |
| `pProductRevisionMinor` | `uint8_t*`    | 输出：次版本号 |

**使用方法**：

```c
uint8_t major, minor;
VL53L0X_GetProductRevision(&vl53l0x_dev, &major, &minor);
```

***

## 十、常用配置示例

### 10.1 快速配置（高速度模式）

```c
// 设置高速度模式
VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
    VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
    (FixPoint1616_t)(0.25 * 65536));
VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
    VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
    (FixPoint1616_t)(32 * 65536));
VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, 20000);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 14);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 10);
```

### 10.2 长距离模式配置

```c
// 设置长距离模式
VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
    VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
    (FixPoint1616_t)(0.1 * 65536));
VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
    VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
    (FixPoint1616_t)(60 * 65536));
VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, 33000);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
```

### 10.3 高精度模式配置

```c
// 设置高精度模式
VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
    VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
    (FixPoint1616_t)(0.25 * 65536));
VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
    VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
    (FixPoint1616_t)(18 * 65536));
VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, 200000);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 14);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 10);
```

***

## 十一、完整初始化流程示例

```c
#include "vl53l0x_api.h"

VL53L0X_Dev_t vl53l0x_dev = {
    .I2cHandle = &hi2c1,
    .I2cDevAddr = 0x52,
};

VL53L0X_RangingMeasurementData_t ranging_data;
VL53L0X_Error status;

void VL53L0X_Init(void) {
    // 1. 数据初始化
    status = VL53L0X_DataInit(&vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) return;

    // 2. 静态初始化
    status = VL53L0X_StaticInit(&vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) return;

    // 3. 参考校准
    uint8_t vhv_settings, phase_cal;
    status = VL53L0X_PerformRefCalibration(&vl53l0x_dev, &vhv_settings, &phase_cal);

    // 4. 参考SPAD管理
    uint32_t ref_spad_count;
    uint8_t is_aperture_spads;
    status = VL53L0X_PerformRefSpadManagement(&vl53l0x_dev, &ref_spad_count, &is_aperture_spads);

    // 5. 设置设备模式为单次测距
    VL53L0X_SetDeviceMode(&vl53l0x_dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);

    // 6. 设置测量参数（长距离模式）
    VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, 33000);
    VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
    VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
}

void VL53L0X_Measure(void) {
    // 执行单次测距
    status = VL53L0X_PerformSingleRangingMeasurement(&vl53l0x_dev, &ranging_data);

    if (status == VL53L0X_ERROR_NONE && ranging_data.RangeStatus == 0) {
        // 读取距离
        uint16_t distance_mm = ranging_data.RangeMilliMeter;
        // 读取信号强度
        float signal_rate = (float)ranging_data.SignalRateRtnMegaCps / 65536.0f;
    }
}
```
