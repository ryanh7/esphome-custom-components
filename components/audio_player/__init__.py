import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import spi, remote_base, remote_receiver, remote_transmitter
from esphome.const import (
    CONF_ID,
    CONF_URL,
    CONF_PIN,
    CONF_BUFFER_SIZE
)

CODEOWNERS = ["@ryan"]

CONF_VOLUME = "volume"

audio_player_ns = cg.esphome_ns.namespace("audio_player")
AudioPlayerComponent = audio_player_ns.class_(
    "AudioPlayerComponent", cg.Component
)

PlayAudioAction = audio_player_ns.class_(
    "PlayAudioAction", automation.Action
)

StopAudioAction = audio_player_ns.class_(
    "StopAudioAction", automation.Action
)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AudioPlayerComponent),
            cv.Optional(CONF_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_VOLUME, default=100): cv.All(
                cv.percentage_int, cv.Range(min=1, max=500)
            ),
            cv.Optional(CONF_BUFFER_SIZE, default=1024): cv.int_range(min=256, max=81920)
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add_library("esp8266Audio", "1.9.1")
    await cg.register_component(var, config)

    cg.add(var.set_volume_percent(config[CONF_VOLUME]))
    cg.add(var.set_buffer_size(config[CONF_BUFFER_SIZE]))

    if CONF_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_PIN])
        cg.add(var.set_pin(pin))


PLAY_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(AudioPlayerComponent),
        cv.Required(CONF_URL): cv.templatable(cv.string),
    }
)


@automation.register_action(
    "audio_player.play", PlayAudioAction, PLAY_SCHEMA
)
async def play_audio_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    template_ = await cg.templatable(config[CONF_URL], args, cg.std_string)
    cg.add(var.set_url(template_))
    return var

STOP_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(AudioPlayerComponent),
    }
)


@automation.register_action(
    "audio_player.stop", StopAudioAction, STOP_SCHEMA
)
async def stop_audio_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    return var
