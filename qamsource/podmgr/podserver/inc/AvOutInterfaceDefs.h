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



#ifndef __AVINTERFACEDEFS_H__
#define __AVINTERFACEDEFS_H__


typedef enum
{
    AV_INTERFACE_TYPE_RF,
    AV_INTERFACE_TYPE_COMPOSITE, /*BB RCA connector*/
    AV_INTERFACE_TYPE_SVIDEO,
    AV_INTERFACE_TYPE_1394,
    AV_INTERFACE_TYPE_DVI,
    AV_INTERFACE_TYPE_COMPONENT,
    AV_INTERFACE_TYPE_HDMI,
    AV_INTERFACE_TYPE_L_R,
    AV_INTERFACE_TYPE_SPDIF,

    AV_INTERFACE_TYPE_MAX
} AV_INTERFACE_TYPE;

typedef enum
{
    AV_PORT_STATUS_DISABLED = 0,
    AV_PORT_STATUS_ENABLED
}AV_PORT_STATUS;


typedef enum
{
    AV_PORT_HDMI_AUDIO_MODE_AUTO = 1,
    AV_PORT_HDMI_AUDIO_MODE_PCM,
    AV_PORT_HDMI_AUDIO_MODE_MAX
}AV_PORT_HDMI_AUDIO_MODE;

typedef struct{

    AV_PORT_STATUS        portStatus;
    AV_INTERFACE_TYPE    avOutPortType;

}AV_INTERFACE_PORT_INFO;

typedef struct s_AVOutInterfaceInfo
{
    unsigned long   deviceHandle;
    unsigned long    numAvOutPorts;

    //AV_INTERFACE_TYPE avOutPortTypes[AV_INTERFACE_TYPE_MAX];
    AV_INTERFACE_PORT_INFO avOutPortInfo[AV_INTERFACE_TYPE_MAX];

}AVOutInterfaceInfo;


#endif//__AVINTERFACE_IORM_HEADER

