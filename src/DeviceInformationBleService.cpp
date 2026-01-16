#include "DeviceInformationBleService.h"
#include <cstring>

namespace sensirion::upt::ble_server {

bool DeviceInformationBleService::begin() {
  mBleLibrary.createService(DEVICE_INFORMATION_SERVICE_UUID);

  mBleLibrary.createCharacteristic(DEVICE_INFORMATION_SERVICE_UUID, MANUFACTURER_NAME_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(MANUFACTURER_NAME_UUID, reinterpret_cast<const uint8_t *>(mManufacturerName.c_str()), mManufacturerName.length());

  mBleLibrary.createCharacteristic(DEVICE_INFORMATION_SERVICE_UUID, MODEL_NUMBER_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(MODEL_NUMBER_UUID, reinterpret_cast<const uint8_t *>(mModelNumber.c_str()), mModelNumber.length());
  mBleLibrary.createCharacteristic(DEVICE_INFORMATION_SERVICE_UUID, FIRMWARE_REVISION_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(FIRMWARE_REVISION_UUID, reinterpret_cast<const uint8_t *>(mFirmwareRevision.c_str()), mFirmwareRevision.length());

  mBleLibrary.startService(DEVICE_INFORMATION_SERVICE_UUID);
  return true;
}

void DeviceInformationBleService::setManufacturerName(std::string manufacturer) {
  mManufacturerName = std::move(manufacturer);
  mBleLibrary.characteristicSetValue(MANUFACTURER_NAME_UUID, reinterpret_cast<const uint8_t *>(mManufacturerName.c_str()), mManufacturerName.length());
}

void DeviceInformationBleService::setModelNumber(std::string model) {
  mModelNumber = std::move(model);
  mBleLibrary.characteristicSetValue(MODEL_NUMBER_UUID, reinterpret_cast<const uint8_t *>(mModelNumber.c_str()), mModelNumber.length());
}

void DeviceInformationBleService::setFirmwareRevision(std::string firmwareRevision) {
  mFirmwareRevision = std::move(firmwareRevision);
  mBleLibrary.characteristicSetValue(FIRMWARE_REVISION_UUID, reinterpret_cast<const uint8_t *>(mFirmwareRevision.c_str()), mFirmwareRevision.length());
}

} // namespace sensirion::upt::ble_server