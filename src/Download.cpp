#include "Download.h"

// DownloadHeader
void DownloadHeader::setDownloadSampleType(const uint16_t type) {
  write16BitLittleEndian(type, 4);
}
void DownloadHeader::setIntervalMilliSeconds(const uint32_t interval) {
  write32BitLittleEndian(interval, 6);
}
void DownloadHeader::setAgeOfLatestSampleMilliSeconds(const uint32_t age) {
  write32BitLittleEndian(age, 10);
}
void DownloadHeader::setDownloadSampleCount(const uint16_t count) {
  write16BitLittleEndian(count, 14);
}

// DownloadPacket
void DownloadPacket::setDownloadSequenceNumber(const uint16_t number) {
  write16BitLittleEndian(number, 0);
}

void DownloadPacket::writeSample(const Sample &sample, const size_t sampleSize,
                                 const size_t position) {
  for (int i = 0; i < sampleSize; ++i) {
    writeSampleByte(sample.getByte(i), position * sampleSize + i);
  }
}

void DownloadPacket::writeSampleByte(const uint8_t byte,
                                     const size_t positionInSampleData) {
  // shift to after downloadSequenceNumber
  writeByte(byte, 2 + positionInSampleData);
}