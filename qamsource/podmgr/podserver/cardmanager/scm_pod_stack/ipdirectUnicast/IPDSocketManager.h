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

#ifndef _IPDSOCKETMANAGER_H
#define _IPDSOCKETMANAGER_H

typedef struct {
  int local_port;
} socketThreadData;

/** 
 *  @file IPDSocketManager.h
 *  @brief IP Direct socket manager is used by ipdirectUnicast-main to process socket flow APDUs.
 *
 *  This file contains the public definitions for the IPDSocketManager object implementation.
 *  
 */


/*
 *	Data types used by the IPDSocketManager.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ____________________________________________________________________ */
/* These functions are the new IP Direct Socket Flow APDU handler functions. */
/* ____________________________________________________________________ */


/* ProcessApduSocketFlowReq is the handler for socket_flow_req APDU.								*/
void ProcessApduSocketFlowReq(unsigned long Port);

/* ProcessApduSocketFlowCnf is the handler for socket_flow_cnf APDU.								*/
/*	output: status, the http status code for the Http Get Request.						*/
/*	return: void.																															*/
void ProcessApduSocketFlowCnf(void);

void KillSocketFlowThread(void);

#ifdef __cplusplus
}
#endif


#endif		/* _IPDSOCKETMANAGER_H */
