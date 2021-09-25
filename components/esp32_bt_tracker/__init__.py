import re
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import (
    CONF_ID,
    ESP_PLATFORM_ESP32,
    CONF_TRIGGER_ID,
    CONF_MAC_ADDRESS,
    CONF_NAME,
    CONF_DURATION
)

ESP_PLATFORMS = [ESP_PLATFORM_ESP32]

CONF_ESP32_BT_ID = "esp32_bt_id"
CONF_SCAN_PARAMETERS = "scan_parameters"
CONF_ON_BT_DISCOVERY = "on_bt_discovery"
esp32_bt_tracker_ns = cg.esphome_ns.namespace("esp32_bt_tracker")
ESP32BTTracker = esp32_bt_tracker_ns.class_("ESP32BTTracker", cg.Component, cg.Nameable)
ESPBTDeviceListener = esp32_bt_tracker_ns.class_("ESPBTDeviceListener")
ESPBTDevice = esp32_bt_tracker_ns.class_("ESPBTDevice")
ESPBTDeviceConstRef = ESPBTDevice.operator("ref").operator("const")
# Triggers
ESPBTAdvertiseTrigger = esp32_bt_tracker_ns.class_(
    "ESPBTAdvertiseTrigger", automation.Trigger.template(ESPBTDeviceConstRef)
)

def as_hex(value):
    return cg.RawExpression(f"0x{value}ULL")


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ESP32BTTracker),
        cv.Optional(CONF_NAME): cv.string,
        cv.Optional(CONF_SCAN_PARAMETERS, default={}): cv.All(
            cv.Schema(
                {
                    cv.Optional(
                        CONF_DURATION, default="5s"
                    ): cv.All(
                        cv.positive_time_period_seconds,
                        cv.Range(min=cv.TimePeriod(seconds=1), max=cv.TimePeriod(seconds=60))
                    ),
                }
            ),
        ),
        cv.Optional(CONF_ON_BT_DISCOVERY): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ESPBTAdvertiseTrigger),
                cv.Optional(CONF_MAC_ADDRESS): cv.mac_address,
            }
        )
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    params = config[CONF_SCAN_PARAMETERS]
    cg.add(var.set_scan_duration(params[CONF_DURATION]))
    if CONF_NAME in config:
        cg.add(var.set_name(config[CONF_NAME]))
    for conf in config.get(CONF_ON_BT_DISCOVERY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        if CONF_MAC_ADDRESS in conf:
            cg.add(trigger.set_address(conf[CONF_MAC_ADDRESS].as_hex))
        await automation.build_automation(trigger, [(ESPBTDeviceConstRef, "x")], conf)


ESP_BT_DEVICE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ESP32_BT_ID): cv.use_id(ESP32BTTracker),
    }
)

async def register_bt_device(var, config):
    paren = await cg.get_variable(config[CONF_ESP32_BT_ID])
    cg.add(paren.register_listener(var))
    return var
