/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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

#ifndef _RMFELEMENTBUFFERS_H
#define _RMFELEMENTBUFFERS_H

#define RMF_EB_SIGNATURE ((((int)'R')<<24)|(((int)'G')<<16)|(((int)'E')<<8)|((int)'B'))

typedef enum

{
     CONTENT_TYPE_PSI = 0,
     CONTENT_TYPE_OTHER
} rmf_content_type;

typedef enum _RMF_EB_FLAG 
{
   RMF_EB_FLAG_NONE=          (0>>0),
   RMF_EB_FLAG_ALLOW_INPLACE= (1>>0)
} RMF_EB_FLAG;

typedef struct _RmfElementBuffer
{
   unsigned int signature;
   int flags;
   int size;
   void *virtualAddress;
   void *physicalAddress;
} RmfElementBuffer;

#endif

