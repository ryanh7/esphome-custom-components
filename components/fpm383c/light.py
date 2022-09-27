import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID, CONF_GAMMA_CORRECT, CONF_DEFAULT_TRANSITION_LENGTH
from . import FPM383cComponent

fpm383c_ns = cg.esphome_ns.namespace("fpm383c")
FPM383cLightOutput = fpm383c_ns.class_("FPM383cLightOutput", light.LightOutput)

CONF_FPM383C_ID = "fpm383c_id"

CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(FPM383cLightOutput),
        cv.GenerateID(CONF_FPM383C_ID): cv.use_id(FPM383cComponent),
        cv.Optional(CONF_GAMMA_CORRECT, default=1): cv.positive_float,
        cv.Optional(
            CONF_DEFAULT_TRANSITION_LENGTH, default="0s"
        ): cv.positive_time_period_milliseconds,
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    fpm383c = await cg.get_variable(config[CONF_FPM383C_ID])
    cg.add(var.set_parent(fpm383c))
