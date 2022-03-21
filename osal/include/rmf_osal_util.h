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
 * @file rmf_osal_util.h
 */

/**
 * @defgroup OSAL_UTILS OSAL Utils
 * RMF OSAL utility API provides the routine utility functions to upper layers. 
 * This includes environment variables, configuration of the platform and run time
 * status like power- level status etc.
 *
 * OSAL utilities is responsible for handling the following operation,
 * <ul>
 * <li> Useful for dealing with errors and interrupts encountered in a
 * low-level subroutine of a program. 
 * <li> Save the current stack image that can be restored the saved stack image contents.
 * <li> This is used to initialize any environment functionality prior to env usage.
 * <li> Get/Set the value of the specified environment variable, which will help all the components
 * to make use of environment settings and perform acordingly.
 * <li> Register for power key & set the power state to stand-by, no-standby, etc.
 * </ul>
 * @ingroup OSAL
 *
 * @defgroup OSAL_UTILS_API OSAL Utils API list
 * RMF OSAL Utils module defines interfaces for common utility functions such as managing environment values and managing power.
 * @ingroup OSAL_UTILS
 *
 * @addtogroup OSAL_UTILS_TYPES OSAL Utils Data Types
 * Describe the details about Data Structure used by OSAL Utils.
 * @ingroup OSAL_UTILS
 */

#if !defined(_RMF_OSAL_UTIL_H)
#define _RMF_OSAL_UTIL_H

#include <rmf_osal_types.h>      /* Resolve basic type references. */
#include <rmf_osal_event.h> 
#include <rmf_osal_error.h>
#include <setjmp.h>		/* Include linux setjmp.h header. */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup OSAL_UTILS_TYPES
 * @{
 */

/***
 * Type definitions:
 */

/* setjmp/longjmp implementation buffer. */
typedef jmp_buf rmf_osal_JmpBuf;

typedef enum
{
    RMF_OSAL_BOOTMODE_RESET = 1, /* Reset STB, equivalent to power-on reset. */
    RMF_OSAL_BOOTMODE_FAST
/* Attempt immediate 2-way (FDC+RDC) connection. */
} rmf_osal_STBBootMode;

/**
 * Type used to indicate the STB power status.
 */
typedef enum
{
    RMF_OSAL_POWER_FULL = 1, /**< Full power mode. */
    RMF_OSAL_POWER_STANDBY,
/**< Standby (low) poer mode. */
} rmf_osal_PowerStatus;

/*
 * OSAL bootstrap status bit definitions, returned from rmf_osal_stbBootStatus().
 */
#define RMF_OSAL_BS_RMF_OSAL_LOWLVL 	0x00000001		/* Low-level initialization done. */
#define RMF_OSAL_BS_NET_NOWAY	0x00000010  	/* Broadcast, FDC, RDC not available. */
#define RMF_OSAL_BS_NET_1WAY     0x00000020      /* Broadcast mode only, no RDC. */
#define RMF_OSAL_BS_NET_2WAY     0x00000040      /* Broadcast, FDC, RDC available. */
#define RMF_OSAL_BS_NET_MASK		0x00000070      /* Mask for network status bits. */

/** @} */ //end of doxygen tag OSAL_UTILS_TYPES

/***
 * Set jump/long jump support API prototypes:
 */

int rmf_osal_setJmp(rmf_osal_JmpBuf jmpBuf);

void rmf_osal_longJmp(rmf_osal_JmpBuf jmpBuf, int val);

/***
 * Environment variable utility API prototypes:
 */

/**
 * @addtogroup OSAL_UTILS_API
 * @{
 */
const char* rmf_osal_envGet(const char *name);

rmf_Error rmf_osal_envSet(const char *name, const char *value);

void rmf_osal_envInit(void);

/* STB related utility API prototypes: */

rmf_Error rmf_osal_registerForPowerKey(rmf_osal_eventqueue_handle_t queueId, void *act);

rmf_osal_PowerStatus rmf_osal_stbGetPowerStatus(void);

rmf_Error rmf_osal_stbBoot(rmf_osal_STBBootMode mode);

uint32_t rmf_osal_stbBootStatus(rmf_osal_Bool update, uint32_t statusBits,
        uint32_t bitMask);

rmf_Error rmf_osal_stbGetAcOutletState(rmf_osal_Bool *state);

rmf_Error rmf_osal_stbSetAcOutletState(rmf_osal_Bool enable);

#if 0
/**
 * <i>rmf_osal_stbGetRootCerts()</i>
 *
 * Acquire the initial set of root certificates for the platform.
 *
 * @param roots is a pointer for returning a pointer to the memory location
 *        containing the platform root certificate(s).
 * @param len is a pointer for returning the size (in bytes) of the memory location
 *        containing the roots.
 *
 * @return RMF_SUCCESS if the pointer to and length of the root certificate image
 *         was successfully acquired.
 */
rmf_Error rmf_osal_stbGetRootCerts(uint8_t **roots, uint32_t *len);
#endif

rmf_Error rmf_osal_stbSetPowerStatus(rmf_osal_PowerStatus newPowerMode);

rmf_Error rmf_osal_env_add_conf_file(const char * path);

void rmf_osal_stbTogglePowerMode(void);

/** @} */ //end of doxygen tag OSAL_UTILS_API

#ifdef __cplusplus
}
#endif
#endif /* _RMF_OSAL_UTIL_H */
