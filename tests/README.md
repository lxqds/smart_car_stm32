# 主机测试

以下测试不会打开MCU、串口或动力外设。

```sh
mkdir -p tests/.build
gcc -std=c11 -Wall -Wextra -Wno-unused-parameter -I tests/stubs -I USB_Log -I hardware/AT24C02 tests/test_review_fixes.c -o tests/.build/test_review_fixes
tests/.build/test_review_fixes
gcc -std=c11 -Wall -Wextra tests/test_control_gate.c framework/control_gate.c -o tests/.build/test_control_gate
tests/.build/test_control_gate
```

`test_review_fixes.c`直接包含生产C模块，替换寄存器/HAL/USB日志为桩，覆盖解析服务预算、TX背压、真实环形缓冲回绕/溢出、错误位注入、I2C状态/重试和DAC换算。
桩不能证明硬件中断时序、HAL实际超时长度或电气状态，仍需板上回归。HAL内部BUSY等待可能使用自己的有限超时，不应把传入10ms宣称为所有故障下的精确总耗时。
`test_control_gate.c`为拟定控制状态框架测试。该框架未加入Keil工程，也未连接UART或GPIO，不能据此声称手柄已经能控制车辆。

`uart7_stress.py`是不同类别的硬件测试：仅在Linux上明确连接诊断固件且动力断开时运行，会打开固定`/dev/ttyUSB0`发送5秒换行洪泛，随后检查PING恢复。允许接收缓冲溢出；应同时检查MCU日志中UI_PROGRESS仍出现，恢复后正常应答。
