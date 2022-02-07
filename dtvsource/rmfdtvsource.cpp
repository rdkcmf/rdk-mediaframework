/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 *
 * File originally created by The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * This file is part of a DTVKit Software Component and is
 * licensed by RDK Management, LLC under the terms of the RDK license.
 * The RDK License agreement constitutes express written consent by DTVKit.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

/**
 * @brief   RDK media framework DTV source
 * @file    rmfdtvsource.cpp
 * @date    November 2019
 */

#include <string.h>
#include <list>

#include "rmfdtvsource.h"

#include "rmfprivate.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_mem.h"

#include "rdk_debug.h"

extern "C"
{
   #include <libwebsockets.h>
};

#define DTVSOURCE_IMPL              static_cast<RMFDTVSourcePrivate*>(getPrivateSourceImpl())

#define JSON_REQUEST_METHOD         "{\"jsonrpc\":\"2.0\",\"id\":%u,\"method\":\"%s\"}"
#define JSON_REQUEST_METHOD_PARAMS  "{\"jsonrpc\":\"2.0\",\"id\":%u,\"method\":\"%s\",\"params\":%s}"
#define JSON_RESULT_STRING          "\"result\":"
#define JSON_TUNER_STRING           "\"tuner\":"
#define JSON_DEMUX_STRING           "\"demux\":"
#define JSON_PMTPID_STRING          "\"pmtpid\":"
#define JSON_DVBURI_STRING          "\"dvburi\":"

typedef struct s_dtvsource_instance
{
   char *uri;
   RMFDTVSource *dtvsrc;
   struct s_dtvsource_instance *next;
} S_DTVSOURCE_INSTANCE;

static bool g_initialised = false;
static rmf_osal_Mutex g_list_mutex;
static S_DTVSOURCE_INSTANCE *g_dtvsource_list = NULL;


static int DtvkitProtocolCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user,
   void *in, size_t len);

static struct lws_protocols ws_protocols[] =
{
   {
      "notification",               /* Protocol name */
      DtvkitProtocolCallback,       /* Protocol callback */
      0,                            /* Per session data size */
      0,                            /* Receive buffer size */
      0,                            /* ID - not used, but could be set to the dtvkit protocol version */
      NULL                          /* User context data */
   },
   {NULL, NULL, 0, 0, 0, NULL}      /* Terminator */

};


/**
 * @class   RPCRequest
 * @brief   Class that holds the data related to a request sent to the DTV nano service
 */
class RPCRequest
{
   public:
      RPCRequest(const char *method, const char *params, const unsigned int id);
      ~RPCRequest();

      bool Sent(void) const {return(m_request_sent);}
      unsigned int Id(void) const {return(m_id);}

      void Send(struct lws *wsi);
      void ResponseReceived(const char *response);
      bool WaitForResponse(uint32_t timeout);

      char* GetResponse(void) const;
      bool GetResponseValue(const char *key, unsigned int &value);
      bool GetResponseValue(const char *key, const char* &value);

   private:
      char *m_request_string;
      size_t m_request_size;
      unsigned int m_id;
      bool m_request_sent;

      char *m_response_string;
      rmf_osal_Cond m_response_received;
};

RPCRequest::RPCRequest(const char *method, const char *params, const unsigned int id) :
   m_id(id), m_request_size(0), m_request_sent(false), m_response_string(NULL)
{
   if (params != NULL)
   {
      unsigned int length = LWS_PRE + strlen(JSON_REQUEST_METHOD_PARAMS) + strlen(method) + strlen(params) + 20;

      m_request_string = new char[length];
      if (m_request_string != NULL)
      {
         m_request_size = sprintf(&m_request_string[LWS_PRE], JSON_REQUEST_METHOD_PARAMS, id, method, params);

         rmf_Error result = rmf_osal_condNew(false, false, &m_response_received);
         if (result != RMF_SUCCESS)
         {
            RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.DTV", "[%s:%d]: Failed to create cond, error %d\n", __FUNCTION__,
               __LINE__, result);
         }
      }
   }
   else
   {
      unsigned int length = LWS_PRE + strlen(JSON_REQUEST_METHOD) + strlen(method) + 20;

      m_request_string = new char[length];
      if (m_request_string != NULL)
      {
         m_request_size = sprintf(&m_request_string[LWS_PRE], JSON_REQUEST_METHOD, id, method);

         rmf_Error result = rmf_osal_condNew(false, false, &m_response_received);
         if (result != RMF_SUCCESS)
         {
            RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.DTV", "[%s:%d]: Failed to create cond, error %d\n", __FUNCTION__,
               __LINE__, result);
         }
      }
   }
}

RPCRequest::~RPCRequest()
{
   if (m_request_string != NULL)
   {
      delete [] m_request_string;
   }

   if (m_response_string != NULL)
   {
      delete [] m_response_string;
   }

   rmf_osal_condDelete(m_response_received);
}

void RPCRequest::Send(struct lws *wsi)
{
   if (!m_request_sent)
   {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: Sending request %u, \"%s\"\n",
         __FUNCTION__, __LINE__, m_id, &m_request_string[LWS_PRE]);

      if (lws_write(wsi, (unsigned char *)&m_request_string[LWS_PRE], m_request_size, LWS_WRITE_TEXT) != 0)
      {
         m_request_sent = true;
      }
      else
      {
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: Failed to send request %u, \"%s\"\n",
            __FUNCTION__, __LINE__, m_id, &m_request_string[LWS_PRE]);
      }
   }
}

void RPCRequest::ResponseReceived(const char *response)
{
   if (m_response_string == NULL)
   {
      m_response_string = new char[strlen(response) + 1];
      if (m_response_string != NULL)
      {
         strcpy(m_response_string, response);
         rmf_osal_condSet(m_response_received);
      }
   }
}

bool RPCRequest::WaitForResponse(uint32_t timeout)
{
   rmf_Error result;
   bool retval = false;

   if (timeout == (uint32_t)-1)
   {
      result = rmf_osal_condGet(m_response_received);
   }
   else
   {
      result = rmf_osal_condWaitFor(m_response_received, timeout);
   }

   if (result == RMF_SUCCESS)
   {
      retval = true;
   }

   return(retval);
}

char* RPCRequest::GetResponse(void) const
{
   return(m_response_string);
}

bool RPCRequest::GetResponseValue(const char *key, unsigned int &value)
{
   bool result = false;

   const char *value_string = strstr(m_response_string, key);
   if (value_string != NULL)
   {
      value_string += strlen(key);

      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s]: %s = \"%s\"\n", __FUNCTION__, key, value_string);
      value = atoi(value_string);
      result = true;
   }

   return(result);
}

bool RPCRequest::GetResponseValue(const char *key, const char* &value)
{
   bool result = false;

   const char *value_string = strstr(m_response_string, key);
   if (value_string != NULL)
   {
      value_string += strlen(key);

      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s]: %s = \"%s\"\n", __FUNCTION__, key, value_string);
      value = value_string;
      result = true;
   }

   return(result);
}

/**
 * @class   RMFDTVSourcePrivate
 * @brief   Class extending RMFMediaSourcePrivate to implement RMFDTVSourcePrivate variables and functions
 */
class RMFDTVSourcePrivate : public RMFMediaSourcePrivate
{
   public:
      RMFDTVSourcePrivate(RMFDTVSource *parent);
      ~RMFDTVSourcePrivate();

      struct lws* CreateWebsocketInterface(void);
      void* createElement();

      RMFResult term();
      RMFResult open(const char* uri, char* mimetype);
      RMFResult close();
      RMFResult play();
      RMFResult play(float& speed, double& time);
      RMFResult stop();

      void StatusMonitor(void);

      rmf_osal_Mutex m_mutex;

      rmf_osal_ThreadId m_monitor_id;
      std::list<RPCRequest*> m_request_list;

      struct lws_context *m_context;
      struct lws *m_wsi;
      rmf_osal_Cond m_connected;

      bool m_run_wsmonitor;
      rmf_osal_Cond m_wsmonitor_stopped;

      bool m_run_statusmonitor;
      rmf_osal_ThreadId m_status_id;
      rmf_osal_Cond m_statusmonitor_stopped;

   private:
      unsigned int m_id;
      GstElement *m_source;
      int m_play_handle;

      RMFResult SendRequest(const char *method, const char *params, unsigned int &id);
      bool WaitForResponse(const unsigned int id, const bool delete_from_list, uint32_t timeout,
         RPCRequest* &request);

      static void WSMonitor(void *param);
      static void StatusTask(void *param);
};


RMFDTVSourcePrivate::RMFDTVSourcePrivate(RMFDTVSource *parent) : RMFMediaSourcePrivate(parent),
   m_wsi(NULL), m_id(0), m_source(NULL), m_play_handle(-1)
{
   /* Create the websocket context */
   struct lws_context_creation_info info;

   //lws_set_log_level((LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_PARSER | LLL_HEADER), NULL);

   memset(&info, 0, sizeof(struct lws_context_creation_info));

   info.port = CONTEXT_PORT_NO_LISTEN;
   info.protocols = ws_protocols;
   info.gid = -1;
   info.uid = -1;

   if ((m_context = lws_create_context(&info)) != NULL)
   {
      RDK_LOG(RDK_LOG_NOTICE, "LOG.RDK.DTV", "[%s:%d]: Created context\n", __FUNCTION__, __LINE__);

      rmf_osal_condNew(false, false, &m_connected);

      rmf_osal_mutexNew(&m_mutex);

      m_run_wsmonitor = true;
      rmf_osal_condNew(false, false, &m_wsmonitor_stopped);

      m_wsi = CreateWebsocketInterface();
      if (m_wsi != NULL)
      {
         /* Start a background task to service the websocket so it can send and receive data */
         rmf_Error result = rmf_osal_threadCreate(WSMonitor, (void *)this, RMF_OSAL_THREAD_PRIOR_DFLT,
            RMF_OSAL_THREAD_STACK_SIZE, &m_monitor_id, "WSMonitor");
         if (result == RMF_SUCCESS)
         {
            /* Wait for the connection to complete */
            result = rmf_osal_condWaitFor(m_connected, 3000);
            if (result == RMF_SUCCESS)
            {
               rmf_osal_mutexAcquire(m_mutex);

               if (m_wsi != NULL)
               {
                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Connected to DTV nano service\n",
                     __FUNCTION__, __LINE__);
               }
               else
               {
                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to connect to DTV nano service\n",
                     __FUNCTION__, __LINE__);
               }

               rmf_osal_mutexRelease(m_mutex);
            }
            else
            {
               RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Timed out waiting for connection to DTV nano service\n",
                  __FUNCTION__, __LINE__);
            }
         }
         else
         {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to create WSMonitor task, error %d\n",
               __FUNCTION__, __LINE__, result);
         }
      }
   }
   else
   {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to create websocket context\n",
         __FUNCTION__, __LINE__);
   }
}

RMFDTVSourcePrivate::~RMFDTVSourcePrivate()
{
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DTV", "[%s:%d]\n", __FUNCTION__, __LINE__);

   /* Stop the websocket monitor thread */
   m_run_wsmonitor = false;
   rmf_osal_condWaitFor(m_wsmonitor_stopped, 2000);

   rmf_osal_threadDestroy(m_monitor_id);
   rmf_osal_condDelete(m_wsmonitor_stopped);

   if (m_context != NULL)
   {
      lws_context_destroy(m_context);
      m_context = NULL;
   }

   if (m_source != NULL)
   {
      gst_object_unref(m_source);
      m_source = NULL;
   }

   rmf_osal_condDelete(m_connected);
}

struct lws* RMFDTVSourcePrivate::CreateWebsocketInterface(void)
{
   struct lws *wsi = NULL;

   if (m_context != NULL)
   {
      /* Need to connect to the Thunder nano service */
      struct lws_client_connect_info ccinfo;

      memset(&ccinfo, 0, sizeof(struct lws_client_connect_info));

      ccinfo.context = m_context;
      ccinfo.address = "127.0.0.1";
      ccinfo.port = 9998;
      ccinfo.path = "/jsonrpc/DTV";
      ccinfo.host = ccinfo.address; //lws_canonical_hostname(m_context);
      ccinfo.origin = ccinfo.address; //"origin";
      ccinfo.protocol = ws_protocols[0].name;
      ccinfo.userdata = (void *)this;

      if ((wsi = lws_client_connect_via_info(&ccinfo)) != NULL)
      {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Created websocket interface\n",
            __FUNCTION__, __LINE__);
      }
      else
      {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to create websocket interface\n",
            __FUNCTION__, __LINE__);
      }
   }

   return(wsi);
}

void* RMFDTVSourcePrivate::createElement()
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]: m_source=%p\n" , __FUNCTION__, __LINE__, m_source);

   if (m_source == NULL)
   {
      m_source = gst_element_factory_make("dtvsource", NULL);

      if (m_source != NULL)
      {
         /* Increment the reference count of the source to ensure it isn't unknowingly freed */
         gst_object_ref(m_source);

         char *name = gst_element_get_name(m_source);

         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: created DTV element %s, %p\n",
            __FUNCTION__, __LINE__, name, m_source);

         g_free(name);
      }
      else
      {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: failed to created DTV element\n",
            __FUNCTION__, __LINE__);
      }
   }

   return m_source;
}

RMFResult RMFDTVSourcePrivate::term()
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n", __FUNCTION__, __LINE__);

   RMFResult result = RMFMediaSourcePrivate::term();

   if (m_source != NULL)
   {
      gst_object_unref(m_source);
      m_source = NULL;
   }

   return(result);
}

/**
 * @brief
 * @param   url URL format "dtv://onet.trans.serv", where the onet, trans and serv are the DVB triplet
 *              of the service to be viewed, in decimal
 *              URL format "dtv://" will view the current live service
 *              URL format "dtv://lcn, where lcn is the channel number of the service to be viewed, in decimal
                (this format is not currently supported)
 * @param   mimetype not used
 * @return  RMF_RESULT_SUCCESS
 */
RMFResult RMFDTVSourcePrivate::open(const char* url, char* mimetype)
{
   RMFResult result;
   unsigned int onet_id, tran_id, serv_id;
   unsigned int lcn;
   char params_string[32];
   unsigned int id;
   RPCRequest *request;

   if (m_source != NULL)
   {
      result = RMF_RESULT_INVALID_ARGUMENT;

      if ((url != NULL) && (*url != '\0'))
      {
         RDK_LOG(RDK_LOG_NOTICE, "LOG.RDK.DTV", "[%s:%d]: URL=\"%s\"\n", __FUNCTION__, __LINE__, url);

         request = NULL;

         if (sscanf(url, "dtv://%u.%u.%u", &onet_id, &tran_id, &serv_id) == 3)
         {
            /* Start tuning to service defined by the given DVB triplet */
            snprintf(params_string, sizeof(params_string), "{\"dvburi\":\"%u.%u.%u\"}", onet_id, tran_id, serv_id);

            result = SendRequest("startPlaying", params_string, id);
            if (result == RMF_RESULT_SUCCESS)
            {
               if (!WaitForResponse(id, false, 5000, request) || (request == NULL))
               {
                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to get response for request %u\n",
                     __FUNCTION__, __LINE__, id);
                  result = RMF_RESULT_INTERNAL_ERROR;
               }
            }
         }
         else if (sscanf(url, "dtv://%u", &lcn) == 1)
         {
            /* Start tuning to service defined by the given LCN */
            snprintf(params_string, sizeof(params_string), "{\"lcn\":%u}", lcn);

            result = SendRequest("startPlaying", params_string, id);
            if (result == RMF_RESULT_SUCCESS)
            {
               if (!WaitForResponse(id, false, 5000, request) || (request == NULL))
               {
                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to get response for request %u\n",
                     __FUNCTION__, __LINE__, id);
                  result = RMF_RESULT_INTERNAL_ERROR;
               }
            }
         }
         else
         {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: invalid URL \"%s\"\n",
               __FUNCTION__, __LINE__, url);
         }

         if ((result == RMF_RESULT_SUCCESS) && (request != NULL))
         {
            m_play_handle = atoi(request->GetResponse());;

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: play handle=%d\n", __FUNCTION__, __LINE__, m_play_handle);

            rmf_osal_mutexAcquire(m_mutex);
            m_request_list.remove(request);
            rmf_osal_mutexRelease(m_mutex);

            if (m_play_handle >= 0)
            {
               /* A valid play handle means the tuner and demux will also be valid,
                * so request the status to get these values */
               snprintf(params_string, sizeof(params_string), "status@%u", m_play_handle);

               result = SendRequest(params_string, NULL, id);
               if (result == RMF_RESULT_SUCCESS)
               {
                  request = NULL;

                  if (WaitForResponse(id, false, 3000, request) && (request != NULL))
                  {
                     unsigned int tuner;
                     unsigned int demux;

                     if (request->GetResponseValue(JSON_TUNER_STRING, tuner))
                     {
                        g_object_set(m_source, "tuner", tuner, NULL);
                     }
                     else
                     {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: No %s value in status\n",
                           __FUNCTION__, __LINE__, JSON_TUNER_STRING);
                     }

                     if (request->GetResponseValue(JSON_DEMUX_STRING, demux))
                     {
                        g_object_set(m_source, "demux", demux, NULL);
                     }
                     else
                     {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: No %s value in status\n",
                           __FUNCTION__, __LINE__, JSON_DEMUX_STRING);
                     }

                     /* Start a background task to wait to get the required info so playing can start */
                     m_run_statusmonitor = true;
                     rmf_osal_condNew(false, false, &m_statusmonitor_stopped);

                     result = rmf_osal_threadCreate(StatusTask, (void *)this, RMF_OSAL_THREAD_PRIOR_DFLT,
                        RMF_OSAL_THREAD_STACK_SIZE, &m_status_id, "StatusTask");
                     if (result != RMF_SUCCESS)
                     {
                        rmf_osal_condDelete(m_statusmonitor_stopped);

                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to create the status monitor task, error %d\n",
                           __FUNCTION__, __LINE__, result);
                     }
                  }
                  else
                  {
                     RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to get response for request %u\n",
                        __FUNCTION__, __LINE__, id);
                  }
               }
            }
            else
            {
               RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to get a play handle\n", __FUNCTION__, __LINE__);
               result = RMF_RESULT_INTERNAL_ERROR;
            }
         }
      }
      else
      {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: empty URL\n", __FUNCTION__, __LINE__);
      }
   }
   else
   {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: no source element\n", __FUNCTION__, __LINE__);
      result = RMF_RESULT_INTERNAL_ERROR;
   }

   return(result);
}

RMFResult RMFDTVSourcePrivate::close()
{
   char params_string[32];
   RMFResult result;
   unsigned int id;
   RPCRequest *request;

   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n", __FUNCTION__, __LINE__);

   result = RMFMediaSourcePrivate::close();

   if (m_play_handle >= 0)
   {
      snprintf(params_string, sizeof(params_string), "%u", m_play_handle);

      result = SendRequest("stopPlaying", params_string, id);
      if (result == RMF_RESULT_SUCCESS)
      {
         if (!WaitForResponse(id, true, 3000, request))
         {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to get response for request %u\n",
               __FUNCTION__, __LINE__, id);
         }
      }

      m_play_handle = -1;

      if (m_run_statusmonitor)
      {
         /* The status monitor is still running, so request it to stop */
         m_run_statusmonitor = false;
         rmf_osal_condWaitFor(m_statusmonitor_stopped, 4000);
      }

      rmf_osal_threadDestroy(m_status_id);
      rmf_osal_condDelete(m_statusmonitor_stopped);
   }

   /* Clear all requests from the list */
   rmf_osal_mutexAcquire(m_mutex);
   m_request_list.clear();
   rmf_osal_mutexRelease(m_mutex);

   return(result);
}

RMFResult RMFDTVSourcePrivate::play()
{
   RMFResult result = RMFMediaSourcePrivate::play();

   return(result);
}

RMFResult RMFDTVSourcePrivate::play(float& speed, double& time)
{
   RMFResult result;

   if (m_source != NULL)
   {
      /* As this is a live source the speed and position are ignored and
       * playing starts from the current time */
      result = RMFMediaSourcePrivate::play();
   }
   else
   {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: no source element", __FUNCTION__, __LINE__);
      result = RMF_RESULT_INTERNAL_ERROR;
   }

   return(result);
}

RMFResult RMFDTVSourcePrivate::stop()
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n", __FUNCTION__, __LINE__);

   /* Clear all requests from the list */
   rmf_osal_mutexAcquire(m_mutex);
   m_request_list.clear();
   rmf_osal_mutexRelease(m_mutex);

   RMFResult result = RMFMediaSourcePrivate::stop();

   return(result);
}

RMFResult RMFDTVSourcePrivate::SendRequest(const char *method, const char *params, unsigned int &id)
{
   RMFResult result = RMF_RESULT_INTERNAL_ERROR;

   rmf_osal_mutexAcquire(m_mutex);

   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: %s, m_wsi=%p\n", __FUNCTION__, __LINE__, method, m_wsi);

   if (m_wsi != NULL)
   {
      m_id++;
      id = m_id;

      RPCRequest *request = new RPCRequest(method, params, m_id);
      if (request != NULL)
      {
         /* Add the request to the list */
         m_request_list.push_back(request);

         /* Send the request when ready and wait for the response */
         lws_callback_on_writable(m_wsi);

         result = RMF_RESULT_SUCCESS;
      }
   }

   rmf_osal_mutexRelease(m_mutex);

   return(result);
}

bool RMFDTVSourcePrivate::WaitForResponse(const unsigned int id, const bool delete_from_list,
   uint32_t timeout, RPCRequest* &request)
{
   bool retval = false;

   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: id %u, delete %u\n", __FUNCTION__, __LINE__,
      id, delete_from_list);

   /* Find the request with the given id */
   rmf_osal_mutexAcquire(m_mutex);

   std::list<RPCRequest*>::iterator req;

   for (req = m_request_list.begin(); req != m_request_list.end(); req++)
   {
      if ((*req)->Id() == id)
      {
         /* Found the request so now wait for a response */
         rmf_osal_mutexRelease(m_mutex);

         if ((*req)->WaitForResponse(timeout))
         {
            if (delete_from_list)
            {
               RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: Got response, deleting...\n", __FUNCTION__, __LINE__);

               rmf_osal_mutexAcquire(m_mutex);
               m_request_list.remove(*req);
               rmf_osal_mutexRelease(m_mutex);
            }
            else
            {
               RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: Got response for %u\n",
                  __FUNCTION__, __LINE__, (*req)->Id());
               request = *req;
            }

            retval = true;
         }

         rmf_osal_mutexAcquire(m_mutex);
         break;
      }
   }

   rmf_osal_mutexRelease(m_mutex);

   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]: retval %u\n", __FUNCTION__, __LINE__, retval);

   return(retval);
}

void RMFDTVSourcePrivate::WSMonitor(void *param)
{
   RMFDTVSourcePrivate *priv = (RMFDTVSourcePrivate *)param;

   while(priv->m_run_wsmonitor)
   {
      lws_service(priv->m_context, 500);
   }

   rmf_osal_condSet(priv->m_wsmonitor_stopped);
}

void RMFDTVSourcePrivate::StatusTask(void *param)
{
   RMFDTVSourcePrivate *priv = (RMFDTVSourcePrivate *)param;

   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: Starting status monitor\n", __FUNCTION__, __LINE__);

   priv->StatusMonitor();

   if (!priv->m_run_statusmonitor)
   {
      /* Only need to set the condition if stop has been requested */
      rmf_osal_condSet(priv->m_statusmonitor_stopped);
   }

   priv->m_run_statusmonitor = false;

   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: Status monitor stopped\n", __FUNCTION__, __LINE__);
}

void RMFDTVSourcePrivate::StatusMonitor(void)
{
   unsigned int pmtpid;
   char request_string[16];
   unsigned int id;
   RMFResult result;
   RPCRequest *request;
   unsigned int serv_id;

   /* Keep requesting the play status until the PMT PID is returned, or this task is told to stop */
   pmtpid = 0;

   snprintf(request_string, sizeof(request_string), "status@%u", m_play_handle);

   while ((pmtpid == 0) && m_run_statusmonitor)
   {
      result = SendRequest(request_string, NULL, id);
      if (result == RMF_RESULT_SUCCESS)
      {
         request = NULL;

         if (WaitForResponse(id, false, 3000, request) && (request != NULL))
         {
            const char *uri;

            /* If the PMT PID isn't available yet then the other values aren't read */
            if (request->GetResponseValue(JSON_PMTPID_STRING, pmtpid) && (pmtpid != 0) && (pmtpid < 0x1fff))
            {
               if (request->GetResponseValue(JSON_DVBURI_STRING, uri))
               {
                  if (sscanf(uri, "\"%*u.%*u.%u\"", &serv_id) == 1)
                  {
                     g_object_set(m_source, "serv-id", serv_id, NULL);
                     g_object_set(m_source, "pmt-pid", pmtpid, NULL);
                  }
                  else
                  {
                     RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to extract the service ID from \"%s\"\n",
                        __FUNCTION__, __LINE__, uri);
                  }
               }
               else
               {
                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: No %s value in status\n",
                     __FUNCTION__, __LINE__, JSON_DVBURI_STRING);
               }
            }
            else
            {
               RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: No %s value in status\n",
                  __FUNCTION__, __LINE__, JSON_PMTPID_STRING);
               pmtpid = 0;
            }

            rmf_osal_mutexAcquire(m_mutex);
            m_request_list.remove(request);
            rmf_osal_mutexRelease(m_mutex);
         }
         else
         {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DTV", "[%s:%d]: Failed to get response for request %u\n",
               __FUNCTION__, __LINE__, id);
         }
      }
   }
}

/**
 * @class   RMFDTVSource
 * @brief   Class extending RMFMediaSourceBase to implement RMFDTVSource variables and functions
 */

RMFDTVSource* RMFDTVSource::getDTVSourceInstance(const char *uri)
{
   RMFDTVSource *dtvsrc;
   S_DTVSOURCE_INSTANCE *ptr;

   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s]: uri=%s\n", __FUNCTION__, uri);

   if (!g_initialised)
   {
      rmf_osal_mutexNew(&g_list_mutex);
      g_initialised = true;
   }

   rmf_osal_mutexAcquire(g_list_mutex);

   /* See if there's a DTVSource instance that's already tuned to the given uri */
   for (dtvsrc = NULL, ptr = g_dtvsource_list; (ptr != NULL) && (dtvsrc == NULL); ptr = ptr->next)
   {
      if ((strcmp(uri, "dtv://") == 0) || (strcmp(uri, ptr->uri) == 0))
      {
         /* This is a request to view the same service so use the same DTVSource instance */
         dtvsrc = ptr->dtvsrc;

         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s]: Found existing DTVSource instance %p for %s\n",
            __FUNCTION__, dtvsrc, uri);
      }
   }

   rmf_osal_mutexRelease(g_list_mutex);

   if (dtvsrc == NULL)
   {
      rmf_Error result = rmf_osal_memAllocP(RMF_OSAL_MEM_MEDIA, sizeof(S_DTVSOURCE_INSTANCE), (void **)&ptr);
      if (result == RMF_SUCCESS)
      {
         memset(ptr, 0, sizeof(S_DTVSOURCE_INSTANCE));

         if ((ptr->uri = strdup(uri)) != NULL)
         {
            ptr->dtvsrc = new RMFDTVSource();
            if (ptr->dtvsrc != NULL)
            {
               dtvsrc = ptr->dtvsrc;

               /* Add the instance to the start of the list */
               rmf_osal_mutexAcquire(g_list_mutex);

               ptr->next = g_dtvsource_list;
               g_dtvsource_list = ptr;

               rmf_osal_mutexRelease(g_list_mutex);

               RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s]: Created new DTVSource instance %p for %s\n",
                  __FUNCTION__, dtvsrc, uri);
            }
            else
            {
               free(ptr->uri);
               rmf_osal_memFreeP(RMF_OSAL_MEM_MEDIA, ptr);

               RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.DTV", "[%s:%d]: Failed to create DTVSource for uri=%s\n",
                  __FUNCTION__, __LINE__, uri);
            }
         }
         else
         {
            rmf_osal_memFreeP(RMF_OSAL_MEM_MEDIA, ptr);

            RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.DTV", "[%s:%d]: Failed to duplicate uri=%s\n",
               __FUNCTION__, __LINE__, uri);
         }
      }
      else
      {
         RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.DTV", "[%s:%d]: Failed to allocate DTVSource instance, uri=%s\n",
            __FUNCTION__, __LINE__, uri);
      }
   }

   return dtvsrc;
}

void RMFDTVSource::freeDTVSourceInstance(RMFDTVSource *dtvsrc)
{
   S_DTVSOURCE_INSTANCE *ptr;
   S_DTVSOURCE_INSTANCE *prev;

   rmf_osal_mutexAcquire(g_list_mutex);

   for (prev = NULL, ptr = g_dtvsource_list; ptr != NULL; prev = ptr, ptr = ptr->next)
   {
      if (ptr->dtvsrc == dtvsrc)
      {
         if (ptr->dtvsrc->refcount() == 0)
         {
            /* Instance can be removed from the list and freed */
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s]: Deleting existing DTVSource %p for %s\n",
               __FUNCTION__, ptr->dtvsrc, ptr->uri);

            if (prev == NULL)
            {
               g_dtvsource_list = ptr->next;
            }
            else
            {
               prev->next = ptr->next;
            }

            free(ptr->uri);
            rmf_osal_memFreeP(RMF_OSAL_MEM_MEDIA, ptr);

            delete dtvsrc;
         }
         else
         {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s]: DTVSource %p has refcount %u\n",
               __FUNCTION__, ptr->dtvsrc, ptr->dtvsrc->refcount());
         }
         break;
      }
   }

   rmf_osal_mutexRelease(g_list_mutex);
}


RMFDTVSource::RMFDTVSource(void) : RMFMediaSourceBase(), m_refcount(0)
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s]\n", __FUNCTION__);
}

RMFDTVSource::~RMFDTVSource(void)
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s]\n", __FUNCTION__);
}

/**
 * @brief
 * @return
 */
RMFResult RMFDTVSource::init()
{
   RMFResult result;

   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n" , __FUNCTION__, __LINE__);

   if (m_refcount == 0)
   {
      result = RMFMediaSourceBase::init();
   }
   else
   {
      result = RMF_RESULT_SUCCESS;
   }

   if (result == RMF_RESULT_SUCCESS)
   {
      m_refcount++;
   }

   return(result);
}

/**
 * @brief
 * @return
 */
RMFResult RMFDTVSource::term()
{
   RMFResult result;

   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]: refcount=%u\n" , __FUNCTION__, __LINE__, m_refcount);

   m_refcount--;

   if (m_refcount == 0)
   {
      result = RMFMediaSourceBase::term();
   }
   else
   {
      result = RMF_RESULT_SUCCESS;
   }

   return(result);
}

/**
 * @brief
 * @return
 */
RMFResult RMFDTVSource::open(const char* url, char* mimetype)
{
   RMFResult result;

   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d] url=\"%s\"\n" , __FUNCTION__, __LINE__, url);

   if (m_refcount == 1)
   {
      result = RMFMediaSourceBase::open(url, mimetype);
   }
   else
   {
      result = RMF_RESULT_SUCCESS;
   }

   return(result);
}

/**
 * @brief
 * @return
 */
RMFResult RMFDTVSource::close()
{
   RMFResult result;

   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n" , __FUNCTION__, __LINE__);

   if (m_refcount == 1)
   {
      result = RMFMediaSourceBase::close();
   }
   else
   {
      result = RMF_RESULT_SUCCESS;
   }

   return(result);
}

/**
 * @brief
 * @return
 */
RMFResult RMFDTVSource::play()
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n" , __FUNCTION__, __LINE__);
   return(RMFMediaSourceBase::play());
}

/**
 * @brief
 * @return
 */
RMFResult RMFDTVSource::play(float& speed, double& time)
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d] speed=%.2f, time=%.2f\n" ,
      __FUNCTION__, __LINE__, speed, time);
   return(RMFMediaSourceBase::play(speed, time));
}

/**
 * @brief
 * @return
 */
RMFResult RMFDTVSource::pause()
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n" , __FUNCTION__, __LINE__);
   return(RMFMediaSourceBase::pause());
}

/**
 * @brief
 * @return
 */
RMFResult RMFDTVSource::stop()
{
   RMFResult result;

   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n" , __FUNCTION__, __LINE__);

   if (m_refcount == 1)
   {
      result = RMFMediaSourceBase::stop();
   }
   else
   {
      result = RMF_RESULT_SUCCESS;
   }

   return(result);
}

/**
 * @brief
 * @return
 */
void *RMFDTVSource::createElement(void)
{
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DTV", "[%s:%d]\n" , __FUNCTION__, __LINE__);
   return(DTVSOURCE_IMPL->createElement());
}

RMFMediaSourcePrivate* RMFDTVSource::createPrivateSourceImpl()
{
   return(new RMFDTVSourcePrivate(this));
}

static int DtvkitProtocolCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user,
   void *in, size_t len)
{
   RMFDTVSourcePrivate *priv = (RMFDTVSourcePrivate *)user;
   unsigned int id;
   std::list<RPCRequest*>::iterator req;

   if (priv != NULL)
   {
      switch (reason)
      {
         case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n",
               __FUNCTION__, __LINE__);
            rmf_osal_mutexAcquire(priv->m_mutex);
            priv->m_wsi = NULL;
            rmf_osal_mutexRelease(priv->m_mutex);
            rmf_osal_condSet(priv->m_connected);
            break;

         case LWS_CALLBACK_CLIENT_ESTABLISHED:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: LWS_CALLBACK_CLIENT_ESTABLISHED\n",
               __FUNCTION__, __LINE__);
            rmf_osal_condSet(priv->m_connected);
            break;

         case LWS_CALLBACK_CLOSED:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: LWS_CALLBACK_CLOSED\n", __FUNCTION__, __LINE__);
            rmf_osal_mutexAcquire(priv->m_mutex);
            priv->m_wsi = NULL;
            rmf_osal_mutexRelease(priv->m_mutex);
            rmf_osal_condSet(priv->m_connected);
            break;

         case LWS_CALLBACK_CLIENT_RECEIVE:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: LWS_CALLBACK_CLIENT_RECEIVE, %ld bytes, \"%s\"\n",
               __FUNCTION__, __LINE__, len, (char *)in);

            if (sscanf((char *)in, "{\"jsonrpc\":\"2.0\",\"id\":%u,\"result\":", &id) == 1)
            {
               /* See if the this is a response to a request */
               rmf_osal_mutexAcquire(priv->m_mutex);

               std::list<RPCRequest*>::iterator req;

               for (req = priv->m_request_list.begin(); req != priv->m_request_list.end(); req++)
               {
                  if ((*req)->Sent() && (id == (*req)->Id()))
                  {
                     const char *result = strstr((const char *)in, JSON_RESULT_STRING);
                     if (result != NULL)
                     {
                        result += strlen(JSON_RESULT_STRING);

                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: Received response for %u, \"%s\"\n",
                           __FUNCTION__, __LINE__, id, result);

                        /* Save the response result and notify that it's ready to be used */
                        (*req)->ResponseReceived(result);
                     }
                     else
                     {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: Failed to find response result for %u\n",
                           __FUNCTION__, __LINE__, id);
                     }
                  }
               }

               rmf_osal_mutexRelease(priv->m_mutex);
            }
            break;

         case LWS_CALLBACK_CLIENT_WRITEABLE:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DTV", "[%s:%d]: LWS_CALLBACK_CLIENT_WRITEABLE\n",
               __FUNCTION__, __LINE__);

            rmf_osal_mutexAcquire(priv->m_mutex);

            for (req = priv->m_request_list.begin(); req != priv->m_request_list.end(); req++)
            {
               if (!(*req)->Sent())
               {
                  (*req)->Send(priv->m_wsi);
               }
            }

            rmf_osal_mutexRelease(priv->m_mutex);
            break;

         default:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DTV", "[%s:%d]: reason=%d\n", __FUNCTION__, __LINE__, reason);
            break;
      }
   }
   else
   {
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DTV", "[%s:%d]: reason=%d, user=NULL\n", __FUNCTION__, __LINE__, reason);
   }

   return(0);
}

