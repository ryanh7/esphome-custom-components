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
    CONF_UPDATE_INTERVAL,
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
CONF_POWER_A = "power_a"
CONF_POWER_B = "power_b"
CONF_POWER_C = "power_c"
CONF_ENERGY_A = "energy_a"
CONF_ENERGY_B = "energy_b"
CONF_ENERGY_C = "energy_c"


class SensorSetting:
    def __init__(self, name, id, format) -> None:
        self.name = name,
        self.id = id,
        self.format = format


Sensors = {
    CONF_POWER: SensorSetting("Power", 0x02030000, 1.2),
    CONF_POWER_A: SensorSetting("Power A", 0x02030100, 1.2),
    CONF_POWER_B: SensorSetting("Power B", 0x02030200, 1.2),
    CONF_POWER_C: SensorSetting("Power C", 0x02030300, 1.2),
    CONF_ENERGY: SensorSetting("Energy", 0x00000000, 3.1),
    CONF_ENERGY_A: SensorSetting("Energy A", 0x00150000, 3.1),
    CONF_ENERGY_B: SensorSetting("Energy B", 0x00290000, 3.1),
    CONF_ENERGY_C: SensorSetting("Energy C", 0x003D0000, 3.1),
}

CONFIG_POWER_SCHEMA = sensor.sensor_schema(
    DLT645Sensor,
    unit_of_measurement=UNIT_KILOWATT,
    accuracy_decimals=4,
    device_class=DEVICE_CLASS_POWER,
    state_class=STATE_CLASS_MEASUREMENT,
).extend(
    {
        cv.Optional(CONF_UPDATE_INTERVAL, default="10s"): cv.positive_time_period_microseconds,
    }
)

CONFIG_ENERGY_SCHEMA = sensor.sensor_schema(
    DLT645Sensor,
    unit_of_measurement=UNIT_KILOWATT_HOURS,
    accuracy_decimals=2,
    device_class=DEVICE_CLASS_ENERGY,
    state_class=STATE_CLASS_MEASUREMENT,
).extend(
    {
        cv.Optional(CONF_UPDATE_INTERVAL, default="30s"): cv.positive_time_period_microseconds,
    }
)

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
            cv.Optional(CONF_POWER): CONFIG_POWER_SCHEMA,
            cv.Optional(CONF_POWER_A): CONFIG_POWER_SCHEMA,
            cv.Optional(CONF_POWER_B): CONFIG_POWER_SCHEMA,
            cv.Optional(CONF_POWER_C): CONFIG_POWER_SCHEMA,
            cv.Optional(CONF_ENERGY): CONFIG_ENERGY_SCHEMA,
            cv.Optional(CONF_ENERGY_A): CONFIG_ENERGY_SCHEMA,
            cv.Optional(CONF_ENERGY_B): CONFIG_ENERGY_SCHEMA,
            cv.Optional(CONF_ENERGY_C): CONFIG_ENERGY_SCHEMA,
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

    for config_type, setting in Sensors.items():
        if config_type in config:
            sensor_config = config[config_type]
            var_sensor = cg.new_Pvariable(
                sensor_config[CONF_ID], setting.name, setting.id, setting.format)
            cg.add(var_sensor.set_interval(sensor_config[CONF_UPDATE_INTERVAL]))
            await sensor.register_sensor(var_sensor, sensor_config)
            cg.add(var.register_sensor(var_sensor))
