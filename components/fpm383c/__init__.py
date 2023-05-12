import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome import automation
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
    CONF_ON_FINGER_SCAN_MATCHED,
    CONF_ON_FINGER_SCAN_UNMATCHED
)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["light"]

CONF_ON_TOUCH = "on_touch"
CONF_ON_RELEASE = "on_release"
CONF_ON_FINGER_REGISTER_PROGRESS="on_finger_register_progress"
CONF_ON_FINGER_REGISTER_SUCESSED="on_finger_register_sucessed"
CONF_ON_FINGER_REGISTER_FAILED="on_finger_register_failed"

fpm383c_ns = cg.esphome_ns.namespace("fpm383c")
FPM383cComponent = fpm383c_ns.class_(
    "FPM383cComponent", cg.PollingComponent, uart.UARTDevice
)
TouchTrigger = fpm383c_ns.class_(
    "TouchTrigger", automation.Trigger.template()
)
ReleaseTrigger = fpm383c_ns.class_(
    "ReleaseTrigger", automation.Trigger.template()
)
RegisterProgress = fpm383c_ns.struct("RegisterProgress")
RegisterProgressTrigger = fpm383c_ns.class_(
    "RegisterProgressTrigger", automation.Trigger.template(RegisterProgress)
)
MatchResult = fpm383c_ns.struct("MatchResult")
MatchSucessedTrigger = fpm383c_ns.class_(
    "MatchSucessedTrigger", automation.Trigger.template(MatchResult)
)
MatchFailedTrigger = fpm383c_ns.class_(
    "MatchFailedTrigger", automation.Trigger.template()
)

RegisterFingerprintAction = fpm383c_ns.class_(
    "RegisterFingerprintAction", automation.Action
)
ClearFingerprintAction = fpm383c_ns.class_(
    "ClearFingerprintAction", automation.Action
)
CancelAction = fpm383c_ns.class_(
    "CancelAction", automation.Action
)
ResetAction = fpm383c_ns.class_(
    "ResetAction", automation.Action
)



CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(FPM383cComponent),
            cv.Optional(
                CONF_ON_TOUCH
            ): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        TouchTrigger
                    ),
                }
            ),
            cv.Optional(
                CONF_ON_RELEASE
            ): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        ReleaseTrigger
                    ),
                }
            ),
            cv.Optional(
                CONF_ON_FINGER_SCAN_MATCHED
            ): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        MatchSucessedTrigger
                    ),
                }
            ),
            cv.Optional(
                CONF_ON_FINGER_SCAN_UNMATCHED
            ): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        MatchFailedTrigger
                    ),
                }
            ),
            cv.Optional(
                CONF_ON_FINGER_REGISTER_PROGRESS
            ): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        RegisterProgressTrigger
                    ),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("200ms"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    for conf in config.get(CONF_ON_TOUCH, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_touch_listener(trigger))
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_RELEASE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_touch_listener(trigger))
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_FINGER_SCAN_MATCHED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_fingerprint_match_listener(trigger))
        await automation.build_automation(trigger, [(MatchResult, "x")], conf)
    for conf in config.get(CONF_ON_FINGER_SCAN_UNMATCHED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_fingerprint_match_listener(trigger))
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_FINGER_REGISTER_PROGRESS, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_fingerprint_register_listener(trigger))
        await automation.build_automation(trigger, [(RegisterProgress, "x")], conf)

ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(FPM383cComponent),
    }
)

@automation.register_action(
    "fpm383c.register", RegisterFingerprintAction, ACTION_SCHEMA
)
async def register_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    return var


@automation.register_action(
    "fpm383c.clear", ClearFingerprintAction, ACTION_SCHEMA
)
async def clear_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    return var

@automation.register_action(
    "fpm383c.cancel", CancelAction, ACTION_SCHEMA
)
async def cancel_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    return var

@automation.register_action(
    "fpm383c.reset", ResetAction, ACTION_SCHEMA
)
async def reset_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    return var
