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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "qamsrc_test_server.h"
#include "mongoose.h"

static struct mg_context *mgServerCtx= NULL;
static  qamtest_data_get_callback_t dataCallBack = NULL;
static  qamtest_data_free_callback_t freeDataCallBack = NULL;
static  void* guserdata = NULL;
static int genablestream = 0;

#define MAX_REQUEST_SIZE_ 16384

int writeFormattedData(mg_connection *mconn, const char *format, ...)
{
	int result = -1;
	va_list ap;
	char buf[MAX_REQUEST_SIZE_];
	int  len;
	va_start(ap, format);
	if (NULL != mconn)
	{
		len = vsnprintf(buf, sizeof(buf), format, ap);
		result = mg_write(mconn,buf,len);
	}
	va_end(ap);
	return result;
}

static void httpRequestCallback( struct mg_connection *conn,
										const struct mg_request_info *request_info,
										void *user_data)
{
	void* data;
	void * freeData;
	int len;
	int ret;
	int status_code = 200;
	char date[64], range[64];
	const char	*fmt = "%a, %d %b %Y %H:%M:%S GMT", *msg = "OK";
	const char	*mime_type = "video/mp2t";
	time_t curtime;
	printf("\n\n=====================%s:%d\n",__FUNCTION__, __LINE__);
	time ( &curtime );
	/* Prepare Etag, Date, Last-Modified headers */
	(void) strftime(date, sizeof(date), fmt, localtime(&curtime));
	range[0] = '\0';
	writeFormattedData( conn, 
					  "HTTP/1.1 %d %s\r\n"
					 "Date: %s\r\n"
					 "Last-Modified: %s\r\n"
					 /*"Etag: \"%s\"\r\n"*/
					 "Content-Type: %s\r\n"
					/* "Content-Length: %llu\r\n"*/
					 "Connection: %s\r\n"
					 "Accept-Ranges: bytes\r\n"
					 "%s\r\n",
					 status_code, msg, date, date,  mime_type, 
					 0? "keep-alive" : "close", range);
	while (genablestream )
	{
		ret = dataCallBack(&data, &len, &freeData, guserdata);
		if (0!= ret)
		{
			printf("%s:%d - dataCallBack returned %d- Exiting\n",
				__FUNCTION__, __LINE__, ret);
			break;
		}
		mg_write(conn, data, len);
		if (freeDataCallBack)
		{
			freeDataCallBack( freeData, guserdata);
		}
	}
}

static void httpServerShow404(struct mg_connection * conn,
		const struct mg_request_info *info, void *user_data)
{
	printf("\n\n=====================%s:%d\n",__FUNCTION__, __LINE__);
	const char* resp404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
						"<html><body><h2>NOT FOUND !!!!</h2></body></html>";
	mg_write(conn,resp404,strlen(resp404));
}

int startServer( qamtest_data_get_callback_t cb, qamtest_data_free_callback_t fcb, void* userdata )
{
	printf("\n\n=====================%s:%d\n",__FUNCTION__, __LINE__);
	mgServerCtx = mg_start();
	if(mgServerCtx == NULL) 
	{
		return -1;
	}
	genablestream = 1;
	mg_set_option(mgServerCtx, "ports", "8080");
	mg_bind_to_uri(mgServerCtx, "*", &httpRequestCallback, NULL);
	mg_bind_to_error_code(mgServerCtx, 404, httpServerShow404, NULL);
	dataCallBack = cb;
	freeDataCallBack = fcb;
	guserdata = userdata;
	return 0;
}

void stopServer()
{
	printf("\n\n=====================%s:%d\n",__FUNCTION__, __LINE__);
	genablestream = 0;
	dataCallBack = NULL;
	if (mgServerCtx)
	{
		mg_stop(mgServerCtx);
	}
	mgServerCtx = NULL;
}

