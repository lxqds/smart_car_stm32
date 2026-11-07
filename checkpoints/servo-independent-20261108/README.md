# 已验证的舵机独立重建版

2026-11-08：用户确认实际回中、左右方向正确、能回中；Y关闭后USB确认arm=0/on=0/out=0。电机动力断开，软件MOTOR_OFF。不是整车或带载验收。

- `source.zip`：本次实际编译副本的源码、HAL、Keil工程、主机代码和测试，共280个文件；不含编译缓存。解压至独立目录，用MDK-ARM/smart_car_stm32.uvprojx编译。
- `firmware.hex`：实际烧录并校验通过的HEX，SHA256为49C99D32D37E7BD77F05C1174130F3C32757F7D82F7D94A129E6278DDCC09CE5。
- `build.txt`：Keil 0错误、3警告。
- `validation.txt`：主机测试、部署及用户实测记录。
- `flash.txt`：OpenOCD Programming Finished / Verified OK。
- `usb-validation.txt`：本次启动及转向后关闭的USB日志片段；日志并非无损。
- `sha256.json`：上述归档文件校验值。

默认SERVO_BENCH_MODE=1；PA2/TIM5_CH3，50Hz，1500us中位，原方向和1140/2000输入阈值，20us/20ms步进。该版本基于当前代码重建，仍含新通知逻辑，不是旧884832FE固件的原样恢复。

此提交仅增加可回退检查点，不修改当前主工程中的联合版工作区。回刷需另行明确授权；不要将本记录解释为自动烧录指令。重编译输出可能受工具链和构建时间等影响，回刷应优先核对已存HEX哈希。
