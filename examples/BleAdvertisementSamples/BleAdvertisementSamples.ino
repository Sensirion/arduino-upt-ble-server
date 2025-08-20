#include "SensirionUptBleServer.h"

using namespace sensirion::upt;

ble_server::NimBLELibraryWrapper lib;
ble_server::UptBleServer uptBleServer(lib, DataType::T_RH_CO2_ALT);

uint16_t t = 0;
uint16_t rh = 0;
uint16_t co2 = 0;

static int64_t lastMeasurementTimeMs = 0;
static int measurementIntervalMs = 1000;

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for Serial monitor to start

  // Initialize the GadgetBle Library
  uptBleServer.begin();

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
    Serial.println(rh);
  }

  // handle download requests
  uptBleServer.handleDownload();

  delay(20);
}
