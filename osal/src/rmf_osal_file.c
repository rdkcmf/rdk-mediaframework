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


#include <string.h>

#include <rmf_osal_file.h>
#include <rmf_osal_file_priv.h>
#include "rdk_debug.h"
#include <rmf_osal_util.h>

#include <unistd.h>

/* port-specific default-system-directory lookup */
rmf_Error rmf_osal_fileGetDefaultDir(char *buf, uint32_t bufSz)
{
    static const char* defaultSysDir = NULL;

    /* insure that we have a valid caller's buffer                            **
     ** otherwise, they are just using this call to get the size of the string */
    if (buf == NULL)
    {
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    if (defaultSysDir == NULL)
    {
        const char* sysDirEnv;
        if ((sysDirEnv = rmf_osal_envGet(RMF_OSAL_FS_ENV_DEFAULT_SYS_DIR)) == NULL) // try to get it from FS.DEFSYSDIR in rmfconfig.ini
        {
            sysDirEnv = OS_FS_DEFAULT_SYS_DIR; // else use hard coded value from rmf_osal_file.h
        }

        /* check for special case when default-system-directory is "." */
        if (strcmp(sysDirEnv, ".") == 0)
        {
            char cwd[RMF_OSAL_FS_MAX_PATH];

            /* get absolute path for our current-working-directory */
            (void) getcwd(cwd, RMF_OSAL_FS_MAX_PATH);
            defaultSysDir = strdup(cwd);
        }
        else
        {
            defaultSysDir = strdup(sysDirEnv);
        }
    }
    if (strlen(defaultSysDir) >= bufSz)
        return RMF_OSAL_FS_ERROR_OUT_OF_MEM;

    strcpy(buf, defaultSysDir);
    return RMF_SUCCESS;
}

#if 0
/* initialize all of the file system drivers for this port */
void rmf_osal_filesysInit(void(*rmf_osal_filesysRegisterDriver)(
        const rmf_osal_filesys_ftable_t *, const char *))
{
    /*
     * Default Filesystem:  hard-disk
     */
    (*rmf_osal_filesysRegisterDriver)(&port_fileHDFTable, "");
}
#endif

