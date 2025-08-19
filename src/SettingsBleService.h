#ifndef ARDUINO_UPT_BLE_SERVER_SETTINGS_BLE_SERVICE_H
#define ARDUINO_UPT_BLE_SERVER_SETTINGS_BLE_SERVICE_H
#include "IBleServiceProvider.h"

constexpr auto SETTINGS_SERVICE_UUID = "00008100-b38d-4985-720e-0f993a68ee41";
constexpr auto WIFI_SSID_UUID = "00008171-b38d-4985-720e-0f993a68ee41";
constexpr auto WIFI_PWD_UUID = "00008172-b38d-4985-720e-0f993a68ee41";
constexpr auto ALT_DEVICE_NAME_UUID = "00008120-b38d-4985-720e-0f993a68ee41";

using wifi_changed_callback_t = std::function<void(std::string, std::string)>;

class SettingsBleService final : public IBleServiceProvider {
public:
  explicit SettingsBleService(IBleServiceLibrary &bleLibrary)
      : IBleServiceProvider(bleLibrary) {}
  bool begin() override;

  /**
   * Enable or disable the Wi-Fi settings. Default: true
   * Wi-Fi settings need to be configured before calling init.
   *
   * @param enable false to disable WiFi settings
   */
  void setEnableWifiSettings(const bool enable) {
    mEnableWifiSettings = enable;
  }
  /**
   * Enable or disable the alternative device name settings. Default: true.
   * AltDeviceName settings need to be configured before calling init.
   * @param enable false to disable alternative device name
   */
  void setEnableAltDeviceName(const bool enable) {
    mEnableAltDeviceName = enable;
  }

  std::string getAltDeviceName() { return mAltDeviceName; }

  void setAltDeviceName(std::string altDeviceName);

  void registerDeviceNameChangeCallback(
      const ble_service_callback_t &callback) const;

  void registerWifiChangedCallback(
      const wifi_changed_callback_t wifiChangedCallback) const;

private:
  bool mEnableWifiSettings = true;
  bool mEnableAltDeviceName = true;
  std::string mWiFiSsid;
  std::string mAltDeviceName;
};

#endif // ARDUINO_UPT_BLE_SERVER_SETTINGS_BLE_SERVICE_H
