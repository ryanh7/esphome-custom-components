import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, binary_sensor, number
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_OCCUPANCY,
    DEVICE_CLASS_MOVING,
    DEVICE_CLASS_MOTION,
    CONF_THRESHOLD,
    CONF_MOTION,
    CONF_MIN_VALUE,
    CONF_MAX_VALUE,
    CONF_VALUE,
    CONF_STEP,
)

CONF_MOV = "moving"
CONF_OCC = "occupancy"

AUTO_LOAD = ["number"]
DEPENDENCIES = ["uart"]

cem5855h_ns = cg.esphome_ns.namespace("cem5855h")
CEM5855hComponent = cem5855h_ns.class_(
    "CEM5855hComponent", cg.Component, uart.UARTDevice
)
ThresholdMovingNumber = cem5855h_ns.class_(
    "ThresholdMovingNumber", number.Number
)
ThresholdOccupancyNumber = cem5855h_ns.class_(
    "ThresholdOccupancyNumber", number.Number
)

def validate_min_max(config):
    if config[CONF_MAX_VALUE] <= config[CONF_MIN_VALUE]:
        raise cv.Invalid("max_value must be greater than min_value")
    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CEM5855hComponent),
            cv.Optional(CONF_THRESHOLD, default={}): cv.All(
                cv.Schema(
                    {
                        cv.Optional(CONF_MOV, default=250): cv.Any(
                            cv.int_range(min=0, max=99999),
                            number.NUMBER_SCHEMA.extend(
                                {
                                    cv.GenerateID(): cv.declare_id(ThresholdMovingNumber),
                                    cv.Optional(CONF_VALUE, default=250): cv.float_,
                                    cv.Optional(CONF_MAX_VALUE, default=99999): cv.float_,
                                    cv.Optional(CONF_MIN_VALUE, default=10): cv.float_,
                                    cv.Optional(CONF_STEP, default=10): cv.positive_float,
                                }
                            ).extend(cv.COMPONENT_SCHEMA),
                            validate_min_max,
                        ),
                        cv.Optional(CONF_OCC, default=250): cv.Any(
                            cv.int_range(min=0, max=99999),
                            number.NUMBER_SCHEMA.extend(
                                {
                                    cv.GenerateID(): cv.declare_id(ThresholdOccupancyNumber),
                                    cv.Optional(CONF_VALUE, default=250): cv.float_,
                                    cv.Optional(CONF_MAX_VALUE, default=50000): cv.float_,
                                    cv.Optional(CONF_MIN_VALUE, default=10): cv.float_,
                                    cv.Optional(CONF_STEP, default=10): cv.positive_float,
                                }
                            ).extend(cv.COMPONENT_SCHEMA),
                            validate_min_max,
                        ),
                    }
                )
            ),
            cv.Optional(CONF_MOV): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_MOVING,
            ),
            cv.Optional(CONF_OCC): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_OCCUPANCY,
            ),
            cv.Optional(CONF_MOTION): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_MOTION,
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    threshold = config[CONF_THRESHOLD]

    config_number = threshold[CONF_MOV]
    if isinstance(config_number, int):
        cg.add(var.set_moving_threshold(config_number))
    else:
        cg.add(var.set_moving_threshold(config_number[CONF_VALUE]))
        number_moving = cg.new_Pvariable(config_number[CONF_ID])
        cg.add(number_moving.set_parent(var))
        await number.register_number(
            number_moving,
            config_number,
            min_value=config_number[CONF_MIN_VALUE],
            max_value=config_number[CONF_MAX_VALUE],
            step=config_number[CONF_STEP],
        )
        cg.add(number_moving.publish_state(config_number[CONF_VALUE]))

    config_number = threshold[CONF_OCC]
    if isinstance(config_number, int):
        cg.add(var.set_occupancy_threshold(config_number))
    else:
        cg.add(var.set_occupancy_threshold(config_number[CONF_VALUE]))
        number_moving = cg.new_Pvariable(config_number[CONF_ID])
        cg.add(number_moving.set_parent(var))
        await number.register_number(
            number_moving,
            config_number,
            min_value=config_number[CONF_MIN_VALUE],
            max_value=config_number[CONF_MAX_VALUE],
            step=config_number[CONF_STEP],
        )
        cg.add(number_moving.publish_state(config_number[CONF_VALUE]))


    if CONF_MOV in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_MOV])
        cg.add(var.set_moving_sensor(sens))
    if CONF_OCC in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_OCC])
        cg.add(var.set_occupancy_sensor(sens))
    if CONF_MOTION in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_MOTION])
        cg.add(var.set_motion_sensor(sens))
