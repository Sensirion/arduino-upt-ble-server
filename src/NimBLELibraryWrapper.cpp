#include "NimBLELibraryWrapper.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

uint NimBLELibraryWrapper::mNumberOfInstances = 0;

struct WrapperPrivateData final : BLECharacteristicCallbacks,
                                  BLEServerCallbacks {
  NimBLEAdvertising *pNimBLEAdvertising{};
  bool BLEDeviceRunning = false;

  // owned by NimBLE
  NimBLEServer *pBLEServer{};
  NimBLEService *services[MAX_NUMBER_OF_SERVICES] = {nullptr};
  NimBLECharacteristic *characteristics[MAX_NUMBER_OF_CHARACTERISTICS] = {
      nullptr};

  // BLEServerCallbacks
  void onConnect(BLEServer *serverInst) override;

  void onDisconnect(BLEServer *serverInst) override;

  // BLECharacteristicCallbacks
  void onWrite(BLECharacteristic *characteristic) override;

  void onSubscribe(NimBLECharacteristic *pCharacteristic,
                   ble_gap_conn_desc *desc, uint16_t subValue) override;

  // DataProvider Callbacks
  IProviderCallbacks *providerCallbacks = nullptr;
};

void WrapperPrivateData::onConnect(NimBLEServer *serverInst) {
  if (providerCallbacks != nullptr) {
    providerCallbacks->onConnectionEvent();
  }
}

void WrapperPrivateData::onDisconnect(BLEServer *serverInst) {
  if (providerCallbacks != nullptr) {
    providerCallbacks->onConnectionEvent();
  }
}

void WrapperPrivateData::onSubscribe(NimBLECharacteristic *pCharacteristic,
                                     ble_gap_conn_desc *desc,
                                     const uint16_t subValue) {
  if ((providerCallbacks != nullptr) && (subValue == 1)) {
    providerCallbacks->onDownloadRequest();
  }
}

void WrapperPrivateData::onWrite(BLECharacteristic *characteristic) {
  if (providerCallbacks == nullptr) {
    return;
  }

  if (characteristic->getUUID().toString() == SAMPLE_HISTORY_INTERVAL_UUID) {
    const std::string value = characteristic->getValue();
    const uint32_t sampleIntervalMs =
        value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);
    providerCallbacks->onHistoryIntervalChange(sampleIntervalMs);
  } else if (characteristic->getUUID().toString() == WIFI_SSID_UUID) {
    providerCallbacks->onWifiSsidChange(characteristic->getValue());
  } else if (characteristic->getUUID().toString() == WIFI_PWD_UUID) {
    providerCallbacks->onWifiPasswordChange(characteristic->getValue());
  } else if (characteristic->getUUID().toString() == SCD_FRC_REQUEST_UUID) {
    const std::string value = characteristic->getValue();
    // co2 level is encoded in lower two bytes, little endian
    // the first two bytes are obfuscation and can be ignored
    const uint16_t referenceCO2Level = value[2] | (value[3] << 8);
    providerCallbacks->onFRCRequest(referenceCO2Level);
  } else if (characteristic->getUUID().toString() == REQUESTED_SAMPLES_UUID) {
    const std::string value = characteristic->getValue();
    const uint32_t nrOfSamples =
        value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);
    providerCallbacks->onNrOfSamplesRequest(nrOfSamples);
  } else if (characteristic->getUUID().toString() == ALT_DEVICE_NAME_UUID) {
    providerCallbacks->onAltDeviceNameChange(characteristic->getValue());
  }
}

WrapperPrivateData *NimBLELibraryWrapper::mData = nullptr;

NimBLELibraryWrapper::NimBLELibraryWrapper() {
  if (mNumberOfInstances == 0) {
    mData = new WrapperPrivateData();
    ++mNumberOfInstances;
  }
}

NimBLELibraryWrapper::~NimBLELibraryWrapper() {
  if (mNumberOfInstances == 1) {
    release();
    delete mData;
    --mNumberOfInstances;
  }
}

void NimBLELibraryWrapper::release() {
  if (mData == nullptr) {
    return;
  }
  if (mData->BLEDeviceRunning) {
    NimBLEDevice::deinit(true);
    mData->BLEDeviceRunning = false;
  }
}

void NimBLELibraryWrapper::init() {
  if (mData->BLEDeviceRunning) {
    return;
  }
  NimBLEDevice::init(GADGET_NAME);
  mData->BLEDeviceRunning = true;

  mData->pNimBLEAdvertising = NimBLEDevice::getAdvertising();
  // Helps with iPhone connection issues (copy/paste)
  mData->pNimBLEAdvertising->setMinPreferred(0x06);
  mData->pNimBLEAdvertising->setMaxPreferred(0x12);

  // Set interval to 1 s. Unit is in 0.625 ms.
  mData->pNimBLEAdvertising->setMinInterval(1600);
  mData->pNimBLEAdvertising->setMaxInterval(1600);
}

void NimBLELibraryWrapper::createServer() {
  mData->pBLEServer = NimBLEDevice::createServer();
  mData->pBLEServer->setCallbacks(mData);
}

bool NimBLELibraryWrapper::createService(const char *uuid) {
  for (auto &service : mData->services) {
    if (service == nullptr) {
      service = mData->pBLEServer->createService(uuid);
      return true;
    }
  }
  return false; // no space in services[]
}

bool NimBLELibraryWrapper::createCharacteristic(const char *serviceUuid,
                                                const char *characteristicUuid,
                                                const Permission permission) {
  NimBLEService *service = lookupService(serviceUuid);
  if (service == nullptr) { // invalid service uuid
    return false;
  }
  for (auto &characteristic : mData->characteristics) {
    if (characteristic == nullptr) {
      switch (permission) {
      case READWRITE_PERMISSION:
        characteristic = service->createCharacteristic(characteristicUuid);
        characteristic->setCallbacks(mData);
        return true;
      case READ_PERMISSION:
        characteristic = service->createCharacteristic(characteristicUuid,
                                                       NIMBLE_PROPERTY::READ);
        return true;
      case WRITE_PERMISSION:
        characteristic = service->createCharacteristic(characteristicUuid,
                                                       NIMBLE_PROPERTY::WRITE);
        characteristic->setCallbacks(mData);
        return true;
      case NOTIFY_PERMISSION:
        characteristic = service->createCharacteristic(characteristicUuid,
                                                       NIMBLE_PROPERTY::NOTIFY);
        characteristic->setCallbacks(mData);
        return true;
      default:
        return false;
      }
    }
  }
  return false; // no space in characteristics[]
}

bool NimBLELibraryWrapper::startService(const char *uuid) {
  NimBLEService *service = lookupService(uuid);
  if (service == nullptr) {
    return false;
  }
  bool success = service->start();
  return success;
}

void NimBLELibraryWrapper::setAdvertisingData(const std::string &data) {
  NimBLEAdvertisementData advert;
  advert.setName(GADGET_NAME);
  advert.setManufacturerData(data);
  mData->pNimBLEAdvertising->setAdvertisementData(advert);
}

void NimBLELibraryWrapper::startAdvertising() {
  mData->pNimBLEAdvertising->start();
}

void NimBLELibraryWrapper::stopAdvertising() {
  mData->pNimBLEAdvertising->stop();
}

std::string NimBLELibraryWrapper::getDeviceAddress() {
  return NimBLEDevice::getAddress().toString();
}

bool NimBLELibraryWrapper::characteristicSetValue(const char *uuid,
                                                  const uint8_t *data,
                                                  size_t size) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  pCharacteristic->setValue(data, size);
  return true;
}

bool NimBLELibraryWrapper::characteristicSetValue(const char *uuid, int value) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  pCharacteristic->setValue(value);
  return true;
}

bool NimBLELibraryWrapper::characteristicSetValue(const char *uuid,
                                                  uint32_t value) {
  return characteristicSetValue(uuid, static_cast<uint64_t>(value));
}

bool NimBLELibraryWrapper::characteristicSetValue(const char *uuid,
                                                  uint64_t value) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  pCharacteristic->setValue(value);
  return true;
}

std::string NimBLELibraryWrapper::characteristicGetValue(const char *uuid) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return "";
  }
  return pCharacteristic->getValue();
}

bool NimBLELibraryWrapper::characteristicNotify(const char *uuid) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  pCharacteristic->notify(true);
  return true;
}

void NimBLELibraryWrapper::setProviderCallbacks(
    IProviderCallbacks *providerCallbacks) {
  mData->providerCallbacks = providerCallbacks;
}

NimBLECharacteristic *
NimBLELibraryWrapper::lookupCharacteristic(const char *uuid) {
  // NimBLECharacteristic* pCharacteristic = nullptr;
  for (const auto &characteristic : mData->characteristics) {
    if (characteristic == nullptr) {
      continue;
    }
    if (strcmp(characteristic->getUUID().toString().c_str(), uuid) == 0) {
      return characteristic;
    }
  }
  return nullptr;
}

NimBLEService *NimBLELibraryWrapper::lookupService(const char *uuid) {
  for (const auto &service : mData->services) {
    if (service == nullptr) {
      continue;
    }
    if (strcmp(service->getUUID().toString().c_str(), uuid) == 0) {
      return service;
    }
  }
  return nullptr;
}