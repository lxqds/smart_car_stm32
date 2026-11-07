# 车辆协议 v1 实现草案与第一阶段交付

**后续选择更新：用户已明确要求兼容旧 main 的24 42格式。当前MCU审核构建使用该旧格式，见`mcu-legacy-driver-review.md`。本文件A5 5A协议及framework/car_link仅保留为历史草案和测试参考，不接入当前固件。**

日期：2026-11-07。状态：已实现纯逻辑双端编解码和无输出状态机，供审核；未冻结车辆接口、未接入UART7、未烧录。板上仍运行此前验证的ASCII诊断固件。

## 本轮文件与边界

| 文件 | 已实现 | 尚未接入 |
|---|---|---|
| `framework/car_protocol.[ch]` | C编码、逐字节解析、有界缓冲、CRC、恢复 | UART接收队列及发送队列 |
| `framework/car_link.[ch]` | 会话、ACK/CAPS、使能/解除/急停/超时 | 主循环时钟、外设、遥测 |
| `host_control/protocol.py` | Python独立编解码 | Linux串口 |
| `host_control/transaction.py` | 管理命令有限重试、ACK关联 | 发送调度和握手编排 |
| `host_control/input_mapper.py` | 归一化轴死区、使能/归中/输入超时 | USB接收器、Linux事件设备、实际轴键映射 |
| `tests/test_car_protocol.c`、`test_host_control.py` | 真实C/Python交叉向量、异常与状态测试 | 板上定时、电气/动力测试 |

上位机模块暂存本仓库作为协议配套参考实现，不改写`smart_car_js`现有视觉/发送线程。后续确认依赖与实际手柄后移入上位机独立分支，不能直接并行运行两个控制发送者。

## 帧格式

拟用115200、8N1，无流控。每帧16～48字节；所有多字节字段小端。仅在将来明确切换到二进制模式后使用，不与当前ASCII诊断解析器混用。

| 偏移 | 字节数 | 字段 |
|---|---:|---|
| 0 | 2 | A5 5A |
| 2 | 1 | 版本01 |
| 3 | 1 | 命令 |
| 4 | 2 | 负载长度0～32 |
| 6 | 4 | sequence，uint32 |
| 10 | 4 | session，uint32非零 |
| 14 | LEN | payload |
| 14+LEN | 2 | CRC16，小端 |

CRC参数：width16、poly0x1021、init0xFFFF、refin=false、refout=false、xorout=0。覆盖从版本到负载末尾，不含帧头和CRC；`123456789`校验值0x29B1。无SUM8、无帧尾。

黄金向量：HELLO，seq=1，session=0x12345678，空负载：
`A5 5A 01 01 00 00 01 00 00 00 78 56 34 12 EC C8`。

解析：C最多保留48字节；完整帧在末字节到达时立即回调。错误版本/超长长度/CRC错逐字节寻找下一帧头；连续字节间隔≥100ms清除未完成帧后处理新字节，使用uint32差值处理计时回绕。没有认证功能，session只防止意外跨会话旧包，不能对抗恶意重放。损坏长度可能暂时吞入下一帧的一部分，恢复依赖后续完整帧或100ms间隔，不承诺保住错误后的每一个包。

## 命令与负载

| 命令 | 方向 | 负载和行为 |
|---|---|---|
| 01 HELLO | RK→MCU | 空；非零session。新session解除使能并清除旧控制序号；同session重试不改变使能与序号。FAULT拒绝握手 |
| 02 CAPS | MCU→RK | 16字节：version:u8、max_payload:u8、state:u8、outputs_enabled:u8、timeout_ms:u32、speed_limit_mm_s:u32、steering_limit_mdeg:u32 |
| 03 ARM | RK→MCU | 9字节：speed:i32、steering:i32、deadman:u8；必须均归中、deadman=1、当前DISARMED、序号新鲜 |
| 04 DISARM | RK→MCU | 空；立即解除使能；有效session内旧序号也可停车，新序号推进防重放边界，阻止晚到旧ARM重新使能 |
| 05 CONTROL | RK→MCU | 同ARM负载；速度mm/s、舵角毫度；deadman只能0/1；deadman=1要求已ARMED，0解除使能 |
| 06 ESTOP | RK→MCU | 空；锁定FAULT，断线/新HELLO不清除；暂不提供远程解锁命令 |
| 07 STATUS | RK→MCU | 空；返回当前状态ACK；不刷新控制新鲜度 |
| 08 ACK | MCU→RK | 4字节：原命令、result、state、outputs_enabled；头部回显请求seq/session |

result：0 OK、1 BAD_SESSION、2 BAD_LENGTH、3 BAD_STATE、4 STALE、5 BAD_VALUE、6 UNSUPPORTED。state：0 LINK_WAIT、1 DISARMED、2 ARMED、3 FAULT。

只对CRC及格式有效的请求生成回复；校验错误静默丢弃。未知命令回复UNSUPPORTED。除成功HELLO回复CAPS外均回复ACK。`STATUS`仅报告状态，不是实际速度/路程遥测。PID配置、参数持久化、速度/路程/YAW遥测及ACK发送背压处理尚未实现，不能据旧仓库行为推定已支持。

## 安全与重试规则

- `CarLink_Init`强制`allow_outputs=0`，CAPS/ACK的outputs_enabled恒为0。ARMED只代表模拟逻辑状态，目标不会写入电机/舵机寄存器。
- MCU集成时必须周期调用`CarLink_Tick`，而不是只在收包时检查超时。当前还未接入Keil调度，不能宣称板上已经实现这套超时停车。
- 序号以uint32递增，差值1～0x7FFFFFFF为新；允许自然回绕，重复及旧序号不刷新控制时间。每次串口重连主机须生成新的非零session、丢弃旧发送队列、重新握手。session生成和串口重连代码留待集成。
- HELLO/STATUS/错误包/重复包均不刷新控制时间；仅成功ARM和有效新CONTROL更新。ARMED超时转DISARMED。拒绝命令不会延长旧目标有效期。停止原因不自动重启。
- 管理命令参考重试间隔100ms、最多3次发送；相同请求复用相同seq/session/负载。ACK必须同时匹配会话、序号、原命令、合法字段；超过最后一次等待期限不再接受。失去ARM ACK时，重复ARM得到STALE，仅当回报仍为ARMED才可确认；已超时解除则拒绝。失败后调用方必须丢弃目标并要求重新归中/松键，不能自动生成新ARM。
- CONTROL禁止放入重试队列；未来发送器只持有最新控制快照。默认建议50Hz和MCU200ms超时仍为审核参数，并非已测停车响应。
- 输入层要求完整当前快照：先归中并松开使能，再按住使能才生成ARM请求；松键、断开或输入样本过期归零。重连仍按住使能不会重新ARM。
- Linux适配器必须区分“摇杆不动，没有变化事件”与“设备断线”。应在设备有效时获取完整快照，SYN_DROPPED/读取错误先失效再重新同步，不能仅以事件间隔判定无线链路存活；无线接收器对手柄掉电的实际报告行为需硬件核查。当前未实现适配器。
- 输入映射与ACK状态尚未编排为发送循环；必须收到ARM成功确认后才发送CONTROL。`InputMapper`输出只是意图，不能直接发送到车辆。

## 测试与验收状态

运行`tests/run_portable_tests.ps1`，可传入`-Compiler`和`-Python`。纯主机过程不打开USB、串口或DAP，不运行动力代码。

已验证：旧三项修复回归；C解析所有分割点、1000连续帧、逐位置损坏恢复、32字节最大负载、10万噪声字节、有界缓冲、计时回绕；Python/C 35组交叉编码向量及混合错误流；会话隔离、丢ACK重试不续命、STATUS不保活、过期ARM被较新的DISARM隔离、急停跨重连锁定；手柄模拟死区/非有限值/松键/掉线/重插/超时；管理命令有限重试和错误ACK关联。

未验证：Keil对新模块编译（新模块未加入工程）、RK3576实际运行、USB无线手柄识别、UART二进制传输、物理断线/丢ACK、实际动力停车。新代码没有烧录；此前Keil/板上测试结果不能套用于本阶段。

下一审核点：帧格式与命令；实际手柄轴/使能/急停按钮；速度与舵角符号；控制频率/超时；CAPS参数来源；正式遥测格式。审核后再将输入适配和串口发送编排接入RK3576，之后对接MCU的模拟输出并验证。
