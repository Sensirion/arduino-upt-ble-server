#include "NimBLELibraryWrapper.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

uint NimBLELibraryWrapper::mNumberOfInstances = 0;

struct WrapperPrivateData final : BLECharacteristicCallbacks,
                                  BLEServerCallbacks {
  NimBLEAdvertising *pNimBLEAdvertising{};
  bool BLEDeviceRunning = false;
  std::unordered_map<std::string, std::vector<callback_t>> mCallbacks;

  // owned by NimBLE
  NimBLEServer *pBLEServer{};
  NimBLEService *services[MAX_NUMBER_OF_SERVICES] = {nullptr};
  NimBLECharacteristic *characteristics[MAX_NUMBER_OF_CHARACTERISTICS] = {
      nullptr};

  // Handle callbacks on characteristics write
  void initCallbackForCharacteristic(const std::string &uuid);
  void registerCallback(const char *uuid, const callback_t &callback);

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
  if (providerCallbacks == nullptr) {
    return;
  }
  providerCallbacks->onConnect();
}

void WrapperPrivateData::onDisconnect(BLEServer *serverInst) {
  if (providerCallbacks == nullptr) {
    return;
  }
  providerCallbacks->onDisconnect();
}

void WrapperPrivateData::onSubscribe(NimBLECharacteristic *pCharacteristic,
                                     ble_gap_conn_desc *desc,
                                     const uint16_t subValue) {
  if (providerCallbacks == nullptr) {
    return;
  }

  providerCallbacks->onSubscribe(pCharacteristic->getUUID().toString(),
                                 subValue);
}

void WrapperPrivateData::onWrite(BLECharacteristic *characteristic) {
  const std::string uuid = characteristic->getUUID().toString();

  if (mCallbacks.find(uuid) == mCallbacks.end()) {
    // no callbacks registered for characteristic
    return;
  }

  const std::string value = characteristic->getValue();
  for (const auto &callback : mCallbacks[uuid]) {
    callback(value);
  }
}

void WrapperPrivateData::initCallbackForCharacteristic(
    const std::string &uuid) {
  if (mCallbacks.find(uuid) == mCallbacks.end()) {
    mCallbacks[uuid].reserve(3);
  }
}

void WrapperPrivateData::registerCallback(const char *const uuid,
                                          const callback_t &callback) {
  mCallbacks[uuid].push_back(callback);
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

bool NimBLELibraryWrapper::createService(const char *const uuid) {
  for (auto &service : mData->services) {
    if (service == nullptr) {
      service = mData->pBLEServer->createService(uuid);
      return true;
    }
  }
  return false; // no space in services[]
}

bool NimBLELibraryWrapper::createCharacteristic(
    const char *const serviceUuid, const char *const characteristicUuid,
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
        mData->initCallbackForCharacteristic(characteristicUuid);
        return true;
      case READ_PERMISSION:
        characteristic = service->createCharacteristic(characteristicUuid,
                                                       NIMBLE_PROPERTY::READ);
        return true;
      case WRITE_PERMISSION:
        characteristic = service->createCharacteristic(characteristicUuid,
                                                       NIMBLE_PROPERTY::WRITE);
        characteristic->setCallbacks(mData);
        mData->initCallbackForCharacteristic(characteristicUuid);
        return true;
      case NOTIFY_PERMISSION:
        characteristic = service->createCharacteristic(characteristicUuid,
                                                       NIMBLE_PROPERTY::NOTIFY);
        characteristic->setCallbacks(mData);
        mData->initCallbackForCharacteristic(characteristicUuid);
        return true;
      default:
        return false;
      }
    }
  }
  return false; // no space in characteristics[]
}

bool NimBLELibraryWrapper::startService(const char *const uuid) {
  NimBLEService *service = lookupService(uuid);
  if (service == nullptr) {
    return false;
  }
  const bool success = service->start();
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

bool NimBLELibraryWrapper::characteristicSetValue(const char *const uuid,
                                                  const uint8_t *data,
                                                  const size_t size) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  pCharacteristic->setValue(data, size);
  return true;
}

bool NimBLELibraryWrapper::characteristicSetValue(const char *const uuid,
                                                  const int value) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  pCharacteristic->setValue(value);
  return true;
}

bool NimBLELibraryWrapper::characteristicSetValue(const char *const uuid,
                                                  const uint32_t value) {
  return characteristicSetValue(uuid, static_cast<uint64_t>(value));
}

bool NimBLELibraryWrapper::characteristicSetValue(const char *const uuid,
                                                  const uint64_t value) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  pCharacteristic->setValue(value);
  return true;
}

std::string
NimBLELibraryWrapper::characteristicGetValue(const char *const uuid) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return "";
  }
  return pCharacteristic->getValue();
}

bool NimBLELibraryWrapper::characteristicNotify(const char *const uuid) {
  NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  pCharacteristic->notify(true);
  return true;
}
void NimBLELibraryWrapper::registerCharacteristicCallback(
    const char *uuid, const callback_t &callback) {
  mData->registerCallback(uuid, callback);
}

void NimBLELibraryWrapper::setProviderCallbacks(
    IProviderCallbacks *providerCallbacks) {
  mData->providerCallbacks = providerCallbacks;
}

NimBLECharacteristic *
NimBLELibraryWrapper::lookupCharacteristic(const char *const uuid) {
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

NimBLEService *NimBLELibraryWrapper::lookupService(const char *const uuid) {
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