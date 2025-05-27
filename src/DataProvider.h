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
#ifndef DATA_PROVIDER_H
#define DATA_PROVIDER_H

#include "AdvertisementHeader.h"
#include "Download.h"
#include "IBLELibraryWrapper.h"
#include "IProviderCallbacks.h"
#include "SampleHistoryRingBuffer.h"
#include "Sensirion_UPT_Core.h"
#include <string>
#include <vector>

enum class BleServerFeatures : uint8_t {
    None = 0,
    EnableWiFiSetting = 1 << 0,
    EnableBatteryService = 1 << 1,
    EnableFrcService = 1 << 2,
    EnableAltDeviceNameSetting = 1 << 3
};

inline BleServerFeatures operator|(BleServerFeatures a, BleServerFeatures b) {
    return static_cast<BleServerFeatures>(static_cast<uint8_t>(a) |
                                          static_cast<uint8_t>(b));
}

inline void operator|=(BleServerFeatures& a, const BleServerFeatures b) {
    a = a | b;
}

inline bool operator==(BleServerFeatures a, BleServerFeatures b) {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

class __attribute__((unused)) DataProvider final : public IProviderCallbacks {
  public:
    __attribute__((unused)) explicit DataProvider(
        IBLELibraryWrapper& libraryWrapper, DataType dataType = T_RH_V3,
        BleServerFeatures features = BleServerFeatures::None)
        : BLELibrary(libraryWrapper),
          mSampleConfig(sampleConfigSelector.at(dataType)) {
        if (features == BleServerFeatures::EnableWiFiSetting) {
            mEnableWifiSettings = true;
        }
        if (features == BleServerFeatures::EnableBatteryService) {
            mEnableBatteryService = true;
        }
        if (features == BleServerFeatures::EnableFrcService) {
            mEnableFRCService = true;
        }
        if (features == BleServerFeatures::EnableAltDeviceNameSetting) {
            mEnableAltDeviceName = true;
        }
    };

    void begin();

    __attribute__((unused)) String getDeviceIdString() const;

    __attribute__((unused)) void setSampleConfig(DataType dataType);

    __attribute__((unused)) void
    writeValueToCurrentSample(float value, SignalType signalType);

    __attribute__((unused)) void commitSample();

    __attribute__((unused)) void setBatteryLevel(int value) const;

    __attribute__((unused)) void handleDownload();

    __attribute__((unused)) bool isDownloading() const;

    __attribute__((unused)) bool isFRCRequested() const;

    __attribute__((unused)) void completeFRCRequest();

    __attribute__((unused)) uint32_t getReferenceCO2Level() const;

    /*
     * enableAltDeviceName must be called before begin() to ensure
     * the characteristic is created.
     * Initially, the alternative device name is empty.
     * Use setAltDeviceName to change the device name.
     */
    __attribute__((unused)) void enableAltDeviceName();

    __attribute__((unused)) std::string getAltDeviceName();

    void setAltDeviceName(std::string altDeviceName);

    __attribute__((unused)) void registerWifiChangedCallback(
        const std::function<void(std::string, std::string)>& callback);

    __attribute__((unused)) void registerDeviceNameChangeCallback(
        const std::function<void(std::string)>& callback);

  private:
    IBLELibraryWrapper& BLELibrary;

    Sample mCurrentSample;
    AdvertisementHeader mAdvertisementHeader;
    SampleHistoryRingBuffer mSampleHistory;
    uint32_t mNrOfSamplesRequested = 0;
    DownloadState mDownloadState = INACTIVE;
    uint16_t mDownloadSequenceIdx = 0;  // the first packet is the header
    uint32_t mNumberOfSamplesToDownload = 0;
    uint32_t mNumberOfSamplePacketsToDownload = 0;
    bool mFrcRequested = false;
    uint32_t mReferenceCo2Level = 0;
    std::string mAltDeviceName;

    bool mEnableWifiSettings = false;
    bool mEnableBatteryService = false;
    bool mEnableFRCService = false;
    bool mEnableAltDeviceName = false;

    SampleConfig mSampleConfig;
    uint64_t mHistoryIntervalMilliSeconds = 600000;  // = 10 minutes
    uint64_t mLatestHistoryTimeStamp = 0;
    uint64_t mLatestHistoryTimeStampAtDownloadStart = 0;

    std::string mWiFiSsid;
    std::vector<std::function<void(std::string, std::string)>>
        mWiFiChangedCallbacks;
    std::vector<std::function<void(std::string)>> mDeviceNameChangeCallbacks;

  private:
    std::string buildAdvertisementData();

    DownloadHeader buildDownloadHeader() const;

    DownloadPacket buildDownloadPacket();

    uint32_t numberOfPacketsRequired(uint32_t numberOfSamples) const;

    void setupBLEInfrastructure();

  private:
    // ProviderCallbacks
    void onHistoryIntervalChange(uint32_t interval) override;

    void onConnectionEvent() override;

    void onDownloadRequest() override;

    void onWifiSsidChange(std::string ssid) override;

    void onWifiPasswordChange(std::string pwd) override;

    void onFRCRequest(uint16_t reference_co2_level) override;

    void onNrOfSamplesRequest(uint32_t nr_of_samples) override;

    void onAltDeviceNameChange(std::string altDeviceName) override;
};

#endif /* DATA_PROVIDER_H */