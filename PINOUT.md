# LogCar 引脚定义说明

### 通用IO引脚

| 引脚名称        | GPIO端口 | 引脚号    | 功能描述          | 方向      | 备注               |
| :---------- | :----- | :----- | :------------ | :------ | :--------------- |
| LED\_Red    | GPIOF  | Pin 9  | 红色LED指示灯      | 输出      | 系统报错指示灯          |
| LED\_IDLE   | GPIOF  | Pin 10 | 空闲状态指示灯       | 输出      | 系统空闲时闪烁          |
| START       | GPIOA  | Pin 3  | 启动按键          | 输入      | EXTI3上升沿中断触发任务启动 |
| OLED\_CS    | GPIOA  | Pin 4  | OLED片选        | 输出      | SPI软件片选          |
| OLED\_DC    | GPIOA  | Pin 6  | OLED数据/命令选择   | 输出      | 配合SPI1使用         |
| OLED\_RES   | GPIOC  | Pin 4  | OLED复位        | 输出      | 低电平有效            |
| SENSOR\_S2  | GPIOE  | Pin 2  | TCS230颜色传感器S2 | 输出      | 输出频率标定滤波选择       |
| SENSOR\_S3  | GPIOE  | Pin 3  | TCS230颜色传感器S3 | 输出      | 输出频率标定滤波选择       |
| SENSOR\_OUT | GPIOD  | Pin 12 | TCS230输出信号    | 输入(TIM) | TIM4\_CH1输入捕获    |

### ADC引脚

| 引脚名称    | GPIO端口 | 引脚号   | 功能描述   | 方向      | 备注            |
| :------ | :----- | :---- | :----- | :------ | :------------ |
| BATTERY | GPIOA  | Pin 2 | 电池电压检测 | 输入(ADC) | ADC1通道2，DMA传输 |

### UART串口引脚

| 串口                | TX引脚 | RX引脚 | 功能描述      |
| :---------------- | :--- | :--- | :-------- |
| Terminal (USART1) | PA9  | PA10 | 终端调试串口    |
| VISION (USART2)   | PD5  | PD6  | 树莓派视觉通信串口 |
| SERVO (USART3)    | PB10 | PB11 | 舵机控制串口    |
| OPS (UART4)       | PC10 | PC11 | 平面定位模块串口  |
| SCANER (UART5)    | PC12 | PD2  | 扫码模块串口    |
| STEP (USART6)     | PC6  | PC7  | 步进电机驱动串口  |

### SPI引脚

| 引脚名称       | GPIO端口 | 引脚号   | 功能描述          |
| :--------- | :----- | :---- | :------------ |
| OLED\_SCK  | GPIOA  | Pin 5 | OLED时钟线(SPI1) |
| OLED\_MOSI | GPIOA  | Pin 7 | OLED数据线(SPI1) |

### I2C引脚

| 引脚名称          | GPIO端口 | 引脚号   | 功能描述              |
| :------------ | :----- | :---- | :---------------- |
| Tracking\_SCL | GPIOB  | Pin 6 | 追踪传感器I2C时钟线(I2C1) |
| Tracking\_SDA | GPIOB  | Pin 7 | 追踪传感器I2C数据线(I2C1) |
| VL53L0X\_SCL  | GPIOF  | Pin 0 | ToF测距传感器时钟线(I2C2) |
| VL53L0X\_SDA  | GPIOF  | Pin 1 | ToF测距传感器数据线(I2C2) |

### 追踪传感器控制引脚

| 引脚名称          | GPIO端口 | 引脚号    | 功能描述 | 方向 |
| :------------ | :----- | :----- | :--- | :- |
| Tracking\_KEY | GPIOG  | Pin 13 | 灰度按键 | 输出 |
| Tracking\_RST | GPIOG  | Pin 14 | 灰度复位 | 输出 |

## 串口配置汇总

| 串口       | 实例     | 波特率     | 数据位 | 停止位 | 校验位  |
| :------- | :----- | :------ | :-- | :-- | :--- |
| Terminal | USART1 | 1000000 | 8   | 1   | None |
| VISION   | USART2 | 115200  | 8   | 1   | None |
| SERVO    | USART3 | 115200  | 8   | 1   | None |
| OPS      | UART4  | 115200  | 8   | 1   | None |
| SCANER   | UART5  | 115200  | 8   | 1   | None |
| STEP     | USART6 | 921600  | 8   | 1   | None |

## 外设配置汇总

| 外设   | 实例  | 功能描述            |
| :--- | :-- | :-------------- |
| SPI1 | SPI | OLED屏幕(SSD1306) |
| I2C1 | I2C | 巡线灰度            |
| I2C2 | I2C | VL53L0X ToF测距   |
| ADC1 | ADC | 电池电压检测          |
| TIM4 | TIM | TCS230频率输入捕获    |
| TIM2 | TIM | 非阻塞超时计时         |
