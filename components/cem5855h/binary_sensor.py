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
ThresholdSensor = cem5855h_ns.class_(
    "ThresholdSensor", binary_sensor.BinarySensor
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


MOVING_NUMBER_SCHEMA = cv.Any(
    cv.int_range(min=0, max=99999),
    number.number_schema(ThresholdMovingNumber).extend(
        {
            cv.Optional(CONF_VALUE, default=250): cv.float_,
            cv.Optional(CONF_MAX_VALUE, default=3000): cv.float_,
            cv.Optional(CONF_MIN_VALUE, default=10): cv.float_,
            cv.Optional(CONF_STEP, default=10): cv.positive_float,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    validate_min_max,
)

OCCUPANCY_NUMBER_SCHEMA = cv.Any(
    cv.int_range(min=0, max=99999),
    number.number_schema(ThresholdOccupancyNumber).extend(
        {
            cv.Optional(CONF_VALUE, default=250): cv.float_,
            cv.Optional(CONF_MAX_VALUE, default=3000): cv.float_,
            cv.Optional(CONF_MIN_VALUE, default=10): cv.float_,
            cv.Optional(CONF_STEP, default=10): cv.positive_float,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    validate_min_max,
)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CEM5855hComponent),
            cv.Optional(CONF_MOV): cv.ensure_list(
                binary_sensor.binary_sensor_schema(
                    ThresholdSensor,
                    device_class=DEVICE_CLASS_MOVING,
                ).extend(
                    cv.Schema({
                        cv.Optional(CONF_THRESHOLD, default=250):  MOVING_NUMBER_SCHEMA
                    })
                )
            ),
            cv.Optional(CONF_OCC): cv.ensure_list(
                binary_sensor.binary_sensor_schema(
                    ThresholdSensor,
                    device_class=DEVICE_CLASS_OCCUPANCY,
                ).extend(
                    cv.Schema({
                        cv.Optional(CONF_THRESHOLD, default=250):  OCCUPANCY_NUMBER_SCHEMA
                    })
                )
            ),
            cv.Optional(CONF_MOTION): cv.ensure_list(
                binary_sensor.binary_sensor_schema(
                    ThresholdSensor,
                    device_class=DEVICE_CLASS_MOTION,
                ).extend(
                    cv.Schema({
                        cv.Optional(CONF_THRESHOLD, default={}): cv.All(
                            cv.Schema(
                                {
                                    cv.Optional(CONF_MOV, default=250): MOVING_NUMBER_SCHEMA,
                                    cv.Optional(CONF_OCC, default=250): OCCUPANCY_NUMBER_SCHEMA,
                                }
                            )
                        ),
                    })
                )
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def moving_threshold_to_code(sens, config_number):
    if isinstance(config_number, int):
        cg.add(sens.set_moving_threshold(config_number))
    else:
        cg.add(sens.set_moving_threshold(config_number[CONF_VALUE]))
        number_moving = cg.new_Pvariable(config_number[CONF_ID])
        cg.add(number_moving.set_parent(sens))
        await number.register_number(
            number_moving,
            config_number,
            min_value=config_number[CONF_MIN_VALUE],
            max_value=config_number[CONF_MAX_VALUE],
            step=config_number[CONF_STEP],
        )
        cg.add(number_moving.publish_state(config_number[CONF_VALUE]))


async def occupancy_threshold_to_code(sens, config_number):
    if isinstance(config_number, int):
        cg.add(sens.set_occupancy_threshold(config_number))
    else:
        cg.add(sens.set_occupancy_threshold(config_number[CONF_VALUE]))
        number_occupancy = cg.new_Pvariable(config_number[CONF_ID])
        cg.add(number_occupancy.set_parent(sens))
        await number.register_number(
            number_occupancy,
            config_number,
            min_value=config_number[CONF_MIN_VALUE],
            max_value=config_number[CONF_MAX_VALUE],
            step=config_number[CONF_STEP],
        )
        cg.add(number_occupancy.publish_state(config_number[CONF_VALUE]))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_MOV in config:
        for sensor_config in config[CONF_MOV]:
            sens = await binary_sensor.new_binary_sensor(sensor_config)
            cg.add(var.register_moving_sensor(sens))

            await moving_threshold_to_code(sens, sensor_config[CONF_THRESHOLD])

    if CONF_OCC in config:
        for sensor_config in config[CONF_OCC]:
            sens = await binary_sensor.new_binary_sensor(sensor_config)
            cg.add(var.register_occupancy_sensor(sens))

            await occupancy_threshold_to_code(sens, sensor_config[CONF_THRESHOLD])

    if CONF_MOTION in config:
        for sensor_config in config[CONF_MOTION]:
            sens = await binary_sensor.new_binary_sensor(sensor_config)
            cg.add(var.register_motion_sensor(sens))

            threshold = sensor_config[CONF_THRESHOLD]

            await moving_threshold_to_code(sens, threshold[CONF_MOV])
            await occupancy_threshold_to_code(sens, threshold[CONF_OCC])
