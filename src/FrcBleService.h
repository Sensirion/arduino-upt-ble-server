#ifndef ARDUINO_UPT_BLE_SERVER_FRC_BLE_SERVICE_H
#define ARDUINO_UPT_BLE_SERVER_FRC_BLE_SERVICE_H

#include "IBleServiceProvider.h"

namespace sensirion::upt::ble_server {

constexpr auto SCD_SERVICE_UUID = "00007000-b38d-4985-720e-0f993a68ee41";
constexpr auto SCD_FRC_REQUEST_UUID = "00007004-b38d-4985-720e-0f993a68ee41";

using frc_request_callback_t = std::function<void(uint16_t)>;

class FrcBleService final : public IBleServiceProvider {
public:
  explicit FrcBleService(IBleServiceLibrary &bleLibrary)
      : IBleServiceProvider(bleLibrary) {}

  bool begin() override;

  void registerFrcRequestCallback(frc_request_callback_t callback) const;
};

} // namespace sensirion::upt::ble_server

#endif // ARDUINO_UPT_BLE_SERVER_FRC_BLE_SERVICE_H
