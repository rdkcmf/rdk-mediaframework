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

#include "TDvbTuner.h"

// C system includes
#include <unistd.h>

// C++ system includes
#include <string>
#include <sstream>

// Other libraries' includes

// Project's includes
#ifdef DVB_ENABLE_TRM
#include "TunerReservationHelper.h"
#endif

using std::string;
using std::stringstream;
using namespace TDvbStorageNamespace; 
  
rmf_ModulationMode TDvbTuner::GetMappedModulation(const TDvbStorageNamespace::TModulationMode& mod)
{
  rmf_ModulationMode rmfMode(RMF_MODULATION_UNKNOWN);
  switch (mod) {
  case  MODULATION_MODE_UNKNOWN:
    rmfMode = RMF_MODULATION_UNKNOWN;
    break;
  case  MODULATION_MODE_QPSK:
    rmfMode = RMF_MODULATION_QPSK;
    break;
  case  MODULATION_MODE_BPSK:
    rmfMode = RMF_MODULATION_BPSK;
    break;
  case  MODULATION_MODE_OQPSK:
    rmfMode = RMF_MODULATION_OQPSK;
    break;
  case  MODULATION_MODE_VSB8:
    rmfMode = RMF_MODULATION_VSB8;
    break;
  case  MODULATION_MODE_VSB16:
    rmfMode = RMF_MODULATION_VSB16;
    break;
  case  MODULATION_MODE_QAM16:
    rmfMode = RMF_MODULATION_QAM16;
    break;
  case  MODULATION_MODE_QAM32:
    rmfMode = RMF_MODULATION_QAM32;
    break;
  case  MODULATION_MODE_QAM64:
    rmfMode = RMF_MODULATION_QAM64;
    break;
  case  MODULATION_MODE_QAM80:
    rmfMode = RMF_MODULATION_QAM80;
    break;
  case  MODULATION_MODE_QAM96:
    rmfMode = RMF_MODULATION_QAM96;
    break;
  case  MODULATION_MODE_QAM112:
    rmfMode = RMF_MODULATION_QAM112;
    break;
  case  MODULATION_MODE_QAM128:
    rmfMode = RMF_MODULATION_QAM128;
    break;
  case  MODULATION_MODE_QAM160:
    rmfMode = RMF_MODULATION_QAM160;
    break;
  case  MODULATION_MODE_QAM192:
    rmfMode = RMF_MODULATION_QAM192;
    break;
  case  MODULATION_MODE_QAM224:
    rmfMode = RMF_MODULATION_QAM224;
    break;
  case  MODULATION_MODE_QAM256:
    rmfMode = RMF_MODULATION_QAM256;
    break;
  case  MODULATION_MODE_QAM320:
    rmfMode = RMF_MODULATION_QAM320;
    break;
  case  MODULATION_MODE_QAM384:
    rmfMode = RMF_MODULATION_QAM384;
    break;
  case  MODULATION_MODE_QAM448:
    rmfMode = RMF_MODULATION_QAM448;
    break;
  case  MODULATION_MODE_QAM512:
    rmfMode = RMF_MODULATION_QAM512;
    break;
  case  MODULATION_MODE_QAM640:
    rmfMode = RMF_MODULATION_QAM640;
    break;
  case  MODULATION_MODE_QAM768:
    rmfMode = RMF_MODULATION_QAM768;
    break;
  case  MODULATION_MODE_QAM896:
    rmfMode = RMF_MODULATION_QAM896;
    break;
  case  MODULATION_MODE_QAM1024:
    rmfMode = RMF_MODULATION_QAM1024;
    break;
  case  MODULATION_MODE_QAM_NTSC: // for analog mode
    rmfMode = RMF_MODULATION_QAM_NTSC;
    break;
  };
  return rmfMode;
}
TDvbTuner::TDvbTuner()
  : RmfQamSrc(NULL),
    ReslutRmfError(RMF_RESULT_SUCCESS)
{
  StreamingStatus.m_status = 0;
  #ifdef DVB_ENABLE_TRM
  TunerHelper = new TunerReservationHelper(NULL);
  #endif
}

TDvbTuner::~TDvbTuner()
{
  UnTune();
  #ifdef DVB_ENABLE_TRM
  delete TunerHelper;
  #endif
}

bool TDvbTuner::Tune(const uint32_t& freq, const TModulationMode& mod, const uint32_t& symbolRate)
{
  stringstream ss;
  bool retVal(false);
  // Let's build the locator
  ss << "tune://frequency=" << freq << "&modulation=" << GetMappedModulation(mod) << "&symbol_rate=" << symbolRate << "&pgmno=0&dvb=1";
  string locator = ss.str();

  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBTUNER", "%s(): locator = %s\n", __FUNCTION__, locator.c_str());

#ifdef DVB_ENABLE_TRM
  if (!TunerHelper->reserveTunerForLive(locator.c_str(), 0, 0, true)) {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBTUNER", "%s(): failed to reserve a tuner for %s\n", __FUNCTION__, locator.c_str());
    return retVal;
  }
#endif
  // Time to get/create a QAMSrc instance
  RmfQamSrc = RMFQAMSrc::getQAMSourceInstance(locator.c_str());
  if (RmfQamSrc == NULL) {
    RDK_LOG(RDK_LOG_ERROR, "%s(): Source is invalid.\n", __FUNCTION__);
    return retVal;
  }
  // Register for events
  if (RMF_RESULT_SUCCESS == RmfQamSrc->addEventHandler(this)) {
    retVal= true;
  }
  return retVal;
}

void TDvbTuner::UnTune()
{
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBTUNER", "%s(): called\n", __FUNCTION__);
  // if QAMSrc instance exists then we have some clean up to do
  if (RmfQamSrc) {
    // Clean up
    RmfQamSrc->removeEventHandler(this);
    ReslutRmfError = RMF_RESULT_SUCCESS;
    StreamingStatus.m_status = 0;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBTUNER", "%s(): About to free the QAMSrc instance\n", __FUNCTION__);
    RMFQAMSrc::freeQAMSourceInstance(RmfQamSrc);
    RmfQamSrc = NULL;
    RMFQAMSrc::freeCachedQAMSourceInstances();
#ifdef DVB_ENABLE_TRM
    TunerHelper->releaseTunerReservation();
#endif
  }
}
