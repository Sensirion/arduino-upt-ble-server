#include "DataProvider.h"
#include <cmath>
#include <utility>

void DataProvider::begin() {
  setupBLEInfrastructure();

  mSampleHistory.setSampleSize(mSampleConfig.sampleSizeBytes);

  // Fill advertisement header
  mAdvertisementHeader.writeCompanyId(0x06D5);
  mAdvertisementHeader.writeSensirionAdvertisementType(0x00);

  // Use part of MAC address as device id
  const std::string macAddress = mBleLibrary.getDeviceAddress();
  mAdvertisementHeader.writeDeviceId(
      static_cast<uint8_t>(
          strtol(macAddress.substr(12, 14).c_str(), nullptr, 16)),
      static_cast<uint8_t>(
          strtol(macAddress.substr(15, 17).c_str(), nullptr, 16)));

  mBleLibrary.setAdvertisingData(buildAdvertisementData());
  mBleLibrary.startAdvertising();
}

void DataProvider::writeValueToCurrentSample(const float value,
                                             const SignalType signalType) {
  // Check for a valid value
  if (isnan(value)) {
    return;
  }

  // Check for the correct signal type
  if (mSampleConfig.sampleSlots.count(signalType) == 0) {
    // implies signal type is not part of the sample
    return;
  }

  // Get offset and converted value from the sample configuration
  const uint16_t convertedValue =
      mSampleConfig.sampleSlots.at(signalType).encodingFunction(value);
  const size_t offset = mSampleConfig.sampleSlots.at(signalType).offset;

  mCurrentSample.writeValue(convertedValue, offset);
}

void DataProvider::commitSample() {
  const uint64_t currentTimeStamp = millis();
  if ((currentTimeStamp - mLatestHistoryTimeStamp) >=
      mHistoryIntervalMilliSeconds) {
    mSampleHistory.putSample(mCurrentSample);
    mLatestHistoryTimeStamp = currentTimeStamp;

    if (mDownloadState == INACTIVE) {
      mLatestHistoryTimeStampAtDownloadStart = currentTimeStamp;
      mBleLibrary.characteristicSetValue(
          NUMBER_OF_SAMPLES_UUID, mSampleHistory.numberOfSamplesInHistory());
    }
  }

  // Update Advertising
  mBleLibrary.stopAdvertising();
  mBleLibrary.setAdvertisingData(buildAdvertisementData());
  mBleLibrary.startAdvertising();
}

void DataProvider::setBatteryLevel(const int value) const {
  mBleLibrary.characteristicSetValue(BATTERY_LEVEL_UUID, value);
}

void DataProvider::handleDownload() {
  if (mDownloadState == INACTIVE) {
    return;
  }

  // Download Completed
  if (mDownloadState == COMPLETED) {
    mDownloadSequenceIdx = 0;
    mNrOfSamplesRequested = 0;
    mNumberOfSamplesToDownload = 0;
    mNumberOfSamplePacketsToDownload = 0;
    mDownloadState = INACTIVE;
    return;
  }

  // Start Download
  if (mDownloadState == START) {
    if (mNrOfSamplesRequested > 0 &&
        mNrOfSamplesRequested < mSampleHistory.numberOfSamplesInHistory()) {
      mNumberOfSamplesToDownload = mNrOfSamplesRequested;
    } else {
      mNumberOfSamplesToDownload = mSampleHistory.numberOfSamplesInHistory();
    }
    mNumberOfSamplePacketsToDownload =
        numberOfPacketsRequired(mNumberOfSamplesToDownload);
    const DownloadHeader header = buildDownloadHeader();
    mBleLibrary.characteristicSetValue(DOWNLOAD_PACKET_UUID,
                                       header.getDataArray().data(),
                                       header.getDataArray().size());
    mDownloadState = DOWNLOADING;
    mSampleHistory.startReadOut(mNumberOfSamplesToDownload);

  } else if (mDownloadState == DOWNLOADING) { // Continue Download
    const DownloadPacket packet = buildDownloadPacket();
    mBleLibrary.characteristicSetValue(DOWNLOAD_PACKET_UUID,
                                       packet.getDataArray().data(),
                                       packet.getDataArray().size());
  }

  mBleLibrary.characteristicNotify(DOWNLOAD_PACKET_UUID);

  ++mDownloadSequenceIdx;
  if (mDownloadSequenceIdx >= mNumberOfSamplePacketsToDownload + 1) {
    mDownloadState = COMPLETED;
  }
}

void DataProvider::setSampleConfig(const DataType dataType) {
  mSampleConfig = sampleConfigSelector.at(dataType);
  mSampleHistory.setSampleSize(mSampleConfig.sampleSizeBytes);
}

String DataProvider::getDeviceIdString() const {
  char cDevId[6];
  const std::string macAddress = mBleLibrary.getDeviceAddress();
  snprintf(cDevId, sizeof(cDevId), "%s:%s", macAddress.substr(12, 14).c_str(),
           macAddress.substr(15, 17).c_str());
  return cDevId;
}

bool DataProvider::isDownloading() const {
  return (mDownloadState != INACTIVE);
}

void DataProvider::enableAltDeviceName() { mEnableAltDeviceName = true; }

std::string DataProvider::getAltDeviceName() { return mAltDeviceName; }

void DataProvider::setAltDeviceName(std::string altDeviceName) {
  mAltDeviceName = std::move(altDeviceName);
  mBleLibrary.characteristicSetValue(
      ALT_DEVICE_NAME_UUID,
      reinterpret_cast<const uint8_t *>(mAltDeviceName.c_str()),
      mAltDeviceName.length());
}

std::string DataProvider::buildAdvertisementData() {
  mAdvertisementHeader.writeSampleType(mSampleConfig.sampleType);
  std::string data = mAdvertisementHeader.getDataString();
  data.append(mCurrentSample.getDataString());
  return data;
}

DownloadHeader DataProvider::buildDownloadHeader() const {
  DownloadHeader header;
  const auto age =
      static_cast<uint32_t>(millis() - mLatestHistoryTimeStampAtDownloadStart);
  header.setDownloadSampleType(mSampleConfig.downloadType);
  header.setIntervalMilliSeconds(mHistoryIntervalMilliSeconds);
  header.setAgeOfLatestSampleMilliSeconds(age);
  header.setDownloadSampleCount(
      static_cast<uint16_t>(mNumberOfSamplesToDownload));
  return header;
}

DownloadPacket DataProvider::buildDownloadPacket() {
  DownloadPacket packet;
  packet.setDownloadSequenceNumber(mDownloadSequenceIdx);
  bool allSamplesRead = false;
  for (int i = 0; i < mSampleConfig.sampleCountPerPacket && !allSamplesRead;
       ++i) {
    Sample sample = mSampleHistory.readOutNextSample(allSamplesRead);
    packet.writeSample(sample, mSampleConfig.sampleSizeBytes, i);
  }
  return packet;
}

uint32_t DataProvider::numberOfPacketsRequired(uint32_t numberOfSamples) const {
  uint32_t numberOfPacketsRequired =
      numberOfSamples / mSampleConfig.sampleCountPerPacket;
  if ((numberOfSamples % mSampleConfig.sampleCountPerPacket) != 0) {
    ++numberOfPacketsRequired;
  }
  return numberOfPacketsRequired;
}

void DataProvider::setupDownloadService() {
  mBleLibrary.createService(DOWNLOAD_SERVICE_UUID);
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                   NUMBER_OF_SAMPLES_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(NUMBER_OF_SAMPLES_UUID,
                                     mSampleHistory.numberOfSamplesInHistory());
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                   REQUESTED_SAMPLES_UUID,
                                   Permission::WRITE_PERMISSION);
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                   SAMPLE_HISTORY_INTERVAL_UUID,
                                   Permission::READWRITE_PERMISSION);
  mBleLibrary.characteristicSetValue(SAMPLE_HISTORY_INTERVAL_UUID,
                                     mHistoryIntervalMilliSeconds);
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID, DOWNLOAD_PACKET_UUID,
                                   Permission::NOTIFY_PERMISSION);
  mBleLibrary.startService(DOWNLOAD_SERVICE_UUID);

  // create and register callback
  auto onHistoryIntervalChange = [&](const std::string &value) {
    const uint32_t sampleIntervalMs =
        value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);

    mHistoryIntervalMilliSeconds = sampleIntervalMs;
    mSampleHistory.reset();
    mBleLibrary.characteristicSetValue(
        NUMBER_OF_SAMPLES_UUID, mSampleHistory.numberOfSamplesInHistory());
  };

  mBleLibrary.registerCharacteristicCallback(SAMPLE_HISTORY_INTERVAL_UUID,
                                             onHistoryIntervalChange);

  auto onNrOfSamplesRequest = [&](const std::string &value) {
    const uint32_t nrOfSamples =
        value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);

    mNrOfSamplesRequested = nrOfSamples;
  };
  mBleLibrary.registerCharacteristicCallback(REQUESTED_SAMPLES_UUID,
                                             onNrOfSamplesRequest);
}

void DataProvider::setupSettingsService() {
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

    // ReSharper disable once CppDFAUnreachableFunctionCall
    auto onWifiPasswordChange = [&](const std::string &value) {
      for (const auto &callback : mWiFiChangedCallbacks) {
        const std::string currentSsid = mWiFiSsid;
        callback(currentSsid, value);
      }
    };

    mBleLibrary.registerCharacteristicCallback(WIFI_SSID_UUID,
                                               onWifiSsidChange);
    mBleLibrary.registerCharacteristicCallback(WIFI_PWD_UUID,
                                               onWifiPasswordChange);
    // ReSharper restore CppDFAUnreachableCode
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
}

void DataProvider::setupBatteryService() const {
  mBleLibrary.createService(BATTERY_SERVICE_UUID);
  mBleLibrary.createCharacteristic(BATTERY_SERVICE_UUID, BATTERY_LEVEL_UUID,
                                   READ_PERMISSION);
  mBleLibrary.characteristicSetValue(BATTERY_LEVEL_UUID, 0);
  mBleLibrary.startService(BATTERY_SERVICE_UUID);
}

void DataProvider::setupFrcService() {
  mBleLibrary.createService(SCD_SERVICE_UUID);
  mBleLibrary.createCharacteristic(SCD_SERVICE_UUID, SCD_FRC_REQUEST_UUID,
                                   WRITE_PERMISSION);
  mBleLibrary.characteristicSetValue(SCD_FRC_REQUEST_UUID, 0);
  mBleLibrary.startService(SCD_SERVICE_UUID);

  auto onFRCRequest = [&](const std::string &value) {
    // co2 level is encoded in lower two bytes, little endian
    // the first two bytes are obfuscation and can be ignored
    const uint16_t referenceCO2Level = value[2] | (value[3] << 8);
    mFrcRequested = true;
    mReferenceCo2Level = referenceCO2Level;
  };

  mBleLibrary.registerCharacteristicCallback(SCD_FRC_REQUEST_UUID,
                                             onFRCRequest);
}

void DataProvider::setupBLEInfrastructure() {
  mBleLibrary.init();
  mBleLibrary.createServer();

  // Download Service
  setupDownloadService();

  // Settings Service
  setupSettingsService();

  // Battery Service
  if (mEnableBatteryService) {
    setupBatteryService();
  }

  // SCD FRC Service
  if (mEnableFRCService) {
    setupFrcService();
  }

  mBleLibrary.setProviderCallbacks(this);
}

void DataProvider::onConnect() {
  mDownloadSequenceIdx = 0;
  mDownloadState = INACTIVE;
}

void DataProvider::onDisconnect() { mDownloadState = INACTIVE; }

void DataProvider::onSubscribe(const std::string &uuid,
                               const uint16_t subValue) {
  if (strcmp(uuid.c_str(), DOWNLOAD_PACKET_UUID) == 0 && subValue == 1) {
    // start download
    mDownloadState = START;
  }
}

void DataProvider::completeFRCRequest() {
  mFrcRequested = false;
  mReferenceCo2Level = 0;
}

bool DataProvider::isFRCRequested() const { return mFrcRequested; }

uint32_t DataProvider::getReferenceCO2Level() const {
  return mReferenceCo2Level;
}

void DataProvider::registerWifiChangedCallback(
    const std::function<void(std::string, std::string)> &callback) {
  mWiFiChangedCallbacks.push_back(callback);
}

void DataProvider::registerDeviceNameChangeCallback(
    const std::function<void(std::string)> &callback) const {
  mBleLibrary.registerCharacteristicCallback(ALT_DEVICE_NAME_UUID, callback);
}
