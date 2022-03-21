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
 * @file ippvsrc.h
 * @brief IPPV source header file.
 */

/**
 * @defgroup ippvsource IPPV Source
 * <b>IPPV source is responsible for the following operations. </b>
 * <ul>
 * <li> Mediastreamer takes  IPPV event streaming requests and parses the url and uses iPPVSrc to handle requests from outside.
 * <li> IPPVSrc sends command to CANH process depending on the request from mediastreamer. It takes the response from CANH and gives back to media streamer.
 * <li> In IPPV source event purchase, IPC buffer mechanism is placed to communicate between CANH server and iPPV IPC client.
 * <li> IPC buffer mechanism is used to share the data across multiple processes and categorized as server and client, where the client requests data and
 * the server responds to client requests.
 * <li> CANH Process is where CANH communicates with cable card through OCAP IPC client using APDUs[Application protocol data unit].
 * <li> CANH IPC Server takes command from iPPV client and uses CANH for executing them.
 * </ul>
 * @ingroup RMF
 *
 * @defgroup ippvsrcapi IPPV Source API list
 * @ingroup ippvsource
 */

#include "rmf_error.h"
#include <rmfqamsrc.h>

/**
 * @class RMFiPPVSrc
 * @brief Class extending RMFQAMSrc to implement RMFiPPVSrc.
 * @ingroup ippvclass
 */

#ifdef USE_SDVSRC
#include "rmfsdvsource.h"
class RMFiPPVSrc:public RMFSDVSrc
#else
class RMFiPPVSrc:public RMFQAMSrc
#endif
{

  public:
	RMFiPPVSrc(  );
	~RMFiPPVSrc(  );

    /**
	 * @brief initialization of platform dependent functionalities
 	 *
 	 * @return RMFResult	 defined in rmfcore.h
	 *
 	 * @retval RMF_RESULT_SUCCESS	   No Error.
     * @retval RMF_RESULT_FAILURE	Failure.
     */
	static RMFResult init_platform(  );

    /**
     * @brief uninitialization of platform dependent functionalities
     *
     * @return RMFResult	 defined in rmfcore.h
     *
     * @retval RMF_RESULT_SUCCESS	   No Error.
     * @retval RMF_RESULT_FAILURE	Failure.
     */
	static RMFResult uninit_platform(  );

    /**
     * @brief Creates private implementation and return. Used
     * by rmf base implementation
     *
     * @return RMFMediaSourcePrivate*	  Private implemention of QAM Src
     *
     */
	RMFMediaSourcePrivate *createPrivateSourceImpl(  );
	RMFResult purchasePPVEvent( unsigned int &eId );

};
