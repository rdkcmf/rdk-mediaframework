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
* \file TLVeventmgr
*/

#ifndef TLV217EVENT_LISTENER_H
#define TLV217EVENT_LISTENER_H

#include "cmThreadBase.h"

#include "dsg_types.h" //TVL parse
#include"bufParseUtilities.h"//TLV217 parsing
#include <vector>
#undef min
#undef max
#include <string>
#include "utilityMacros.h"
#include "TlvConfigObjs.h"
#include "ip_types.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_event.h"
#include "vlMutex.h"
using namespace std;

class TlvConfig:public CMThread
	{
public:
    //! Constructor.
    TlvConfig();
    ~TlvConfig();

	
    int is_default();
    int initialize(void);
    int init_complete(void );
	
    //! Main thread routine. Pulls events from the #event_queue and dispatches them in #epgm.
    void run(void);
    void loadLocalTlv217ConfigFile();
    VL_TLV_217_FIELD_DATA vlSnmpProcessTlv217Field(VL_BYTE_STREAM * pParentStream,  int nParentLength);
    int vlSnmpParseTlv217Data (VL_BYTE_STREAM * pParentStream, int nParentLength);
    int vlsnmpIsValidNewTag(VL_DSG_TLV_217_TYPE eTag);

    int vlSNMPv1v2cParse(VL_BYTE_STREAM * pParentStream,  int nParentLength);
    VL_TLV_TRANSPORT_ADDR_AND_MASK vlSNMPv1v2cTransportAddressAccessParse(VL_BYTE_STREAM * pParentStream,  int nParentLength);
    int vlSNMPv3_AccessViewConfigParse(VL_BYTE_STREAM * pParentStream, int nParentLength);
    int vlSNMPmibObjectParse(VL_BYTE_STREAM * pParentStream,  int nParentLength);
    int vlSNMPIpModeControlParse(VL_BYTE_STREAM * pParentStream,  int nParentLength);
    VL_TLV_217_FIELD_DATA vlSNMPVendorIdParse(VL_BYTE_STREAM * pParentStream,  int nParentLength);
    int vlSNMPVendorSpecificParse(VL_BYTE_STREAM * pParentStream, int nParentLength);
    int vlSNMPv3_NotificationReceiver_Parse(VL_BYTE_STREAM * pParentStream,int nParentLength);
    string vlGetCharParseBuff(VL_TLV_217_FIELD_DATA buffparse, int lenght);
    VL_TLV_217_OID vlGetAsnEncodedOidParseBuff(VL_TLV_217_FIELD_DATA buffparse, int lenght);
    VL_TLV_217_OID vlGetTlvOid(VL_BYTE_STREAM * pParentStream,  int nParentLength, int eTlvType);
    VL_TLV_SNMP_VAR_BIND vlGetSnmpVarBind(VL_BYTE_STREAM * pParentStream,  int nParentLength);
    unsigned long vlGetInt_typeParseBuff(VL_TLV_217_FIELD_DATA buffparse, int lenght);

    //54.4
    vector<v3AccessviewList_t> V3AccviewLs;

    //tlv 11--OB
    vector<VL_TLV_SNMP_VAR_BIND> SNMPmibObjectList;
    //tlv 08
    vector<vector<unsigned char> > VendorIdList;
    vector<int> IpModeControlList;
    //tlv 28---0x2B
    vector<vector<unsigned char> > VendorSpecificList;
    vector<int>    tlvdefault_UIlist; //undientified tlv-subtlv list
    vector<TLV53_V1V2_CONFIG> Tlv217loopList;
    vector<v3NotificationReceiver_t> v3NotificationReceiver_list;
    /* check's the Tlv217loopList for the duplicates and empty values and all error events cases in this method*/
    void checkEventlistInTlv217list();
    void DisableSnmpdConfUpdate(const char * pszFunction, int nLineNo);
    void RejectV1V2CoexistenceConfig(const char * pszFunction, int nLineNo);
    void RejectV3AccessViewConfig(const char * pszFunction, int nLineNo);

    /* To Genrate the Events */
    //53.1
    void Generate_Events_V1V2Community();
    //53.2--53.2.1, 53.2.2
    void Generate_Events_V1V2TransportAddress_MaxAdd();
    //53.3
    void Generate_Events_V1V2TypeNameAccess();
    //54.4
    void Generate_Events_V1V2NameAccess();
    //54-TLV
    void Generate_Events_V3AVMaskTypeEvent();
    //11-TLV
    void Generate_Events_MIB_OID_VALUE();
    //8-TLV
    void Generate_Events_VendorID();
    //48-TLV
    void Generate_Events_VendorSpec();
    //1-TLV
    void Generate_Events_IpModeControl();
    //38-TLV
    void Generate_Events_v3NotificationReceiver();
    //General Events
    void Generate_Events_Genral();


    /** Set The ALL list of avilabel valus if gcSetfalg is true */
    void Set_AllListV1v2v3objvalus(VL_HOST_IP_INFO * pPodIpInfo = NULL); //write into the snmpd.conf file
    void registerBootupConfig();
    /**Snmp set command to set the oid and value of specfic community (snmpset system commnad is used)*/
    void vlSnmpSet_OIDvalue(char* OIDset, void *oidValue,char valuetype, char *IPaddress, char *CommunityString);

    static bool m_bUpdateSnmpdConf;
    
    static class vlMutex & vlGetTlvEventDbLock();
    
private:
    int doInitialize();
    rmf_osal_eventqueue_handle_t m_pEvent_queue; //!< Event queue to listen to. Set by constructor.
    void processTlv217SnmpSetRequest(char * szOidStr, int eVarBindType, unsigned char * mibOIDvalue, int nVarBindLength);
    unsigned char * m_pTlv217Data;
    int m_nTlv217DataBytes;
    
    void ProcessTlv217Data();
    static void taskProcessTlv217Data(void * pParam);

    void setVolatileConfig(VL_TLV_SNMP_STRING_VAR_BIND_LIST & raConfig);
};

#endif//TLV217EVENT_LISTENER_H
