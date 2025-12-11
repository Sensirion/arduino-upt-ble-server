/*
 * Copyright (c) 2025, Sensirion AG
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

#ifndef I_BLE_ADVERTISEMENT_LIBRARY_H
#define I_BLE_ADVERTISEMENT_LIBRARY_H
#include <string>

namespace sensirion::upt::ble_server {

/**
 * @brief Interface for configuring BLE advertising and connection parameters.
 */
class IBleAdvertisementLibrary {
public:
  virtual ~IBleAdvertisementLibrary() = default;

  /**
   * @brief Set the advertisement payload (e.g. name and manufacturer data).
   * @param data Raw advertisement payload to be used by the implementation.
   */
  virtual void setAdvertisingData(const std::string &data) = 0;

  /**
   * @brief Start advertising.
   */
  virtual void startAdvertising() = 0;

  /**
   * @brief Stop advertising.
   */
  virtual void stopAdvertising() = 0;

  /**
   * @brief Get the BLE device address as a string.
   */
  virtual std::string getDeviceAddress() = 0;

  /**
   * @param minIntervalMs The minimal advertising interval in ms.
   * @param maxIntervalMs The maximal advertising interval in ms.
   * @return true if successful, else false
   * @note It is not guaranteed that the exact values can be configured.
   *       Please check the BLE library implementation for details of supported
   *       values.
   */
  virtual bool setAdvertisingInterval(float minIntervalMs, float maxIntervalMs);

  /**
   * @param minIntervalMs The minimal connection interval in ms.
   * @param maxIntervalMs The maximal connection interval in ms.
   * @return true if successful, else false
   * @note It is not guaranteed that the exact values can be configured.
   *       Please check the BLE library implementation for details of supported
   *       values.
   */
  virtual bool setPreferredConnectionInterval(float minIntervalMs,
                                              float maxIntervalMs);
};

} // namespace sensirion::upt::ble_server

#endif // I_BLE_ADVERTISEMENT_LIBRARY_H
