/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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
*/ 



#include "cardUtils.h"
#ifdef OOB_TUNE 
//#include "pfcoobtunerdriver.h"
#endif
#include "tunerdriver.h"

#include <rdk_debug.h>
    

ctuner_driver::ctuner_driver( CardManager *cm)
{
    
    this->cm = cm;
#ifdef OOB_TUNE 
#ifdef USE_POD
    oob_tuner_device = pfc_kernel_global->get_oob_tuner_device();
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ctuner_driver::ctuner_driver..%x\n",oob_tuner_device);
    if(oob_tuner_device)
    {
        oob_tuner_device->open_device( OOB_RX_TUNER);
        //oob_tuner_device->open_device(0,OOB_TX_TUNER); currently using OOB_Rx  handle for  tx too 
    }else
#endif
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","error getting oob_tuner_device\n");
    inband_tuner_device = pfc_kernel_global->get_tuner_device();
#endif

}


ctuner_driver::ctuner_driver()
{
    this->cm = 0;
#ifdef OOB_TUNE 
this->oob_tuner_device = 0;
this->inband_tuner_device = 0;
#endif

}

ctuner_driver::~ctuner_driver()
{

}    

#ifdef OOB_TUNE   
pfcTunerStatus ctuner_driver:: rx_tune(TUNER_PARAMS *params)
{
    pfcTunerStatus status =PFC_TUNER_STATUS_LOCK;
   
    if(oob_tuner_device)
    {
        
        status = oob_tuner_device->oob_rx_tune(   params );
    }

    return status;    
}
#endif

#ifdef OOB_TUNE 
pfcTunerStatus ctuner_driver::tx_tune(TUNER_PARAMS *params)
{
pfcTunerStatus status =PFC_TUNER_STATUS_LOCK;

    if(oob_tuner_device)
    {
         
        status = oob_tuner_device->oob_tx_tune(   params );
    }

    return status;
}
#endif

#ifdef OOB_TUNE 
pfcTunerStatus ctuner_driver::inband_tune(long freqInHz, unsigned long modulation)
{
    pfcTunerModulationMode    tunerMode;
    pfcTunerStatus status = PFC_TUNER_STATUS_INVALID_TUNER;
    if(inband_tuner_device)
    {
        if(modulation == 1)
        {
            tunerMode = PFC_TUNER_MODULATION_QAM64;
        }
        else if(modulation == 2)
        {
            tunerMode = PFC_TUNER_MODULATION_QAM256;
        }
        else
        {
            return PFC_TUNER_STATUS_INVALID_MODULATION;
        }

        status = inband_tuner_device->tune_to_freq(freqInHz, tunerMode, 0xFFFFFFFF, true);
                                        
    }

    return status;
}
#endif

#ifdef OOB_TUNE 

extern "C" int GetNumInBandTuners(int *pNumInbandTuners)
{
    pfcTunerDriver *pInbandTuner;
    pInbandTuner = pfc_kernel_global->get_tuner_device();

    if(pInbandTuner)
    {
        return pInbandTuner->GetNumInbandTuners(pNumInbandTuners);
    }
    return 0;
}
#endif

