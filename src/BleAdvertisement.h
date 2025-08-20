#ifndef ARDUINO_UPT_BLE_SERVER_BLE_ADVERTISEMENT_H
#define ARDUINO_UPT_BLE_SERVER_BLE_ADVERTISEMENT_H
#include "AdvertisementHeader.h"
#include "BLEProtocol.h"
#include "IBleAdvertisementLibrary.h"
#include "Sample.h"

namespace sensirion::upt::ble_server {

class BleAdvertisement {
public:
  explicit BleAdvertisement(IBleAdvertisementLibrary &bleLibrary,
                            const SampleConfig &sampleConfig)
      : mSampleConfig(sampleConfig), mAdvertisementLibrary(bleLibrary){};

  void begin();

  void setSampleConfig(const SampleConfig &sampleConfig);
  void commitSample(Sample &sample);

private:
  SampleConfig mSampleConfig;
  IBleAdvertisementLibrary &mAdvertisementLibrary;
  AdvertisementHeader mAdvertisementHeader;

private:
  std::string buildAdvertisementData(Sample &sample);
};

} // namespace sensirion::upt::ble_server

#endif // ARDUINO_UPT_BLE_SERVER_BLE_ADVERTISEMENT_H
