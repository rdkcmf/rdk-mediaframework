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
 * ============================================================================
 * @file SDVDiscovery.h *
 * @brief Interface to SDVDiscovery class for MCP discovery operations
 * for the SDV client. *
 * @Author Jason Bedard jasbedar@cisco.com
 * @Date Sept 11 2014
 */
#ifndef SDVDISCOVERY_H_
#define SDVDISCOVERY_H_

#include <rmfsdvsrcpriv.h>

/**
 * @class SDVDiscovery
 *
 * @brief Interface to SDVDiscovery class for MCP discovery operations
 *
 * MCMIS Configuration also known as MCP data is carried on all SDV transports
 * and contains critical Configuration information needed by an SDV client to
 * communicate with the SDV Server and resolve tuning parameters.
 *
 * In order for an SDVAgent to resolve tuning parameters it first needs to tune to
 * a transport containing the MCP information and read it.
 *
 * SDVDiscovery provides the attemptDiscoveryIfNeeded API which should be called
 * by any application which does tuning operations prior to construction of a
 * QAMSource object.
 *
 * SDVDiscovery will know if the SDVAgent is in a state where MCP data is required.
 * it is is the attemptDiscoveryIfNeeded will block while an autodiscovery attempt
 * is made to a single SDV frequency.  If this tune did not result in finding MCP
 * data another attempt will be made the next time attemptDiscoveryIfNeeded is called
 * but to a different sdv frequency
 *
 * SDVDiscovery will look in /tmp/sdvscanlist.txt for a list of locators it should
 * for discovery operations.
 *
 * SDVDiscovery will also listen for SectionFiltering events which are received
 * automatically when the SDV MCP data changes while a QAMSource is tuned to an
 * SVD channel. The events will be propagated to SDVAgent.
 * @ingroup sdvclass
 */
class SDVDiscovery {
public:
    static SDVDiscovery * getInstance();
protected:
    /**
     * @constructor
     * @brief private constructor for singleton
     */
    SDVDiscovery(){};
    virtual ~SDVDiscovery(){};
public:
    virtual void start() = 0;

    virtual void attemptDiscoveryIfNeeded(RMFQAMSrcImpl * qamsource) = 0;
};

#endif /* SDVDISCOVERY_H_ */
