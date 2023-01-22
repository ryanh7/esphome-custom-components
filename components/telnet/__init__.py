import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import (
    CONF_ID,
    CONF_PORT
)

DEPENDENCIES = ["uart", "network"]

telnet_ns = cg.esphome_ns.namespace("telnet")
TelnetComponent = telnet_ns.class_(
    "TelnetComponent", cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TelnetComponent),
            cv.Optional(CONF_PORT, default=23): cv.port,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_port(config[CONF_PORT]))
