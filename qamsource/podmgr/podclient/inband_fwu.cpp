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
#include <rmf_error.h> 
#include <rmf_osal_event.h> 
#include <rdk_debug.h> 
#include <rmf_pod.h> 
#include <rmfqamsrc.h> 
#include <TunerReservationHelper.h>
#include <rmfcore.h> 
#include <rmf_osal_ipc.h> 
#include <podServer.h> 
#include <string.h>
#include <stdlib.h>
#include <rmf_osal_util.h>
#ifdef USE_IPDIRECT_UNICAST
#include <rmf_pod.h>
#endif

#define CARD_FWU_DOWNLOAD_REQUEST_RECEIVED 0
#define CARD_FWU_DOWNLOAD_COMPLETED 1
#define CARD_FWU_DOWNLOAD_TUNER_LOCKED 2
#define CARD_FWU_DOWNLOAD_TUNE_FAILED 3
#define CARD_FWU_TUNER_RESERVATION_TOKEN "CableCardFWU"
#define CARD_INB_TUNE_REQUEST_RECEIVED 4
#define CARD_INB_TUNE_COMPLETED 5
#define CARD_INB_TUNE_TUNER_LOCKED 6
static volatile bool gCFWInProgress = false;

class QAMSrcEvent : public IRMFMediaEvents
{
public:

	QAMSrcEvent( rmf_osal_eventqueue_handle_t eventq )
	{
		this->eventq = eventq;
	}
	QAMSrcEvent( )
	{
		eventq = 0;
	}
	~QAMSrcEvent(){ }	
	void status(const RMFStreamingStatus& status)
	{	
		uint32_t QAMStatus = status.m_status;
		switch ( QAMStatus )
		{
			case RMF_QAMSRC_EVENT_TUNE_SYNC:
			{
				sendQAMEvent( QAMStatus );
			}
			break;
			default:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s REceiving other events from QAMSrc for inband cable card firmware upgrade %d\n", 
					__FUNCTION__, QAMStatus );
			break;
		}
	}
	void error(RMFResult err, const char* msg)
	{
		(void) msg;	
		switch ( err )
		{
			case RMF_QAMSRC_ERROR_TUNER_NOT_LOCKED:
			case RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR:
			case RMF_QAMSRC_ERROR_TUNE_FAILED:
			{
				sendQAMEvent(err);
			}
			break;
			default:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s REceiving other events from QAMSrc for inband cable card firmware upgrade %d\n", 
					__FUNCTION__, err );
			break;
		}		
	}	
private:
	rmf_osal_eventqueue_handle_t eventq;	
	void sendQAMEvent( uint32_t event )
	{
		rmf_Error err = RMF_OSAL_EINVAL;
		rmf_osal_event_handle_t event_handle;

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s Tuner locked for inband cable card firmware upgrade %d\n", 
			__FUNCTION__, err );

		rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD, 
								event,
								NULL, &event_handle );	
		err = rmf_osal_eventqueue_add( eventq, event_handle );
		if ( RMF_SUCCESS != err )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s rmf_osal_eventqueue_add failed %d\n", __FUNCTION__, err );
		}		
	}
};

void  IPC_HAL_SYS_Reboot(const char * pszRequestor, const char * pszReason)
{	
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;
	int32_t lenOfRequestor = 1; // '\0' char
	int32_t lenOfReason = 1; // '\0' char
	uint32_t pod_server_cmd_port_no = 0;	
	const char *server_port;
	
	server_port = rmf_osal_envGet( "POD_SERVER_CMD_PORT_NO" );
	if( NULL == server_port )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		pod_server_cmd_port_no = POD_SERVER_CMD_DFLT_PORT_NO;
	}
	else
	{
		pod_server_cmd_port_no = atoi( server_port );
	}
	lenOfRequestor = strlen(pszRequestor);
	lenOfReason = strlen(pszReason);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s:Received Reboot Request pszRequestor='%s', pszReason='%s' \n", __FUNCTION__, pszRequestor, pszReason);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s:length pszRequestor=%d, length pszReason=%d \n", __FUNCTION__, lenOfRequestor, lenOfReason);
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_STB_REBOOT);

	result_recv |= ipcBuf.addData( ( const void * ) &lenOfRequestor, sizeof(int32_t));
	result_recv |= ipcBuf.addData( ( const void * ) pszRequestor, lenOfRequestor);
	result_recv |= ipcBuf.addData( ( const void * ) &lenOfReason, sizeof(int32_t));
	result_recv |= ipcBuf.addData( ( const void * ) pszReason, lenOfReason);
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"\nPOD Comm add data failed in %s:%d: %d\n",
				  __FUNCTION__, __LINE__ , result_recv);
		return ;
	}
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return ;
	}

	result_recv = ipcBuf.getResponse( &ret );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return ;
	}
	return;
}

void InbandCableCardFwuMonitor( void *param )
{
	rmf_Error rmf_error = RMF_SUCCESS;
	rmf_osal_eventqueue_handle_t eventq = 0;

	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_params_t event_params = {0};
	rmf_osal_event_category_t event_category;
	uint32_t event_type;
	RMFQAMSrc *pQAMInstance = NULL;
	uint8_t status = CARD_FWU_DOWNLOAD_COMPLETED;
#ifdef USE_IPDIRECT_UNICAST
	uint8_t InbandTuneStatus = CARD_INB_TUNE_COMPLETED;
#endif
	char uri [1024]; 
	IRMFMediaEvents* pEvents = NULL;
	TunerReservationHelper *trh = NULL;
#ifdef USE_IPDIRECT_UNICAST
        bool bIPdirectMode = false;
#endif

	rmf_error = rmf_osal_eventqueue_create((const uint8_t*) "InbandCableCardFwuMonitorQ", &eventq );
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s - rmf_osal_eventqueue_create failed.\n", __FUNCTION__);
		return;
	}
		
	rmf_error = rmf_podmgrRegisterEvents( eventq );
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s - rmf_podmgrRegisterEvents failed.\n", __FUNCTION__);
		return;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s - Started \n", __FUNCTION__);

	pEvents = new QAMSrcEvent( eventq );
	
#ifdef USE_TRM_OBSOLETE
	trh = new TunerReservationHelper(NULL);
#endif

	while( 1 )
	{
		rmf_error = rmf_osal_eventqueue_get_next_event( 
			eventq, &event_handle, &event_category, &event_type, &event_params );
		if (RMF_SUCCESS != rmf_error )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s - rmf_osal_eventqueue_get_next_event failed.\n", __FUNCTION__);
			continue;
		}

		switch( event_type )
		{
#ifdef USE_IPDIRECT_UNICAST
                  case RMF_PODMGR_EVENT_INB_TUNE_RELEASE:
                  {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
                            "%s - RMF_PODMGR_EVENT_INB_TUNE_RELEASE\n",
                            __func__);

                    if (pQAMInstance != NULL)
                    {
                      pQAMInstance->removeEventHandler(pEvents);
                      RMFQAMSrc::freeQAMSourceInstance(pQAMInstance);
                    }
                    pQAMInstance = NULL;
#ifdef USE_TRM_OBSOLETE
                    trh->releaseTunerReservation();
#endif
                    // send release status to host ctl
                    rmf_podmgrStartHomingDownload(0, RMF_PODMGR_INB_FWU_TUNER_BUSY);
                    bIPdirectMode = false;
                    break;
                  }

                  case RMF_PODMGR_EVENT_INB_TUNE_START:
                  {
                    uint32_t LtsId;
                    bool bTrm = false;
                    gCFWInProgress = true;

                    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                      "%s - RMF_PODMGR_EVENT_INB_TUNE_START received, InbandTuneStatus is %d \n",
                      __FUNCTION__, InbandTuneStatus);

                    if( CARD_INB_TUNE_COMPLETED != InbandTuneStatus )
                    {
                      RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "%s - RMF_PODMGR_EVENT_INB_TUNE_START received at invalid state \n",
                        __FUNCTION__);
                      if( ( CARD_INB_TUNE_REQUEST_RECEIVED == InbandTuneStatus ) ||
                        ( CARD_INB_TUNE_TUNER_LOCKED == InbandTuneStatus) )
                      {
                        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                          "%s - Removing EventHandler, Freeing QAMSource Instance and releasing TunerReservation\n",
                          __FUNCTION__);
                      if (pQAMInstance != NULL)
                      {
                        pQAMInstance->removeEventHandler( pEvents );
                        RMFQAMSrc::freeQAMSourceInstance( pQAMInstance, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
                      }
                        pQAMInstance = NULL;
#ifdef USE_TRM_OBSOLETE
                        trh->releaseTunerReservation();
#endif
                        InbandTuneStatus = CARD_INB_TUNE_COMPLETED;
                        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                          "%s - after cleanup new InbandTuneStatus is %d \n",
                          __FUNCTION__, InbandTuneStatus);
                      }
                    }
#ifdef USE_TRM_OBSOLETE
                    bTrm = trh->reserveTunerForLive( CARD_FWU_TUNER_RESERVATION_TOKEN, 0, 0, true);
                    if( !bTrm)
                    {
                      rmf_podmgrStartInbandTune( 0, RMF_PODMGR_INB_FWU_TUNER_BUSY);
                      InbandTuneStatus = CARD_INB_TUNE_COMPLETED;
                      gCFWInProgress = false;
                      break;
                    }
#endif
                    uri[ 0 ] = 0;
                    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                      "%s - RMF_PODMGR_EVENT_INB_TUNE_START: freq=%d, modulation=%d\n",
                      __FUNCTION__, event_params.data, event_params.data_extension );
                    snprintf( uri, sizeof(uri), "tune://frequency=%d&modulation=%d",
                      event_params.data, event_params.data_extension );
                    pQAMInstance = RMFQAMSrc::getQAMSourceInstance( uri, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
                    if( NULL == pQAMInstance )
                    {
                      RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "%s - NULL == pQAMInstance, tuner may be busy \n",
                        __FUNCTION__);
                      /* Tuner should be pre-empted in this case */
                      rmf_podmgrStartInbandTune( 0, RMF_PODMGR_INB_FWU_TUNER_BUSY );
                      InbandTuneStatus = CARD_INB_TUNE_COMPLETED;
                      gCFWInProgress = false;
                      break;
                    }
                    pQAMInstance->addEventHandler( pEvents );
                    InbandTuneStatus = CARD_INB_TUNE_REQUEST_RECEIVED;
                    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                      "%s - RMF_PODMGR_EVENT_INB_TUNE_START processed \n",
                      __FUNCTION__);
                  }
                  break;
                  case RMF_PODMGR_EVENT_INB_TUNE_COMPLETE:
                  {
                    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                      "%s - RMF_PODMGR_EVENT_INB_TUNE_COMPLETE received, status is %d \n",
                      __FUNCTION__, status);

                    if( CARD_INB_TUNE_COMPLETED == status )
                    {
                      RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "%s - RMF_PODMGR_EVENT_INB_TUNE_COMPLETE received \n",
                        __FUNCTION__);
                      break;
                    }
                    if (pQAMInstance != NULL)
                    {
                      pQAMInstance->removeEventHandler( pEvents );
                      RMFQAMSrc::freeQAMSourceInstance( pQAMInstance, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
                    }
                    pQAMInstance = NULL;
#ifdef USE_TRM_OBSOLETE
                    trh->releaseTunerReservation();
#endif
                    status = CARD_INB_TUNE_COMPLETED;
                    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                      "%s - RMF_PODMGR_EVENT_INB_TUNE_COMPLETE processed \n",
                      __FUNCTION__);
                    //IPC_HAL_SYS_Reboot( "InbandFirmwareUpgrade", "FirmwareUpgradeCompleted" );
                    gCFWInProgress = false;
                  }
                  break;
#endif

			case RMF_PODMGR_EVENT_INB_FWU_START:
			{
				
				uint32_t LtsId;
				bool bTrm = false;
				gCFWInProgress = true;
				
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - RMF_PODMGR_EVENT_INB_FWU_START received, status is %d \n",
					__FUNCTION__, status);	
				
				if( CARD_FWU_DOWNLOAD_COMPLETED != status ) 
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
						"%s - RMF_PODMGR_EVENT_INB_FWU_START received at invalid state \n", 
						__FUNCTION__);
					/* 
					 * Card Issued new event for Firmware Upgrade previous still not completed.
					 * Current status is not in completed, hence remove/clean previous session
					 * set status to completed and proceed with reservingTuner
					 */
					//break;
					if( ( CARD_FWU_DOWNLOAD_REQUEST_RECEIVED == status ) ||
					( CARD_FWU_DOWNLOAD_TUNER_LOCKED == status) )
					{
					    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
						    "%s - Removing EventHandler, Freeing QAMSource Instance and releasing TunerReservation\n", 
						    __FUNCTION__);
                                            if (pQAMInstance != NULL)
                                            {
					      pQAMInstance->removeEventHandler( pEvents );
					      RMFQAMSrc::freeQAMSourceInstance( pQAMInstance, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
                                            }
					    pQAMInstance = NULL;					
#ifdef USE_TRM_OBSOLETE
					    trh->releaseTunerReservation();
#endif
					    status = CARD_FWU_DOWNLOAD_COMPLETED;
					    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
						    "%s - after cleanup new status is %d \n", 
						    __FUNCTION__, status);
					}
				}
#ifdef USE_TRM_OBSOLETE
				bTrm = trh->reserveTunerForLive( CARD_FWU_TUNER_RESERVATION_TOKEN, 0, 0, true);
				if( !bTrm)
				{
					rmf_podmgrStartHomingDownload( 0, RMF_PODMGR_INB_FWU_TUNER_BUSY);
					status = CARD_FWU_DOWNLOAD_COMPLETED;
					gCFWInProgress = false;
					break;
				}
#endif				
				uri[ 0 ] = 0;
                                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                                        "%s - RMF_PODMGR_EVENT_INB_FWU_START: freq=%d, modulation=%d\n",
                                        __FUNCTION__, event_params.data, event_params.data_extension );
				snprintf( uri, sizeof(uri), "tune://frequency=%d&modulation=%d", 
					event_params.data, event_params.data_extension );
				pQAMInstance = RMFQAMSrc::getQAMSourceInstance( uri, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
				if( NULL == pQAMInstance )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
						"%s - NULL == pQAMInstance, tuner may be busy \n", 
						__FUNCTION__);				
					/* Tuner should be pre-empted in this case */
					rmf_podmgrStartHomingDownload( 0, RMF_PODMGR_INB_FWU_TUNER_BUSY );
					status = CARD_FWU_DOWNLOAD_COMPLETED;				
					gCFWInProgress = false;
					break;
				}
				pQAMInstance->addEventHandler( pEvents );
				status = CARD_FWU_DOWNLOAD_REQUEST_RECEIVED;

				qamsrc_status_t query_status;
				RMFResult retVal = 0;
				if(RMF_SUCCESS == pQAMInstance->getStatus(query_status))
				{
					if(query_status.is_locked)
					{
						RMFStreamingStatus qam_status;
						qam_status.m_status= RMF_QAMSRC_EVENT_TUNE_SYNC;
						pEvents->status(qam_status);
					}
				}

				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - RMF_PODMGR_EVENT_INB_FWU_START processed \n", 
					__FUNCTION__);				
			}
			break;
			case RMF_PODMGR_EVENT_INB_FWU_COMPLETE:
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - RMF_PODMGR_EVENT_INB_FWU_COMPLETE received, status is %d \n", 
					__FUNCTION__, status);	
				
				if(( CARD_FWU_DOWNLOAD_COMPLETED == status )
#ifdef USE_IPDIRECT_UNICAST
					&& (CARD_INB_TUNE_COMPLETED == InbandTuneStatus)
#endif
				)					
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
						"%s - RMF_PODMGR_EVENT_INB_FWU_COMPLETE received \n", 
						__FUNCTION__);
					break;
				}
                                if (pQAMInstance != NULL)
                                {
				  pQAMInstance->removeEventHandler( pEvents );
				  RMFQAMSrc::freeQAMSourceInstance( pQAMInstance, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
                                }
				pQAMInstance = NULL;
#ifdef USE_TRM_OBSOLETE
				trh->releaseTunerReservation();
#endif 
				status = CARD_FWU_DOWNLOAD_COMPLETED;
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - RMF_PODMGR_EVENT_INB_FWU_COMPLETE processed \n", 
					__FUNCTION__);
				IPC_HAL_SYS_Reboot( "InbandFirmwareUpgrade", "FirmwareUpgradeCompleted" );
				gCFWInProgress = false;
			}
			break;
			case RMF_QAMSRC_ERROR_TUNER_NOT_LOCKED:
			case RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR:
			case RMF_QAMSRC_ERROR_TUNE_FAILED:
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - RMF_QAMSRC_ERROR_XXX received, status is %d \n", 
					__FUNCTION__, status );
				rmf_podmgrStartHomingDownload( 0, RMF_PODMGR_INB_FWU_TUNE_FAILURE );

				if( ( CARD_FWU_DOWNLOAD_REQUEST_RECEIVED == status ) ||
					( CARD_FWU_DOWNLOAD_TUNER_LOCKED == status) )
				{
					status = CARD_FWU_DOWNLOAD_COMPLETED;
					
                                        if (pQAMInstance != NULL)
                                        {
					  pQAMInstance->removeEventHandler( pEvents );
					  RMFQAMSrc::freeQAMSourceInstance( pQAMInstance, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
                                        }
					pQAMInstance = NULL;					
#ifdef USE_TRM_OBSOLETE
					trh->releaseTunerReservation();
#endif 	
				}
			
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - RMF_QAMSRC_ERROR_XXX processed\n", 
					__FUNCTION__ );
				gCFWInProgress = false;				
			}
			break;
			case RMF_QAMSRC_EVENT_TUNE_SYNC:
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - RMF_QAMSRC_EVENT_TUNE_SYNC received, status is %d \n", 
					__FUNCTION__, status );
				
				if( CARD_FWU_DOWNLOAD_REQUEST_RECEIVED == status )
				{
					uint8_t LtsId;

                                        if (pQAMInstance != NULL)
                                        {
					  rmf_error = pQAMInstance->getLTSID( LtsId );
                                        }
					if( RMF_SUCCESS != rmf_error )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"%s - getLTSID failed \n", 
							__FUNCTION__);
						rmf_podmgrStartHomingDownload( 0, 
							RMF_PODMGR_INB_FWU_TUNE_FAILURE );
						status = CARD_FWU_DOWNLOAD_COMPLETED;	
                                                if (pQAMInstance != NULL)
                                                {
						  pQAMInstance->removeEventHandler( pEvents );
						  RMFQAMSrc::freeQAMSourceInstance( pQAMInstance, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
                                                }
						pQAMInstance = NULL;
#ifdef USE_TRM_OBSOLETE
						trh->releaseTunerReservation();
#endif
						gCFWInProgress = false;
					}
					else
					{
						rmf_podmgrStartHomingDownload( LtsId, 
							RMF_PODMGR_INB_FWU_TUNING_ACCEPTED );
						status = CARD_FWU_DOWNLOAD_TUNER_LOCKED;
					}
				}
				
#ifdef USE_IPDIRECT_UNICAST
                                else if( CARD_INB_TUNE_REQUEST_RECEIVED == InbandTuneStatus )
                                {
                                  uint8_t LtsId;

                                  if (pQAMInstance != NULL)
                                  {
                                    rmf_error = pQAMInstance->getLTSID( LtsId );
                                  }
                                  if( RMF_SUCCESS != rmf_error )
                                  {
                                    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                                      "%s - getLTSID failed \n",
                                    __FUNCTION__);
                                    rmf_podmgrStartInbandTune( 0, RMF_PODMGR_INB_FWU_TUNE_FAILURE );
                                    InbandTuneStatus = CARD_INB_TUNE_COMPLETED;
                                    if (pQAMInstance != NULL)
                                    {
                                      pQAMInstance->removeEventHandler( pEvents );
                                      RMFQAMSrc::freeQAMSourceInstance( pQAMInstance, RMFQAMSrc::USAGE_PODMGR, CARD_FWU_TUNER_RESERVATION_TOKEN );
                                    }
                                    pQAMInstance = NULL;
#ifdef USE_TRM_OBSOLETE
                                    trh->releaseTunerReservation();
#endif
                                    gCFWInProgress = false;
                                  }
                                  else
                                  {
                                    rmf_podmgrStartInbandTune( LtsId, RMF_PODMGR_INB_FWU_TUNING_ACCEPTED );
                                    InbandTuneStatus = CARD_INB_TUNE_TUNER_LOCKED;
                                  }
                                }
#endif

				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - RMF_QAMSRC_EVENT_TUNE_SYNC processed \n", 
					__FUNCTION__ );				
			}
			break;
			default:
			break;
		}//end of switch
		rmf_osal_event_delete( event_handle );
	}//End of while
}

bool IsCFWUInProgress(  )
{
	return gCFWInProgress;
}

