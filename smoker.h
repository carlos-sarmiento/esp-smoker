using namespace esphome;

// #define SMOKER_DEBUG 1

#ifdef SMOKER_DEBUG

#define FLAMEOUT_TIMEOUT_IN_MINUTES 1
#define STARTUP_FEED_TIMER_IN_MINUTES 1
#define FEED_TIMEOUT_IN_MINUTES 1
#define SHUTDOWN_TIMEOUT_IN_MINUTES 1
#define FLAMEOUT_THRESHOLD_TEMPERATURE 60
#define SECONDS_IN_MINUTE 10

#else

#define FLAMEOUT_TIMEOUT_IN_MINUTES 10    // 10 minutes
#define STARTUP_FEED_TIMER_IN_MINUTES 3      // 8 minutes
#define FEED_TIMEOUT_IN_MINUTES 7         // 7 minutes
#define SHUTDOWN_TIMEOUT_IN_MINUTES 60    // 60 minutes
#define FLAMEOUT_THRESHOLD_TEMPERATURE 60 // 60 degrees celsius
#define SMOKER_THERMOCOUPLE_FAILURE_WAIT_TIME  30 // 30 seconds
#define SECONDS_IN_MINUTE 60

#endif

#define SHUTDOWN_THRESHOLD_TEMPERATURE 30

#define MODE_SHUTDOWN "SHUTDOWN"
#define MODE_PID "PID"
#define MODE_OFF "OFF"
#define MODE_STARTUP "STARTUP"
#define MODE_FLAMEOUT "FLAMEOUT"
#define MODE_FEED "FEED"

void turnOffGeneralTimer()
{
    ESP_LOGI("smoker", "turnOffGeneralTimer");
    id(general_timer_enabled).publish_state(false);
    id(general_timer).publish_state(-1);
}

void turnOnGeneralTimer(int minutes)
{
    ESP_LOGI("smoker", "turnOnGeneralTimer");
    id(general_timer_enabled).publish_state(true);
    id(general_timer).publish_state(SECONDS_IN_MINUTE * minutes);
}

void turnOffFlameoutTimer()
{
    ESP_LOGI("smoker", "turnOffFlameoutTimer");
    id(flameout_watch).publish_state(false);
    id(flameout_timer).publish_state(-1);
}

void turnOnFlameoutTimer(int minutes)
{
    ESP_LOGI("smoker", "turnOnFlameoutTimer");
    id(flameout_watch).publish_state(true);
    id(flameout_timer).publish_state(SECONDS_IN_MINUTE * minutes);
}

void onClockTick()
{
    bool is_flameout_watch_enabled = id(flameout_watch).state;
    if (is_flameout_watch_enabled)
    {
        id(flameout_timer).publish_state(id(flameout_timer).state - 1);
    }

    bool is_general_timer_enabled = id(general_timer_enabled).state;
    if (is_general_timer_enabled)
    {
        id(general_timer).publish_state(id(general_timer).state - 1);
    }
}

void onFlameoutWatchTimerUpdated()
{
    // ESP_LOGI("smoker", "onFlameoutWatchTimerUpdated");
    bool is_flameout_watch_enabled = id(flameout_watch).state;
    auto timer = id(flameout_timer).state;
    if (is_flameout_watch_enabled && timer <= 0)
    {
        id(active_mode).publish_state(MODE_FLAMEOUT);
        turnOffFlameoutTimer();
    }
}


//////////////////////////////////////////////////////////////////////
// SERVICES
//////////////////////////////////////////////////////////////////////

void onForceOff()
{
    ESP_LOGI("smoker", "onForceOff");
    id(active_mode).publish_state(MODE_OFF);
}

void onBypass()
{
    ESP_LOGI("smoker", "onBypass");
    auto currentMode = id(active_mode).state;
    if (currentMode == MODE_STARTUP)
    {
        id(active_mode).publish_state(MODE_PID);
    }
}

void onActivateFeed()
{
    ESP_LOGI("smoker", "onActivateFeed");
    auto currentMode = id(active_mode).state;
    if (currentMode == MODE_OFF)
    {
        id(active_mode).publish_state(MODE_FEED);
    }
}

//////////////////////////////////////////////////////////////////////
// SWITCHES
//////////////////////////////////////////////////////////////////////

void turnOnPID()
{
    ESP_LOGI("smoker", "turnOnPID");
    id(smoker_pid).reset_integral_term();
    
    auto targetTemp = id(smoker_control).target_temperature;
    auto update_call = id(smoker_pid).make_call();
    update_call.set_mode(ClimateMode::CLIMATE_MODE_AUTO);
    update_call.set_target_temperature(targetTemp);
    update_call.perform();
}

void turnOffPID()
{
    ESP_LOGI("smoker", "turnOffPID");
    auto update_call = id(smoker_pid).make_call();
    update_call.set_mode(ClimateMode::CLIMATE_MODE_OFF);
    update_call.perform();
}

void turnOffSmokerClimate()
{
    ESP_LOGI("smoker", "turnOffSmokerClimate");
    auto update_call = id(smoker_control).make_call();
    update_call.set_mode(ClimateMode::CLIMATE_MODE_OFF);
    update_call.perform();
}

void turnOnAuger()
{
    ESP_LOGI("smoker", "turnOnAuger");
    id(auger_pwm).set_level(1.0f);
}

void turnOffAuger()
{
    ESP_LOGI("smoker", "turnOffAuger");
    id(auger_pwm).set_level(0.0f);
}

void turnOnBlower()
{
    ESP_LOGI("smoker", "turnOnBlower");
    id(blower_pwm).set_level(1.0f);
}

void turnOffBlower()
{
    ESP_LOGI("smoker", "turnOffBlower");
    id(blower_pwm).set_level(0.0f);
}

void turnOnIgniter()
{
    ESP_LOGI("smoker", "turnOnIgniter");
    id(igniter).turn_on();
}

void turnOffIgniter()
{
    ESP_LOGI("smoker", "turnOffIgniter");
    id(igniter).turn_off();
}

void onSmokerTemperatureUpdate(int temperature)
{
    // ESP_LOGI("smoker", "onSmokerTemperatureUpdate");
    bool is_flameout_watch_enabled = id(flameout_watch).state;
    auto activeMode = id(active_mode).state;
    auto coldJunctionTemp = id(smoker_cold_junction).state;

    if (isnan(temperature) &&
       (activeMode == MODE_PID ||
        activeMode == MODE_STARTUP )) {
        auto remainingTime = id(flameout_timer).state;

        if (!is_flameout_watch_enabled || remainingTime > SMOKER_THERMOCOUPLE_FAILURE_WAIT_TIME)
        {
            turnOnFlameoutTimer(SMOKER_THERMOCOUPLE_FAILURE_WAIT_TIME);
        }
    }

    if (temperature <= FLAMEOUT_THRESHOLD_TEMPERATURE &&
        (activeMode == MODE_PID ||
        (activeMode == MODE_STARTUP && id(general_timer_enabled).state == false ))
    ) {
        if (!is_flameout_watch_enabled)
        {
            turnOnFlameoutTimer(FLAMEOUT_TIMEOUT_IN_MINUTES);
        }
    }
    else if (is_flameout_watch_enabled)
    {
        turnOffFlameoutTimer();
    }
    else if (temperature > FLAMEOUT_THRESHOLD_TEMPERATURE && (activeMode == MODE_STARTUP))
    {
        id(active_mode).publish_state(MODE_PID);
    }
    else if (temperature <= coldJunctionTemp + 2 && (activeMode == MODE_SHUTDOWN))
    {
        id(active_mode).publish_state(MODE_OFF);
    }
}

void onGeneralTimerUpdated()
{
    bool is_general_timer_enabled = id(general_timer_enabled).state;
    auto timer = id(general_timer).state;
    if (is_general_timer_enabled && timer <= 0)
    {
        auto currentMode = id(active_mode).state;
        if (currentMode == MODE_STARTUP)
        {
            turnOffAuger();
            turnOffGeneralTimer();
        }
        if (currentMode == MODE_SHUTDOWN)
        {
            id(active_mode).publish_state(MODE_OFF);
        }
        if (currentMode == MODE_FEED)
        {
            id(active_mode).publish_state(MODE_OFF);
        }
    }
}

void onActiveModeUpdate(std::string newMode)
{
    ESP_LOGI("smoker", "onActiveModeUpdate");
    if (newMode == MODE_OFF)
    {
        turnOffPID();
        turnOffBlower();
        turnOffIgniter();
        turnOffAuger();
        turnOffSmokerClimate();

        turnOffGeneralTimer();
        turnOffFlameoutTimer();
    }
    else if (newMode == MODE_STARTUP)
    {
        turnOnBlower();
        turnOnIgniter();
        turnOnAuger();

        turnOffPID();

        turnOnGeneralTimer(STARTUP_FEED_TIMER_IN_MINUTES);
        turnOffFlameoutTimer();
    }
    else if (newMode == MODE_PID)
    {
        turnOnPID();
        turnOnBlower();

        turnOffIgniter();
        turnOffAuger();

        turnOffGeneralTimer();
    }
    else if (newMode == MODE_FLAMEOUT)
    {
        turnOnBlower();

        turnOffPID();
        turnOffIgniter();
        turnOffAuger();

        turnOffGeneralTimer();
        turnOffFlameoutTimer();
    }
    else if (newMode == MODE_SHUTDOWN)
    {
        turnOnBlower();

        turnOffPID();
        turnOffIgniter();
        turnOffAuger();

        turnOnGeneralTimer(SHUTDOWN_TIMEOUT_IN_MINUTES);
        turnOffFlameoutTimer();
    }
    else if (newMode == MODE_FEED)
    {
        turnOnAuger();

        turnOffPID();
        turnOffBlower();
        turnOffIgniter();

        turnOnGeneralTimer(FEED_TIMEOUT_IN_MINUTES);
        turnOffFlameoutTimer();
    }
}

void onClimateModeUpdated(std::string climateMode)
{
    ESP_LOGI("smoker", "onClimateModeUpdated");
    auto currentMode = id(active_mode).state;

    if (climateMode == "HEAT" && (currentMode == MODE_OFF || currentMode == MODE_FEED))
    {
        id(active_mode).publish_state(MODE_STARTUP);
    }
    else if (climateMode == MODE_OFF && currentMode != MODE_OFF && currentMode != MODE_SHUTDOWN)
    {
        id(active_mode).publish_state(MODE_SHUTDOWN);
    }
}
