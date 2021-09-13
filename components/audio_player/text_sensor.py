import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_ICON, ICON_TIMELAPSE
from . import audio_player_ns, AudioPlayerComponent

StatusListener = audio_player_ns.class_(
    "StatusListener", cg.Component
)

AudioPlayerStatusTextSensor = audio_player_ns.class_(
    "AudioPlayerStatusTextSensor", text_sensor.TextSensor, cg.Component, StatusListener
)

CONF_AUDIO_PLAYER_ID = "audio_player_id"

CONFIG_SCHEMA = text_sensor.TEXT_SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(AudioPlayerStatusTextSensor),
        cv.GenerateID(CONF_AUDIO_PLAYER_ID): cv.use_id(AudioPlayerComponent),
        cv.Optional(CONF_ICON, default=ICON_TIMELAPSE): text_sensor.icon,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await text_sensor.register_text_sensor(var, config)
    await cg.register_component(var, config)
    player = await cg.get_variable(config[CONF_AUDIO_PLAYER_ID])
    cg.add(player.register_listener(var))
