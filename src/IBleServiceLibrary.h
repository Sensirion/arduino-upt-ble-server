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
#ifndef I_BLE_SERVICE_LIBRARY_H
#define I_BLE_SERVICE_LIBRARY_H
#include <functional>
#include <string>

namespace sensirion::upt::ble_server {

/**
 * @brief Bitmask of GATT characteristic permissions.
 */
enum class Permission : uint8_t {
  READ_PERMISSION = 1 << 0,
  WRITE_PERMISSION = 1 << 1,
  NOTIFY_PERMISSION = 1 << 2
};

inline Permission operator|(Permission a, Permission b) {
  return static_cast<Permission>(static_cast<uint8_t>(a) |
                                 static_cast<uint8_t>(b));
}

inline Permission &operator|=(Permission &a, const Permission b) {
  a = a | b;
  return a;
}

inline bool operator==(Permission a, Permission b) {
  return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * @brief Callback type invoked when the value of a characteristic changes.
 *
 * The string carries the raw value as provided by the underlying BLE stack.
 */
using ble_service_callback_t = std::function<void(std::string)>;

class IBleServiceLibrary {
public:
  virtual ~IBleServiceLibrary() = default;

  /**
   * @brief Create a GATT service.
   * @param uuid 128-bit UUID string (or 16-bit/32-bit if supported by stack).
   * @return true on success, false otherwise.
   */
  virtual bool createService(const char *uuid) = 0;

  /**
   * @brief Start a previously created GATT service.
   * @param uuid Service UUID used during creation.
   * @return true on success, false otherwise.
   */
  virtual bool startService(const char *uuid) = 0;

  /**
   * @brief Create a GATT characteristic under a service with permissions.
   * @param serviceUuid Parent service UUID.
   * @param characteristicUuid Characteristic UUID.
   * @param permission Bitwise OR of `Permission` flags.
   * @return true on success, false otherwise.
   */
  virtual bool createCharacteristic(const char *serviceUuid,
                                    const char *characteristicUuid,
                                    Permission permission) = 0;

  /**
   * @brief Set a characteristic value from a byte buffer.
   * @param uuid Characteristic UUID.
   * @param data Pointer to data buffer.
   * @param size Number of bytes in buffer.
   * @return true on success, false otherwise.
   */
  virtual bool characteristicSetValue(const char *uuid, const uint8_t *data,
                                      size_t size) = 0;

  /**
   * @brief Set a characteristic value from an integer.
   */
  virtual bool characteristicSetValue(const char *uuid, int value) = 0;

  /**
   * @brief Set a characteristic value from an unsigned 32-bit integer.
   */
  virtual bool characteristicSetValue(const char *uuid, uint32_t value) = 0;

  /**
   * @brief Set a characteristic value from an unsigned 64-bit integer.
   */
  virtual bool characteristicSetValue(const char *uuid, uint64_t value) = 0;

  /**
   * @brief Read the characteristic value as a string.
   * @param uuid Characteristic UUID.
   * @return The value as string; empty if unavailable.
   */
  virtual std::string characteristicGetValue(const char *uuid) = 0;

  /**
   * @brief Send a GATT notification for a characteristic.
   * @param uuid Characteristic UUID.
   * @return true if notification was queued/sent.
   */
  virtual bool characteristicNotify(const char *uuid) = 0;

  /**
   * @brief Register a callback invoked on characteristic writes/updates.
   * @param uuid Characteristic UUID.
   * @param callback Function to call with the new value.
   */
  virtual void
  registerCharacteristicCallback(const char *uuid,
                                 const ble_service_callback_t &callback) = 0;

  /**
   * @brief Check whether any central devices are connected.
   * @return true if at least one device is connected.
   */
  virtual bool hasConnectedDevices() = 0;

  /**
   * @brief Configure the default connection timeout for newly connected
   *        devices.
   *
   * @param timeoutMs the timeout in milliseconds
   */
  virtual void setDefaultConnectionTimeout(uint16_t timeoutMs) = 0;
};

} // namespace sensirion::upt::ble_server

#endif // I_BLE_SERVICE_LIBRARY_H
