import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, text_sensor, esp32_ble_tracker
from esphome.const import (
    CONF_BATTERY_LEVEL,
    CONF_MAC_ADDRESS,
    STATE_CLASS_MEASUREMENT,
    ICON_EMPTY,
    UNIT_PERCENT,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_SMOKE,
    DEVICE_CLASS_EMPTY,
    CONF_ID,
    CONF_BINDKEY,
)

CONF_STATUS = "status"
CONF_STATUS_TEXT = "status_text"
CONF_ALERT = "alert"

DEPENDENCIES = ["esp32_ble_tracker"]
AUTO_LOAD = ["binary_sensor", "text_sensor"]

xiaomi_smoke_detector_ns = cg.esphome_ns.namespace("xiaomi_smoke_detector")
XiaomiSmokeDetector = xiaomi_smoke_detector_ns.class_(
    "XiaomiSmokeDetector", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

StatusListener = xiaomi_smoke_detector_ns.class_(
    "StatusListener"
)

SmokerDetectorStatusSensor = xiaomi_smoke_detector_ns.class_(
    "SmokerDetectorStatusSensor", sensor.Sensor, StatusListener
)

SmokerDetectorStatusTextSensor = xiaomi_smoke_detector_ns.class_(
    "SmokerDetectorStatusTextSensor", text_sensor.TextSensor, StatusListener
)

SmokerDetectorAlarmSensor = xiaomi_smoke_detector_ns.class_(
    "SmokerDetectorAlarmSensor", binary_sensor.BinarySensor, StatusListener
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiSmokeDetector),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Required(CONF_BINDKEY): cv.bind_key,
            cv.Optional(CONF_ALERT): binary_sensor.binary_sensor_schema(
                SmokerDetectorAlarmSensor,
                device_class=DEVICE_CLASS_SMOKE,
            ).extend(cv.COMPONENT_SCHEMA),
            cv.Optional(CONF_STATUS): sensor.sensor_schema(
                SmokerDetectorStatusSensor,
                icon="mdi:smoke-detector",
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_STATUS_TEXT): text_sensor.text_sensor_schema(
                SmokerDetectorStatusTextSensor,
                icon="mdi:smoke-detector",
            ).extend(cv.COMPONENT_SCHEMA),
            cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_EMPTY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_bindkey(config[CONF_BINDKEY]))
    
    if CONF_ALERT in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ALERT])
        cg.add(var.register_listener(sens))
    if CONF_STATUS in config:
        sens = await sensor.new_sensor(config[CONF_STATUS])
        cg.add(var.register_listener(sens))
    if CONF_STATUS_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STATUS_TEXT])
        cg.add(var.register_listener(sens))
    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level(sens))
