#include "DataProvider.h"

#include "DownloadBleService.h"

#include <cmath>

void DataProvider::begin() {
  setupBLEInfrastructure();

  // Setup download service
  mDownloadBleService.begin();
  mBleAdvertisement.begin();
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
  mBleAdvertisement.commitSample(mCurrentSample);
  mDownloadBleService.commitSample(mCurrentSample);
}

void DataProvider::handleDownload() { mDownloadBleService.handleDownload(); }

void DataProvider::setSampleConfig(const DataType dataType) {
  mSampleConfig = sampleConfigSelector.at(dataType);
  mBleAdvertisement.setSampleConfig(mSampleConfig);
  mDownloadBleService.setSampleConfig(mSampleConfig);
}

String DataProvider::getDeviceIdString() const {
  char cDevId[6];
  const std::string macAddress = mBleLibrary.getDeviceAddress();
  snprintf(cDevId, sizeof(cDevId), "%s:%s", macAddress.substr(12, 14).c_str(),
           macAddress.substr(15, 17).c_str());
  return cDevId;
}

void DataProvider::setupBLEInfrastructure() {
  mBleLibrary.init();
  mBleLibrary.createServer();

  mBleLibrary.setProviderCallbacks(this);
}

void DataProvider::onConnect() { mDownloadBleService.onConnect(); }

void DataProvider::onDisconnect() { mDownloadBleService.onDisconnect(); }

void DataProvider::onSubscribe(const std::string &uuid,
                               const uint16_t subValue) {
  mDownloadBleService.onSubscribe(uuid, subValue);
}
