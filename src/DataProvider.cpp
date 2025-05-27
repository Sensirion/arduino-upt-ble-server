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
    std::string macAddress = BLELibrary.getDeviceAddress();
    mAdvertisementHeader.writeDeviceId(
        static_cast<uint8_t>(
            strtol(macAddress.substr(12, 14).c_str(), nullptr, 16)),
        static_cast<uint8_t>(
            strtol(macAddress.substr(15, 17).c_str(), nullptr, 16)));

    BLELibrary.setAdvertisingData(buildAdvertisementData());
    BLELibrary.startAdvertising();
}

__attribute__((unused)) void
DataProvider::writeValueToCurrentSample(float value, SignalType signalType) {
    // Check for valid value
    if (isnan(value)) {
        return;
    }

    // Check for correct signal type
    if (mSampleConfig.sampleSlots.count(signalType) == 0) {
        // implies signal type is not part of sample
        return;
    }

    // Get relevant metaData
    uint16_t (*encodingFunction)(float value) =
        mSampleConfig.sampleSlots.at(signalType).encodingFunction;
    size_t offset = mSampleConfig.sampleSlots.at(signalType).offset;

    // Convert float to 16-bit int
    uint16_t convertedValue = encodingFunction(value);
    mCurrentSample.writeValue(convertedValue, offset);
}

__attribute__((unused)) void DataProvider::commitSample() {
    uint64_t currentTimeStamp = millis();
    if ((currentTimeStamp - mLatestHistoryTimeStamp) >=
        mHistoryIntervalMilliSeconds) {
        mSampleHistory.putSample(mCurrentSample);
        mLatestHistoryTimeStamp = currentTimeStamp;

        if (mDownloadState == INACTIVE) {
            mLatestHistoryTimeStampAtDownloadStart = currentTimeStamp;
            BLELibrary.characteristicSetValue(
                NUMBER_OF_SAMPLES_UUID,
                mSampleHistory.numberOfSamplesInHistory());
        }
    }

    // Update Advertising
    BLELibrary.stopAdvertising();
    BLELibrary.setAdvertisingData(buildAdvertisementData());
    BLELibrary.startAdvertising();
}

__attribute__((unused)) void
DataProvider::setBatteryLevel(const int value) const {
    BLELibrary.characteristicSetValue(BATTERY_LEVEL_UUID, value);
}

__attribute__((unused)) void DataProvider::handleDownload() {
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
            mNumberOfSamplesToDownload =
                mSampleHistory.numberOfSamplesInHistory();
        }
        mNumberOfSamplePacketsToDownload =
            numberOfPacketsRequired(mNumberOfSamplesToDownload);
        DownloadHeader header = buildDownloadHeader();
        BLELibrary.characteristicSetValue(DOWNLOAD_PACKET_UUID,
                                          header.getDataArray().data(),
                                          header.getDataArray().size());
        mDownloadState = DOWNLOADING;
        mSampleHistory.startReadOut(mNumberOfSamplesToDownload);

    } else if (mDownloadState == DOWNLOADING) {  // Continue Download
        DownloadPacket packet = buildDownloadPacket();
        BLELibrary.characteristicSetValue(DOWNLOAD_PACKET_UUID,
                                          packet.getDataArray().data(),
                                          packet.getDataArray().size());
    }

    BLELibrary.characteristicNotify(DOWNLOAD_PACKET_UUID);

    ++mDownloadSequenceIdx;
    if (mDownloadSequenceIdx >= mNumberOfSamplePacketsToDownload + 1) {
        mDownloadState = COMPLETED;
    }
}

__attribute__((unused)) void DataProvider::setSampleConfig(DataType dataType) {
    mSampleConfig = sampleConfigSelector.at(dataType);
    mSampleHistory.setSampleSize(mSampleConfig.sampleSizeBytes);
}

__attribute__((unused)) String DataProvider::getDeviceIdString() const {
    char cDevId[6];
    std::string macAddress = BLELibrary.getDeviceAddress();
    snprintf(cDevId, sizeof(cDevId), "%s:%s", macAddress.substr(12, 14).c_str(),
             macAddress.substr(15, 17).c_str());
    return cDevId;
}

__attribute__((unused)) bool DataProvider::isDownloading() const {
    return (mDownloadState != DownloadState::INACTIVE);
}

__attribute__((unused)) void DataProvider::enableAltDeviceName() {
    mEnableAltDeviceName = true;
}

__attribute__((unused)) std::string DataProvider::getAltDeviceName() {
    return mAltDeviceName;
}

void DataProvider::setAltDeviceName(std::string altDeviceName) {
    mAltDeviceName = std::move(altDeviceName);
    BLELibrary.characteristicSetValue(
        ALT_DEVICE_NAME_UUID,
        reinterpret_cast<const uint8_t*>(mAltDeviceName.c_str()),
        mAltDeviceName.length());
}

void DataProvider::onAltDeviceNameChange(std::string altDeviceName) {
    setAltDeviceName(altDeviceName);
    for (const auto& callback : mDeviceNameChangeCallbacks) {
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
    auto age = static_cast<uint32_t>(millis() -
                                     mLatestHistoryTimeStampAtDownloadStart);
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
    BLELibrary.init();
    BLELibrary.createServer();

    // Download Service
    BLELibrary.createService(DOWNLOAD_SERVICE_UUID);
    BLELibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                    NUMBER_OF_SAMPLES_UUID,
                                    Permission::READ_PERMISSION);
    BLELibrary.characteristicSetValue(NUMBER_OF_SAMPLES_UUID, 0);
    BLELibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                    REQUESTED_SAMPLES_UUID,
                                    Permission::WRITE_PERMISSION);
    BLELibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                    SAMPLE_HISTORY_INTERVAL_UUID,
                                    Permission::READWRITE_PERMISSION);
    BLELibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID, DOWNLOAD_PACKET_UUID,
                                    Permission::NOTIFY_PERMISSION);
    BLELibrary.startService(DOWNLOAD_SERVICE_UUID);

    // Settings Service
    BLELibrary.createService(SETTINGS_SERVICE_UUID);
    if (mEnableWifiSettings) {
        BLELibrary.createCharacteristic(SETTINGS_SERVICE_UUID, WIFI_SSID_UUID,
                                        Permission::READWRITE_PERMISSION);
        const char* ssid = "ssid";
        BLELibrary.characteristicSetValue(
            WIFI_SSID_UUID, reinterpret_cast<const uint8_t*>(ssid),
            strlen(ssid));
        BLELibrary.createCharacteristic(SETTINGS_SERVICE_UUID, WIFI_PWD_UUID,
                                        Permission::WRITE_PERMISSION);
        const char* pwd = "password";
        BLELibrary.characteristicSetValue(
            WIFI_PWD_UUID, reinterpret_cast<const uint8_t*>(pwd), strlen(pwd));
    }
    if (mEnableAltDeviceName) {
        BLELibrary.createCharacteristic(SETTINGS_SERVICE_UUID,
                                        ALT_DEVICE_NAME_UUID,
                                        Permission::READWRITE_PERMISSION);
        setAltDeviceName(mAltDeviceName);
    }
    BLELibrary.startService(SETTINGS_SERVICE_UUID);

    // Battery Service
    if (mEnableBatteryService) {
        BLELibrary.createService(BATTERY_SERVICE_UUID);
        BLELibrary.createCharacteristic(BATTERY_SERVICE_UUID,
                                        BATTERY_LEVEL_UUID,
                                        Permission::READ_PERMISSION);
        BLELibrary.characteristicSetValue(BATTERY_LEVEL_UUID, 0);
        BLELibrary.startService(BATTERY_SERVICE_UUID);
    }
    // SCD FRC Service
    if (mEnableFRCService) {
        BLELibrary.createService(SCD_SERVICE_UUID);
        BLELibrary.createCharacteristic(SCD_SERVICE_UUID, SCD_FRC_REQUEST_UUID,
                                        Permission::WRITE_PERMISSION);
        BLELibrary.characteristicSetValue(SCD_FRC_REQUEST_UUID, 0);
        BLELibrary.startService(SCD_SERVICE_UUID);
    }

    BLELibrary.setProviderCallbacks(this);
    BLELibrary.characteristicSetValue(SAMPLE_HISTORY_INTERVAL_UUID,
                                      mHistoryIntervalMilliSeconds);
    BLELibrary.characteristicSetValue(
        NUMBER_OF_SAMPLES_UUID, mSampleHistory.numberOfSamplesInHistory());
}

void DataProvider::onHistoryIntervalChange(uint32_t interval) {
    mHistoryIntervalMilliSeconds = static_cast<uint64_t>(interval);
    mSampleHistory.reset();
    BLELibrary.characteristicSetValue(
        NUMBER_OF_SAMPLES_UUID, mSampleHistory.numberOfSamplesInHistory());
}

void DataProvider::onConnectionEvent() {
    mDownloadSequenceIdx = 0;
    mDownloadState = INACTIVE;
}

void DataProvider::onDownloadRequest() {
    mDownloadState = START;
}

void DataProvider::onFRCRequest(uint16_t reference_co2_level) {
    mFrcRequested = true;
    mReferenceCo2Level = reference_co2_level;
}

void DataProvider::onNrOfSamplesRequest(uint32_t nr_of_samples) {
    mNrOfSamplesRequested = nr_of_samples;
}

__attribute__((unused)) void DataProvider::completeFRCRequest() {
    mFrcRequested = false;
    mReferenceCo2Level = 0;
}

__attribute__((unused)) bool DataProvider::isFRCRequested() const {
    return mFrcRequested;
}

__attribute__((unused)) uint32_t DataProvider::getReferenceCO2Level() const {
    return mReferenceCo2Level;
}

void DataProvider::onWifiSsidChange(std::string ssid) {
    mWiFiSsid = ssid;
}

void DataProvider::onWifiPasswordChange(std::string pwd) {
    for (const auto& callback : mWiFiChangedCallbacks) {
        std::string ssid = mWiFiSsid;
        callback(ssid, pwd);
    }
}

__attribute__((unused)) void DataProvider::registerWifiChangedCallback(
    const std::function<void(std::string, std::string)>& callback) {
    mWiFiChangedCallbacks.push_back(callback);
}

__attribute__((unused)) void DataProvider::registerDeviceNameChangeCallback(
    const std::function<void(std::string)>& callback) {
    mDeviceNameChangeCallbacks.push_back(callback);
}
