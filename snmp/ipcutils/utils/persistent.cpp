/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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



/** \file pfcpersistentconfig.h General persistent config system. */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <rdk_debug.h>
#include <errno.h>
#ifdef GCC4_XXX
#include <stdlib.h>
#include <string.h>

#else

#endif

#ifndef PFCPERSISTENTCONFIG_H
#include "persistentconfig.h"
#endif

#include "cmhash.h"

#define MAX_PATH 1024

#define PFC_ROOT            "/"
#define PFC_STATIC_ROOT     "/opt/"
#define PFC_DYNAMIC_ROOT    "/opt/"
#define PFC_VOLATILE_ROOT   "/opt/"
#define PFC_RC              "/etc/pfcrc"

CMpersistentConfig::CMpersistentConfig (CONFIG_TYPE eConfigType, char *rootpath, char * relativepathname)
{
    if(NULL == relativepathname) relativepathname = PFC_RC;
    initialize(eConfigType, rootpath, relativepathname);
}

CMpersistentConfig::RESULT
CMpersistentConfig::initialize (CMpersistentConfig::CONFIG_TYPE eConfigType, char *rootpath, char * relativepathname)
{
    FILE * testopen = NULL;
    char * pfcrootdir    = getenv("PFC_ROOT");
    char * altrootdir    = NULL;
    int i = 0;
    char * pDir = 0;
    char szDir[MAX_PATH];

    switch(eConfigType)
    {
        case STATIC:
        {
            altrootdir = PFC_STATIC_ROOT;
        }
        break;
        
        case DYNAMIC:
        {
            altrootdir = PFC_DYNAMIC_ROOT;
        }
        break;
        
        case VOLATILE:
        {
            altrootdir = PFC_VOLATILE_ROOT;
        }
        break;
    }
    
    // initialize with default root
    memset (m_szActualPathName, '\0', sizeof(m_szActualPathName));
    strncpy(m_szActualPathName, PFC_ROOT, sizeof(m_szActualPathName)-1);

    // try using supplied root path
    if( NULL != rootpath )
    {       
            if( rootpath[0] != '/' )
            {
                strncat(m_szActualPathName, rootpath, (MAX_PATH1 - strlen(m_szActualPathName) - 1) );
            }
            else
            {
                strncat(m_szActualPathName, &rootpath[1], (MAX_PATH1 - strlen(m_szActualPathName) - 1));
            }
    }
    // try using mfr configuration path
    else if(NULL != altrootdir)
    {       
             if(altrootdir[0] != '/')
             {
                 strncat(m_szActualPathName, altrootdir, (MAX_PATH1 - strlen(m_szActualPathName) - 1));
             }
             else
             {
                 strncat(m_szActualPathName, altrootdir+1, (MAX_PATH1 - strlen(m_szActualPathName) - 1));
             }	
    }
    // try using pfc root path
    else if(NULL != pfcrootdir)
    {       
             if(pfcrootdir[0] != '/')
             {
                 strncat(m_szActualPathName, pfcrootdir, (MAX_PATH1 - strlen(m_szActualPathName) - 1));
             }
             else
             {
                 strncat(m_szActualPathName, pfcrootdir+1, (MAX_PATH1 - strlen(m_szActualPathName) - 1));
             }	
    }

        if(m_szActualPathName[strlen (m_szActualPathName) - 1] != '/')
        {
            strncat(m_szActualPathName, "/", (MAX_PATH1 - strlen(m_szActualPathName) - 1));
        }
  

    if(NULL != relativepathname)
    {
             if(relativepathname[0] != '/')
             {
                 strncat(m_szActualPathName, relativepathname, (MAX_PATH1 - strlen(m_szActualPathName) - 1));
             }
             else
             {
                 strncat(m_szActualPathName, relativepathname+1, (MAX_PATH1 - strlen(m_szActualPathName) - 1));
             }
    }

    if(m_szActualPathName[strlen (m_szActualPathName) - 1] == '/')
    {
        //RDK_LOG(RDK_LOG_INFO,RMF_MOD_OS,"%s : '%s' is a directory\n", __FUNCTION__, m_szActualPathName);
        strcpy(m_szActualPathName, "");
        return ERROR_FILE_IS_A_DIRECTORY;
    }

    // now create the directory if it doesn't exist
    memset ( szDir, '\0', sizeof(szDir));
    strncpy(szDir, m_szActualPathName, sizeof(szDir) -1 );
    pDir = strchr(szDir, '/');
    while(NULL != pDir)
    {
        strncpy(szDir, m_szActualPathName, sizeof(szDir)-1);
        pDir = strchr(pDir+1, '/');

        if(NULL != pDir)
        {
            int ret_val = 0;
            // Terminate the directory
            *pDir = '\0';
            // RDK_LOG(RDK_LOG_INFO,"LOG.RDK.OS","%s : Creating directory '%s'.\n", __FUNCTION__, szDir);
            ret_val = mkdir(szDir, 0777);
            if( ret_val && (errno != EEXIST) )  
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s:%d: Directory - %s -creation failed  \n", __FUNCTION__,__LINE__, szDir);
            }			
        }
    }

    testopen = fopen(m_szActualPathName, "r+");
    if(NULL != testopen) fclose(testopen);
    
    if(NULL != testopen) return SUCCESS;
    
    testopen = fopen(m_szActualPathName, "r");
    if(NULL != testopen) fclose(testopen);
    
    if(NULL != testopen)
    {
         //RDK_LOG(RDK_LOG_INFO,"LOG.RDK.OS","%s : '%s' is read-only.\n", __FUNCTION__, m_szActualPathName);
        return READ_ONLY;
    }
    
    testopen = fopen(m_szActualPathName, "w+");
    if(NULL != testopen) fclose(testopen);

    if(NULL != testopen) return SUCCESS;

    strcpy(m_szActualPathName, "");
    return ERROR_INVALID_DIRECTORY_PATH;
}

char * CMpersistentConfig::get_actual_path_name()
{
    return m_szActualPathName;
}

int CMpersistentConfig::save_to_disk ()
{
    if(0 == strlen(m_szActualPathName)) return NOT_INITIALIZED;
    return CMHash::save_to_disk(m_szActualPathName);
}

int CMpersistentConfig::load_from_disk ()
{
    if(0 == strlen(m_szActualPathName)) return NOT_INITIALIZED;
    return CMHash::load_from_disk(m_szActualPathName);
}

