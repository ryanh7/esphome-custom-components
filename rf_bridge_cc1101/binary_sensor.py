import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, remote_base
from esphome.const import CONF_NAME, CONF_ID, CONF_CODE, CONF_PROTOCOL
from . import RFBridgeComponent

CONFIG_RF_BRIDGE_CC1101_ID = "rf_bridge_cc1101_id"

DEPENDENCIES = ["rf_bridge_cc1101"]

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(remote_base.RCSwitchRawReceiver),
        cv.GenerateID(CONFIG_RF_BRIDGE_CC1101_ID): cv.use_id(RFBridgeComponent),
        cv.Required(CONF_CODE): cv.templatable(cv.string),
        cv.Optional(CONF_PROTOCOL, default=1): cv.int_range(min=1, max=8),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_name(config[CONF_NAME]))
    await binary_sensor.register_binary_sensor(var, config)

    cg.add(var.set_protocol(remote_base.build_rc_switch_protocol(config[CONF_PROTOCOL])))
    cg.add(var.set_code(config[CONF_CODE]))

    receicer = await cg.get_variable(config[CONFIG_RF_BRIDGE_CC1101_ID])
    cg.add(receicer.register_listener(var))
