#include "esphome.h"
using namespace esphome;

class SmokerController : public Component, public Climate
{

public:
    void setup() override
    {
        this->target_temperature = 95;
    }

    void loop() override
    {
        auto currentMode = id(active_mode).state;
        auto pidAction = ClimateAction::CLIMATE_ACTION_OFF;

        if (currentMode == "OFF")
        {
            pidAction = ClimateAction::CLIMATE_ACTION_OFF;

            if (id(smoker_pid).mode != ClimateMode::CLIMATE_MODE_OFF)
            {
                auto update_call = id(smoker_pid).make_call();
                update_call.set_mode(ClimateMode::CLIMATE_MODE_OFF);
                update_call.perform();
            }
        }
        else if (currentMode == "STARTUP")
        {
            pidAction = ClimateAction::CLIMATE_ACTION_HEATING;
        }
        else if (currentMode == "PID")
        {
            pidAction = id(smoker_pid).action;
        }
        else if (currentMode == "FLAMEOUT")
        {
            pidAction = ClimateAction::CLIMATE_ACTION_COOLING;
        }
        else if (currentMode == "SHUTDOWN")
        {
            pidAction = ClimateAction::CLIMATE_ACTION_COOLING;
        }
        else if (currentMode == "FEED")
        {
            pidAction = ClimateAction::CLIMATE_ACTION_OFF;
        }

        if (this->action != pidAction)
        {
            this->action = pidAction;
            this->publish_state();
        }

        if (this->current_temperature != id(smoker_temperature).state)
        {
            this->current_temperature = id(smoker_temperature).state;
            this->publish_state();
        }
    }

    void control(const ClimateCall &call) override
    {
        if (call.get_mode().has_value() && this->mode != *call.get_mode())
        {
            this->mode = *call.get_mode();

            if (this->mode == ClimateMode::CLIMATE_MODE_OFF)
            {
                onClimateModeUpdated("OFF");
            }
            else if (this->mode == ClimateMode::CLIMATE_MODE_HEAT)
            {
                onClimateModeUpdated("HEAT");
            }
        }

        if (call.get_target_temperature().has_value())
        {
            this->target_temperature = *call.get_target_temperature();
            auto update_call = id(smoker_pid).make_call();
            update_call.set_target_temperature(this->target_temperature);
            update_call.perform();
        }

        this->publish_state();
    }

    climate::ClimateTraits traits()
    {
        auto traits = climate::ClimateTraits();
        traits.set_supports_current_temperature(true);
        traits.set_supports_heat_mode(true);
        traits.set_visual_min_temperature(70);
        traits.set_visual_min_temperature(260);
        traits.set_visual_temperature_step(5);
        traits.set_supports_action(true);

        // Disabled Traits
        traits.set_supports_two_point_target_temperature(false);
        traits.set_supports_away(false);
        traits.set_supports_auto_mode(false);
        traits.set_supports_cool_mode(false);
        return traits;
    }
};

class PidModeFloatOutput : public Component, public FloatOutput
{
public:
    void setup() override
    {
    }

    void write_state(float state) override
    {
        if (id(active_mode).state == "PID") {
            id(auger_pwm).set_level(state);
            float adjusted_blower_value = (state * (1 - 0.3)) + 0.3;
            ESP_LOGI("smoker", "Adjusted Blower Value: %.5f", adjusted_blower_value);
            id(blower_pwm).set_level(adjusted_blower_value);
        }
    }
};






class PidCoolModeFloatOutput : public Component, public FloatOutput
{
public:
    void setup() override
    {
    }

    void write_state(float state) override
    {
        if (id(active_mode).state == "PID" && state > 0.3) {        
            id(auger_pwm).set_level(0.4 * state);
        }
    }
};


// #ifdef SMOKER_DEBUG
// class DebuggingFloatOutput : public Component, public FloatOutput
// {
// public:
//     void setup() override
//     {
//     }

//     void write_state(float state) override
//     {
//         id(debug_auger_sensor).publish_state(state);
//     }
// };
// #endif
