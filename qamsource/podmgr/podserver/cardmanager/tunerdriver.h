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



#ifndef _CTUNER_DRIVER_H_
#define _CTUNER_DRIVER_H_
#ifdef OOB_TUNE
//#include "pfcoobtunerdriver.h"
#endif
//#include "pfctunerdriver.h"

class CardManager;

class ctuner_driver
{
friend class CardManager;
public:
    
    ctuner_driver(CardManager *cm);
     ctuner_driver();
    ~ctuner_driver();
#ifdef OOB_TUNE
    pfcTunerStatus  rx_tune(TUNER_PARAMS *params);
    pfcTunerStatus  tx_tune(TUNER_PARAMS *params);
    pfcTunerStatus  inband_tune(long freqInMhz, unsigned long modulation);    
private:
    pfcOOBTunerDriver    *oob_tuner_device;
    pfcTunerDriver        *inband_tuner_device;
#endif
    CardManager         *cm;
};
#endif


