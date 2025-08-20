#include "BatteryBleService.h"
#include "SensirionUptBleServer.h"

NimBLELibraryWrapper lib;
BatteryBleService batteryBleService(lib);
UptBleServer uptBleServer(lib, T_RH_CO2_ALT);

uint16_t t = 0;
uint16_t rh = 0;
uint16_t co2 = 0;
uint8_t batteryLevel = 100;

static int64_t lastMeasurementTimeMs = 0;
static int measurementIntervalMs = 1000;

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for Serial monitor to start

  // Register battery ble services
  uptBleServer.registerBleServiceProvider(batteryBleService);

  // Initialize the GadgetBle Library
  uptBleServer.begin();

  // Set the initial battery level after the service is initialized
  batteryBleService.setBatteryLevel(batteryLevel);

  Serial.print("Sensirion GadgetBle Lib initialized with deviceId = ");
  Serial.println(uptBleServer.getDeviceIdString());
}

void loop() {
  if (millis() - lastMeasurementTimeMs >= measurementIntervalMs) {
    uptBleServer.writeValueToCurrentSample(
        ++t % 50, SignalType::TEMPERATURE_DEGREES_CELSIUS);
    uptBleServer.writeValueToCurrentSample(
        ++rh % 100, SignalType::RELATIVE_HUMIDITY_PERCENTAGE);
    uptBleServer.writeValueToCurrentSample(++co2 % 1000,
                                           SignalType::CO2_PARTS_PER_MILLION);
    // Update battery
    batteryLevel--;
    if (batteryLevel == 0 || batteryLevel > 100) {
      batteryLevel = 100;
    }
    batteryBleService.setBatteryLevel(batteryLevel);

    uptBleServer.commitSample();
    lastMeasurementTimeMs = millis();
    // Provide the sensor values for Tools -> Serial Monitor or Serial Plotter
    Serial.print("mockCO2[ppm]:");
    Serial.print(co2);
    Serial.print("\t");
    Serial.print("mockTemperature[℃]:");
    Serial.print(t);
    Serial.print("\t");
    Serial.print("mockHumidity[%]:");
    Serial.print(rh);
    Serial.print("\t");
    Serial.print("BatteryLevel[%]:");
    Serial.println(batteryLevel);
  }

  // handle download requests
  uptBleServer.handleDownload();

  delay(20);
}
