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


#include "cablecarddriver.h"
#include <stdio.h>

cableCardDriver::cableCardDriver ()
{

	plugin_title_argument = NULL;

}

cableCardDriver::~cableCardDriver ()
{
	
}

char *
cableCardDriver::plugin_title ()
{
	return plugin_title_argument;
}

void
cableCardDriver::initialize ()
{
// 	fprintf (stderr, "cableCardDriver::initialize undefined\n");
}

int
cableCardDriver::open_device (int device_instance)
{
	return 0;
}

int
cableCardDriver::close_device (int device_instance)
{
	return 0;
}

 
 
int
cableCardDriver::register_callback(eCallbackType type , void *cb, int device_instance)
{
	return 0;
}


int
cableCardDriver::send_data(void *ptr, CHANNEL_TYPE_T  type, int device_instance)
{
 
	return 0;
}

 

int
cableCardDriver::get_device_count()
{

	return 0;
}



void
cableCardDriver::ConnectSourceToPOD(unsigned long tuner_in_handle,int device_instance,unsigned long videoPid)
{
	return;
}

int
cableCardDriver::oob_control(int device_instance, unsigned long eCommand, void * pData)
{
    return 0;
}

int
cableCardDriver::if_control(int device_instance, unsigned long eCommand, void * pData)
{
    return 0;
}


int  cableCardDriver::configPodCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *pStrPtr)
{
  return 0;
}
int  cableCardDriver::stopconfigPodCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids)
{
  return 0;
}
