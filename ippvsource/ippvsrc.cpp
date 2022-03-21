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


/**
 * @file ippvsrc.cpp
 * @brief Interactive/Impulsive Pay Per View source file.
 */
#include "ippvsrc.h"
#ifdef USE_SDVSRC
#include "rmfsdvsrcpriv.h"
#else
#include "rmfqamsrcpriv.h"
#endif
#include "ippvclient.h"
#include "canhClient.h"
#include <unistd.h>

#ifdef USE_SDVSRC
class RMFiPPVSrcImpl:public RMFSDVSrcImpl
#else
class RMFiPPVSrcImpl:public RMFQAMSrcImpl
#endif
{
  public:
	GstElement * src_element;
	void *getLowLevelElement(  );
	RMFiPPVSrcImpl( RMFMediaSourceBase * parent );
	~RMFiPPVSrcImpl();
	RMFResult open( const char *uri, char *mimetype );
	RMFResult close();
	RMFResult purchasePPVEvent( uint32_t eId );

  private:
	RMFResult setEvents( IRMFMediaEvents * events );
	RMFResult close_on_exit();	 
	rmf_osal_ThreadId monitorThreadId;
	rmf_osal_eventqueue_handle_t queueId;
	uint32_t m_eId;
	uint16_t srcId;
	bool m_purchaseInitiated;
	bool m_purchaseSuccess;
	IRMFMediaEvents *m_events;
	sem_t *canhDoneSem;
  protected:
	inline virtual rmf_osal_Bool stopCAWhenPaused(void) const {return FALSE;}
};

#ifdef USE_SDVSRC
RMFiPPVSrcImpl::RMFiPPVSrcImpl( RMFMediaSourceBase * parent ): RMFSDVSrcImpl( parent )
#else
RMFiPPVSrcImpl::RMFiPPVSrcImpl( RMFMediaSourceBase * parent ): RMFQAMSrcImpl( parent )
#endif
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "[%s:%d]\n", __FUNCTION__, __LINE__ );
	m_eId = 0;
	m_purchaseInitiated = false;
	m_purchaseSuccess = false;
	rmf_osal_eventqueue_create( (const uint8_t*)"iPPVMonitor", &queueId );
}

RMFiPPVSrcImpl::~RMFiPPVSrcImpl()
{
	RMFQAMSrc::freeLowLevelElement( ( void * ) src_element );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "[%s:%d]\n", __FUNCTION__, __LINE__ );

	rmf_osal_eventqueue_delete( queueId );
}

void *RMFiPPVSrcImpl::getLowLevelElement()
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "[%s:%d]\n", __FUNCTION__, __LINE__ );
	src_element = (GstElement *)RMFQAMSrc::getLowLevelElement();
	return src_element;
}


/**
 * @fn RMFiPPVSrc::RMFiPPVSrc()
 * @brief Default constructor which logs a message into logfile as "Logfile LOG.RDK.IPPV" for IPPVSource.
 *
 * @param None.
 * @return None.
 * @ingroup ippvsrcapi
 */
RMFiPPVSrc::RMFiPPVSrc()
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "[%s:%d]\n", __FUNCTION__, __LINE__ );
}


/**
 * @fn RMFiPPVSrc::~RMFiPPVSrc()
 * @brief Default destructor which logs a message into logfile as "Logfile LOG.RDK.IPPV" for IPPVSource.
 *
 * @param None.
 * @return None.
 * @ingroup ippvsrcapi
 */
RMFiPPVSrc::~RMFiPPVSrc()
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "[%s:%d]\n", __FUNCTION__, __LINE__ );
}


/**
 * @fn RMFResult RMFiPPVSrc::init_platform()
 * @brief This function is not implemented.
 *
 * @param None.
 * @return RMFResult
 * @retval RMF_SUCCESS If successfully initialized the RMFIPPV source platform functionality.
 * @ingroup ippvsrcapi
 */
RMFResult RMFiPPVSrc::init_platform()
{
	return RMF_SUCCESS;
}


/**
 * @fn RMFResult RMFiPPVSrc::uninit_platform()
 * @brief This function is not implemented.
 *
 * @param None.
 * @return RMFResult
 * @retval RMF_SUCCESS If successfully uninitialized the RMFIPPV source platform functionality.
 * @ingroup ippvsrcapi
 */
RMFResult RMFiPPVSrc::uninit_platform()
{
	return RMF_SUCCESS;
}


/**
 * @fn RMFResult RMFiPPVSrc::purchasePPVEvent( unsigned int &eId )
 * @brief This function is used to purchase PPV event by using entitlement id.
 * A monitor EID command is sent from IPPVSrc to CANH IPC server when a tune request is received for an entitlement id.
 * When client request for an ippv event, mediastreamer makes a purchase call to ippvsrc which will send the status back to Media streamer.
 *
 * @param[in] eID Entitlement id to check the status of event.
 *
 * @return RMFResult
 * @retval RMF_SUCCESS. Else, refer ippv macros for other return values.
 * @ref ippvmacros
 * @ingroup ippvsrcapi
 */
RMFResult RMFiPPVSrc::purchasePPVEvent( unsigned int &eId )
{
	RMFResult ret = RMF_SUCCESS;
	RMFMediaSourcePrivate *ppvImpl = getPrivateSourceImpl();

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: Eid: 0x%x\n", __FUNCTION__, eId );

	ret = ((RMFiPPVSrcImpl*)ppvImpl)->purchasePPVEvent( eId );
	if ( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV","[%s:%d] PPV event purchase failed\n",
			__FUNCTION__, __LINE__ );
	}

	return ret;
}

RMFResult RMFiPPVSrcImpl::open( const char *uri, char *mimetype )
{
	RMFResult ret = RMF_SUCCESS;
	char qamUri[32] = "ocap://";
	IppvClient ppv;

	sscanf( uri, "ippv://%[^#]#%x", &qamUri[7], &m_eId );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: Source id: %s\n",
		__FUNCTION__, qamUri );

#ifdef USE_SDVSRC
	if (RMFSDVSrcImpl::isSdvDiscoveryUri(uri)) {
	    ret = RMFSDVSrcImpl::open( uri, mimetype );
	}
	else {
	ret = RMFSDVSrcImpl::open( qamUri, mimetype );
	}

#else
	ret = RMFQAMSrcImpl::open( qamUri, mimetype );
#endif
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: QAMSrc open: %d\n",	__FUNCTION__, ret );
	if( RMF_RESULT_SUCCESS != ret )
	{
		return ret;
	}

	return RMF_SUCCESS;
}

RMFResult RMFiPPVSrcImpl::close_on_exit()
{
	int32_t ret = 0;

	if ( true == m_purchaseInitiated )
	{
		ret = canhStopEIDMonitoring( m_eId, CANH_TARGET_IPPV );
	}

	return RMF_RESULT_SUCCESS;
}

RMFResult RMFiPPVSrcImpl::close()
{
	RMFResult ret = RMF_RESULT_SUCCESS;

	ret = close_on_exit();
	if( RMF_RESULT_SUCCESS != ret )
	{
		return ret;
	}
	m_purchaseInitiated = false;
	m_purchaseSuccess = false;

#ifdef USE_SDVSRC
	ret = RMFSDVSrcImpl::close();
#else
	ret = RMFQAMSrcImpl::close();
#endif
	return ret;
}

RMFResult RMFiPPVSrcImpl::purchasePPVEvent( uint32_t eId )
{
	RMFResult ret = RMF_RESULT_SUCCESS;
	int canhRetry = 0;
	IppvClient ppv;
	m_eId = eId;

	while ( false == isCANHReady() )
	{
		if ( canhRetry++ > 5 )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s CANH is not yet ready\n",
				 __FUNCTION__ );
			notifyError( RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT, NULL );
			return RMF_RESULT_NOT_INITIALIZED;
		}
		sleep( 2 );
	}

	ret = canhStartEIDMonitoring( m_eId, CANH_TARGET_IPPV );
	ret = ppv.purchaseIppvEvent( m_eId );
	m_purchaseInitiated = true;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: purchaseIppvEvent status: %s\n",
		__FUNCTION__, ppv.getStatusByString(ret) );

	if ( (RMF_IPPVSRC_PURCHASE_PENDING != ret)
		&& (RMF_IPPVSRC_ALREADY_AUTHORIZED != ret)
		&& (RMF_IPPVSRC_SUCCESS != ret) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: Exit with Error: %d\n",
				 __FUNCTION__, ret );
		notifyError( RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT, NULL );
		return ret;
	}
	m_purchaseSuccess = true;

	return RMF_SUCCESS;
}

RMFMediaSourcePrivate *RMFiPPVSrc::createPrivateSourceImpl()
{
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.IPPV", "[%s:%d]\n", __FUNCTION__, __LINE__ );
	return new RMFiPPVSrcImpl( this );
}

RMFResult RMFiPPVSrcImpl::setEvents( IRMFMediaEvents * events )
{
	m_events = events;
	return RMFQAMSrcImpl::setEvents( events );
}

