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
#ifndef I_BLE_LIBRARY_WRAPPER_H
#define I_BLE_LIBRARY_WRAPPER_H

#include "IBleAdvertisementLibrary.h"
#include "IBleServiceLibrary.h"
#include "IProviderCallbacks.h"

const auto GADGET_NAME = "S";

// when adding a new characteristic, make sure to
// write numbers in lower case and to increase
// the constants MAX_NUMBER_OF_xxx

static constexpr auto DOWNLOAD_SERVICE_UUID =
    "00008000-b38d-4985-720e-0f993a68ee41";
static constexpr auto SAMPLE_HISTORY_INTERVAL_UUID =
    "00008001-b38d-4985-720e-0f993a68ee41";
static constexpr auto NUMBER_OF_SAMPLES_UUID =
    "00008002-b38d-4985-720e-0f993a68ee41";
static constexpr auto REQUESTED_SAMPLES_UUID =
    "00008003-b38d-4985-720e-0f993a68ee41";
static constexpr auto DOWNLOAD_PACKET_UUID =
    "00008004-b38d-4985-720e-0f993a68ee41";

static constexpr auto SETTINGS_SERVICE_UUID =
    "00008100-b38d-4985-720e-0f993a68ee41";
static constexpr auto WIFI_SSID_UUID = "00008171-b38d-4985-720e-0f993a68ee41";
static constexpr auto WIFI_PWD_UUID = "00008172-b38d-4985-720e-0f993a68ee41";
static constexpr auto ALT_DEVICE_NAME_UUID =
    "00008120-b38d-4985-720e-0f993a68ee41";
static constexpr auto BATTERY_SERVICE_UUID =
    "0000180f-0000-1000-8000-00805f9b34fb";
static constexpr auto BATTERY_LEVEL_UUID =
    "00002a19-0000-1000-8000-00805f9b34fb";

static constexpr auto SCD_SERVICE_UUID = "00007000-b38d-4985-720e-0f993a68ee41";
static constexpr auto SCD_FRC_REQUEST_UUID =
    "00007004-b38d-4985-720e-0f993a68ee41";

static constexpr unsigned int MAX_NUMBER_OF_SERVICES = 4;
static constexpr unsigned int MAX_NUMBER_OF_CHARACTERISTICS = 12;

// abstract class
class IBleLibraryWrapper : public IBleServiceLibrary,
                           public IBleAdvertisementLibrary {
public:
  virtual void init() = 0;

  virtual void createServer() = 0;

  virtual std::string getDeviceAddress() = 0;

  virtual void setProviderCallbacks(IProviderCallbacks *providerCallbacks) = 0;
};

#endif /* I_BLE_LIBRARY_WRAPPER_H */