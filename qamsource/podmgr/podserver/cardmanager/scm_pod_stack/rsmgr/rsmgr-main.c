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


//#include <pthreadLibrary.h>
#include <pthread.h>
#include <global_event_map.h>
#include <global_queues.h>
#include "poddef.h"
#include <lcsm_log.h>
#include <memory.h>

#include "mrerrno.h"
#include "rsmgr-main.h"
#include "utils.h"
#include "link.h"
#include "transport.h"
#include "podhighapi.h"
#include "podlowapi.h"
#include "applitest.h"
#include <string.h>
#include "appmgr.h"
#include "core_events.h"
#include <rdk_debug.h>
#include <rmf_osal_thread.h>
#include "rmf_osal_util.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

#ifdef __cplusplus
extern "C" {
#endif


static LCSM_DEVICE_HANDLE   handle = 0;
static LCSM_TIMER_HANDLE    timer = 0;
static pthread_t            rmThread;
static rmControl_t          rmCtl = { RM_UNINITIALIZED, 0, };
static int                  openSessions = 0;
static int                  sessionNums[ MAX_OPEN_SESSIONS ] = { 0, };
static int                  numOfResources = 0;     // number supported
static Boolean              notifyPodofChanges = FALSE;
extern "C" void PostBootEvent(int eEventType, int nMsgCode, unsigned long ulData);
#if 0
    // POD resources supported by the MR Host
    // nb: this may need to be shared with App Mgr
/****   ALL SUPPORTED RESOURCES MUST BE LISTED FIRST   ****/
static VL_POD_RESOURCE vl_SPOD_Resource_List[ ] =
{  //                  max
   // resource ID   sessions  resource Q         name       supported   registered
        /* Resource Manager must be first... */
    { RESRC_MGR_ID,    32,    POD_RSMGR_QUEUE,   "RsMgr",   TRUE,       TRUE  },
        /* ...then, all other supported resources... */
    { MMI_ID,           1,    POD_MMI_QUEUE,     "MMI",     TRUE,       TRUE  },
    { SYS_TIME_ID,      1,    POD_SYSTIME_QUEUE, "Time",    TRUE,       TRUE  },
    { HOST_CTL_ID,      1,    POD_HOST_QUEUE,    "HostCtl", TRUE,       TRUE  },
    { CA_ID,            1,    POD_CA_QUEUE,      "CA",      TRUE,       TRUE  },

    /* ...with non-supported resources last */
    { APP_INFO_ID,      1,    POD_APPINFO_QUEUE, "AppInfo", TRUE,       TRUE  },
   { HOMING_ID,     1,    POD_HOMING_QUEUE,  "Homing",  TRUE,       TRUE  },
    { EXT_CHAN_ID,      1,    POD_XCHAN_QUEUE,   "ExtCh",   TRUE,       TRUE  },
    //{ CP_ID,            1,    POD_CPROT_QUEUE,   "CP",      TRUE,       TRUE},
     { SPEC_APP_ID,     32,    POD_SAS_QUEUE,     "SAS",     TRUE,       TRUE },
        //  comment these in and compute numOfResources
  { LOW_SPEED_ID,     1,    POD_LOWSPD_QUEUE,  "LowSpd",  TRUE,      TRUE },
//  { GEN_IPPV_ID,      1,    POD_IPPV_QUEUE,    "IPPV",    FALSE,      FALSE },
    { GEN_FEATURE_ID,   1,    POD_FEATURE_QUEUE, "Feature", TRUE,      TRUE },
     { SYSTEM_CONTROL_ID,   1,    POD_SYSTEM_QUEUE, "System", TRUE,      TRUE },
     { GEN_DIAG_ID,   1,    POD_DIAG_QUEUE, "Diag", TRUE,      TRUE }


};
#endif
static VL_POD_RESOURCE vl_MPOD_Resource_List[ ] =
{   //                  max
    // resource ID   sessions  resource Q         name       supported   registered
    /* Resource Manager must be first... */
    { M_RESRC_MGR_ID,    1,    POD_RSMGR_QUEUE,   "RsMgr",   TRUE,       TRUE  },//0
    { M_MMI_ID,           1,    POD_MMI_QUEUE,     "MMI",     TRUE,       TRUE  },//1
    { M_SYS_TIME_ID,      1,    POD_SYSTIME_QUEUE, "Time",    TRUE,       TRUE  },//2
    { M_HOST_CTL_ID,      1,    POD_HOST_QUEUE,    "HostCtl", TRUE,       TRUE  },//3
    { M_CA_ID,            1,    POD_CA_QUEUE,      "CA",      TRUE,       TRUE  },//4
    { M_CARD_RES_ID,      1,    POD_CARD_RES_QUEUE, "CardRes",      TRUE,       TRUE  },//5
    { M_CARD_MIB_ACC_ID,  1,    POD_CARD_MIB_ACC_QUEUE, "CardMibAc",      TRUE,       TRUE  },//6
#ifdef USE_DSG
    { M_DSG_ID,           1,    POD_DSG_QUEUE, "Dsg",      TRUE,       TRUE  },//7
#endif
    { M_APP_INFO_ID_V2,   1,    POD_APPINFO_QUEUE, "AppInfo_V2", TRUE,       TRUE  },//9
    { M_HOMING_ID,        1,    POD_HOMING_QUEUE,  "Homing",  TRUE,       TRUE  },//10
    { M_EXT_CHAN_ID_V6,   1,    POD_XCHAN_QUEUE,   "ExtCh-V6",   TRUE,       TRUE  },//12
     { M_CP_ID_V3,         1,    POD_CPROT_QUEUE,   "CP V3",      TRUE,       TRUE },//13
    { M_CP_ID_V4,         1,    POD_CPROT_QUEUE,   "CP V4",      TRUE,       TRUE },//14
    { M_SPEC_APP_ID,     32,    POD_SAS_QUEUE,     "SAS",     TRUE,       TRUE },//15
    { M_LOW_SPEED_ID,     1,    POD_LOWSPD_QUEUE,  "LowSpd",  TRUE,      TRUE },//16
    { M_GEN_FEATURE_ID,   1,    POD_FEATURE_QUEUE, "Feature", TRUE,      TRUE },//17
    { M_SYSTEM_CONTROL_T1_ID,   1,    POD_SYSTEM_QUEUE, "System T1", TRUE,      TRUE },//18
    { M_SYSTEM_CONTROL_T2_ID,   1,    POD_SYSTEM_QUEUE, "System T2", TRUE,      TRUE },//19
    { M_GEN_DIAG_ID,      1,    POD_DIAG_QUEUE, "Diag", TRUE,      TRUE },//20
    { M_HOST_ADD_PRT,     1,    POD_HOST_ADD_PRT_QUEUE, "HostAppPrt", TRUE,      TRUE },//21
    { M_HEAD_END_COMM,    1,    POD_HEAD_END_COMM_QUEUE, "Headend Comm", TRUE,      TRUE },//22
#ifdef USE_IPDM
    { M_IPDM_ID,          1,    POD_IPDM_COMM_QUEUE, "IpDm"    , TRUE,      TRUE },//23 // IP Direct Multicast
#endif // USE_IPDM


    //  { M_GEN_IPPV_ID,      1,    POD_IPPV_QUEUE,    "IPPV",    FALSE,      FALSE },

};

int bIPDirectUnicast = 0;
static VL_POD_RESOURCE vl_MPOD_Resource_List_IPD[ ] =
{   //                  max
    // resource ID   sessions  resource Q         name       supported   registered
    /* Resource Manager must be first... */
    { M_RESRC_MGR_ID,    1,    POD_RSMGR_QUEUE,   "RsMgr",   TRUE,       TRUE  },//0
    { M_MMI_ID,           1,    POD_MMI_QUEUE,     "MMI",     TRUE,       TRUE  },//1
    { M_SYS_TIME_ID,      1,    POD_SYSTIME_QUEUE, "Time",    TRUE,       TRUE  },//2
    { M_HOST_CTL_ID,      1,    POD_HOST_QUEUE,    "HostCtl", TRUE,       TRUE  },//3
    { M_CA_ID,            1,    POD_CA_QUEUE,      "CA",      TRUE,       TRUE  },//4
    { M_CARD_RES_ID,      1,    POD_CARD_RES_QUEUE, "CardRes",      TRUE,       TRUE  },//5
    { M_CARD_MIB_ACC_ID,  1,    POD_CARD_MIB_ACC_QUEUE, "CardMibAc",      TRUE,       TRUE  },//6
#ifdef USE_DSG
    { M_DSG_ID,           1,    POD_DSG_QUEUE, "Dsg",      TRUE,       TRUE  },//7
#endif
    { M_APP_INFO_ID,      1,    POD_APPINFO_QUEUE, "AppInfo_V2", TRUE,       TRUE  },//9
    { M_HOMING_ID,        1,    POD_HOMING_QUEUE,  "Homing",  TRUE,       TRUE  },//10
    { M_EXT_CHAN_ID,      1,    POD_XCHAN_QUEUE,   "ExtCh-V6",   TRUE,       TRUE  },//12
    { M_CP_ID_V3,         1,    POD_CPROT_QUEUE,   "CP V3",      TRUE,       TRUE },//13
    { M_CP_ID_V4,         1,    POD_CPROT_QUEUE,   "CP V4",      TRUE,       TRUE },//14
    { M_SPEC_APP_ID,     32,    POD_SAS_QUEUE,     "SAS",     TRUE,       TRUE },//15
    { M_LOW_SPEED_ID,     1,    POD_LOWSPD_QUEUE,  "LowSpd",  TRUE,      TRUE },//16
    { M_GEN_FEATURE_ID,   1,    POD_FEATURE_QUEUE, "Feature", TRUE,      TRUE },//17
    { M_SYSTEM_CONTROL_T1_ID,   1,    POD_SYSTEM_QUEUE, "System T1", TRUE,      TRUE },//18
    { M_SYSTEM_CONTROL_T2_ID,   1,    POD_SYSTEM_QUEUE, "System T2", TRUE,      TRUE },//19
    { M_GEN_DIAG_ID,      1,    POD_DIAG_QUEUE, "Diag", TRUE,      TRUE },//20
    { M_HOST_ADD_PRT,     1,    POD_HOST_ADD_PRT_QUEUE, "HostAppPrt", TRUE,      TRUE },//21
    { M_HEAD_END_COMM,    1,    POD_HEAD_END_COMM_QUEUE, "Headend Comm", TRUE,      TRUE },//22
#ifdef USE_IPDM
    { M_IPDM_ID,          1,    POD_IPDM_COMM_QUEUE, "IpDm"    , TRUE,      TRUE },//23 // IP Direct Multicast
#endif // USE_IPDM

#ifdef USE_IPDIRECT_UNICAST
    { M_IPDIRECT_UNICAST_ID, 1,    POD_IPDIRECT_UNICAST_COMM_QUEUE, "IpDir Unicast", TRUE,      TRUE },//24 // IP Direct Unicast
#endif // USE_IPDIRECT_UNICAST

    //  { M_GEN_IPPV_ID,      1,    POD_IPPV_QUEUE,    "IPPV",    FALSE,      FALSE },

};

unsigned long  RmGetGenFetParamResId()
{
    return M_GEN_FEATURE_ID;
}

unsigned long RM_Get_ResourceId(unsigned long Resource)
{
    int nResources,ii;
    VL_POD_RESOURCE *pMPod_Resrc_List = NULL;
       
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","bIPDirectUnicast set as %d %s \n",bIPDirectUnicast, __FUNCTION__);

    if(vl_gCardType == POD_MODE_MCARD)
    {
       if (bIPDirectUnicast)
       {
         pMPod_Resrc_List = (VL_POD_RESOURCE*)&vl_MPOD_Resource_List_IPD[0];
         nResources = (  ( sizeof( vl_MPOD_Resource_List_IPD) / sizeof( VL_POD_RESOURCE ) ));
       }
       else
       {
        pMPod_Resrc_List = (VL_POD_RESOURCE*)&vl_MPOD_Resource_List[0];
        nResources = (  ( sizeof( vl_MPOD_Resource_List ) / sizeof( VL_POD_RESOURCE ) ));
       }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n>>>>>>>>>>>>>>>>> M-CARD Resouce List Size details<<<<<<<<<<<<<<<<< \n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",">>>>>>>>>>>>>>>>> M-CARD Resouce List Size details<<<<<<<<<<<<<<<<< \n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," sizeof( vl_MPOD_Resource_List ):%d sizeof( VL_POD_RESOURCE ):%d nResources:%d\n",
        sizeof( vl_MPOD_Resource_List ),sizeof( VL_POD_RESOURCE ),nResources);
    }
#if 0
    else
    {
        pMPod_Resrc_List = (VL_POD_RESOURCE*)&vl_SPOD_Resource_List[0];
        nResources = (  ( sizeof( vl_SPOD_Resource_List ) / sizeof( VL_POD_RESOURCE ) ));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n>>>>>>>>>>>>>>>>> S-CARD Resouce List Size details<<<<<<<<<<<<<<<<< \n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>>>>>>>>>> S-CARD Resouce List Size details<<<<<<<<<<<<<<<<< \n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," sizeof( vl_MPOD_Resource_List ):%d sizeof( VL_POD_RESOURCE ):%d nResources:%d\n",
        sizeof( vl_SPOD_Resource_List ),sizeof( VL_POD_RESOURCE ),nResources);
    }
#endif

    if(pMPod_Resrc_List == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","RM_Get_ResourceId: Error Getting the Resource Id. Resource List is NULL \n");
        return 0xFFFFFFFF;
    }

    for(ii = 0 ; ii < nResources; ii++)
    {
        if(((pMPod_Resrc_List->resId & 0xFFFF0000) >> 16) == Resource)
            break;
        pMPod_Resrc_List++;
    }

    if(ii >= nResources)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","RM_Get_ResourceId: Error Getting the Resource Id \n");
        return 0xFFFFFFFF;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","RM_Get_ResourceId: Returning ResId:0x%08X \n",pMPod_Resrc_List->resId);
    return pMPod_Resrc_List->resId;
}
/***  "services" offered to App Mgr & other Host resources  ***/

unsigned long RM_Get_Class_Type( unsigned long id )
{
    // *not* right-adjusted
    return ( id & ( RESOURCE_CLASS_MASK | RESOURCE_TYPE_MASK ) );
}

unsigned long RM_Get_Ver( unsigned long id )
{
    // *not* right-adjusted
    return ( id & RESOURCE_VERSION_MASK );
}

unsigned long RM_Is_Private_ReSrc( unsigned long id )
{
    // only private if both ID type bits are set
    return ( ( ( id & RESOURCE_ID_TYPE_MASK ) == RESOURCE_ID_TYPE_MASK ) ? 1 : 0 );
}

VL_POD_RESOURCE * RM_Find_Resrc( unsigned long id )
{
    int ii;
    VL_POD_RESOURCE *pResource = NULL;
    VL_POD_RESOURCE *pResource_List = NULL;// = &vl_SPOD_Resource_List[0];

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","bIPDirectUnicast set as %d %s \n",bIPDirectUnicast, __FUNCTION__);

    if(vl_gCardType == POD_MODE_MCARD)
      {
       if (bIPDirectUnicast)
        {
          pResource_List = &vl_MPOD_Resource_List_IPD[0];
        }
        else
        {
         pResource_List = &vl_MPOD_Resource_List[0];
        }
      }
#if 0
    else
        pResource_List = &vl_SPOD_Resource_List[0];
#endif
/*
    for( ii = 0; ii < numOfResources; ii++ )
    {
        // NB: doesn't consider ID type or version!
        if( RM_Get_Class_Type( id ) == RM_Get_Class_Type( vl_SPOD_Resource_List[ ii ].resId ) )
        {
            pResource = &vl_SPOD_Resource_List[ ii ];
            break;
        }
    }
*/

    if(pResource_List == NULL)
        return NULL;

    for( ii = 0; ii < numOfResources; ii++ )
    {
        // NB: doesn't consider ID type or version!
        if( RM_Get_Class_Type( id ) == RM_Get_Class_Type( pResource_List[ ii ].resId ) )
        {
            pResource = &pResource_List[ ii ];
            break;
        }
    }

    return pResource;
}


int32 RM_Get_Queue( unsigned long id )
{
    VL_POD_RESOURCE * pResource;
    int32                 queue = -1;  // valid queue numbers range from 0..n

    if( ( pResource = RM_Find_Resrc( id ) ) != NULL )
        queue = pResource->resQ;

    MDEBUG (DPM_RSMGR, "id=0x%x, queue=%d\n", id, queue );
    return queue;
}

        /***  thread control functionality  ***/

static inline void setLoop( int cont )
{
    // NB: the thread soon dies after loop is set to 0!
    pthread_mutex_lock( &( rmCtl.lock ) );
    if( ! cont )
        rmCtl.loop = 0;
    else
        rmCtl.loop = 1;
    pthread_mutex_unlock( &( rmCtl.lock ) );
}

static int setState( rmState_t newState )
{
    int status = EXIT_FAILURE;

    pthread_mutex_lock( &( rmCtl.lock ) );

    switch( rmCtl.state )
    {
        case RM_UNINITIALIZED:
            break;

        case RM_INITIALIZED:
            if( newState == RM_STARTED )
            {
                rmCtl.state = newState;
                status = EXIT_SUCCESS;
            }
            break;

        case RM_STARTED:
            if( newState == RM_SHUTDOWN )
            {
                rmCtl.state = newState;
                status = EXIT_SUCCESS;
            }
            break;

        default:
            MDEBUG (DPM_ERROR, "ERROR: unknown new state=%d\n", newState );
    }
    pthread_mutex_unlock( &( rmCtl.lock ) );
    return status;
}

// debugging aid - dump APDU prior to sending
static void dumpApdu( UCHAR * pApdu, USHORT len )
{
    int ii, jj;

    MDEBUG (DPM_RSMGR, "     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n" );
    for( ii = 0; ii < len; ii += 0x10 )
    {
        MDEBUG2 (DPM_RSMGR, "%03x", ii );
        for( jj = 0; jj < 0x10  &&  ii+jj < len; jj++ )
            MDEBUG2 (DPM_RSMGR, " %02x", pApdu[ ii+jj ] );
        MDEBUG2 (DPM_RSMGR, "\n");
    }
}

static void RM_APDU_FROM_HOST( USHORT sessNbr, UCHAR * pApdu, USHORT len )
{
//  MDEBUG (DPM_RSMGR, "sessNbr=%d, len=%d\n", sessNbr, len );
//  dumpApdu( pApdu, len );
    AM_APDU_FROM_HOST( sessNbr, pApdu, len );
}

        /***  event handlers  ***/

// fwd refs
static void sendProfInqApdu( USHORT sessNbr );
static void sendProfReplyApdu( USHORT sessNbr );
static void sendProfChangeApdu( USHORT sessNbr );

static void evResourcePresent( ULONG resId, int32 queueId, int maxSessions )
{
    VL_POD_RESOURCE *pResource;
    MDEBUG (DPM_RSMGR, "resId=0x%x queueId=%d maxSessions=%d\n",
            resId, queueId, maxSessions );

    if( ( pResource = RM_Find_Resrc( resId ) ) == NULL)
    {
        MDEBUG (DPM_ERROR, "ERROR: unknown resId=0x%x present\n", resId );
    }
    else
    {
        pResource->resId       = resId;
        pResource->resQ        = queueId;
        pResource->maxSessions = maxSessions;
        pResource->supported   = TRUE;
        pResource->registered  = TRUE;
        MDEBUG (DPM_RSMGR, "'%s' registered\n", pResource->resName );
    }
    // if a resource registers AFTER we've responded to the initial
    // Profile_Inquiry request from POD,
    // then notify the POD that our profile has changed.
    // It will then send another Profile_Inquiry request to which
    // we can inform it of the new set of resources
    if( notifyPodofChanges )
    {
        int ii;
        int sent = 0;

        for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++ )
        {
            if( sessionNums[ ii ] != 0 )
            {
                sendProfChangeApdu( sessionNums[ ii ] );
                if( ++sent >= openSessions )
                    break;
            }
        }
    }
}

static void evOpenSessionReq( ULONG resId, UCHAR tcId )
{
    UCHAR status = 0;
    MDEBUG (DPM_RSMGR, "resId=0x%x tcId=%d openSessions=%d\n",
            resId, tcId, openSessions );

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","resId=0x%08X tcId=%d openSessions=%d\n",
            resId, tcId, openSessions );

     if( openSessions < MAX_OPEN_SESSIONS )
    {
    /*if( transId != tcId )
        {
            status = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_ERROR, "ERROR: non-existent resource, transId in OPEN_SS=%d, exp=%d\n", tcId, transId );
        } else */
    /*if( RM_Get_Ver( RESRC_MGR_ID ) < RM_Get_Ver( resId ) )
        {
            status = 0xF2;  // this version < requested
            MDEBUG (DPM_ERROR, "ERROR: bad version=0x%x < req=0x%x\n",
                    RM_Get_Ver( RESRC_MGR_ID ), RM_Get_Ver( resId ) );
        } else*/
        {
            MDEBUG (DPM_RSMGR, "RM session %d opened\n", openSessions+1 );
        }
    }
    else
    {
        status = 0xF3;      // exists but busy
        MDEBUG (DPM_ERROR, "ERROR: max sessions opened=%d\n", openSessions );
    }

    AM_OPEN_SS_RSP( status, resId, transId );
}

static void evSessionOpened( USHORT sessNbr, ULONG resId, UCHAR tcId )
{
    int ii;
    MDEBUG (DPM_RSMGR, "sessNbr=%d resId=0x%x tcId=%d\n",
            sessNbr, resId, tcId );


    for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++ )
    {
        if( sessionNums[ ii ] == 0 )
        {
            //  might want to save resId and/or tcId, too
            sessionNums[ ii ] = sessNbr;
            openSessions++;

            PODSleep (4);        // we just sent an AM_OPEN_SS_RSP apdu to the POD, so wait a while
            sendProfInqApdu( sessNbr );
            break;
        }
    }
}

static void evCloseSession( USHORT sessNbr )
{
    int ii;
    // decrement but make sure it's not negative
    for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++ )
    {
        if( sessionNums[ ii ] == sessNbr )
        {
            sessionNums[ ii ] = 0;
            openSessions = ( --openSessions < 0 )  ?  0 : openSessions;
            break;
        }
    }

    MDEBUG (DPM_RSMGR, "sessNbr=%d openSessions=%d\n",
            sessNbr, openSessions );

    // flush the queue when no more open sessions remain
   // if( openSessions == 0 )
     //   ( handle, POD_RSMGR_QUEUE );
}

static void sendProfInqApdu( USHORT sessNbr )
{
    USHORT len;
    uint8  apdu[ MAX_APDU_HEADER_BYTES ];

    buildApdu( apdu, &len, PROFILE_INQ_TAG, 0, NULL );
    PODSleep(100);    // 100 msec wait. This is necessary for the HPNX to not lose the next APDU (?)
    RM_APDU_FROM_HOST( sessNbr, apdu, len );
    // start timer in order to detect POD response time-out
    LCSM_EVENT_INFO msg;
    memset( &msg, 0, sizeof( msg ) );

    msg.event = POD_TIMER_EXPIRES;
    timer = podSetTimer( POD_RSMGR_QUEUE, &msg, 5000 );
}
static int vlg_nResources = 0;
void vlSetResourceList(unsigned long *pList, unsigned long size);
static void sendProfReplyApdu( USHORT sessNbr )
{
    int     ii;
    int     res = 0;
    USHORT  len;
    UCHAR * data;//,*apdu=NULL;
    VL_POD_RESOURCE *pResource_List = NULL;
    unsigned long ResourceList[32] = {0};
   // int VL_NUM_POD_RESOURCES  =  sizeof( vl_MPOD_Resource_List ) / sizeof( VL_POD_RESOURCE ) ;
    UCHAR   apdu[ MAX_APDU_HEADER_BYTES + ( /*VL_NUM_POD_RESOURCES*/vlg_nResources * 4 ) ];

    if(vl_gCardType == POD_MODE_MCARD)
    {
    // apdu = (UCHAR*)vlMalloc(sizeof( vl_MPOD_Resource_List ) / sizeof( VL_POD_RESOURCE));
    }
    else
    {
     //apdu = (UCHAR*)vlMalloc(sizeof( vl_SPOD_Resource_List ) / sizeof( VL_POD_RESOURCE));
    }

    data = buildApdu( apdu, &len, PROFILE_REPLY_TAG, ( 4 * numOfResources ), NULL );
    if(data == NULL)
    {
        MDEBUG (DPM_ERROR, "sendProfReplyApdu: ERROR: Unable to build apdu.\n");
        return;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","sendProfReplyApdu len = %x\n",len);
/*
    // copy the resource list
    for( ii = 0; ii < numOfResources; ii++ )
    {
        if( vl_SPOD_Resource_List[ ii ].registered )
        {
            *data++ = ( ( vl_SPOD_Resource_List[ ii ].resId >> 24 ) & 0xFF );
            *data++ = ( ( vl_SPOD_Resource_List[ ii ].resId >> 16 ) & 0xFF );
            *data++ = ( ( vl_SPOD_Resource_List[ ii ].resId >>  8 ) & 0xFF );
            *data++ =   ( vl_SPOD_Resource_List[ ii ].resId         & 0xFF);
            res++;

        }
        else
        {
            len-=4;
        }
        MDEBUG (DPM_RSMGR, "reported %d resources available to POD\n", res);
    }
*/

    if(vl_gCardType == POD_MODE_MCARD)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," M-Card Resources \n");
       if (bIPDirectUnicast)
        {
         pResource_List = &vl_MPOD_Resource_List_IPD[0];
        }
        else
        {
         pResource_List = &vl_MPOD_Resource_List[0];
        }
    }
#if 0
    else
        {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," S-Card Resources \n");
        pResource_List = &vl_SPOD_Resource_List[0];
    }
#endif

    if(pResource_List == NULL)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," sendProfReplyApdu: Resource List is Empty \n");
        return;
    }

    // copy the resource list
    for( ii = 0; ii < numOfResources; ii++ )
    {
        if( pResource_List[ ii ].registered )
        {
            int *pData;
            pData = (int *)data;
            *data++ = ( ( pResource_List[ ii ].resId >> 24 ) & 0xFF );
            *data++ = ( ( pResource_List[ ii ].resId >> 16 ) & 0xFF );
            *data++ = ( ( pResource_List[ ii ].resId >>  8 ) & 0xFF );
            *data++ =   ( pResource_List[ ii ].resId         & 0xFF);
            res++;
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ############### Resource Data :0x%08X %s \n",pResource_List[ ii ].resId,pResource_List[ii].resName);
            ResourceList[ii] = pResource_List[ ii ].resId;
        }
        else
        {
            len-=4;
        }
        MDEBUG (DPM_RSMGR, "reported %d resources available to POD\n", res);
    }

    vlSetResourceList(ResourceList, numOfResources);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," sessNbr:%d :%X \n",sessNbr,sessNbr);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","sendProfReplyApdu len = %x\n",len);
    RM_APDU_FROM_HOST( sessNbr, apdu, len );
    notifyPodofChanges = TRUE;
}

static void sendProfChangeApdu( USHORT sessNbr )
{
    USHORT len;
    uint8  apdu[ MAX_APDU_HEADER_BYTES ];

    buildApdu( apdu, &len, PROFILE_CHANGED_TAG, 0, NULL );
    RM_APDU_FROM_HOST( sessNbr, apdu, len );
}

static void evApdu( USHORT sessNbr, UCHAR * pApdu, USHORT len )
{
    MDEBUG (DPM_RSMGR, "RM_APDU_FROM_POD: sessNbr=%d len=%d\n", sessNbr, len );
    dumpApdu( pApdu, len );

    // 1st long int is 24-bit tag + length_field()
//    MDEBUG (DPM_RSMGR, "pApdu[0]=0x%x shift left by 16=0x%x\n",
//            pApdu[ 0 ], ( (uint32) pApdu[ 0 ]) << 16 );
    uint32 tag = ( ( (uint32) pApdu[ 0 ] ) << 16 )  |
                 ( ( (uint32) pApdu[ 1 ] ) <<  8 )  |
                   ( (uint32) pApdu[ 2 ] );

    switch( tag )
    {
        case PROFILE_INQ_TAG:
            MDEBUG (DPM_RSMGR, "RM: Profile Inquiry received\n" );
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","PROFILE_INQ_TAG: Before waiting for 1 sec \n");
        rmf_osal_threadSleep(1000, 0);  
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","PROFILE_INQ_TAG: After waiting for 1 sec \n");
            sendProfReplyApdu( (USHORT) sessNbr );
            break;
        case PROFILE_CHANGED_TAG:
            MDEBUG (DPM_RSMGR, "RM: Profile Changed received\n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","PROFILE_CHANGED_TAG: Before waiting for 1 sec \n");
        rmf_osal_threadSleep(1000, 0);  
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","PROFILE_CHANGED_TAG: After waiting for 1 sec \n");
            sendProfInqApdu( (USHORT) sessNbr );
            break;
        case PROFILE_REPLY_TAG:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," >>>>>>>>>>> calling podCancelTimer >>>>>>>>> \n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","PROFILE_REPLY_TAG: Before waiting for 1 sec \n");
        rmf_osal_threadSleep(1000, 0);  
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","PROFILE_REPLY_TAG: After waiting for 1 sec \n");
            podCancelTimer( timer );    // kill time-out detection, if any
            MDEBUG (DPM_RSMGR, "RM: Profile Reply received\n" );
            if( len != 0 )
                MDEBUG (DPM_RSMGR, "RM: Profile Reply not empty\n");
            sendProfChangeApdu( (USHORT) sessNbr );
            break;
        default:
            MDEBUG (DPM_ERROR, "ERROR: Invalid APDU tag=0x%x\n", tag );
            return;
    }
//            rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pApdu );  //  check how POD stack uses mem
}


void rsmgr_init(void){
    int                   ii,VL_NUM_POD_RESOURCES;
    VL_POD_RESOURCE * pResource = NULL;
// determine which resources in the table are implemented...
    numOfResources = 0;     // supports restarting
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n\n\n>>>>>>>>>>>  rsmgr_init: Entered <<<<<<<<<<<<<<<<<\n");
    //setLoop( 1 );
    setState( RM_STARTED );
    const char *envVar;
    envVar = rmf_osal_envGet("USE_IPDIRECT_UNICAST");
    if (NULL != envVar)
    {
        bIPDirectUnicast = (strcasecmp(envVar, "TRUE") == 0);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","bIPDirectUnicast DEFINED %d %s \n",bIPDirectUnicast, __FUNCTION__);
    }
   else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "bIPDirectUnicast NOT DEFINED %s \n", __FUNCTION__);
    }

    if (access( "/opt/persistent/ipduEnable", F_OK ) != -1 )
    {
        bIPDirectUnicast = TRUE;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","bIPDirectUnicast DEFINED via ipduEnable file %d %s \n",bIPDirectUnicast, __FUNCTION__);
    }
#if 0
    if(vl_gCardType == POD_MODE_MCARD)
    {
    pResource = &vl_MPOD_Resource_List[ 0 ];
    VL_NUM_POD_RESOURCES = ( sizeof( vl_MPOD_Resource_List ) / sizeof( VL_POD_RESOURCE ) );
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," rsmgr_init:M_CARD VL_NUM_POD_RESOURCES:%d sizeof( vl_MPOD_Resource_List ):%d sizeof( VL_POD_RESOURCE ):%d \n",VL_NUM_POD_RESOURCES,sizeof( vl_MPOD_Resource_List ), sizeof( VL_POD_RESOURCE ));
    }
    else
    {
    pResource = &vl_SPOD_Resource_List[ 0 ];
    VL_NUM_POD_RESOURCES = ( sizeof( vl_SPOD_Resource_List ) / sizeof( VL_POD_RESOURCE ) );
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","rsmgr_init:S_CARD VL_NUM_POD_RESOURCES:%d sizeof( vl_MPOD_Resource_List ):%d sizeof( VL_POD_RESOURCE ):%d \n",VL_NUM_POD_RESOURCES,sizeof( vl_SPOD_Resource_List ), sizeof( VL_POD_RESOURCE ));
    }
    for( ii = 0;  ii < VL_NUM_POD_RESOURCES; ii++ )
    {
        if( pResource->supported )
            numOfResources++;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","rsmgr_init::numOfResources:%d \n\n\n",numOfResources);
#endif

}

void rsource_init(void)
{
    int ii, VL_NUM_POD_RESOURCES;
    VL_POD_RESOURCE * pResource = NULL;
    // determine which resources in the table are implemented...
    numOfResources = 0;     // supports restarting
   
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","bIPDirectUnicast set as %d %s \n",bIPDirectUnicast, __FUNCTION__); 

    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n\n\n>>>>>>>>>>>  rsmgr_init: Entered <<<<<<<<<<<<<<<<<\n");
    //setLoop( 1 );
    //setState( RM_STARTED );
    if(vl_gCardType == POD_MODE_MCARD)
    {
       if (bIPDirectUnicast)
        {/*advertise IPDirect if /etc/rmfconfig.ini has USE_IPDIRECT TRUE*/
         pResource = &vl_MPOD_Resource_List_IPD[ 0 ];
         VL_NUM_POD_RESOURCES = ( sizeof( vl_MPOD_Resource_List_IPD ) / sizeof( VL_POD_RESOURCE ) );
        }
        else
        {
         pResource = &vl_MPOD_Resource_List[ 0 ];
         VL_NUM_POD_RESOURCES = ( sizeof( vl_MPOD_Resource_List ) / sizeof( VL_POD_RESOURCE ) );
        }
        vlg_nResources = VL_NUM_POD_RESOURCES;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," rsmgr_init:M_CARD VL_NUM_POD_RESOURCES:%d sizeof( vl_MPOD_Resource_List ):%d sizeof( VL_POD_RESOURCE ):%d \n",VL_NUM_POD_RESOURCES,sizeof( vl_MPOD_Resource_List ), sizeof( VL_POD_RESOURCE ));
    }
#if 0
    else
    {
        pResource = &vl_SPOD_Resource_List[ 0 ];
        VL_NUM_POD_RESOURCES = ( sizeof( vl_SPOD_Resource_List ) / sizeof( VL_POD_RESOURCE ) );
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","rsmgr_init:S_CARD VL_NUM_POD_RESOURCES:%d sizeof( vl_MPOD_Resource_List ):%d sizeof( VL_POD_RESOURCE ):%d \n",VL_NUM_POD_RESOURCES,sizeof( vl_SPOD_Resource_List ), sizeof( VL_POD_RESOURCE ));
    }
#endif

    if(pResource == NULL)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," rsmgr_init: Resource List is Empty \n");
        return;
    }

    for( ii = 0;  ii < VL_NUM_POD_RESOURCES; ii++ )
    {
        if( pResource->supported )
            numOfResources++;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","rsmgr_init::numOfResources:%d \n\n\n",numOfResources);
}

// the resource manager thread control
void rmProc( void * par )
{
   // int                   ii;
    LCSM_EVENT_INFO       *msg = (LCSM_EVENT_INFO       *)par;
    VL_POD_RESOURCE * pResource = NULL;

   // setLoop( 1 ); setState( RM_STARTED );

   // MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );

    if (msg == NULL)
        return;

    // determine which resources in the table are implemented...
/*    numOfResources = 0;     // supports restarting
    for( ii = 0, pResource = &vl_SPOD_Resource_List[ 0 ]; ii < VL_NUM_POD_RESOURCES; ii++ )
    {
        if( pResource->supported )
            numOfResources++;
    }
*/
    //while( rmCtl.loop ) Hannah
    {
       // lcsm_get_event( handle, POD_RSMGR_QUEUE, &msg, LCSM_WAIT_FOREVER );   Hannah
//        MDEBUG (DPM_RSMGR, "event received\n" );
        switch( msg->event )
        {
            // host resource registers its existence (x=resId, y=queueId, z=maxSessions)
            case POD_RESOURCE_PRESENT:

                evResourcePresent( (ULONG) msg->x, (int32) msg->y, (int) msg->z );
                break;
            // POD requests open session (x=resId, y=tcId)
            case POD_OPEN_SESSION_REQ:
                //rsource_init();
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","rmProc: POD_OPEN_SESSION_REQ Before Sleeping for 1 sec \n");
                
                rmf_osal_threadSleep(1000, 0);  
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","rmProc: POD_OPEN_SESSION_REQ After Sleeping for 1 sec \n");
                evOpenSessionReq( (ULONG) msg->x, (UCHAR) msg->y );
                break;
            // POD confirms open session (x=sessNbr, y=resId, z=tcId)
            case POD_SESSION_OPENED:
                evSessionOpened( (USHORT) msg->x, (ULONG) msg->y, (UCHAR) msg->z );
                PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_CA, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
                break;
            // POD closes session (x=sessNbr)
            case POD_CLOSE_SESSION:
                evCloseSession( (USHORT) msg->x );
                break;
            // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
            case POD_APDU:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," >>>>>>>>>> POD_APDU >>>>>>>>>> \n");
                evApdu( (USHORT) msg->x, (UCHAR *) msg->y, (USHORT) msg->z );
                break;
            case POD_TIMER_EXPIRES:
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," Reporting Error to reportPodError \n");
                vlReportPodError( PODERR_PROF_TIMEOUT );
                PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 17);
                break;
            // we've been commanded to bail
            // (rmCtl.loop will be cleared by now)
            case EXIT_REQUEST:
                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n\n >>>>>>>>>>>>>>>>> EXIT_REQUEST <<<<<<<<<<<<<<<<, \n\n");
                rsource_init();
                break;

            default:
                MDEBUG (DPM_ERROR, "invalid RM msg received=0x%x\n", msg->event );
        }
    }
    //pthread_exit( NULL ); Hannah
}

    /***  sub-module control  ***/

static inline int rm_init( )
{
    rmCtl.state = RM_INITIALIZED;
    rmCtl.loop = 0;
    if( pthread_mutex_init( &( rmCtl.lock ), NULL ) != 0 )
    {
        MDEBUG (DPM_ERROR, "Cant initialize state mutex\n" );
        return EXIT_FAILURE;
    }
    notifyPodofChanges = FALSE;     // don't send Profile_Change to POD
    return EXIT_SUCCESS;
}

// Sub-module entry point
int rsmgr_start( LCSM_DEVICE_HANDLE h )
{
    int res;

    if( h < 1 )
    {
        MDEBUG (DPM_ERROR, "Invalid LCSM device handle=%d\n", h );
        // This failure should be logged by the application manager
        return -EINVAL;
    }

    // Assign handle
    handle = h;
    // Flush queue
    //lcsm_flush_queue( handle, POD_RSMGR_QUEUE ); Hannah

    // Initialize RM's control environment
    rm_init();

    /*
     * Start RSMGR processing thread
     * For now just use default settings for the new thread. It's more important
     * to be able to check the return value that would otherwise be swallowed by
     * the pthread wrapper library call
     */
//    res = (int) pthread_createThread((void *)&rmProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create( &rmThread, NULL, (void *)&rmProcThread, NULL );
  //  if( res == 0 )
   // {
   //     MDEBUG (DPM_ERROR, "ERROR: Unable to create thread=%d\n", res );
   //     return EXIT_FAILURE;
  //  }
    return EXIT_SUCCESS;
}

// Sub-module exit point
int rsmgr_stop()
{
    int status = EXIT_SUCCESS;

    // Terminate message loop
    setLoop( 0 ); setState( RM_SHUTDOWN );

    // get control loop's attention NOW
    LCSM_EVENT_INFO msg;
    msg.event = EXIT_REQUEST;
    msg.dataPtr = NULL;
    msg.dataLength = 0;


    lcsm_send_immediate_event( handle, POD_RSMGR_QUEUE, &msg );

    // Wait for message processing thread to join
    if( pthread_join( rmThread, NULL ) != 0 )
    {
        MDEBUG (DPM_ERROR, "ERROR: pthread_join failed\n" );
        status = EXIT_FAILURE;
    }
    return status;
}

#ifdef __cplusplus
}
#endif



