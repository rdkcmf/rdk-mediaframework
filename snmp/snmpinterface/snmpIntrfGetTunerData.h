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


/** @defgroup SNMP_INT_TUNER SNMP Interface Tuner Data
 * This module gets the necessary info about available tuners.
 * @ingroup SNMP_INTRF
 * @{
 */
  
#ifndef __SNMP_TUNER_INTERFACE__
#define __SNMP_TUNER_INTERFACE__

#include "SnmpIORM.h"

class snmpTunerInterface
{

       public:
        
        snmpTunerInterface();
        ~snmpTunerInterface();
        
        int Snmp_getTunerdetails(SNMPocStbHostInBandTunerTable_t* vl_InBandTunerTable, unsigned long tunerIndex);
        
        void Snmp_getQPSKTunerdetails(/*QpskObj*/);
         
        void fillUp_InBandTunerTable(SNMPocStbHostInBandTunerTable_t* vl_InBandTunerTable, void* ptSnmpHALTunersInfo, unsigned int numOfTuners);


    private:
};

#endif //__SNMP_TUNER_INTERFACE__
/** @} */
