# LogCar 引脚定义说明

### 通用IO引脚

| 引脚名称     | GPIO端口 | 引脚号    | 功能描述     | 方向      | 备注       |
| :------- | :----- | :----- | :------- | :------ | :------- |
| LED\_Red | GPIOF  | Pin 9  | 红色LED指示灯 | 输出      | 系统状态指示   |
| LED\_IDLE | GPIOF  | Pin 10 | 空闲状态指示灯 | 输出      | 系统空闲时闪烁 |
| KEY\_UP  | GPIOA  | Pin 0  | 按键输入     | 输入      | 用户按键     |
| START  | GPIOA  | Pin 1  | 按键输入     | 输入      | 启动按键     |
| BATTERY  | GPIOA  | Pin 2  | 电池电压检测   | 输入(ADC) | 连接ADC1通道 |

### UART串口引脚

| 串口                | TX引脚 | RX引脚 | 功能描述     |
| :---------------- | :--- | :--- | :------- |
| Terminal (USART1) | PA9  | PA10 | 终端调试串口   |
| SERVO (USART3)    | PB10 | PB11 | 舵机控制串口   |
| STEP (USART6)      | PC6  | PC7  | 步进电机驱动串口 |
| OPS (UART4)      | PC10 | PC11 | 平面定位模块串口     |

### I2C引脚

| 引脚名称          | GPIO端口 | 引脚号   | 功能描述        |
| :------------ | :----- | :---- | :---------- |
| Tracking\_SCL | GPIOB  | Pin 6 | 追踪传感器I2C时钟线 |
| Tracking\_SDA | GPIOB  | Pin 7 | 追踪传感器I2C数据线 |

### 追踪传感器控制引脚

| 引脚名称          | GPIO端口 | 引脚号    | 功能描述    | 方向 |
| :------------ | :----- | :----- | :------ | :- |
| Tracking\_KEY | GPIOG  | Pin 13 | 追踪传感器按键 | 输出 |
| Tracking\_RST | GPIOG  | Pin 14 | 追踪传感器复位 | 输出 |

## 串口配置汇总

| 串口                | 实例    | 波特率     | 数据位 | 停止位 | 校验位  |
| :---------------- | :----- | :------- | :-- | :-- | :--- |
| Terminal          | USART1 | 1000000  | 8   | 1   | None |
| SERVO             | USART3 | 115200   | 8   | 1   | None |
| STEP              | USART6 | 921600   | 8   | 1   | None |
| OPS               | UART4  | 115200   | 8   | 1   | None |
