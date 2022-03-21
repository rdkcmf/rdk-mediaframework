/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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

#ifndef DIAG_MSG_EVENT_H
#define DIAG_MSG_EVENT_H


//New event types and data structures for Boot up manager
typedef enum
{
    RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD = 0xA001,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_CA,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_CP,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_FWDNLD,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_XAIT,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_MONITOR,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_AUTO,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_SI,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_GENMSG,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_TRU2WAY_PROXY,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_XRE,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_FLASH_WRITE,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_INIT,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_COLD_INIT,
    RMF_BOOTMSG_DIAG_EVENT_TYPE_FP_STATUS,    
    RMF_BOOTMSG_DIAG_EVENT_TYPE_MAX
} RMF_BOOTMSG_DIAG_EVENT_TYPE;

typedef enum
{
    RMF_BOOTMSG_DIAG_MESG_CODE_INIT,
    RMF_BOOTMSG_DIAG_MESG_CODE_START,
    RMF_BOOTMSG_DIAG_MESG_CODE_PROGRESS,
    RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE,
    RMF_BOOTMSG_DIAG_MESG_CODE_ERROR,
    RMF_BOOTMSG_DIAG_MESG_CODE_MSG,
    RMF_BOOTMSG_DIAG_MESG_CODE_DAC_INIT,
    RMF_BOOTMSG_DIAG_MESG_CODE_DAC_COLD_INIT,
    RMF_BOOTMSG_DIAG_MESG_CODE_FP_TIME_ENABLE,
    RMF_BOOTMSG_DIAG_MESG_CODE_FP_TIME_DISABLE,
}RMF_BOOTMSG_DIAG_MESG_CODE;

typedef struct {

    RMF_BOOTMSG_DIAG_MESG_CODE   msgCode;
    /*data :-  will contain additional information about error codes (in case of error message) for the display
    to show and in case of “In Progress” message it may contain the data to represent like %complete , etc.*/
    int                 data;		//NOTE: data will contain the address of the string when RMF_BOOTMSG_DIAG_MESG_CODE_MSG is used
    int 				dataLength;//Used only when RMF_BOOTMSG_DIAG_MESG_CODE_MSG is used for message display.
}RMF_BOOTMSG_DIAG_EVENT_INFO;

#endif

