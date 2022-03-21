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


#ifndef __OOB_H__
#define __OOB_H__

#include <linux/sockios.h>


#define OOB_IOCTL_READ   SIOCDEVPRIVATE
#define OOB_IOCTL_WRITE (SIOCDEVPRIVATE + 1)

#define OOB_MTU 1500

typedef struct oob_data 
{
    unsigned int uiLen; 
    unsigned char pucBuf[OOB_MTU];
} oob_data;





#endif // __OOB_H__

