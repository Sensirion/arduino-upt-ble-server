#include "BatteryBleService.h"

bool BatteryBleService::begin() {
  mBleLibrary.createService(BATTERY_SERVICE_UUID);
  mBleLibrary.createCharacteristic(BATTERY_SERVICE_UUID, BATTERY_LEVEL_UUID,
                                   READ_PERMISSION);
  mBleLibrary.characteristicSetValue(BATTERY_LEVEL_UUID, 0);
  mBleLibrary.startService(BATTERY_SERVICE_UUID);

  return true;
}

void BatteryBleService::setBatteryLevel(const int value) const {
  mBleLibrary.characteristicSetValue(BATTERY_LEVEL_UUID, value);
}
