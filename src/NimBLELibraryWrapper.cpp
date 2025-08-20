#include "NimBLELibraryWrapper.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

namespace sensirion::upt::ble_server {

static constexpr unsigned int INITIAL_NUMBER_OF_SERVICES = 4;
static constexpr unsigned int INITIAL_NUMBER_OF_CHARACTERISTICS = 12;

uint NimBLELibraryWrapper::mNumberOfInstances = 0;

struct WrapperPrivateData final : NimBLECharacteristicCallbacks,
                                  NimBLEServerCallbacks {
  NimBLEAdvertising *pNimBLEAdvertising{};
  bool BLEDeviceRunning = false;
  std::unordered_map<std::string, std::vector<ble_service_callback_t>>
      mCallbacks;

  // owned by NimBLE
  NimBLEServer *pBLEServer{};
  std::vector<NimBLEService *> services{};
  std::vector<NimBLECharacteristic *> characteristics{};

  // Handle callbacks on characteristics write
  void initCallbackForCharacteristic(const std::string &uuid);
  void registerCallback(const char *uuid,
                        const ble_service_callback_t &callback);

  // BLEServerCallbacks
  void onConnect(NimBLEServer *serverInst, NimBLEConnInfo &connInfo) override;

  void onDisconnect(NimBLEServer *serverInst, NimBLEConnInfo &connInfo,
                    int reason) override;

  // BLECharacteristicCallbacks
  void onWrite(NimBLECharacteristic *characteristic,
               NimBLEConnInfo &connInfo) override;

  void onSubscribe(NimBLECharacteristic *characteristic,
                   NimBLEConnInfo &connInfo, uint16_t subValue) override;

  // DataProvider Callbacks
  IProviderCallbacks *providerCallbacks = nullptr;
};

void WrapperPrivateData::onConnect(NimBLEServer *serverInst,
                                   NimBLEConnInfo &connInfo) {
  if (providerCallbacks == nullptr) {
    return;
  }
  providerCallbacks->onConnect();
}

void WrapperPrivateData::onDisconnect(BLEServer *serverInst,
                                      NimBLEConnInfo &connInfo, int reason) {
  if (providerCallbacks == nullptr) {
    return;
  }
  providerCallbacks->onDisconnect();
}

void WrapperPrivateData::onSubscribe(NimBLECharacteristic *characteristic,
                                     NimBLEConnInfo &connInfo,
                                     const uint16_t subValue) {
  if (providerCallbacks == nullptr) {
    return;
  }

  providerCallbacks->onSubscribe(characteristic->getUUID().toString(),
                                 subValue);
}

void WrapperPrivateData::onWrite(BLECharacteristic *characteristic,
                                 NimBLEConnInfo &connInfo) {
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

void WrapperPrivateData::registerCallback(
    const char *const uuid, const ble_service_callback_t &callback) {
  mCallbacks[uuid].push_back(callback);
}

WrapperPrivateData *NimBLELibraryWrapper::mData = nullptr;

NimBLELibraryWrapper::NimBLELibraryWrapper() {
  if (mNumberOfInstances == 0) {
    mData = new WrapperPrivateData();
    mData->services.reserve(INITIAL_NUMBER_OF_SERVICES);
    mData->characteristics.reserve(INITIAL_NUMBER_OF_CHARACTERISTICS);
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

bool NimBLELibraryWrapper::setAdvertisingInterval(const float minIntervalMs,
                                                  const float maxIntervalMs) {
  if (mData->BLEDeviceRunning) {
    return false;
  }

  const uint16_t minInterval = static_cast<uint16_t>(minIntervalMs / 1.25);
  const uint16_t maxInterval = static_cast<uint16_t>(maxIntervalMs / 1.25);

  mData->pNimBLEAdvertising->setMinInterval(minInterval);
  mData->pNimBLEAdvertising->setMaxInterval(maxInterval);
  return true;
}

bool NimBLELibraryWrapper::setPreferredConnectionInterval(
    const float minIntervalMs, const float maxIntervalMs) {
  if (mData->BLEDeviceRunning) {
    return false;
  }

  const uint16_t minInterval = static_cast<uint16_t>(minIntervalMs / 0.625);
  const uint16_t maxInterval = static_cast<uint16_t>(maxIntervalMs / 0.625);

  return mData->pNimBLEAdvertising->setPreferredParams(minInterval,
                                                       maxInterval);
}

void NimBLELibraryWrapper::init() {
  if (mData->BLEDeviceRunning) {
    return;
  }
  NimBLEDevice::init(GADGET_NAME);
  mData->BLEDeviceRunning = true;

  mData->pNimBLEAdvertising = NimBLEDevice::getAdvertising();
  // Helps with iPhone connection issues (copy/paste)
  setPreferredConnectionInterval(7.5, 22.5);

  // Set interval to advertise between 0.5 s and 2 s
  setAdvertisingInterval(500.0, 2000.0);
}

void NimBLELibraryWrapper::createServer() {
  mData->pBLEServer = NimBLEDevice::createServer();
  mData->pBLEServer->setCallbacks(mData);
  mData->pBLEServer->advertiseOnDisconnect(true);
}

bool NimBLELibraryWrapper::createService(const char *const uuid) {
  if (lookupService(uuid) != nullptr) {
    // service already registered
    return true;
  }
  const auto service = mData->pBLEServer->createService(uuid);
  mData->services.push_back(service);
  return true;
}

bool NimBLELibraryWrapper::createCharacteristic(
    const char *const serviceUuid, const char *const characteristicUuid,
    const Permission permission) {

  NimBLEService *service = lookupService(serviceUuid);
  if (service == nullptr) { // invalid service uuid
    return false;
  }

  if (lookupCharacteristic(characteristicUuid) != nullptr) {
    // characteristic already registered
    return true;
  }

  NimBLECharacteristic *characteristic;
  switch (permission) {
  case READWRITE_PERMISSION:
    characteristic = service->createCharacteristic(characteristicUuid);
    characteristic->setCallbacks(mData);
    mData->initCallbackForCharacteristic(characteristicUuid);
    break;
  case READ_PERMISSION:
    characteristic = service->createCharacteristic(characteristicUuid,
                                                   NIMBLE_PROPERTY::READ);
    break;
  case WRITE_PERMISSION:
    characteristic = service->createCharacteristic(characteristicUuid,
                                                   NIMBLE_PROPERTY::WRITE);
    characteristic->setCallbacks(mData);
    mData->initCallbackForCharacteristic(characteristicUuid);
    break;
  case NOTIFY_PERMISSION:
    characteristic = service->createCharacteristic(characteristicUuid,
                                                   NIMBLE_PROPERTY::NOTIFY);
    characteristic->setCallbacks(mData);
    mData->initCallbackForCharacteristic(characteristicUuid);
    break;
  default:
    return false;
  }
  mData->characteristics.push_back(characteristic);
  return true;
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
  const NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return "";
  }
  return pCharacteristic->getValue();
}

bool NimBLELibraryWrapper::characteristicNotify(const char *const uuid) {
  const NimBLECharacteristic *pCharacteristic = lookupCharacteristic(uuid);
  if (nullptr == pCharacteristic) {
    return false;
  }
  return pCharacteristic->indicate();
}
void NimBLELibraryWrapper::registerCharacteristicCallback(
    const char *uuid, const ble_service_callback_t &callback) {
  mData->registerCallback(uuid, callback);
}

void NimBLELibraryWrapper::setProviderCallbacks(
    IProviderCallbacks *providerCallbacks) {
  mData->providerCallbacks = providerCallbacks;
}

template <typename Container, typename UuidGetter>
typename Container::value_type findByUuid(Container &container,
                                          const char *const targetUuid,
                                          UuidGetter getUuid) {
  if (targetUuid == nullptr) {
    return nullptr;
  }

  const std::string targetUuidSw{targetUuid};

  const auto it =
      std::find_if(container.begin(), container.end(),
                   [&](auto *elem) { return getUuid(elem) == targetUuidSw; });
  return it != container.end() ? *it : nullptr;
}

NimBLECharacteristic *
NimBLELibraryWrapper::lookupCharacteristic(const char *const uuid) {
  return findByUuid(mData->characteristics, uuid,
                    [](auto *elem) { return elem->getUUID().toString(); });
}

NimBLEService *NimBLELibraryWrapper::lookupService(const char *const uuid) {
  return findByUuid(mData->services, uuid,
                    [](auto *elem) { return elem->getUUID().toString(); });
}

} // namespace sensirion::upt::ble_server