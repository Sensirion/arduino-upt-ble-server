#include <Arduino.h>
#include <SensirionI2cSen66.h>

#include "SensirionUptBleServer.h"
#include "SettingsBleService.h"

using namespace sensirion::upt;

ble_server::NimBLELibraryWrapper lib;
ble_server::SettingsBleService settingsBleService(lib);
ble_server::UptBleServer uptBleServer(lib, core::T_RH_CO2_VOC_NOX_PM25);

float massConcentrationPm1p0 = 0.0;
float massConcentrationPm2p5 = 0.0;
float massConcentrationPm4p0 = 0.0;
float massConcentrationPm10p0 = 0.0;
float humidity = 0.0;
float temperature = 0.0;
float vocIndex = 0.0;
float noxIndex = 0.0;
uint16_t co2 = 0;

static int64_t lastMeasurementTimeMs = 0;
static const int measurementIntervalMs = 1000;

SensirionI2cSen66 sen66;

static char errorMessage[32];
static int16_t error;

void setup()
{
    Serial.begin(115200);
    delay(1000); // Wait for Serial monitor to start

    Wire.begin();
    sen66.begin(Wire, SEN66_I2C_ADDR_6B);

    error = sen66.deviceReset();
    if (error != HighLevelError::NoError)
    {
        Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }

    delay(1200);

    error = sen66.startContinuousMeasurement();
    if (error != HighLevelError::NoError)
    {
        Serial.print("Error trying to execute startContinuousMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }

    // setup settings BLE services
    settingsBleService.setAltDeviceName("My SEN66 Gadget");
    settingsBleService.setEnableWifiSettings(false);
    uptBleServer.registerBleServiceProvider(settingsBleService);

    // Initialize the GadgetBle Library
    uptBleServer.begin();

    Serial.print("Sensirion GadgetBle Lib initialized with deviceId = ");
    Serial.println(uptBleServer.getDeviceIdString());
}

void loop()
{
    if (millis() - lastMeasurementTimeMs >= measurementIntervalMs)
    {
        error = sen66.readMeasuredValues(
            massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
            massConcentrationPm10p0, humidity, temperature, vocIndex, noxIndex,
            co2);
        if (error != HighLevelError::NoError)
        {
            Serial.print("Error trying to execute readMeasuredValues(): ");
            errorToString(error, errorMessage, sizeof errorMessage);
            Serial.println(errorMessage);
            return;
        }
        lastMeasurementTimeMs = millis();

        uptBleServer.writeValueToCurrentSample(
            temperature, core::SignalType::TEMPERATURE_DEGREES_CELSIUS);
        uptBleServer.writeValueToCurrentSample(
            humidity, core::SignalType::RELATIVE_HUMIDITY_PERCENTAGE);
        uptBleServer.writeValueToCurrentSample(
            co2, core::SignalType::CO2_PARTS_PER_MILLION);
        uptBleServer.writeValueToCurrentSample(
            massConcentrationPm2p5, core::SignalType::PM2P5_MICRO_GRAMM_PER_CUBIC_METER);
        uptBleServer.writeValueToCurrentSample(
            vocIndex, core::SignalType::VOC_INDEX);
        uptBleServer.writeValueToCurrentSample(
            noxIndex, core::SignalType::NOX_INDEX);

        uptBleServer.commitSample();
    }
    
    // handle download requests
    uptBleServer.handleDownload();

    delay(200);
}
