/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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
#include <rdk_debug.h>
#include <podimpl.h>
#include <rmf_osal_util.h>
#include <rmf_osal_mem.h>
#include "rmf_osal_sync.h"
#include <rmf_osal_thread.h>
#include <rmf_osal_types.h>
#include <rmf_osal_event.h>
#include <stdlib.h>
#include <string.h>
#include <podmgr.h>
#include "rmfqamsrc.h"
#include "podServer.h"
#include "diagMsgEvent.h"
#include "appmgr.h"
#include "cardManagerIf.h"


#include "si_util.h"
#include <podIf.h>
#include <rmf_oobService.h>
#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif


extern rmf_PODDatabase podDB;
extern uint32_t casSessionId;
extern uint16_t casResourceVersion; 
rmf_Error podmgrRegisterHomingQueue(rmf_osal_eventqueue_handle_t eventQueueID, void* act);

extern rmf_Error podmgrCreateCAPMT_APDU( parsedDescriptors *
										 pPodSIDescriptors,
										 uint8_t programIdx,
										 uint8_t transactionId, uint8_t ltsid,
										 uint16_t ca_system_id,
										 uint8_t ca_pmt_cmd_id,
										 uint8_t ** apdu_buffer,
										 uint32_t * apdu_buffer_size );

static rmf_Error podProcessApduInt( uint32_t sessionId, uint8_t * apdu,
								 int32_t len);
//extern rmf_Error vlMpeosPostPodEvent(int ePodEvent, void  *nEventData, uint32_t data_extension);
char *podImplCAEventString( rmf_PODSRVDecryptSessionEvent event );
parsedDescriptors *dupDescriptor(parsedDescriptors *src);

void podmgrImpl_freefn( void *data )
{

	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, data );

}

/* local globals */
static rmf_osal_ThreadId podImplThreadId = NULL;
static rmf_osal_eventqueue_handle_t podImplEvQ = 0;
static char *podImplEvQName = "POD_IMPL_EvQ";

uint16_t ca_system_id = 0;
static uint8_t g_ltsid = 0;

static unsigned long g_nLatestTsId = 0;
/* logging text */
#define PODMODULE "<<POD_IMPL>>"
/* detailed logging info */
#define PODIMPL_DETAILED_DEBUG	(1)


#define		 ACTIVE_DECRYPT_STATE(state)		((state == RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY) \
											  || (state == RMF_PODMGR_DECRYPT_STATE_ISSUED_MMI) \
											  || (state == RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING)\
											  || (state == RMF_PODMGR_DECRYPT_STATE_FAILED_DESCRAMBLING))

static rmf_osal_Mutex table_mutex;		/* Mutex for synchronous access */
typedef LINKHD *ListPIDs;

typedef struct
{
	uint8_t transactionId;
	uint8_t ltsid;
	uint16_t programNum;
	uint16_t sourceId;
	uint8_t ca_pmt_cmd_id;
	RMF_PODMGRDecryptState state;
	uint8_t priority;
	ListPIDs authorizedPids;
	rmf_PODSRVDecryptSessionEvent lastEvent;
	uint16_t ecmPid;
	podmgr_CPSession cpSession;
	uint8_t tunerId;
} podImpl_ProgramIndexTableRow;



typedef struct
{
	uint32_t transportStreamsUsed;
	uint32_t programsUsed;
	uint32_t elementaryStreamsUsed;
	uint32_t numRows;
	podImpl_ProgramIndexTableRow *rows; /* array of rows */
} podImpl_ProgramIndexTableStruct;

static podImpl_ProgramIndexTableStruct programIndexTable =
	{ 0, 0, 0, 0, NULL };

typedef struct
{
	uint8_t programIdx;
	parsedDescriptors *pServiceHandle;
	uint32_t qamContext;
	uint8_t tunerId;
	uint8_t priority;
	uint16_t sourceId;
	rmf_PODMGRDecryptRequestState state;
	uint8_t ca_pmt_cmd_id;
	rmf_osal_Bool mmiEnable;
	rmf_osal_Cond requestCondition;
	uint32_t numPids;
	rmf_MediaPID *pids;
	rmf_osal_eventqueue_handle_t eventQ;		/* queue to communicate session events to higher levels */
	uint8_t ltsId;
	int16_t CCI;			//cci information of the program
	unsigned char EMI;			// EMI part of cci
	unsigned char APS;			// APS part of cci
	unsigned char CIT;			// CIT part of cci
	unsigned char RCT;			//
	unsigned char CCIAuthStatus;		// if this flag is set to 1 then CCI Auth success else if set to 0 then failure
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
	rmf_osal_Bool isRecording;
#endif
/* Standard code deviation: ext: X1 end */
} podImpl_RequestTableRow;

typedef LINKHD *ListRequests;
static ListRequests requestTable;

typedef struct _QueueEntry
{
	rmf_osal_eventqueue_handle_t m_queue;
	struct _QueueEntry *next;
} QueueEntry;

static QueueEntry *podImplEventQueueList;
static rmf_osal_Mutex podImplEventQueueListMutex;
static uint8_t podImplEventQueueListInitialized = 0;
/* forward references */
static void podImplEventHandlerThread( void *data );

#define    VALID_PROGRAM_INDEX(index)	 (((index >= 0) \
		 && (index <= programIndexTable.numRows)) && (programIndexTable.numRows > 0))

#define    LTSID_UNDEFINED	-1
static rmf_Error initProgramIndexTable( void );
static void initRequestTable( void );

static rmf_Error sendPodEvent( uint32_t event,
							   rmf_osal_event_params_t eventData );



static rmf_Error sendDecryptSessionEvent( rmf_PODSRVDecryptSessionEvent event,
										  rmf_PodSrvDecryptSessionHandle
										  sessionHandlePtr,
										  uint32_t edEventData3 );


static rmf_Error releaseDecryptRequest( podImpl_RequestTableRow *
										requestPtr );

static rmf_osal_Bool checkAvailableResources( uint8_t tunerId,
											  uint8_t numStreams );
static rmf_osal_Bool tunerInUse( uint8_t tunerId );
static rmf_Error releaseProgramIndexRow( uint8_t programIdx );
static rmf_osal_Bool updateSessionInfo( rmf_PodSrvDecryptSessionHandle *
										handle,
										VlCardManagerCCIData_t * pCciData );

static void
removeAllDecryptSessionsAndResources( rmf_PODSRVDecryptSessionEvent event );

static int getNextAvailableProgramIndexTableRow( void );
/* Standard code deviation: ext: X1 start */
//VL updated POD implementation
static void commitProgramIndexTableRow( uint8_t programIdx,
										uint8_t ca_pmt_cmd_id,
										uint16_t progNum, uint16_t sourceId,
										uint8_t transactionId,
										uint8_t tunerId, uint8_t ltsId,
										uint8_t priority, uint8_t numStreams,
										uint16_t ecmPid,
										rmf_PodmgrStreamDecryptInfo
										streamInfo[], uint8_t state
#ifdef CMCST_EXT_X1
										, rmf_osal_Bool isRecording
#endif
	 );
/* Standard code deviation: ext: X1 end */
static uint8_t getLtsid( void );
static rmf_osal_Bool isLtsidInUse( uint8_t ltsid );
static uint8_t getLtsidForTunerId( uint8_t tuner );
#ifdef CMCST_EXT_X1
extern "C" uint32_t getLtsId( uint8_t tunerId );
#endif
static uint8_t getTransactionIdForProgramIndex( uint8_t programIdx );
static uint8_t getNextTransactionIdForProgramIndex( uint8_t programIdx );
static rmf_osal_Bool isProgramIndexInUse( uint8_t programIdx );
static rmf_Error getDecryptRequestProgramIndex( rmf_PodSrvDecryptSessionHandle
												handle,
												uint8_t * programIdxPtr );
static rmf_Error setPidArrayForProgramIndex( uint8_t programIdx,
											 uint16_t numPids,
											 uint16_t elemStreamPidArray[],
											 uint8_t caEnableArray[] );
static rmf_PODSRVDecryptSessionEvent getLastEventForProgramIndex( uint8_t
																  programIndex );
static void updateProgramIndexTablePriority( uint8_t programIndex,
											 uint8_t priority );
static void updatePriority( uint8_t programIndex );
static void logProgramIndexTable( char *localStr );
static char *programIndexTableStateString( uint8_t state );
static rmf_Error findNextSuspendedRequest( uint8_t * tunerId,
										   parsedDescriptors ** handle );
static rmf_Error findActiveDecryptRequestForReuse( uint8_t tunerId,
												   uint16_t programNumber,
												   int *index );
#ifdef CMCST_EXT_X1
static rmf_Error findActiveDecryptRequestForReuseExtended(decryptRequestParams *decryptRequestPtr, uint16_t programNumber, int *index);
#endif
static void removeUnusedRequestFromRequestTable( char *localStr );
static void addRequestToTable( podImpl_RequestTableRow * newRequest );
static void removeRequestFromTable( podImpl_RequestTableRow * newRequest );
static rmf_osal_Bool findRequest( rmf_PodSrvDecryptSessionHandle handle );
//static rmf_Error getDecryptSessionHandleForProgramIndex(uint8_t programIdx, rmf_PodSrvDecryptSessionHandle *handle);
static rmf_Error getDecryptSessionsForProgramIndex( uint8_t programIdx,
													uint8_t * numHandles,
													rmf_PodSrvDecryptSessionHandle
													handle[] );
static rmf_Error updatePODDecryptRequestAndResource( uint8_t programIdx,
													 uint8_t transactionId,
													 uint16_t numPids,
													 uint16_t
													 elemStreamPidArray[],
													 uint8_t
													 caEnableArray[] );
static void logRequestTable( char *localStr );
static char *requestTableStateString( uint8_t state );
static rmf_Error createAndSendCaPMTApdu( parsedDescriptors * serviceHandle,
										 uint8_t programIdx,
										 uint8_t transactionId, uint8_t ltsid,
										 uint8_t ca_cmd_id );
static void sendEventImpl( rmf_PODSRVDecryptSessionEvent event,
						   uint8_t programIdx );
static rmf_Error
findRequestProgramIndexWithLowerPriority( podImpl_RequestTableRow *
										  requestPtr, int *programIdx );
static rmf_Error activateSuspendedRequests( uint8_t programIdx );
static uint8_t getSuspendedRequests( void );
static uint8_t caCommandToState( uint8_t cmd_id );
uint16_t getCASystemIdFromCaReply( uint8_t * apdu );
static uint32_t getApduTag( uint8_t * apdu );
rmf_osal_Bool isCaInfoReplyAPDU( uint32_t sessionId, uint8_t * apdu );
static rmf_osal_Bool isCaReplyAPDU( uint32_t sessionId, uint8_t * apdu );
static rmf_osal_Bool isCaUpdateAPDU( uint32_t sessionId, uint8_t * apdu );
static uint16_t getApduDataOffset( uint8_t * apdu );
static int32_t getApduDataLen( uint8_t * apdu );

static char *caCommandString( uint8_t ca_pmt_cmd );

/* raw data, needs to be parsed to determine if there is valid data in byte */
static uint8_t getOuterCaEnableFromCaReply( uint8_t * apdu );
static void getInnerCaEnablesFromCaReply( uint8_t * apdu, uint16_t * numPids,
										  uint16_t elemStreamPidArray[],
										  uint8_t caEnableArray[] );
static uint8_t getProgramIdxFromCaReply( uint8_t * apdu );
static uint8_t getTransactionIdFromCaReply( uint8_t * apdu );

static uint8_t getProgramIdxFromCaUpdate( uint8_t * apdu );
static uint8_t getTransactionIdFromCaUpdate( uint8_t * apdu );
/* raw data, needs to be parsed to determine if there is valid data in byte */
static uint8_t getOuterCaEnableFromCaUpdate( uint8_t * apdu );
static void getInnerCaEnablesFromCaUpdate( uint8_t * apdu, uint16_t * numPids,
										   uint16_t elemStreamPidArray[],
										   uint8_t caEnableArray[] );

static rmf_osal_Bool isValidReply( uint8_t programIdx,
								   uint8_t transactionId );
static rmf_osal_Bool isValidUpdate( uint8_t programIdx,
									uint8_t transactionId );

static void shutdownQAMSrcLevelRegisteredPodEvents(  );

static void notifyQAMSrcLevelRegisteredPodEventQueues( uint32_t event,
													   rmf_osal_event_params_t
													   eventData );

static void freeDescriptor( parsedDescriptors *pParseOutput );

static rmf_Error suspendActiveCASessions( void );
static rmf_Error activateSuspendedCASessions( void );

#define DEFAULT_HEXDUMP_LINE_BUFFER_SIZE 128
static uint32_t g_podImpl_ca_enable = 0;
static uint32_t g_podImpl_ca_retry_count = 0;
static uint32_t g_podImpl_ca_retry_timeout = 0;


static void hexDump( uint8_t * preamble, uint8_t * buffer,
					 uint32_t buffer_length )
{
	uint8_t byLineBuf[DEFAULT_HEXDUMP_LINE_BUFFER_SIZE];

	size_t uiInputBufIndex = 0;
	size_t uiLineBufIndex;

	int iCharacter;

	while ( uiInputBufIndex < buffer_length )
	{
		memset( byLineBuf, ' ', DEFAULT_HEXDUMP_LINE_BUFFER_SIZE );
		byLineBuf[DEFAULT_HEXDUMP_LINE_BUFFER_SIZE - 1] = 0;

		for ( uiLineBufIndex = 0;
			  ( uiLineBufIndex < 16 )
			  && ( uiInputBufIndex + uiLineBufIndex ) < buffer_length;
			  uiLineBufIndex++ )
		{
			snprintf( ( char * ) &byLineBuf[uiLineBufIndex * 3], 4, "%02x ",
					  buffer[uiLineBufIndex + uiInputBufIndex] );

			iCharacter = buffer[uiLineBufIndex + uiInputBufIndex];
			if ( iCharacter < ' ' || iCharacter > '~' )
				iCharacter = '.';

			snprintf( ( char * ) &byLineBuf[( 16 * 3 ) + uiLineBufIndex + 1],
					  2, "%c", iCharacter );
		}

		byLineBuf[uiLineBufIndex * 3] = ' ';
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: [%06d]: %s\n", preamble,
				 uiInputBufIndex, byLineBuf );

		uiInputBufIndex += uiLineBufIndex;
	}
}

/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
rmf_Error podImplGetCaSystemId( unsigned short *pCaSystemId )
{
	if ( NULL != pCaSystemId )
	{
		*pCaSystemId = ca_system_id;
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SNMP",
				 "%s: %s: returning ca_system_id=%d\n", PODMODULE,
				 __FUNCTION__, ca_system_id );
		return RMF_SUCCESS;
	}

	return RMF_OSAL_EINVAL;
}
#endif
/* Standard code deviation: ext: X1 end */

rmf_Error podImplInit(	)
{
	rmf_Error retCode = RMF_SUCCESS;
	const char *podImpl_ca_enable = NULL;
	const char *podImpl_ca_retry_count = 0;
	const char *podImpl_ca_retry_timeout = 0;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: podImplInit\n", PODMODULE );
	registerForInternalApdu( &podProcessApduInt );

	// Create the global mutex
	rmf_osal_mutexNew( &table_mutex );

	// Init the structures
	podImplEventQueueList = NULL;
	rmf_osal_mutexNew( &podImplEventQueueListMutex );
	podImplEventQueueListInitialized = 1;
	
	if ( ( retCode =
		   rmf_osal_threadCreate( podImplEventHandlerThread, NULL,
								  RMF_OSAL_THREAD_PRIOR_DFLT,
								  RMF_OSAL_THREAD_STACK_SIZE,
								  &podImplThreadId,
								  "podImplEventHandlerThread" ) ) !=
		 RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: podImplInit: failed to create pod worker thread (err=%d).\n",
				 PODMODULE, retCode );
	}

	podImpl_ca_enable = rmf_osal_envGet( "POD.MPE.CA.ENABLE" ); //"POD.OLD.CA.ENABLE"
	if ( ( podImpl_ca_enable != NULL )
		 && ( strcasecmp( podImpl_ca_enable, "TRUE" ) == 0 ) )
	{
		g_podImpl_ca_enable = TRUE;
	}
	else
	{
		// Default value is FALSE
		// If POD CA management is disabled the stack does not
		// send/process APDUs on the CAS session.
		g_podImpl_ca_enable = FALSE;
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<%s::podImplInit> - POD CA management is %s\n", PODMODULE,
			 ( g_podImpl_ca_enable ? "ON" : "OFF" ) );

	podImpl_ca_retry_count = rmf_osal_envGet( "POD.MPE.CA.APDU.SEND.RETRY.COUNT" );		//POD.OLD.CA.APDU.SEND.RETRY.COUNT
	if ( podImpl_ca_retry_count != NULL )
	{
		g_podImpl_ca_retry_count = atoi( podImpl_ca_retry_count );
	}
	else
	{
		// Default is 8
		g_podImpl_ca_retry_count = 8;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<%s::podImplInit> - POD CA APDU retry count is %d\n", PODMODULE,
			 g_podImpl_ca_retry_count );

	podImpl_ca_retry_timeout = rmf_osal_envGet( "POD.MPE.CA.APDU.SEND.RETRY.TIMEOUT" ); //POD.OLD.CA.APDU.SEND.RETRY.TIMEOUT
	if ( podImpl_ca_retry_timeout != NULL )
	{
		g_podImpl_ca_retry_timeout = atoi( podImpl_ca_retry_timeout );
	}
	else
	{
		// Default is 4 sec
		g_podImpl_ca_retry_timeout = 4000;
	}

	// Initialize the decrypt request table
	initRequestTable(  );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<%s::podImplInit> - POD CA APDU retry timeout is %d\n",
			 PODMODULE, g_podImpl_ca_retry_timeout );

	/*
	 * Seed for the random ltsid
	 */
	srand( ( unsigned ) time( NULL ) );
	g_ltsid = ( uint8_t ) rand(  );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "<%s::podImplInit> - g_ltsid: %d\n",
			 PODMODULE, g_ltsid );

	return retCode;
}

rmf_Error podProcessApduInt( uint32_t sessionId,  uint8_t * apdu,
								 int32_t len)
{
	rmf_Error retCode;
	int8_t outerCaEnable;

	/* these two together make up the unique identifier for the originating message */
	int8_t programIdx;
	int8_t transactionId;
	rmf_osal_Bool internallyProcessedAPDU = FALSE;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: %s: enter\n", PODMODULE,
			 __FUNCTION__ );

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s: rmf_podmgrReceiveAPDU sessionId=0x%x apdu=0x%p, len=%d\n",
			 PODMODULE, sessionId, apdu, len );
	/*
	 * for(i=0;i<10;i++)
	 * {
	 * RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
	 * "%s: rmf_podmgrReceiveAPDU apdu[%d]=0x%02x \n", PODMODULE, i, (*apdu)[i]);
	 * }
	 */

	/* CA APDUs do not need to be propagated upto Java layer */
	if ( sessionId == casSessionId )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s: rmf_podmgrReceiveAPDU received CA APDU \n",
				 PODMODULE );

		internallyProcessedAPDU = TRUE;

		if ( g_podImpl_ca_enable == FALSE )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU CA APDU processing is OFF...\n",
					 PODMODULE );

			return RMF_OSAL_ENODATA;
		}
		else
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU CA APDU processing is ON, continuing...\n",
					 PODMODULE );
		}
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s: rmf_podmgrReceiveAPDU not CA-PMT apdu...\n",
				 PODMODULE );

		internallyProcessedAPDU = FALSE;

		return RMF_SUCCESS;
	}

	// Assert: CA_PMT processing is ON and we're processing a CAS APDU

	/* Acquire mutex */
	if ( rmf_osal_mutexAcquire( table_mutex ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: rmf_podmgrReceiveAPDU error acquiring table mutex..\n",
				 PODMODULE );
	}

	// is this a ca_info_reply message?
	if ( isCaInfoReplyAPDU( sessionId, apdu ) )
	{
		// This is in response to the ca_info_inquiry apdu
		// Extract the CA_System_id (2 bytes after the tag and length fields)
		ca_system_id = getCASystemIdFromCaReply( apdu );

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: rmf_podmgrReceiveAPDU ca_system_id=%d\n", PODMODULE,
				 ca_system_id );

	}
	// is this a ca_reply message? (ca_pmt reply)
	else if ( isCaReplyAPDU( sessionId, apdu ) )
	{
		// This is in response to ca_pmt apdu
		// with ca_cmd_id = 0x03 (query)

		// Use random size arrays for elementary streams
		uint8_t innerCaEnableArray[512];
		uint16_t elemStreamPidArray[512];
		uint16_t numPids = 0;

		programIdx = getProgramIdxFromCaReply( apdu );
		transactionId = getTransactionIdFromCaReply( apdu );

		// Check if the reply is valid
		if ( !isValidReply( programIdx, transactionId ) )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU received CA_PMT_REPLY, INVALID programIdx:%d, or transactionId:%d .\n",
					 PODMODULE, programIdx, transactionId );
			goto doneProcessingAPDU;
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: rmf_podmgrReceiveAPDU received CA_PMT_REPLY, programIdx:%d, transactionId:%d.\n",
				 PODMODULE, programIdx, transactionId );

		outerCaEnable = getOuterCaEnableFromCaReply( apdu );
		if ( ( outerCaEnable & RMF_PODMGR_CA_ENABLE_SET ) > 0 )
		{
			uint8_t caEnableArray[] = { outerCaEnable & 0x7f };

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU CA_PMT_REPLY, outerCaEnable:0x%02x, caEnable(program level):0x%x.\n",
					 PODMODULE, outerCaEnable, caEnableArray[0] );

			updatePODDecryptRequestAndResource( programIdx, transactionId,
												0, NULL, caEnableArray );
		}

		// Both outer code and for inner codes can exist
		getInnerCaEnablesFromCaReply( apdu, &numPids, elemStreamPidArray,
									  innerCaEnableArray );
		if ( numPids != 0 )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU CA_PMT_REPLY, getInnerCaEnablesFromCaReply returned numPids:%d\n",
					 PODMODULE, numPids );

			updatePODDecryptRequestAndResource( programIdx, transactionId,
												numPids,
												elemStreamPidArray,
												innerCaEnableArray );
		}
	}
	// is this a ca_update message? (ca_pmt update)
	else if ( isCaUpdateAPDU( sessionId, apdu ) )
	{
		// use a large # for array size.
		uint8_t caEnableArray[512];
		uint16_t elemStreamPidArray[512];
		uint16_t numPids = 0;

		programIdx = getProgramIdxFromCaUpdate( apdu );
		transactionId = getTransactionIdFromCaUpdate( apdu );

		// Check if the update is valid
		if ( !isValidUpdate( programIdx, transactionId ) )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU received CA_PMT_UPDATE, INVALID programIdx:%d, or transactionId:%d .\n",
					 PODMODULE, programIdx, transactionId );
			goto doneProcessingAPDU;
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: rmf_podmgrReceiveAPDU received CA_PMT_UPDATE programIdx:%d, transactionId:%d.\n",
				 PODMODULE, programIdx, transactionId );

		// if so, is it a success or failure message
		// get outer code if it exists
		outerCaEnable = getOuterCaEnableFromCaUpdate( apdu );
		if ( ( outerCaEnable & RMF_PODMGR_CA_ENABLE_SET ) > 0 )
		{
			uint8_t caEnableArray[] = { outerCaEnable & 0x7f };

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU CA_PMT_UPDATE, outerCaEnable:0x%02x, caEnable(program level):0x%x.\n",
					 PODMODULE, outerCaEnable, caEnableArray[0] );

			updatePODDecryptRequestAndResource( programIdx, transactionId,
												0, NULL, caEnableArray );
		}

		// Both inner and outer level authorizations may exist
		// Stream level authorization over-rides the program level
		// authorization
		getInnerCaEnablesFromCaUpdate( apdu, &numPids,
									   elemStreamPidArray,
									   caEnableArray );

		if ( numPids != 0 )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU CA_PMT_UPDATE, getInnerCaEnablesFromCaUpdate returned numPids:%d\n",
					 PODMODULE, numPids );

			updatePODDecryptRequestAndResource( programIdx, transactionId,
												numPids,
												elemStreamPidArray,
												caEnableArray );
		}
	}
	else
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: rmf_podmgrReceiveAPDU not CA-PMT apdu...\n",
				 PODMODULE );
	}

  doneProcessingAPDU:

	logProgramIndexTable( "ReceiveAPDU" );
	/* Release mutex */
	rmf_osal_mutexRelease( table_mutex );

	/* CAS APDUs do not need to be propagated up to Java layer - just release and read again */

	//Moved up to be called to be with in the table_mutex control
//		  logProgramIndexTable("ReceiveAPDU");
	logRequestTable( "ReceiveAPDU" );

	internallyProcessedAPDU = TRUE;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s: rmf_podmgrReceiveAPDU completed processing CA APDU. Issuing new read...\n",
			 PODMODULE );

	return RMF_OSAL_ENODATA;

}

rmf_Error rmf_podmgrReceiveAPDU( uint32_t * sessionId, uint8_t ** apdu,
								 int32_t * len )
{
	rmf_Error retCode;

	/* these two together make up the unique identifier for the originating message */
	rmf_osal_Bool internallyProcessedAPDU = FALSE;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: %s: enter\n", PODMODULE,
			 __FUNCTION__ );

	do
	{
		retCode = podReceiveAPDU( sessionId, apdu, len );

		if ( retCode != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU returned error = %d\n",
					 PODMODULE, retCode );
			rmf_osal_threadSleep( 200, 0 );
			continue;
		}
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s: rmf_podmgrReceiveAPDU sessionId=0x%x apdu=0x%p, len=%d\n",
				 PODMODULE, *sessionId, apdu, *len );
		/*
		 * for(i=0;i<10;i++)
		 * {
		 * RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
		 * "%s: rmf_podmgrReceiveAPDU apdu[%d]=0x%02x \n", PODMODULE, i, (*apdu)[i]);
		 * }
		 */

		if( *sessionId != casSessionId ) 
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: rmf_podmgrReceiveAPDU not CA-PMT apdu...\n",
					 PODMODULE );

			internallyProcessedAPDU = FALSE;

			continue;
		}

	}
	while ( internallyProcessedAPDU );

	return retCode;
}								// END rmf_podmgrReceiveAPDU()
rmf_Error podmgrStartDecrypt( decryptRequestParams *decryptRequestPtr,
								  rmf_osal_eventqueue_handle_t queueId,
								  rmf_PodSrvDecryptSessionHandle *
								  sessionHandlePtr )
{
	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			 "%s::rmf_podmgrStartDecrypt starting \n", PODMODULE );

	if ( g_podImpl_ca_enable == FALSE )
	{
		// If POD CA management is disabled the stack does not
		// send/process APDUs on the CAS session.

		*sessionHandlePtr = NULL;
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
		         "%s::rmf_podmgrStartDecrypt POD CA management is disabled!, no APDUs on the CAS session \n", PODMODULE );
		return RMF_SUCCESS;
	}
	else
	{
		rmf_Error retCode;
		int programIndex = -1;
		uint8_t transactionId;
		uint32_t sizeOfRequest = 0;
		int i = 0;

/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
		// VL updated POD POD implementation
		const char *skip_ca_query = NULL;
		char skip_ca_query_flag = 0;
		rmf_osal_Bool isClear = FALSE;
#endif
/* Standard code deviation: ext: X1 end */

		podImpl_RequestTableRow *newRequest = NULL;
		sizeOfRequest = sizeof( podImpl_RequestTableRow );


		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s::podImplStartDecrypt Enter..\n", PODMODULE );

		/* start routine */
		*sessionHandlePtr = NULL;

		if ( checkAvailableResources
			 ( decryptRequestPtr->tunerId, decryptRequestPtr->pDescriptor->noES ) != TRUE )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<%s::startDecrypt> - No resource available for source id, hence returning \n",
					 PODMODULE, decryptRequestPtr->pDescriptor->srcId );
			return RMF_OSAL_ENODATA;
		}

		// Check if there are any CA descriptors for the given service handle
		// This method populates the input table with elementary stream Pids associated with
		// the service. The CA status field is set to unknown. It will
		// be filled when ca reply/update is received from CableCARD.
		if ( decryptRequestPtr->pDescriptor->caDescFound != 1 )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<%s::startDecrypt> - No CA descriptors for source id (could be clear channel..)\n",
					 PODMODULE, decryptRequestPtr->pDescriptor->srcId );
			// CCIF 2.0 section 9.7.3
			// Even if the PMT does not contain any CA descriptors
			// a CA_PMT still needs to be sent to the card but with no CA descriptors
			// This path can no longer be used as an early return but used just
			// to retrieve the component information
			/* Standard code deviation: ext: X1 start */
			isClear = TRUE;
#ifdef CMCST_EXT_X1
		}
/* Standard code deviation: ext: X1 end */

#endif

		/* Acquire table mutex */
		rmf_osal_mutexAcquire( table_mutex );
		if(NULL == programIndexTable.rows)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
				"<%s:: Program Index Table array is NULL. Possibly due to CARD REMOVED event \n", PODMODULE);
			rmf_osal_mutexRelease( table_mutex );	
			return RMF_OSAL_ENODATA;
		}
		
		/* Create a new request session */
		{
			/* allocate request session
			 */
			if ( ( retCode =
				   rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeOfRequest,
									   ( void ** ) &newRequest ) ) !=
				 RMF_SUCCESS )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s::startDecrypt> - Could not malloc memory for newRequest\n",
						 PODMODULE );

				goto freeSessionReturn;
			}

			memset( newRequest, 0x0, sizeOfRequest );

			newRequest->state = RMF_PODMGR_DECRYPT_REQUEST_STATE_UNKNOWN;
			newRequest->tunerId = decryptRequestPtr->tunerId;
			newRequest->qamContext = decryptRequestPtr->qamContext;
			newRequest->mmiEnable = decryptRequestPtr->mmiEnable;
			newRequest->priority = decryptRequestPtr->priority;
			newRequest->eventQ = queueId;

			newRequest->sourceId = decryptRequestPtr->pDescriptor->srcId;
			newRequest->pServiceHandle = dupDescriptor( decryptRequestPtr->pDescriptor );
			newRequest->CCI = -1;

			// use a random number as ltsid
			// ltsid value for the new request is set below after determining
			// if an existing CA session can be shared
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
			// updated MPE POD implementation			
			// Set if request is for recording
//			newRequest->isRecording = decryptRequestPtr->isRecording;
#endif
/* Standard code deviation: ext: X1 end */
                        // make sure we malloc enough including space for the ICN PID we are
                        // going to force into the data structure below
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"<%s::startDecrypt> - malloc memory for pids numpids %d data structure size %d\n",
			PODMODULE, decryptRequestPtr->numPids , sizeof(rmf_MediaPID));
			if ( ( retCode =
				   rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									   ( decryptRequestPtr->numPids *
										 sizeof( rmf_MediaPID ) ),
									   ( void ** ) &newRequest->pids ) ) !=
				 RMF_SUCCESS )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s::startDecrypt> - Could not malloc memory for pids numpids %d\n",
						 PODMODULE, decryptRequestPtr->numPids );

				goto freeSessionReturn;
			}

			// Copy PIDs from decryptRequestParams to newRequest
			for ( i = 0; i < decryptRequestPtr->numPids; i++ )
			{
				newRequest->pids[i].pid = decryptRequestPtr->pids[i].pid;
				newRequest->pids[i].pidType =
					decryptRequestPtr->pids[i].pidType;
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						 "<%s::startDecrypt> - Pod decrypt request Pids, pid=0x%x, type=%0x\n",
						 PODMODULE, newRequest->pids[i].pid,
						 newRequest->pids[i].pidType );
			}
			newRequest->numPids = decryptRequestPtr->numPids;
		}
/* commenting since it is not being used for RDK 2.0 as all the resource handling are done at qamsrc level */		
#if 0

		/* Find a matching entry in the request table to see
		 * if an existing session can be shared (Ex: between broadcast and TSB) */
#ifdef CMCST_EXT_X1 
    if((retCode = findActiveDecryptRequestForReuseExtended(decryptRequestPtr, decryptRequestPtr->pDescriptor->prgNo, &programIndex))
      != RMF_SUCCESS)
#else
		if ( ( retCode =
			   findActiveDecryptRequestForReuse( decryptRequestPtr->tunerId,
												 decryptRequestPtr->pDescriptor->prgNo,
												 &programIndex ) ) !=
			 RMF_SUCCESS )
#endif
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<%s::startDecrypt> - No active decrypt session for tunerId=%d programNumber=%d\n",
					 PODMODULE, decryptRequestPtr->tunerId,
					 decryptRequestPtr->pDescriptor->prgNo );
		}
		else
		{
			// Active session found
			podImpl_ProgramIndexTableRow *rowPtr =
				&programIndexTable.rows[programIndex];

			/* If active session found, add the new request to the request table,
			 * set the state to active. Return the request session pointer
			 * as a session handle to the caller */

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<%s::startDecrypt> - Found a active decrypt session for tunerId=%d programNumber=%d\n",
					 PODMODULE, decryptRequestPtr->tunerId,
					 decryptRequestPtr->pDescriptor->prgNo );

			// Set the program index for the request
			newRequest->programIdx = programIndex;
			// Set the state to 'active'
			newRequest->state = RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE;
			newRequest->ca_pmt_cmd_id = rowPtr->ca_pmt_cmd_id;
			// set the new request ltsid to be same as the rowptr ltsid
			newRequest->ltsId = rowPtr->ltsid;
			// Add new request to the table
			addRequestToTable( newRequest );

			// Update priority of program table index row to reflect higher of the priorities
			updateProgramIndexTablePriority( programIndex,
											 newRequest->priority );

			// Set the session handle and send an event
			*sessionHandlePtr = ( rmf_PodSrvDecryptSessionHandle ) newRequest;

			// Get the state of the existing request and post event based on that
			// Send decryption event

			sendDecryptSessionEvent( getLastEventForProgramIndex
									 ( programIndex ),
									 ( rmf_PodSrvDecryptSessionHandle ) *
									 sessionHandlePtr, rowPtr->ltsid );


			// Done here, return the handle
			goto sessionReturn;
		}
		// No active requests found to share with
		// Check if there are resources needed to service the new request
		if ( checkAvailableResources
			 ( newRequest->tunerId, decryptRequestPtr->pDescriptor->noES ) != TRUE )
		{
			/* When the incoming request has a higher
			 * priority than an existing request the lower priority request will be
			 * pre-empted and placed in a waiting_for_resources state and the new request
			 * will take its place
			 */
			{
				rmf_osal_Bool done = FALSE;
				parsedDescriptors *pServiceHandle = NULL;
				uint8_t ltsid = 0;
				uint8_t transId;

				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						 "<%s::startDecrypt> - No decrypt resources available...\n",
						 PODMODULE );

				while ( !done )
				{
					retCode =
						findRequestProgramIndexWithLowerPriority( newRequest,
																  &programIndex );

					if ( retCode == RMF_SUCCESS )
					{
						// 0. Found low priority request (program index) to preempt
						// 1. Find all requests for this program index
						// 2. Stop decrypt session (program index table row) if active
						// 3. Send 'not_selected' apdu
						// 4. Release program index table row
						// 5. Send JNI events (resource not available) to preempted requests
						// (Requests remain in the request table but their state is set to 'WAITING_FOR_RESOURCES')
						// 6. Check if we have enough resources
						// 7. If not repeat step 0 next request with lower priority
						// 8. If yes, assign the released program index to new request

						// Random size array
						rmf_PodSrvDecryptSessionHandle handles[10];
						uint8_t numHandles = 0;
						int i;
						rmf_PodSrvDecryptSessionHandle rHandle;

						podImpl_ProgramIndexTableRow *rowPtr =
							&programIndexTable.rows[programIndex];

						// There may be multiple requests per program index table row
						if ( ( retCode =
							   getDecryptSessionsForProgramIndex
							   ( programIndex, &numHandles,
								 handles ) ) != RMF_SUCCESS )
						{
							RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
									 "<%s::startDecrypt> error retrieving decrypt sessions for program index:%d.\n",
									 PODMODULE, programIndex );
							goto freeSessionReturn;
						}

						for ( i = 0; i < numHandles; i++ )
						{
							podImpl_RequestTableRow *request =
								( podImpl_RequestTableRow * ) handles[i];

							pServiceHandle = request->pServiceHandle;

							rHandle = handles[i];

							// Reset the requests state and program index
							request->state =
								RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES;
							request->programIdx = -1;
							// Save the command id so that it can be re-activated when
							// resources are available
							request->ca_pmt_cmd_id = rowPtr->ca_pmt_cmd_id;


							// Send an event to notify of resource loss
							sendDecryptSessionEvent
								( RMF_PODSRV_DECRYPT_EVENT_RESOURCE_LOST,
								  rHandle, ltsid );

						}

						ltsid = rowPtr->ltsid;
						transId =
							getNextTransactionIdForProgramIndex
							( programIndex );

						// Send a ca_pmt apdu with 'not_selected' (0x04) when decrypt session
						// is no longer in use

						if ( ( retCode =
							   createAndSendCaPMTApdu( decryptRequestPtr->pDescriptor,
													   programIndex, transId,
													   ltsid,
													   RMF_PODMGR_CA_NOT_SELECTED )
							   != RMF_SUCCESS ) )
						{
							RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
									 "<%s::startDecrypt> - Error sending CA_PMT apdu (RMF_PODMGR_CA_NOT_SELECTED) (error %d)\n",
									 PODMODULE, retCode );
							goto freeSessionReturn;
						}

						// Release the CP session
						if ( rowPtr->cpSession != NULL )
						{
							if ( RMF_SUCCESS !=
								 podStopCPSession( rowPtr->cpSession ) )
							{
								RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
										 "%s::startDecrypt Error stopping CP session...\n",
										 PODMODULE );
							}
						}

						if ( ( retCode =
							   releaseProgramIndexRow( programIndex ) ) !=
							 RMF_SUCCESS )
						{
							goto freeSessionReturn;
						}

						// Restore the transaction id
						rowPtr->transactionId = transId;

						// Check again for resources and repeat if not done
						if ( checkAvailableResources
							 ( newRequest->tunerId,
							   decryptRequestPtr->pDescriptor->noES ) == TRUE )
						{
							done = TRUE;
						}
					}
					else
					{
						// Did not find a request to preempt, the new request will be added to table but will be in
						// waiting_for_resources state.
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
								 "<%s::startDecrypt> - Did not find a lower priority requests to preempt..\n",
								 PODMODULE );

						newRequest->state =
							RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES;
						newRequest->programIdx = -1;

						// Add new request to the request table
						addRequestToTable( newRequest );


						// Send an event to notify of resource loss
						sendDecryptSessionEvent
							( RMF_PODSRV_DECRYPT_EVENT_RESOURCE_LOST,
							  ( rmf_PodSrvDecryptSessionHandle ) newRequest,
							  0 );

						goto sessionReturn;
					}
				}				// END while (!done)
			}
		}
#endif
		if ( podmgrIsReady(  ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s::podImplStartDecrypt() POD is not READY.\n",
					 PODMODULE );

			newRequest->state =
				RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES;
			newRequest->programIdx = -1;

			// Add new request to the request table
			addRequestToTable( newRequest );



			// Send an event to notify of resource loss
			sendDecryptSessionEvent( RMF_PODSRV_DECRYPT_EVENT_RESOURCE_LOST,
									 ( rmf_PodSrvDecryptSessionHandle )
									 newRequest, 0 );


			goto sessionReturn;
		}

		/* Get the next available program index table row that's not in use */
		programIndex = getNextAvailableProgramIndexTableRow(  );

		/* Now begin creating CA_PMT and go through
		 * steps. Once sendAPDU is successful, set the 'state' and other fields in
		 * program index table. Set the 'programsUsed' field. */

		/* begin creating a CA_PMT */
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "<%s::startDecrypt> - Attempting CA_PMT creation...\n",
				 PODMODULE );
		// use a random number as ltsid
		// 'ltsid' requirements:
		// 1. Not already in use (since we are starting at 8 bit random number and incrementing,
		// when it wraps around at 255 make sure the the number is not being used by another
		// program index row)
		// 2. It is unique per transport stream(CCIF 2.0)
		// Even when two separate CA sessions are opened on the same tuner they
		// need to have the same ltsid
		newRequest->ltsId = decryptRequestPtr->ltsId;
		
		// If an existing ltsid for this tuner is not found get the next 'random' ltsid

#if 0
		if ( newRequest->ltsId == LTSID_UNDEFINED )
		{
			// getLtsid() validates that the new ltsid is not in use by any other
			// program index rows
			newRequest->ltsId = getLtsid(  );
		}
#endif
		// Set the program index for the request
		newRequest->programIdx = programIndex;

		// Add new request to the request table
		addRequestToTable( newRequest );

		// Set the transaction id
		transactionId = getNextTransactionIdForProgramIndex( programIndex );
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
// updated POD implementation
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "<%s::startDecrypt> - Calling createAndSendCaPMTApdu with programIndex:%d transactionId:%d RMF_PODMGR_CA_QUERY newRequest->isRecording=%d\n",
				 PODMODULE, programIndex, transactionId,
				 newRequest->isRecording );
		if ( !isClear )
		{

			if ( !newRequest->isRecording )
			{
				skip_ca_query = ( char * ) rmf_osal_envGet( "MPE_MPOD_CA_QUERY_DISABLED" );		//OLD_MPOD_CA_QUERY_ENABLED

				if ( ( ( NULL == skip_ca_query )
					   || ( strcasecmp( skip_ca_query, "TRUE" ) == 0 ) )
					 && decryptRequestPtr->priority != 31 )
				{
					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
							 "<%s::startDecrypt> - skip_ca_query_flag POD_CA_QUERY_ENABLED = TRUE\n", PODMODULE );
					skip_ca_query_flag = true;
				}
				else
				{
					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
							 "<%s::startDecrypt> - skip_ca_query_flag POD_CA_QUERY_ENABLED = FALSE\n", PODMODULE );
					skip_ca_query_flag = false;
				}
			}
		}
		else
		{
			skip_ca_query_flag = true;
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "<%s::startDecrypt> - Initiating CA Query:Is new request for recording? %d and priority =%d \n",
				 PODMODULE, newRequest->isRecording, newRequest->priority );
		if ( skip_ca_query_flag == false )
		{
			// For ltsid -> use tunerId
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<%s::startDecrypt> - Initiating CA Query:Is new request for recording? %d and priority =%d \n",
					 PODMODULE, newRequest->isRecording,
					 newRequest->priority );
#endif
/* Standard code deviation: ext: X1 start */
			retCode =
				createAndSendCaPMTApdu( decryptRequestPtr->pDescriptor, programIndex,
										transactionId, newRequest->ltsId,
										RMF_PODMGR_CA_QUERY );
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
//updated POD implementation			  
		}
		else
		{

			// For ltsid -> use tunerId
			retCode =
				createAndSendCaPMTApdu( decryptRequestPtr->pDescriptor, programIndex,
										transactionId, newRequest->ltsId,
										RMF_PODMGR_CA_OK_DESCRAMBLE );
		}
#endif
/* Standard code deviation: ext: X1 start */
		// now deal with the various return conditions.
		if ( retCode == RMF_SUCCESS )
		{
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
//updated POD implementation	  
			if ( skip_ca_query_flag == false )
			{
#endif
/* Standard code deviation: ext: X1 end */
				// set state to RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY
				commitProgramIndexTableRow( programIndex, RMF_PODMGR_CA_QUERY,
											decryptRequestPtr->pDescriptor->prgNo,
											decryptRequestPtr->pDescriptor->srcId,
											transactionId,
											decryptRequestPtr->tunerId,
											newRequest->ltsId,
											newRequest->priority,
											decryptRequestPtr->pDescriptor->noES,
											decryptRequestPtr->pDescriptor->ecmPid,
											decryptRequestPtr->pDescriptor->pESInfo,
											RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
											, newRequest->isRecording
#endif
/* Standard code deviation: ext: X1 end */
					 );

				// Set the state of request table entry based on the return code from sendAPDU
				newRequest->state = RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE;
				newRequest->ca_pmt_cmd_id = RMF_PODMGR_CA_QUERY;
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
// updated POD implementation		 
			}
			else
			{

				// set state to RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY
				commitProgramIndexTableRow( programIndex,
											/* RMF_PODMGR_CA_QUERY */
											RMF_PODMGR_CA_OK_DESCRAMBLE,
											decryptRequestPtr->pDescriptor->prgNo,
											decryptRequestPtr->pDescriptor->srcId,
											transactionId,
											decryptRequestPtr->tunerId,
											newRequest->ltsId,
											newRequest->priority,
											decryptRequestPtr->pDescriptor->noES,
											decryptRequestPtr->pDescriptor->ecmPid,
											decryptRequestPtr->pDescriptor->pESInfo,
											RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING
											/*RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY */
											, newRequest->isRecording );

				// Set the state of request table entry based on the return code from sendAPDU
				newRequest->state = RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE;
				newRequest->ca_pmt_cmd_id = RMF_PODMGR_CA_OK_DESCRAMBLE;		//RMF_PODMGR_CA_QUERY;
				if(skip_ca_query != NULL && (strcasecmp(skip_ca_query, "FALSE") == 0) && isClear)
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "<%s::startDecrypt> - Sending POD_DECRYPT_EVENT_FULLY_AUTHORIZED for clear channel\n",PODMODULE);
					podImpl_ProgramIndexTableRow *rowPtr1 = &programIndexTable.rows[programIndex];
					rowPtr1->lastEvent = RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED;
					//  sendEvent(getLastEventForProgramIndex(programIndex),programIndex);
				}
				if(!isClear && skip_ca_query_flag)
					sendEventImpl( RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED, programIndex );
			}
#endif
/* Standard code deviation: ext: X1 end */
#ifdef PODIMPL_FRONTPANEL_DEBUG
			{
				int i;

				for ( i = 0; i < decryptRequestPtr->pDescriptor->noES; i++ )
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							 "<%s::startDecrypt> - pPodSIDescriptors->pStreamPidInfo[%d].pid: %d\n",
							 PODMODULE, i,
							 decryptRequestPtr->pDescriptor->pESInfo[i].pid );
					snprintf( fp_debug_lines[i + 1], 5, "%d",
							  decryptRequestPtr->pDescriptor->pESInfo[i].pid );
				}
			}
#endif
			goto sessionReturn;
		}
		else if ( retCode == RMF_OSAL_ENODATA )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<%s::startDecrypt> - No CA_PMT created (due to no CA descriptors)\n",
					 PODMODULE );

			// We didn't fail - there's just no session required
			*sessionHandlePtr = NULL;
			retCode = RMF_SUCCESS;

			/* clear channel (no CA_Descriptor). Not really an error but the session memory needs to be freed */
			goto freeSessionReturn;
		}
		else
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<%s::startDecrypt> - Error generating CA_PMT (error 0x%x)\n",
					 PODMODULE, retCode );
			goto freeSessionReturn;
		}

	  sessionReturn:
		logProgramIndexTable( "startDecrypt sessionReturn" );
		if ( ( retCode =
			   rmf_osal_mutexRelease( table_mutex ) != RMF_SUCCESS ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<%s::startDecrypt> - sessionReturn, failed to release programIndexTable mutex, error=%d\n",
					 PODMODULE, retCode );
			retCode = RMF_OSAL_EMUTEX;
		}

		/* don't setup session unless everything successful */
		*sessionHandlePtr = ( rmf_PodSrvDecryptSessionHandle ) newRequest;

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "<%s::startDecrypt> - sessionHandle=0x%p\n", PODMODULE,
				 newRequest );

		//Moved up to be called to be with in the table_mutex control
		//logProgramIndexTable("startDecrypt sessionReturn");
		logRequestTable( "startDecrypt sessionReturn" );

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "<%s::startDecrypt> - done..\n",
				 PODMODULE );
		return retCode;
		/* Generally called when there an error */
	  freeSessionReturn:

		if ( rmf_osal_mutexRelease( table_mutex ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<%s::startDecrypt> - freeSessionReturn, failed to release programIndexTable mutex, error=%d\n",
					 PODMODULE, retCode );
			retCode = RMF_OSAL_EMUTEX;			
		}

		// Remove request from list
		removeRequestFromTable( newRequest );

		// De-allocate request
		if ( newRequest != NULL )
		{
			if ( newRequest->pids != NULL )
			{
				rmf_osal_memFreeP( RMF_OSAL_MEM_POD, newRequest->pids );
				newRequest->pids = NULL;
			}
			
			if ( newRequest->pServiceHandle != NULL )
			{
				freeDescriptor( newRequest->pServiceHandle );
				newRequest->pServiceHandle = NULL;				
			}
			
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, newRequest );
			newRequest = NULL;
		}

		return retCode;
	}
}

rmf_Error podmgrStopDecrypt( rmf_PodSrvDecryptSessionHandle
								 sessionHandle )
{
	if ( g_podImpl_ca_enable == FALSE )
	{
		// If POD CA management is disabled the stack does not
		// send/process APDUs on the CAS session.

		return RMF_SUCCESS;
	}
	else
	{
		uint32_t programIdx;
		rmf_Error retCode = RMF_SUCCESS;
		podImpl_ProgramIndexTableRow *rowPtr = NULL;
		podImpl_RequestTableRow *request = NULL;
		parsedDescriptors *pServiceHandle = NULL;
		//uint8_t tunerId = 0;
		uint8_t ltsId;
		uint8_t transId;

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s:rmf_podmgrStopDecrypt sessionHandle=0x%p\n", PODMODULE,
				 sessionHandle );

		/* Acquire mutex */
		rmf_osal_mutexAcquire( table_mutex );

		// Find the request in the request table
		if ( !findRequest( sessionHandle ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<%s::podImplStopDecrypt> - Did not find the session handle: 0x%p\n",
					 PODMODULE, sessionHandle );
			retCode = RMF_OSAL_EINVAL;
			goto stopReturn;
		}


		// Termination of decrypt session, no more events will be sent on this queue.
		retCode =
			sendDecryptSessionEvent
			( RMF_PODSRV_DECRYPT_EVENT_SESSION_SHUTDOWN, sessionHandle, 0 );


		request = ( podImpl_RequestTableRow * ) sessionHandle;

		// Retrieve the corresponding program index, service handle and tunerId
		programIdx = request->programIdx;
		pServiceHandle = request->pServiceHandle;

		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, request->pids );
		//tunerId = request->tunerId;


		// Remove the decrypt request from the table and de-allocate memory
		if ( ( retCode =
			   releaseDecryptRequest( ( podImpl_RequestTableRow * )
									  sessionHandle ) ) != RMF_SUCCESS )
		{
			goto stopReturn;
		}

		if ( !VALID_PROGRAM_INDEX( programIdx ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<%s::podImplStopDecrypt> invalid programIdx:%d\n",
					 PODMODULE, programIdx );
			goto stopReturn;
		}

	    	if(NULL == programIndexTable.rows)
	    	{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
			"%s:: Program Index Table array is NULL. Possibly due to CARD REMOVED event and RESOURCE IS LEAKED...!!\n", 
			__FUNCTION__);
			goto stopReturn;
	    	}
		rowPtr = &programIndexTable.rows[programIdx];

/* commenting since it is not being used for RDK 2.0 as all the resource handling are done at qamsrc level */
#if 0
		// Update priority in the program index array
		updatePriority( programIdx );
#endif
		// Now remove the corresponding entry from the program index table if no other
		// uses remain (based on the program index)
		// check if this programIndex is being used by any other decrypt request
/* commenting since it is not being used for RDK 2.0 as all the resource handling are done at qamsrc level */
#if 0
		if ( !isProgramIndexInUse( programIdx ) )
#endif			
		{
			transId = getNextTransactionIdForProgramIndex( programIdx );
			ltsId = rowPtr->ltsid;
			// Send a ca_pmt apdu with 'not_selected' (0x04) when decrypt session
			// is no longer in use

			if ( ( retCode =
				   createAndSendCaPMTApdu( pServiceHandle, programIdx,
										   transId, ltsId,
										   RMF_PODMGR_CA_NOT_SELECTED ) !=
				   RMF_SUCCESS ) )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s::podImplStopDecrypt> - Error sending CA_PMT apdu (RMF_PODMGR_CA_NOT_SELECTED) (error %d)\n",
						 PODMODULE, retCode );
				goto stopReturn;
			}

			// Release the CP session
			if ( rowPtr->cpSession != NULL )
			{
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						 "%s::podImplStopDecrypt stopping CP session:0x%x\n",
						 PODMODULE, rowPtr->cpSession );
				if ( RMF_SUCCESS != podStopCPSession( rowPtr->cpSession ) )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							 "%s::mpe_podImplStopDecrypt Error stopping CP session...\n",
							 PODMODULE );
				}
			}

			rowPtr->transactionId = transId;

			// Shutting down the decrypt session
			rowPtr->lastEvent = RMF_PODSRV_DECRYPT_EVENT_SESSION_SHUTDOWN;

			if ( ( retCode =
				   releaseProgramIndexRow( programIdx ) ) != RMF_SUCCESS )
			{
				goto stopReturn;
			}
			
/* commenting since it is not being used for RDK 2.0 as all the resource handling are done at qamsrc level */
#if 0
			// Re-activate a suspended request. There may be multiple requests that can share this
			// program index table row
			retCode = activateSuspendedRequests( programIdx );
			if ( retCode == RMF_OSAL_ENODATA )	/* as there is no data to find */
			{
				retCode = RMF_SUCCESS;
			}
			if ( retCode != RMF_SUCCESS )
			{
				goto stopReturn;
			}
#endif
		}

		logProgramIndexTable( "stopDecrypt" );
		logRequestTable( "stopDecrypt" );

	  stopReturn:

		if ( pServiceHandle )
		{
			freeDescriptor( pServiceHandle );
		}

		rmf_osal_mutexRelease( table_mutex );
		/* Release mutex */

		return retCode;
	}
}

rmf_Error rmf_podmgrGetDecryptStreamStatus( rmf_PodSrvDecryptSessionHandle
											handle, uint8_t numStreams,
											rmf_PodmgrStreamDecryptInfo
											streamInfo[] )
{
	if ( g_podImpl_ca_enable == FALSE )
	{
		// If POD CA management is disabled the stack does not
		// send/process APDUs on the CAS session.

		return RMF_SUCCESS;
	}
	else
	{
		rmf_Error retCode = RMF_SUCCESS;
		uint8_t programIdx;
		LINK *lp;
		int i = 0;
		podImpl_ProgramIndexTableRow *rowPtr;

		if ( handle == NULL || numStreams == 0 || streamInfo == NULL )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s::podImplGetDecryptStreamStatus - invalid parameter..\n",
					 PODMODULE );
			return RMF_OSAL_EINVAL;
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s::podImplGetDecryptStreamStatus handle:0x%p\n", PODMODULE,
				 handle );

		/* Acquire mutex */
		rmf_osal_mutexAcquire( table_mutex );

		// Find the request in the request table
		if ( !findRequest( handle ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<%s::podImplStopDecrypt> - Did not find the session handle: 0x%p\n",
					 PODMODULE, handle );
			retCode = RMF_OSAL_EINVAL;
			goto cleanupAndExit;
		}

		if ( ( retCode =
			   getDecryptRequestProgramIndex( handle,
											  &programIdx ) ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s::podImplGetDecryptStreamStatus failed - unable to retrieve requested program index %d..\n",
					 PODMODULE, retCode );
			goto cleanupAndExit;
		}

		if ( ( ( podImpl_RequestTableRow * ) handle )->state !=
			 RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s::podImplGetDecryptStreamStatus session is not active...\n",
					 PODMODULE );
			for ( i = 0; i < numStreams; ++i )
			{
				streamInfo[i].status =
					RMF_PODMGR_CA_ENABLE_DESCRAMBLING_TECH_FAIL;
			}
			goto cleanupAndExit;
		}

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s::podImplGetDecryptStreamStatus programIdx:%d\n",
				 PODMODULE, programIdx );

    	if(NULL == programIndexTable.rows)
    	{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
			"%s::rmf_podImplGetDecryptStreamStatus  Program Index Table array is NULL. Possibly due to CARD REMOVED event \n", PODMODULE);
			goto cleanupAndExit;
    	}

		rowPtr = &programIndexTable.rows[programIdx];

		for ( i = 0; i < numStreams; ++i )
		{
			if ( rowPtr->authorizedPids != NULL
				 && llist_cnt( rowPtr->authorizedPids ) != 0 )
			{
				lp = llist_first( rowPtr->authorizedPids );
				while ( lp )
				{
					rmf_PodmgrStreamDecryptInfo *pidInfo =
						( rmf_PodmgrStreamDecryptInfo * ) llist_getdata( lp );
					if ( pidInfo )
					{
						if ( streamInfo[i].pid == pidInfo->pid )
						{
							streamInfo[i].status = pidInfo->status;
							RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
									 "%s::podImplGetDecryptStreamStatus pid:0x%x status:%d\n",
									 PODMODULE, pidInfo->pid,
									 pidInfo->status );
						}
					}
					lp = llist_after( lp );
				}
			}
			else
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						 "%s::podImplGetDecryptStreamStatus pid list empty..\n",
						 PODMODULE );
			}
		}

	  cleanupAndExit:
		rmf_osal_mutexRelease( table_mutex );
		/* Release mutex */

		return retCode;
	}
}


/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
// updated POD implementation
static rmf_osal_Bool podWasAlreadyReady = FALSE;
#endif
/* Standard code deviation: ext: X1 end */
static void podImplEventHandlerThread( void *data )
{
	rmf_Error retCode = RMF_SUCCESS;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_category_t event_category;
	uint32_t event_type;
	void *edEventData1, *edEventData2;
	uint32_t edEventData3;
	rmf_osal_event_params_t eventData = { 0 };
	uint32_t timeout = 0;		/* infinite timeout */
	rmf_osal_Bool exitWorkerThread = FALSE;
	uint16_t paramValue = 0;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: podWorkerThread started.\n",
			 PODMODULE );

	if ( ( retCode =
		   rmf_osal_eventqueue_create( ( const uint8_t * ) podImplEvQName,
									   &podImplEvQ ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: unable to create event queue, error=%d... terminating thread.\n",
				 PODMODULE, retCode );
		return;
	}

	if ( ( retCode = vlMpeosPodRegister( podImplEvQ, NULL ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: podWorkerThread, podRegister failed %d\n", PODMODULE,
				 retCode );
		return;
	}
	
	/* firmware upgrade support */
	if ( ( retCode = podmgrRegisterHomingQueue( podImplEvQ, NULL ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: podWorkerThread, podRegister for fwu failed %d\n", PODMODULE,
				 retCode );
		return;
	}

	if ( podDB.pod_isReady )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: podWorkerThread POD READY at startup - pushing event\n",
				 PODMODULE );
		// Temporarily set the isReady flag to FALSE while we process
		// POD_READY event in the handler below
		podDB.pod_isReady = FALSE;

		if ( rmf_osal_event_create
			 ( RMF_OSAL_EVENT_CATEGORY_POD, RMF_PODSRV_EVENT_READY, NULL,
			   &event_handle ) == RMF_SUCCESS )
		{
			if ( rmf_osal_eventqueue_add( podImplEvQ, event_handle ) !=
				 RMF_SUCCESS )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "%s: podWorkerThread,	eventQueueSend RMF_PODSRV_EVENT_READY failed.\n",
						 PODMODULE );
			}
		}
		else
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: podWorkerThread,	Could not create RMF_PODSRV_EVENT_READY event.\n",
					 PODMODULE );
		}

	}

	while ( !exitWorkerThread )
	{
		RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				 "%s: podWorkerThread starting waitNext -- podImplEvQ 0x%x (%d)\n",
				 PODMODULE, podImplEvQ, podImplEvQ );

		if ( ( retCode =
			   rmf_osal_eventqueue_get_next_event( podImplEvQ, &event_handle,
												   &event_category,
												   &event_type,
												   &eventData ) ) !=
			 RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: podWorkerThread, eventQueueWaitNext failed %d\n",
					 PODMODULE, retCode );
			return;
		}

		rmf_osal_event_delete( event_handle );

		RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				 "%s: podWorkerThread -- acquired DB mutex..\n", PODMODULE );

		{						// This should be a rare case. But we'll treat consecutive
			//	POD_READYs as a degenerate POD reset
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1

// updated POD implementation			 
			//rmf_osal_Bool podWasAlreadyReady = podDB.pod_isReady;

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: podWorkerThread: Processing event:0x%x\n",
					 PODMODULE, event_type );
#else

			rmf_osal_Bool podWasAlreadyReady = podDB.pod_isReady;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "%s: podWorkerThread: Processing event:0x%x\n",
					 PODMODULE, event_type );
#endif
/* Standard code deviation: ext: X1 end */
		}


		if ( RMF_PODSRV_EVENT_GF_UPDATE == event_type )
		{
			uint32_t featureID = ( uint32_t ) eventData.data;
			retCode = podGetFeatureParam( &podDB, featureID );
			if ( RMF_SUCCESS != retCode )
			{

				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "%s: podGetFeatureParam failed Error=%d\n",
						 PODMODULE, retCode );
			}
		}

		if ( RMF_PODSRV_EVENT_CCI_CHANGED == event_type )
		{

			VlCardManagerCCIData_t CciData;
			rmf_PodSrvDecryptSessionHandle Sessionhandle;
//			  rmf_osal_event_params_t sendEventData = {0};

			memcpy( &CciData, eventData.data,
					sizeof( VlCardManagerCCIData_t ) );
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, eventData.data );

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "%s: podWorkerThread: Processing RMF_PODSRV_EVENT_CCI_CHANGED CCI is %d\n",
					 PODMODULE, CciData.CCI );

			if ( !updateSessionInfo( &Sessionhandle, &CciData ) )
			{

				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "%s: podWorkerThread: Processing Unable to retrieve Session handle from LTSID %d\n",
						 PODMODULE, CciData.LTSID );
				continue;

			}
//			  sendEventData.data = (void *)CciData.CCI;
//			  sendEventData.data_extension = (uint32_t)Sessionhandle;
			sendDecryptSessionEvent( RMF_PODSRV_DECRYPT_EVENT_CCI_STATUS,
									 Sessionhandle,
									 ( uint32_t ) CciData.CCI );
			if ( ( 0 != CciData.CCIAuthStatus )
				 && ( 1 != CciData.CCIAuthStatus ) )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "%s: podWorkerThread: Unkown Auth value %d\n",
						 PODMODULE, CciData.CCIAuthStatus );
				continue;
			}

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "%s: podWorkerThread:	CCIAuthStatus is [%d]\n",
					 PODMODULE, CciData.CCIAuthStatus );
/*
			sendDecryptSessionEvent( CciData.
									 CCIAuthStatus ?
									 RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED
									 :
									 RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT,
									 Sessionhandle, 0 );

//			  sendPodEvent(RMF_PODSRV_EVENT_CCI_CHANGED, sendEventData);
*/
			uint8_t cciBits;
			podmgrGetCCIBits( Sessionhandle, &cciBits );
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: Retrieving CCI =%d\n",
					 PODMODULE, cciBits );

			continue;
		}


		if ( RMF_PODSRV_EVENT_DSGIP_ACQUIRED == event_type )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG IP is =%s\n",
					 PODMODULE, eventData.data );
		}

		if ( RMF_PODSRV_EVENT_INBAND_TUNE_START == event_type )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: RMF_PODSRV_EVENT_INBAND_TUNE_START received\n",
					 PODMODULE );
		}

		if ( POD_HOMING_FIRMUPGRADE == event_type )
		{
			Oob_ProgramInfo_t PInfo;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s: Rcvd RMF_PODSRV_EVENT_INB_FWU_START\n",
				PODMODULE);
			/* indicates only source id is present */
			if( eventData.data_extension == 0)
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s: RMF_PODSRV_EVENT_INB_FWU_START source id is %x \n",
					PODMODULE, eventData.data );
				if (RMF_SUCCESS != OobGetProgramInfo( (uint32_t)  eventData.data, &PInfo) )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
						"%s: failed OobGetProgramInfo for firmware upgrade\n",
						PODMODULE);
					continue;
				}				
				eventData.data = (void *)PInfo.carrier_frequency;
				eventData.data_extension = PInfo.modulation_mode;
			}
			event_type = RMF_PODSRV_EVENT_INB_FWU_START;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s: RMF_PODSRV_EVENT_INB_FWU_START Freq= %d, Mod = %d\n",
				PODMODULE, eventData.data, eventData.data_extension );
		}

		if ( POD_HOMING_FIRMUPGRADE_COMPLETE == event_type )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: Recieved RMF_PODSRV_EVENT_INB_FWU_COMPLETE event %d\n",
					 PODMODULE, eventData.data );
			event_type = RMF_PODSRV_EVENT_INB_FWU_COMPLETE;
		}
#if 1
		/*	need to filterout what all are the events to be excluded */
		if ( sendPodEvent( event_type, eventData ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: podWorkerThread,	eventQueueSend failed -  event_type=%d\n",
					 PODMODULE, event_type );
		}
#endif
		switch ( event_type )
		{
		  case RMF_PODSRV_EVENT_SHUTDOWN:
			  {
				  /* check for shutdown sent by the unregister method */
				  /* the unregister code that sent the shutdown handles closing down the JNI level */
				  rmf_osal_mutexAcquire( podDB.pod_mutex );
				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "%s: podWorkerThread: Processing RMF_PODSRV_EVENT_SHUTDOWN\n",
						   PODMODULE );

				  vlMpeosPodUnregister(  );

				  /* this level created the queue, it will also delete it */
				  if ( ( retCode =
						 rmf_osal_eventqueue_delete( podImplEvQ ) ) !=
					   RMF_SUCCESS )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread,  eventQueueDelete of queue failed\n",
							   PODMODULE );
					  rmf_osal_mutexRelease( podDB.pod_mutex );
					  return;
				  }

				  /* clear out saved info */
				  podImplEvQ = 0;

				  exitWorkerThread = true;
				  rmf_osal_mutexRelease( podDB.pod_mutex );

				  break;
			  }					// END case RMF_PODSRV_EVENT_SHUTDOWN
		  case RMF_PODSRV_EVENT_RECV_APDU:
			  {
				  /*
				   * do nothing here. Pass the event up to java where podRecvAPDU will be called.
				   * Do resource management in the podRecvAPDU call.
				   */
#ifdef CMCST_EXT_X1
				  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						   "%s: podWorkerThread: Processing RMF_PODSRV_EVENT_RECV_APDU \n",
						   PODMODULE );
#else
				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "%s: podWorkerThread: Processing RMF_PODSRV_EVENT_RECV_APDU \n",
						   PODMODULE );
#endif
				  break;
			  }					// END case RMF_PODSRV_EVENT_RECV_APDU
		  case RMF_PODSRV_EVENT_INSERTED:
			  {
				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "%s: podWorkerThread,  RMF_PODSRV_EVENT_INSERTED\n",
						   PODMODULE );
				  podDB.pod_isPresent = true;
				  break;
			  }					// END case RMF_PODSRV_EVENT_INSERTED
		  case RMF_PODSRV_EVENT_READY:
			  {

				  rmf_osal_mutexAcquire( podDB.pod_mutex );
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
//VL updated MPE POD modification
				  podDB.pod_isReady = TRUE;
				  podWasAlreadyReady = podDB.pod_isReady;
#else
				  /* Add the code here from br_1.1 code base  */
#endif
				  rmf_osal_mutexRelease( podDB.pod_mutex );

				  break;
			  }					// END case RMF_PODSRV_EVENT_READY
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
//updated POD implementation			
		  case RMF_PODSRV_EVENT_CA_SYSTEM_READY:
			  {
				  uint8_t apdu_buf[1] = { 0 };
				  uint32_t apdu_length = 0;

				  rmf_osal_mutexAcquire( podDB.pod_mutex );

				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "%s: podWorkerThread: Processing RMF_PODSRV_EVENT_CA_SYSTEM_READY\n",
						   PODMODULE );

				  /* get data on the card */
				  if ( ( retCode =
						 vlMpeosPodGetParam( RMF_PODMGR_PARAM_ID_MAX_NUM_ES,
											 &paramValue ) ) != RMF_SUCCESS )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread,  vlMpeosPodGetParam(RMF_PODMGR_PARAM_ID_MAX_NUM_ES) failed.  Error=%d\n",
							   PODMODULE, retCode );
				  }
				  else
				  {
					  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							   "%s: podWorkerThread,  vlMpeosPodGetParam(RMF_PODMGR_PARAM_ID_MAX_NUM_ES)=%d\n",
							   PODMODULE, paramValue );
					  podDB.pod_maxElemStreams = paramValue;
					  paramValue = 0;
				  }

				  if ( ( retCode =
						 vlMpeosPodGetParam
						 ( RMF_PODMGR_PARAM_ID_MAX_NUM_PROGRAMS,
						   &paramValue ) ) != RMF_SUCCESS )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread,  vlMpeosPodGetParam(RMF_PODMGR_PARAM_ID_MAX_NUM_PROGRAMS) failed.	Error=%d\n",
							   PODMODULE, retCode );
				  }
				  else
				  {
					  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							   "%s: podWorkerThread,  vlMpeosPodGetParam(RMF_PODMGR_PARAM_ID_MAX_NUM_PROGRAMS)=%d\n",
							   PODMODULE, paramValue );
					  podDB.pod_maxPrograms = paramValue;
					  paramValue = 0;
				  }
				  if ( ( retCode =
						 vlMpeosPodGetParam( RMF_PODMGR_PARAM_ID_MAX_NUM_TS,
											 &paramValue ) ) != RMF_SUCCESS )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread,  vlMpeosPodGetParam(RMF_PODMGR_PARAM_ID_MAX_NUM_TS) failed.  Error=%d\n",
							   PODMODULE, retCode );
				  }
				  else
				  {
					  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							   "%s: podWorkerThread,  vlMpeosPodGetParam(RMF_PODMGR_PARAM_ID_MAX_NUM_TS)=%d\n",
							   PODMODULE, paramValue );
					  podDB.pod_maxTransportStreams = paramValue;
					  paramValue = 0;
				  }

				  rmf_osal_mutexRelease( podDB.pod_mutex );

				  // Initialize the program index table and request table
				  rmf_osal_mutexAcquire( table_mutex );
				  initProgramIndexTable(  );
				  rmf_osal_mutexRelease( table_mutex );

				  rmf_osal_mutexAcquire( podDB.pod_mutex );

				  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						   "<%s::podWorkerThread>: Calling rmf_podmgrCASConnect \n",
						   PODMODULE );
				  if ( ( retCode =
						 rmf_podmgrCASConnect( &casSessionId,
											   &casResourceVersion ) ) !=
					   RMF_SUCCESS )
				  {
					  casSessionId = 0;
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread,  rmf_podmgrCASConnect.  Error=%d\n",
							   PODMODULE, retCode );
					  rmf_osal_mutexRelease( podDB.pod_mutex );

					  break;
				  }

				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "<%s::podWorkerThread> casSessionId: 0x%x...\n",
						   PODMODULE, casSessionId );

				  // Send ca_info_inquiry apdu
				  // to retrieve the CA_System_id
				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "<%s::podWorkerThread> - Sending CA_INFO_INQUIRY APDU...\n",
						   PODMODULE );

				  {
					  rmf_osal_Bool done = false;
					  uint8_t retry_count = g_podImpl_ca_retry_count;
					  while ( !done )
					  {
						  retCode =
							  rmf_podmgrSendAPDU( casSessionId,
												  RMF_PODMGR_CA_INFO_INQUIRY_TAG,
												  apdu_length, apdu_buf );
						  if ( retCode == RMF_SUCCESS )
						  {
							  done = true;
						  }
						  else
						  {
							  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
									   "<%s::podWorkerThread> - Error sending CA_INFO_INQUIRY (error %d)\n",
									   PODMODULE, retCode );
							  // Re-attempt sending the apdu if retry count is set
							  if ( retry_count )
							  {
								  retry_count--;
							  }

							  if ( retry_count == 0 )
							  {
								  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
										   "<%s::createAndSendCaPMTApdu> - Error sending CA_INFO_INQUIRY apdu after %d retries..\n",
										   PODMODULE,
										   g_podImpl_ca_retry_count );
								  done = true;
							  }
							  else
							  {
								  // Wait here before attempting a re-send
								  // Configurable via ini variable
								  rmf_osal_threadSleep
									  ( g_podImpl_ca_retry_timeout, 0 );
							  }
						  }
					  }			// End while(!done)
				  }

				  // v	v  v  v  v	v
				  // TEST CODE - REMOVE!!!!
				  //ca_system_id = 0x02;
				  // END TEST CODE!!!!
				  // ^	^  ^  ^  ^	^

//			  mpeos_memStats(FALSE, "LOG.RDK.POD",
//					  "DUMP AFTER PROGRAMINDEXTABLE MALLOC");

				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "%s: podWorkerThread: Completed programIndexTable initialization\n",
						   PODMODULE );

				  rmf_osal_mutexRelease( podDB.pod_mutex );


				  // A POD_READY event may be received following CableCARD reset
				  // Activate all suspended CA sessions
				  rmf_osal_mutexAcquire( table_mutex );
				  logProgramIndexTable
					  ( "podWorkerThread POD_CA_SYSTEM_READY" );
				  activateSuspendedCASessions(	);
				  rmf_osal_mutexRelease( table_mutex );
				  break;
			  }					// END case RMF_PODSRV_EVENT_CA_SYSTEM_READY
#endif
/* Standard code deviation: ext: X1 end */


		  case RMF_PODSRV_EVENT_REMOVED:
			  {
			  	
				  rmf_osal_Bool resetStatus = TRUE;
				  podmgrSASResetStat( TRUE, &resetStatus );				  
				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "%s: podWorkerThread: Processing RMF_PODSRV_EVENT_REMOVED\n",
						   PODMODULE );



				  if ( ( retCode =
						 rmf_osal_mutexAcquire( table_mutex ) !=
						 RMF_SUCCESS ) )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread, failed to get table mutex, error=%d\n",
							   PODMODULE, retCode );
					  return;
				  }

				  rmf_osal_mutexAcquire( podDB.pod_mutex );

				  removeAllDecryptSessionsAndResources
					  ( RMF_PODSRV_DECRYPT_EVENT_POD_REMOVED );

				  if ( programIndexTable.rows != NULL )
				  {
					  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							   "%s: programIndexTable row memory free, programIndexTable.rows=%p\n",
							   PODMODULE, programIndexTable.rows );

					  /*
					   * Release the programIndexTable mutex in memory free failure case.
					   */
					  if ( ( retCode =
							 rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
												programIndexTable.rows ) ) !=
						   RMF_SUCCESS )
					  {
						  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
								   "<%s::startDecrypt> - Could not free memory for programIndexTable.rows, error=%d\n",
								   PODMODULE, retCode );

						  rmf_osal_mutexRelease( podDB.pod_mutex );
						  rmf_osal_mutexRelease( table_mutex );
						  return;
					  }

					  programIndexTable.rows = NULL;
					  programIndexTable.numRows = 0;
					  programIndexTable.elementaryStreamsUsed = 0;
					  programIndexTable.programsUsed = 0;
					  programIndexTable.transportStreamsUsed = 0;
				  }

				  rmf_osal_mutexRelease( podDB.pod_mutex );

				  if ( ( retCode =
						 rmf_osal_mutexRelease( table_mutex ) !=
						 RMF_SUCCESS ) )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread, failed to release table mutex, error=%d\n",
							   PODMODULE, retCode );

					  return;
				  }

				  rmf_osal_mutexAcquire( podDB.pod_mutex );

				  podDB.pod_isPresent = false;
				  podDB.pod_isReady = false;
				  podDB.pod_maxElemStreams = 0;
				  podDB.pod_maxPrograms = 0;
				  podDB.pod_maxTransportStreams = 0;
				  rmf_osal_mutexRelease( podDB.pod_mutex );

				  ca_system_id = 0;

				  if ( ( retCode =
						 rmf_podmgrCASClose( casSessionId ) ) != RMF_SUCCESS )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread,  rmf_podmgrCASClose.  Error=%d\n",
							   PODMODULE, retCode );
				  }

				  casSessionId = 0;
				  break;
			  }					// END case RMF_PODSRV_EVENT_REMOVED
		  case RMF_PODSRV_EVENT_RESET_PENDING:
			  {
				  rmf_osal_Bool resetStatus = TRUE;			  	
				  podmgrSASResetStat( TRUE, &resetStatus );
				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "%s: podWorkerThread: Not Processing RMF_PODSRV_EVENT_RESET_PENDING\n",
						   PODMODULE );
				  break;
#if 0 /*Dead Code*/
				  if ( ( retCode =
						 rmf_osal_mutexAcquire( table_mutex ) !=
						 RMF_SUCCESS ) )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread, failed to get table mutex, error=%d\n",
							   PODMODULE, retCode );
					  return;
				  }

				  rmf_osal_mutexAcquire( podDB.pod_mutex );

				  podDB.pod_isReady = FALSE;
#ifdef CMCST_EXT_X1
				  podWasAlreadyReady = FALSE;
#endif
				  podDB.pod_maxElemStreams = 0;
				  podDB.pod_maxPrograms = 0;
				  podDB.pod_maxTransportStreams = 0;

				  // Suspend all active CA sessions
				  suspendActiveCASessions(	);

				  if ( programIndexTable.rows != NULL )
				  {
					  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							   "%s: programIndexTable row memory free, programIndexTable.rows=%p\n",
							   PODMODULE, programIndexTable.rows );

					  /*
					   * Release the programIndexTable mutex in memory free failure case.
					   */
					  if ( ( retCode =
							 rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
												programIndexTable.rows ) ) !=
						   RMF_SUCCESS )
					  {
						  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
								   "<%s::startDecrypt> - Could not free memory for programIndexTable.rows, error=%d\n",
								   PODMODULE, retCode );

						  rmf_osal_mutexRelease( podDB.pod_mutex );
						  rmf_osal_mutexRelease( table_mutex );
						  return;
					  }

					  programIndexTable.rows = NULL;
					  programIndexTable.numRows = 0;
					  programIndexTable.elementaryStreamsUsed = 0;
					  programIndexTable.programsUsed = 0;
					  programIndexTable.transportStreamsUsed = 0;
				  }

				  rmf_osal_mutexRelease( podDB.pod_mutex );

				  if ( ( retCode =
						 rmf_osal_mutexRelease( table_mutex ) !=
						 RMF_SUCCESS ) )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread, failed to release table mutex, error=%d\n",
							   PODMODULE, retCode );
				  }

				  ca_system_id = 0;

				  if ( ( retCode =
						 rmf_podmgrCASClose( casSessionId ) ) != RMF_SUCCESS )
				  {
					  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							   "%s: podWorkerThread,  rmf_podmgrCASClose.  Error=%d\n",
							   PODMODULE, retCode );
				  }

				  casSessionId = 0;
				  break;
#endif
			  }					// END case RMF_PODSRV_EVENT_RESET_PENDING
		  case RMF_PODSRV_EVENT_SEND_APDU_FAILURE:
			  {
				  //RDK_LOG(RDK_LOG_INFO,
				  //		"LOG.RDK.POD",
				  //		"%s: podWorkerThread: Ignoring RMF_PODSRV_EVENT_SEND_APDU_FAILURE\n",
				  //		PODMODULE);
				  break;
			  }
		  case RMF_PODSRV_EVENT_RESOURCE_AVAILABLE:
			  {
				  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						   "%s: podWorkerThread: Ignoring RMF_PODSRV_EVENT_RESOURCE_AVAILABLE\n",
						   PODMODULE );
				  break;
			  }
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_CA:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_CP:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_FWDNLD:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_XAIT:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_MONITOR:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_AUTO:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_SI:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_GENMSG:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_TRU2WAY_PROXY:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_XRE:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_FLASH_WRITE:
                case RMF_BOOTMSG_DIAG_EVENT_TYPE_MAX:
                case RMF_PODSRV_EVENT_GF_UPDATE:
                case RMF_PODSRV_EVENT_APPINFO_UPDATE:

                case RMF_PODSRV_EVENT_CCI_CHANGED:
                case RMF_PODSRV_EVENT_DSGIP_ACQUIRED:
                case RMF_PODSRV_EVENT_ENTITLEMENT:
                case RMF_PODSRV_EVENT_INB_FWU_START:
                case RMF_PODSRV_EVENT_INB_FWU_COMPLETE:
                case RMF_PODSRV_EVENT_INBAND_TUNE_START:
                case RMF_PODSRV_EVENT_INBAND_TUNE_COMPLETE:
                case RMF_PODSRV_EVENT_INBAND_TUNE_RELEASE:
			   {
#if 0
				/* Already posting event before switch. commenting this block*/
				if ( sendPodEvent( event_type, eventData ) != RMF_SUCCESS )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							 "%s: podWorkerThread,	eventQueueSend failed -  event_type=%d\n",
							 PODMODULE, event_type );
				}
#endif
			   }
			break;
		  default:
			  {
				  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						   "<%s::podWorkerThread: Received unknown event (%d)\n",
						   PODMODULE, event_type );
				  break;
			  }

		}						// END switch (event_type)
	}
	/* exit out of thread */
}

static rmf_Error initProgramIndexTable(  )
{
	rmf_Error retCode = RMF_SUCCESS;

	// Allocate program index table
	uint32_t programIndexTableLength = 0;
	programIndexTableLength =
		sizeof( podImpl_ProgramIndexTableRow ) * podDB.pod_maxPrograms;

	// Allocate program index table rows (size is equal to number of
	// programs the CableCARD can de-scramble simultaneously)
	programIndexTable.numRows = podDB.pod_maxPrograms;

	if ( programIndexTable.rows == NULL )
	{
		if ( ( retCode =
			   rmf_osal_memAllocP( RMF_OSAL_MEM_POD, programIndexTableLength,
								   ( void ** ) &programIndexTable.rows ) ) !=
			 RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: initProgramIndexTable, failed malloc memory for programIndexTable, error=%d\n",
					 PODMODULE, retCode );
			return retCode;
		}

		memset( programIndexTable.rows, 0, programIndexTableLength );
	}

	return RMF_SUCCESS;
}

static void initRequestTable(  )
{
	// Create new request table
	requestTable = llist_create(  );
}

void commitProgramIndexTableRow( uint8_t programIdx, uint8_t ca_pmt_cmd_id,
								 uint16_t progNum, uint16_t sourceId,
								 uint8_t transactionId, uint8_t tunerId,
								 uint8_t ltsid, uint8_t priority,
								 uint8_t numStreams, uint16_t ecmPid,
								 rmf_PodmgrStreamDecryptInfo streamInfo[],
								 uint8_t state
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
								 , rmf_osal_Bool isRecording
#endif
/* Standard code deviation: ext: X1 end */
	 )
{
	int i = 0;
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
	const char *skip_ca_query = NULL;
	char skip_ca_query_flag = 0;
    if(NULL == programIndexTable.rows)
    {
		RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD",
		"<%s::commitProgramIndexTableRow> Program Index Table array is NULL. Possibly due to CARD REMOVED event \n", PODMODULE);
		return;
    }
	if ( !isRecording )
	{
		skip_ca_query =
			( char * ) rmf_osal_envGet( "MPE_MPOD_CA_QUERY_DISABLED" );
		if ( ( ( NULL == skip_ca_query )
			   || ( strcasecmp( skip_ca_query, "TRUE" ) == 0 ) )
			 && priority != 31 )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "<%s::commitProgramIndexTableRow> - skip_ca_query_flag POD_CA_QUERY_ENABLED = TRUE\n",
					 PODMODULE );
			skip_ca_query_flag = true;
		}
		else
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "<%s::commitProgramIndexTableRow> - skip_ca_query_flag POD_CA_QUERY_ENABLED = FALSE\n",
					 PODMODULE );
			skip_ca_query_flag = false;
		}
	}

#endif
/* Standard code deviation: ext: X1 end */
	rmf_PodmgrStreamDecryptInfo *pidInfo;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::commitProgramIndexTableRow, programIdx=%d\n", PODMODULE,
			 programIdx );

	// Update the number of transport streams used
	if ( !tunerInUse( tunerId ) )
	{
		programIndexTable.transportStreamsUsed++;
	}
	// Update the number of programs used
	programIndexTable.programsUsed++;

	// Update the number of elementary streams used
	programIndexTable.elementaryStreamsUsed += numStreams;

	podImpl_ProgramIndexTableRow *rowPtr =
		&programIndexTable.rows[programIdx];

	rowPtr->ca_pmt_cmd_id = ca_pmt_cmd_id;
	rowPtr->programNum = progNum;
	rowPtr->transactionId = transactionId;
	rowPtr->ltsid = ltsid;
	rowPtr->sourceId = sourceId;
	rowPtr->priority = priority;
	rowPtr->state = ( RMF_PODMGRDecryptState ) state;
	rowPtr->ecmPid = ecmPid;
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
// VL updated POD implementation	
	if ( skip_ca_query_flag == true )
	{
		rowPtr->lastEvent = RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED;
	}
#endif
/* Standard code deviation: ext: X1 end */
	// Set authorized pids after receiving ca_pmt_reply/update
	rowPtr->authorizedPids = llist_create(	);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::commitProgramIndexTableRow, numStreams=%d\n", PODMODULE,
			 numStreams );

	for ( i = 0; i < numStreams; i++ )
	{
		LINK *lp;
		// Allocate a new entry
		if ( rmf_osal_memAllocP
			 ( RMF_OSAL_MEM_POD, sizeof( rmf_PodmgrStreamDecryptInfo ),
			   ( void ** ) &pidInfo ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "commitProgramIndexTableRow error allocating memory\n" );
			return;
		}
		pidInfo->pid = streamInfo[i].pid;
		pidInfo->status = streamInfo[i].status;
		lp = llist_mklink( ( void * ) pidInfo );
		llist_append( rowPtr->authorizedPids, lp );

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "commitProgramIndexTableRow - pid:%d ca_status:%d \n",
				 pidInfo->pid, pidInfo->status );
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s::commitProgramIndexTableRow transportStreamsUsed=%d programsUsed=%d elementaryStreamsUsed=%d\n",
			 PODMODULE, programIndexTable.transportStreamsUsed,
			 programIndexTable.programsUsed,
			 programIndexTable.elementaryStreamsUsed );
}

static rmf_osal_Bool tunerInUse( uint8_t tunerId )
{
	int index = 0;
	for ( index = 0; index < programIndexTable.numRows; index++ )
	{
		podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[index];
		if ( !rowPtr->tunerId && ( rowPtr->tunerId == tunerId ) )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s::tunerInUse returning TRUE given tuner id = %d, got tuner id =%d, index = %d\n",
					 PODMODULE, tunerId, rowPtr->tunerId, index );
			return TRUE;
		}
	}
	return FALSE;
}

// Combination of programIndex and transactionId uniquely identifies a response
// on an active decrypt session
// (caller has the table mutex)
static rmf_osal_Bool isValidReply( uint8_t programIdx, uint8_t transactionId )
{
	int index = 0;
	for ( index = 0; index < programIndexTable.numRows; index++ )
	{
		// Reply is always a response to a query
		podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[index];
		if ( ( index == programIdx )
			 && ( rowPtr->transactionId == transactionId )
			 && ( rowPtr->state == RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY ) )
			return TRUE;
	}
	return FALSE;
}

// Combination of programIndex and transactionId uniquely identifies a response
// on an active decrypt session
// (caller has the table mutex)
static rmf_osal_Bool isValidUpdate( uint8_t programIdx,
									uint8_t transactionId )
{
	int index = 0;
	for ( index = 0; index < programIndexTable.numRows; index++ )
	{
		// Update always is response to a ca pmt that is not a query
		// (what about RMF_PODMGR_DECRYPT_STATE_NOT_SELECTED?)
		podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[index];
		if ( ( index == programIdx )
			 && ( rowPtr->transactionId == transactionId )
			 && ( rowPtr->state != RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY ) )
			return TRUE;
	}
	return FALSE;
}

static int getNextAvailableProgramIndexTableRow( void )
{
	int idx;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s: getNextAvailableProgramIndexTableRow, programIndexTable.numRows=%d\n",
			 PODMODULE, programIndexTable.numRows );

	/* add a new session to an unused row */
	for ( idx = 0; idx < programIndexTable.numRows; idx++ )
	{
		podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[idx];
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s: getNextAvailableProgramIndexTableRow, &programIndexTable.rows[%d]=%p\n",
				 PODMODULE, idx, rowPtr );

		// Find a program index table row which is not in use
		if ( rowPtr->state == RMF_PODMGR_DECRYPT_STATE_NOT_SELECTED )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s: getNextAvailableProgramIndexTableRow, available index=%d\n",
					 PODMODULE, idx );
			return idx;
		}
	}
	return -1;
}

static rmf_osal_Bool checkAvailableResources( uint8_t tunerId,
											  uint8_t numStreams )
{
	uint8_t tunersNeeded;

	rmf_osal_mutexAcquire( table_mutex );
	if(NULL == programIndexTable.rows)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
			"<%s::checkAvailableResources> Program Index Table array is NULL. Possibly due to CARD REMOVED event \n", PODMODULE);
		rmf_osal_mutexRelease( table_mutex );	
		return FALSE;
	}
	
	if ( !programIndexTable.transportStreamsUsed )		// None used
		tunersNeeded = 1;
	else
		tunersNeeded =
			tunerInUse( tunerId ) ? programIndexTable.transportStreamsUsed : ( programIndexTable.transportStreamsUsed )++;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s: checkAvailableResources, numTuners=%d numPrograms=%d numStreams=%d podDB.pod_maxPrograms =%d,podDB.pod_maxElemStreams=%d\n",
			 PODMODULE, tunersNeeded,
			 ( ( programIndexTable.programsUsed ) + 1 ),
			 ( ( programIndexTable.elementaryStreamsUsed ) + numStreams ),
			 podDB.pod_maxPrograms, podDB.pod_maxElemStreams );

	/* check the total number of discrete tuners in use and test against
	 * podDB.pod_maxTransportStreams */
	/* tunersNeeded >= podDB.pod_maxTransportStreams || */
	if ( ( ( programIndexTable.programsUsed ) + 1 ) <= podDB.pod_maxPrograms
		 && ( ( programIndexTable.elementaryStreamsUsed ) + numStreams ) <=
		 podDB.pod_maxElemStreams )
	{
		rmf_osal_mutexRelease( table_mutex );	
		return TRUE;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s: getResourcesNeededForRequest, resource limit exceeded..\n",
			 PODMODULE );
	rmf_osal_mutexRelease( table_mutex );		
	return FALSE;
}

static rmf_Error releaseDecryptRequest( podImpl_RequestTableRow * request )
{
	rmf_Error retCode = RMF_SUCCESS;
	LINK *lp = NULL;

	if ( request == NULL )
		return RMF_OSAL_EINVAL;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s: releaseDecryptRequest, request=0x%p\n", PODMODULE,
			 request );

	lp = llist_linkof( requestTable, ( void * ) request );
	llist_rmlink( lp );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s: releaseDecryptRequest, requestTable count=%lu\n", PODMODULE,
			 llist_cnt( requestTable ) );

	if ( ( retCode =
		   rmf_osal_memFreeP( RMF_OSAL_MEM_POD, request ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: releasePODDecryptSession, FAIL to free memory located at request=%p\n",
				 PODMODULE, request );
	}

	return retCode;
}

static rmf_Error releaseProgramIndexRow( uint8_t programIdx )
{
	uint8_t transId;
    if(NULL == programIndexTable.rows)
    {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
			"<%s::releaseProgramIndexRow> Program Index Table array is NULL. Possibly due to CARD REMOVED event \n", PODMODULE);
		return RMF_OSAL_EINVAL;
    }	
	podImpl_ProgramIndexTableRow *rowPtr =
		&programIndexTable.rows[programIdx];
	LINK *lp;

	if ( programIndexTable.programsUsed )
	{
		programIndexTable.programsUsed--;
	}
	else
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.POD",
				 "%s::releaseProgramIndexRow programIdx = %d, rowPtr->programsUsed was 0...\n",
				 PODMODULE, programIdx );
	}

	if ( rowPtr->authorizedPids != NULL )
	{
		if ( llist_cnt( rowPtr->authorizedPids ) >
			 programIndexTable.elementaryStreamsUsed )
		{
			RDK_LOG( RDK_LOG_WARN, "LOG.RDK.POD",
					 "%s::releaseProgramIndexRow programIdx = %d, authorizedPids count invalid...\n",
					 PODMODULE, programIdx );
			programIndexTable.elementaryStreamsUsed = 0;
		}
		else
		{
			programIndexTable.elementaryStreamsUsed -=
				llist_cnt( rowPtr->authorizedPids );
		}

		/* memory leak fix */
		lp = llist_first( rowPtr->authorizedPids );

		while ( lp )
		{
			rmf_PodmgrStreamDecryptInfo *pidInfo =
				( rmf_PodmgrStreamDecryptInfo * ) llist_getdata( lp );
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pidInfo );
			lp = llist_after( lp );
		}


		// De-allocate authorized Pids list
		llist_free( rowPtr->authorizedPids );
		rowPtr->authorizedPids = NULL;

	}
	else
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.POD",
				 "%s::releaseProgramIndexRow programIdx = %d, rowPtr->authorizedPids was NULL...\n",
				 PODMODULE, programIdx );
	}

	if ( programIndexTable.transportStreamsUsed )
	{
		programIndexTable.transportStreamsUsed--;
	}
	else
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.POD",
				 "%s::releaseProgramIndexRow programIdx = %d, rowPtr->transportStreamsUsed was 0...\n",
				 PODMODULE, programIdx );
	}

	// save transaction Id
	transId = rowPtr->transactionId;

	memset( rowPtr, 0, sizeof( podImpl_ProgramIndexTableRow ) );

	// restore transaction Id
	rowPtr->transactionId = transId;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s::releaseProgramIndexRow programsUsed=%d elementaryStreamsUsed=%d\n",
			 PODMODULE, programIndexTable.programsUsed,
			 programIndexTable.elementaryStreamsUsed );

	return RMF_SUCCESS;
}

rmf_Error releasePODDecryptSessionAndResources( uint8_t programIdx,
												rmf_osal_Bool
												sendSessionEvent,
												rmf_PODSRVDecryptSessionEvent
												event,
												rmf_osal_Bool
												sendResourceEvent )
{
	rmf_Error retCode = RMF_SUCCESS;
	rmf_PodSrvDecryptSessionHandle releasedSessionHandle;
	uint8_t numHandles = 0;
	int i;
	// Random size array
	rmf_PodSrvDecryptSessionHandle handles[10];
	rmf_osal_event_params_t eventData = { 0 };

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s::releasePODDecryptSessionAndResources for programIdx:%d .\n",
			 PODMODULE, programIdx );

	// There may be multiple requests per program index table row
	if ( ( retCode =
		   getDecryptSessionsForProgramIndex( programIdx, &numHandles,
											  handles ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s::handleCaEnable error retrieving decrypt sessions for program index:%d.\n",
				 PODMODULE, programIdx );
		return retCode;
	}

	for ( i = 0; i < numHandles; i++ )
	{
		releasedSessionHandle = handles[i];

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: releasePODDecryptSession, releasedSessionHandle=%p\n",
				 PODMODULE, releasedSessionHandle );



		if ( sendSessionEvent && retCode == RMF_SUCCESS )
		{
			retCode =
				sendDecryptSessionEvent( event, releasedSessionHandle, 0 );
		}


		if ( sendResourceEvent && retCode == RMF_SUCCESS )
		{

			eventData.data = (void *)releasedSessionHandle;
			sendPodEvent( RMF_PODSRV_EVENT_RESOURCE_AVAILABLE, eventData );
		}
	}
	return retCode;
}

static void removeUnusedRequestFromRequestTable( char *localStr )
{
    int idx = 0;
	int released = 0;
    podImpl_RequestTableRow *row = NULL;
    LINK *lp = NULL;

    if ( requestTable == NULL )
    {
        return;
    }
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s::removeUnusedRequestFromRequestTable %s\n",
             PODMODULE, localStr );

    lp = llist_first( requestTable );
    while ( lp )
    {
        row = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		released = 0;
        if ( row != NULL )
        {
            RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
                     "\t<requestTable[%d]>\t: programIndex=%d, tunerId=%d, ltsId=%d, priority=%d, sourceId=0x%x, state=%s mmiEnable=%d serviceHandle=0x%x\n",
                     idx, row->programIdx, row->tunerId, row->ltsId,
                     row->priority, row->sourceId,
                     requestTableStateString( row->state ), row->mmiEnable,
                     row->pServiceHandle );
	    	if(255 == row->programIdx)
	    	{
			    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "WARNING : programIdx is 255, clearing the \t<requestTable[%d]>\t\n", idx);
        		lp = llist_after( lp );
			    releaseDecryptRequest( row);
				released = 1;
			    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "released decrypt request for \t<requestTable[%d]>\t\n", idx);
	    	}
            idx++;
        }
		if(!released)
        	lp = llist_after( lp );
    }
}

/* designed to be called inside a acquire mutex block */
void removeAllDecryptSessionsAndResources( rmf_PODSRVDecryptSessionEvent
										   event )
{
	int idx;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s: releasePODDecryptSession, programIndexTable.numRows=%d\n",
			 PODMODULE, programIndexTable.numRows );

	/*	When card is removed, signal to all sessions that the cable card has been removed */
	for ( idx = 0; idx < programIndexTable.numRows; idx++ )
	{
		podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[idx];
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: releasePODDecryptSession, &programIndexTable.rows[%d]=%p\n",
				 PODMODULE, idx, rowPtr );

		//if (rowPtr->sessionHandle != NULL)
		// {
		/* remove and signal event passed in. */
		//	  releasePODDecryptSessionAndResources(idx, TRUE, event, FALSE);
		//}
	}
	/*  
	** Calling suspendActiveCASessions () and removeUnusedRequestFromRequestTable () 
	*/
	// Suspend all active CA sessions
    suspendActiveCASessions(  );
	removeUnusedRequestFromRequestTable( "removeUnusedRequestFromRequestTable" );

}

static rmf_Error getDecryptSessionsForProgramIndex( uint8_t programIdx,
													uint8_t * numHandles,
													rmf_PodSrvDecryptSessionHandle
													handle[] )
{
	LINK *lp = NULL;
	int i = 0;

	if ( handle == NULL )
		return RMF_OSAL_EINVAL;

	if ( requestTable == NULL )
	{
		return RMF_SUCCESS;
	}

	// Finds all the matching requests with the program index
	lp = llist_first( requestTable );
	while ( lp )
	{
		podImpl_RequestTableRow *row =
			( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( row && ( row->programIdx == programIdx ) )
		{
			handle[i++] = ( rmf_PodSrvDecryptSessionHandle ) row;
		}
		lp = llist_after( lp );
	}

	*numHandles = i;
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::getDecryptSessionsForProgramIndex numHandles:%d\n",
			 PODMODULE, *numHandles );
	return RMF_SUCCESS;
}

static rmf_Error getDecryptRequestProgramIndex( rmf_PodSrvDecryptSessionHandle
												handle,
												uint8_t * programIdxPtr )
{
	podImpl_RequestTableRow *row = ( podImpl_RequestTableRow * ) handle;

	if ( handle == NULL )
		return RMF_OSAL_EINVAL;

	*programIdxPtr = row->programIdx;

	return RMF_SUCCESS;
}

/*
 * This methods checks if the given program index is in use
 * by any of the decryt requests.
 */
/* commenting since it is not being used for RDK 2.0 as all the resource handling are done at qamsrc level */
#if 0
static rmf_osal_Bool isProgramIndexInUse( uint8_t programIdx )
{
	podImpl_RequestTableRow *row = NULL;
	LINK *lp = NULL;
    if(NULL == programIndexTable.rows)
    {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
		"<%s::isProgramIndexInUse> Program Index Table array is NULL. Possibly due to CARD REMOVED event \n", PODMODULE);
		return FALSE;
    }	
	podImpl_ProgramIndexTableRow *rowPtr =

		&programIndexTable.rows[programIdx];

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s::isProgramIndexInUse programIdx:%d\n", PODMODULE,
			 programIdx );

	lp = llist_first( requestTable );
	while ( lp )
	{
		// State check is not necessary
		row = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( row->programIdx == programIdx
			 && ACTIVE_DECRYPT_STATE( rowPtr->state ) )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "%s::isProgramIndexInUse returning TRUE\n", PODMODULE );
			return TRUE;
		}
		lp = llist_after( lp );
	}
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::isProgramIndexInUse returning FALSE\n", PODMODULE );
	return FALSE;
}


static void updateProgramIndexTablePriority( uint8_t idx, uint8_t priority )
{
	podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[idx];
	if ( rowPtr->priority < priority )
		rowPtr->priority = priority;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::updateProgramIndexTablePriority priority:%d\n", PODMODULE,
			 rowPtr->priority );
}

static void updatePriority( uint8_t idx )
{
	uint8_t saved = 0;
	podImpl_RequestTableRow *request = NULL;
	podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[idx];
	LINK *lp = NULL;

	saved = rowPtr->priority;
	lp = llist_first( requestTable );
	while ( lp )
	{
		request = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( request->programIdx == idx )
		{
			if ( request->priority < saved )
			{
				saved = rowPtr->priority = request->priority;
			}
		}
		lp = llist_after( lp );
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s::updatePriority priority:%d\n",
			 PODMODULE, rowPtr->priority );
}

#endif
rmf_Error sendPodEvent( uint32_t event, rmf_osal_event_params_t eventData )
{
	rmf_Error retCode = RMF_SUCCESS;
	rmf_osal_event_handle_t event_handle;

	/* notify all listeners */
	notifyQAMSrcLevelRegisteredPodEventQueues( event, eventData );
	return retCode;
}

/* commenting since it is not being used for RDK 2.0 as all the resource handling are done at qamsrc level */	
#if 0
/*
 * This methods finds an active request based on the tuner number and program number
 * for sharing with an incoming request.
 */
static rmf_Error findActiveDecryptRequestForReuse( uint8_t tunerId,
												   uint16_t programNumber,
												   int *index )
{
	podImpl_RequestTableRow *request = NULL;
	LINK *lp = NULL;

	if ( tunerId == 0 || programNumber == 0 )
		return RMF_OSAL_EINVAL;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::findActiveDecryptRequestForReuse tunerId:%d programNumber:%d\n",
			 PODMODULE, tunerId, programNumber );

	lp = llist_first( requestTable );
	while ( lp )
	{
		request = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( request != NULL )
		{
			podImpl_ProgramIndexTableRow *rowPtr =
				&programIndexTable.rows[request->programIdx];

			// Match based on tunerId, program number and state
			if ( request->tunerId == tunerId
				 && rowPtr->programNum == programNumber
				 && request->state ==
				 RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE )
			{
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						 "%s::findActiveDecryptRequestForReuse returning program index:%d \n",
						 PODMODULE, request->programIdx );
				*index = request->programIdx;
				return RMF_SUCCESS;
			}
		}
		lp = llist_after( lp );
	}
	return RMF_OSAL_EINVAL;
}

#ifdef CMCST_EXT_X1

static rmf_Error findActiveDecryptRequestForReuseExtended(decryptRequestParams *decryptRequestPtr, uint16_t programNumber, int *index)
{
    podImpl_RequestTableRow *request = NULL;
    LINK *lp = NULL;
    uint8_t tunerId;
    int i,j;
    rmf_osal_Bool found=false;;
    
    tunerId = decryptRequestPtr->tunerId;
    
    if (tunerId == 0 || programNumber == 0)
        return RMF_OSAL_EINVAL;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s::findActiveDecryptRequestForReuseExtended tunerId:%d programNumber:%d\n", PODMODULE,
            tunerId, programNumber);

    lp = llist_first(requestTable);
    
    while (lp)
    {
        request = (podImpl_RequestTableRow *) llist_getdata(lp);
        
	if (request != NULL)
        {
	    podImpl_ProgramIndexTableRow* rowPtr = &programIndexTable.rows[request->programIdx];
	  
	    // Match based on tunerId, program number and state
	    if(request->tunerId == tunerId
                    &&  rowPtr->programNum == programNumber
                    &&  request->state == RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE)
	    {
		// Copy PIDs from decryptRequestParams to newRequest
		for (i = 0; i < decryptRequestPtr->numPids; i++)
		{
		    found = false;
		    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s::findActiveDecryptRequestForReuseExtended :decryptRequestPtr->pids[%d].pidType=%d,decryptRequestPtr->pids[%d].pid= %d,request->numPids=%d\n", 
			PODMODULE,i,decryptRequestPtr->pids[i].pidType,i,decryptRequestPtr->pids[i].pid,request->numPids);
		    for(j=0; j< request->numPids;j++)
		    {
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s::findActiveDecryptRequestForReuseExtended :request->pids[%d].pidType=%d,request->pids[%d].pid=%d \n", 
			PODMODULE,j,request->pids[j].pidType,j,request->pids[j].pid);
		
			if(decryptRequestPtr->pids[i].pidType == request->pids[j].pidType)
			{
			    if(decryptRequestPtr->pids[i].pid == request->pids[j].pid)
			    {
				found = true;
			    }
			}
		    }
		    if(!found ||(found && (i+1) == request->numPids))
		      break;
		    
		}
	    
	      if(found)
	      {
		  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s::findActiveDecryptRequestForReuseExtended returning program index:%d \n", PODMODULE, request->programIdx);
		  *index = request->programIdx;
		  return RMF_SUCCESS;
	      }
	    }
	}
        lp = llist_after(lp);
    }
    return RMF_OSAL_EINVAL;
}
#endif

static uint8_t getSuspendedRequests(  )
{
	LINK *lp = NULL;
	uint8_t count = 0;
	podImpl_RequestTableRow *request = NULL;
	lp = llist_first( requestTable );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s::getSuspendedRequests...\n",
			 PODMODULE );

	while ( lp )
	{
		request = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( request != NULL
			 && request->state ==
			 RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES )
		{
			count++;
		}
		lp = llist_after( lp );
	}
	return count;
}
#endif
/*
 * This methods finds a suspended request with the highest priority.
 * Returns the associated tunerId and service handle.
 */
static rmf_Error findNextSuspendedRequest( uint8_t * tunerId,
										   parsedDescriptors ** handle )
{
	podImpl_RequestTableRow *request = NULL;
	uint8_t saved = 0;
	int found = 0;
	LINK *lp = NULL;
	rmf_Error retCode = RMF_SUCCESS;
	parsedDescriptors *pTempHandle;

	if ( requestTable == NULL )
	{
		return RMF_SUCCESS;
	}
	lp = llist_first( requestTable );

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s::findSuspendedRequest...\n",
			 PODMODULE );

	while ( lp )
	{
		request = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( request != NULL
			 && request->state ==
			 RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES )
		{
			if ( request->priority > saved )
			{
				saved = request->priority;
				*tunerId = request->tunerId;
				*handle = request->pServiceHandle;
				pTempHandle = request->pServiceHandle;

				if ( checkAvailableResources( *tunerId, pTempHandle->noES ) )
				{
					// We can service this request with available resources
					found = 1;
				}
			}
		}
		lp = llist_after( lp );
	}

	if ( found )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s::findSuspendedRequest...returning tunerId:%d handle:0x%x priority:%d\n",
				 PODMODULE, *tunerId, *handle, saved );
		return RMF_SUCCESS;
	}
	return RMF_OSAL_ENODATA;
}

/*
 * This methods appends the new request to the request table
 */
static void addRequestToTable( podImpl_RequestTableRow * newRequest )
{
	LINK *lp = llist_mklink( ( void * ) newRequest );
	llist_append( requestTable, lp );

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "<%s::addRequestToTable> - New request added to table, count=%lu\n",
			 PODMODULE, llist_cnt( requestTable ) );
}


/*
 * This methods removes a request from the request table.
 * The link is removed from the table but not de-allocated.
 */
static void removeRequestFromTable( podImpl_RequestTableRow * request )
{
	LINK *lp = NULL;
	if ( request == NULL )
		return;
	lp = llist_linkof( requestTable, ( void * ) request );
	if ( lp )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: removeRequestFromTable, request=0x%p\n", PODMODULE,
				 request );

		llist_rmlink( lp );
		llist_freelink( lp );
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "<%s::removeRequestFromTable> - request table count=%lu\n",
				 PODMODULE, llist_cnt( requestTable ) );
	}
}

static rmf_osal_Bool updateSessionInfo( rmf_PodSrvDecryptSessionHandle *
										handle,
										VlCardManagerCCIData_t * pCciData )
{
	podImpl_RequestTableRow *row = NULL;
	rmf_Error err;
	rmf_osal_Bool retVal = FALSE;
	if ( ( err = rmf_osal_mutexAcquire( table_mutex ) != RMF_SUCCESS ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: podWorkerThread, failed to get table mutex, error=%d\n",
				 PODMODULE, err );
		return FALSE;
	}


	LINK *lp = llist_first( requestTable );
	while ( lp )
	{
		row = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( row != NULL )
		{
			if ( ( row->ltsId == pCciData->LTSID )
				 && ( programIndexTable.rows[row->programIdx].programNum ==
					  pCciData->prgNum ) )
			{
				row->CCI = (int16_t) pCciData->CCI;
				row->EMI = pCciData->EMI;
				row->APS = pCciData->APS;
				row->CIT = pCciData->CIT;
				row->RCT = pCciData->RCT;
				row->CCIAuthStatus = pCciData->CCIAuthStatus;
				*handle = ( rmf_PodSrvDecryptSessionHandle ) row;
				retVal = TRUE;
				break;
			}
		}
		lp = llist_after( lp );
	}

	if ( ( err = rmf_osal_mutexRelease( table_mutex ) != RMF_SUCCESS ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: podWorkerThread, failed to get table mutex, error=%d\n",
				 PODMODULE, err );
		return FALSE;
	}

	return retVal;


}

/**
 * <i>rmf_podmgrGetCCIBits<i/>
 *
 * Get the Copy Control Information for the specified service.
 *
 * @param frequency the frequency of the transport stream on which the desired
 *		  service is carried
 * @param programNumber the program number of the desired service
 * @param tsID the transport stream ID associated with the transport stream on
 *		  which the desired service is carried
 * @param cciBits ouput pointer where the requested CCI bits are returned
 *
 * @return RMF_SUCCESS if the feature parameter was successfully retrieved, RMF_OSAL_EINVAL
 * if the service information or the return parameter is invalid, RMF_OSAL_ENODATA if the
 * specified service is not tuned or CCI data is not available
 */
rmf_Error podmgrGetCCIBits( rmf_PodSrvDecryptSessionHandle handle,
								uint8_t * pCciBits )
{
	podImpl_RequestTableRow *row = NULL;

	rmf_Error err = RMF_OSAL_ENODATA;
	rmf_Error ret = RMF_OSAL_ENODATA;
	
	if ( ( err = rmf_osal_mutexAcquire( table_mutex ) != RMF_SUCCESS ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: rmf_podmgrGetCCIBits, failed to get table mutex, error=%d\n",
				 PODMODULE, err );
		return err;
	}

	LINK *lp = llist_first( requestTable );
	while ( lp )
	{
		row = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( row != NULL )
		{
			if ( ( rmf_PodSrvDecryptSessionHandle ) row == handle )
			{
				if( -1 == row->CCI )
				{
					ret = RMF_OSAL_ENODATA;
					break;
				}
				*pCciBits = row->CCI;
				ret = RMF_SUCCESS;
				break;
			}
		}
		lp = llist_after( lp );
	}

	if ( ( err = rmf_osal_mutexRelease( table_mutex ) != RMF_SUCCESS ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: rmf_podmgrGetCCIBits, failed to get table mutex, error=%d\n",
				 PODMODULE, err );
		return err;
	}
	
	return ret;
}

/*
 * This methods finds the given decrypt request (identified by the handle)
 * in the request table.
 */
static rmf_osal_Bool findRequest( rmf_PodSrvDecryptSessionHandle handle )
{
	podImpl_RequestTableRow *row = NULL;
	LINK *lp = llist_first( requestTable );
	while ( lp )
	{
		row = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( row != NULL )
		{
			if ( ( rmf_PodSrvDecryptSessionHandle ) row == handle )
			{
				return TRUE;
			}
		}
		lp = llist_after( lp );
	}
	return FALSE;
}
/* commenting since it is not being used for RDK 2.0 as all the resource handling are done at qamsrc level */
#if 0

static rmf_Error
findRequestProgramIndexWithLowerPriority( podImpl_RequestTableRow *
										  requestPtr, int *programIdx )
{
	uint8_t saved;
	int found = 0;
	int idx;
	saved = requestPtr->priority;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<%s::findRequestProgramIndexWithLowerPriority> ...\n",
			 PODMODULE );

	for ( idx = 0; idx < programIndexTable.numRows; idx++ )
	{
		podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[idx];
		// Find a program index table row that has a lower priority.
		// and state is one of the active states
		if ( rowPtr->priority && ( rowPtr->priority < saved )
			 && ACTIVE_DECRYPT_STATE( rowPtr->state ) )
		{
			saved = rowPtr->priority;
			*programIdx = idx;
			found = 1;
		}
	}
	if ( found )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "<%s::findRequestProgramIndexWithLowerPriority> - Found priority = %d @ programIdx = %d\n",
				 PODMODULE, saved, *programIdx );
		return RMF_SUCCESS;
	}
	return RMF_OSAL_EINVAL;
}
#endif

static rmf_Error activateSuspendedRequests( uint8_t programIndex )
{
	uint8_t tunerId = 0;
	uint8_t transactionId;
	uint8_t priority = 0;
	parsedDescriptors *pSeviceHandle = NULL;
	uint8_t ca_cmd_id = 0;
	podImpl_RequestTableRow *request = NULL;
	rmf_Error retCode = RMF_SUCCESS;
	LINK *lp;
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
//VL updated POD implementation    
	rmf_osal_Bool isRecording = 0;
#endif
	uint8_t ltsId = LTSID_UNDEFINED;


	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "<%s::activateSuspendedRequests> - Enter...programIndex: %d\n",
			 PODMODULE, programIndex );

	// Find a suspended request that can be serviced with the available resources
	if ( ( retCode =
		   findNextSuspendedRequest( &tunerId,
									 &pSeviceHandle ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "<%s::activateSuspendedRequests> - No suspended requests found...\n",
				 PODMODULE );
		return retCode;
	}

	lp = llist_first( requestTable );

	while ( lp )
	{
		request = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( request != NULL )
		{
			// Find requests that share common data (servicehandle, tunerId etc.)
			// Priorities may be different, but the higher priority will be
			// represented in the program index table row
			if ( request->tunerId == tunerId
				 && request->pServiceHandle == pSeviceHandle )
			{
				// Set the program index and state
				request->programIdx = programIndex;
				request->state = RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE;
				// priority of the program index table row should be higher of the request priorities
				// if there are multiple requests sharing a session
				if ( request->priority > priority )
					priority = request->priority;
				tunerId = request->tunerId;
				// When re-activating suspended requests
				// Use a new 'random' ltsid

/* Need to clear with the original implementer  */
#if 0
				if ( ltsId == LTSID_UNDEFINED )
				{
					ltsId = getLtsidForTunerId( request->tunerId );
					// If an existing ltsid for this tuner is not found get the next 'random' ltsid
					if ( request->ltsId == LTSID_UNDEFINED )
					{
						// getLtsid() validates that the new ltsid is not in use by any other
						// program index rows
						ltsId = getLtsid(  );
					}
				}
#endif
				// Requests sharing servicehandle and tunerId share the same ltsid
				request->ltsId = ltsId;
				pSeviceHandle = request->pServiceHandle;
				ca_cmd_id = request->ca_pmt_cmd_id;
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
//VL updated POD implementation				   
				isRecording = request->isRecording;
#endif
/* Standard code deviation: ext: X1 end */
			}
		}
		lp = llist_after( lp );
	}

	// Check if there are any CA descriptors for this handle
	// This method populates elementary stream Pids associated with
	// the service. The CA status field is set to unknown. It will
	// be filled when ca reply/update is received from CableCARD.
	if ( pSeviceHandle->caDescFound != 1 )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "<%s::activateSuspendedRequests> - No CA descriptors for service handle: 0x%x (could be analog, or clear channel..\n",
				 PODMODULE, pSeviceHandle );

		return RMF_OSAL_EINVAL;
	}


	// Done with SI DB, release the lock

	// Set the transaction id
	transactionId = getNextTransactionIdForProgramIndex( programIndex );

	// If the request was never started the command_id is 0
	// Set it to query
	if ( ca_cmd_id == 0 )
	{
		ca_cmd_id = RMF_PODMGR_CA_QUERY;
	}

	if ( ( retCode =
		   createAndSendCaPMTApdu( pSeviceHandle, programIndex, transactionId,
								   ltsId, ca_cmd_id ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "<%s::activateSuspendedRequests> - Error generating CA_PMT (error 0x%x)\n",
				 PODMODULE, retCode );
	}
	else
	{
		// set state based on the previous command id
		commitProgramIndexTableRow( programIndex, ca_cmd_id,
									pSeviceHandle->prgNo,
									pSeviceHandle->srcId, transactionId,
									tunerId, ltsId, priority,
									pSeviceHandle->noES,
									pSeviceHandle->ecmPid,
									pSeviceHandle->pESInfo,
									caCommandToState( ca_cmd_id )
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
									, isRecording
#endif
/* Standard code deviation: ext: X1 end */
			 );

		// If it was de-scrambling earlier we are going to issue a
		// RMF_PODMGR_CA_OK_DESCRAMBLE apdu and signal an event
		if ( ca_cmd_id == RMF_PODMGR_CA_OK_DESCRAMBLE )
		{
			// Set all Pids state to authorized
			LINK *lp1;
			/* Not adding NULL checks here since it is done from the caller itself  */
			podImpl_ProgramIndexTableRow *rowPtr =
				&programIndexTable.rows[programIndex];
			lp1 = llist_first( rowPtr->authorizedPids );
			while ( lp1 )
			{
				rmf_PodmgrStreamDecryptInfo *pidInfo =
					( rmf_PodmgrStreamDecryptInfo * ) llist_getdata( lp1 );
				if ( pidInfo )
				{
					pidInfo->status =
						RMF_PODMGR_CA_ENABLE_DESCRAMBLING_NO_CONDITIONS;
				}
				lp1 = llist_after( lp1 );
			}
			sendEventImpl( RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED,
						   programIndex );
		}
	}

	// De-allocate pSeviceHandle->pStreamPidInfo
	if ( pSeviceHandle->pESInfo != NULL )
	{
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pSeviceHandle->pESInfo );
		pSeviceHandle->pESInfo = NULL;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "<%s::activateSuspendedRequests> - programIndex:%d, done...\n",
			 PODMODULE, programIndex );

	return retCode;
}

static uint8_t caCommandToState( uint8_t cmd_id )
{
	if ( cmd_id == RMF_PODMGR_CA_OK_DESCRAMBLE )
		return RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING;
	else if ( cmd_id == RMF_PODMGR_CA_OK_MMI )
		return RMF_PODMGR_DECRYPT_STATE_ISSUED_MMI;
	else if ( cmd_id == RMF_PODMGR_CA_QUERY )
		return RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY;
	else if ( cmd_id == RMF_PODMGR_CA_NOT_SELECTED )
		return RMF_PODMGR_DECRYPT_STATE_NOT_SELECTED;
	else
		return RMF_PODMGR_DECRYPT_STATE_NOT_SELECTED;
}


/*
 * This methods returns the current transaction id for the given program index.
 */
static uint8_t getTransactionIdForProgramIndex( uint8_t programIdx )
{
	if( NULL == programIndexTable.rows  )
	{
		return 0;
	}
	podImpl_ProgramIndexTableRow *rowPtr =
		&programIndexTable.rows[programIdx];

	return rowPtr->transactionId;
}

/*
 * This methods returns the next transaction id for the given program index.
 *
 * Transaction id is an 8-bit value generated by the host and is returned
 * in the ca_pmt_reply() and/or ca_update() apdu from the CableCARD.
 * The host should increment the value, modulo 255, with every message.
 * A separate transaction id counter is maintained for each program index,
 * so that the transaction ids increment independently for each index.
 */
static uint8_t getNextTransactionIdForProgramIndex( uint8_t programIdx )
{
	uint8_t transactionId = getTransactionIdForProgramIndex( programIdx ) + 1;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "<%s::getNextTransactionIdForProgramIndex> - transaction id=%d\n",
			 PODMODULE, ( transactionId % 255 ) );

	return ( transactionId % 255 );
}

static void logProgramIndexTable( char *localStr )
{
	int idx;
	int i;
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s::programIndexTable %s\n",
			 PODMODULE, localStr );

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "\t<programIndexTable>\t\t: rowsUsed=%d, numRows=%d\n",
			 programIndexTable.programsUsed, programIndexTable.numRows );

	for ( idx = 0; idx < programIndexTable.numRows; idx++ )
	{
		podImpl_ProgramIndexTableRow *rowPtr = &programIndexTable.rows[idx];
		LINK *lp;

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "\t<programIndexTable.row[%d]>\t: transactionId=%d, ca_pmt_cmd_id=%s, ltsid=%d, programNum=%d, sourceId=0x%x, priority=%d, state=%s\n",
				 idx, rowPtr->transactionId,
				 caCommandString( rowPtr->ca_pmt_cmd_id ), rowPtr->ltsid,
				 rowPtr->programNum, rowPtr->sourceId, rowPtr->priority,
				 programIndexTableStateString( rowPtr->state ) );

		if ( rowPtr->authorizedPids == NULL )
			continue;

		lp = llist_first( rowPtr->authorizedPids );
		i = 0;
		while ( lp )
		{
			rmf_PodmgrStreamDecryptInfo *pidInfo =
				( rmf_PodmgrStreamDecryptInfo * ) llist_getdata( lp );
			if ( pidInfo )
			{
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						 "\t\t\t\t\t<authorized Pids>[%d] pid:0x%x status:%d\n",
						 i, pidInfo->pid, pidInfo->status );
				i++;
			}
			lp = llist_after( lp );
		}
	}
}

static char *programIndexTableStateString( uint8_t state )
{
	if ( state == RMF_PODMGR_DECRYPT_STATE_NOT_SELECTED )
		return "RMF_PODMGR_DECRYPT_STATE_NOT_SELECTED";
	else if ( state == RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY )
		return "RMF_PODMGR_DECRYPT_STATE_ISSUED_QUERY";
	else if ( state == RMF_PODMGR_DECRYPT_STATE_ISSUED_MMI )
		return "RMF_PODMGR_DECRYPT_STATE_ISSUED_MMI";
	else if ( state == RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING )
		return "RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING";
	else if ( state == RMF_PODMGR_DECRYPT_STATE_FAILED_DESCRAMBLING )
		return "RMF_PODMGR_DECRYPT_STATE_FAILED_DESCRAMBLING";
	else
		return "RMF_PODMGR_DECRYPT_STATE_NOT_SELECTED ";
}

static void logRequestTable( char *localStr )
{
	int idx = 0;
	podImpl_RequestTableRow *row = NULL;
	LINK *lp = NULL;

	if ( requestTable == NULL )
	{
		return;
	}
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s::logRequestTable %s\n",
			 PODMODULE, localStr );

	lp = llist_first( requestTable );
	while ( lp )
	{
		row = ( podImpl_RequestTableRow * ) llist_getdata( lp );
		if ( row != NULL )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "\t<requestTable[%d]>\t: programIndex=%d, tunerId=%d, ltsId=%d, priority=%d, sourceId=0x%x, state=%s mmiEnable=%d serviceHandle=0x%x\n",
					 idx, row->programIdx, row->tunerId, row->ltsId,
					 row->priority, row->sourceId,
					 requestTableStateString( row->state ), row->mmiEnable,
					 row->pServiceHandle );
			idx++;
		}
		lp = llist_after( lp );
	}
}

static char *requestTableStateString( uint8_t state )
{
	if ( state == RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE )
		return "RMF_PODMGR_DECRYPT_REQUEST_STATE_ACTIVE";
	else if ( state ==
			  RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES )
		return "RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES";
	else
		return "RMF_PODMGR_DECRYPT_REQUEST_STATE_UNKNOWN";
}

char *podImplCAEventString( rmf_PODSRVDecryptSessionEvent event )
{
	if ( event == RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT )
		return "RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT";
	else if ( event == RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES )
		return "RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES";
	if ( event == RMF_PODSRV_DECRYPT_EVENT_MMI_PURCHASE_DIALOG )
		return "RMF_PODSRV_DECRYPT_EVENT_MMI_PURCHASE_DIALOG";
	else if ( event == RMF_PODSRV_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG )
		return "RMF_PODSRV_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG";
	if ( event == RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED )
		return "RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED";
	else if ( event == RMF_PODSRV_DECRYPT_EVENT_SESSION_SHUTDOWN )
		return "RMF_PODSRV_DECRYPT_EVENT_SESSION_SHUTDOWN";
	else
		return "UNKNOWN ";
}

static char *caCommandString( uint8_t ca_pmt_cmd )
{
	if ( ca_pmt_cmd == RMF_PODMGR_CA_OK_DESCRAMBLE )
		return "RMF_PODMGR_CA_OK_DESCRAMBLE";
	else if ( ca_pmt_cmd == RMF_PODMGR_CA_OK_MMI )
		return "RMF_PODMGR_CA_OK_MMI";
	else if ( ca_pmt_cmd == RMF_PODMGR_CA_QUERY )
		return "RMF_PODMGR_CA_QUERY";
	else if ( ca_pmt_cmd == RMF_PODMGR_CA_NOT_SELECTED )
		return "RMF_PODMGR_CA_NOT_SELECTED";
	else
		return " ";
}

#define APDU_LEN_SIZE_INDICATOR_IDX APDU_TAG_SIZE		/* Beginning of length section, Zero based index */
#define CA_REPLY_OUTER_CA_ENABLE_DATA_OFFSET	 (7)	/* if data starts at 6, the outer ca_enable will be at 13 (6+7) */
#define CA_UPDATE_OUTER_CA_ENABLE_DATA_OFFSET	 (7)	/* if data starts at 6, the outer ca_enable will be at 13 (6+7) */

rmf_osal_Bool isCaInfoReplyAPDU( uint32_t sessionId, uint8_t * apdu )
{
	if ( sessionId == casSessionId )
	{
		uint32_t apduTag = getApduTag( apdu );

		if ( apduTag == RMF_PODMGR_CA_INFO_TAG )
			return TRUE;
	}
	return FALSE;
}

static rmf_osal_Bool isCaReplyAPDU( uint32_t sessionId, uint8_t * apdu )
{
	if ( sessionId == casSessionId )
	{
		uint32_t apduTag = getApduTag( apdu );

		if ( apduTag == RMF_PODMGR_CA_PMT_REPLY_TAG )
			return TRUE;
	}
	return FALSE;
}

static rmf_osal_Bool isCaUpdateAPDU( uint32_t sessionId, uint8_t * apdu )
{
	if ( sessionId == casSessionId )
	{
		uint32_t apduTag = getApduTag( apdu );

		if ( apduTag == RMF_PODMGR_CA_PMT_UPDATE_TAG )
			return TRUE;
	}
	return FALSE;
}

static uint32_t getApduTag( uint8_t * apdu )
{
	uint32_t tag = ( apdu[0] << 16 ) & 0x00FF0000;
	tag |= ( apdu[1] << 8 );
	tag |= apdu[2];

	return tag;
}

/* length does not include tag or length_field() itself.
 * maximum valid length returned is 0xFFFF
 */
static int32_t getApduDataLen( uint8_t * apdu )
{
	uint8_t size_byte = apdu[APDU_LEN_SIZE_INDICATOR_IDX];
	uint32_t length_field_size = size_byte & 0x7F;
	uint32_t length_value = 0;

	if ( size_byte & 0x80 )
	{
		int idx;

		if ( length_field_size > 2 )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: getApduLen, length_field_size > 2, size=%d\n",
					 PODMODULE, programIndexTable.numRows );

			return -1;
		}

		/* CCIF2.0:  "bytes shall be concatenated, first byte at the most significant end, to encode an integer value." */
		for ( idx = 1; idx <= 2; idx++ )
		{
			/* 8 bits bslbf */
			length_value |=
				apdu[APDU_LEN_SIZE_INDICATOR_IDX +
					 idx] << ( ( length_field_size - idx ) * 8 );
		}
	}
	else
	{
		length_value = length_field_size;
	}
	return length_value;
}

static uint16_t getApduDataOffset( uint8_t * apdu )
{
	uint16_t dataIdx = APDU_LEN_SIZE_INDICATOR_IDX + 1;

	int8_t size_field = apdu[APDU_LEN_SIZE_INDICATOR_IDX];
	if ( size_field & 0x80 )
	{
		dataIdx += ( size_field & 0x7F );
	}

	return dataIdx;
}

static uint8_t getProgramIdxFromCaReply( uint8_t * apdu )
{
	uint16_t program_idx_offset = getApduDataOffset( apdu );

	return apdu[program_idx_offset];
}

static uint8_t getTransactionIdFromCaReply( uint8_t * apdu )
{
	uint16_t program_idx_offset = getApduDataOffset( apdu );
	return apdu[program_idx_offset + 1];
}

uint16_t getCASystemIdFromCaReply( uint8_t * apdu )
{
	uint16_t system_id = 0;
	system_id = apdu[APDU_TAG_SIZE + 1] << 8;
	system_id |= apdu[APDU_TAG_SIZE + 2];

	return system_id;
}

static uint8_t getOuterCaEnableFromCaReply( uint8_t * apdu )
{
	uint16_t data_offset = getApduDataOffset( apdu );
	uint16_t outer_offset =
		data_offset + CA_REPLY_OUTER_CA_ENABLE_DATA_OFFSET;
	return apdu[outer_offset];
}

static void getInnerCaEnablesFromCaReply( uint8_t * apdu, uint16_t * numPids,
										  uint16_t elemStreamPidArray[],
										  uint8_t caEnableArray[] )
{
	uint16_t data_offset = getApduDataOffset( apdu );

	// outer data is not valid, but space is still used.
	uint16_t outer_offset =
		data_offset + CA_REPLY_OUTER_CA_ENABLE_DATA_OFFSET;

	// beginning of inner data loop
	uint16_t apdu_idx = outer_offset + 1;

	uint16_t array_idx = 0;
	uint16_t data_size = getApduDataLen( apdu );
	uint16_t apdu_size = data_offset + data_size;
	uint16_t es_pid = 0;
	uint8_t ca_enable_byte = 0;
	uint8_t ca_enable = 0;

	while ( apdu_idx < apdu_size )
	{
		es_pid = ( apdu[apdu_idx++] << 8 );		// & 0x1FFF;  // upper 3 bits are reserved, so set them to zero
		es_pid |= apdu[apdu_idx++];		// lower 8 bits
		ca_enable_byte = apdu[apdu_idx++];
		if ( ( ca_enable_byte & RMF_PODMGR_CA_ENABLE_SET ) > 0 )
		{
			ca_enable = ca_enable_byte & 0x7F;
			elemStreamPidArray[array_idx] = es_pid;
			caEnableArray[array_idx] = ca_enable;
			array_idx++;
		}
	}
	*numPids = array_idx;
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::getInnerCaEnablesFromCaReply numPids:%d.\n", PODMODULE,
			 *numPids );
}

static rmf_Error setPidArrayForProgramIndex( uint8_t programIdx,
											 uint16_t numPids,
											 uint16_t elemStreamPidArray[],
											 uint8_t caEnableArray[] )
{
	int i = 0;
	LINK *lp;
	uint8_t ca_enable = 0;
	podImpl_ProgramIndexTableRow *rowPtr =
		&programIndexTable.rows[programIdx];

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::setPidArrayForProgramIndex - programIdx=%d numPids=%d\n",
			 PODMODULE, programIdx, numPids );

	lp = llist_first( rowPtr->authorizedPids );
	i = 0;
	while ( lp )
	{
		rmf_PodmgrStreamDecryptInfo *pidInfo =
			( rmf_PodmgrStreamDecryptInfo * ) llist_getdata( lp );
		if ( pidInfo )
		{
			if ( elemStreamPidArray == NULL && numPids == 0 )	/* program level outer ca_enable byte */
			{
				ca_enable = caEnableArray[0] & 0x7F;
				pidInfo->status = ca_enable;
			}
			else if ( numPids > 0 && ( elemStreamPidArray != NULL )
					  && ( caEnableArray != NULL ) )
			{
				pidInfo->pid = elemStreamPidArray[i];
				pidInfo->status = caEnableArray[i];
				i++;

				if ( i == numPids )
					break;
			}

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "setPidArrayForProgramIndex - pid:%d ca_status:%d \n",
					 pidInfo->pid, pidInfo->status );
		}
		lp = llist_after( lp );
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "setPidArrayForProgramIndex - pid count:%lu \n",
			 llist_cnt( rowPtr->authorizedPids ) );
	return RMF_SUCCESS;
}

static rmf_PODSRVDecryptSessionEvent getLastEventForProgramIndex( uint8_t
																  programIndex )
{
	podImpl_ProgramIndexTableRow *rowPtr =
		&programIndexTable.rows[programIndex];
	return rowPtr->lastEvent;
}

static uint8_t getProgramIdxFromCaUpdate( uint8_t * apdu )
{
	uint16_t program_idx_offset = getApduDataOffset( apdu );
	return apdu[program_idx_offset];
}

static uint8_t getTransactionIdFromCaUpdate( uint8_t * apdu )
{
	uint16_t program_idx_offset = getApduDataOffset( apdu );
	return apdu[program_idx_offset + 1];
}

static uint8_t getOuterCaEnableFromCaUpdate( uint8_t * apdu )
{
	uint16_t data_offset = getApduDataOffset( apdu );
	uint16_t outer_offset =
		data_offset + CA_UPDATE_OUTER_CA_ENABLE_DATA_OFFSET;
	return apdu[outer_offset];
}

static void getInnerCaEnablesFromCaUpdate( uint8_t * apdu, uint16_t * numPids,
										   uint16_t elemStreamPidArray[],
										   uint8_t caEnableArray[] )
{
	uint16_t data_offset = getApduDataOffset( apdu );

	// outer data is not valid, but space is still used.
	uint16_t outer_offset =
		data_offset + CA_UPDATE_OUTER_CA_ENABLE_DATA_OFFSET;

	// beginning of inner data loop
	uint16_t apdu_idx = outer_offset + 1;

	uint16_t array_idx = 0;
	uint16_t data_size = getApduDataLen( apdu );
	uint16_t apdu_size = data_offset + data_size;
	uint16_t es_pid = 0;
	uint8_t ca_enable_byte = 0;
	uint8_t ca_enable = 0;

	while ( apdu_idx < apdu_size )
	{
		es_pid = ( apdu[apdu_idx++] << 8 );		// & 0x1FFF;  // upper 3 bits are reserved, so set them to zero
		es_pid |= apdu[apdu_idx++];		// lower 8 bits
		ca_enable_byte = apdu[apdu_idx++];
		if ( ( ca_enable_byte & RMF_PODMGR_CA_ENABLE_SET ) > 0 )
		{
			ca_enable = ca_enable_byte & 0x7F;
			elemStreamPidArray[array_idx] = es_pid;
			caEnableArray[array_idx] = ca_enable;
			array_idx++;
		}
	}
	*numPids = array_idx;
}

static rmf_Error handleCaEnable( uint8_t ca_enable, uint8_t programIdx )
{
	rmf_Error retCode;
	rmf_PODSRVDecryptSessionEvent event;
	uint8_t numHandles = 0;
	int i;
	rmf_osal_Bool resourcesAvailable = FALSE;
	// Random size array
	rmf_PodSrvDecryptSessionHandle handles[10];

	podImpl_ProgramIndexTableRow *rowPtr =
		&programIndexTable.rows[programIdx];

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s::handleCaEnable programIdx:%d ca_enable:0x%x.\n", PODMODULE,
			 programIdx, ca_enable );

	// There may be multiple requests per program index table row
	if ( ( retCode =
		   getDecryptSessionsForProgramIndex( programIdx, &numHandles,
											  handles ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s::handleCaEnable error retrieving decrypt sessions for program index:%d.\n",
				 PODMODULE, programIdx );
		return retCode;
	}

	switch ( ca_enable )
	{
	  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_NO_CONDITIONS:    /* De-scrambling */
		  {
			  uint8_t transId;
			  podImpl_RequestTableRow *request;


			  // Check the current state of program index table row
			  if ( rowPtr->state == RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING )
			  {
				  // Nothing to do, return...
				  return RMF_SUCCESS;
			  }

			  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					   "%s::handleCaEnable, ca_enable:RMF_PODMGR_CA_ENABLE_DESCRAMBLING_NO_CONDITIONS.\n",
					   PODMODULE );
			  event = RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED;
			  if ( rowPtr->state != RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING )
			  {
				  // If this is in response to a query or a ca_update() and
				  // we have not sent a ca_pmt() APDU with 'ok_descramble' before
				  // send one now and setup a CP session
				  // otherwise just signal an event
				  if ( rowPtr->ca_pmt_cmd_id != RMF_PODMGR_CA_OK_DESCRAMBLE )
				  {
					  transId =
						  getNextTransactionIdForProgramIndex( programIdx );

					  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							   "<%s::handleCaEnable> - Sending CA_PMT apdu (RMF_PODMGR_CA_OK_DESCRAMBLE)\n",
							   PODMODULE );
					  // Even if multiple requests (handles) are sharing a decrypt session,
					  // the service handle and tuner Id will be same
					  request = ( podImpl_RequestTableRow * ) handles[0];
					  if ( ( retCode =
							 createAndSendCaPMTApdu( request->pServiceHandle,
													 programIdx, transId,
													 rowPtr->ltsid,
													 RMF_PODMGR_CA_OK_DESCRAMBLE ) )
						   == RMF_SUCCESS )
					  {
						  // Start the CP session

						  // rowPtr->authorizedPids should never be null
						  if ( rowPtr->authorizedPids != NULL
							   && llist_cnt( rowPtr->authorizedPids ) != 0 )
						  {
							  int index = 0;
							  uint32_t numPids =
								  llist_cnt( rowPtr->authorizedPids );
							  uint32_t *pidArray = NULL;
							  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
									   "%s::handleCaEnable pidArray size:%d\n",
									   PODMODULE, numPids );
							  if ( rmf_osal_memAllocP
								   ( RMF_OSAL_MEM_POD,
									 ( sizeof( uint32_t ) * numPids ),
									 ( void ** ) &pidArray ) == RMF_SUCCESS )
							  {
								  LINK *lp =
									  llist_first( rowPtr->authorizedPids );
								  while ( lp )
								  {
									  rmf_PodmgrStreamDecryptInfo *pidInfo =
										  ( rmf_PodmgrStreamDecryptInfo * )
										  llist_getdata( lp );
									  if ( pidInfo )
									  {
										  pidArray[index++] = pidInfo->pid;
										  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
												   "%s::handleCaEnable pidArray[%d]:0x%x\n",
												   PODMODULE, index - 1,
												   pidArray[index - 1] );
									  }
									  lp = llist_after( lp );
								  }
								  // Setup CP session here
								  retCode =
									  podStartCPSession( request->tunerId,
														 rowPtr->programNum,
														 rowPtr->ltsid,
														 rowPtr->ecmPid,
														 programIdx, numPids,
														 pidArray,
														 &( rowPtr->
															cpSession ) );
								  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
										   "%s::handleCaEnable created CP session:0x%x\n",
										   PODMODULE, rowPtr->cpSession );
								  if ( retCode != RMF_SUCCESS )
								  {
									  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
											   "%s::handleCaEnable error setting up CP session...%d\n",
											   PODMODULE, retCode );
									  // What to do here?
								  }
							  }
							  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pidArray );
						  }
						  rowPtr->ca_pmt_cmd_id = RMF_PODMGR_CA_OK_DESCRAMBLE;
						  rowPtr->transactionId = transId;
						  request->ca_pmt_cmd_id =
							  RMF_PODMGR_CA_OK_DESCRAMBLE;
					  }
					  else
					  {
						  // What to do here?
						  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
								   "<%s::handleCaEnable> - Sending CA_PMT apdu failed..in state: %s\n",
								   PODMODULE,
								   programIndexTableStateString( rowPtr->
																 state ) );
					  }
				  }
				  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						   "%s::handleCaEnable, programIndexTableRow current state: %s, new state: RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING\n",
						   PODMODULE,
						   programIndexTableStateString( rowPtr->state ) );
				  rowPtr->state = RMF_PODMGR_DECRYPT_STATE_DESCRAMBLING;		/* Set the program index table entry state */
				  rowPtr->lastEvent = event;
				  sendEventImpl( event, programIdx );
			  }
		  }
		  break;				// END case:RMF_PODMGR_CA_ENABLE_DESCRAMBLING_NO_CONDITIONS

	  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_PAYMENT:	   /* De-scrambling possible with purchase */
	  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_TECH:		   /* De-scrambling possible with technical dialog */
		  {
			  uint8_t transId;
			  podImpl_RequestTableRow *request;

			  // Signal events such that JMF can correctly enter into
			  // alternate content presentation state

			  if ( ca_enable ==
				   RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_PAYMENT )
			  {
				  event = RMF_PODSRV_DECRYPT_EVENT_MMI_PURCHASE_DIALOG;
				  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						   "%s::handleCaEnable, ca_enable:RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_PAYMENT (0x2).\n",
						   PODMODULE );
			  }
			  else
			  {
				  event = RMF_PODSRV_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG;
				  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						   "%s::handleCaEnable, ca_enable:RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_TECH (0x3).\n",
						   PODMODULE );
			  }

			  // Issue a ca_pmt() APDU with cmd_id 'ok_descramble'
			  transId = getNextTransactionIdForProgramIndex( programIdx );

			  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					   "<%s::handleCaEnable> - Sending CA_PMT apdu (RMF_PODMGR_CA_OK_DESCRAMBLE)\n",
					   PODMODULE );
			  // Even if multiple requests (handles) are sharing a decrypt session,
			  // the service handle and tuner Id will be same
			  request = ( podImpl_RequestTableRow * ) handles[0];
			  if ( ( retCode =
					 createAndSendCaPMTApdu( request->pServiceHandle,
											 programIdx, transId,
											 rowPtr->ltsid,
											 RMF_PODMGR_CA_OK_DESCRAMBLE ) )
				   == RMF_SUCCESS )
			  {
				  // Start the CP session

				  // rowPtr->authorizedPids should never be null
				  if ( rowPtr->authorizedPids != NULL
					   && llist_cnt( rowPtr->authorizedPids ) != 0 )
				  {
					  int index = 0;
					  uint32_t numPids = llist_cnt( rowPtr->authorizedPids );
					  uint32_t *pidArray = NULL;
					  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
							   "%s::handleCaEnable pidArray size:%d\n",
							   PODMODULE, numPids );
					  if ( rmf_osal_memAllocP
						   ( RMF_OSAL_MEM_POD,
							 ( sizeof( uint32_t ) * numPids ),
							 ( void ** ) &pidArray ) == RMF_SUCCESS )
					  {
						  LINK *lp = llist_first( rowPtr->authorizedPids );
						  while ( lp )
						  {
							  rmf_PodmgrStreamDecryptInfo *pidInfo =
								  ( rmf_PodmgrStreamDecryptInfo * )
								  llist_getdata( lp );
							  if ( pidInfo )
							  {
								  pidArray[index++] = pidInfo->pid;
								  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
										   "%s::handleCaEnable pidArray[%d]:0x%x\n",
										   PODMODULE, index - 1,
										   pidArray[index - 1] );
							  }
							  lp = llist_after( lp );
						  }

						  // Setup CP session here
						  retCode =
							  podStartCPSession( request->tunerId,
												 rowPtr->programNum,
												 rowPtr->ltsid,
												 rowPtr->ecmPid, programIdx,
												 numPids, pidArray,
												 &( rowPtr->cpSession ) );
						  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
								   "%s::handleCaEnable created CP session:0x%x\n",
								   PODMODULE, rowPtr->cpSession );
						  if ( retCode != RMF_SUCCESS )
						  {
							  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
									   "%s::handleCaEnable error setting up CP session...%d\n",
									   PODMODULE, retCode );
							  // What to do here?
						  }
					  }
					  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pidArray );
				  }
				  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						   "%s::handleCaEnable, programIndexTableRow current state: %s, new state: RMF_PODMGR_DECRYPT_STATE_ISSUED_MMI\n",
						   PODMODULE,
						   programIndexTableStateString( rowPtr->state ) );

				  rowPtr->state = RMF_PODMGR_DECRYPT_STATE_ISSUED_MMI;	/* Set the program index table entry state */
				  rowPtr->lastEvent = event;
				  rowPtr->ca_pmt_cmd_id = RMF_PODMGR_CA_OK_DESCRAMBLE;
				  rowPtr->transactionId = transId;
				  request->ca_pmt_cmd_id = RMF_PODMGR_CA_OK_DESCRAMBLE;

				  sendEventImpl( event, programIdx );
			  }
			  else
			  {
				  // What to do here?
				  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						   "<%s::handleCaEnable> - Sending CA_PMT apdu failed..in state: %s\n",
						   PODMODULE,
						   programIndexTableStateString( rowPtr->state ) );
			  }
		  }





		  break;				// END case:RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_PAYMENT
		  // END case:RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_TECH
		  /*
		   * De-scrambling not possible since there is no entitlement
		   */
	  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_PAYMENT_FAIL:
		  {
			  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					   "%s::handleCaEnable, ca_enable:RMF_PODMGR_CA_ENABLE_DESCRAMBLING_PAYMENT_FAIL (0x71).\n",
					   PODMODULE );

			  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					   "%s::handleCaEnable, programIndexTableRow current state: %s, new state: RMF_PODMGR_DECRYPT_STATE_FAILED_DESCRAMBLING\n",
					   PODMODULE,
					   programIndexTableStateString( rowPtr->state ) );

			  rowPtr->state = RMF_PODMGR_DECRYPT_STATE_FAILED_DESCRAMBLING;		// De-scrambling failed
			  event = RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT;
			  rowPtr->lastEvent = event;
			  rowPtr->ca_pmt_cmd_id = 0;

			  sendEventImpl( event, programIdx );
			  break;
		  }
		  /* de-scrambling not possible for technical reasons.	For example, all elementary stream available are being used */
	  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_TECH_FAIL:
		  {
			  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					   "%s::handleCaEnable, ca_enable:RMF_PODMGR_CA_ENABLE_DESCRAMBLING_TECH_FAIL (0x73).\n",
					   PODMODULE );

			  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					   "%s::handleCaEnable, programIndexTableRow current state: %s, new state: RMF_PODMGR_DECRYPT_STATE_FAILED_DESCRAMBLING\n",
					   PODMODULE,
					   programIndexTableStateString( rowPtr->state ) );

			  rowPtr->state = RMF_PODMGR_DECRYPT_STATE_FAILED_DESCRAMBLING;		// De-scrambling failed
			  event = RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES;
			  rowPtr->lastEvent = event;
			  rowPtr->ca_pmt_cmd_id = 0;

			  sendEventImpl( event, programIdx );
			  break;
		  }
	  default:
		  {
			  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					   "%s::handleCaEnable unexpected error...\n",
					   PODMODULE );
			  break;
		  }
	}							// End switch (ca_enable)

    // If there are requests that are waiting for resources service those: Commenting dead code
#if 0
    if(resourcesAvailable  && (getSuspendedRequests() > 0))
    {
        podImpl_RequestTableRow *request = NULL;

        for(i=0; i<numHandles; i++)
        {
            request = (podImpl_RequestTableRow*)handles[i];
            // Reset the requests state (Is this the right state here?)
            request->state = RMF_PODMGR_DECRYPT_REQUEST_STATE_UNKNOWN;
            // Reset the program index??
            request->programIdx = -1;
            // Save the command id
            request->ca_pmt_cmd_id = rowPtr->ca_pmt_cmd_id;
        }

        // Now remove the corresponding entry from the program index table if no other
        // uses remain (based on the program index)
        // check if this programIndex is being used by any other decrypt request
        if(!isProgramIndexInUse(programIdx) && request)
        {
            uint8_t transId = getNextTransactionIdForProgramIndex(programIdx);
            // Send a ca_pmt apdu with 'not_selected' (0x04) when decrypt session
            // is no longer in use
            if((retCode = createAndSendCaPMTApdu(request->pServiceHandle, programIdx,
                    transId, request->ltsId,
                    RMF_PODMGR_CA_NOT_SELECTED)!= RMF_SUCCESS))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                        "<%s::handleCaEnable> - Error sending CA_PMT apdu (MPE_MPOD_CA_NOT_SELECTED) (error %d)\n",
                        PODMODULE, retCode);
            }

            // Release the CP session
            if(rowPtr->cpSession != NULL)
            {
                if(RMF_SUCCESS != podStopCPSession(rowPtr->cpSession))
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                            "%s::handleCaEnable Error stopping CP session...\n",
                            PODMODULE);
                }
            }

            rowPtr->transactionId = transId;
            if ((retCode = releaseProgramIndexRow(programIdx))
                    != RMF_SUCCESS)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                        "<%s::handleCaEnable> - Error releasing program index row (error %d)\n",
                        PODMODULE, retCode);
            }

            // Re-activate a suspended request. There may be multiple requests that can share this
            // program index table row
            if ((retCode = activateSuspendedRequests(programIdx)) != RMF_SUCCESS)
            {
                return retCode;
            }
        }
    }
#endif
	return retCode;
}

static void sendEventImpl( rmf_PODSRVDecryptSessionEvent event,
						   uint8_t programIdx )
{
	rmf_PodSrvDecryptSessionHandle sessionHandle;
	uint8_t numHandles = 0;
	int i;
	// Random size array
	rmf_PodSrvDecryptSessionHandle handles[10];

    if(NULL == programIndexTable.rows)
    {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
			"<%s::sendEvent> Program Index Table array is NULL. Possibly due to CARD REMOVED event \n", PODMODULE);
		return;
    }
	podImpl_ProgramIndexTableRow *rowPtr =
		&programIndexTable.rows[programIdx];

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s::sendEventImpl programIdx:%d\n",
			 PODMODULE, programIdx );

	// There may be multiple requests per program index table row
	if ( getDecryptSessionsForProgramIndex( programIdx, &numHandles, handles )
		 != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "%s::sendEventImpl error retrieving decrypt sessions for program index:%d.\n",
				 PODMODULE, programIdx );
		return;
	}

	// Send events to all requests corresponding to the program index array
	for ( i = 0; i < numHandles; i++ )
	{
		sessionHandle = handles[i];

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s::sendEventImpl event %s for sessionHandle:0x%p.\n",
				 PODMODULE, podImplCAEventString( event ), sessionHandle );


		sendDecryptSessionEvent( event, sessionHandle, rowPtr->ltsid );

	}
}

static rmf_Error createAndSendCaPMTApdu( parsedDescriptors *pPodSIDescriptors,
										 uint8_t programIdx,
										 uint8_t transactionId, uint8_t ltsid,
										 uint8_t ca_cmd_id )
{
	uint8_t *apdu_buf = NULL;
	uint8_t *apdu_buf_data = NULL;
	uint32_t apdu_length = 0;
	uint8_t apdu_length_field_length;
	rmf_osal_Bool done = false;
	uint8_t retry_count = g_podImpl_ca_retry_count;
	rmf_Error retCode = RMF_SUCCESS;


	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "<%s::createAndSendCaPMTApdu> - Enter.. \n", PODMODULE );

	//retCode = allocateDescInfo( pPodSIDescriptors, ca_cmd_id );
	if( 0 != pPodSIDescriptors->CAPMTCmdIdIndex )
	{
		pPodSIDescriptors->pStreamInfo[pPodSIDescriptors->CAPMTCmdIdIndex] = ca_cmd_id;
	}	
	
	//if ( retCode != RMF_SUCCESS )
		//return retCode;

	if ( ( retCode =
		   podmgrCreateCAPMT_APDU( pPodSIDescriptors, programIdx,
								   transactionId, ltsid, ca_system_id,
								   ca_cmd_id, &apdu_buf,
								   &apdu_length ) ) != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "<%s::createAndSendCaPMTApdu> - Error creating CA_PMT (error 0x%x)\n",
				 PODMODULE, retCode );

		return retCode;
	}

	// Length is either 1 byte or n bytes, depending on high order bit of length_field
    /*apdu_length_field_length = (apdu_buf[3] & 0x80) ? (apdu_buf[3] & 0x7F)
                                                    : 1;*/
	// The length of the APDU header length needs to take into account the tag field (3 bytes).
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1 
    // Length is either 1 byte or n bytes, depending on high order bit of length_field
    //apdu_length_field_length = (apdu_buf[3] & 0x80) ? (apdu_buf[3] & 0x7F) : 1;
    apdu_length_field_length = 0;
	// updated	POD implementation 
	// apdu_length_field_length += 3;
#else    
    // Length is either 1 byte or n bytes, depending on high order bit of length_field
    apdu_length_field_length = (apdu_buf[3] & 0x80) ? (apdu_buf[3] & 0x7F) : 1;
    apdu_length_field_length += 3;    
#endif    
/* Standard code deviation: ext: X1 end */
	apdu_buf_data = &( apdu_buf[apdu_length_field_length] );

	if ( retCode == RMF_SUCCESS )
	{
		// Dump the contents of the CA PMT APDU
		hexDump( ( uint8_t * ) ( PODMODULE " DUMP_CA_PMT" ), apdu_buf,
				 apdu_length );
	}

	while ( !done )
	{
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
		retCode =
			vlMpeosPodSendCaPmtAPDU( casSessionId, RMF_PODMGR_CA_PMT_TAG,
									 ( apdu_length -
									   apdu_length_field_length ),
									 apdu_buf_data );
#else
		retCode =
			rmf_podmgrSendAPDU( casSessionId, RMF_PODMGR_CA_PMT_TAG,
								( apdu_length - apdu_length_field_length ),
								apdu_buf_data );
#endif
		if ( retCode == RMF_SUCCESS )
		{
			done = true;
		}
		else
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "<%s::createAndSendCaPMTApdu> - Error sending CA_PMT, retCode: %d ..\n",
					 PODMODULE, retCode );

			// Re-attempt sending the apdu if retry count is set
			if ( retry_count )
			{
				retry_count--;
			}

			if ( retry_count == 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s::createAndSendCaPMTApdu> - Error sending CA_PMT apdu after %d retries..\n",
						 PODMODULE, g_podImpl_ca_retry_count );
				done = true;
			}
			else
			{
				// Wait here before attempting a re-send
				// Configurable via ini variable
				rmf_osal_threadSleep( g_podImpl_ca_retry_timeout, 0 );
			}
		}
	}							// End while(!done)

	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, apdu_buf );
	apdu_buf = NULL;


	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "<%s::createAndSendCaPMTApdu> - done.. \n", PODMODULE );

	if ( retCode == RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "ISSUED CA PMT command = %s, WAITING FOR REPLY/UPDATE for transaction Id: %d\n",
				 caCommandString( ca_cmd_id ), transactionId );
	}
/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<%s::createAndSendCaPMTApdu> - done.. \n", PODMODULE );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "ISSUED CA PMT command = %s, WAITING FOR REPLY/UPDATE for transaction Id: %d\n",
			 caCommandString( ca_cmd_id ), transactionId );
#endif
/* Standard code deviation: ext: X1 end */
#ifdef PODIMPL_FRONTPANEL_DEBUG
	{
		snprintf( fp_debug_lines[0], 30, "%d-%d", programIdx, transactionId );
	}
#endif

	return retCode;
}

static rmf_Error updatePODDecryptRequestAndResource( uint8_t programIdx,
													 uint8_t transactionId,
													 uint16_t numPids,
													 uint16_t
													 elemStreamPidArray[],
													 uint8_t caEnableArray[] )
{
	rmf_Error retCode = RMF_SUCCESS;
	uint8_t ca_enable = 0;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s::updatePODDecryptRequestAndResource - programIdx=%d\n",
			 PODMODULE, programIdx );
        
    if(NULL == programIndexTable.rows)
    {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
			"<%s::updatePODDecryptRequestAndResource> Program Index Table array is NULL. Possibly due to CARD REMOVED event \n", PODMODULE);
		return RMF_OSAL_EINVAL;
    }

	if ( ( retCode =
		   setPidArrayForProgramIndex( programIdx, numPids,
									   elemStreamPidArray,
									   caEnableArray ) ) != RMF_SUCCESS )
	{
		return retCode;
	}

	if ( elemStreamPidArray == NULL && numPids == 0 )	/* deal with outer ca_enable byte */
	{
		ca_enable = caEnableArray[0] & 0x7F;
		retCode = handleCaEnable( ca_enable, programIdx );
	}
	else						/* else, inner ca_enable bytes */
	{
		uint8_t successValue = RMF_PODMGR_CA_ENABLE_NO_VALUE;
		uint8_t mmiValue = RMF_PODMGR_CA_ENABLE_NO_VALUE;
		uint8_t failValue = RMF_PODMGR_CA_ENABLE_NO_VALUE;
		int i;
		/*
		 * report any failure first, but if failure is not found report any MMI activity.  If everything is okay, report
		 * decrypt started
		 */
		for ( i = 0;
			  i < numPids && failValue == RMF_PODMGR_CA_ENABLE_NO_VALUE; i++ )
		{
			ca_enable = caEnableArray[i] & 0x7F;

			switch ( ca_enable )
			{
			  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_NO_CONDITIONS:    /* descrambling is occurring */
				  successValue = ca_enable;
				  break;
			  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_PAYMENT:	   /* descrambling possible with purchase */
			  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_WITH_TECH:		   /* descrambling possible with technical intervention. */
				  mmiValue = ca_enable;
				  break;
			  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_PAYMENT_FAIL:	   /* descrambling not possible since there is no entitlement */
			  case RMF_PODMGR_CA_ENABLE_DESCRAMBLING_TECH_FAIL:		   /* descrambling not possible for technical reasons.	For example, all elementary stream available are being used */
				  failValue = ca_enable;
				  break;
			}
		}

		if ( failValue > RMF_PODMGR_CA_ENABLE_NO_VALUE )
		{
			retCode = handleCaEnable( failValue, programIdx );
		}
		else if ( mmiValue > RMF_PODMGR_CA_ENABLE_NO_VALUE )
		{
			retCode = handleCaEnable( mmiValue, programIdx );
		}
		else if ( successValue > RMF_PODMGR_CA_ENABLE_NO_VALUE )
		{
			retCode = handleCaEnable( successValue, programIdx );
		}
	}

	return retCode;
}

static void FreeEventDataFn(void *pData)
{
    if(pData)
    {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
             "sendDecryptSessionEvent - freeing event Data, qamhndl = %d\n",
             ((qamEventData_t *)pData)->qamContext );
        free(pData);
        pData = NULL;
    }
}

/*
 * This methods dispatches a decrypt event with the associated session handle
 * to callers
 */
rmf_Error sendDecryptSessionEvent( rmf_PODSRVDecryptSessionEvent event,
								   rmf_PodSrvDecryptSessionHandle handle,
								   uint32_t edEventData )
{
	podImpl_RequestTableRow *row = ( podImpl_RequestTableRow * ) handle;
	rmf_osal_event_handle_t event_handle;
	rmf_Error err;

	qamEventData_t *eventData = NULL;
	eventData = (qamEventData_t *)malloc(sizeof(qamEventData_t));
	if(eventData == NULL)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s failed to allocate memory for eventData.\n", __FUNCTION__);
		return RMF_OSAL_ENODATA;
	}
	eventData->qamContext = row->qamContext;
	eventData->handle = handle;
	eventData->cci = edEventData;

	rmf_osal_event_params_t event_params = { 0,
		( void * ) eventData,
		( uint32_t ) handle,
		FreeEventDataFn
	};

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s: sendDecryptSessionEvent() Event=0x%x handle=0x%p\n",
			 PODMODULE, event, handle );
	err =
		rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD, event,
							   &event_params, &event_handle );

	if ( RMF_SUCCESS == err )
	{
		err = rmf_osal_eventqueue_add( row->eventQ, event_handle );

		if ( RMF_SUCCESS != err )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s failed %d", __FUNCTION__,
					 err );
		}
	}
	else
	{
		// Report any error to the log.
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s failed \n", __FUNCTION__,
				 err );
	}

	return err;
}


static void shutdownQAMSrcLevelRegisteredPodEvents(  )
{
	QueueEntry *p;
	QueueEntry *save;
	rmf_Error err;
	rmf_osal_event_handle_t event_handle;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<QAM_LEVEL_POD_QUEUES> shutdownRegisteredPodEvents \n" );

	if ( podImplEventQueueListMutex == NULL )
		return;

	rmf_osal_mutexAcquire( podImplEventQueueListMutex );

	// Shutdown the queues and delete the entries
	p = podImplEventQueueList;

	while ( NULL != p )
	{


		// Send the event.
		err =
			rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD,
								   RMF_OSAL_ETHREADDEATH, NULL,
								   &event_handle );

		if ( RMF_SUCCESS == err )
		{
			err = rmf_osal_eventqueue_add( p->m_queue, event_handle );

			if ( RMF_SUCCESS != err )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "%s Shutdown event send failed %d", __FUNCTION__,
						 err );
			}
		}
		else
		{
			// Report any error to the log.
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s Shutdown event creation failed\n", __FUNCTION__,
					 err );
		}

		save = p;
		p = p->next;
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, save );
	}

	// Release and kill the mutex
	rmf_osal_mutexRelease( podImplEventQueueListMutex );
	rmf_osal_mutexDelete( podImplEventQueueListMutex );
}

static void notifyQAMSrcLevelRegisteredPodEventQueues( uint32_t event,
													   rmf_osal_event_params_t
													   eventData )
{
	QueueEntry *p;
	rmf_Error err;
	rmf_osal_event_handle_t event_handle;
	uint8_t *ip_addr = ( uint8_t * ) eventData.data;

	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
			 "<QAM_LEVEL_POD_QUEUES> notifyQAMSrcLevelRegisteredPodEventQueues - eventId = %d, podImplEventQueueList=0x%p\n",
			 event, podImplEventQueueList );

	rmf_osal_mutexAcquire( podImplEventQueueListMutex );

	// Just run thru the list and send events.
	for ( p = podImplEventQueueList; NULL != p; p = p->next )
	{

		if ( RMF_PODSRV_EVENT_DSGIP_ACQUIRED == event )
		{
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, RMF_PODMGR_IP_ADDR_LEN, &( eventData.data ) );
			memcpy( eventData.data, ip_addr, RMF_PODMGR_IP_ADDR_LEN );
			eventData.free_payload_fn = podmgrImpl_freefn;
		}
		// Send the event.
		err =
			rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD, event,
								   &eventData, &event_handle );

		if ( RMF_SUCCESS == err )
		{
			if ( rmf_osal_eventqueue_add( p->m_queue, event_handle ) !=
				 RMF_SUCCESS )
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						 "<QAM_LEVEL_POD_QUEUES> notifyQAMSrcLevelRegisteredPodEventQueues() - error sending POD event %d to queue 0x%x: %d\n",
						 event, p->m_queue, err );
			}
		}
		else
		{
			// Report any error to the log.
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<QAM_LEVEL_POD_QUEUES> notifyQAMSrcLevelRegisteredPodEventQueues() - error creating POD event %d to queue 0x%x: %d\n",
					 event, p->m_queue, err );
		}
	}

	rmf_osal_mutexRelease( podImplEventQueueListMutex );
}

/**
 * Designed to be called even before this manager is fully initialized
 */
rmf_Error podmgrRegisterEvents( rmf_osal_eventqueue_handle_t queueId )
{
	QueueEntry *newEntry;
	rmf_Error err;

	if( podImplEventQueueListInitialized != 1)
	{

		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			 "<QAM_LEVEL_POD_QUEUES> rmf_podmgrRegisterEvents - not initialized \n");
		return RMF_OSAL_ENODATA;
	}

	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
			 "<QAM_LEVEL_POD_QUEUES> rmf_podmgrRegisterEvents - queue 0x%x\n",
			 queueId );

	// Allocate a new entry
	err =
		rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( QueueEntry ),
							( void ** ) &newEntry );

	if ( RMF_SUCCESS != err )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "<QAM_LEVEL_POD_QUEUES> rmf_podmgrRegisterEvents - failure, queue 0x%x, %d\n",
				 queueId, err );
		return err;
	}

	// Init it.
	newEntry->m_queue = queueId;

	// Add it to the list.	In front is OK.
	rmf_osal_mutexAcquire( podImplEventQueueListMutex );
	newEntry->next = podImplEventQueueList;
	podImplEventQueueList = newEntry;
	rmf_osal_mutexRelease( podImplEventQueueListMutex );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<QAM_LEVEL_POD_QUEUES> rmf_podmgrRegisterEvents - podImplEventQueueList=0x%p\n",
			 podImplEventQueueList );

	return RMF_SUCCESS;
}

rmf_Error podmgrUnRegisterEvents( rmf_osal_eventqueue_handle_t queueId )
{
	QueueEntry **p;
	QueueEntry *save;
	rmf_Error err = RMF_OSAL_EINVAL;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<QAM_LEVEL_POD_QUEUES> rmf_podmgrUnRegisterEvents - queue 0x%x\n",
			 queueId );

	// Lock the list for modification
	rmf_osal_mutexAcquire( podImplEventQueueListMutex );

	// Find the entry with this ID
	for ( p = &podImplEventQueueList; NULL != *p; p = &( ( *p )->next ) )
	{
		// Find it?
		if ( ( *p )->m_queue == queueId )
		{
			// Remove it and exit
			// Save it
			save = *p;
			// Reset the current pointer to skip it.
			( *p ) = ( *p )->next;

			// Free the node
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, save );

			// Set up the successful return
			err = RMF_SUCCESS;

			// Drop out of the loop
			break;
		}
	}

	// Release the structure
	rmf_osal_mutexRelease( podImplEventQueueListMutex );

	return err;
}

rmf_osal_Bool podmgrSASResetStat( rmf_osal_Bool Set, rmf_osal_Bool *pStatus )
{
	static rmf_osal_Bool SASResetState = TRUE;
	if( Set == TRUE )
	{
		SASResetState = *pStatus;
	}
	
	return SASResetState;
}

static rmf_Error suspendActiveCASessions( void )
{
	int programIndex = 0;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "<%s::suspendActiveCASessions> ...\n",
			 PODMODULE );

	for ( programIndex = 0; programIndex < programIndexTable.numRows;
		  programIndex++ )
	{
		rmf_PodSrvDecryptSessionHandle handles[10];
		uint8_t numHandles = 0;
		int i;
		rmf_PodSrvDecryptSessionHandle rHandle;
		uint8_t ltsid = 0;
		uint8_t transId;
		rmf_Error retCode = RMF_SUCCESS;
		parsedDescriptors *pServiceHandle = NULL;
		podImpl_ProgramIndexTableRow *rowPtr =
			&programIndexTable.rows[programIndex];
		// Find active session
		if ( ACTIVE_DECRYPT_STATE( rowPtr->state ) )
		{
			// There may be multiple requests per program index table row
			if ( ( retCode =
				   getDecryptSessionsForProgramIndex( programIndex,
													  &numHandles,
													  handles ) ) !=
				 RMF_SUCCESS )
			{
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						 "<%s::suspendActiveCASessions> error retrieving decrypt sessions for program index:%d.\n",
						 PODMODULE, programIndex );
				//continue;
			}

			for ( i = 0; i < numHandles; i++ )
			{
				podImpl_RequestTableRow *request =
					( podImpl_RequestTableRow * ) handles[i];

				pServiceHandle = request->pServiceHandle;

				rHandle = handles[i];

				// Reset the requests state and program index
				request->state =
					RMF_PODMGR_DECRYPT_REQUEST_STATE_WAITING_FOR_RESOURCES;
				request->programIdx = -1;
				// Save the command id so that it can be re-activated when
				// resources are available
				//request->ca_pmt_cmd_id = rowPtr->ca_pmt_cmd_id;
				request->ca_pmt_cmd_id = RMF_PODMGR_CA_QUERY;


				// Send an event to notify of resource loss
				sendDecryptSessionEvent
					( RMF_PODSRV_DECRYPT_EVENT_RESOURCE_LOST, rHandle,
					  ltsid );

			}

			ltsid = rowPtr->ltsid;
			transId = getNextTransactionIdForProgramIndex( programIndex );

			// Send a ca_pmt apdu with 'not_selected' (0x04) when decrypt session
			// is no longer in use
			if ( pServiceHandle )
			{
				if ( ( retCode =
					   createAndSendCaPMTApdu( pServiceHandle, programIndex,
											   transId, ltsid,
											   RMF_PODMGR_CA_NOT_SELECTED ) !=
					   RMF_SUCCESS ) )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							 "<%s::suspendActiveCASessions> - Error sending CA_PMT apdu (RMF_PODMGR_CA_NOT_SELECTED) (error %d)\n",
							 PODMODULE, retCode );
				}
			}
			// Release the CP session
			if ( rowPtr->cpSession != NULL )
			{
				if ( RMF_SUCCESS != podStopCPSession( rowPtr->cpSession ) )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
							 "%s::suspendActiveCASessions Error stopping CP session...\n",
							 PODMODULE );
				}
			}

			if ( ( retCode =
				   releaseProgramIndexRow( programIndex ) ) != RMF_SUCCESS )
			{
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						 "<%s::suspendActiveCASessions> error releasing program index table row:%d.\n",
						 PODMODULE, programIndex );
			}

			// Restore the transaction id
			//rowPtr->transactionId = transId;
			rowPtr->transactionId = 0;
		}						// End if(ACTIVE_DECRYPT_STATE(rowPtr->state))
	}							// End for (programIndex = 0; programIndex < programIndexTable.numRows; programIndex++)

	logProgramIndexTable( "suspendActiveCASessions" );
	logRequestTable( "suspendActiveCASessions" );
	return RMF_SUCCESS;
}

static rmf_Error activateSuspendedCASessions( void )
{
	int programIndex = 0;
	rmf_Error retCode = RMF_SUCCESS;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "<%s::activateSuspendedCASessions> ...\n", PODMODULE );
	if ( requestTable == NULL )
	{
		// There are no suspended requests
		return RMF_SUCCESS;
	}

	for ( programIndex = 0; programIndex < programIndexTable.numRows;
		  programIndex++ )
	{
		if ( ( retCode =
			   activateSuspendedRequests( programIndex ) ) != RMF_SUCCESS )
		{
			return retCode;
		}
	}
	logProgramIndexTable( "activateSuspendedCASessions" );
	logRequestTable( "activateSuspendedCASessions" );
	return RMF_SUCCESS;
}

/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
// VL updated POD implementation
rmf_Error podImplGetTransportStreamId( unsigned long *pnTsId )
{
	int programIndex = 0;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "<%s::suspendActiveCASessions> ...\n",
			 PODMODULE );

	return RMF_SUCCESS;
}
#endif
/* Standard code deviation: ext: X1 end */
uint8_t getLtsid( void )
{
	do
	{
		// 'g_ltsid' starts as a random number
		g_ltsid = ( g_ltsid % 255 ) + 1;		// Range: 1..255
	}
	while ( isLtsidInUse( g_ltsid ) );	// If ltsid is in use get the next ltsid
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "<%s::getLtsid> ...ltsid:%d\n",
			 PODMODULE, g_ltsid );
	return g_ltsid;
}

static rmf_osal_Bool isLtsidInUse( uint8_t ltsid )
{
	int programIndex = 0;
	for ( programIndex = 0; programIndex < programIndexTable.numRows;
		  programIndex++ )
	{
		podImpl_ProgramIndexTableRow *rowPtr =
			&programIndexTable.rows[programIndex];
		//Check if ltsid is already in use
		if ( rowPtr->ltsid == ltsid )
		{
			return TRUE;
		}
	}
	return FALSE;
}

static uint8_t getLtsidForTunerId( uint8_t tuner )
{
	int programIndex = 0;
	for ( programIndex = 0; programIndex < programIndexTable.numRows;
		  programIndex++ )
	{
		podImpl_ProgramIndexTableRow *rowPtr =
			&programIndexTable.rows[programIndex];
		if ( rowPtr->tunerId == tuner )
		{
			//return rowPtr->ltsid;
		}
	}

#ifdef CMCST_EXT_X1
	{
		unsigned char ltsId = 0;
		rmf_Error retCode1;

		if ( ( retCode1 =
			   rmf_qamsrc_getLtsid( ( int32_t ) tuner,
									&ltsId ) ) != RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "Failed to get a ltsid for tuner %d - status 0x%x\n",
					 tuner, retCode1 );
			ltsId = tuner;		// return tunerId as best effort
		}
		return ltsId;
	}
#endif

#ifndef CMCST_EXT_X1
	return LTSID_UNDEFINED;		// invalid ltsid
#endif
}


#define POD_RET_ON_ERR(err) if( RMF_SUCCESS != err) return NULL;
parsedDescriptors *dupDescriptor(parsedDescriptors *src)
{
	rmf_Error ret;
	parsedDescriptors *dest = NULL;
	ret = rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof(parsedDescriptors),
								 ( void ** ) &dest );
	POD_RET_ON_ERR( ret );

	if(NULL == dest)
	{
		return NULL;
	}
	else
	{
		memset(dest, 0, sizeof(parsedDescriptors)); /*Clear the memory for first use*/
		memcpy(dest, src, sizeof(parsedDescriptors));
	}
    
	if( src->noES )
	{
		ret = rmf_osal_memAllocP( RMF_OSAL_MEM_POD,  (src->noES * (sizeof( rmf_PodmgrStreamDecryptInfo ))) ,	
								( void ** ) &dest->pESInfo );
		//POD_RET_ON_ERR( ret );
		if(RMF_SUCCESS != ret)
		{
			if(dest)
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_POD,( void * ) dest);
				dest = NULL;
			}
			return NULL;
		}
		memcpy( dest->pESInfo, src->pESInfo, src->noES * (sizeof( rmf_PodmgrStreamDecryptInfo )) );
	}
	
	if( src->streamInfoLen )
	{
		ret = rmf_osal_memAllocP( RMF_OSAL_MEM_POD,  src->streamInfoLen,
									 ( void ** ) &dest->pStreamInfo );	
		//POD_RET_ON_ERR( ret );
		if(RMF_SUCCESS != ret)
		{
			if(dest->pESInfo)
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_POD,( void * ) dest->pESInfo);
				dest->pESInfo = NULL;
			}
			if(dest)
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_POD,( void * ) dest);
				dest = NULL;
			}
			return NULL;
		}
		memcpy( dest->pStreamInfo, src->pStreamInfo,  src->streamInfoLen );
	}
	
	if( src->ProgramInfoLen  )
	{
		ret = rmf_osal_memAllocP( RMF_OSAL_MEM_POD,  src->ProgramInfoLen,
									 ( void ** ) &dest->pProgramInfo);	
		//POD_RET_ON_ERR( ret );
		if(RMF_SUCCESS != ret)
		{
			if(dest->pESInfo)
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_POD,( void * ) dest->pESInfo);
				dest->pESInfo = NULL;
			}
			if(dest->pStreamInfo)
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_POD,( void * ) dest->pStreamInfo);
				dest->pStreamInfo = NULL;
			}			
			if(dest)
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_POD,( void * ) dest);
				dest = NULL;
			}
			return NULL;
		}
		memcpy( dest->pProgramInfo, src->pProgramInfo,  src->ProgramInfoLen);
	}
	
	return dest;
}

void freeDescriptor( parsedDescriptors *pParseOutput )
{
	if ( pParseOutput )
	{
		if ( pParseOutput->pESInfo )
		{
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
							   pParseOutput->pESInfo );
			pParseOutput->pESInfo = NULL;
		}

		if ( pParseOutput->pStreamInfo )
		{
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pParseOutput->pStreamInfo );
			pParseOutput->pStreamInfo = NULL;
		}

		if ( pParseOutput->pProgramInfo )
		{
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pParseOutput->pProgramInfo );
			pParseOutput->pProgramInfo = NULL;
		}
		
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pParseOutput );
	}
}



