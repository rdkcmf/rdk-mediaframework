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


/**
 * @defgroup SNMP SNMP (Simple Network Management Protocol)
 * Simple Network Management Protocol (SNMP) is an application-layer protocol
 * defined by the Internet Architecture Board (IAB) in RFC1157 for exchanging
 * management information between network devices. It is a part of Transmission
 * Control Protocol/Internet Protocol (TCP/IP) protocol suite. SNMP enables network
 * administrators to manage network performance, find and solve network problems,
 * and plan for network growth.
 *
 * @par Key Components:
 * An SNMP-managed network consists of three key components:
 * - Managed Device: It is a network node that contains an SNMP agent and that 
 * resides on a managed network. Managed devices collect and store management
 * information and make this information available to NMSs using SNMP. Managed
 * devices, sometimes called network elements, can be routers and access servers,
 * switches and bridges, hubs, computer hosts, or printers.
 * 
 * - SNMP Agent: An agent is a network-management software module that resides in
 * a managed device. An agent has local knowledge of management information and
 * translates that information into a form compatible with SNMP.
 *
 * - Network-Management Systems: An NMS executes applications that monitor and
 * control managed devices. NMSs provide the bulk of the processing and memory
 * resources required for network management. One or more NMSs must exist on any
 * managed network.
 *
 * @image html SNMP_Components.png
 *
 * @defgroup SNMP_INTRF SNMP Interface
 * SNMP interface deals with the information about the interfaces used in the SNMP.
 * It Includes:
 * - Data from AV out interface like DVI/HDMI, number of tuners, type of video,
 * port type like hdmi etc.
 * - Info about the tuners like tunerIndex, frequency, mode, lock status etc.
 * and saves these info to the corresponding tables.
 * 
 * @ingroup SNMP
 *
 * @defgroup SNMP_MGR SNMP Manager
 * SNMP Manager deals with the information about how SNMP manages with the interfaces.
 * It includes:
 * - SNMP boot handler, where event queue for the SNMP boot handler are registered.
 * - Host interface, where info about AV interface, Tuner, DVI/HDMI, video format
 * etc are retrieved and copied them to their respective Info table.
 * - Data about host utility. These are used to get info like channel number,
 * Service, encryptions, closed caption, front panel led status etc.
 * - ECM and VACM (Visual-based Access Control Management) to get info about oid,
 * max id, channel frequency, IP address etc.
 * - It also include info about xcalibur client, getting ID of varios parameters
 * like plant id etc., power condition, DVR related info.
 * - About Vidipath and Vividlogic certification related info Interfaces like
 * MOCA, media streamer server, handling requests etc.
 *
 * @ingroup SNMP
 *
 */



 

  
#ifndef __SNMPMGR_H__
#define __SNMPMGR_H__

//#include "pfckernel.h"
#include "cmThreadBase.h"
//#include "pfccondition.h"
//#include "pfcresource.h"
#include "cmevents.h"

//#include "gateway.h"
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif
//#include "hal_api.h"

//#include "Tlvevent.h"

typedef int HostId_t;
typedef int HardwareId_t;
typedef int UIMode;
typedef int CaSystemId_t;
typedef int LCSM_DEVICE_HANDLE;

typedef void (* CallBackFn)( unsigned queue,LCSM_EVENT_INFO *eventInfo );  // for send_event ( called by POD stack)

#define TUNER_MASK(enumtype)   (1 << enumtype)

#ifdef __cplusplus
 
extern "C" {

#endif

//extern void POD_STACK_NOTIFY_CALLBACK(void * pFn);
extern void rmProc( void * par );
extern void rsmgr_init(void);
//extern int POD_STACK_INIT(LCSM_DEVICE_HANDLE *h); 
 
#if 0
void  pdtTunerNotifyFn( DEVICE_HANDLE_t         hDevicehandle, 
                       DEVICE_LOCK_t        eStatus, 
                       UINT32                ulFrequency,
                       TUNER_MODE_t            enTunerMode,
                       UINT32                ulSymbolRate,
                       void                 *pulData);
#endif
#ifdef __cplusplus
}
#endif

#define MAX_TUNERS    2
 

class SnmpManager;
class cPODDriverWrapper;
class cOOB;
class ctuner_driver;
class cPOD_driver;

class SnmpManager: public CMThread
{

public:

    SnmpManager();
    ~SnmpManager();
    int is_default();
    int initialize(void );
    int init_complete(void );
    void run(void);
    
private:
        int doInitialize();

};



#endif 




