import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import (
    CONF_ID,
    CONF_TEMPERATURE,
    UNIT_PARTS_PER_MILLION,
    ICON_WATER_PERCENT,
    ICON_THERMOMETER,
    DEVICE_CLASS_EMPTY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

CONF_SOURCE_TDS = "source_tds"
CONF_CLEAN_TDS = "clean_tds"

DEPENDENCIES = ["uart"]

ssw_tds_ns = cg.esphome_ns.namespace("ssw_tds")
SSWTDSComponent = ssw_tds_ns.class_(
    "SSWTDSComponent", cg.PollingComponent, uart.UARTDevice
)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SSWTDSComponent),
            cv.Optional(CONF_SOURCE_TDS): sensor.sensor_schema(
                UNIT_PARTS_PER_MILLION, ICON_WATER_PERCENT, 0, DEVICE_CLASS_EMPTY, STATE_CLASS_MEASUREMENT
            ),
            cv.Optional(CONF_CLEAN_TDS): sensor.sensor_schema(
                UNIT_PARTS_PER_MILLION, ICON_WATER_PERCENT, 0, DEVICE_CLASS_EMPTY, STATE_CLASS_MEASUREMENT
            ),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                UNIT_CELSIUS, ICON_THERMOMETER, 0, DEVICE_CLASS_TEMPERATURE, STATE_CLASS_MEASUREMENT
            ),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_SOURCE_TDS in config:
        sens = await sensor.new_sensor(config[CONF_SOURCE_TDS])
        cg.add(var.set_source_tds_sensor(sens))
    if CONF_CLEAN_TDS in config:
        sens = await sensor.new_sensor(config[CONF_CLEAN_TDS])
        cg.add(var.set_clean_tds_sensor(sens))
    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))