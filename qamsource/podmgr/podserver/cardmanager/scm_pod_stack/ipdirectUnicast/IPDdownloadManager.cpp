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
/* file: IPDdownloadManager.cpp */

/* Because the ipdirectUnicast task/process does not use the conditional feature build  */
/* switch USE_IPDIRECT_UNICAST the IPDdownloadManager will always be compiled in and    */
/* the function contents will use the USE_IPDIRECT_UNICAST build switch.                */

// These next 3 headers are required to use RDK_LOG.
#include "poddef.h"
#include "lcsm_log.h"
#include <rdk_debug.h>
#include <string.h>

#include "IPDdownloadManager.h"
#include "httpFileFetcher.h"
#include <curl/curl.h>

/* Globals */
IPDdownloaderManager_t tIPDdownloaderManager;
/* The IP Direct unicast HTTP Post APDU requires the payload data to be base64 encoded. */
std::string encodedHttpPostFile;

#ifdef USE_IPDIRECT_UNICAST
	extern HttpFileFetcher_t	tHttpFileFetcher;
#endif


/* ProcessApduHttpGetReq is the handler for http_get_req APDU.								*/
/*	return: void.																															*/
IPDdownloadMgrRetCode ProcessApduHttpGetReq(char * cUrl)
{
	IPDdownloadMgrRetCode retCode = IPD_DM_NOT_SUPPORTED;
#ifdef USE_IPDIRECT_UNICAST
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Url= %s \n", __FUNCTION__, cUrl);
	retCode = IPD_DM_NO_ERROR;

    CheckInitialization();
    std::string strUrl = cUrl;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Begin IPDU file download.\n", __FUNCTION__);
    CURLcode curlRet = DownloadFile(strUrl);
    if (CURLE_FAILED_INIT == curlRet)
    {
	    // If failed init then no connection attempt was made so return error. Else
	    // client can just use http server response.
	    retCode = IPD_DM_ERROR;
    }
    /* Else now start a timer to monitor download progress. */
#endif
	return retCode;
}

/* ProcessApduTransferConfirmation is the handler for all transfer requests.  */
/*	return: void.																															*/
void ProcessApduTransferConfirmation(char* cUrl, int* status)
{
#ifdef USE_IPDIRECT_UNICAST
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    CheckInitialization();
	GetTransferStatus(cUrl, status);
#endif
}


/* ProcessApduHttpRecvDataSegInfo is the handler for http_recv_data_seg_info APDU.			*/
void ProcessApduHttpRecvDataSegInfo(char* cUrl, unsigned int* totalFileSize, unsigned int* numSegments)
{
#ifdef USE_IPDIRECT_UNICAST
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
	unsigned int fileSize;
	unsigned int segments;
	const unsigned char* file ;
    CheckInitialization();
	GetDownloadedFile(cUrl, &file, &fileSize);
	if (fileSize  <= MAX_CABLE_CARD_DATASEG_SIZE)
	{
		segments = 1;
	}
	else
	{
		segments = fileSize/MAX_CABLE_CARD_DATASEG_SIZE;
		/* Adjust segment count for partial segments. */
		if (fileSize % (segments*MAX_CABLE_CARD_DATASEG_SIZE))
		{
			segments++;
		}
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: File Size= %u.\n", __FUNCTION__, fileSize);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Num cable card segments = %u.\n", __FUNCTION__, segments);
#endif
}

/* ProcessApduFlowDeleteCnf is the handler for flow_delete_cnf APDU.					*/
void ProcessApduFlowDeleteReq(char* cUrl)
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
#ifdef USE_IPDIRECT_UNICAST
    CheckInitialization();
	/* Not sure what the Flow Delete req is intended for. */
	/* Currently it is configured to terminate and cleanup the download. */
	TerminateDownload(cUrl);
#endif
}


/* InitializeIPDdownloaderManager.																						*/
/*	return: bool, 0=success, 1=failure																				*/
bool InitializeIPDdownloaderManager()
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
	bool retCode = false;

#ifdef USE_IPDIRECT_UNICAST
    if(!IsInitialized)
    {
	CURLcode curlRetCode = InitHttpFileFetcher();
	if (CURLE_OK != curlRetCode)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: InitHttpFileFetcher failed.\n", __FUNCTION__);
		retCode = true;
	}
	else
	{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s Download Manager initialized.\n", __FUNCTION__);
	}
    IsInitialized = true;
    }
    else
    {
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s Skip Download Manager initialization.\n", __FUNCTION__);
    }
#endif
	return retCode;
}

void CheckInitialization()
{
    if(!IsInitialized)
    {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: IPDdownloaderManager NOT initialized before being used.\n", __FUNCTION__);
        // Force an init.
        InitializeIPDdownloaderManager();
    }
}

IPDdownloadMgrRetCode GetTransferStatus(char* cUrl, int* httpStatus)
{
	IPDdownloadMgrRetCode retCode = IPD_DM_NOT_SUPPORTED;
#ifdef USE_IPDIRECT_UNICAST
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Url= %s \n", __FUNCTION__, cUrl);
	CURL* curlEasyHandle;
	retCode = IPD_DM_NO_ERROR;
	*httpStatus = HTTP_RESPONSE_ON_CURL_ERROR;																	/* Default to Bad Request. */

	//First have to get curl handle from url so the http status can be looked up.
	CurlSessions_t::iterator i = tHttpFileFetcher.tCurlSessions.find(cUrl);
	if (i != tHttpFileFetcher.tCurlSessions.end())
	{
		/* url found, get associated curl handle. */
		curlEasyHandle = i->second;
  	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: \tassociated curl handle= %u \n", __FUNCTION__, (unsigned int)curlEasyHandle);
	}
	else
	{
		RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No curl handle found for specified url.\n", __FUNCTION__);
		retCode = IPD_DM_INVALID_URL;
	}
	// If curl handle found proceed to get http status for that handle.
	if (IPD_DM_NO_ERROR == retCode)
	{
		HttpStatusCodes_t::iterator i = tHttpFileFetcher.tHttpStatusCodes.find(curlEasyHandle);
		if (i != tHttpFileFetcher.tHttpStatusCodes.end())
		{
			*httpStatus = i->second;				/* Extract actual http status code. */
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: returning http status= %d for url %s.\n", __FUNCTION__, *httpStatus, cUrl);
		}
		else
		{
			RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No transfer status for specified handle.\n", __FUNCTION__);
			retCode = IPD_DM_INVALID_URL;
		}
	}
#endif
	return retCode;
}


IPDdownloadMgrRetCode CheckTransferComplete(char* cUrl)
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s for url %s \n", __FUNCTION__, cUrl);
	IPDdownloadMgrRetCode retCode = IPD_DM_NO_ERROR;
	CURL* curlEasyHandle;

	//First have to get curl handle from url so the http status can be looked up.
	CurlSessions_t::iterator i = tHttpFileFetcher.tCurlSessions.find(cUrl);
	if (i != tHttpFileFetcher.tCurlSessions.end())
	{
		/* url found, get associated curl handle. */
		curlEasyHandle = i->second;
  	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: \tassociated curl handle= %u \n", __FUNCTION__, (unsigned int)curlEasyHandle);
	}
	else
	{
		RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No curl handle found for specified url.\n", __FUNCTION__);
		retCode = IPD_DM_INVALID_URL;
	}
	if (IPD_DM_NO_ERROR == retCode)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Checking download status.\n", __FUNCTION__);
		CURLcode curlRet = CheckDownloadStatus(curlEasyHandle);
		if (CURLE_OK != curlRet)
		{
  		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Currently downloading.\n", __FUNCTION__);
			retCode = IPD_DM_BUSY;
		}
    else
    {
  		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s Download Manager back to Idle State.\n", __FUNCTION__);
    }
	}
	return retCode;
}


IPDdownloadMgrRetCode GetDownloadedFile(char * cUrl, const unsigned char** fileData, unsigned int* fileSize)
{
	IPDdownloadMgrRetCode retCode = IPD_DM_NOT_SUPPORTED;
#ifdef USE_IPDIRECT_UNICAST
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
	CURL* curlEasyHandle;
	retCode = IPD_DM_NO_ERROR;

	//First have to get curl handle from url so the http status can be looked up.
	CurlSessions_t::iterator i = tHttpFileFetcher.tCurlSessions.find(cUrl);
	if (i != tHttpFileFetcher.tCurlSessions.end())
	{
		/* url found, get associated curl handle. */
		curlEasyHandle = i->second;
  	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: \tassociated curl handle= %u \n", __FUNCTION__, (unsigned int)curlEasyHandle);
	}
	else
	{
		RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No curl handle found for specified url.\n", __FUNCTION__);
		retCode = IPD_DM_INVALID_URL;
	}
	// If curl handle found proceed to get downloaded file.
	if (IPD_DM_NO_ERROR == retCode)
	{
		/* curl handle found, get associated file. */
		HttpSessions_t::iterator i = tHttpFileFetcher.tHttpSessions.find(curlEasyHandle);
		if (i != tHttpFileFetcher.tHttpSessions.end())
		{
			/* Access the actual data for the downloaded file. */
			*fileData = reinterpret_cast<const unsigned char*>((i->second).data());
			*fileSize = static_cast<unsigned int>((i->second).size());
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: returning pointer to downloaded file, size= %u.\n", __FUNCTION__, *fileSize);
		}
		else
		{
			RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No download file for specified url.\n", __FUNCTION__);
			retCode = IPD_DM_INVALID_URL;
		}
	}
#endif
	return retCode;
}

void TerminateDownload(char * cUrl)
{
#ifdef USE_IPDIRECT_UNICAST
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
	CURL* curlEasyHandle;

	//First have to get curl handle from url so the http status can be looked up.
	CurlSessions_t::iterator i = tHttpFileFetcher.tCurlSessions.find(cUrl);
	if (i != tHttpFileFetcher.tCurlSessions.end())
	{
		/* url found, get associated curl handle. */
		curlEasyHandle = i->second;
  	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: \tassociated curl handle= %u \n", __FUNCTION__, (unsigned int)curlEasyHandle);
		// If curl handle found proceed to terminate download/cleanup.
		CancelDownload(curlEasyHandle);
		tHttpFileFetcher.tCurlSessions.erase(cUrl);

	}
	else
	{
		RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No curl handle found for specified url.\n", __FUNCTION__);
	}
#endif
}

IPDdownloadMgrRetCode ProcessApduHttpPost(char* cUrl, std::string outputFile, int* httpStatus)
{
	IPDdownloadMgrRetCode retCode = IPD_DM_NOT_SUPPORTED;
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

#ifdef USE_IPDIRECT_UNICAST
	retCode = IPD_DM_NO_ERROR;
	std::string strUrl = cUrl;

    CheckInitialization();
	// The IP Direct unicast HTTP Post APDU requires the payload data to be base64 encoded.
	encodedHttpPostFile = Base64Encode((unsigned char*)outputFile.data(), outputFile.size());

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s, url= %s, encoded file size= %u  \n", __FUNCTION__, cUrl, encodedHttpPostFile.size());
	CURLcode curlRetCode = UploadFile(strUrl, encodedHttpPostFile, httpStatus);
	if (CURLE_OK != curlRetCode)
	{
		RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s curlError= %d.\n", __FUNCTION__, curlRetCode);
		retCode = IPD_DM_ERROR;
	}
#endif
	return retCode;
}

bool IsSynchronousTransferMode()
{
#ifdef USE_IPDIRECT_UNICAST
	return UsingBlockingTransferMode();
#else
	return false;
#endif
}

void SetFileDownloadTimeout(int timeoutInseconds)
{
  SetHttpGetTimeout(timeoutInseconds);
}

void SetFileUploadTimeout(int timeoutInseconds)
{
  SetHttpPostTimeout(timeoutInseconds);
}

void SetFileDownloadConnectionTimeout(int timeoutInseconds)
{
  SetHttpGetConnectionTimeout(timeoutInseconds);
}

void SetFileUploadConnectionTimeout(int timeoutInseconds)
{
  SetHttpPostConnectionTimeout(timeoutInseconds);
}


