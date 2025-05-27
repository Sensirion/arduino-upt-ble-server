#include "SampleHistoryRingBuffer.h"
#include <cmath>

void SampleHistoryRingBuffer::putSample(const Sample &sample) {
  // iterate outSampleIndex if overwriting
  if (isFull()) {
    mTail = nextIndex(mTail);
  }
  // copy byte wise
  writeSample(sample);

  // iterate mHead
  mHead = nextIndex(mHead);
}

void SampleHistoryRingBuffer::setSampleSize(size_t sampleSize) {
  mSampleSizeBytes = sampleSize;
  reset();
}

uint32_t SampleHistoryRingBuffer::numberOfSamplesInHistory() const {
  if (mHead >= mTail) {
    return mHead - mTail;
  } else {
    return sizeInSamples() - (mTail - mHead);
  }
}

bool SampleHistoryRingBuffer::isFull() const {
  return nextIndex(mHead) == mTail;
}

void SampleHistoryRingBuffer::startReadOut(uint32_t nrOfSamples) {
  if (nrOfSamples >= numberOfSamplesInHistory()) {
    mSampleReadOutIndex = mTail;
  } else {
    mSampleReadOutIndex = mHead - nrOfSamples;
    if (mSampleReadOutIndex < 0) {
      mSampleReadOutIndex = mSampleReadOutIndex + sizeInSamples();
    }
  }
}

// May give out invalid sample if called on an empty sample history
Sample SampleHistoryRingBuffer::readOutNextSample(bool &allSamplesRead) {
  Sample sample = readSample(mSampleReadOutIndex);
  if (!allSamplesRead) {
    mSampleReadOutIndex = nextIndex(mSampleReadOutIndex);
  }
  allSamplesRead = (mSampleReadOutIndex == mHead);
  return sample;
}

void SampleHistoryRingBuffer::reset() {
  mHead = 0;
  mTail = 0;
  mSampleReadOutIndex = 0;
}

uint32_t SampleHistoryRingBuffer::nextIndex(uint32_t index) const {
  return (index + 1) % sizeInSamples();
}

size_t SampleHistoryRingBuffer::sizeInSamples() const {
  if (mSampleSizeBytes == 0) {
    return 0;
  } else {
    return (SAMPLE_HISTORY_RING_BUFFER_SIZE_BYTES / mSampleSizeBytes);
  }
}

void SampleHistoryRingBuffer::writeSample(const Sample &sample) {
  for (int byteIndex = 0; byteIndex < mSampleSizeBytes; ++byteIndex) {
    _data[mHead * mSampleSizeBytes + byteIndex] = sample.getByte(byteIndex);
  }
}

Sample SampleHistoryRingBuffer::readSample(uint32_t sampleIndex) const {
  Sample sample;
  for (size_t i = 0; i < mSampleSizeBytes; ++i) {
    uint8_t byte = getByte((sampleIndex * mSampleSizeBytes) + i);
    sample.setByte(byte, i);
  }
  return sample;
}
