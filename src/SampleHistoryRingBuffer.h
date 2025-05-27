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

const static size_t SAMPLE_HISTORY_RING_BUFFER_SIZE_BYTES = 30000;

// Logs Samples over time to be downloaded
class SampleHistoryRingBuffer
    : protected ByteArray<SAMPLE_HISTORY_RING_BUFFER_SIZE_BYTES> {
public:
  void putSample(const Sample &sample);

  void setSampleSize(size_t sampleSize);

  uint32_t numberOfSamplesInHistory() const;

  bool isFull() const;

  void startReadOut(uint32_t nrOfSamples);

  Sample readOutNextSample(bool &allSamplesRead);

  void reset();

private:
  uint32_t nextIndex(uint32_t index) const;

  size_t sizeInSamples() const;

  void writeSample(const Sample &sample);

  Sample readSample(uint32_t sampleIndex) const;

private:
  uint32_t mHead = 0;
  uint32_t mTail = 0;
  uint32_t mSampleReadOutIndex = 0;

  size_t mSampleSizeBytes = 0;
};

#endif /* SAMPLE_HISTORY_RING_BUFFER */