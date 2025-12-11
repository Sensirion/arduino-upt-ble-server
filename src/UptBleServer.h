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

/**
 * @brief High-level BLE server for Sensirion UPT gadgets.
 *
 * This class wires together the BLE library wrapper, advertisement handling
 * and built-in services (e.g. download service). It exposes a compact API to
 * initialize BLE, feed samples, and register additional BLE service providers.
 */
class UptBleServer final : public IProviderCallbacks {
public:
  /**
   * @brief Construct a new UptBleServer.
   *
   * @param libraryWrapper Reference to the BLE library abstraction used by
   *        the server. The reference must remain valid for the lifetime of
   *        the server.
   * @param dataType Initial sample data type configuration (default
   *        `core::T_RH_V3`).
   */
  explicit UptBleServer(IBleLibraryWrapper &libraryWrapper,
                        const core::DataType dataType = core::T_RH_V3)
      : mBleLibrary{libraryWrapper},
        mSampleConfig{core::GetSampleConfiguration(dataType)},
        mDownloadBleService{mBleLibrary, mSampleConfig},
        mBleAdvertisement{mBleLibrary, mSampleConfig} {};

  // Don't allow copy of UptBleServer
  UptBleServer& operator=(const UptBleServer&&) = delete;

  /**
   * @brief Initialize the BLE stack, services and advertising.
   *
   * Creates the BLE server, initializes the download service and any
   * registered providers, and starts the advertisement setup.
   */
  void begin();

  /**
   * @brief Get a short device identifier derived from the BLE MAC address.
   *
   * @return Short device ID string (e.g. last bytes of the MAC) suitable for
   *         displaying in UIs.
   */
  [[nodiscard]] String getDeviceIdString() const;

  /**
   * @brief Check whether any central device is currently connected.
   *
   * @return true if at least one connection is active, false otherwise.
   */
  [[nodiscard]] bool hasConnectedDevices() const;

  /**
   * @brief Set the sample configuration by data type.
   *
   * Updates internal encoding/offsets for samples and propagates the
   * configuration to advertisement and download services.
   *
   * @param dataType Desired sample data type configuration.
   */
  void setSampleConfig(core::DataType dataType);

  /**
   * @brief Write a single signal value into the current sample buffer.
   *
   * Invalid numbers (NaN) are ignored. If the provided signal type is not part
   * of the active sample configuration, the call is ignored.
   *
   * @param value Floating point value to encode and write.
   * @param signalType Signal type designating the target slot.
   */
  void writeValueToCurrentSample(float value, core::SignalType signalType);

  /**
   * @brief Finalize and publish the current sample.
   *
   * Commits the buffered sample to advertisement and download services and
   * prepares the buffer for the next sample.
   */
  void commitSample();

  /**
   * @brief Handle pending download requests.
   *
   * Call this periodically from the main loop to service download operations.
   */
  void handleDownload();

  /**
   * @brief Register an additional BLE service provider.
   *
   * The provider's lifetime must outlive the server. Registration allows the
   * server to initialize the provider and forward connection/subscription
   * events.
   *
   * @param serviceProvider The provider to register.
   */
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