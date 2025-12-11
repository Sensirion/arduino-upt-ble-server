#include "UptBleServer.h"

#include "DownloadBleService.h"

#include <cmath>

namespace sensirion::upt::ble_server {

void UptBleServer::begin() {
  setupBLEInfrastructure();

  // Setup download service
  mDownloadBleService.begin();

  // setup additional services
  for (IBleServiceProvider *provider : mBleServiceProviders) {
    provider->begin();
  }

  // Setup advertisement
  mBleAdvertisement.begin();
}

void UptBleServer::writeValueToCurrentSample(
    const float value, const core::SignalType signalType) {
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

void UptBleServer::commitSample() {
  mBleAdvertisement.commitSample(mCurrentSample);
  mDownloadBleService.commitSample(mCurrentSample);
}

void UptBleServer::handleDownload() { mDownloadBleService.handleDownload(); }

bool UptBleServer::hasConnectedDevices() const {
  return mBleLibrary.hasConnectedDevices();
}
void UptBleServer::setDefaultConnectionTimeout(const uint16_t timeoutMs) const {
  mBleLibrary.setDefaultConnectionTimeout(timeoutMs);
}

void UptBleServer::registerBleServiceProvider(
    IBleServiceProvider &serviceProvider) {
  mBleServiceProviders.push_back(&serviceProvider);
}

void UptBleServer::setSampleConfig(const core::DataType dataType) {
  mSampleConfig = core::GetSampleConfiguration(dataType);
  mBleAdvertisement.setSampleConfig(mSampleConfig);
  mDownloadBleService.setSampleConfig(mSampleConfig);
}

String UptBleServer::getDeviceIdString() const {
  char cDevId[6];
  const std::string macAddress = mBleLibrary.getDeviceAddress();
  snprintf(cDevId, sizeof(cDevId), "%s:%s", macAddress.substr(12, 14).c_str(),
           macAddress.substr(15, 17).c_str());
  return cDevId;
}

void UptBleServer::setupBLEInfrastructure() {
  mBleLibrary.init();
  mBleLibrary.createServer();

  mBleLibrary.setProviderCallbacks(this);
}

void UptBleServer::onConnect() {
  mDownloadBleService.onConnect();

  for (IBleServiceProvider *provider : mBleServiceProviders) {
    provider->onConnect();
  }
}

void UptBleServer::onDisconnect() {
  mDownloadBleService.onDisconnect();

  for (IBleServiceProvider *provider : mBleServiceProviders) {
    provider->onDisconnect();
  }
}

void UptBleServer::onSubscribe(const std::string &uuid,
                               const uint16_t subValue) {
  mDownloadBleService.onSubscribe(uuid, subValue);

  for (IBleServiceProvider *provider : mBleServiceProviders) {
    provider->onSubscribe(uuid, subValue);
  }
}

} // namespace sensirion::upt::ble_server