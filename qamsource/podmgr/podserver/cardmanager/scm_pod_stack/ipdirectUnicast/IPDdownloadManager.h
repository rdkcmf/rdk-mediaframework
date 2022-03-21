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

#ifndef _IPDDOWNLOADMANAGER_H
#define _IPDDOWNLOADMANAGER_H

#include "httpFileFetcher.h"

/** 
 *  @file IPDdownloadManager.h
 *  @brief IP Direct download manager is used by ipdirectUnicast-main to manage required IP Controller document downloads.
 *
 *  This file contains the public definitions for the IPDdownloadManager object implementation.
 *  
 */


/*
 *	Data types used by the IPDdownloaderManager.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Define for the max size of a single data segment transaction between host and cable card. */ 
#define MAX_CABLE_CARD_DATASEG_SIZE					4096

typedef enum
{
    IPD_DM_STATE_INIT,
    IPD_DM_STATE_IDLE,
    IPD_DM_STATE_DOWNLOADING,
    IPD_DM_STATE_FAILED
}IPDdownloadMgrStates;

typedef enum
{
    IPD_DM_NO_ERROR,
    IPD_DM_ERROR,
    IPD_DM_INVALID_URL,
    IPD_DM_BUSY,
    IPD_DM_NOT_SUPPORTED
}IPDdownloadMgrRetCode;


/* Define an object that tracks the current IPDU Download Manager state on a coonection or URL basis. */
/* This is needed to support multiple concurrent connections by the IPDM. */
typedef std::map<std::string, IPDdownloadMgrStates> IPDMconnectionState_t;

static bool IsInitialized = false;                   /* Indicates if the IPDdownloaderManager has been initialized */
typedef struct _IPDdownloaderManager
{
	//IPDdownloadMgrStates State;
  //IPDMconnectionState_t tConnectionState;
	HttpFileFetcher_t	tHttpFileFetcher;


} IPDdownloaderManager_t;

/*
 *	These C style functions provide an interfaces between the HttpFileFetcher and the IP Direct Unicast module
 *	(ipdirectUnicast-main.cpp).
 */

/* ____________________________________________________________________ */
/* These functions are the new IP Direct APDU handler functions. */
/* ____________________________________________________________________ */

/* Currently all apdu requests are keyed off of the url provided by the cable card. */
/* This could be changed if required. */

/* ProcessApduHttpGetReq is the handler for http_get_req APDU.								*/
/*	input: cUrl, Identifies the url to fetch.																	*/
/*	return: IPDdownloadMgrRetCode.																						*/
IPDdownloadMgrRetCode ProcessApduHttpGetReq(char* cUrl);

/* ProcessApduTransferConfirmation is the handler for http_get_cnf and post_cnf APDU.	*/
/*	output: status, the http status code for the requested transfer.					*/
/*	return: void.																															*/
void ProcessApduTransferConfirmation(char* cUrl, int* status);

/* ProcessApduHttpGetCnf is the handler for http_recv_data_seg_info APDU.			*/
/*	output: totalFileSize, the size of the fiel downloaded from cUrl in bytes.*/
/*	output: numSegments, the http status code for the Http Get Request.				*/
/*	return: void.																															*/
void ProcessApduHttpRecvDataSegInfo(char* cUrl, unsigned int* totalFileSize, unsigned int* numSegments);

/* ProcessApduFlowDeleteReq is the handler for flow_delete_cnf APDU.					*/
/*	input: cUrl, Identifies the url to fetch.																	*/
/*	return: void.																															*/
void ProcessApduFlowDeleteReq(char* cUrl);

/* ProcessApduHttpPost performs a HTTP Post to send a file to a specified URL. 			*/
/*	input: cUrl, Identifies the url to send the outputfile to.											*/
/*	input: outputFile, Contains the data (file) that is to be sent to cUrl.					*/
/*	output: httpStatus, the http serevr status. Valid if IPDdownloadMgrRetCode = No error	*/
/*	return: IPDdownloadMgrRetCode.																						*/
IPDdownloadMgrRetCode ProcessApduHttpPost(char* cUrl, std::string outputFile, int* httpStatus);

/* ____________________________________________________________________ */
/* These functions are utility functions used by the IP Direct feature. */
/* ____________________________________________________________________ */

/* InitializeIPDdownloaderManager- initialize the object.											*/
/*	return: false on success, true on error.																	*/
bool InitializeIPDdownloaderManager(/* */);

/* CheckInitialization- Check Initialization of the IPDU DownloadManager		        				  */
/* Assures the IPDU downloadManager will get init'd if the IPDU APDU sequence from cable card changes.    */
void CheckInitialization();

/* GetTransferStatus used to get the transfer status for a given url.					*/
/*	input: cUrl, Identifies the url for which status is being requested.			*/
/*	output: httpStatus, the http server status. Valid if IPDdownloadMgrRetCode = No error	*/
/*	return: IPDdownloadMgrRetCode.																						*/
IPDdownloadMgrRetCode GetTransferStatus(char* cUrl, int* httpStatus);

/* CheckTransferComlete used poll curl status for asynchronous transfers.					*/
/*	input: cUrl, Identifies the url for which status is being requested.			*/
/*	return: IPDdownloadMgrRetCode. IPD_DM_BUSY== transfer in progress, IPD_DM_NO_ERROR==transfer complete*/
IPDdownloadMgrRetCode CheckTransferComplete(char* cUrl);

/* GetDownloadStatus used to get the download status for a given url.					*/
/*	input: cUrl, Identifies the url for which status is being requested.			*/
/*	output: httpStatus, the http serevr status. Valid if IPDdownloadMgrRetCode = No error	*/
/*	return: IPDdownloadMgrRetCode.																						*/
IPDdownloadMgrRetCode GetDownloadStatus(char* cUrl, int* httpStatus);

/* GetDownloadedFile is to access the downloaded file for a given url.								*/
/*	input: cUrl, Identifies the url for which downloaded file is being requested.			*/
/*	output: fileData, pointer to pointer to an array of downloaded file data.					*/
/*	output: fileSize, Size of the download file.																			*/
/*	return: Pointer to the downloaded file. NULL if file not available.								*/
IPDdownloadMgrRetCode GetDownloadedFile(char * cUrl, const unsigned char** fileData, unsigned int* fileSize);

/* TerminateDownload is a cleanup utility, should be called to cleanup							*/
/* after a url has been downloaded from a server and uploaded to the cable card.		*/
/*	input: cUrl, Identifies the url to terminate.																		*/
/*	return: void.																																		*/
void TerminateDownload(char * cUrl);

/* Indicates HTTP Xfer mode, Synchronous (blocking) or Asynchronous (non-blocking). TRUE=Synchronous download, FALSE=Asynchronous. */
bool IsSynchronousTransferMode();

/* Configures the desired File Download timeout value. */
void SetFileDownloadTimeout(int timeoutInseconds);

/* Configures the desired File Upload timeout value. */
void SetFileUploadTimeout(int timeoutInseconds);

/* Configures the desired File Download tcp connection timeout value. */
void SetFileDownloadConnectionTimeout(int timeoutInseconds);

/* Configures the desired File Upload tcp connection timeout value. */
void SetFileUploadConnectionTimeout(int timeoutInseconds);

#ifdef __cplusplus
}
#endif


#endif		/* _IPDDOWNLOADMANAGER_H */
