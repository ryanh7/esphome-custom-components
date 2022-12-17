import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import (
    remote_transmitter,
    remote_receiver,
    remote_base,
    sensor
)
from esphome.const import (
    CONF_ID,
    CONF_INTERVAL,
    CONF_POWER,
    CONF_ENERGY,
    UNIT_KILOWATT,
    UNIT_KILOWATT_HOURS,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_ENERGY,
    STATE_CLASS_MEASUREMENT,
)
from esphome.components.remote_base import CONF_RECEIVER_ID, CONF_TRANSMITTER_ID

AUTO_LOAD = ["sensor", "remote_base"]

dlt645_ns = cg.esphome_ns.namespace("dlt645")
DLT645Component = dlt645_ns.class_(
    "DLT645Component", cg.Component, remote_base.RemoteReceiverListener
)
DLT645Sensor = dlt645_ns.class_(
    "DLT645Sensor", sensor.Sensor
)

CONF_ADDRESS = "address"
CONF_ENERGY_A = "energy_a"
CONF_ENERGY_B = "energy_b"
CONF_ENERGY_C = "energy_c"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DLT645Component),
            cv.GenerateID(CONF_TRANSMITTER_ID): cv.use_id(
                remote_transmitter.RemoteTransmitterComponent
            ),
            cv.GenerateID(CONF_RECEIVER_ID): cv.use_id(
                remote_receiver.RemoteReceiverComponent
            ),
            cv.Optional(CONF_ADDRESS): cv.string,
            cv.Optional(CONF_POWER): sensor.sensor_schema(
                DLT645Sensor,
                unit_of_measurement=UNIT_KILOWATT,
                accuracy_decimals=4,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERVAL, default="5s"): cv.positive_time_period_microseconds,
                }
            ),
            cv.Optional(CONF_ENERGY): sensor.sensor_schema(
                DLT645Sensor,
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERVAL, default="10s"): cv.positive_time_period_microseconds,
                }
            ),
            cv.Optional(CONF_ENERGY_A): sensor.sensor_schema(
                DLT645Sensor,
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERVAL, default="10s"): cv.positive_time_period_microseconds,
                }
            ),
            cv.Optional(CONF_ENERGY_B): sensor.sensor_schema(
                DLT645Sensor,
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERVAL, default="10s"): cv.positive_time_period_microseconds,
                }
            ),
            cv.Optional(CONF_ENERGY_C): sensor.sensor_schema(
                DLT645Sensor,
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERVAL, default="10s"): cv.positive_time_period_microseconds,
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(receiver.register_listener(var))

    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))

    if CONF_ADDRESS in config:
        cg.add(var.set_address(config[CONF_ADDRESS]))

    if CONF_POWER in config:
        sensor_config = config[CONF_POWER]
        var_sensor = cg.new_Pvariable(sensor_config[CONF_ID])
        cg.add(var_sensor.set_interval(sensor_config[CONF_INTERVAL]))
        await sensor.register_sensor(var_sensor, sensor_config)
        cg.add(var.set_power_sensor(var_sensor))
    if CONF_ENERGY in config:
        sensor_config = config[CONF_ENERGY]
        var_sensor = cg.new_Pvariable(sensor_config[CONF_ID])
        cg.add(var_sensor.set_interval(sensor_config[CONF_INTERVAL]))
        await sensor.register_sensor(var_sensor, sensor_config)
        cg.add(var.set_energy_sensor(var_sensor))
    if CONF_ENERGY_A in config:
        sensor_config = config[CONF_ENERGY_A]
        var_sensor = cg.new_Pvariable(sensor_config[CONF_ID])
        cg.add(var_sensor.set_interval(sensor_config[CONF_INTERVAL]))
        await sensor.register_sensor(var_sensor, sensor_config)
        cg.add(var.set_energy_a_sensor(var_sensor))
    if CONF_ENERGY_B in config:
        sensor_config = config[CONF_ENERGY_B]
        var_sensor = cg.new_Pvariable(sensor_config[CONF_ID])
        cg.add(var_sensor.set_interval(sensor_config[CONF_INTERVAL]))
        await sensor.register_sensor(var_sensor, sensor_config)
        cg.add(var.set_energy_b_sensor(var_sensor))
    if CONF_ENERGY_C in config:
        sensor_config = config[CONF_ENERGY_C]
        var_sensor = cg.new_Pvariable(sensor_config[CONF_ID])
        cg.add(var_sensor.set_interval(sensor_config[CONF_INTERVAL]))
        await sensor.register_sensor(var_sensor, sensor_config)
        cg.add(var.set_energy_c_sensor(var_sensor))
