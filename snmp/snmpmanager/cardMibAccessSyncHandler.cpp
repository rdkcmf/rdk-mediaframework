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



#define VL_USE_SYNC_IMPL_FOR_CARD_MIB_ACCESS
#ifdef VL_USE_SYNC_IMPL_FOR_CARD_MIB_ACCESS

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
#include <sys/socket.h>
#include "docsDevEventTable_enums.h"
#include "vlMutex.h"
#include "vlMap.h"
#include "cm_api.h"
#include "cardMibAccessProxyServer.h"
#include "net-snmp/library/asn1.h"
#include "bufParseUtilities.h"
#include "coreUtilityApi.h"
#include "ipcclient_impl.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "rmf_osal_sync.h"
#include <semaphore.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define vlStrCpy(pDest, pSrc, nDestCapacity)            \
            strcpy(pDest, pSrc)
			
#define vlMemCpy(pDest, pSrc, nCount, nDestCapacity)            \
                                                  memcpy(pDest, pSrc, nCount)
                                                  
#ifdef SNMP_DEBUGE_MESG
    #define VL_SNMP_DEBUG_FLAG  1
    
    #undef DEBUGMSG
    #undef DEBUGMSGTL
    #undef DEBUGMSGOID
    
    #define DEBUG_FUNC_PRINT(nm, fm, args...) fprintf( stderr, "%s:%d:"#fm"\n", nm,__LINE__, ##args)
    
    #define DEBUGMSG(x)        do {if (VL_SNMP_DEBUG_FLAG) {DEBUG_FUNC_PRINT x;} }while(0)
    #define DEBUGMSGTL(x)      do {if (VL_SNMP_DEBUG_FLAG) {DEBUG_FUNC_PRINT x;} }while(0)
    #define DEBUGMSGOID(x)     do {if (VL_SNMP_DEBUG_FLAG) {__DBGMSGOID(x);} }while(0)
#endif // SNMP_DEBUGE_MESG

extern Status_t gcardsnmpAccCtl;

extern "C" int snmp_build(u_char ** pkt, size_t * pkt_len,
                           size_t * offset, netsnmp_session * pss,
                           netsnmp_pdu *pdu);
                           
#define VL_CMA_PROXY_SERVER_COMMUNITY           "public"
#define VL_CMA_PROXY_SERVER_COMMUNITY_LENGTH    strlen(VL_CMA_PROXY_SERVER_COMMUNITY)

#define VL_SNMP_CARD_MIB_ACCESS_PROXY_INITIAL_PACKET_SIZE   2048

static vlMutex vlg_cardMibAccess_lock;
static vlMap vlg_cardMibAccessProxy_response_map;
static netsnmp_handler_registration * vlg_CardMibAccessProxy_reg = NULL;

//static long vlg_semaphore = 0;
sem_t* vlg_semaphore = NULL;

int
vlSnmp_cardMibAccessProxy_handler_got_response(int operation,
        netsnmp_agent_request_info *reqinfo,
        netsnmp_pdu *pdu, void *cb_data);

static void vlSendCardMibAccessRequest(unsigned char * pData, unsigned int nBytes)
{
    if((NULL != pData) && (nBytes > 0))
    {
		IPC_CLIENT_SNMPSendCardMibAccSnmpReq(pData, nBytes);
    }
}

int vlSnmp_cardMibAccessProxy_async_send(netsnmp_pdu * pdu, void *cb_data)
{
    int reqid  = 0;
    if((NULL != pdu) && (NULL != cb_data))
    {
        struct timeval tm;
        VL_ZERO_STRUCT(tm);
        gettimeofday(&tm, NULL);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s : Got Request %d at %u.%u\n", __FUNCTION__, pdu->reqid, (tm.tv_sec), (tm.tv_usec/1000));
        netsnmp_session session, *ss;
        VL_ZERO_STRUCT(session);
        snmp_sess_init( &session );                   /* set up defaults */
        session.peername = "127.0.0.1";
        session.version = SNMP_VERSION_2c;
        session.community = (unsigned char*)VL_CMA_PROXY_SERVER_COMMUNITY;
        session.community_len = strlen((char*)(session.community));
        ss = snmp_open(&session);
        
        if(NULL != ss)
        {
            //vlg_cardMibAccessProxy_request_map.setElementAt((vlMap::KEY)(pdu->reqid), (vlMap::ELEMENT)cb_data);
            //unsigned char * pktbuf = (unsigned char*)malloc(VL_SNMP_CARD_MIB_ACCESS_PROXY_INITIAL_PACKET_SIZE);
            unsigned char *pktbuf = NULL;
            rmf_osal_memAllocP(RMF_OSAL_MEM_POD, VL_SNMP_CARD_MIB_ACCESS_PROXY_INITIAL_PACKET_SIZE, (void**)&pktbuf);
            size_t length = VL_SNMP_CARD_MIB_ACCESS_PROXY_INITIAL_PACKET_SIZE;
            unsigned int encoded_offset = 0;
        
            if(NULL != pktbuf)
            {
                pdu->version = session.version;
                /*
                * do we expect a response?
                */
                switch (pdu->command) {

                    case SNMP_MSG_RESPONSE:
                    case SNMP_MSG_TRAP:
                    case SNMP_MSG_TRAP2:
                    case SNMP_MSG_REPORT:
                        pdu->flags &= ~UCD_MSG_FLAG_EXPECT_RESPONSE;
                        break;
            
                    default:
                        pdu->flags |= UCD_MSG_FLAG_EXPECT_RESPONSE;
                        break;
                }
                int bld_result = snmp_build(&pktbuf, &length, &encoded_offset, ss, pdu);
#ifdef NETSNMP_USE_REVERSE_ASNENCODING
                unsigned char * encoded_packet = pktbuf + length - encoded_offset;
#else // NETSNMP_USE_REVERSE_ASNENCODING
                unsigned char * encoded_packet = pktbuf;
#endif // else NETSNMP_USE_REVERSE_ASNENCODING
                SNMP_DEBUGPRINT("snmp_build() result = %d, size  = %d\n", bld_result, encoded_offset);
                if((NULL != encoded_packet) && (0 == bld_result) && (encoded_offset > 0))
                {
                    //vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.SNMP", encoded_packet, encoded_offset);
                    vlSendCardMibAccessRequest(encoded_packet, encoded_offset);
                    reqid = pdu->reqid;
                }
                snmp_free_pdu(pdu);
                if(NULL != pktbuf)
                    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pktbuf);
                    //free(pktbuf);
            }
            
            snmp_close(ss);
        }
    }
    
    return reqid;
}

int
vlSnmp_cardMibAccessProxy_handler(netsnmp_mib_handler *handler,
                netsnmp_handler_registration *reginfo,
                netsnmp_agent_request_info *reqinfo,
                netsnmp_request_info *requests)
{
    netsnmp_pdu    *pdu;
    struct simple_proxy *sp;
    oid            *ourname;
    size_t          ourlength;
    netsnmp_request_info *request = requests;
    u_char         *configured = NULL;
    int             reqid = 0;

    DEBUGMSGTL((__FUNCTION__, "proxy handler starting, mode = %d\n",
                reqinfo->mode));

    switch (reqinfo->mode) {
        case MODE_GET:
        case MODE_GETNEXT:
            case MODE_GETBULK:         /* WWWXXX */
                pdu = snmp_pdu_create(reqinfo->mode);
                break;

        case MODE_SET_ACTION:
            pdu = snmp_pdu_create(SNMP_MSG_SET);
            break;

        case MODE_SET_UNDO:
        /*
            *  If we set successfully (status == NOERROR),
            *     we can't back out again, so need to report the fact.
            *  If we failed to set successfully, then we're fine.
        */
            for (request = requests; request; request=request->next) {
                if (request->status == SNMP_ERR_NOERROR) {
                    netsnmp_set_request_error(reqinfo, requests,
                                              SNMP_ERR_UNDOFAILED);
                    return SNMP_ERR_UNDOFAILED;
                }
            }
            return SNMP_ERR_NOERROR;

        case MODE_SET_RESERVE1:
        case MODE_SET_RESERVE2:
        case MODE_SET_FREE:
        case MODE_SET_COMMIT:
        /*
            *  Nothing to do in this pass
        */
            return SNMP_ERR_NOERROR;

        default:
            snmp_log(LOG_WARNING, "unsupported mode for proxy called (%d)\n",
                     reqinfo->mode);
            return SNMP_ERR_NOERROR;
    }

    if (!pdu || !reginfo) {
        netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
        return SNMP_ERR_NOERROR;
    }

    while (request) {
        ourname = request->requestvb->name;
        ourlength = request->requestvb->name_length;

        snmp_pdu_add_variable(pdu, ourname, ourlength,
                              request->requestvb->type,
                              request->requestvb->val.string,
                              request->requestvb->val_len);
        //request->delegated = 0;
        request = request->next;
    }

    reqid = vlSnmp_cardMibAccessProxy_async_send(pdu, requests);
    
    struct timeval tm_start;
    VL_ZERO_STRUCT(tm_start);
    gettimeofday(&tm_start, NULL);
    struct timeval tm_now;
    VL_ZERO_STRUCT(tm_now);
    while(reqid)
    {
        int ret = 0;
        //cSemTake(vlg_semaphore, 100);
        struct timespec timeout;
        timeout.tv_sec=0;
        timeout.tv_nsec=100000000;
        while ((ret = sem_timedwait(vlg_semaphore, &timeout)) == -1 && (errno == EINTR) )
        continue; 
        		
        gettimeofday(&tm_now, NULL);
        netsnmp_pdu * pdu =  (netsnmp_pdu *) vlg_cardMibAccessProxy_response_map.getElementAt((vlMap::KEY)(reqid));
        if(NULL != pdu)
        {
            int ret =
                    vlSnmp_cardMibAccessProxy_handler_got_response(
                        NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE, reqinfo,
                        pdu, requests);
            if(ret < 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "vlSnmp_cardMibAccessProxy_handler: vlSnmp_cardMibAccessProxy_handler_got_response(): Failed ret = %d !\n", ret);
            }
            else
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s : Delivered Response %d at %u.%u\n", __FUNCTION__, pdu->reqid, (tm_now.tv_sec), (tm_now.tv_usec/1000));
            }
            vlg_cardMibAccessProxy_response_map.removeElementAt((vlMap::KEY)(reqid));
            break;
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlSnmp_cardMibAccessProxy_handler() : Sleeping...\n");
            rmf_osal_threadSleep(10,0);
            if(vlAbs(tm_now.tv_sec - tm_start.tv_sec) > 6)
            {
                vlSnmp_cardMibAccessProxy_handler_got_response(
                        NETSNMP_CALLBACK_OP_TIMED_OUT, reqinfo,
                        NULL, requests);
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s : Request %d timed out after %u secs\n", __FUNCTION__, reqid, vlAbs(tm_now.tv_sec - tm_start.tv_sec));
                system("../bin/target-snmp/sbin/restart_snmpd.sh");
                break;
            }
        }
    }
    
    return SNMP_ERR_NOERROR;
}

int
vlSnmp_cardMibAccessProxy_handler_got_response(int operation,
                                               netsnmp_agent_request_info *reqinfo,
                                               netsnmp_pdu *pdu, void *cb_data)
{
    VL_AUTO_LOCK(vlg_cardMibAccess_lock);   // for multi-thread protection
    
    netsnmp_request_info  *requests, *request = NULL;
    netsnmp_variable_list *vars,     *var     = NULL;
    
    requests = (netsnmp_request_info*) cb_data;

    //struct simple_proxy *sp;
    oid             myname[MAX_OID_LEN];
    size_t          myname_len = MAX_OID_LEN;

    //sp = (struct simple_proxy *) cache->localinfo;

    if (!vlg_CardMibAccessProxy_reg)
    {
        DEBUGMSGTL((__FUNCTION__, "a proxy request was no longer valid.\n"));
	 //auto_unlock_ptr(vlg_cardMibAccess_lock);
        return -2;
    }

    switch (operation)
    {
        case NETSNMP_CALLBACK_OP_TIMED_OUT:
            /*
            * WWWXXX: don't leave requests delayed if operation is
            * something like TIMEOUT 
            */
            DEBUGMSGTL((__FUNCTION__, "got timed out... requests = %08p\n", requests));

            netsnmp_handler_mark_requests_as_delegated(requests,
                    REQUEST_IS_NOT_DELEGATED);
            if(reqinfo->mode != MODE_GETNEXT)
            {
                DEBUGMSGTL((__FUNCTION__, "  ignoring timeout\n"));
                netsnmp_set_request_error(reqinfo, requests, /* XXXWWW: should be index = 0 */
                                          SNMP_ERR_GENERR);
            }
	     //auto_unlock_ptr(vlg_cardMibAccess_lock);
            return -3;

        case NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE:
        {
            if( pdu == NULL ) break;
            vars = pdu->variables;

            if (pdu->errstat != SNMP_ERR_NOERROR)
            {
                /*
                *  If we receive an error from the proxy agent, pass it on up.
                *  The higher-level processing seems to Do The Right Thing.
                *
                * 2005/06 rks: actually, it doesn't do the right thing for
                * a get-next request that returns NOSUCHNAME. If we do nothing,
                * it passes that error back to the comman initiator. What it should
                * do is ignore the error and move on to the next tree. To
                * accomplish that, all we need to do is clear the delegated flag.
                * Not sure if any other error codes need the same treatment. Left
                * as an exercise to the reader...
                */
                DEBUGMSGTL((__FUNCTION__, "got error response (%d)\n", pdu->errstat));
                
                if((reqinfo->mode == MODE_GETNEXT) &&
                    (SNMP_ERR_NOSUCHNAME == pdu->errstat))
                {
                    DEBUGMSGTL((__FUNCTION__, "  ignoring error response\n"));
                }
                else if ((reqinfo->mode == MODE_SET_ACTION))
                {
                    /*
                    * In order for netsnmp_wrap_up_request to consider the
                    * SET request complete,
                    * there must be no delegated requests pending.
                    * https://sourceforge.net/tracker/
                    *  ?func=detail&atid=112694&aid=1554261&group_id=12694
                    */
                    DEBUGMSGTL((__FUNCTION__,
                                "got SET error %s, index %d\n",
                                snmp_errstring(pdu->errstat), pdu->errindex));
                    netsnmp_request_set_error_idx(requests, pdu->errstat,
                            pdu->errindex);
                }
                else
                {
                    netsnmp_request_set_error_idx(requests, pdu->errstat,
                            pdu->errindex);
                }

                /*
                * update the original request varbinds with the results 
                */
            }
            else for (var = vars, request = requests;
            request && var;
            request = request->next, var = var->next_variable)
            {
                /*
                * XXX - should this be done here?
                *       Or wait until we know it's OK?
                */
                snmp_set_var_typed_value(request->requestvb, var->type,
                                         var->val.string, var->val_len);

                DEBUGMSGTL((__FUNCTION__, "got response... "));
                DEBUGMSGOID((__FUNCTION__, var->name, var->name_length));
                DEBUGMSG((__FUNCTION__, "\n"));
                request->delegated = 0;

                /*
                * Check the response oid is legitimate,
                *   and discard the value if not.
                *
                * XXX - what's the difference between these cases?
                */
                if (vlg_CardMibAccessProxy_reg->rootoid_len &&
                    (var->name_length < vlg_CardMibAccessProxy_reg->rootoid_len ||
                    snmp_oid_compare(var->name, vlg_CardMibAccessProxy_reg->rootoid_len, vlg_CardMibAccessProxy_reg->rootoid,
                                     vlg_CardMibAccessProxy_reg->rootoid_len) != 0))
                {
                    DEBUGMSGTL(( __FUNCTION__, "out of registered range... "));
                    DEBUGMSGOID((__FUNCTION__, var->name, vlg_CardMibAccessProxy_reg->rootoid_len));
                    DEBUGMSG((   __FUNCTION__, " (%d) != ", vlg_CardMibAccessProxy_reg->rootoid_len));
                    DEBUGMSGOID((__FUNCTION__, vlg_CardMibAccessProxy_reg->rootoid, vlg_CardMibAccessProxy_reg->rootoid_len));
                    DEBUGMSG((   __FUNCTION__, "\n"));
                    snmp_set_var_typed_value(request->requestvb, ASN_NULL, NULL, 0);

                    continue;
                }
                else if (!vlg_CardMibAccessProxy_reg->rootoid_len &&
                          (var->name_length < vlg_CardMibAccessProxy_reg->rootoid_len ||
                          snmp_oid_compare(var->name, vlg_CardMibAccessProxy_reg->rootoid_len, vlg_CardMibAccessProxy_reg->rootoid,
                                           vlg_CardMibAccessProxy_reg->rootoid_len) != 0))
                {
                    DEBUGMSGTL(( __FUNCTION__, "out of registered base range... "));
                    DEBUGMSGOID((__FUNCTION__, var->name, vlg_CardMibAccessProxy_reg->rootoid_len));
                    DEBUGMSG((   __FUNCTION__, " (%d) != ", vlg_CardMibAccessProxy_reg->rootoid_len));
                    DEBUGMSGOID((__FUNCTION__, vlg_CardMibAccessProxy_reg->rootoid, vlg_CardMibAccessProxy_reg->rootoid_len));
                    DEBUGMSG((   __FUNCTION__, "\n"));
                    snmp_set_var_typed_value(request->requestvb, ASN_NULL, NULL, 0);
                    continue;
                }
                else
                {
                    /*
                    * If the returned OID is legitimate, then update
                    *   the original request varbind accordingly.
                    */
                    if (vlg_CardMibAccessProxy_reg->rootoid_len)
                    {
                        /*
                        * XXX: oid size maxed? 
                        */
                        memcpy(myname, vlg_CardMibAccessProxy_reg->rootoid, sizeof(oid) * vlg_CardMibAccessProxy_reg->rootoid_len);
                        myname_len =
                                vlg_CardMibAccessProxy_reg->rootoid_len + var->name_length - vlg_CardMibAccessProxy_reg->rootoid_len;
                        if (myname_len > MAX_OID_LEN)
                        {
                            snmp_log(LOG_WARNING,
                                     "proxy OID return length too long.\n");
                            netsnmp_set_request_error(reqinfo, requests,
                                    SNMP_ERR_GENERR);
                            if (pdu)
                                snmp_free_pdu(pdu);
				//auto_unlock_ptr(vlg_cardMibAccess_lock);
                            return 1;
                        }

                        if (var->name_length > vlg_CardMibAccessProxy_reg->rootoid_len)
                        {
                            memcpy(&myname[vlg_CardMibAccessProxy_reg->rootoid_len],
                                    &var->name[vlg_CardMibAccessProxy_reg->rootoid_len],
                                    sizeof(oid) * (var->name_length -
                                            vlg_CardMibAccessProxy_reg->rootoid_len));
                            /*RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "My Name : ");
                            {
                                int i = 0;
                                for(i = 0; i < myname_len; i++)
                                {
                                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", ".%d", myname[i]);
                                }
                            }
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\n");*/
                            snmp_set_var_objid(request->requestvb, var->name,
                                               var->name_length);
                        }
                    }
                    else
                    {
                        snmp_set_var_objid(request->requestvb, var->name,
                                           var->name_length);
                    }
                }
            }

            if (request || var)
            {
                /*
                * ack, this is bad.  The # of varbinds don't match and
                * there is no way to fix the problem 
                */
                if (pdu)
                    snmp_free_pdu(pdu);
                snmp_log(LOG_ERR,
                         "response to proxy request illegal.  We're screwed.\n");
                netsnmp_set_request_error(reqinfo, requests,
                                          SNMP_ERR_GENERR);
            }

            /* fix bulk_to_next operations */
            if (reqinfo->mode == MODE_GETBULK)
                netsnmp_bulk_to_next_fix_requests(requests);
        
            /*
            * free the response 
            */
            if (pdu && 0)
                snmp_free_pdu(pdu);
            break;
    	}
        default:
            DEBUGMSGTL((__FUNCTION__, "no response received: op = %d\n",
                        operation));
            break;
    }
    //auto_unlock_ptr(vlg_cardMibAccess_lock);
    return 1;
}

void vlReturnCardMibAccessReply(unsigned char * pData, unsigned int nBytes)
{
    if((NULL != pData) && (nBytes > 0))
    {
        VL_BYTE_STREAM_INST(CardMibAccess_Response, ParseBuf, pData, nBytes);
        int eAsnTag    = vlParseByte(pParseBuf);
        if(ASN_OPAQUE_TAG2 != eAsnTag)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlReturnCardMibAccessReply: Received SNMP PDU tag %d: NOT_PROCESSING\n", eAsnTag);
            return; // not yet
        }

        int nRequestLength = vlParseCcApduLengthField(pParseBuf);
        nRequestLength += VL_BYTE_STREAM_USED(pParseBuf);
        eAsnTag    = vlParseByte(pParseBuf);
        if(ASN_INTEGER != eAsnTag)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlReturnCardMibAccessReply: Received version tag %d: NOT_PROCESSING\n", eAsnTag);
            return; // not yet
        }

        int nAsnLength = vlParseCcApduLengthField(pParseBuf);
        if(1 != nAsnLength)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlReturnCardMibAccessReply: Received version length %d: NOT_PROCESSING\n", nAsnLength);
            return; // not yet
        }

        int nVersion = vlParseByte(pParseBuf);
        if(nVersion > SNMP_VERSION_2c)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlReturnCardMibAccessReply: Received version %d: NOT_PROCESSING\n", nVersion);
            return; // not yet
        }

        eAsnTag    = vlParseByte(pParseBuf);
        if(ASN_OCTET_STR != eAsnTag)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlReturnCardMibAccessReply: Received communtity tag %d: NOT_PROCESSING\n", eAsnTag);
            return; // not yet
        }

        int nStrLength = vlParseCcApduLengthField(pParseBuf);
        vlParseBuffer(pParseBuf, NULL, nStrLength, 0);

        SNMP_DEBUGPRINT("vlReturnCardMibAccessReply : Received %d bytes of reply, returning to snmp daemon...\n", nBytes);
        netsnmp_pdu * pdu = snmp_pdu_create(SNMP_MSG_RESPONSE);
        
        if(NULL != pdu)
        {
            unsigned char * pdu_buf = VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf);
            size_t nRemainder       = VL_BYTE_STREAM_REMAINDER(pParseBuf);
            
            if(0 == snmp_pdu_parse(pdu, pdu_buf, &nRemainder))
            {
                vlg_cardMibAccessProxy_response_map.setElementAt((vlMap::KEY)(pdu->reqid), (vlMap::ELEMENT)(pdu));
                //cSemGive(vlg_semaphore);
                sem_post(vlg_semaphore );
            }
            else
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlReturnCardMibAccessReply: snmp_pdu_parse() failed.\n");
                snmp_free_pdu(pdu);
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlReturnCardMibAccessReply: snmp_pdu_create() failed.\n");
        }
    }
}
        
static char vlg_szOcStbHostCardRootOid[MAX_OID_LEN*4] = "";
static char vlg_tmpOcStbHostCardRootOid[MAX_OID_LEN*4] = "";
static int  vlg_nOcStbHostCardRootOidLength = 0;
static oid  vlg_ocStbHostCardRootOid[MAX_OID_LEN];
static int  vlg_ocStbHostCardRootOid_len = 0;

void vlStartupCardMibAccessProxy()
{
    if((NULL == vlg_CardMibAccessProxy_reg) && (vlg_ocStbHostCardRootOid_len > 0))
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "vlInitCardMibAccessProxy: Creating Handler for '%s' : len = %d\n", vlg_szOcStbHostCardRootOid, vlg_ocStbHostCardRootOid_len);
        
        vlg_CardMibAccessProxy_reg =
                netsnmp_create_handler_registration("CardMibAccessProxy",
                vlSnmp_cardMibAccessProxy_handler,
                vlg_ocStbHostCardRootOid,
                vlg_ocStbHostCardRootOid_len,
                HANDLER_CAN_RWRITE);
                
        if(0 == vlg_semaphore)
        {
            //vlg_semaphore = cSemCreate(256, 0);
            vlg_semaphore = new sem_t;
            sem_init( vlg_semaphore, 0 , 0);
        }
        
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "vlg_CardMibAccessProxy_reg = 0x%08p\n", vlg_CardMibAccessProxy_reg);
        int ret = netsnmp_register_handler(vlg_CardMibAccessProxy_reg);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "netsnmp_register_handler() retuned %d\n", ret);
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "vlg_CardMibAccessProxy_reg = %p, vlg_ocStbHostCardRootOid_len = %d\n", vlg_CardMibAccessProxy_reg, vlg_ocStbHostCardRootOid_len);
    }
}

void vlInitCardMibAccessProxy(char * pszRootOID, int nOidLen)
{
    VL_AUTO_LOCK(vlg_cardMibAccess_lock);   // for multi-thread protection

    if((NULL == pszRootOID) || (nOidLen <= 0)) 
	{
        //auto_unlock_ptr(vlg_cardMibAccess_lock);		
		return;
	}
    
    // parse the OID
    if(nOidLen >= sizeof(vlg_szOcStbHostCardRootOid)) nOidLen = (sizeof(vlg_szOcStbHostCardRootOid)-1);
    vlMemCpy(vlg_szOcStbHostCardRootOid, pszRootOID, nOidLen, sizeof(vlg_szOcStbHostCardRootOid));
    vlg_szOcStbHostCardRootOid[nOidLen] = '\0';
    vlg_nOcStbHostCardRootOidLength =  nOidLen;
    
    vlStrCpy(vlg_tmpOcStbHostCardRootOid, vlg_szOcStbHostCardRootOid, sizeof(vlg_tmpOcStbHostCardRootOid));
    
    {
        char * pszSavePtr = NULL;
        char * pszOid = strtok_r(vlg_tmpOcStbHostCardRootOid, ".", &pszSavePtr);
        //VL_ZERO_MEMORY(vlg_ocStbHostCardRootOid);
        memset(vlg_ocStbHostCardRootOid, 0, sizeof(vlg_ocStbHostCardRootOid));
        vlg_ocStbHostCardRootOid_len = 0;
        while(NULL != pszOid)
        {
            vlg_ocStbHostCardRootOid[vlg_ocStbHostCardRootOid_len] = atoi(pszOid);
            pszOid = strtok_r(NULL, ".", &pszSavePtr);
            vlg_ocStbHostCardRootOid_len++;
        }
    }
    
    int i = 0;
    vlg_tmpOcStbHostCardRootOid[0] = '\0';
    for(i = 0; i < vlg_ocStbHostCardRootOid_len; i++)
    {
        int iPos = strlen(vlg_tmpOcStbHostCardRootOid);
        snprintf(vlg_tmpOcStbHostCardRootOid+iPos, sizeof(vlg_tmpOcStbHostCardRootOid)-iPos, ".%d", vlg_ocStbHostCardRootOid[i]);
    }
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "Card Root OID : '%s'.\n", vlg_tmpOcStbHostCardRootOid);
     
    // check if cable-card proxying is allowed.
    if(STATUS_FALSE == gcardsnmpAccCtl)
    {
        // avoid registering cable-card proxy to agent
        //auto_unlock_ptr(vlg_cardMibAccess_lock);	
        return;
    }
    
    vlStartupCardMibAccessProxy();
    //auto_unlock_ptr(vlg_cardMibAccess_lock);	
}

void vlDisableCardMibAccessProxy()
{
    VL_AUTO_LOCK(vlg_cardMibAccess_lock);   // for multi-thread protection

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "vlShutdownCardMibAccessProxy() called\n");
    if(NULL != vlg_CardMibAccessProxy_reg)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "Calling netsnmp_unregister_handler()\n");
        netsnmp_unregister_handler(vlg_CardMibAccessProxy_reg);
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling netsnmp_handler_registration_free()\n");
        //netsnmp_handler_registration_free(vlg_CardMibAccessProxy_reg);
        vlg_CardMibAccessProxy_reg = NULL;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "vlShutdownCardMibAccessProxy() Exiting...\n");
    //auto_unlock_ptr(vlg_cardMibAccess_lock);	
}

void vlShutdownCardMibAccessProxy()
{
    VL_AUTO_LOCK(vlg_cardMibAccess_lock);   // for multi-thread protection
    //auto_lock_ptr(vlg_cardMibAccess_lock);   // for multi-thread protection
    
    vlg_ocStbHostCardRootOid_len = 0;
    vlDisableCardMibAccessProxy();
//    //auto_unlock_ptr(vlg_cardMibAccess_lock);	
}

#endif // VL_USE_SYNC_IMPL_FOR_CARD_MIB_ACCESS
