import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, esp32_bt_tracker
from esphome.const import (
    CONF_MAC_ADDRESS,
    CONF_ID,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    STATE_CLASS_MEASUREMENT,
    UNIT_DECIBEL,
    ICON_EMPTY,
)

DEPENDENCIES = ["esp32_bt_tracker"]

ble_rssi_ns = cg.esphome_ns.namespace("bt_rssi")
BLERSSISensor = ble_rssi_ns.class_(
    "BTRSSISensor", sensor.Sensor, cg.Component, esp32_bt_tracker.ESPBTDeviceListener
)

CONFIG_SCHEMA = cv.All(
    sensor.sensor_schema(
        UNIT_DECIBEL,
        ICON_EMPTY,
        0,
        DEVICE_CLASS_SIGNAL_STRENGTH,
        STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(): cv.declare_id(BLERSSISensor),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
        }
    )
    .extend(esp32_bt_tracker.ESP_BT_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_bt_tracker.register_bt_device(var, config)
    await sensor.register_sensor(var, config)

    if CONF_MAC_ADDRESS in config:
        cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))

