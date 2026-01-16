#ifndef ARDUINO_UPT_BLE_SERVER_DEVICE_INFORMATION_BLE_SERVICE_H
#define ARDUINO_UPT_BLE_SERVER_DEVICE_INFORMATION_BLE_SERVICE_H
#include "IBleServiceProvider.h"

namespace sensirion::upt::ble_server {

constexpr auto DEVICE_INFORMATION_SERVICE_UUID = "0000180a-0000-1000-8000-00805f9b34fb";
constexpr auto MANUFACTURER_NAME_UUID = "00002a29-0000-1000-8000-00805f9b34fb";
constexpr auto MODEL_NUMBER_UUID = "00002a24-0000-1000-8000-00805f9b34fb";
constexpr auto FIRMWARE_REVISION_UUID = "00002a26-0000-1000-8000-00805f9b34fb";

constexpr auto DEFAULT_MANUFACTURER_NAME = "SensirionDiY";
constexpr auto DEFAULT_MODEL_NUMBER = "DiY BLE Gadget";
constexpr auto DEFAULT_FIRMWARE_REVISION = "0.1.0";


class DeviceInformationBleService final : public IBleServiceProvider {
public:
  explicit DeviceInformationBleService(IBleServiceLibrary &bleLibrary)
      : IBleServiceProvider(bleLibrary) {}

  bool begin() override;

  void setManufacturerName(std::string manufacturer);
  void setModelNumber(std::string modelNumber);
  void setFirmwareRevision(std::string firmwareRevision);

private:
  std::string mManufacturerName = DEFAULT_MANUFACTURER_NAME;
  std::string mModelNumber = DEFAULT_MODEL_NUMBER;
  std::string mFirmwareRevision = DEFAULT_FIRMWARE_REVISION;
};

} // namespace sensirion::upt::ble_server

#endif // ARDUINO_UPT_BLE_SERVER_DEVICE_INFORMATION_BLE_SERVICE_H
