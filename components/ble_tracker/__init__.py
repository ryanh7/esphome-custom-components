import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker
from esphome.const import (
    CONF_ID,
)

DEPENDENCIES = ["esp32_ble_tracker", "network"]

api_ns = cg.esphome_ns.namespace("api")
CustomAPIDevice = api_ns.class_("CustomAPIDevice")
ble_tracker_ns = cg.esphome_ns.namespace("ble_tracker")
BLETracker = ble_tracker_ns.class_(
    "BLETracker", cg.PollingComponent, CustomAPIDevice, esp32_ble_tracker.ESPBTDeviceListener
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BLETracker),
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("500ms")),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    