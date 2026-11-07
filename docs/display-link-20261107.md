# RK3576 → UART7 → MCU屏幕无动力联调

日期：2026-11-07。用户确认上一版ca8a685独立复审通过，并明确授权本次编译测试后烧录、动力保持断开。本轮已下载并完成真实串口验证，用户确认新增显示及原界面正常。这是**接收与显示链路**验收，不是手柄驾驶、速度闭环或动力验收。

## 当前实现

- RK3576：新增`host_control/display_probe.py`，标准库Linux串口发送器，默认dry-run，显式`--send --port`才打开设备；115200 8N1，10Hz，每次最多30秒；支持部分写/有限写超时，退出发送一次零速中位。限制测试目标±500mm/s、指令角度±30。串口写成功不等于MCU接收，所以程序保持`mcu_confirmed=false`，由MCU独立USB日志确认。
- 格式仍为main兼容`24 42 01 0B float32速度m/s uint16 PWM SUM8`。测试映射固定`PWM=1500-angle_command*3`，与此前main的send_k=3一致。角度字段单位未做机械校准，不宣称实际舵角。
- MCU：`VehicleApp_GetRxView`提供独立接收快照，包括末字节原始到达时间、收包计数和范围检查；接收有效不代表控制已获许可。计数回绕不影响seen判断，200ms到期为STALE，RX溢出也使fresh失效。
- `vehicle_display.[ch]`在原界面底部y168～231增加四行，保存并恢复绘图颜色，不清除整屏、不改变原业务；仅变化行重画。尺寸不足320×232不绘制以免越界。
- 显示WAIT/FRESH/STALE/INVALID、目标速度mm/s、PWM、由PWM反算的指令角度（K=3，not measured）、接收年龄及fault。fault=1/ENC:UNCAL为未填校准的预期状态，不是实测编码器零速度。
- 每次绘制后输出SCREEN_STATE/TARGET/ANGLE USB记录，可对照屏幕实际使用的同一快照。显示刷新受原业务循环影响，不能把200ms新鲜度门限宣称为200ms物理屏幕更新保证。

## 编译与主机验证

全部主机回归通过：包含此前P2复现回归、实际USB日志格式/分包测试、新增显示边界与正负目标测试、接收快照时间回绕/溢出检查；Python共10项测试。新增发送器测试涵盖部分写与超时，不打开真实串口。

RK3576 Python3.10.12实际运行通过。Linux PTY进一步验证真实发送代码、串口配置、5帧格式和最后归中包；PTY测试不算硬件通信结果。

Keil：0错误、3个原有警告；Code63578、RO7402、RW300、ZI17620。
已下载HEX SHA256：`572D7C16E5A16BBCA6C7F9377408ED0511E9732B92C9EF512ED764FF971AAB9B`。

## 烧录及实测

初次DAP产品字符串超时；Hub重启后CMD_INFO失败；Windows单接口重启被拒绝（权限不足），未执行成功。用户将DAP的USB和SWD同时断开彻底断电再接回后，OpenOCD成功Programming Finished、Verified OK并复位。此次恢复不等于证明DAP/Hub间歇故障根因已解决。

RK工作目录：`/home/cat/vehicle_display.zobUMY`。
设备：`/dev/serial/by-id/usb-1a86_5523-if00-port0` → `/dev/ttyUSB0`。USB日志由Windows COM8读取，结束后关闭；发送器也已退出并释放串口。

| 测试目标 | 发包数 | MCU屏幕/USB记录 |
|---|---:|---|
| 0mm/s、角度0，2秒 | 20+1归中 | speed=0、PWM1500、angle=0.0 |
| +250mm/s、角度+10，8秒 | 80+1归中 | speed=250、PWM1470、angle=10.0 |
| -150mm/s、角度-10，8秒 | 80+1归中 | speed=-150、PWM1530、angle=-10.0 |
| 0mm/s、角度0，2秒 | 20+1归中 | speed=0、PWM1500，停止发送后STALE |

合计204帧，MCU最终收包计数204。捕获期间ORE/FE/NE/PE/FULL均为0；所有车辆输出日志duty=0、pulse=0。用户人工确认“看到了，新增显示和原界面都正常”。

USB日志包含dropped=0和551，不能说日志从未丢失；连接日志读取前固件已运行并积累日志，后续采集可见551。204/204依据RK发送总数与MCU最终接收计数，不声称逐帧USB日志完整。

证据在`docs/validation/display-link-20261107/`：主机回归、Keil输出、成功烧录日志、RK发送日志、MCU USB日志、机器汇总JSON。本记录独立于旧P2未烧录记录，不覆盖历史结论。

## 后续复现

在RK3576上述目录，先dry-run：

```sh
python3 -m host_control.display_probe --speed-mm-s 250 --angle 10
```

仅在当前已确认的无动力固件及动力断开状态下，可执行：

```sh
sudo -n python3 -m host_control.display_probe --send --port /dev/serial/by-id/usb-1a86_5523-if00-port0 --speed-mm-s 250 --angle 10 --duration 3
```

屏幕保留最后收到的目标并标记STALE；它是历史接收显示，不是持续执行该目标。程序结束的零包是尽力发送，真正动力安全仍需已审核的许可/超时策略，不能依赖退出包。

## 未完成边界

VEHICLE_OUTPUTS_ALLOWED、CAR_ACTUATORS_ENABLED、POWER_OUTPUT_ENABLED均为0；校准表未改。没有启用电机、舵机或电源输出。未实现真实手柄输入、旧PID握手、UART遥测/ACK、堵转保护及动力校准；RK确认依赖MCU到Windows的独立USB日志。下次若换为可输出固件，不可直接运行本测试脚本，必须重新审核使能和安全规则。
