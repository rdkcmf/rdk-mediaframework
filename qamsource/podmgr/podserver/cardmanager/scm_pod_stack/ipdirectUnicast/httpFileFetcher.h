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

#ifndef _HTTPFILEFETCHER_H
#define _HTTPFILEFETCHER_H

#include <string>
#include <curl/curl.h>
#include <map>

/** 
 *  @file HttpFileFetcher.h
 *  @brief Interface header file for an HTTP File fetcher using cURL.
 *
 *  This file contains the public definitions for the HttpFileFetcher object implementation.
 *  
 */

/* The http file fetcher data types are required whether USE_IPDIRECT_UNICAST is defined or not. */

/*
 *	This structure provides a simple HTTP file fetcher interface for curl. It utilizes cURL
 *	multi-handle to read files in a non-blocking mode.
 */

#define HTTP_BLOCKING_TRANSFER_MODE			true
#define HTTP_NONBLOCKING_TRANSFER_MODE	false
#define HTTP_GET_TIMEOUT_SEC						240             // Overall xfer timeout
#define FILE_UPLOAD_TIMEOUT_SEC					240             // Overall xfer timeout
#define HTTP_GET_CONNECT_TIMEOUT_SEC		180             // Server connection timeout
#define FILE_UPLOAD_CONNECT_TIMEOUT_SEC	180             // Server connection timeout

#define HTTP_FILE_TRANSFER_MODE					HTTP_NONBLOCKING_TRANSFER_MODE
// This is used to return is curl cannot even initiate a transaction to get a real HTTP response.
#define HTTP_RESPONSE_ON_CURL_ERROR			600   // Bad Request
                                              // 600 was selected by the IPDU dev team to signal the
                                              // network is not up, e.g. no IP address.
#define HTTP_RESPONSE_ON_NO_STT			    601   // This value is beyond the defined response codes.
                                              // 601 was selected by the IPDU dev team to signal the
                                              // STB does not have System Time.

/* This associative list maps curl easy handle to the "file" or string that the file is downloaded into. */
/* typedef pair<const Key, T> */
typedef std::map<CURL*, std::string> HttpSessions_t;		/* track curl easy handles and associated download file. */
typedef std::map<CURL*, std::string> HttpSessionUrls_t;	/* track curl easy handles and associated URL. */
typedef std::map<CURL*, int> HttpStatusCodes_t;					/* track http responses by curl easy handles. */
typedef std::map<std::string, CURL*> CurlSessions_t;		/* track curl handles http responses by url. */

typedef struct _HttpFileFetcher
{
    bool useBlockingTransfer;									/* HttpFileFetcher can optionally operate in a blocking mode. */
																						/* This eliminates the need for a perodic polling of the transfer status. */
    int uploadTimeout;                        /* HTTP Post, units in seconds */
    int downloadTimeout;                      /* HTTP Get, units in seconds */
    int uploadConnectionTimeout;              /* HTTP Post server connect timeout, units in seconds */
    int downloadConnectionTimeout;            /* HTTP Get server connect timeout, units in seconds */
    CURLM* curlMultiHandle;
    HttpSessions_t tHttpSessions;
    HttpSessionUrls_t tHttpSessionUrls;
    HttpStatusCodes_t tHttpStatusCodes;
    CurlSessions_t tCurlSessions;
    bool networkUp;                           /* Flag indicating network accessability. */
} HttpFileFetcher_t;

#ifdef USE_IPDIRECT_UNICAST

/*
 *	These C style functions use the HttpFileFetcher_t structure to HTTP download files using cURL.
 */
/* Public style interface. */
CURLcode InitHttpFileFetcher();							/* Inits the curl library and HttpFileFetcher_t object. */
CURLcode DownloadFile(std::string url);			/* Initiates the download of a new file. */
CURLcode DownloadStatus(CURL* handle);			/* Check Download status. */
CURLcode CancelDownload(CURL* handle);			/* Cancel download and release file buffer. */
std::string GetFile(CURL* handle);					/* Returns the contents of the downloaded file. */
CURLcode UploadFile(std::string url, std::string outputFile, int* httpStatus);				/* Initiates a file upload. */

/* Public utility functions */
void SetHttpGetTimeout(int timeoutInSeconds);
void SetHttpPostTimeout(int timeoutInSeconds);
void SetHttpGetConnectionTimeout(int timeoutInSeconds);
void SetHttpPostConnectionTimeout(int timeoutInSeconds);
/* Returns TRUE when network up, FALSE if network is not accessible. */
/* This is workaround for until a network interface method is found. This function uses an IP address obtained from */
/* a IPDU get or Post request and waits for curl to return no error to determine the host can access the network. */
/* If curl returns an error for a get/post it is assumed the network is not reachable. */
bool IsNetworkUp();
/* Private style interface. */
CURLMcode InitiateTransfers(CURL* handle);	/* This method will initiate reads or writes for all easy. */
																						/* handles added to curlMultiHandle. */
CURLMcode TerminateTransfer(CURL* handle);	/* This method will stop and cleanup the specified transfer. */
CURLcode CheckDownloadStatus(CURL* handle);	/* This should be called by a periodic event or timer to check on donwload status. */
bool IsDownloading();												/* Indicates whether a transfer is still in progress. TRUE=downloading, FALSE=transferring. */
bool UsingBlockingTransferMode();						/* Indicates HTTP Xfer mode, blocking or non-blocking. TRUE=Using blocking download, FALSE=Using Non-Blocking download. */
/* writeCurlCallback is the callback used by the HttpFilefetcher to recieve data from curl for each easy handle session/stream. */
/*	vpNewData: New data available from the source URL.																																				*/
/*	size: Size of one Data Item in bytes																																											*/
/*	nmemb: Number of Data Items																																																*/
/*	vpStreamData: Session/Easy Handle specific buffer into which the file is received.																				*/
/*	return: The number of bytes successully hanlded/written.																																	*/
static size_t writeCurlCallback(char *vpNewData, size_t size, size_t nmemb, void *vpStreamData);

/* Provides curl diagnostics with the CURLOPT_DEBUGFUNCTION option. */
int CurlDebugCallback(CURL* handle, curl_infotype type, char *data, size_t size, void *vpUserPtr);


//Utilities
std::string Base64Encode(const unsigned char* in, uint32_t size);
void sslDiagnostics(CURL* handle);

#endif 		/* USE_IPDIRECT_UNICAST */

#endif		/* _HTTPFILEFETCHER_H */
