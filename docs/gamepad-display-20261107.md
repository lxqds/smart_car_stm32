# USB无线手柄屏幕联调（无动力）

本轮仅修改RK3576上位机Python与主机测试。MCU沿用df5b20e屏幕固件，未重新烧录；三个输出开关保持0，校准参数未修改。

## 实际设备与映射

手柄开机前接收器为3537:2106（Zikway HID zk），无evdev轴/按键。用户开机后枚举3537:1041，名称Zikway HID gamepad，主接口event6；另有Consumer Control和Keyboard接口，不用于控制。

设备profile同时匹配VID/PID、名称、所需轴和按键能力；只允许唯一匹配。设备重插后不依赖event6编号，多个同型号设备同时出现时停止并等待，不随意选一个。

- 左摇杆X（ABS_X=0）：转向，左负右正，范围±15指令角度。
- 左摇杆Y（ABS_Y=1）：速度，向前为正，范围±500mm/s。
- 两轴原始0～255，中位128，归一化后使用10%死区。
- 用户确认的实体左肩键L1（实测报告BTN_TL2=312；不能按Linux名称推断实体键标签）：保持使能。先归中并松键，再按住才能产生非零目标。
- 格式仍为24 42，转向按旧main的PWM=1500-angle_command×3编码。这不是实际机械角度校准。

## 实现文件

- `host_control/gamepad_evdev.py`：按身份发现设备，周期读取完整当前轴/键快照。排空变更事件；SYN_DROPPED、事件积压、读取失败或肩键释放先使输入失效，关闭设备并重新发现。重开后必须再次松键归中。
- `host_control/gamepad_control.py`：复用InputMapper；按住使能时20Hz发送控制，未使能不持续发送。启动、松键或掉线转为零目标时发送一次中立包，然后停止发送，MCU进入STALE。输入处理停顿≥200ms也撤销使能。
- `host_control/gamepad_display.py`：默认dry-run；显式--send才打开115200串口。串口独占、有限写超时，退出尽力发送中立包；错误不自动重连串口或继续旧目标。单次运行1～600秒，不创建后台自启服务。
- `host_control/zikway_display_profile.json`：实测设备ID和所选轴键。

不能把“摇杆静止，无变化事件”当作断线，所以使用当前状态查询；但查询成功只证明Linux设备仍存在，不证明无线电链路存活。接收器断电/手柄关机是否删除event节点必须做实际测试，不能仅凭代码宣称所有无线掉线已覆盖。

SYN_DROPPED处理依据：[Linux输入事件文档](https://docs.kernel.org/input/event-codes.html)。当前实现直接废弃旧文件描述符并撤销使能，重新打开后查询状态；不继续消费失同步的旧事件。[python-evdev接口文档](https://python-evdev.readthedocs.io/en/latest/apidoc.html)。

## RK3576运行环境

目录：`/home/cat/vehicle_gamepad.sA3j4o`。系统apt安装`python3-evdev 1.4.0+dfsg-1build2`，未升级系统其他软件；原有显示测试目录保留。Python3.10.12。

```sh
cd /home/cat/vehicle_gamepad.sA3j4o
sudo -n python3 -m host_control.gamepad_display --list
sudo -n python3 -m host_control.gamepad_display --profile host_control/zikway_display_profile.json --duration 120
```

第二行运行输入检查，不发串口。当前无动力固件和动力断开条件下才可发送：

```sh
sudo -n python3 -m host_control.gamepad_display --profile host_control/zikway_display_profile.json --send --port /dev/serial/by-id/usb-1a86_5523-if00-port0 --duration 120
```

操作：归中、松开L1/LB→按住L1/LB→拨动左摇杆；松开L1/LB归零。重新连接时保持按键不会自动恢复旧目标。Ctrl+C停止；零包是尽力发送，串口断线时只能依赖MCU新鲜度/独立使能门，不能把退出包当作动力安全保障。

## 验证记录

完整主机回归及3项新增手柄测试通过：按住/松开、无使能拒绝、掉线与重连、样本过期、归一化范围、静止状态查询、SYN_DROPPED、同一批次释放后再按下不可隐藏释放、设备移除。新脚本已在RK3576编译检查通过。

实测进展：已确认手柄枚举、肩键使能/释放事件；不按使能拨动摇杆时上位机目标和MCU显示均保持零。其余现场验证以随后保存的日志和最终交付结果为准，不能将旧固定发包204/204测试作为本轮手柄测试结果。

补充记录：初次按BTN_TL=310配置时未产生有效的非零控制。仅按肩键的监听确认用户实体L1报告312，现已修正profile。修正后的第二轮发送79包（包括退出中立包），记录中按下/松开已识别，但摇杆保持中位，不能据此宣称摇杆到屏幕联调通过。第三轮操作验证进行中。
## 第四轮现场结果（2026-11-07）

用户确认“我看到都有效果了”。RK3576日志记录237条状态变化，其中168条非零目标；速度目标范围-500～500mm/s，指令角度范围-15～15。末尾已记录使能释放、零目标和重新等待使能。此处为状态变化记录数，不是发送包数；统计时发送累计440包。

MCU USB日志快照记录屏幕速度-491～500mm/s、指令角度-15～15，最后归零至speed=0/PWM=1500/angle=0。屏幕/日志采样不覆盖每个发包，不能据范围差异推断丢包，也不宣称逐包全部核验。所有已捕获VEHICLE_OUTPUT均为duty=0 pulse=0；末条UART错误统计ORE/FE/NE/PE/FULL均为0。USB日志累计dropped=28499，不宣称日志无丢弃。

证据：[MCU USB快照](validation/gamepad-20261107/gamepad-live4-usb-snapshot.txt)。RK日志保留在/home/cat/vehicle_gamepad.sA3j4o/gamepad-live4.txt。此次不烧录、不修改MCU，不进行动力测试。现场无线关机/重连保护尚未验证，仍是下一步验收项。
### 无线关机测试第一轮（待补测）

用户报告保持L1和摇杆后关机；现场日志确实出现设备失联，MCU保持零目标。但本轮发送计数一直为1（仅启动中立包），关机前未建立非零控制，不能记为“非零目标掉线归零通过”。需先验证重连门禁，再重新确认L1使能和非零目标后补做关机。
