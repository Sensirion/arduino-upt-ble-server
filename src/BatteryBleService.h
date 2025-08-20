#ifndef ARDUINO_UPT_BLE_SERVER_BATTERY_BLE_SERVICE_H
#define ARDUINO_UPT_BLE_SERVER_BATTERY_BLE_SERVICE_H
#include "IBleServiceProvider.h"

constexpr auto BATTERY_SERVICE_UUID = "0000180f-0000-1000-8000-00805f9b34fb";
constexpr auto BATTERY_LEVEL_UUID = "00002a19-0000-1000-8000-00805f9b34fb";

class BatteryBleService final : public IBleServiceProvider {
public:
  explicit BatteryBleService(IBleServiceLibrary &bleLibrary)
      : IBleServiceProvider(bleLibrary) {}
  bool begin() override;

  void setBatteryLevel(uint8_t value) const;
};

#endif // ARDUINO_UPT_BLE_SERVER_BATTERY_BLE_SERVICE_H
