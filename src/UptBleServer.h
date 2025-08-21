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
#ifndef UPT_BLE_SERVER_H
#define UPT_BLE_SERVER_H

#include "BleAdvertisement.h"
#include "DownloadBleService.h"
#include "IBleLibraryWrapper.h"
#include "IBleServiceProvider.h"
#include "IProviderCallbacks.h"
#include "Sensirion_UPT_Core.h"

#include <string>

namespace sensirion::upt::ble_server {

class UptBleServer final : public IProviderCallbacks {
public:
  explicit UptBleServer(IBleLibraryWrapper &libraryWrapper,
                        const core::DataType dataType = core::T_RH_V3)
      : mBleLibrary{libraryWrapper},
        mSampleConfig{core::sampleConfigSelector.at(dataType)},
        mDownloadBleService{mBleLibrary, mSampleConfig},
        mBleAdvertisement{mBleLibrary, mSampleConfig} {};

  void begin();

  [[nodiscard]] String getDeviceIdString() const;

  void setSampleConfig(core::DataType dataType);
  void writeValueToCurrentSample(float value, core::SignalType signalType);
  void commitSample();
  void handleDownload();

  void registerBleServiceProvider(IBleServiceProvider &serviceProvider);

private:
  IBleLibraryWrapper &mBleLibrary;

  core::SampleConfig mSampleConfig;
  Sample mCurrentSample;

  DownloadBleService mDownloadBleService;
  BleAdvertisement mBleAdvertisement;
  std::vector<IBleServiceProvider *> mBleServiceProviders;

private:
  void setupBLEInfrastructure();

private:
  // ProviderCallbacks
  void onConnect() override;

  void onDisconnect() override;

  void onSubscribe(const std::string &uuid, uint16_t subValue) override;
};

} // namespace sensirion::upt::ble_server

#endif /* UPT_BLE_SERVER_H */