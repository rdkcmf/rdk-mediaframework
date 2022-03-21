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

#include <sstream>

#include "TDvbConfig.h"

#include <rmf_osal_util.h>
#include <rdk_debug.h>


// private function implementations.

void TDvbConfig::ReadEnvironmentVariables()
{
  std::string value("");
  value = rmf_osal_envGet("FEATURE.DVB.DB_FILENAME");
  if (!value.empty()) {
    DvbConfigurations.DatabaseFile = value;
  }
  else {
    DvbConfigurations.DatabaseFile = "/opt/persistent/si/default.db";
  }
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.PREFERRED_NETWORK_ID");
  if (!value.empty()) {
    DvbConfigurations.PreferredNetworkId = value;
  }
  else {
    DvbConfigurations.PreferredNetworkId = "0";
  }
  value.clear();
#if 0
  value = rmf_osal_envGet("FEATURE.DVB.BOUQUET_ID_LIST");
  if (!value.empty()) {
    DvbConfigurations.BouquetId = value;
  }
  else {
    DvbConfigurations.BouquetId = "0";
  }
  value.clear();
#endif
  value = rmf_osal_envGet("FEATURE.DVB.HOME_TS_FREQUENCY");
  if (!value.empty()) {
    DvbConfigurations.HomeTsFrequency = value;
  }
  else {
    DvbConfigurations.HomeTsFrequency = "0";
  }
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.HOME_TS_MODULATION");
  if (!value.empty()) {
    DvbConfigurations.HomeTsModulation = value;
  }
  else {
    DvbConfigurations.HomeTsModulation = "0";
  }
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.HOME_TS_SYMBOL_RATE");
  if (!value.empty()) {
    DvbConfigurations.HomeTsSymbolRate = value;
  }
  else {
    DvbConfigurations.HomeTsSymbolRate = "0";
  }
#if 0
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.BARKER_TS_FREQUENCY");
  if (!value.empty()) {
    DvbConfigurations.BarkerTsFrequency = value;
  }
  else {
    DvbConfigurations.BarkerTsFrequency = "0";
  }
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.BARKER_TS_MODULATION");
  if (!value.empty()) {
    DvbConfigurations.BarkerTsModulation = value;
  }
  else {
    DvbConfigurations.BarkerTsModulation = "0";
  }
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.BARKER_TS_SYMBOL_RATE");
  if (!value.empty()) {
    DvbConfigurations.BarkerTsSymbolRate = value;
  }
  else {
    DvbConfigurations.BarkerTsSymbolRate = "0";
  }
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.BARKER_EIT_TIMEOUT");
  if (!value.empty()) {
    DvbConfigurations.BarkerEitTimeout = value;
  }
  else {
    DvbConfigurations.BarkerEitTimeout = "0";
  }
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.FAST_SCAN_SMART");
  if (!value.empty()) {
    DvbConfigurations.FastScanSmart = value;
  }
  else {
    DvbConfigurations.FastScanSmart = "FALSE";
  }
#endif
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.BACKGROUND_SCAN_INTERVAL");
  if (!value.empty()) {
    DvbConfigurations.BackGroundScanInterval = value;
  }
  else {
    DvbConfigurations.BackGroundScanInterval = "0";
  }
  value.clear();
  value = rmf_osal_envGet("FEATURE.DVB.NETWORK_PROFILE_FILENAME");
  if (!value.empty()) {
    DvbConfigurations.NetworkConfigurationFile = value;
  }
  else {
    DvbConfigurations.NetworkConfigurationFile = "/opt/persistent/si/dvbSiConfig.json";
  }
}

void TDvbConfig::PrintEnvironmentVariables()
{
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.DB_FILENAME = %s\n", DvbConfigurations.DatabaseFile.c_str());
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.PREFERRED_NETWORK_ID = %s\n", DvbConfigurations.PreferredNetworkId.c_str());
//   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.BOUQUET_ID_LIST = %s\n", DvbConfigurations.BouquetId.c_str());
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.HOME_TS_FREQUENCY = %s\n", DvbConfigurations.HomeTsFrequency.c_str());
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.HOME_TS_SYMBOL_RATE = %s\n", DvbConfigurations.HomeTsSymbolRate.c_str());
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.HOME_TS_MODULATION = %s\n", DvbConfigurations.HomeTsModulation.c_str());
//   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.BARKER_TS_FREQUENCY = %s\n", DvbConfigurations.BarkerTsFrequency.c_str());
  // RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.BARKER_TS_MODULATION = %s\n", DvbConfigurations.BarkerTsModulation.c_str());
//   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.BARKER_TS_SYMBOL_RATE = %s\n", DvbConfigurations.BarkerTsSymbolRate.c_str());
  // RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.BARKER_EIT_TIMEOUT = %s\n", DvbConfigurations.BarkerEitTimeout.c_str());
//   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.FAST_SCAN_SMART = %s\n", DvbConfigurations.FastScanSmart.c_str());
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.BACKGROUND_SCAN_INTERVAL = %s\n", DvbConfigurations.BackGroundScanInterval.c_str());
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBCONFIG", "FEATURE.DVB.NETWORK_PROFILE_FILENAME = %s\n", DvbConfigurations.NetworkConfigurationFile.c_str());
}

// public function implementations.
TDvbConfig::TDvbConfig()
  : HomeTsFreq(0),
    HomeTsMod(0),
    HomeTsSymbRate(0),  
    PrefNetworkId(0),
    BarkerFreq(0),
    BarkerMod(0),
    BarkerSymbRate(0),
    BarkerEitTimeout(0),
    BackgrndScanInterval(0)
{
  ReadEnvironmentVariables();
  PrintEnvironmentVariables();
}

TDvbConfig::~TDvbConfig()
{
  // Empty.
}

TDvbConfig::TDvbConfigurations& TDvbConfig::GetDvbConfig()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> \n", __FUNCTION__);
  return DvbConfigurations;
}

const std::string& TDvbConfig::GetDatabaseFilePath()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> DatabaseFile = %s\n", __FUNCTION__, (DvbConfigurations.DatabaseFile).c_str());
  return DvbConfigurations.DatabaseFile;
}

const std::string& TDvbConfig::GetNetworkConfigFilePath()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> DvbConfigurations.NetworkConfigurationFile = %s \n", __FUNCTION__,
    (DvbConfigurations.NetworkConfigurationFile).c_str());
  return DvbConfigurations.NetworkConfigurationFile;
}

const uint32_t& TDvbConfig::GetHomeTsFrequency()
{
  std::istringstream ss((DvbConfigurations.HomeTsFrequency).c_str());
  ss >> HomeTsFreq;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> HomeTsFreq = %d\n", __FUNCTION__, HomeTsFreq);
  return HomeTsFreq;
}

const uint32_t& TDvbConfig::GetHomeTsModulation()
{
  std::istringstream ss((DvbConfigurations.HomeTsModulation).c_str());
  ss >> HomeTsMod;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> HomeTsMod = %d\n", __FUNCTION__, HomeTsMod);
  return HomeTsMod;
}

const uint32_t& TDvbConfig::GetHomeTsSymolRate()
{
  std::istringstream ss((DvbConfigurations.HomeTsSymbolRate).c_str());
  ss >> HomeTsSymbRate;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s>  HomeTsSymbRate = %d \n", __FUNCTION__, HomeTsSymbRate);
  return HomeTsSymbRate;
}

const uint16_t& TDvbConfig::GetPreferredNetworkId()
{
  std::istringstream ss((DvbConfigurations.PreferredNetworkId).c_str());
  ss >> PrefNetworkId;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> PrefNetworkId = %d \n", __FUNCTION__, PrefNetworkId);
  return PrefNetworkId;
}

const uint32_t& TDvbConfig::GetBarkerFrequencey()
{
  std::istringstream ss(DvbConfigurations.BarkerTsFrequency);
  ss >> BarkerFreq;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> BarkerFreq = %d \n", __FUNCTION__, BarkerFreq);
  return BarkerFreq;
}

const uint32_t& TDvbConfig::GetBarkerModulation()
{
  std::istringstream ss(DvbConfigurations.BarkerTsModulation);
  ss >> BarkerMod;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> BarkerMod = %d \n", __FUNCTION__, BarkerMod);
  return BarkerMod;
}

const uint32_t& TDvbConfig::GetBarkerSymbolRate()
{
  std::istringstream ss(DvbConfigurations.BarkerTsSymbolRate);
  ss >> BarkerSymbRate;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> BarkerSymbRate = %d \n", __FUNCTION__, BarkerSymbRate);
  return BarkerSymbRate;
}

const uint32_t& TDvbConfig::GetBarkerEitTimeout()
{
  std::istringstream ss(DvbConfigurations.BarkerEitTimeout);
  ss >> BarkerEitTimeout;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> BarkerEitTimeout = %d \n", __FUNCTION__, BarkerEitTimeout);
  return BarkerEitTimeout;
}

const uint32_t& TDvbConfig::GetBackgroundScanInterval()
{
  std::istringstream ss(DvbConfigurations.BackGroundScanInterval);
  ss >> BackgrndScanInterval;
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> BackgrndScanInterval = %d \n", __FUNCTION__, BackgrndScanInterval);
  return BackgrndScanInterval;
}

bool TDvbConfig::GetFastScanStatus()
{
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBCONFIG", "<%s> \n", __FUNCTION__);
  std::string value("TRUE");
  return (DvbConfigurations.FastScanSmart.compare(value) == 0) ? true : false;
}
