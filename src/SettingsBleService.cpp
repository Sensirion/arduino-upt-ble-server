#include "SettingsBleService.h"

#include <HardwareSerial.h>
#include <cstring>

bool SettingsBleService::begin() {
  mBleLibrary.createService(SETTINGS_SERVICE_UUID);
  // ReSharper disable once CppDFAConstantConditions
  if (mEnableWifiSettings) {
    // ReSharper disable CppDFAUnreachableCode
    mBleLibrary.createCharacteristic(SETTINGS_SERVICE_UUID, WIFI_SSID_UUID,
                                     Permission::READWRITE_PERMISSION);
    const auto ssid = "ssid";
    mBleLibrary.characteristicSetValue(
        WIFI_SSID_UUID, reinterpret_cast<const uint8_t *>(ssid), strlen(ssid));
    mBleLibrary.createCharacteristic(SETTINGS_SERVICE_UUID, WIFI_PWD_UUID,
                                     Permission::WRITE_PERMISSION);
    const auto pwd = "password";
    mBleLibrary.characteristicSetValue(
        WIFI_PWD_UUID, reinterpret_cast<const uint8_t *>(pwd), strlen(pwd));

    // ReSharper disable once CppDFAUnreachableFunctionCall
    auto onWifiSsidChange = [&](const std::string &value) {
      mWiFiSsid = value;
    };

    mBleLibrary.registerCharacteristicCallback(WIFI_SSID_UUID,
                                               onWifiSsidChange);
  }

  // ReSharper disable once CppDFAConstantConditions
  if (mEnableAltDeviceName) {
    mBleLibrary.createCharacteristic(SETTINGS_SERVICE_UUID,
                                     ALT_DEVICE_NAME_UUID,
                                     Permission::READWRITE_PERMISSION);
    setAltDeviceName(mAltDeviceName);

    auto onAltDeviceNameChange = [&](const std::string &deviceName) {
      setAltDeviceName(deviceName);
    };

    mBleLibrary.registerCharacteristicCallback(ALT_DEVICE_NAME_UUID,
                                               onAltDeviceNameChange);
  }

  mBleLibrary.startService(SETTINGS_SERVICE_UUID);
  return true;
}

void SettingsBleService::setAltDeviceName(std::string altDeviceName) {
  mAltDeviceName = std::move(altDeviceName);
  mBleLibrary.characteristicSetValue(
      ALT_DEVICE_NAME_UUID,
      reinterpret_cast<const uint8_t *>(mAltDeviceName.c_str()),
      mAltDeviceName.length());
}

void SettingsBleService::registerDeviceNameChangeCallback(
    const ble_service_callback_t &callback) const {
  mBleLibrary.registerCharacteristicCallback(ALT_DEVICE_NAME_UUID, callback);
}

void SettingsBleService::registerWifiChangedCallback(
    // ReSharper disable once CppPassValueParameterByConstReference
    const wifi_changed_callback_t wifiChangedCallback) const {
  auto onWifiPwdChanged = [=](const std::string &wifiPwd) {
    wifiChangedCallback(mWiFiSsid, wifiPwd);
  };
  mBleLibrary.registerCharacteristicCallback(WIFI_PWD_UUID, onWifiPwdChanged);
}