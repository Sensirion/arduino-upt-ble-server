/*
 * Copyright (c) 2022, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SAMPLE_HISTORY_RING_BUFFER
#define SAMPLE_HISTORY_RING_BUFFER

#include "ByteArray.h"
#include "Sample.h"

static constexpr size_t SAMPLE_HISTORY_RING_BUFFER_SIZE_BYTES = 30000;

// Logs Samples over time to be downloaded
template <size_t BUFFER_SIZE = SAMPLE_HISTORY_RING_BUFFER_SIZE_BYTES>
class SampleHistoryRingBuffer : protected ByteArray<BUFFER_SIZE> {
public:
  void putSample(const Sample &sample) {
    // iterate outSampleIndex if overwriting
    if (isFull()) {
      mTail = nextIndex(mTail);
    }
    // copy byte wise
    writeSample(sample);

    // iterate mHead
    mHead = nextIndex(mHead);
  };

  void setSampleSize(const size_t sampleSize) {
    mSampleSizeBytes = sampleSize;
    reset();
  };

  [[nodiscard]] uint32_t numberOfSamplesInHistory() const {
    if (mHead >= mTail) {
      return mHead - mTail;
    }
    return sizeInSamples() - (mTail - mHead);
  };

  [[nodiscard]] bool isFull() const { return nextIndex(mHead) == mTail; };

  void startReadOut(const uint32_t nrOfSamples) {
    // read out the whole sample buffer
    if (nrOfSamples >= numberOfSamplesInHistory()) {
      mSampleReadOutIndex = mTail;
      return;
    }

    int64_t nextReadOutIndex = static_cast<int64_t>(mHead) - nrOfSamples;
    if (nextReadOutIndex < 0) {
      nextReadOutIndex += sizeInSamples();
    }
    mSampleReadOutIndex = static_cast<uint32_t>(nextReadOutIndex);
  };

  // May give out an invalid sample if called on an empty sample history
  Sample readOutNextSample(bool &allSamplesRead) {
    const Sample sample = readSample(mSampleReadOutIndex);
    if (!allSamplesRead) {
      mSampleReadOutIndex = nextIndex(mSampleReadOutIndex);
    }
    allSamplesRead = (mSampleReadOutIndex == mHead);
    return sample;
  };

  void reset() {
    mHead = 0;
    mTail = 0;
    mSampleReadOutIndex = 0;
  };

private:
  [[nodiscard]] uint32_t nextIndex(const uint32_t index) const {
    return (index + 1) % sizeInSamples();
  };

  [[nodiscard]] size_t sizeInSamples() const {
    if (mSampleSizeBytes == 0) {
      return 0;
    }
    return (BUFFER_SIZE / mSampleSizeBytes);
  };

  void writeSample(const Sample &sample) {
    for (int byteIndex = 0; byteIndex < mSampleSizeBytes; ++byteIndex) {
      this->mData[mHead * mSampleSizeBytes + byteIndex] =
          sample.getByte(byteIndex);
    }
  };

  [[nodiscard]] Sample readSample(const uint32_t sampleIndex) const {
    Sample sample;
    for (size_t i = 0; i < mSampleSizeBytes; ++i) {
      const uint8_t byte = this->getByte((sampleIndex * mSampleSizeBytes) + i);
      sample.setByte(byte, i);
    }
    return sample;
  };

private:
  uint32_t mHead = 0;
  uint32_t mTail = 0;
  uint32_t mSampleReadOutIndex = 0;

  size_t mSampleSizeBytes = 0;
};

#endif /* SAMPLE_HISTORY_RING_BUFFER */