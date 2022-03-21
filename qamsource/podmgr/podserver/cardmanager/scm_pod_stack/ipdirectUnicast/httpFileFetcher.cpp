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

/* file: httpFileFetcher.cpp */

#ifdef USE_IPDIRECT_UNICAST

// These next 3 headers are required to use RDK_LOG.
#include <string.h>
#include "poddef.h"
#include "lcsm_log.h"
#include <rdk_debug.h>
#include "httpFileFetcher.h"

/*    Currently the httpfileFetcher is implemented as a C module. This file scope tHttpFileFetcher object is the C- structure
        Note- If this module can be implemented as a C++ object this can be eliminated.
*/
HttpFileFetcher_t    tHttpFileFetcher;

CURLcode InitHttpFileFetcher()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    CURLcode retCode = CURLE_OK;

    tHttpFileFetcher.uploadTimeout = FILE_UPLOAD_TIMEOUT_SEC;
    tHttpFileFetcher.downloadTimeout = HTTP_GET_TIMEOUT_SEC;
    tHttpFileFetcher.uploadConnectionTimeout = FILE_UPLOAD_CONNECT_TIMEOUT_SEC;
    tHttpFileFetcher.downloadConnectionTimeout = HTTP_GET_CONNECT_TIMEOUT_SEC;
    tHttpFileFetcher.networkUp = false;

    char* pcCurlVer = curl_version();
    if(NULL != pcCurlVer)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s libcurl Version= %s.\n", __FUNCTION__, pcCurlVer);
    }
    else
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s libcurl Version could not retrieved.\n", __FUNCTION__);
    }

    tHttpFileFetcher.useBlockingTransfer = HTTP_FILE_TRANSFER_MODE;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s tHttpFileFetcher.useBlockingTransfer = %d.\n", __FUNCTION__, tHttpFileFetcher.useBlockingTransfer);
    curl_global_init(CURL_GLOBAL_ALL);

    /* Using the curl multi interface allows multiple simultaneous curl easy handles to be serviced in a non-blocking manner. */
      tHttpFileFetcher.curlMultiHandle = curl_multi_init();                        /* Create a curl multi handle. */


    if(NULL == tHttpFileFetcher.curlMultiHandle)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_multi_init failed.\n", __FUNCTION__);
        retCode = CURLE_FAILED_INIT;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: New curlMultiHandle= %u\n", __FUNCTION__, (unsigned int)tHttpFileFetcher.curlMultiHandle);
    }

    return retCode;
}


CURLcode DownloadFile(std::string url)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: url= %s \n", __FUNCTION__, url.c_str());
    CURLcode retCode = CURLE_FAILED_INIT;
    CURLMcode multiError ;

    /* There is no validation of the URL requested by the CC. */
    CURL* curlEasyHandle = curl_easy_init();
    if(NULL != curlEasyHandle)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: New curlEasyHandle= %u\n", __FUNCTION__, (unsigned int)curlEasyHandle);
        retCode = CURLE_OK;
        tHttpFileFetcher.tCurlSessions[url] = curlEasyHandle;
        /* Now save the new download file url request. */
        tHttpFileFetcher.tHttpSessionUrls[curlEasyHandle] = url;
        /* Now associate the curl handle with the file (string) the data will be downloaded into. */ 
        tHttpFileFetcher.tHttpSessions[curlEasyHandle] = "";            /* Start with an empty file. */

        /* Now configure the new connection. */
        curl_easy_setopt(curlEasyHandle, CURLOPT_WRITEFUNCTION, writeCurlCallback);
        /* So the writeCurlCallback can append new data to the appropriate "file" or buffer use the CURLOPT_WRITEDATA option. */
        /* Pass in the std::string associated with the easy handle. */

        std::string* pDownloadFile = NULL;
        HttpSessions_t::iterator i = tHttpFileFetcher.tHttpSessions.find(curlEasyHandle);
        if (i != tHttpFileFetcher.tHttpSessions.end()) {
            /* found handle, now check status. */
            pDownloadFile = &(i->second);
        }
        else
        {
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No download file for specified handle.\n", __FUNCTION__);
        }
        /* Execute tasks common between blocking and non-blocking. */
        curl_easy_setopt(curlEasyHandle, CURLOPT_WRITEDATA, static_cast<void*>(pDownloadFile));
        curl_easy_setopt(curlEasyHandle, CURLOPT_HEADER, 0);
        curl_easy_setopt(curlEasyHandle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curlEasyHandle, CURLOPT_PRIVATE, url.c_str());
        curl_easy_setopt(curlEasyHandle, CURLOPT_TIMEOUT,  tHttpFileFetcher.downloadTimeout);
        curl_easy_setopt(curlEasyHandle, CURLOPT_CONNECTTIMEOUT,  tHttpFileFetcher.downloadConnectionTimeout);
        curl_easy_setopt(curlEasyHandle, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curlEasyHandle, CURLOPT_DEBUGFUNCTION, CurlDebugCallback);
        // Add support SSL, https: urls
        curl_easy_setopt(curlEasyHandle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curlEasyHandle, CURLOPT_SSL_VERIFYHOST, 2L);
        // HTTP get cert file name
        curl_easy_setopt(curlEasyHandle, CURLOPT_CAPATH , "/etc/ssl/certs");  // HTTP Get required cert
        curl_easy_setopt(curlEasyHandle, CURLOPT_CERTINFO, 1L);                                   // Debug info

        /* Execute download mode specific tasks. */
        if (!tHttpFileFetcher.useBlockingTransfer)
        {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Add easyhandle %u to multi session\n", __FUNCTION__, (unsigned int)curlEasyHandle);
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Easyhandle writing to downloadfile at %p.\n", __FUNCTION__, static_cast<void*>(pDownloadFile));
            multiError = curl_multi_add_handle(tHttpFileFetcher.curlMultiHandle, curlEasyHandle);
            if (CURLM_OK != multiError)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_multi_add_handle error= %u.\n", __FUNCTION__, multiError);
            }
        }

        CURLMcode multiError = InitiateTransfers(curlEasyHandle);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curlErrorCode= %u, curlHandle= %u\n", __FUNCTION__, (unsigned int)multiError, (unsigned int)curlEasyHandle);
        if (!tHttpFileFetcher.useBlockingTransfer)
        {
            if (CURLM_OK == multiError)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Non-Blocking curl transfer started.\n", __FUNCTION__);
                /* start a timer to periodically test for download complete only when using non-blocking mode. */
                /* Currently transfer timer is started/managed in ipdirectUnicast-main.c, vlIpduDownloadProgressTimer. */
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Non-Blocking curl transfer failed to start.\n", __FUNCTION__);
                TerminateTransfer(curlEasyHandle);
                retCode = CURLE_FAILED_INIT;
            }
        }
        if (tHttpFileFetcher.useBlockingTransfer)
        {
            if (CURLM_OK == multiError)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Blocking curl transfer completed with no error.\n", __FUNCTION__);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Blocking curl transfer failed.\n", __FUNCTION__);
                TerminateTransfer(curlEasyHandle);
                retCode = CURLE_FAILED_INIT;
            }
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_easy_init failed.\n", __FUNCTION__);
    }
    return retCode;
}

/* This may not be a needed method. */
CURLcode DownloadStatus(CURL* handle)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    CURLcode retCode = CURLE_OK;
    HttpSessions_t::iterator i = tHttpFileFetcher.tHttpSessions.find(handle);
    if (i != tHttpFileFetcher.tHttpSessions.end()) {
        /* found handle, now check status. */
        /* get status. */
    }
    else
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No curl connection for specified handle.\n", __FUNCTION__);
        retCode = CURLE_COULDNT_CONNECT;                    /* Used to signal invalid CURL handle. */
    }

    return retCode;
}

CURLcode CancelDownload(CURL* handle)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    CURLcode retCode = CURLE_OK;
    HttpSessions_t::iterator i = tHttpFileFetcher.tHttpSessions.find(handle);
    if (i != tHttpFileFetcher.tHttpSessions.end()) 
    {
        // cleanup non curl related.
        TerminateTransfer(handle);
        tHttpFileFetcher.tHttpSessionUrls.erase(handle);
        tHttpFileFetcher.tHttpStatusCodes.erase(handle);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: curl download cancelled.\n", __FUNCTION__);
    }
    else 
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No curl connection for specified handle.\n", __FUNCTION__);
        retCode = CURLE_COULDNT_CONNECT;                    /* Used to signal invalid CURL handle. */
    }

    return retCode;
}

std::string GetFile(CURL* handle)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: curlEasyHandle= %u \n", __FUNCTION__, (unsigned int)handle);
    std::string downloadedFile = "";
    HttpSessions_t::iterator i = tHttpFileFetcher.tHttpSessions.find(handle);
    if (i != tHttpFileFetcher.tHttpSessions.end()) 
    {
        downloadedFile = i->second;
    }
    else
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s: No curl connection for specified handle.\n", __FUNCTION__);
    }
    // empty file name is returned if handle not found.
    return downloadedFile;
}

extern "C" {
extern void vlMpeosDumpBuffer(rdk_LogLevel lvl, const char *mod, const void * pBuffer, int nBufSize);
}

CURLcode UploadFile(std::string url, std::string outputFile, int* httpStatus)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: server url= %s  \n", __FUNCTION__, url.c_str());
    CURLcode retCode = CURLE_FAILED_INIT;
    CURLMcode multiError ;

    /* There is no validation of the URL or the file to be uploaded. */
    *httpStatus = HTTP_RESPONSE_ON_CURL_ERROR;
    CURL* curlEasyHandle = curl_easy_init();
    if(NULL != curlEasyHandle)
    {
        retCode = CURLE_OK;
        tHttpFileFetcher.tCurlSessions[url] = curlEasyHandle;
        /* Now save the new download file url request. */
        tHttpFileFetcher.tHttpSessionUrls[curlEasyHandle] = url;
        /* Now associate the curl handle with the file (string) the data will be downloaded into. */ 
        tHttpFileFetcher.tHttpSessions[curlEasyHandle] = "";            /* Start with an empty file. */

        /* Now configure the upload connection. */
        curl_easy_setopt(curlEasyHandle, CURLOPT_URL, url.c_str());
        //curl_easy_setopt(curlEasyHandle, CURLOPT_USERAGENT, "XG1V1");
        curl_easy_setopt(curlEasyHandle, CURLOPT_POSTFIELDS, outputFile.data());
        curl_easy_setopt(curlEasyHandle, CURLOPT_POSTFIELDSIZE, outputFile.size());
        curl_easy_setopt(curlEasyHandle, CURLOPT_POST, 1L);
        curl_easy_setopt(curlEasyHandle, CURLOPT_TIMEOUT, tHttpFileFetcher.uploadTimeout);
        curl_easy_setopt(curlEasyHandle, CURLOPT_CONNECTTIMEOUT,  tHttpFileFetcher.uploadConnectionTimeout);
        curl_easy_setopt(curlEasyHandle, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curlEasyHandle, CURLOPT_DEBUGFUNCTION, CurlDebugCallback);
        // Add support SSL, https: urls
        // As of now only the Post is using SSL.
        curl_easy_setopt(curlEasyHandle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curlEasyHandle, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curlEasyHandle, CURLOPT_CAPATH , "/etc/ssl/certs");     // HTTP Post required cert
        curl_easy_setopt(curlEasyHandle, CURLOPT_CERTINFO, 1L);                                     // Debug info

        // Set specific Content-Type header;
        struct curl_slist *list = NULL;
        list = curl_slist_append(list, "Content-Type: text/plain");
        curl_easy_setopt(curlEasyHandle, CURLOPT_HTTPHEADER, list);

        /* Execute download mode specific tasks. */
        if (!tHttpFileFetcher.useBlockingTransfer)
        {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Add easyhandle %u to multi session\n", __FUNCTION__, (unsigned int)curlEasyHandle);
            multiError = curl_multi_add_handle(tHttpFileFetcher.curlMultiHandle, curlEasyHandle);
            if (CURLM_OK != multiError)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_multi_add_handle error= %u.\n", __FUNCTION__, multiError);
            }
        }

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Start blocking file upload, file size = %u.\n", __FUNCTION__, outputFile.size());

        multiError = InitiateTransfers(curlEasyHandle);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curlErrorCode= %u, curlEasyHandle= %u\n", __FUNCTION__, (unsigned int)multiError, (unsigned int)curlEasyHandle);
        if (!tHttpFileFetcher.useBlockingTransfer)
        {
            if (CURLM_OK == multiError)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Non-Blocking curl transfer started.\n", __FUNCTION__);
                /* start a timer to periodically test for download complete only when using non-blocking mode. */
                /* Currently transfer timer is started/managed in ipdirectUnicast-main.c, vlIpduDownloadProgressTimer. */
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Non-Blocking curl transfer failed to start.\n", __FUNCTION__);
                TerminateTransfer(curlEasyHandle);
                retCode = CURLE_FAILED_INIT;
            }
        }
        if (tHttpFileFetcher.useBlockingTransfer)
        {
            if (CURLM_OK == multiError)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Blocking curl transfer completed with no error.\n", __FUNCTION__);
                long response ;
                retCode = curl_easy_getinfo(curlEasyHandle, CURLINFO_RESPONSE_CODE, &response);
                if (CURLE_OK == retCode)
                {
                    *httpStatus = response;
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curl http Response=%d\n", __FUNCTION__, *httpStatus);
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_easy_getinfo ERROR- %d.\n", __FUNCTION__, retCode);
                }
                sslDiagnostics(curlEasyHandle);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Blocking curl transfer failed.\n", __FUNCTION__);
                TerminateTransfer(curlEasyHandle);
                retCode = CURLE_FAILED_INIT;
            }
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s:  curl_easy_init failed.\n", __FUNCTION__);
    }
    return retCode;
}

void SetHttpGetTimeout(int timeoutInSeconds)
{
    tHttpFileFetcher.downloadTimeout = timeoutInSeconds;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: downloadTimeout = %d\n", __FUNCTION__, tHttpFileFetcher.downloadTimeout);
}

void SetHttpPostTimeout(int timeoutInSeconds)
{
    tHttpFileFetcher.uploadTimeout = timeoutInSeconds;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: uploadTimeout = %d\n", __FUNCTION__, tHttpFileFetcher.uploadTimeout);
}

void SetHttpGetConnectionTimeout(int timeoutInSeconds)
{
    tHttpFileFetcher.downloadConnectionTimeout = timeoutInSeconds;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: downloadConnectionTimeout = %d\n", __FUNCTION__, tHttpFileFetcher.downloadConnectionTimeout);
}

void SetHttpPostConnectionTimeout(int timeoutInSeconds)
{
    tHttpFileFetcher.uploadConnectionTimeout = timeoutInSeconds;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: uploadConnectionTimeout = %d\n", __FUNCTION__, tHttpFileFetcher.uploadConnectionTimeout);
}

bool IsNetworkUp()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: networkUp = %d\n", __FUNCTION__, tHttpFileFetcher.networkUp);
    return tHttpFileFetcher.networkUp;
}

CURLMcode InitiateTransfers(CURL* handle)
{
    int numRunningHandles;
    CURLMcode multiError = CURLM_OK;

    if (!tHttpFileFetcher.useBlockingTransfer)
    {
        /* For non-blocking the handle input parameter is not used. */
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Non-Blocking curlMulti Xfer Mode \n", __FUNCTION__);
        multiError = curl_multi_perform(tHttpFileFetcher.curlMultiHandle, &numRunningHandles);
        if (CURLM_OK == multiError)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curl_multi_perform numRunningHandles= %d.\n", __FUNCTION__, numRunningHandles);
            if(0 == numRunningHandles)
            {
                // No longer any transfers in progress, therefore  can get status.
                CheckDownloadStatus(handle);
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Transfer started, numRunningHandles= %d.\n", __FUNCTION__, numRunningHandles);
            }
        }
        else if (CURLM_CALL_MULTI_PERFORM == multiError)
        {
            // Means just call again.
            multiError = curl_multi_perform(tHttpFileFetcher.curlMultiHandle, &numRunningHandles);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: ReCall curl_multi_perform returned error %u.\n", __FUNCTION__, (unsigned int)multiError);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: ReCall- curl_multi_perform numRunningHandles= %d.\n", __FUNCTION__, numRunningHandles);
        }
        else //if (CURLM_OK != multiError)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_multi_perform returned error %u.\n", __FUNCTION__, (unsigned int)multiError);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Blocking Xfer Mode \n", __FUNCTION__);
        CURLcode curlCode = curl_easy_perform(handle);                            /* Will block until transfer complete. */
        if (curlCode == CURLE_OK)
        {
            // For blocking download get the http status once downloaded.
            CheckDownloadStatus(handle);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_easy_perform returned error %u.\n", __FUNCTION__, (unsigned int)curlCode);
            multiError = CURLM_INTERNAL_ERROR ;            /* Just return an non OK error if the blocking tranfers returns an error. */
        }
    }
    return multiError;
}

CURLMcode TerminateTransfer(CURL* handle)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: curl Handle= %u  \n", __FUNCTION__, (unsigned int)handle);
    CURLMcode multiError = CURLM_BAD_HANDLE;
    // Do multi curl cleanup
    if (NULL != handle && NULL != tHttpFileFetcher.curlMultiHandle)
    {
    /* Remove easy handle from multi handle list. */
        multiError = curl_multi_remove_handle(tHttpFileFetcher.curlMultiHandle, handle);
    }
    // Now do easy cleanup
    curl_easy_cleanup(handle);
    HttpSessions_t::iterator i = tHttpFileFetcher.tHttpSessions.find(handle);
    if (i != tHttpFileFetcher.tHttpSessions.end()) 
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Removing curl easy handle= %u from tHttpSessions.\n", __FUNCTION__, (unsigned int)handle);
        tHttpFileFetcher.tHttpSessions.erase(i);                                    /* Remove handle from multi handle list. */
    }
    return multiError;
}

// For blocking transfer this should be called when curl_easy_perform returns.
// For non-blocking transfer this should be called when downlaod has completed.
CURLcode CheckDownloadStatus(CURL* handle)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: curlEasyHandle= %u  \n", __FUNCTION__, (unsigned int)handle);
    CURLcode retCode = CURLE_OK;
    CURLcode msgRetCode;
    CURLMsg* pMsg;
    CURLcode curlRet;
    int numMsgs = 0;
    int httpStatus = 0;

    bool downloading = IsDownloading();

    if (!tHttpFileFetcher.useBlockingTransfer)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Non-Blocking mode\n", __FUNCTION__);
        /* In non-blocking mode you must first determine if the Xfer is done and then querry for status. */
        /* Since the order of the pMsg as related to curleasy handle/url is not deterministic process all Msgs */
        /* each time CheckDownloadStatus is called. */
        retCode = CURLE_GOT_NOTHING;      /* curl is still busy, use following error to indicate busy. */
        do {
            pMsg = curl_multi_info_read(tHttpFileFetcher.curlMultiHandle, &numMsgs);
            if (pMsg)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curl_multi_info_read numMsgs= %d.\n", __FUNCTION__, numMsgs);
                if (pMsg->msg == CURLMSG_DONE)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curl_multi_info_read, CURLMsg=CURLMSG_DONE\n", __FUNCTION__);
                    /* When !IsDownloading the result should be CURLMSG_DONE. */
                    CURL* easyHandle = pMsg->easy_handle;
                    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s:msg for curl Handle= %u  \n", __FUNCTION__, (unsigned int)easyHandle);
                    msgRetCode = pMsg->data.result;
                    if (msgRetCode == CURLE_OK)
                    {
                        msgRetCode = curl_easy_getinfo(easyHandle, CURLINFO_RESPONSE_CODE, &httpStatus);
                        if (msgRetCode == CURLE_OK)
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: http Response=%d, curlEasyHandle= %u\n", __FUNCTION__, httpStatus, (unsigned int)easyHandle);
                            tHttpFileFetcher.tHttpStatusCodes[easyHandle] = httpStatus;
                            tHttpFileFetcher.networkUp = true;
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_easy_getinfo error= %d.\n", __FUNCTION__, msgRetCode);
                        }
                        sslDiagnostics(easyHandle);
                    }
                    else
                    {
                        // This means curl returned an error so force an HTTP error.
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_multi_info_read data error= %d.\n", __FUNCTION__, pMsg->data.result);
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Async curl, no http Response, force to 600, Bad Request. curlEasyHandle= %u\n", __FUNCTION__, (unsigned int)easyHandle);
                        tHttpFileFetcher.tHttpStatusCodes[easyHandle] = HTTP_RESPONSE_ON_CURL_ERROR;
                    }
                    // To enable true asynchronous operation compare the requested handle to the Curl response
                    if (handle == easyHandle)
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Transfer complete for curlEasyHandle= %u\n", __FUNCTION__, (unsigned int)easyHandle);
                        retCode = CURLE_OK;
                    }
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_multi_info_read msg error= %d.\n", __FUNCTION__, pMsg->msg);
                }
            }
            else
            {
                // If the http status was obtained under a different easy handle query we must identify
                // complete in this else case. A http status of non-0 is a valid response.
                if (0 != tHttpFileFetcher.tHttpStatusCodes[handle])
                {
                    retCode = CURLE_OK;
                }
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curl_multi_info_read, pMsg=NULL\n", __FUNCTION__);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Previously obtained HttpStatus = %d for curlEasyHandle= %u\n", __FUNCTION__, tHttpFileFetcher.tHttpStatusCodes[handle], (unsigned int)handle);
            }
        } while(pMsg);
        // Must handle the case where there are no active transfers but there are no msg for CheckDownloadStatus(handle)
        // because the status was previously processed by a call to CheckDownloadStatus for a different handle.
        if (!downloading)
        {
            retCode = CURLE_OK;
        }
    }
    else
    {
        //In blocking mode the transfer will be complete, failed or success.
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Blocking mode\n", __FUNCTION__);
        // Blocking mode
        curlRet = curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &httpStatus);
        if (CURLE_OK == curlRet)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curl http Response=%d, curlEasyHandle= %u\n", __FUNCTION__, httpStatus, (unsigned int)handle);
            tHttpFileFetcher.tHttpStatusCodes[handle] = httpStatus;
            tHttpFileFetcher.networkUp = true;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: no curl http Response, force to 600, Bad Request. curlEasyHandle= %u\n", __FUNCTION__, (unsigned int)handle);
            tHttpFileFetcher.tHttpStatusCodes[handle] = HTTP_RESPONSE_ON_CURL_ERROR;
        }
        sslDiagnostics(handle);
    }
    return retCode;
}

bool IsDownloading()
{
    // This function only checks to see if there are ANY active transfers.
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int numRunningHandles;
    bool isDownloading = false;
    if (!tHttpFileFetcher.useBlockingTransfer)
    {
        CURLMcode multiError = curl_multi_perform(tHttpFileFetcher.curlMultiHandle, &numRunningHandles);
        if (CURLM_OK == multiError)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curl_multi_perform numRunningHandles= %d.\n", __FUNCTION__, numRunningHandles);
            if (numRunningHandles > 0)
            {
                isDownloading = true;
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_multi_perform returned error %u.\n", __FUNCTION__, (unsigned int)multiError);
        }
    }
    return isDownloading;
}

bool UsingBlockingTransferMode()
{
    bool isBlockingMode = false;
    if (tHttpFileFetcher.useBlockingTransfer)
    {
        isBlockingMode = true;
    }
    return isBlockingMode;
}

static size_t writeCurlCallback(char *vpNewData, size_t size, size_t nmemb, void *vpStreamData)
{
    size_t realsize = size * nmemb;                        /* Total number of bytes received. */
    std::string* pDataFile = (static_cast<std::string*>(vpStreamData));
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: received %u bytes.\n", __FUNCTION__, realsize);
    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Data written to downloadfile at %p.\n", __FUNCTION__, vpStreamData);
    unsigned char* data = (unsigned char*)vpNewData;
    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "     %02X%02X  %02X%02X  %02X%02X  %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X \n", *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++, *data++);

    if (NULL != pDataFile)
    {
        pDataFile->append(vpNewData, realsize);
    }
    return realsize;
}


/* jes2feb16 */
int CurlDebugCallback(CURL* handle, curl_infotype type, char *data, size_t size, void *vpUserPtr)
{
    char* text;
    std::string curlHeaderData;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: curlEasyHandle= %u\n", __FUNCTION__, (unsigned int)handle);
    switch(type)
    {
        case CURLINFO_TEXT:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "CURLDBG Info: %s.\n", data);
            return 0;
            break;

        case CURLINFO_HEADER_OUT:
            text = "=> Send Header";
            curlHeaderData.append(data, size);                // Should be string formatted data.
            break;

        case CURLINFO_DATA_OUT:
            text = "=> Send Data";
          vlMpeosDumpBuffer(RDK_LOG_TRACE5, "LOG.RDK.POD", (void*)data, size);
            break;

        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL Data";
          vlMpeosDumpBuffer(RDK_LOG_TRACE5, "LOG.RDK.POD", (void*)data, size);
            break;

        case CURLINFO_HEADER_IN:
            text = "<= Recv Header";
            curlHeaderData.append(data, size);                // Should be string formatted data.
            break;

        case CURLINFO_DATA_IN:
            text = "<= Recv Data";
          vlMpeosDumpBuffer(RDK_LOG_TRACE5, "LOG.RDK.POD", (void*)data, size);
            break;

        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL Data";
          vlMpeosDumpBuffer(RDK_LOG_TRACE5, "LOG.RDK.POD", (void*)data, size);
            break;

        default:
            return 0;
            break;
    }
    /* include data* in log dump. */
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "CURLDBG: %s.\n", text);
    if(curlHeaderData.size())
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "CURLDBG data: %s.\n", curlHeaderData.data());
    }
    return 0;
}


// Base64 Encoding utilities

const char Base64Table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string Base64Encode(const uint8_t* in, uint32_t size)
{
    uint32_t blocks = (size + 2) / 3;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Size= %u, blocks= %u.\n", __FUNCTION__, size, blocks);
    if (blocks == 0 || in == NULL)
    {
        return "";
    }

    std::string out;
    for (uint32_t i = 0; i < blocks - 1; ++i)
    {
        out += Base64Table[((in[0] & 0xFC) >> 2)];
        out += Base64Table[((in[0] & 0x03) << 4) + ((in[1] & 0xF0) >> 4)];
        out += Base64Table[((in[1] & 0x0F) << 2) + ((in[2] & 0xC0) >> 6)];
        out += Base64Table[ (in[2] & 0x3F) ];
        in += 3;
    }
    out += Base64Table[((in[0] & 0xFC) >> 2)];
    if (size % 3 == 1)
    {
        out += Base64Table[((in[0] & 0x03) << 4)];
        out += "==";
    }
    else {
        out += Base64Table[((in[0] & 0x03) << 4) + ((in[1] & 0xF0) >> 4)];
        if (size % 3 == 2)
        {
            out += Base64Table[((in[1] & 0x0F) << 2)];
            out += "=";
        }
        else {
            out += Base64Table[((in[1] & 0x0F) << 2) + ((in[2] & 0xC0) >> 6)];
            out += Base64Table[ (in[2] & 0x3F) ];
        }
    }
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: base64 encoded buffer.\n", __FUNCTION__);
    vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", (void*)out.data(), (int)out.size());
    return out;
}

void sslDiagnostics(CURL* handle)
{
    CURLcode retCode;
    long result;
    int i;

    union
    {
        struct curl_slist*    to_info;
        struct curl_certinfo* to_certinfo;
    } ptr;
    ptr.to_info = NULL;
 
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    // 1. CURLINFO_CERTINFO required CURLOPT_CERTINFO being set true
    retCode = curl_easy_getinfo(handle, CURLINFO_CERTINFO, &ptr.to_info);
    if (CURLE_OK == retCode)
    {
        if (ptr.to_info)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: CURLINFO_CERTINFO num certs= %d\n", __FUNCTION__, ptr.to_certinfo->num_of_certs);
            for(i=0 ; i<ptr.to_certinfo->num_of_certs ; i++)
            {
                struct curl_slist* slist;
                for(slist=ptr.to_certinfo->certinfo[i]; slist; slist=slist->next)
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s\n", slist->data);
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: CURLINFO_CERTINFO- to data retrieved .\n", __FUNCTION__);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_easy_getinfo CURLINFO_CERTINFO ERROR- %d.\n", __FUNCTION__, retCode);
    }
    // 2. CURLINFO_SSL_VERIFY_RESULT
    retCode = curl_easy_getinfo(handle, CURLINFO_SSL_VERIFYRESULT, &result);
    if (CURLE_OK == retCode)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: CURLINFO_SSL_VERIFY_RESULT (VerifyPeer)=%d\n", __FUNCTION__, result);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: curl_easy_getinfo CURLINFO_SSL_VERIFYRESULT ERROR- %d.\n", __FUNCTION__, retCode);
    }
}

#endif        /* USE_IPDIRECT_UNICAST */

/* End httpFileFetcher.cpp */

