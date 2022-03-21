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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "net-snmp/net-snmp-config.h"
#include "net-snmp/definitions.h"
#include "SnmpIORM.h"
#include "ip_types.h"
#include "snmp_types.h"
#include "utilityMacros.h"
#include "rmf_osal_scriptexecutor.h"
#include "docsDevEventTable_enums.h"
#include "rdk_debug.h"

static int vlg_nValue = 0;
 
int vlSnmpLogDhcpEvent_ver_1_0(unsigned long nEvent, VL_DHCP_CLIENT_TYPE eType, VL_DHCP_CLIENT_ENTITY eClient)
{

    SNMP_DEBUGPRINT("vlSnmpLogDhcpEvent_ver_1_0: Received IP %s event %u, for %s\n", (eType ? "V6":"V4"), nEvent, (eClient ? "CableCard":"eSTB"));
    SNMP_DEBUGPRINT("vlSnmpLogDhcpEvent_ver_1_0: vlg_nValue = %d\n", vlg_nValue);
    //cases for DHCP EVENT ID
    
    if((VL_DHCP_CLIENT_TYPE_IPV6 == eType) && (VL_DHCP_CLIENT_ENTITY_ESTB == eClient))
    {
        // for IPV6 only
        switch(nEvent)
        {
            case ERRORCODE_D12_0: //68001200
            case ERRORCODE_D12_1: //68001201
            case ERRORCODE_D12_2: //68001202
            case ERRORCODE_D12_3: //68001203
            case ERRORCODE_D13_1: //68001301
            case ERRORCODE_D13_2: //68001302
            {
                EdocsDevEventhandling(nEvent, eType, eClient);
            }
            break;
            
            default:
            {
                // do nothing
            }
            break;
        }
    }

    // common for IPV4 and IPV6 only
    switch(nEvent)
    {
        case ERRORCODE_D01_0: //68000100
        case ERRORCODE_D02_0: //68000200
        case ERRORCODE_D03_1: //68000301
        case ERRORCODE_D101_0: //68010100
        case ERRORCODE_D102_0: //68010200
        case ERRORCODE_D103_0: //68010300
        case ERRORCODE_D104_0: //68010400
        {
            EdocsDevEventhandling(nEvent, eType, eClient);
        }
        break;
        
        default:
        {
            //nothing to do; 
        }
        break; 
    }

    return 0;
}

static RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY vlg_aDocsDhcpEventSymbols[] =
{
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(vlSnmpLogDhcpEvent_ver_1_0, 3),
    RMF_OSAL_SCRIPT_EXEC_MAKE_LVAL_SYMBOL_ENTRY(vlg_nValue)
};

RMF_OSAL_SCRIPT_EXEC_INST_SYMBOL_TABLE(vlg_DocsDhcpEventSymbolTable, vlg_aDocsDhcpEventSymbols);

int vlStartDocsEventServer()
{
//    vlCreateUdpScriptServer(54127, &vlg_DocsDhcpEventSymbolTable);
    rmf_osal_CreateUdpScriptServer(54127, &vlg_DocsDhcpEventSymbolTable);
	
    return 1;
}
