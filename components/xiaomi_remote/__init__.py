import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
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
    CONF_TRIGGER_ID
)

CONF_BUTTON = "button"
CONF_PRESS = "press"
CONF_ACTION = "action"
CONF_ACTION_TEXT = "action_text"
CONF_ON_CLICK = "on_click"
CONF_ON_DOUBLE_CLICK = "on_double_click"
CONF_ON_TRIPLE_CLICK = "on_triple_click"
CONF_ON_LONG_PRESS = "on_long_press"

DEPENDENCIES = ["esp32_ble_tracker"]
AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor"]
MULTI_CONF = True

xiaomi_remote_ns = cg.esphome_ns.namespace("xiaomi_remote")
XiaomiRemote = xiaomi_remote_ns.class_(
    "XiaomiRemote", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

ButtonListener = xiaomi_remote_ns.class_(
    "ButtonListener"
)

RemoteBinarySensor = xiaomi_remote_ns.class_(
    "RemoteBinarySensor", binary_sensor.BinarySensor, ButtonListener
)

RemoteSensor = xiaomi_remote_ns.class_(
    "RemoteSensor", sensor.Sensor, ButtonListener
)

RemoteTextSensor = xiaomi_remote_ns.class_(
    "RemoteTextSensor", text_sensor.TextSensor, ButtonListener
)

ClickTrigger = xiaomi_remote_ns.class_(
    "ClickTrigger", automation.Trigger.template()
)

DoubleClickTrigger = xiaomi_remote_ns.class_(
    "DoubleClickTrigger", automation.Trigger.template()
)

TripleClickTrigger = xiaomi_remote_ns.class_(
    "TripleClickTrigger", automation.Trigger.template()
)

LongPressTrigger = xiaomi_remote_ns.class_(
    "LongPressTrigger", automation.Trigger.template()
)

def trigger_schema(trigger): 
    return automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                trigger
            ),
            cv.Optional(CONF_BUTTON): cv.int_range(min=0, max=9),
        }
    )

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XiaomiRemote),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Optional(CONF_BINDKEY): cv.bind_key,
            cv.Optional(CONF_PRESS): cv.ensure_list(
                binary_sensor.binary_sensor_schema(
                    RemoteBinarySensor,
                    device_class=DEVICE_CLASS_SMOKE,
                ).extend(
                    {
                        cv.Optional(CONF_BUTTON): cv.int_range(min=0, max=9),
                    }
                )
            ),
            cv.Optional(CONF_ACTION): cv.ensure_list(
                sensor.sensor_schema(
                    RemoteSensor,
                    icon="mdi:counter",
                    accuracy_decimals=1,
                    device_class=DEVICE_CLASS_EMPTY,
                    state_class=STATE_CLASS_MEASUREMENT,
                ).extend(
                    {
                        cv.Optional(CONF_BUTTON): cv.int_range(min=0, max=9),
                    }
                )
            ),
            cv.Optional(CONF_ACTION_TEXT): cv.ensure_list(
                text_sensor.text_sensor_schema(
                    RemoteTextSensor,
                ).extend(
                    {
                        cv.Optional(CONF_BUTTON): cv.int_range(min=0, max=9),
                    }
                )
            ),
            cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_EMPTY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ON_CLICK): trigger_schema(ClickTrigger),
            cv.Optional(CONF_ON_DOUBLE_CLICK): trigger_schema(DoubleClickTrigger),
            cv.Optional(CONF_ON_TRIPLE_CLICK): trigger_schema(TripleClickTrigger),
            cv.Optional(CONF_ON_LONG_PRESS): trigger_schema(LongPressTrigger),
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

    if CONF_BINDKEY in config:
        cg.add(var.set_bindkey(config[CONF_BINDKEY]))
    
    for conf in config.get(CONF_PRESS, []):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.register_listener(sens))
        if CONF_BUTTON in conf:
            cg.add(sens.set_button_index(conf[CONF_BUTTON]))
    for conf in config.get(CONF_ACTION, []):
        sens = await sensor.new_sensor(conf)
        cg.add(var.register_listener(sens))
        if CONF_BUTTON in conf:
            cg.add(sens.set_button_index(conf[CONF_BUTTON]))
    for conf in config.get(CONF_ACTION_TEXT, []):
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.register_listener(sens))
        if CONF_BUTTON in conf:
            cg.add(sens.set_button_index(conf[CONF_BUTTON]))

    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level(sens))

    
    for trigger_key in [CONF_ON_CLICK, CONF_ON_DOUBLE_CLICK, CONF_ON_TRIPLE_CLICK, CONF_ON_LONG_PRESS]:
        for conf in config.get(trigger_key, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
            cg.add(var.register_listener(trigger))
            if CONF_BUTTON in conf:
                cg.add(trigger.set_button_index(conf[CONF_BUTTON]))
            await automation.build_automation(trigger, [], conf)