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

#ifndef RMFVODSOURCE_H
#define RMFVODSOURCE_H

/**
 * @file rmfvodsource.h
 * @brief VOD Source header file
 */

/**
 * @defgroup vodsource VOD Source
 * RMF VOD Source is a source element used in VOD playback. MediaStreamer receives the VOD requests
 * from clients. VOD source element sends session start request to JAVA VOD client. Java VOD client
 * sends a tune request(Freq, Modultion and Program Number) to VOD source which tunes and starts the playback.
 * @ingroup RMF
 *
 * @defgroup vodsourceapi VOD Source API list
 * @ingroup vodsource
 * @defgroup vodsourceadapi VOD Source API list (Auto Discovery)
 * @ingroup vodsource
 * @defgroup vodsourcet2pcapi VOD Source API list (T2P Client)
 * @ingroup vodsource
 * @defgroup vodsourcet2pserverapi VOD Source API list (T2P Server)
 * @ingroup vodsource
 */

#include <rmfqamsrc.h>

#define VODSRC_ERROR_SRM_20 (-20)


#define RMF_VODSRC_ERROR_BASE 				(0x500)
#define RMF_VODSRC_ERROR_CL_NO_SESSION			(RMF_VODSRC_ERROR_BASE + 0x0001)
#define RMF_VODSRC_ERROR_NE_NO_CALLS			(RMF_VODSRC_ERROR_BASE + 0x0002)
#define RMF_VODSRC_ERROR_NE_INVALID_CLIENT		(RMF_VODSRC_ERROR_BASE + 0x0003)
#define RMF_VODSRC_ERROR_NE_INVALID_SERVER 		(RMF_VODSRC_ERROR_BASE + 0x0004)
#define RMF_VODSRC_ERROR_NE_NO_SESSION			(RMF_VODSRC_ERROR_BASE + 0x0005)
#define RMF_VODSRC_ERROR_SE_NO_CALLS			(RMF_VODSRC_ERROR_BASE + 0x0006)
#define RMF_VODSRC_ERROR_SE_INVALID_CLIENT		(RMF_VODSRC_ERROR_BASE + 0x0007)
#define RMF_VODSRC_ERROR_SE_NO_SERVICE			(RMF_VODSRC_ERROR_BASE + 0x0008)

/* Used to indicate no server (SRM and pump) response */
#define RMF_VODSRC_ERROR_CL_NO_RESPONSE 		(RMF_VODSRC_ERROR_BASE + 0x000a)
#define RMF_VODSRC_ERROR_SE_NO_SESSION			(RMF_VODSRC_ERROR_BASE + 0x0010)
#define RMF_VODSRC_ERROR_SE_NO_PERMISSION       (RMF_VODSRC_ERROR_BASE  + 0x0014)
#define RMF_VODSRC_ERROR_SE_SESSION_RELEASE     (RMF_VODSRC_ERROR_BASE  + 0x001A)
#define RMF_VODSRC_ERROR_SE_NO_RESOURCE 		(RMF_VODSRC_ERROR_BASE + 0x0020)
#define RMF_VODSRC_ERROR_SE_NO_PROC_ERROR		(RMF_VODSRC_ERROR_BASE + 0x0024)

/* Message from SRM or PUMP did not parse properly or was inconsistent */
#define RMF_VODSRC_ERROR_NE_NOFORMAT_ERROR		(RMF_VODSRC_ERROR_BASE + 0x0026)
#define RMF_VODSRC_ERROR_SE_NOFORMAT_ERROR		(RMF_VODSRC_ERROR_BASE + 0x0027)
#define RMF_VODSRC_ERROR_NE_NOT_OWNER			(RMF_VODSRC_ERROR_BASE + 0x0042)

#define RMF_VODSRC_ERROR_SE_NO_ASSET			(RMF_VODSRC_ERROR_BASE + 0x9000)
#define RMF_VODSRC_ERROR_SE_NO_REPLICA			(RMF_VODSRC_ERROR_BASE + 0x9001)
#define RMF_VODSRC_ERROR_SE_BAD_CREDIT			(RMF_VODSRC_ERROR_BASE + 0x9002)
#define RMF_VODSRC_ERROR_SE_DURATION_TMO		(RMF_VODSRC_ERROR_BASE + 0x9003)
#define RMF_VODSRC_ERROR_SE_INTERNAL_ERROR		(RMF_VODSRC_ERROR_BASE + 0x9004)
#define RMF_VODSRC_ERROR_SE_BAD_ASSET			(RMF_VODSRC_ERROR_BASE + 0x9005)
#define RMF_VODSRC_ERROR_NE_CAS_FAILURE 		(RMF_VODSRC_ERROR_BASE + 0x9006)
#define RMF_VODSRC_ERROR_SE_PKG_ASSET_ID_MISMATCH	(RMF_VODSRC_ERROR_BASE + 0x9007)
#define RMF_VODSRC_ERROR_SE_NOT_SUBSCRIBED		(RMF_VODSRC_ERROR_BASE + 0x9008)
#define RMF_VODSRC_ERROR_SE_BLOCKED_ASSET		(RMF_VODSRC_ERROR_BASE + 0x9009)
#define RMF_VODSRC_ERROR_SE_VIEW_LIMIT_REACHED		(RMF_VODSRC_ERROR_BASE + 0x900a)
#define RMF_VODSRC_ERROR_SE_APP_NAME_SYNTAX_ERR 	(RMF_VODSRC_ERROR_BASE + 0x900b)
#define RMF_VODSRC_ERROR_SE_APP_NAME_NOT_FOUND		(RMF_VODSRC_ERROR_BASE + 0x900c)
#define RMF_VODSRC_ERROR_SE_PROVIDER_ID_SYNTAX_ERR	(RMF_VODSRC_ERROR_BASE + 0x900d)
#define RMF_VODSRC_ERROR_SE_PROVIDER_ID_NOT_FOUND	(RMF_VODSRC_ERROR_BASE + 0x900e)

#define RMF_VODSRC_ERROR_SRM_ERROR_001			(RMF_VODSRC_ERROR_BASE + 0x901b)
#define RMF_VODSRC_ERROR_SRM_ERROR_002			(RMF_VODSRC_ERROR_BASE + 0x901c)
#define RMF_VODSRC_ERROR_SRM_ERROR_003			(RMF_VODSRC_ERROR_BASE + 0x901d)
#define RMF_VODSRC_ERROR_SRM_ERROR_004			(RMF_VODSRC_ERROR_BASE + 0x901e)
#define RMF_VODSRC_ERROR_SRM_ERROR_005			(RMF_VODSRC_ERROR_BASE + 0x901f)
#define RMF_VODSRC_ERROR_SRM_ERROR_006			(RMF_VODSRC_ERROR_BASE + 0x9020)

#define RMF_VODSRC_ERROR_SRM_ITV_TUNING_ERROR		(RMF_VODSRC_ERROR_BASE + 0x8001)
#define RMF_VODSRC_ERROR_SRM_ITV_CM_CONNECT_ERROR	(RMF_VODSRC_ERROR_BASE + 0x8002)
#define RMF_VODSRC_ERROR_SRM_ITV_POSITION_ERROR 	(RMF_VODSRC_ERROR_BASE + 0x8003)
#define RMF_VODSRC_ERROR_SRM_ITV_USER_ERROR		(RMF_VODSRC_ERROR_BASE + 0x8004)
#define RMF_VODSRC_ERROR_SRM_ITV_SETTOP 		(RMF_VODSRC_ERROR_BASE + 0x8005)
#define RMF_VODSRC_ERROR_SRM_ITV_OTHER_ERROR		(RMF_VODSRC_ERROR_BASE + 0x8010)
#define RMF_VODSRC_ERROR_SRM_ITV_VIDEO_LOST_QAM_OK	(RMF_VODSRC_ERROR_BASE + 0x8012)
#define RMF_VODSRC_ERROR_SRM_ITV_RF_NOT_AVAILABLE	(RMF_VODSRC_ERROR_BASE + 0x8013)
#define RMF_VODSRC_ERROR_SRM_ITV_NO_MPEG_DATA		(RMF_VODSRC_ERROR_BASE + 0x8014)
#define RMF_VODSRC_ERROR_SRM_ITV_ISA_ERROR		(RMF_VODSRC_ERROR_BASE + 0x8015)
#define RMF_VODSRC_ERROR_SRM_ITV_TUNE_FREQ_MISMATCH	(RMF_VODSRC_ERROR_BASE + 0x8016)
#define RMF_VODSRC_ERROR_SRM_ITV_TUNE_TSID_MISMATCH	(RMF_VODSRC_ERROR_BASE + 0x8017)

#define RMF_VODSRC_ERROR_SRM_ERROR_016			(RMF_VODSRC_ERROR_BASE + 0x0021)

#define RMF_VODSRC_ERROR_SRM_ITV_TUNING_ERROR_IN_MIDDLE (RMF_VODSRC_ERROR_BASE + 0x8107)
#define RMF_VODSRC_ERROR_SRM_ITV_AD_FAILURE		(RMF_VODSRC_ERROR_BASE + 0x8101)

/**
 * @defgroup vodsourceclass VOD Source Classes
 * @ingroup vodsource
 *
 * @class RMFVODSrc
 * @brief This class defines the member functions for initialization and uninitialization of platform
 * dependent functionalities for VOD source and creates the private source implementation for rmf base class.
 * @ingroup vodsourceclass
 */
class RMFVODSrc:public RMFQAMSrc
{
  public:
	RMFVODSrc();
	~RMFVODSrc();
	RMFResult init();
	static RMFResult init_platform(  );
	static RMFResult uninit_platform(  );
	RMFMediaSourcePrivate *createPrivateSourceImpl(  );
	static void EOSEventCallback(void* objRef);

	void setWaitForClose();
};

#endif // RMFVODSOURCE_H
