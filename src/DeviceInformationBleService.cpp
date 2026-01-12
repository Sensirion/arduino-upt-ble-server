#include "DeviceInformationBleService.h"
#include <cstring>

namespace sensirion::upt::ble_server {

bool DeviceInformationBleService::begin() {
  mBleLibrary.createService(DEVICE_INFORMATION_SERVICE_UUID);

  mBleLibrary.createCharacteristic(DEVICE_INFORMATION_SERVICE_UUID, MANUFACTURER_NAME_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(MANUFACTURER_NAME_UUID, reinterpret_cast<const uint8_t *>(DEFAULT_MANUFACTURER_NAME), strlen(DEFAULT_MANUFACTURER_NAME));

  mBleLibrary.createCharacteristic(DEVICE_INFORMATION_SERVICE_UUID, MODEL_NUMBER_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(MODEL_NUMBER_UUID, reinterpret_cast<const uint8_t *>(DEFAULT_MODEL_NUMBER), strlen(DEFAULT_MODEL_NUMBER));

  mBleLibrary.createCharacteristic(DEVICE_INFORMATION_SERVICE_UUID, FIRMWARE_REVISION_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(FIRMWARE_REVISION_UUID, reinterpret_cast<const uint8_t *>(DEFAULT_FIRMWARE_REVISION), strlen(DEFAULT_FIRMWARE_REVISION));

  mBleLibrary.startService(DEVICE_INFORMATION_SERVICE_UUID);
  return true;
}

void DeviceInformationBleService::setManufacturerName(std::string manufacturer) const {
  mBleLibrary.characteristicSetValue(MANUFACTURER_NAME_UUID, reinterpret_cast<const uint8_t *>(manufacturer.c_str()), manufacturer.length());
}

void DeviceInformationBleService::setModelNumber(std::string model) const {
  mBleLibrary.characteristicSetValue(MODEL_NUMBER_UUID, reinterpret_cast<const uint8_t *>(model.c_str()), model.length());
}

void DeviceInformationBleService::setFirmwareRevision(std::string firmwareRevision) const {
  mBleLibrary.characteristicSetValue(FIRMWARE_REVISION_UUID, reinterpret_cast<const uint8_t *>(firmwareRevision.c_str()), firmwareRevision.length());
}

} // namespace sensirion::upt::ble_server