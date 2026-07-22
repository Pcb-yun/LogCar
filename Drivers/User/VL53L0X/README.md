# VL53L0X 驱动移植指南

## 一、文件结构

```
STM32CubeExpansion_VL53L0X_V1.2.0/
└── Drivers/
    └── BSP/
        ├── Components/
        │   └── vl53l0x/              # VL53L0X核心驱动（芯片无关）
        │       ├── vl53l0x_api.c/h           # 主API接口
        │       ├── vl53l0x_api_core.c/h      # 核心API
        │       ├── vl53l0x_api_ranging.c/h   # 测距API
        │       ├── vl53l0x_api_calibration.c/h # 校准API
        │       ├── vl53l0x_api_strings.c/h   # 字符串转换API
        │       ├── vl53l0x_def.h             # 类型定义和常量
        │       ├── vl53l0x_types.h           # 基本类型定义
        │       ├── vl53l0x_device.h          # 设备寄存器定义
        │       ├── vl53l0x_tuning.h          # 调谐参数表
        │       ├── vl53l0x_interrupt_threshold_settings.h # 中断阈值设置
        │       └── vl53l0x_platform_log.c/h  # 日志接口
        └── X-NUCLEO-53L0A1/           # 平台适配层（STM32 HAL I2C）
            ├── vl53l0x_platform.c            # I2C底层实现
            └── vl53l0x_platform.h           # 平台接口头文件
```

## 二、文件功能说明

### 2.1 核心驱动层（Components/vl53l0x/）

#### 主API接口

| 文件 | 功能 |
|------|------|
| vl53l0x_api.h | 主API头文件，声明所有公开API函数，应用层只需包含此文件 |
| vl53l0x_api.c | 主API实现，包括初始化、设备信息读取、参数设置、电源管理 |

#### 核心功能模块

| 文件 | 功能 |
|------|------|
| vl53l0x_api_core.h/c | 底层核心功能，SPAD管理、参考校准、序列配置、定时器预算计算 |
| vl53l0x_api_ranging.h/c | 测距功能实现，单次测距、连续测距、获取测量数据 |
| vl53l0x_api_calibration.h/c | 校准功能，参考校准(VHV/Phase)、偏移校准、串扰测量与校准 |
| vl53l0x_api_strings.h/c | 错误码/状态码转字符串，用于调试日志输出 |

#### 定义和类型文件

| 文件 | 功能 |
|------|------|
| vl53l0x_def.h | 全局类型定义：错误码、设备模式、测距数据结构体、设备参数结构体 |
| vl53l0x_types.h | 基本类型定义，FixPoint1616_t定点数类型 |
| vl53l0x_device.h | 芯片内部寄存器地址和位域定义，驱动内部使用 |
| vl53l0x_tuning.h | 出厂默认调谐参数数组，初始化时写入芯片 |
| vl53l0x_interrupt_threshold_settings.h | 中断模式下的阈值配置定义 |

#### 日志接口

| 文件 | 功能 |
|------|------|
| vl53l0x_platform_log.h/c | 日志等级和模块定义，默认不启用日志 |

### 2.2 平台适配层（X-NUCLEO-53L0A1/）

| 文件 | 功能 |
|------|------|
| vl53l0x_platform.h | 平台接口头文件，定义设备结构体VL53L0X_Dev_t和I2C读写函数声明 |
| vl53l0x_platform.c | 基于STM32 HAL库的I2C驱动实现，包含读写单字节/双字/四字寄存器、批量读写 |

## 三、移植流程

### 3.1 前提条件

- STM32 HAL库项目
- I2C外设已配置（建议400kHz快速模式）
- VL53L0X传感器已连接到I2C总线

### 3.2 步骤

#### 步骤1：复制文件到项目

将以下目录复制到你的STM32项目中：

```
Drivers/BSP/Components/vl53l0x/
Drivers/BSP/X-NUCLEO-53L0A1/
```

#### 步骤2：添加到工程编译

在IDE中添加以下源文件到编译列表：

```
vl53l0x_api.c
vl53l0x_api_core.c
vl53l0x_api_ranging.c
vl53l0x_api_calibration.c
vl53l0x_api_strings.c
vl53l0x_platform.c
vl53l0x_platform_log.c
```

#### 步骤3：添加头文件路径

在IDE的Include路径中添加：

```
Drivers/BSP/Components/vl53l0x/
Drivers/BSP/X-NUCLEO-53L0A1/
```

#### 步骤4：适配平台层（如果需要）

打开 `vl53l0x_platform.h` 和 `vl53l0x_platform.c`，确认HAL头文件引用：

```c
#include "stm32f4xx_hal.h"  // 根据你的MCU型号修改
```

#### 步骤5：初始化设备

在代码中初始化设备结构体：

```c
#include "vl53l0x_api.h"

VL53L0X_Dev_t vl53l0x_dev = {
    .I2cHandle = &hi2c1,       // 你的I2C句柄
    .I2cDevAddr = 0x52,        // 8位格式的I2C地址
};

VL53L0X_RangingMeasurementData_t ranging_data;
```

#### 步骤6：设备初始化

```c
VL53L0X_Error status;

// 1. 数据初始化（加载NVM校准数据）
status = VL53L0X_DataInit(&vl53l0x_dev);
if (status != VL53L0X_ERROR_NONE) {
    // 初始化失败处理
}

// 2. 静态初始化（应用默认设置）
status = VL53L0X_StaticInit(&vl53l0x_dev);

// 3. 执行参考校准
uint8_t vhv_settings, phase_cal;
status = VL53L0X_PerformRefCalibration(&vl53l0x_dev, &vhv_settings, &phase_cal);

// 4. 参考SPAD管理
uint32_t ref_spad_count;
uint8_t is_aperture_spads;
status = VL53L0X_PerformRefSpadManagement(&vl53l0x_dev, &ref_spad_count, &is_aperture_spads);

// 5. 设置设备模式为单次测距
status = VL53L0X_SetDeviceMode(&vl53l0x_dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
```

#### 步骤7：执行测距

```c
// 单次测距（阻塞模式）
status = VL53L0X_PerformSingleRangingMeasurement(&vl53l0x_dev, &ranging_data);
if (status == VL53L0X_ERROR_NONE && ranging_data.RangeStatus == 0) {
    uint16_t distance_mm = ranging_data.RangeMilliMeter;
    // 距离有效，使用数据
}
```

## 四、三种测距配置模式

| 模式 | 信号阈值 | Sigma阈值 | 时间预算 | VCSEL周期 | 适用场景 |
|------|---------|----------|----------|-----------|---------|
| LONG_RANGE | 0.1 MCPS | 60 mm | 33ms | Pre:18, Final:14 | 远距离测量 |
| HIGH_SPEED | 0.25 MCPS | 32 mm | 20ms | Pre:14, Final:10 | 高速测量 |
| HIGH_ACCURACY | 0.25 MCPS | 18 mm | 200ms | Pre:14, Final:10 | 高精度测量 |

配置示例：

```c
// 设置长距离模式
VL53L0X_SetLimitCheckValue(&vl53l0x_dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, (FixPoint1616_t)(0.1*65536));
VL53L0X_SetLimitCheckValue(&vl53l0x_dev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, (FixPoint1616_t)(60*65536));
VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, 33000);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
```

## 五、注意事项

### 5.1 I2C地址格式

- VL53L0X默认地址：**8位格式 0x52**（不是7位格式0x29）
- 驱动内部自动处理读/写地址切换（读地址=0x53）

### 5.2 RTOS环境

如果使用RTOS且多个任务访问同一I2C总线，需要实现互斥锁：

```c
// 在vl53l0x_platform.h之前定义
extern osMutexId_t I2cBusMutexHandle;

#define VL53L0X_GetI2cBus(...) osMutexWait(I2cBusMutexHandle, osWaitForever)
#define VL53L0X_PutI2cBus(...) osMutexRelease(I2cBusMutexHandle)
```

### 5.3 多设备

可通过 `VL53L0X_SetDeviceAddress()` 修改设备地址，支持多个VL53L0X传感器。**必须通过XSHUT引脚控制逐个上电**：

```c
// 步骤1：初始化GPIO作为XSHUT引脚输出（推挽输出）
// GPIO配置示例：SHUT1 -> PA0, SHUT2 -> PA1

// 步骤2：关闭所有传感器
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
HAL_Delay(10);

// 步骤3：打开第一个传感器，使用默认地址0x52
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
HAL_Delay(10);
VL53L0X_DataInit(&vl53l0x_dev1);
// ... 其他初始化步骤 ...

// 步骤4：打开第二个传感器，修改地址为0x54
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
HAL_Delay(10);
VL53L0X_SetDeviceAddress(&vl53l0x_dev2, 0x54);  // 将第二个传感器地址改为0x54
VL53L0X_DataInit(&vl53l0x_dev2);
// ... 其他初始化步骤 ...

// 步骤5：两个传感器地址分别为0x52和0x54，可正常通信
```

### 5.4 校准数据

`VL53L0X_DataInit()` 会从NVM加载校准数据，首次上电必须调用。如果无法执行硬件复位，需要手动保存和恢复偏移校准数据。

## 六、核心API速查

### 初始化

| 函数 | 功能 |
|------|------|
| VL53L0X_DataInit() | 数据初始化（加载NVM校准数据） |
| VL53L0X_StaticInit() | 静态初始化（应用默认设置） |
| VL53L0X_ResetDevice() | 复位设备 |

### 校准

| 函数 | 功能 |
|------|------|
| VL53L0X_PerformRefCalibration() | 执行参考校准 |
| VL53L0X_PerformRefSpadManagement() | 参考SPAD管理 |
| VL53L0X_PerformXTalkMeasurement() | 执行串扰测量 |

### 参数配置

| 函数 | 功能 |
|------|------|
| VL53L0X_SetDeviceMode() | 设置设备模式 |
| VL53L0X_SetMeasurementTimingBudgetMicroSeconds() | 设置测量时间预算 |
| VL53L0X_SetVcselPulsePeriod() | 设置VCSEL脉冲周期 |
| VL53L0X_SetLimitCheckEnable() | 启用限制检查 |

### 测量

| 函数 | 功能 |
|------|------|
| VL53L0X_PerformSingleRangingMeasurement() | 执行单次测距（阻塞） |
| VL53L0X_GetRangingMeasurementData() | 获取测距数据 |
| VL53L0X_StartMeasurement() | 启动连续测量 |
| VL53L0X_StopMeasurement() | 停止测量 |
