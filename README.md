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
* ssw_tds
>支持型号为ssw-tds-2u的一款基于urat协议的2路带水温tds检测仪。可输出双路tds及一路水温。
```yml
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