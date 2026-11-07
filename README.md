# STM32H743 智能车主工程

本分支在原主工程中合入 UART7 中断接收、USB CDC 日志及屏幕刷新期间的通信服务。
保留原屏幕和业务代码；当前电机、舵机及电源输出使能均关闭。

## 构建入口

`MDK-ARM/smart_car_stm32.uvprojx`，Keil MDK 5.39 / Arm Compiler 6.21 / STM32H7xx_DFP 4.0.0。
编译结果：0 错误、3 个原有警告。编译输出不作为本次修改提交。
原工程引用的七个不存在的文件已从此构建入口排除，不代表相应迁移模块已实现。
`.ioc` 尚未同步手工合入内容，不要重新生成覆盖代码。

## 接线与运行

- UART7：PB3 RX、PB4 TX（AF11），115200 8N1，无流控；外部转串口须为3.3V TTL并交叉接线、共地。
- MCU USB CDC：PA11 D−、PA12 D+，仅输出日志；收到CDC数据不会触发动力控制。
- PD6作为W25Q64片选保持高，复位期间仍需板级上拉。
- `CAR_ACTUATORS_ENABLED=0`：不初始化TIM2/TIM5、不启动电机舵机PWM。
- `POWER_OUTPUT_ENABLED=0`：禁止按键打开电源使能。原ADC/DAC及显示代码保留。
- PLL1Q改为48MHz供USB使用，SPI3分频改为2以保持24MHz SPI时钟。

通信仍是 `HELLO` / `PING XXXXXXXX` / `PONG XXXXXXXX`（LF结束）的诊断协议，
不是正式车辆控制协议。USB日志输出 `RX_STATS` 和 `PWM_OFF`；突发时日志队列可能丢日志。

## 实测（2026-11-07）

合入版已烧录、校验通过，并与RK3576做了串口测试：

- 100/100应答成功，平均4.337892ms、最大4.648956ms；逐包USB收发序号完整对应。
- 分包、连续双包、连续64包、未知命令、非法hex、超长行、二进制字符、半包超时与恢复，9/9通过。
- UART ORE/FE/NE/PE及软件接收缓冲溢出计数均为0。
- 保留界面已由用户确认正常；最终固件日志确认LCD初始化和主循环执行。
- 修复前，原LCD阻塞刷新使平均应答延迟约716ms；增加LCD服务钩子后降至约4.34ms。
- HEX SHA256：`8553408411E596C0862E078127C7DDD5D6D61AEE19D64A5087B93845360E16D6`。

原始测试记录在 `docs/validation/2026-11-07/`。100次逐包测试日志完整；
突发时部分USB日志被丢弃，64条应答完整性由RK测试脚本逐字节校验。
这些结果不覆盖长期压力、物理断线重连、正式ACK/心跳、PID或整车闭环。
DAP经级联Hub有间歇性连接失败，根因未确定；本次通过不代表USB链路长期可靠。

Linux测试脚本：

```sh
python3 tools/uart7_ping_test.py --port /dev/ttyUSB0 --count 100
python3 tools/uart7_fault_test.py
```

fault脚本当前固定`/dev/ttyUSB0`。测试需先确认正确设备及动力断电。
`tools/flash-main-uart7.cfg`须从仓库根目录运行，会实际烧录、校验并复位MCU。

## 来源及审查范围

基于上游master `b872584724a3fb0625b731d2da0389e24ac05b97`。
本地已测试工作区提交 `0428c2c` 的 `debug_environment/stm32-baseline/` 映射至本仓库根目录；
此发布副本保持上游历史，不导入本机备份、凭据和调试下载目录。
USB HAL来源：stm32h7xx HAL v1.11.5，提交`57ed86b4c358e2333a71fd895b17f1ccdb68739e`；
USB Device来源：v2.11.3，提交`0b06460a43c30670a764cb7e58c5a7918e789f6e`。
保留`USB_Log/`随附许可证。USB端口配置参考STM32CubeH7 v1.11.2 H743I-EVAL CDC示例。

本分支等待独立代码审核；不要仅凭诊断测试通过就启用动力输出。
