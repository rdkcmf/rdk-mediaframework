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

#ifndef DVBTUNER_H_
#define DVBTUNER_H_

// Other libraries' includes
#include "TDvbStorageNamespace.h"

// Project's includes
#include "rmfbase.h"
#include "rmfqamsrc.h"
#include "rmf_qamsrc_common.h"

// Forward declarations
class RMFQAMSrc;

#ifdef DVB_ENABLE_TRM
class TunerReservationHelper;
#endif

/**
 * RMF DVB Tuner class. Used by DvbSiStorage to implement the DVB Scan feature.
 */
class TDvbTuner : public IRMFMediaEvents
{
private:
  // Make it non copyable.
  TDvbTuner(const TDvbTuner& other)  = delete;
  TDvbTuner& operator=(const TDvbTuner&)  = delete;
  RMFQAMSrc* RmfQamSrc;

  // Add a sink when needed
  #ifdef DVB_ENABLE_TRM
  TunerReservationHelper* TunerHelper;
  #endif
  RMFStreamingStatus StreamingStatus;
  RMFResult ReslutRmfError;
public:
  TDvbTuner();
  ~TDvbTuner();
  
  rmf_ModulationMode GetMappedModulation(const TDvbStorageNamespace::TModulationMode& mod);
  bool Tune(const uint32_t& freq, const TDvbStorageNamespace::TModulationMode& mod, const uint32_t& symbol_rate);
  void UnTune();
};

#endif /* DVBTUNER_H_ */
