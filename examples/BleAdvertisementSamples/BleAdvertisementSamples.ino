#include "SensirionUptBleServer.h"

NimBLELibraryWrapper lib;
DataProvider provider(lib, DataType::T_RH_CO2_ALT);

uint16_t t = 0;
uint16_t rh = 0;
uint16_t co2 = 0;

static int64_t lastMeasurementTimeMs = 0;
static int measurementIntervalMs = 1000;

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for Serial monitor to start

  // Initialize the GadgetBle Library
  provider.begin();

  Serial.print("Sensirion GadgetBle Lib initialized with deviceId = ");
  Serial.println(provider.getDeviceIdString());
}

void loop() {
  if (millis() - lastMeasurementTimeMs >= measurementIntervalMs) {
    Serial.println("Measurement");
    provider.writeValueToCurrentSample(++t % 50,
                                       SignalType::TEMPERATURE_DEGREES_CELSIUS);
    provider.writeValueToCurrentSample(
        ++rh % 100, SignalType::RELATIVE_HUMIDITY_PERCENTAGE);
    provider.writeValueToCurrentSample(++co2 % 1000,
                                       SignalType::CO2_PARTS_PER_MILLION);
    provider.commitSample();
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

  delay(3);
}
