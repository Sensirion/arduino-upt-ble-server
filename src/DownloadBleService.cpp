#include "DownloadBleService.h"

#include "BLEProtocol.h"

namespace sensirion::upt::ble_server {

bool DownloadBleService::begin() {
  // set sample size for history before creating services and characteristics
  mSampleHistory.setSampleSize(mSampleConfig.sampleSizeBytes);

  mBleLibrary.createService(DOWNLOAD_SERVICE_UUID);
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                   NUMBER_OF_SAMPLES_UUID,
                                   Permission::READ_PERMISSION);
  mBleLibrary.characteristicSetValue(NUMBER_OF_SAMPLES_UUID,
                                     mSampleHistory.numberOfSamplesInHistory());
  mBleLibrary.createCharacteristic(DOWNLOAD_SERVICE_UUID,
                                   REQUESTED_SAMPLES_UUID,
                                   Permission::WRITE_PERMISSION);
  mBleLibrary.createCharacteristic(
      DOWNLOAD_SERVICE_UUID, SAMPLE_HISTORY_INTERVAL_UUID,
      Permission::READ_PERMISSION | Permission::WRITE_PERMISSION);
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
  return true;
}

void DownloadBleService::commitSample(const Sample &sample) {
  const uint64_t currentTimeStamp = millis();
  if (currentTimeStamp - mLatestHistoryTimeStamp >=
      mHistoryIntervalMilliSeconds) {
    mSampleHistory.putSample(sample);
    mLatestHistoryTimeStamp = currentTimeStamp;

    if (mDownloadState == INACTIVE) {
      mLatestHistoryTimeStampAtDownloadStart = currentTimeStamp;
      mBleLibrary.characteristicSetValue(
          NUMBER_OF_SAMPLES_UUID, mSampleHistory.numberOfSamplesInHistory());
    }
  }
}

void DownloadBleService::handleDownload() {
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

bool DownloadBleService::isDownloading() const {
  return (mDownloadState != INACTIVE);
}

void DownloadBleService::onConnect() {
  mDownloadSequenceIdx = 0;
  mDownloadState = INACTIVE;
}

void DownloadBleService::onDisconnect() { mDownloadState = INACTIVE; }
void DownloadBleService::onSubscribe(const std::string &uuid,
                                     const uint16_t subValue) {
  if (strcmp(uuid.c_str(), DOWNLOAD_PACKET_UUID) == 0 && subValue == 1) {
    // start download
    mDownloadState = START;
  }
}

DownloadHeader DownloadBleService::buildDownloadHeader() const {
  DownloadHeader header;
  const uint32_t age =
      static_cast<uint32_t>(millis() - mLatestHistoryTimeStampAtDownloadStart);
  header.setDownloadSampleType(mSampleConfig.downloadType);
  header.setIntervalMilliSeconds(mHistoryIntervalMilliSeconds);
  header.setAgeOfLatestSampleMilliSeconds(age);
  header.setDownloadSampleCount(
      static_cast<uint16_t>(mNumberOfSamplesToDownload));
  return header;
}

DownloadPacket DownloadBleService::buildDownloadPacket() {
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

uint32_t DownloadBleService::numberOfPacketsRequired(
    const uint32_t numberOfSamples) const {
  uint32_t numberOfPacketsRequired =
      numberOfSamples / mSampleConfig.sampleCountPerPacket;

  if (numberOfSamples % mSampleConfig.sampleCountPerPacket != 0) {
    ++numberOfPacketsRequired;
  }
  return numberOfPacketsRequired;
}

} // namespace sensirion::upt::ble_server