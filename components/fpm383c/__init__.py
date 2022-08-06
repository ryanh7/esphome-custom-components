import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import (
    CONF_ID,
)

DEPENDENCIES = ["uart"]

fpm383c_ns = cg.esphome_ns.namespace("fpm383c")
FPM383cComponent = fpm383c_ns.class_(
    "FPM383cComponent", cg.Component, uart.UARTDevice
)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(FPM383cComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)