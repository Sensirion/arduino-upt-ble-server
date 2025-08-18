#include "BleAdvertisement.h"

void BleAdvertisement::begin() {
  // Fill advertisement header
  mAdvertisementHeader.writeCompanyId(0x06D5);
  mAdvertisementHeader.writeSensirionAdvertisementType(0x00);

  // Use part of MAC address as device id
  const std::string macAddress = mAdvertisementLibrary.getDeviceAddress();
  mAdvertisementHeader.writeDeviceId(
      static_cast<uint8_t>(
          strtol(macAddress.substr(12, 14).c_str(), nullptr, 16)),
      static_cast<uint8_t>(
          strtol(macAddress.substr(15, 17).c_str(), nullptr, 16)));

  Sample initialSample;
  mAdvertisementLibrary.setAdvertisingData(
      buildAdvertisementData(initialSample));
  mAdvertisementLibrary.startAdvertising();
}

void BleAdvertisement::setSampleConfig(const SampleConfig &sampleConfig) {
  mSampleConfig = sampleConfig;
}

void BleAdvertisement::commitSample(Sample &sample) {
  // Update Advertising
  mAdvertisementLibrary.stopAdvertising();
  mAdvertisementLibrary.setAdvertisingData(buildAdvertisementData(sample));
  mAdvertisementLibrary.startAdvertising();
}

std::string BleAdvertisement::buildAdvertisementData(Sample &sample) {
  mAdvertisementHeader.writeSampleType(mSampleConfig.sampleType);
  std::string data = mAdvertisementHeader.getDataString();
  data.append(sample.getDataString());
  return data;
}