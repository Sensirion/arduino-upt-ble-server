#ifndef ARDUINO_UPT_BLE_SERVER_DOWNLOAD_BLE_SERVICE_H
#define ARDUINO_UPT_BLE_SERVER_DOWNLOAD_BLE_SERVICE_H
#include "Download.h"
#include "IBleServiceProvider.h"
#include "SampleHistoryRingBuffer.h"

#include <BLEProtocol.h>

namespace sensirion::upt::ble_server {

static constexpr auto DOWNLOAD_SERVICE_UUID =
    "00008000-b38d-4985-720e-0f993a68ee41";
static constexpr auto SAMPLE_HISTORY_INTERVAL_UUID =
    "00008001-b38d-4985-720e-0f993a68ee41";
static constexpr auto NUMBER_OF_SAMPLES_UUID =
    "00008002-b38d-4985-720e-0f993a68ee41";
static constexpr auto REQUESTED_SAMPLES_UUID =
    "00008003-b38d-4985-720e-0f993a68ee41";
static constexpr auto DOWNLOAD_PACKET_UUID =
    "00008004-b38d-4985-720e-0f993a68ee41";

#ifndef BLE_SERVER_HISTORY_BUFFER_SIZE
#define BLE_SERVER_HISTORY_BUFFER_SIZE SAMPLE_HISTORY_RING_BUFFER_SIZE_BYTES
#endif

class DownloadBleService final : IBleServiceProvider {
public:
  explicit DownloadBleService(IBleServiceLibrary &bleLibrary,
                              const core::SampleConfig &sampleConfig)
      : IBleServiceProvider(bleLibrary), mSampleConfig(sampleConfig){};

  bool begin() override;

  void commitSample(const Sample &sample);
  void handleDownload();
  bool isDownloading() const;
  void setSampleConfig(const core::SampleConfig &sampleConfig) {
    mSampleConfig = sampleConfig;
    mSampleHistory.setSampleSize(mSampleConfig.sampleSizeBytes);
  }

  void onConnect() override;
  void onDisconnect() override;
  void onSubscribe(const std::string &uuid, uint16_t subValue) override;

private:
  core::SampleConfig mSampleConfig;
  // ReSharper disable once CppRedundantTemplateArguments
  SampleHistoryRingBuffer<BLE_SERVER_HISTORY_BUFFER_SIZE> mSampleHistory;
  uint32_t mNrOfSamplesRequested = 0;
  DownloadState mDownloadState = INACTIVE;
  uint16_t mDownloadSequenceIdx = 0; // the first packet is the header
  uint32_t mNumberOfSamplesToDownload = 0;
  uint32_t mNumberOfSamplePacketsToDownload = 0;

  uint64_t mHistoryIntervalMilliSeconds = 600000; // = 10 minutes
  uint64_t mLatestHistoryTimeStamp = 0;
  uint64_t mLatestHistoryTimeStampAtDownloadStart = 0;

private:
  DownloadHeader buildDownloadHeader() const;

  DownloadPacket buildDownloadPacket();

  uint32_t numberOfPacketsRequired(uint32_t numberOfSamples) const;
};

} // namespace sensirion::upt::ble_server

#endif // ARDUINO_UPT_BLE_SERVER_DOWNLOAD_BLE_SERVICE_H
