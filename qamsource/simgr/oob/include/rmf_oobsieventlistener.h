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
 

#ifndef __RMF_OOBSIEVENTLISTENER_H__
#define __RMF_OOBSIEVENTLISTENER_H__

class rmf_OobSiEventListener
{
public:
	virtual void send_si_event(uint32_t siEvent, uint32_t optional_data, uint32_t changeType) = 0;
};

#endif
