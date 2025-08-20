#include "FrcBleService.h"

namespace sensirion::upt::ble_server {

bool FrcBleService::begin() {
  mBleLibrary.createService(SCD_SERVICE_UUID);
  mBleLibrary.createCharacteristic(SCD_SERVICE_UUID, SCD_FRC_REQUEST_UUID,
                                   WRITE_PERMISSION);
  mBleLibrary.characteristicSetValue(SCD_FRC_REQUEST_UUID, 0);
  mBleLibrary.startService(SCD_SERVICE_UUID);

  return true;
}

void FrcBleService::registerFrcRequestCallback(
    // ReSharper disable once CppPassValueParameterByConstReference
    const frc_request_callback_t callback) const {
  auto onFRCRequest = [=](const std::string &value) {
    // co2 level is encoded in lower two bytes, little endian
    // the first two bytes are obfuscation and can be ignored
    const uint16_t referenceCO2Level = value[2] | (value[3] << 8);
    callback(referenceCO2Level);
  };

  mBleLibrary.registerCharacteristicCallback(SCD_FRC_REQUEST_UUID,
                                             onFRCRequest);
}

} // namespace sensirion::upt::ble_server