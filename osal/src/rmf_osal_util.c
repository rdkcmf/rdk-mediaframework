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

/**
 * @file rmf_osal_util.c
 */

/*
 * This file provides the CableLabs Reference Implementation of the rmf_osal_ utility APIs.
 */

/* Header Files */
//#include <rmf_osal_config.h>     /* Resolve configuration type references. */
#include <rmf_osal_types.h>      /* Resolve basic type references. */
#include <rmf_osal_error.h>      /* Resolve erroc code definitions */
#include <rmf_osal_socket.h>
#include <rmf_osal_file.h>
#include "rdk_debug.h"      /* Resolved RDK_LOG support. */
#include <rmf_osal_sync.h>     /* Resolve generic STB config structure type. */
#include <rmf_osal_util.h>     /* Resolve generic STB config structure type. */
#include <rmf_osal_mem.h>      /* Resolve memory functions */
#include <rmf_osal_util_priv.h>        /* Resolve target specific utility definitions. */
#include <rmf_osal_file.h>        /* Resolve target specific utility definitions. */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

/*Resolve reboot and related macros.*/
#include <unistd.h> 
#include <sys/reboot.h>
#include <linux/reboot.h>

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

extern void dispSetEnableOutputPort(rmf_osal_PowerStatus);
int os_getNumTuners(void);

#define RMF_OSAL_MEM_DEFAULT RMF_OSAL_MEM_UTIL

/* Node for storing env vars in a linked list cache */
typedef struct EnvVarNode
{
    char* name;
    char* value;
    struct EnvVarNode *next;
} EnvVarNode;

/* Env var cache */
static EnvVarNode *g_envCache = NULL;
static rmf_osal_Mutex g_cacheMutex;

/* Cached ED queue ID for power state change notification */
static rmf_osal_eventmanager_handle_t g_em = 0;

/* Cached ED handle for power state change notification */
static void *g_powerStateEdHandle = NULL;

/*flag to enable events.*/
static rmf_osal_Bool g_event_subscribed = FALSE;

/* Cached power state value */
static rmf_osal_PowerStatus g_powerState = RMF_OSAL_POWER_FULL;

/* AC outlet state */
static rmf_osal_Bool g_acOutletState = FALSE;

/**
 * OSAL private utility function
 * Recursively remove files and directories from the given directory path
 *
 * Returns 0 if successful, -1 otherwise
 */
rmf_Error removeFiles(char *dirPath)
{
    char path[RMF_OSAL_FS_MAX_PATH];
    rmf_osal_Dir dir;
    rmf_osal_DirEntry dirEnt;
    rmf_Error err;

    err = rmf_osal_dirOpen(dirPath,&dir);

    // If the directory does not exist, then we are good to go
    if (err == RMF_OSAL_FS_ERROR_NOT_FOUND)
        return RMF_SUCCESS;

    if (err != RMF_SUCCESS)
        return err;

    while (rmf_osal_dirRead(dir,&dirEnt) == RMF_SUCCESS)
    {
        /* Skip ".." and "." entries */
        if (strcmp(dirEnt.name,".") == 0 || strcmp(dirEnt.name,"..") == 0)
            continue;

        if (strlen(dirPath)+strlen(dirEnt.name)+strlen(RMF_OSAL_FS_SEPARATION_STRING) >= RMF_OSAL_FS_MAX_PATH)
            continue;

        /* Create the full path for this directory entry */
        memset( path, '\0', sizeof(path));
        strncpy(path,dirPath,sizeof(path) -1 );
        strcat(path,RMF_OSAL_FS_SEPARATION_STRING);
        strcat(path,dirEnt.name);

        /* Determine if this is a file or directory */
        if (dirEnt.isDir)
        {
            if (removeFiles(path) != RMF_SUCCESS)
                return RMF_OSAL_FS_ERROR_FAILURE;
        }
        else if ((err = rmf_osal_fileDelete(path)) != RMF_SUCCESS)
            return err;
    }
	rmf_osal_dirClose(dir);

    // Finally, remove the directory itself
    err = rmf_osal_dirDelete(dirPath);
    if (err != RMF_SUCCESS && err != RMF_OSAL_FS_ERROR_NOT_FOUND)
    {
        return err;
    }

    return RMF_SUCCESS;
}

#if 0 /*Functions Not implemented*/
void *rmf_osal_atomicOperation(void *(*op)(void*), void *data)
{
    RMF_OSAL_UNUSED_PARAM(op);
    RMF_OSAL_UNUSED_PARAM(data);
    return NULL;
}

uint32_t  rmf_osal_atomicIncrement(uint32_t *value)
{
    RMF_OSAL_UNUSED_PARAM(value);
    return 0;
}

uint32_t  rmf_osal_atomicDecrement(uint32_t *value)
{
    RMF_OSAL_UNUSED_PARAM(value);
    return 0;
}
#endif

/**
 * @brief This function is used to save the current stack context for subsequent non-local dispatch (jump).
 * The return value indicates whether the return is from the original direct
 * call to save the current stack or from an rmf_osal_longJmp() operation
 * that restored the saved stack image contents.
 *
 * @param[in] jmpBuf is the pointer to the "jump" buffer for saving the context information.
 *
 * @return an integer value indicating the return context.  A value of zero indicates
 *          a return from a direct call and a non-zero value indicates an indirect
 *          return (i.e. via rmf_osal_longJmp()).
 */
int rmf_osal_setJmp(rmf_osal_JmpBuf jmpBuf)
{
    return setjmp(jmpBuf);
}

/**
 * @brief This function is used to perform a non-local dispatch (jump) to the saved stack context.
 * It is useful for dealing with errors and interrupts encountered in a low-level subroutine of a program.
 *
 * @param[in] jmpBuf is a pointer to the "jump" buffer context to restore.
 * @param[in] val Value of automatic variable.
 */
void rmf_osal_longJmp(rmf_osal_JmpBuf jmpBuf, int val)
{
    longjmp(jmpBuf, val);
}

static void trim(char *instr, char* outstr)
{
    char *ptr = instr;
    char *endptr = instr + strlen(instr)-1;
    int length;

    /* Advance pointer to first non-whitespace char */
    while (isspace(*ptr))
        ptr++;

    if (ptr > endptr)
    {
        /*
         * avoid breaking things when there are
         * no non-space characters in instr
         */
        outstr[0] = '\0';
        return;
    }

    /* Move end pointer toward the front to first non-whitespace char */
    while (isspace(*endptr))
        endptr--;

    length = endptr + 1 - ptr;
    strncpy(outstr,ptr,length);
    outstr[length] = '\0';

}

/**
 * @brief This function is used to initialize any environment functionality prior to env usage
 *
 * @return None.
 */
void rmf_osal_envInit()
{
    const int hostaddr_buf_len = 20;
    const int hostname_buf_len = 256;
    char buf[hostaddr_buf_len];
    char hostNameBuf[hostname_buf_len];
    char dramMem[1024] = {'\0'};
    char videoMem[1024] = {'\0'};;

    // Init the mutex.
    rmf_osal_mutexNew(&g_cacheMutex);
#if 0
    //Init system configuration
    ri_platform_getTotalSystemMemory(dramMem);	
    ri_platform_getTotalVideoMemory(videoMem);	

    // Set up "automatic" values.
    rmf_osal_envSet("ocap.hardware.serialnum",  ri_platform_getSerialNumber(value));
    rmf_osal_envSet("ocap.hardware.vendor_id",  ri_platform_getVendorId(value));
    rmf_osal_envSet("ocap.hardware.version_id", ri_platform_getVersionId(value));
    rmf_osal_envSet("ocap.hardware.createdate",    ri_platform_getCreateDate(value));
#endif
    rmf_osal_envSet("ocap.memory.total", 			dramMem);
    rmf_osal_envSet("ocap.memory.video", 			videoMem);
    rmf_osal_envSet("ocap.system.highdef", 		"TRUE");
    rmf_osal_envSet("MPE.SYS.OSVERSION", 			"2.6");
    rmf_osal_envSet("MPE.SYS.OSNAME", 				OS_NAME);
    rmf_osal_envSet("MPE.SYS.MPEVERSION", 			RMF_OSAL_VERSION);
    //rmf_osal_envSet("MPE.SYS.OCAPVERSION", 		OCAP_VERSION);
    rmf_osal_envSet("MPE.SYS.AC.OUTLET", 			"TRUE");

    if (rmf_osal_envGet("MPE.SYS.RFMACADDR") == NULL)
    {
        rmf_osal_socket_getMacAddress(NULL, buf);
        rmf_osal_envSet("MPE.SYS.RFMACADDR", buf);
    }

    if (rmf_osal_envGet("MPE.SYS.ID") == NULL)
    {
        /* Set the unique system ID to the RF MAC Address (withouth the ':' characters) */
        const char * mac = rmf_osal_envGet("MPE.SYS.RFMACADDR");
        if (mac != NULL)
        {
            // strip out the ':' characters
            int i = 0, j = 0;
            do
                if (mac[i] != ':')
                    buf[j++] = mac[i];
            while (mac[i++]);
            rmf_osal_envSet("MPE.SYS.ID", buf);
        }
    }

    // Get the system IP address, defaulting to 0.0.0.0
    if (rmf_osal_socketGetHostName(hostNameBuf, hostname_buf_len) != 0)
    {
        rmf_osal_SocketHostEntry* hostEntry = rmf_osal_socketGetHostByName(hostNameBuf);
        if (hostEntry != NULL && (hostEntry->h_addr_list != NULL) 
            && (NULL != hostEntry->h_addr_list[0]) )
        {
            snprintf(buf, hostaddr_buf_len, "%d.%d.%d.%d", hostEntry->h_addr_list[0][0],
                hostEntry->h_addr_list[0][1], hostEntry->h_addr_list[0][2],
                hostEntry->h_addr_list[0][3]);
        }
    }
    else
    {
        strcpy(buf, "0.0.0.0");
    }

    // Add the hostname to the default list.
    rmf_osal_envSet("MPE.SYS.IPADDR", buf);

    // Set the number of tuners
    // First attempt to retrieve the number of tuners from rmfconfig.ini.
    if (rmf_osal_envGet("MPE.SYS.NUM.TUNERS") == NULL)
    {
        // Not set, so retrieve the number of tuners from the target platform.
        int numTuners = os_getNumTuners();
        if (numTuners > 0)
        {
            snprintf(buf,sizeof(buf), "%d", numTuners);
            rmf_osal_envSet("MPE.SYS.NUM.TUNERS", buf);
        }
        else
            // Default to a single tuner.
            rmf_osal_envSet("MPE.SYS.NUM.TUNERS", "1");
    }
}

/**
 * @brief This function is used to set up the environment variable storage by parsing configuration file.
 *
 * @param[in] path Path of the file.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_env_add_conf_file( const char * path)
{
    const int line_buf_len = 256;

    FILE* f;
    char lineBuffer[line_buf_len];

    /* Open the env file */
    if ((f = fopen( path,"r")) == NULL)
    {
        printf("***************************************************\n");
        printf("***************************************************\n");
        printf("**    ERROR!  Could not open configuration file!    **\n");
        printf("***************************************************\n");
        printf("***************************************************\n");
        printf("(Tried %s\n", path);
        return RMF_OSAL_EINVAL;
    }
    printf("Conf file %s open success\n", path);

    rmf_osal_mutexAcquire(g_cacheMutex);
    /* Read each line of the file */
    while (fgets(lineBuffer,line_buf_len,f) != NULL)
    {
        char name[line_buf_len];
        char value[line_buf_len];
        char trimname[line_buf_len];
        char trimvalue[line_buf_len];
        EnvVarNode *node;
        char *equals;
        int length;

        /* Ignore comment lines */
        if (lineBuffer[0] == '#')
            continue;

        /* Ignore lines that do not have an '=' char */
        if ((equals = strchr(lineBuffer,'=')) == NULL)
            continue;

        /* Read the property and store in the cache */
        length = equals - lineBuffer;
        strncpy( name,lineBuffer,length);
        name[ length] = '\0'; /* have to null-term */

        length = lineBuffer + strlen(lineBuffer) - equals + 1;
        strncpy( value,equals+1,length);
        value[ length] = '\0' ;

        /* Trim all whitespace from name and value strings */
        trim( name,trimname);
        trim( value,trimvalue);

        node = ( EnvVarNode*)malloc(sizeof(EnvVarNode));
        node->name = strdup( trimname);
        node->value = strdup( trimvalue);

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.UTIL", "rmf_osal_envInit: %s=%s\n", node->name, node->value); // needs to be printf
        /* Insert at the front of the list */
        node->next = g_envCache;
        g_envCache = node;
    }

    rmf_osal_mutexRelease( g_cacheMutex);

    fclose( f);
    return RMF_SUCCESS;
}


/**
 * @brief This function is used to get the value of the specified environment variable.
 *
 * @param name is a pointer to the name of the target environment variable.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
const char* rmf_osal_envGet(const char *name)
{
    EnvVarNode *node = g_envCache;

    rmf_osal_mutexAcquire(g_cacheMutex);

    while (node != NULL)
    {
        /* Env var name match */
        if (strcmp(name,node->name) == 0)
        {
            /* return the value */
            rmf_osal_mutexRelease(g_cacheMutex);
            return node->value;
        }

        node = node->next;
    }

    /* Not found */
    rmf_osal_mutexRelease(g_cacheMutex);
    return NULL;
}

/**
 * @brief This function is used to set the value of the specified environment variable.
 *
 * @param name is a pointer to the name of the target environment variable.
 * @param value is a pointer to a NULL terminated value string.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_envSet(const char *name, const char *value)
{
    EnvVarNode *node = g_envCache;

    if(NULL == name)
    {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL", "rmf_osal_envSet: NULL POINTER  passed for name string\n");
      return RMF_OSAL_EINVAL;
    }
    if(NULL == value)
    {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL", "rmf_osal_envSet: NULL POINTER  passed for value string\n");
      return RMF_OSAL_EINVAL;
    }

    rmf_osal_mutexAcquire(g_cacheMutex);

    /* Search for an existing entry */
    while (node != NULL)
    {
        if (strcmp(name,node->name) == 0)
        {
            /* Free the existing value and store the new one */
            free(node->value);
            node->value = strdup(value);
            rmf_osal_mutexRelease(g_cacheMutex);
            return RMF_SUCCESS;
        }

        node = node->next;
    }

    /* Not found, so create a new entry */
    node = (EnvVarNode*)malloc(sizeof(EnvVarNode));
    if (node)
    {
        node->name = strdup(name);
        node->value = strdup(value);

        /* Place at the front of the list */
        node->next = g_envCache;
    }
    g_envCache = node;

    rmf_osal_mutexRelease(g_cacheMutex);
    return RMF_SUCCESS;
}

/**
 * @brief Get the current STB power status.
 *
 * @return unsigned integer representing the current power mode status.
 */
rmf_osal_PowerStatus rmf_osal_stbGetPowerStatus(void)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL", "rmf_osal_stbGetPowerStatus : %d\n", g_powerState);
  return g_powerState;
}

/**
 * @brief This function is used to registers the queue Id and the Event dispatcher handle to be used
 * to notify a power state change. The values are cached in global variables
 * and used by rmf_osal_stbTogglePowerMode when power key is detected.
 *
 * @param[in] event_queue specifies the ID of the queue to be used for events notification.
 * @param[in] act User data to be set on the event.
 *
 * @return Returns RMF_SUCCESS, perhaps the call should fail if someone has already registered
 */
rmf_Error rmf_osal_registerForPowerKey(rmf_osal_eventqueue_handle_t event_queue, void *act)
{
    rmf_Error ret;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL", "rmf_osal_registerForPowerKey\n");
    ret = rmf_osal_eventmanager_register_handler(
                g_em,
                event_queue,
                RMF_OSAL_EVENT_CATEGORY_POWER);
    if (RMF_SUCCESS != ret )
    {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL",
            " rmf_osal_eventmanager_register_handler failed\n");
    }
    g_powerStateEdHandle = act;
    g_event_subscribed = TRUE;

    return RMF_SUCCESS;
}


/**
 * @brief This function is used to service various "bootstrap" related requests.
 * The bootstrap operations consist of:
 * <ul>
 * <li> 1. Reboot the entire STB (equivilent to power-on reset).
 * <li> 2. Fast boot 2-way mode (forward & reverse data channels enabled).
 * </ul>
 *
 * @param[in] code Boot code such as Reset, Fast bootup, etc.
 * @retval RMF_SUCCESS On successful operated the boot code.
 * @retval RMF_OSAL_EINVAL Invalid code supplied as an argument.
 */
rmf_Error rmf_osal_stbBoot(rmf_osal_STBBootMode code)
{
    switch (code)
    {
        case RMF_OSAL_BOOTMODE_RESET:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL",
                    "rmf_osal_stbBoot: resetting the stack\n");

            printf("*+*+*+*+*+*+*+* calling platform_reset *+*+*+*+*+*+*+*\n");
            system( "sh /rebootNow.sh -s Rmf_platformReset -o 'Rebooting the box due to calling platform_reset...'" );
            return RMF_SUCCESS;

        case RMF_OSAL_BOOTMODE_FAST:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL",
                "rmf_osal_stbBoot: fast boot for 2-way network invoked\n");
            return RMF_SUCCESS;

        default:
            return RMF_OSAL_EINVAL;
    }
}

/**
 * @brief This function is used to allow the entire stack to query and update the bootstrap status
 * of the stack itself (e.g.  managers installed, JVM started, UI events available, 
 * 2-way network status, etc).
 *
 * The boot status information is simply a 32-bit value with each bit defined to
 * represent a "stage" of the boot process or the "availability" of a resource
 * (e.g. network status).
 *
 * If the "update" flag parameter is TRUE the call is an update call and the second
 * parameter contains the additional boot status information to be added to the current
 * status.  The third parameter is used to optionally clear any status bit information
 * for any status that may have changed.  If any bits are not to be cleared 0xFFFFFFFF
 * should be passed for the bit mask value.
 *
 * @param[in] update is a boolean indicating a call to update the boot status
 * @param[in] statusBits is a 32-bit value representing the new status update information
 *        the bits are logically ORed with the current boot status.
 * @param[in] bitMask is a bit pattern for optionally clearing any particular status buts.
 *
 * @return Returns a 32-bit bit pattern indicating the status of the various elements of the system.
 */
uint32_t rmf_osal_stbBootStatus(rmf_osal_Bool update, uint32_t statusBits,
        uint32_t bitMask)
{
    static uint32_t bootStatus = 0;

    /* Check for updating status. */
    if (TRUE == update)
    {
        bootStatus &= bitMask; /* Clear any selected status bits. */
        bootStatus |= statusBits; /* Set new bits. */
    }

    /* Return bootstrap status. */
    return bootStatus;
}

/**
 * @brief This function is used to get the current AC outlet state.
 *
 * @param[out] state indicating the current state of the external AC outlet.
 *
 * @retval RMF_SUCCESS if the outlet state was successfully retrieved
 * @retval RMF_OSAL_EINVAL if the start parameter is invalid
 */
rmf_Error rmf_osal_stbGetAcOutletState(rmf_osal_Bool *state)
{
    if (NULL == state)
    	return RMF_OSAL_EINVAL;
    *state = g_acOutletState;
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to set the current AC outlet state.
 *
 * @param[in] enable value indicating the new state of the AC outlet
 *
 * @return Returns RMF_SUCCESS if the AC outlet state was set successfully
 */
rmf_Error rmf_osal_stbSetAcOutletState(rmf_osal_Bool enable)
{
    g_acOutletState = enable;
    return RMF_SUCCESS;
}

#if 0
/**
 * <i>rmf_osal_stbGetRootCerts()</i>
 *
 * Acquire the initial set of root certificates for the platform. The format of
 * the root certificate(s) must be in either raw "binary" or base64 encoding. In
 * the latter case each certificate must be properly delineated with the
 * '-----BEGIN CERTIFICATE-----' and '-----END CERTIFICATE-----' separators.
 * Essentially, the byte array must be in a format suitable for use with the
 * java.security.cert.CertificateFactory.generateCertificate(ByteArrayInputStream)
 * method.
 *
 * @param roots is a pointer for returning a pointer to the memory location
 *        containing the platform root certificate(s).
 * @param len is a pointer for returning the size (in bytes) of the memory location
 *        containing the roots.
 *
 * @return RMF_SUCCESS if the pointer to and length of the root certificate image
 *         was successfully acquired.
 */
rmf_Error rmf_osal_stbGetRootCerts(uint8_t **roots, uint32_t *len)
{
	return ri_platform_mfr_nvm_GetRootCerts(roots, len);
}
#endif

/**
 * @brief This function supposed to resets the STB. It is not yet implemented.
 *
 * @retval RMF_SUCCESS
 */

rmf_Error rmf_osal_stbResetAllDefaults(void)
{
    /* not yet implemented */
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to set the power state to the value passed in.
 * Does not move the power state through a state machine, just sets the value and notifies listeners.
 *
 * @param[in] newPowerMode one of the rmf_osal_PowerStatus values.
 * @retval RMF_SUCCESS
 */
rmf_Error rmf_osal_stbSetPowerStatus(rmf_osal_PowerStatus newPowerMode)
{

   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL",
            "rmf_osal_stbSetPowerStatus: power mode has changed from %s to %s\n",
            ((g_powerState == RMF_OSAL_POWER_FULL) ? "RMF_OSAL_POWER_FULL"
                    : "RMF_OSAL_POWER_STANDBY"),
            ((newPowerMode == RMF_OSAL_POWER_FULL) ? "RMF_OSAL_POWER_FULL"
                    : "RMF_OSAL_POWER_STANDBY"));

    g_powerState = newPowerMode;

    /* If somebody is listening, let them know that the power state has changed. */
    if (TRUE == g_event_subscribed )
    {
        rmf_osal_event_handle_t event_handle;
        rmf_osal_event_params_t event_params;
        rmf_Error ret;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL",
                "Push power key event into ED queue\n");
#if 0
        rmf_osal_eventQueueSend(g_powerStateQueueId, g_powerState, 0,
                (void*) g_powerStateEdHandle, 0);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n [RI]: %s: Entering... \n",__FUNCTION__);
#endif
        /* Send the event. */
        event_params.data = (void*)g_powerStateEdHandle;
        event_params.data_extension = 0;
        event_params.free_payload_fn = NULL;
        event_params.priority = 0;
        ret = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POWER, 
                g_powerState,  &event_params, &event_handle);
        if ( RMF_SUCCESS != ret )
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL",
                        "rmf_osal_event_create() failed\n");
                return ret;
        }

        ret = rmf_osal_eventmanager_dispatch_event( g_em ,
                event_handle);
        if ( RMF_SUCCESS != ret )
        {
                rmf_osal_event_delete( event_handle );
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL",
                        "rmf_osal_eventmanager_dispatch_event() failed\n");
                return ret;
        }
    }

    return RMF_SUCCESS;
}

/**
 * @brief This function is called when the power key is pressed. Used to push the power state change
 * events into the Event Dispatcher queue. 

 * Currently the supported power states are RMF_OSAL_POWER_FULL and RMF_OSAL_POWER_STANDBY.
 * @return None
 */
void rmf_osal_stbTogglePowerMode(void)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL", "rmf_osal_stbTogglePowerMode\n");

  if (g_powerState == RMF_OSAL_POWER_FULL)
        (void) rmf_osal_stbSetPowerStatus(RMF_OSAL_POWER_STANDBY);
  else
        (void) rmf_osal_stbSetPowerStatus(RMF_OSAL_POWER_FULL);
}



/**
 * @brief OSAL private utility supposed to retrieve the number of tuners from the underlying target platform.
 * This is not yet implemented
 * @retval 0 always.
 */
int os_getNumTuners(void)
{
    unsigned int numTuners = 0;
#if 0 
    // There is a 1-to-1 association between a RI Platform pipeline and a tuner.
    // If this association changes, the number of tuners may be extracted from the
    // RI platform.cfg file instead.
    ri_pipeline_manager_t *mgr = ri_get_pipeline_manager();
    (void) mgr->get_live_pipelines(mgr, &numTuners);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL",
            "os_getNumTuners: %d tuners found in target platform.\n", numTuners);
#endif

    return numTuners;
}

