# esphome-custom-components
esphome扩展组件
## 如何使用
请参考[esphome文档](https://www.esphome.io/components/external_components.html)。
```yaml
#引用组件库示例
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ example ]
```

## 组件列表
### Audio Player
> esp8266 通过http流播放wav
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ audio_player ]

media_player:
  - platform: audio_player
    name: "Media Player"
    volume: 100% #可选，基准音量
    buffer_size: 10240 #可选，http流缓冲区大小
    i2sNoDAC: {} # 二选一，RX直接输出音频
    i2s: # 二选一，外置DAC输出
      bclk: GPIO15
      wclk: GPIO3
      dout: GPIO2
```
</details>

### CC1101
> cc1101 315/433mHz射频收发
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ rf_bridge_cc1101 ]

rf_bridge_cc1101:
  spi_id: cc1101 #与cc1101连接的spi总线id
  cs_pin: GPIO5 #cc1101的片选pin
  pin: GPIO14 #cc1101的GD0连接在esp上的pin
  frequency: 433.92 #指定收发频率
  tolerance: 80% #容许信号时长误差
  filter: 100us #忽略小于这个时长的信号
  idle: 4ms #信号超时时间
  buffer_size: 2kb #信号缓存大小
  dump: #可选，未匹配到相关sensor或者没有配置on_code_received时，在控制台打印接收到的内容
    - rc_switch
   on_code_received: #示例，收到内容时打印
     then:
       - lambda: |-
           ESP_LOGD("Trigger","received code %06X", x.code);

binary_sensor: 
  - platform: rf_bridge_cc1101 #示例，接收到指定内容时触发一个传感器
    name: "Motion Sensor"
    device_class: "motion"
    filters:
      - delayed_off: 500ms
    code: "101010111110010010101010" #dump中打印的值
    protocol: 1
```
</details>

### Xiaomi Toothbrush M1ST500
> 小米电动牙刷T500
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ xiaomi_m1st500 ]

esp32_ble_tracker:

sensor:
  - platform: xiaomi_m1st500
    mac_address: "XX:XX:XX:XX:XX:XX" #你的牙刷mac地址
    score:
      name: "Toothbrush Score" #刷牙分数，在一次完整的2分钟刷牙后得出
    battery_level:
       name: "Toothbrush Battery Level" #电池电量
```
</details>

### Xiaomi Smoke Detector MCN02
> 小米烟雾报警器蓝牙版mcn02
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ xiaomi_smoke_detector ]

esp32_ble_tracker:

sensor:
  - platform: xiaomi_smoke_detector
    mac_address: "XX:XX:XX:XX:XX:XX" #烟雾报警器蓝牙mac地址
    bindkey: "0000000000000000000000000000000" #token
    alert: # 可选：报警 binary sensor
      name: "Smoke Detector Alert" 
    status: # 可选：报警器状态 sensor
      name: "Smoke Detector Status"
    status_text: # 可选：报警器文本状态 text sensor
      name: "Smoke Detector Status Text"
      filters: # 可选：翻译文本状态
        - map:
          - normal -> 正常监测
          - alert -> 火灾报警
          - fault -> 设备故障
          - self-check -> 设备自检
          - analog alert -> 模拟报警
          - unkown -> 未知状态
    battery_level: # 可选：报警器电量 sensor
      name: "Smoke Detector Battery"
```
</details>

### ssw_tds
> 支持型号为ssw-tds-2u的一款基于urat协议的2路带水温tds检测仪。可输出双路tds及一路水温。
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ ssw_tds ]

uart:
  id: uart_tds
  baud_rate: 9600
  rx_pin: RX #你的RX引脚
  tx_pin: TX #你的TX引脚
  
sensor:
  - platform: ssw_tds
    source_tds:
      name: "Source TDS" #自来水TDS
    clean_tds:
      name: "Clean TDS" #净水TDS
    temperature:
      name: "Water Temperature" #水温
    update_interval: 5s # 可选，更新间隔默认5s
```
</details>

### CEM5855H
> 支持某国产芯片的毫米波雷达，同样适用于LD1115H。只需要连接VCC,GND,TX(CEM5855H)->RX(MCU)。阈值配置参考[Number](https://esphome.io/components/number/index.html)
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ cem5855h ]

logger:
  level: VERBOSE #如果需要在日志中查看串口信息，设置日志级别为VERBOSE

uart:
  id: uart_cem5855h
  baud_rate: 115200
  rx_pin: RX #你的RX引脚
  
binary_sensor:
  - platform: cem5855h
    moving: #（可选）有人移动传感器
      - name: mov
        filters:
          - delayed_off: 3s
        threshold: 250 #示例：阈值可以是数字
      - name: mov2 # 示例：可以配置多个不同阈值的传感器
        filters:
          - delayed_off: 3s
        threshold: 350
    occupancy: #（可选）有人静止传感器
      name: occ
      filters:
        - delayed_off: 3s
      threshold: #示例： 阈值可以是数字组件
        name: "occ"
        value: 250 #阈值
        min_value: 10 #可调节的最小阈值
        max_value: 3000 #可调节的最大阈值
        step: 10 #单步调节量
    motion: #（可选）人在传感器
      name: motion
      filters:
        - delayed_off: 3s #设置一个合适的状态持续时间
      threshold:
        moving: 250
        occupancy: 250
```
</details>

### esp32_bt_tracker, bt_presence, bt_rssi
> 通过经典蓝牙扫描发现蓝牙设备，蓝牙设备须处于可被发现状态
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ esp32_bt_tracker, bt_presence, bt_rssi ]

esp32_bt_tracker: #启用经典蓝牙扫描组件，控制台会以debug消息打印扫描到的未设置sensor的设备mac
  scan_parameters:
      duration: 5s #每次扫描持续时间，影响不大

binary_sensor: #示例，扫描到设备
  - platform: bt_presence
    mac_address: XX:XX:XX:XX:XX:XX #安卓手机蓝牙mac地址
    name: "android"

sensor: #示例，显示设备信号强度
  - platform: bt_rssi
    mac_address: XX:XX:XX:XX:XX:XX #安卓手机蓝牙mac地址
    name: "android"
```
</details>

### fpm383c
> 支持海凌科fpm383c电容式指纹模块
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ fpm383c ]

esphome:
  on_boot: # 可选，上电重置指纹模块
    priority: 250
    then:
      - fpm383c.reset

uart: #指定串口
  id: uart_fpm383c
  baud_rate: 57600
  rx_pin: GPIO26
  tx_pin: GPIO25

fpm383c:
  auto_learning: True # 默认根据匹配结果自动更新指纹
  on_reset: # 可选，指纹模块完成初始化
      - light.turn_on: #示例，打开蓝灯
          id: fpm383c_id
          brightness: 50%
          red: 0
          green: 0
          blue: 100%
  on_touch: #手指触摸模块时触发
    - light.turn_on: #示例，打开黄灯
        id: fpm383c_id
        brightness: 50%
        red: 100%
        green: 100%
        blue: 0
  on_release: #手指松开时触发
    - light.turn_on: #示例，打开蓝灯
        id: fpm383c_id
        brightness: 50%
        red: 0
        green: 0
        blue: 100%
  on_finger_scan_unmatched: #指纹匹配失败时触发
    - light.turn_on: #示例，打开红灯
        id: fpm383c_id
        brightness: 50%
        red: 100%
        green: 0
        blue: 0
  on_finger_scan_matched: #指纹匹配成功时触发
    - light.turn_on: #示例，打开绿灯
        id: fpm383c_id
        brightness: 50%
        red: 0
        green: 100%
        blue: 0
  on_finger_register_progress: #指纹注册进度，可以获取当前指纹编号和注册进度
    then:
    - lambda: |-
        ESP_LOGD("PROGRESS", "id: %04X, progress: %d%%", x.id, x.progress_in_percent); // 示例，打印进度日志

light: 
  - platform: fpm383c #配置RGB指示灯，所有颜色都会被重新映射到6种颜色（白色映射到红色）
    name: "指示灯"
    id: fpm383c_id
    effects: # 仅支持以下效果 
      - breathing: # 呼吸灯
          name: "Breathing" # 效果默认名称
          min_brightness: 0% # 最小亮度
          max_brightness: 100% # 最大亮度
          rate: 50% # 每秒变化百分比
      - flashing: # 闪烁
          name: "Flashing" # 效果默认名称
          on_length: 200ms # 亮灯时间（10ms倍数时间）
          off_length: 100ms # 灭灯时间（10ms倍数时间）
          count: 3 # 闪烁次数

# 灯光效果示例
button:
  - platform: template
    name: "breathing"
    on_press:
      - light.turn_on: # 打开蓝色呼吸灯
          id: fpm383c_id
          red: 0
          green: 0
          blue: 100%
          effect: Breathing
```
</details>

> 可用动作
> - fpm383c.register 注册新指纹
> - fpm383c.clear 删除已注册指纹
> - fpm383c.cancel 取消指纹匹配或注册
> - fpm383c.reset 重启指纹模块

### DL/T 645
> 支持符合DL/T 645-2007规范的电表
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ dlt645 ]

remote_transmitter:
  pin: GPIO4
  carrier_duty_percent: 50%

remote_receiver:
  pin:  GPIO5

dlt645:
  address: 123456789012 #可选，12位电表地址，一般印刷于电表上。不配置可自动获取。
  power:
    name: "Power" #瞬时总有功功率
    update_interval: 10s #更新间隔
  power_a:
    name: "Power A" #瞬时A相有功功率
    update_interval: 10s #更新间隔
  power_b:
    name: "Power B" #瞬时B相有功功率
    update_interval: 10s #更新间隔
  power_c:
    name: "Power C" #瞬时C相有功功率
    update_interval: 10s #更新间隔
  energy:
    name: "Energy" #当前组合有功总电能
    update_interval: 30s #更新间隔
  energy_a:
    name: "Energy A" #当前A相正向有功电能
    update_interval: 30s #更新间隔
  energy_b:
    name: "Energy B" #当前B相正向有功电能
    update_interval: 30s #更新间隔
  energy_c:
    name: "Energy C" #当前C相正向有功电能
    update_interval: 30s #更新间隔
```
</details>

### Telnet
> tcp转发串口
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ telnet ]

uart:
  rx_pin: RX
  tx_pin: TX
  baud_rate: 115200
  rx_buffer_size: 1kB

telnet:
  port: 23
```
</details>

### PTX_YK1_QMIMB
> 支持平头熊无线蓝牙开关（型号：PTX_YK1_QMIMB）
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ ptx_yk1 ]

esp32_ble_tracker:
  scan_parameters:
    interval: 300ms
    window: 300ms
    active: false
  on_ble_advertise: # 调试用途，按下无线按钮可查找蓝牙开关MAC地址
    then:
      - lambda: |-
          for (auto data : x.get_manufacturer_datas()) {
            if (data.uuid == esp32_ble_tracker::ESPBTUUID::from_uint16(0x5348)) {
              ESP_LOGD("ptx_yk1", "Found ptx_yk1_qmimb: %s", x.address_str().c_str());
              return;
            }
          }

binary_sensor:
  - platform: ptx_yk1
    mac_address: "XX:XX:XX:XX:XX:XX" # 蓝牙开关MAC地址，可使用上面的调试代码查找新开关的MAC地址
    name: "BLE Button"
    timeout: 300ms # 可选，蓝牙BLE信号接收的超时时间。取决于信号环境和esp32_ble_tracker的scan_parameters配置。如果时间设置过短，可能会导致长按误判为短按；而时间设置过长则会延迟蓝牙开关的松开判定。
    on_multi_click: # 单击、双击、长按示例配置
      - timing:
          - ON for at most 2s # 短按须少于2秒
          - OFF for at least 1s # 1秒内无按下动作，判定为单击
        then:
          - logger.log: "Clicked" # 配置单击动作
      - timing:
          - ON for at most 2s # 第一次短按须少于2秒
          - OFF for 0s to 1s # 两次短按须间隔1秒以内
          - ON for at most 2s # 第二次短按须少于2秒
          - OFF for at least 0s
        then:
          - logger.log: "Double-Clicked" # 配置双击动作
      - timing:
          - ON for at least 2s # 长按须大于2秒
        then:
          - logger.log: "Long-Pressed" # 配置长按动作
```
</details>

注意：这款蓝牙开关可能无法识别快速双击。

### 小米通用遥控
> 支持小米通用遥控协议
<details>
  <summary>配置示例</summary>

```yaml
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ xiaomi_remote ]

esp32_ble_tracker:
  scan_parameters:
    interval: 300ms
    window: 300ms
    active: false

xiaomi_remote:
  mac_address: XX:XX:XX:XX:XX:XX
  bindkey: "0000000000000000000000000000000" #token，可选，不配置为不加密
  press: #按下动作传感器，可配置一到多个，示例为一个
    name: button binary
    button: 0 # 可选，配置按钮编号（0-9），默认任意按钮
  action: #按键动作传感器（0:无动作 1:单击 2：双击 3：三击 99：长按），可配置一到多个，示例为多个
    - name: button 0 action
      button: 0 # 可选，配置按钮编号（0-9），默认任意按钮
    - name: button 1 action
      button: 1
  action_text: #按键动作文本传感器（Idle:无动作 Click:单击 Double-click：双击 Triple-click：三击 Long press：长按），可配置一到多个，示例为多个
    name: button text
    button: 0 # 可选，配置按钮编号（0-9），默认任意按钮
  on_click: # 单击示例
    - button: 0
      then:
        - logger.log: "单击0"
    - button: 1
      then:
        - logger.log: "单击1"
  on_double_click: # 双击示例
    then:
      - logger.log: "双击"
  on_triple_click: # 三击示例
    then:
      - logger.log: "三击"
  on_long_press: # 长按示例
    then:
      - logger.log: "长按"
```
</details>
