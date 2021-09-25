# esphome-custom-components
esphome的一些新增组件支持
## 如何使用
请参考[esphome文档](https://www.esphome.io/components/external_components.html)。
```yaml
#引用组件库示例
external_components:
  - source:
      type: git
      url: https://github.com/ryanh7/esphome-custom-components
```

## 组件列表
* esp8266 通过http流播放wav
```yaml
audio_player:
  volume: 100% #可选，基准音量
  buffer_size: 1024 #可选，http流缓冲区大小

text_sensor:
  - platform: audio_player #示例，绑定一个传感器用于表明播放器状态
    name: "Media Player Status"
```
* cc1101 射频收发
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
* 小米电动牙刷T500
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
* ssw_tds
>支持型号为ssw-tds-2u的一款基于urat协议的2路带水温tds检测仪。可输出双路tds及一路水温。
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