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
#ifndef I_BLE_SERVICE_PROVIDER_H
#define I_BLE_SERVICE_PROVIDER_H
#include "IBleServiceLibrary.h"

namespace sensirion::upt::ble_server {

class IBleServiceProvider {
  /*
   * Responsibilities of a Ble Service Provider
   *
   * - Setup BLE services and characteristics.
   * - Register callbacks on the BLE implementation.
   * - Provide convenience functions for the user.
   * - Handle internal states.
   */

public:
  explicit IBleServiceProvider(IBleServiceLibrary &bleLibrary)
      : mBleLibrary(bleLibrary){};

  virtual ~IBleServiceProvider() = default;

  // Don't allow copy of services
  IBleServiceProvider &operator=(const IBleServiceProvider &&) = delete;

  /*
   * Create BLE services and characteristics.
   * Set initial values on characteristics.
   * Start BLE services
   * Register callbacks
   */
  virtual bool begin() = 0;

  virtual void onConnect(){};

  virtual void onDisconnect(){};

  virtual void onSubscribe(const std::string &uuid, uint16_t subValue){};

protected:
  IBleServiceLibrary &mBleLibrary;
};

} // namespace sensirion::upt::ble_server

#endif // I_BLE_SERVICE_PROVIDER_H
