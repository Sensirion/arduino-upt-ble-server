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
    DownloadHeader header = buildDownloadHeader();
    mBleLibrary.characteristicSetValue(DOWNLOAD_PACKET_UUID,
                                       header.getDataArray().data(),
                                       header.getDataArray().size());
    mDownloadState = DOWNLOADING;
    mSampleHistory.startReadOut(mNumberOfSamplesToDownload);

  } else if (mDownloadState == DOWNLOADING) { // Continue Download
    DownloadPacket packet = buildDownloadPacket();
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

void DataProvider::onAltDeviceNameChange(std::string altDeviceName) {
  setAltDeviceName(altDeviceName);
  for (const auto &callback : mDeviceNameChangeCallbacks) {
    callback(altDeviceName);
  }
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

void DataProvider::setupBLEInfrastructure() {
  mBleLibrary.init();
  mBleLibrary.createServer();

  // Download Service
  mBleLibrary.createService(DOWNLOAD_SERVICE_UUID);
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                   NUMBER_OF_SAMPLES_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(NUMBER_OF_SAMPLES_UUID, 0);
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                   REQUESTED_SAMPLES_UUID,
                                   Permission::WRITE_PERMISSION);
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                   SAMPLE_HISTORY_INTERVAL_UUID,
                                   Permission::READWRITE_PERMISSION);
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID, DOWNLOAD_PACKET_UUID,
                                   Permission::NOTIFY_PERMISSION);
  mBleLibrary.startService(DOWNLOAD_SERVICE_UUID);

  // Settings Service
  mBleLibrary.createService(SETTINGS_SERVICE_UUID);
  if (mEnableWifiSettings) {
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
  }
  if (mEnableAltDeviceName) {
    mBleLibrary.createCharacteristic(SETTINGS_SERVICE_UUID,
                                     ALT_DEVICE_NAME_UUID,
                                     Permission::READWRITE_PERMISSION);
    setAltDeviceName(mAltDeviceName);
  }
  mBleLibrary.startService(SETTINGS_SERVICE_UUID);

  // Battery Service
  if (mEnableBatteryService) {
    mBleLibrary.createService(BATTERY_SERVICE_UUID);
    mBleLibrary.createCharacteristic(BATTERY_SERVICE_UUID, BATTERY_LEVEL_UUID,
                                     READ_PERMISSION);
    mBleLibrary.characteristicSetValue(BATTERY_LEVEL_UUID, 0);
    mBleLibrary.startService(BATTERY_SERVICE_UUID);
  }

  // SCD FRC Service
  if (mEnableFRCService) {
    mBleLibrary.createService(SCD_SERVICE_UUID);
    mBleLibrary.createCharacteristic(SCD_SERVICE_UUID, SCD_FRC_REQUEST_UUID,
                                     WRITE_PERMISSION);
    mBleLibrary.characteristicSetValue(SCD_FRC_REQUEST_UUID, 0);
    mBleLibrary.startService(SCD_SERVICE_UUID);
  }

  mBleLibrary.setProviderCallbacks(this);
  mBleLibrary.characteristicSetValue(SAMPLE_HISTORY_INTERVAL_UUID,
                                     mHistoryIntervalMilliSeconds);
  mBleLibrary.characteristicSetValue(NUMBER_OF_SAMPLES_UUID,
                                     mSampleHistory.numberOfSamplesInHistory());
}

void DataProvider::onHistoryIntervalChange(const uint32_t interval) {
  mHistoryIntervalMilliSeconds = static_cast<uint64_t>(interval);
  mSampleHistory.reset();
  mBleLibrary.characteristicSetValue(NUMBER_OF_SAMPLES_UUID,
                                     mSampleHistory.numberOfSamplesInHistory());
}

void DataProvider::onConnectionEvent() {
  mDownloadSequenceIdx = 0;
  mDownloadState = INACTIVE;
}

void DataProvider::onDownloadRequest() { mDownloadState = START; }

void DataProvider::onFRCRequest(const uint16_t referenceCo2Level) {
  mFrcRequested = true;
  mReferenceCo2Level = referenceCo2Level;
}

void DataProvider::onNrOfSamplesRequest(const uint32_t nrOfSamples) {
  mNrOfSamplesRequested = nrOfSamples;
}

void DataProvider::completeFRCRequest() {
  mFrcRequested = false;
  mReferenceCo2Level = 0;
}

bool DataProvider::isFRCRequested() const { return mFrcRequested; }

uint32_t DataProvider::getReferenceCO2Level() const {
  return mReferenceCo2Level;
}

void DataProvider::onWifiSsidChange(const std::string ssid) {
  mWiFiSsid = ssid;
}

void DataProvider::onWifiPasswordChange(const std::string pwd) {
  for (const auto &callback : mWiFiChangedCallbacks) {
    const std::string ssid = mWiFiSsid;
    callback(ssid, pwd);
  }
}

void DataProvider::registerWifiChangedCallback(
    const std::function<void(std::string, std::string)> &callback) {
  mWiFiChangedCallbacks.push_back(callback);
}

void DataProvider::registerDeviceNameChangeCallback(
    const std::function<void(std::string)> &callback) {
  mDeviceNameChangeCallbacks.push_back(callback);
}
