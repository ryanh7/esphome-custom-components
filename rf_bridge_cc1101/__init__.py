import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import spi, remote_base
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
    CONF_PROTOCOL,
    CONF_CODE,
    CONF_PIN,
    CONF_REPEAT,
    CONF_TIMES,
    CONF_WAIT_TIME,
    CONF_MODE,
    CONF_DUMP,
    CONF_FREQUENCY
)

AUTO_LOAD = ["remote_base"]
DEPENDENCIES = ["spi"]
CODEOWNERS = ["@ryan"]

rf_bridge_cc1101_ns = cg.esphome_ns.namespace("rf_bridge_cc1101")
RFBridgeComponent = rf_bridge_cc1101_ns.class_(
    "RFBridgeComponent", cg.Component, spi.SPIDevice
)


RFBridgeReceiverModeAction = rf_bridge_cc1101_ns.class_(
    "RFBridgeReceiverModeAction", automation.Action
)

RFBridgeTransmitterModeAction = rf_bridge_cc1101_ns.class_(
    "RFBridgeTransmitterModeAction", automation.Action
)

RFBridgeTransmitAction = rf_bridge_cc1101_ns.class_(
    "RFBridgeTransmitAction", automation.Action
)


CONF_ON_CODE_RECEIVED = "on_code_received"


MODE = {"receiver": 0, "transmitter": 1}

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(RFBridgeComponent),
            cv.Optional(CONF_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_MODE, default="receiver"): cv.enum(
                MODE, upper=False
            ),
            cv.Optional(CONF_FREQUENCY, default=433.92): cv.float_range(min=300, max=928),
            cv.Optional(CONF_DUMP, default=[]): remote_base.validate_dumpers,
            cv.Optional(CONF_ON_CODE_RECEIVED): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        remote_base.RCSwitchTrigger
                    ),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)


async def to_code(config):

    if CONF_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_PIN])
        var = cg.new_Pvariable(config[CONF_ID], pin)
    else:
        var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    cg.add(var.set_mode(config[CONF_MODE]))
    cg.add(var.set_frequency_mhz(config[CONF_FREQUENCY]))

    dumpers = await remote_base.build_dumpers(config[CONF_DUMP])
    for dumper in dumpers:
        cg.add(var.register_dumper(dumper))

    for conf in config.get(CONF_ON_CODE_RECEIVED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_listener(trigger))
        await automation.build_automation(trigger, [(remote_base.RCSwitchData, "x")], conf)


RF_BRIDGE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(RFBridgeComponent),
    }
)


@automation.register_action(
    "rf_bridge_cc1101.receiver_mode", RFBridgeReceiverModeAction, RF_BRIDGE_SCHEMA
)
async def receiver_mode_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    return var


@automation.register_action(
    "rf_bridge_cc1101.transmitter_mode", RFBridgeTransmitterModeAction, RF_BRIDGE_SCHEMA
)
async def transmitter_mode_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    return var


RF_BRIDGE_TRANSMIT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(RFBridgeComponent),
        cv.Required(CONF_CODE): cv.templatable(cv.string),
        cv.Optional(CONF_PROTOCOL, default=1): cv.templatable(cv.int_range(min=1, max=8)),
        cv.Optional(CONF_REPEAT, default={CONF_TIMES: 5}): cv.Schema(
            {
                cv.Required(CONF_TIMES): cv.templatable(cv.positive_int),
                cv.Optional(CONF_WAIT_TIME, default="0us"): cv.templatable(
                    cv.positive_time_period_microseconds
                ),
            }
        ),
    }
)


@automation.register_action(
    "rf_bridge_cc1101.transmit", RFBridgeTransmitAction, RF_BRIDGE_TRANSMIT_SCHEMA
)
async def transmit_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    cg.add(var.set_code((await cg.templatable(config[CONF_CODE], args, cg.std_string))))
    template_ = await cg.templatable(config[CONF_PROTOCOL], args, cg.uint8)
    cg.add(var.set_protocol(template_))
    template_ = await cg.templatable(config[CONF_REPEAT][CONF_TIMES], args, cg.uint32)
    cg.add(var.set_send_times(template_))
    template_ = await cg.templatable(config[CONF_REPEAT][CONF_WAIT_TIME], args, cg.uint32)
    cg.add(var.set_send_wait(template_))
    return var
