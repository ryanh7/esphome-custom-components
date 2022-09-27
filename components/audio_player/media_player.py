import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_BUFFER_SIZE
)
from esphome.core import CORE

CODEOWNERS = ["@ryan"]

AUTO_LOAD = ["network"]

CONF_VOLUME = "volume"
CONF_I2S = "i2s"
CONF_I2S_NO_DAC = "i2sNoDAC"
CONF_BCLK = "bclk"
CONF_WCLK = "wclk"
CONF_DOUT = "dout"

audio_player_ns = cg.esphome_ns.namespace("audio_player")
AudioMediaPlayer = audio_player_ns.class_(
    "AudioMediaPlayer", cg.Component, media_player.MediaPlayer
)

PlayerOutputI2SNoDAC = audio_player_ns.class_(
    "PlayerOutputI2SNoDAC"
)

PlayerOutputI2S = audio_player_ns.class_(
    "PlayerOutputI2S"
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AudioMediaPlayer),
            cv.Optional(CONF_VOLUME, default=100): cv.All(
                cv.percentage_int, cv.Range(min=1, max=1000)
            ),
            cv.SplitDefault(
                CONF_BUFFER_SIZE, esp32="10240B", esp8266="1024B"
            ): cv.validate_bytes,
            cv.Optional(CONF_I2S_NO_DAC): cv.All(
                cv.Schema(
                    {
                        cv.GenerateID(): cv.declare_id(PlayerOutputI2SNoDAC),
                    }
                ),
            ),
            cv.Optional(CONF_I2S): cv.All(
                cv.Schema(
                    {
                        cv.GenerateID(): cv.declare_id(PlayerOutputI2S),
                        cv.Required(CONF_BCLK): pins.internal_gpio_output_pin_schema,
                        cv.Required(CONF_WCLK): pins.internal_gpio_output_pin_schema,
                        cv.Required(CONF_DOUT): pins.internal_gpio_output_pin_schema
                    }
                ),
            )
        }
    )
    .extend(media_player.MEDIA_PLAYER_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    cv.has_exactly_one_key(CONF_I2S_NO_DAC, CONF_I2S)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    cg.add_library("SPI", None)
    if CORE.is_esp32:
        cg.add_library("WiFi", None)
        cg.add_library("WiFiClientSecure", None)
        cg.add_library("FS", None)
    if CORE.is_esp8266:
        cg.add_library("ESP8266SdFat", None)
        cg.add_library("SDFS", None)
    cg.add_library("ESP8266Audio", "1.9.7")

    await cg.register_component(var, config)
    await media_player.register_media_player(var, config)

    cg.add(var.set_base_volume(config[CONF_VOLUME] / 100))
    cg.add(var.set_buffer_size(config[CONF_BUFFER_SIZE]))

    if CONF_I2S_NO_DAC in config:
        params = config[CONF_I2S_NO_DAC]
        output = cg.new_Pvariable(params[CONF_ID])
    
    elif CONF_I2S in config:
        params = config[CONF_I2S]
        output = cg.new_Pvariable(params[CONF_ID])
        pin_blck = await cg.gpio_pin_expression(params[CONF_BCLK])
        pin_wclk = await cg.gpio_pin_expression(params[CONF_WCLK])
        pin_dout = await cg.gpio_pin_expression(params[CONF_DOUT])
        cg.add(output.set_pins(pin_blck, pin_wclk, pin_dout))
    
    cg.add(var.set_output(output))
    cg.add(var.set_volume_controller(output))
    cg.add(var.set_ext_info(output))
