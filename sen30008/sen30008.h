#include "esphome.h"
#include "PlayingWithFusion_MAX31856.h"
#include "PlayingWithFusion_MAX31856_STRUCT.h"

static const char *TAG = "sen30008";

class SEN30008Device : public PollingComponent
{
public:
    Sensor *internal_temp = new Sensor();
    Sensor *tc_0 = new Sensor();
    Sensor *tc_1 = new Sensor();
    Sensor *tc_2 = new Sensor();
    Sensor *tc_3 = new Sensor();

    SEN30008Device(
        uint8_t sck,
        uint8_t miso,
        uint8_t mosi,
        uint8_t tc_0,
        uint8_t tc_1,
        uint8_t tc_2,
        uint8_t tc_3) : PollingComponent(1000)
    {
        sck_pin = sck;
        miso_pin = miso;
        mosi_pin = mosi;
        tc_0_pin = tc_0;
        tc_1_pin = tc_1;
        tc_2_pin = tc_2;
        tc_3_pin = tc_3;
    }

    void setup() override
    {
        thermocouple0 = new PWF_MAX31856(tc_0_pin);
        thermocouple1 = new PWF_MAX31856(tc_1_pin);
        thermocouple2 = new PWF_MAX31856(tc_2_pin);
        thermocouple3 = new PWF_MAX31856(tc_3_pin);

        SPI.begin(sck_pin, miso_pin, mosi_pin);
        SPI.setClockDivider(SPI_CLOCK_DIV16);
        SPI.setDataMode(SPI_MODE3);

        // call config command... options can be seen in the PlayingWithFusion_MAX31856.h file
        thermocouple0->MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_4SAMP, CMODE_OFF);
        thermocouple1->MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_4SAMP, CMODE_OFF);
        thermocouple2->MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_4SAMP, CMODE_OFF);
        thermocouple3->MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_4SAMP, CMODE_OFF);
    }

    void update() override
    {
        readSingleThermocouple(thermocouple0, tc_0);
        readSingleThermocouple(thermocouple1, tc_1);
        readSingleThermocouple(thermocouple2, tc_2);
        readSingleThermocouple(thermocouple3, tc_3);
    }

protected:
    uint8_t sck_pin;
    uint8_t miso_pin;
    uint8_t mosi_pin;
    uint8_t tc_0_pin;
    uint8_t tc_1_pin;
    uint8_t tc_2_pin;
    uint8_t tc_3_pin;

    PWF_MAX31856 *thermocouple0;
    PWF_MAX31856 *thermocouple1;
    PWF_MAX31856 *thermocouple2;
    PWF_MAX31856 *thermocouple3;

    void readSingleThermocouple(PWF_MAX31856 *thermocouple, Sensor *tc_sensor)
    {
        struct var_max31856 tc_ptr;
        thermocouple->MAX31856_1shot_start();
        delay(180);
        thermocouple->MAX31856_update(&tc_ptr);
        auto hasFault = updateSensor(&tc_ptr, tc_sensor);
        if (hasFault == true)
        {
            thermocouple->clear_fault();
        }
    }

    bool updateSensor(struct var_max31856 *tc_ptr, Sensor *tc_sensor)
    {
        if (tc_0 == tc_sensor)
        {
            double itemp = (double)tc_ptr->ref_jcn_temp * 0.015625;
            internal_temp->publish_state((float)itemp);
        }

        if (tc_ptr->status)
        {
            logFaults(tc_ptr->status, tc_sensor);
            tc_sensor->publish_state(NAN);
            return true;
        }

        ESP_LOGV(TAG, "Read Temperature Successfully: '%s'...", tc_sensor->get_name().c_str());
        double temp = (double)tc_ptr->lin_tc_temp * 0.0078125;
        tc_sensor->publish_state((float)temp);
        return false;
    }

    void logFaults(uint8_t status, Sensor *tc_sensor)
    {
        if (0x01 & status)
        {
            ESP_LOGW(TAG, "Thermocouple Open-Circuit Fault (possibly not connected): '%s'...", tc_sensor->get_name().c_str());
            return;
        }
        if (0x02 & status)
        {
            ESP_LOGW(TAG, "Overvoltage or Undervoltage Input Fault: '%s'...", tc_sensor->get_name().c_str());
        }
        if (0x04 & status)
        {
            ESP_LOGW(TAG, "Thermocouple Temperature Low Fault: '%s'...", tc_sensor->get_name().c_str());
        }
        if (0x08 & status)
        {
            ESP_LOGW(TAG, "Thermocouple Temperature High Fault: '%s'...", tc_sensor->get_name().c_str());
        }
        if (0x10 & status)
        {
            ESP_LOGW(TAG, "Cold-Junction Low Fault: '%s'...", tc_sensor->get_name().c_str());
        }
        if (0x20 & status)
        {
            ESP_LOGW(TAG, "Cold-Junction High Fault: '%s'...", tc_sensor->get_name().c_str());
        }
        if (0x40 & status)
        {
            ESP_LOGW(TAG, "Thermocouple Out-of-Range: '%s'...", tc_sensor->get_name().c_str());
        }
        if (0x80 & status)
        {
            ESP_LOGW(TAG, "Cold Junction Out-of-Range: '%s'...", tc_sensor->get_name().c_str());
        }
    }
};