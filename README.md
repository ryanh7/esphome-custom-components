# esphome-custom-components
esphome的一些新增组件支持
## 如何使用
请参考[esphome文档](https://www.esphome.io/components/external_components.html)。
```yaml
#引用组件库示例
external_components:
  - source: github://ryanh7/esphome-custom-components
    components: [ example ]
```

## 组件列表
* audio_player
> esp8266 通过http流播放wav
```yaml
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
* rf_bridge_cc1101
> cc1101 射频收发
```yaml
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
* xiaomi_m1st500
> 小米电动牙刷T500
```yaml
#配置示例
esp32_ble_tracker:
sensor:
  - platform: xiaomi_m1st500
    mac_address: "F0:A9:97:FF:03:7E" #你的牙刷mac地址
    score:
      name: "Toothbrush Score" #刷牙分数，在一次完整的2分钟刷牙后得出
    battery_level:
       name: "Toothbrush Battery Level" #电池电量
```
* xiaomi_smoke_detector
> 小米烟雾报警器蓝牙版mcn02
```yaml
#配置示例
esp32_ble_tracker:
sensor:
  - platform: xiaomi_smoke_detector
    mac_address: "00:00:00:00:00:00" #烟雾报警器蓝牙mac地址
    bindkey: "0000000000000000000000000000000" #token
    alert: # 可选：报警 binary sensor
      name: "Smoke Detector Alert" 
    status: # 可选：报警器状态 sensor
      name: "Smoke Detector Status"
    status_text: # 可选：报警器文本状态 text sensor
      name: "Smoke Detector Status Text"
    battery_level: # 可选：报警器电量 sensor
      name: "Smoke Detector Battery"
```
* ssw_tds
> 支持型号为ssw-tds-2u的一款基于urat协议的2路带水温tds检测仪。可输出双路tds及一路水温。
```yaml
#配置示例
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

* cem5855h
> 支持某国产芯片的毫米波雷达，同样适用于LD1115H。只需要连接VCC,GND,TX(CEM5855H)->RX(MCU)。阈值配置参考[Number](https://esphome.io/components/number/index.html)
```yaml
#配置示例
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
* esp32_bt_tracker, bt_presence, bt_rssi
> 通过经典蓝牙扫描发现蓝牙设备，蓝牙设备须处于可被发现状态
```yaml
#配置示例
esp32_bt_tracker: #启用经典蓝牙扫描组件，控制台会以debug消息打印扫描到的未设置sensor的设备mac
  scan_parameters:
      duration: 5s #每次扫描持续时间，影响不大

binary_sensor: #示例，扫描到设备
  - platform: bt_presence
    mac_address: C4:C0:82:0F:E8:54 #安卓手机蓝牙mac地址
    name: "android"

sensor: #示例，显示设备信号强度
  - platform: bt_rssi
    mac_address: C4:C0:82:0F:E8:54 #安卓手机蓝牙mac地址
    name: "android"
```
* fpm383c
> 支持海凌科fpm383c电容式指纹模块
```yaml
#配置示例
uart: #指定串口
  id: uart_fpm383c
  baud_rate: 57600
  rx_pin: GPIO26
  tx_pin: GPIO25

fpm383c:
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
```
> 可用动作
> - fpm383c.register 注册新指纹
> - fpm383c.clear 删除已注册指纹
> - fpm383c.cancel 取消指纹匹配或注册
> - fpm383c.reset 重启指纹模块

* DL/T 645
> 支持符合DL/T 645-2007规范的电表
```yaml
#配置示例
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
* Telnet
> tcp转发串口
```yaml
#配置示例
uart:
  rx_pin: RX
  tx_pin: TX
  baud_rate: 115200
  rx_buffer_size: 1kB

telnet:
  port: 23
```