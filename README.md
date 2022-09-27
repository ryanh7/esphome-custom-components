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
    threshold: #（可选）触发移动的阈值，支持数字或者Number(esphome)
      moving: 250 #示例配置-数字
      occupancy: #示例配置-esphome number
        name: "occ"
        value: 250 #阈值
        min_value: 10 #可调节的最小阈值
        max_value: 99999 #可调节的最大阈值
        step: 10 #单步调节量

    moving: #（可选）有人移动传感器
      name: mov
      filters:
        - delayed_off: 3s
    occupancy: #（可选）有人静止传感器
      name: occ
      filters:
        - delayed_off: 3s
    motion: #（可选）人在传感器
      name: motion
      filters:
        - delayed_off: 3s #设置一个合适的状态持续时间
```
