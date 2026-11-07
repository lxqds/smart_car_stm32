# 赛车式手柄与挡位显示（2026-11-07，未烧录）

## 已实现

保留24 42 CMD=01总长11的速度float32 m/s和PWM uint16控制包。新增显示元数据24 42 30 07 01 GEAR SUM8：版本1、挡位1..7。解析器校验长度、版本、挡位、SUM8，保存末字节原始时间，不调用控制回调，不更新控制期限或许可。元数据500ms过期、接收丢失后显示GEAR:-。旧主机兼容。

屏幕保留原业务区域，在底部4行首行显示GEAR，后续保留CMD速度mm/s、PWM、CMD角度（K=3，非实测）、错误与未校准状态。速度显示为目标而非编码器实测。

host_control/racing_control.py实现软件1..7挡上限500/1000/1500/2000/3000/4000/5000mm/s，RT油门、LT刹车优先、左摇杆转向±15、B升挡、X降挡、A撤销使能、L1保持使能。换挡按下沿触发，同时按升降挡不换挡。初始/断线回1挡，必须松L1且油门刹车松开、转向回中，再按L1使能。输入停顿>=200ms、异常、掉线归零并撤销使能。禁用状态只发一次中立控制包，随后只发挡位元数据，不延续控制新鲜度。

模拟目标加速500mm/s²，松油门减速1000mm/s²，刹车2000mm/s²；这些是软件演示参数，不是实车制动性能或校准。实际动力仍禁用。无倒挡。

host_control/racing_evdev.py及gamepad_display.py --racing接入驱动，要求racing_mapping_verified=true。zikway_racing_profile.pending.json关键未验证字段为null且验证标记false，不能直接运行。原手柄显示模式保留。

## 验证

全套纯主机回归通过，包括原C回归、10项原Python测试、3项手柄测试及2项赛车综合测试。新增C挡位测试覆盖版本、范围、坏校验、恢复、时间回绕及元数据不触发控制回调；显示测试覆盖挡位和过期。赛车测试覆盖使能、7挡、保持按键不连跳、5m/s模拟上限、斜坡、刹车优先、停止、断线、输入超时和uint32回绕。

Keil实际主工程MDK-ARM/smart_car_stm32.uvprojx重编译0错误3警告：已有AT24C02头文件u8/u16重复typedef以及LCD字符串指针符号警告。最初误用旧smart_car_buildcheck.uvprojx因USB include缺失失败，随后使用实际主工程成功；未修改旧buildcheck工程。

HEX SHA256：F9401BFB62982242A57CE2BA3C464CE053888C63E6520669E4105732189E0854

VEHICLE_OUTPUTS_ALLOWED、CAR_ACTUATORS_ENABLED、POWER_OUTPUT_ENABLED仍为0；校准参数未改。新HEX未烧录、未硬件验证，没有运行下载器或复位MCU。

证据：validation/racing-20261107/host-tests.txt、keil-build.txt。

## 待完成

RT/LT/B/X/A实体映射未实测。第一轮监听启动时设备不存在，程序StopIteration退出，不能采用用户操作作为映射证据。最新RK USB列表3537:2106 Zikway HID zk，无evdev手柄节点；正常工作曾为3537:1041。需确认手柄开机、模式及配对，设备恢复后再启动并确认只读监听，再请用户操作。

键位确认后补驱动测试、RK部署与无动力屏幕验收。当前板上仍是旧无挡位固件，不能宣称实体屏幕已显示新挡位。旧手柄关机归零测试未完成非零前置条件，重连保持按键测试也未完成；均不可继承为本版验证结论。

## 第二轮实体键位采集

用户确认按RT、LT、B、X、A顺序操作。racing-map2.txt记录RT轴9范围0..255，附带键313；LT轴10范围0..196，附带键312；B=305、X=307、A=304。LT满量程尚未采到，不以196重标定。此前使能312与本次LT冲突，因此赛车profile使能编号置null，verified仍false。待单独确认左上L1/LB并补测LT，不能沿用旧312作为使能。

## 用户修订：Y切换使能（以本节为准）

L1保持使能方案取消。实体Y实测为308，两次按下/释放均捕获。Y按下沿切换，释放不关闭；首次开启需先观察Y松开且RT/LT松开、转向归中。再次Y、A、掉线、样本超时关闭；重连需再次松开Y并在中位按下。该使能仅是主机目标输入状态，不能开启MCU本地动力许可。

显示协议V2：24 42 30 08 02 GEAR ENABLED SUM8，ENABLED仅0/1；V1仍接收但使能显示强制OFF。元数据不刷新控制期限。屏幕显示CTRL:ON/OFF；蓝色屏幕圆灯表示输入使能，黑色表示关闭。只有控制包和元数据均新鲜且元数据使能为1时亮灯；不是板上GPIO灯，也不是动力输出状态。

配置zikway_racing_profile.json：Y308、B305、X307、A304、RT轴9、LT轴10、转向轴0。pending文件保留为之前未确认模板。RT实测0..255；LT已确认轴及松开零位，但最大按压仅采到196，仍采用设备声明0..255，不以196重标定。最终满量程和实车制动能力尚未验证。

新版主机回归通过；Keil主工程0错误3个既有警告。新HEX SHA256 A842A1F6385BFBE18E1A35B21BB976B2E3EB7544DAEF00EFF78259BB0A328919。未烧录，实体屏幕蓝灯尚未验收，所有动力开关仍0。证据另存validation/racing-y-20261107，不覆盖上一版记录。

RK已部署Python脚本并编译检查，racing-dryrun1.txt启动状态connected/gear1/enabled=false/sent0；正在进行不发串口的实体手柄操作验收。

## PE3实体LED修正（以本节为准）

用户明确指示灯是PE3实体蓝色LED，极性未知，后续实测。已删除屏幕圆灯，保留CTRL文字。vehicle_indicator.h的VEHICLE_LED_POLARITY_VERIFIED=0，当前固件仍保留TIM14原心跳翻转。未假设哪一电平是熄灭。核实后需同时设置VERIFIED=1和ACTIVE_HIGH=0或1，才能关闭心跳并由10ms车辆任务根据新鲜控制包、新鲜显示元数据和主机使能状态驱动PE3。当前条件编译关闭的GPIO使能分支尚未硬件验证，不能宣称实体灯已完成。

新版全套纯主机回归通过，实际Keil主工程0错误3既有警告。HEX SHA256：75BF7A11BC5150D2E82C4C6BECE79CE9C80B99AC4D04395EAEBE01CE2FB3162B。未烧录，三个动力输出开关仍0，校准参数未改。证据独立保存validation/racing-pe3-20261107。

RK赛车dry-run首轮记录81条状态变化，Y开启，挡位1→2→3→2，最高模拟目标1313mm/s；Y再次按下关闭，随后油门达到1.0时目标仍0；所有记录sent=0。LT输入被识别，日志尾部刹车状态速度为0，尚不能单凭该摘要断言从非零速度开始的实测刹车过程。该结果不是UART端到端或新固件硬件验收。完整日志位于RK /home/cat/vehicle_gamepad.sA3j4o/racing-dryrun1.txt。

## 本轮授权烧录尝试

用户已明确要求烧录测试。目标HEX核对为75BF7A11BC5150D2E82C4C6BECE79CE9C80B99AC4D04395EAEBE01CE2FB3162B，三个动力开关0。已停止RK赛车dry-run。OpenOCD HID可识别DAP固件2.1.0，在1000kHz与100kHz均报Error connecting DP: cannot read IDR，初始化失败，未进入Programming Started，未完成擦写或新固件测试。日志debug_environment/flash-racing-pe3.stderr.txt及flash-racing-slow.stderr.txt。等待用户确认MCU供电与重插SWD接头；此故障不能直接归因为过去的USB Hub超时。

### 实际烧录成功与启动

用户重插SWD接头后，100kHz下载成功，OpenOCD返回0，Verified OK并复位启动；本轮烧录HEX为75BF7A11BC5150D2E82C4C6BECE79CE9C80B99AC4D04395EAEBE01CE2FB3162B。此前“未烧录”仅描述之前状态。USB已读取新SCREEN_STATE格式GEAR/CTRL，输出duty=0 pulse=0。RK赛车live1启动，600秒有限测试，待用户操作验收。PE3极性仍未知，固件仍保留原心跳闪烁，不把它当Y状态指示通过。

## 用户授权直接配置 PE3/Y（取消极性预验证）

用户要求直接写入，反了再反馈。当前vehicle_indicator.h使用CONFIGURED=1、ACTIVE_HIGH=0，明确是暂定低电平亮，非实测极性；取消VERIFIED命名，TEST_LOW=0。PE3初始化为暂定熄灭高电平，TIM14不再翻转，10ms应用任务按控制/元数据新鲜度和Y输入使能更新实体LED。屏幕保留CTRL文字，无圆灯。临时固定低电平测试版下载失败（DAP无法匹配），没有写入MCU。

实际应用GPIO路径主机测试覆盖开启低电平、控制200ms过期高电平、主机关闭高电平；完整回归通过。Keil 0错误3既有警告。联动HEX SHA256 CDB5470AD06FDA2CD5C83787FC1892438B8F5262D39BEE0D8AF07E18ABA1AF3B，写入结果以随后记录为准。

此前赛车屏幕版用户确认挡位、速度、角度及Y开关正常。RK日志挡位1..4、速度0..500mm/s、角度-15..15；USB捕获输出均duty=0 pulse=0，末条UART错误ORE/FE/NE/PE/FULL为0，USB dropped=159，不宣称日志无丢弃。停止发送后准备本次LED版本。该屏幕结果不证明PE3极性正确。

### PE3/Y版本烧录成功

用户重新连接DAP后，100kHz下载返回0，Programming Finished / Verified OK / Resetting Target。已写入HEX CDB5470AD06FDA2CD5C83787FC1892438B8F5262D39BEE0D8AF07E18ABA1AF3B。启动USB日志读取，RK已启动led-y-live1.txt有限600秒无动力测试。PE3低电平亮仍是用户允许的暂定极性，等待实体灯反馈。此记录取代上一条“尚未写入”的状态，不将烧录成功当作极性验证。

### PE3/Y现场验收通过

用户明确反馈“ON亮、OFF灭，正常”，确认本板PE3低电平点亮。RK日志记录两轮enabled false→true→false，MCU末尾CTRL:OFF、目标为零，捕获输出duty=0 pulse=0，末条ORE/FE/NE/PE/FULL均0。实体极性依据用户目视，不是软件读回GPIO推断。测试发送程序已终止；USB证据另存validation/led-y-20261107/led-y-live1-usb.txt。固件仍为CDB5470AD06FDA2CD5C83787FC1892438B8F5262D39BEE0D8AF07E18ABA1AF3B，没有改变动力输出开关。无线关机/重连保护及真实动力闭环尚未验收，不能因LED正常而视为通过。
