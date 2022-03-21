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


#ifndef _OB_QPSK_H_
#define _OB_QPSK_H_
//===================================================================
//   INCLUDES AND PUBLIC DATA DECLARATIONS
//===================================================================


typedef enum
{
    OOB_DS_QPSK_TUNE_OK        = 0,
    OOB_DS_QPSK_TUNE_NA        = 1,
    OOB_DS_QPSK_TUNE_FAILED    = 2,    //busy
    OOB_DS_QPSK_TUNE_BAD_PARAM = 3,
    OOB_DS_QPSK_TUNE_TIMED_OUT = 4,  //used only by POD
    OOB_DS_QPSK_TUNE_IGNORE    = 5   //used only by POD==reserved
} oob_ds_tune_status;

typedef enum
{
    OOB_US_QPSK_TUNE_OK        = 0,
    OOB_US_QPSK_TUNE_NA        = 1,
    OOB_US_QPSK_TUNE_FAILED    = 2,    //busy
    OOB_US_QPSK_TUNE_BAD_PARAM = 3,
    OOB_US_QPSK_TUNE_TIMED_OUT = 4,  //used only by POD
    OOB_US_QPSK_TUNE_IGNORE    = 5   //used only by POD==reserved
} oob_us_tune_status;

typedef enum
{
    OOB_QPSK_TUNE_OK                = 0,
    OOB_QPSK_TUNE_NO_TRANSMITTER    = 1,
    OOB_QPSK_TUNE_TRANSMITTER_BUSY  = 2,    //busy
    OOB_QPSK_TUNE_INVALID_PARAMS    = 3,
    OOB_QPSK_TUNE_OTHER_REASONS     = 4,  //used only by POD
} oob_qpsk_tune_status;
typedef enum
{
   INBAND_TUNE_OK                = 0,
   INBAND_TUNE_INVALID_FREQ    = 1,
   INBAND_TUNE_INVALID_MOD  = 2,    //busy
   INBAND_TUNE_HW_FAIL    = 3,
   INBAND_TUNE_TUNER_BUSY    = 4,  //used only by POD
#ifdef USE_IPDIRECT_UNICAST
   INBAND_TUNE_TUNER_RELEASE_ACK = 5,  //used only by POD
   INBAND_TUNE_OTHER_RES    = 6,  //used only by POD
#else
   INBAND_TUNE_OTHER_RES    = 5,  //used only by POD
#endif
} inband_tune_status;

#define OB_IF_Frequency     50.0        // OB DS IF frequency in MHz

#ifdef __cplusplus
extern "C" {
#endif

void ConfigureDownstreamOB (unsigned long ObDsRfFrequency, 
                            unsigned long ObDsSymbolRate);

Bool ob_ds_lock_read (void);
void ConfigureOB (void);
void bc_ObDsRead(int access, int option, int *val1, int *val2);

int frontend_ds_oob_tune_request(unsigned long rf_freq, unsigned long baud_rate);

#ifdef __cplusplus
}
#endif

#endif  /* end of _OB_QPSK_H_ */

