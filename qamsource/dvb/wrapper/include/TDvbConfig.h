/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2015 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
* ============================================================================
* Contributed by ARRIS Group, Inc.
* ============================================================================
*/


#ifndef TDVBCONFIG_H
#define TDVBCONFIG_H

#include <string>
#include <stdint.h>

class TDvbConfig {
private:
  uint32_t HomeTsFreq; 
  uint32_t HomeTsMod;
  uint32_t HomeTsSymbRate;
  uint16_t PrefNetworkId;
  uint32_t BarkerFreq; 
  uint32_t BarkerMod;
  uint32_t BarkerSymbRate;
  uint32_t BarkerEitTimeout;
  uint32_t BackgrndScanInterval;

  typedef struct {
    std::string DatabaseFile;
    std::string NetworkConfigurationFile;
    std::string PreferredNetworkId;
    std::string BouquetId;
    std::string HomeTsFrequency;
    std::string HomeTsModulation;
    std::string HomeTsSymbolRate;
    std::string BarkerTsFrequency;
    std::string BarkerTsModulation;
    std::string BarkerTsSymbolRate;
    std::string BarkerEitTimeout;
    std::string FastScanSmart;
    std::string BackGroundScanInterval;
  } TDvbConfigurations;

  void ReadEnvironmentVariables();
  void PrintEnvironmentVariables();
public:
  TDvbConfigurations DvbConfigurations;
  TDvbConfig();
  ~TDvbConfig();
  TDvbConfigurations& GetDvbConfig();
  const std::string& GetDatabaseFilePath();
  const std::string& GetNetworkConfigFilePath();
  const uint32_t& GetHomeTsFrequency();
  const uint32_t& GetHomeTsModulation();
  const uint32_t& GetHomeTsSymolRate();
  const uint16_t& GetPreferredNetworkId();
  const uint32_t& GetBackgroundScanInterval();
  const uint32_t& GetBarkerSymbolRate();
  const uint32_t& GetBarkerModulation();
  const uint32_t& GetBarkerFrequencey();
  const uint32_t& GetBarkerEitTimeout();
  bool GetFastScanStatus();
};

#endif // TDVBCONFIG_H
