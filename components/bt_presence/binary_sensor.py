import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, esp32_bt_tracker
from esphome.const import CONF_MAC_ADDRESS, CONF_ID

DEPENDENCIES = ["esp32_bt_tracker"]

ble_presence_ns = cg.esphome_ns.namespace("bt_presence")
BLEPresenceDevice = ble_presence_ns.class_(
    "BTPresenceDevice",
    binary_sensor.BinarySensor,
    cg.Component,
    esp32_bt_tracker.ESPBTDeviceListener,
)

CONFIG_SCHEMA = cv.All(
    binary_sensor.BINARY_SENSOR_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(BLEPresenceDevice),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
        }
    )
    .extend(esp32_bt_tracker.ESP_BT_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_bt_tracker.register_bt_device(var, config)
    await binary_sensor.register_binary_sensor(var, config)

    if CONF_MAC_ADDRESS in config:
        cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))

