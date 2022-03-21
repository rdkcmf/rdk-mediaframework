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




#ifndef _CRYPTO_USER_H_
#define _CRYPTO_USER_H_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

//#include <lcsm_userSpaceInterface.h>
#include "cardmanagergentypes.h"
 

typedef enum crypto_EncType 
{
    Crypto_MIN = 0,
    Crypto_DES,
    Crypto_3DES,
    Crypto_MAX,

} Crypto_EncType;


typedef struct crypto_DeviceSession
{
    int cryptoDevice;           // file descriptor of crypto driver

} CRYPTO_DeviceSession;

  

/*********************************************************************
 *
 * Paste into your file & run to load the crypto dynamic library
 *
 *********************************************************************/
#if 0
void *trhandle;
static CRYPTO_INTERFACE *cryptoInterface;

static int loadCryptoInterface(void)
{
    int   returnValue;
    char  *error;
    
    returnValue = 0;
    trhandle = dlopen ("./libcrypto_mr.so", RTLD_LAZY);
    if ( trhandle == NULL ) 
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "Cannot load crypto Library \n");
        returnValue = -1;
    }
    else
    {
        cryptoInterface = (CRYPTO_INTERFACE *)dlsym(trhandle, "cryptoInt");
        if ((error = dlerror()) != NULL)  
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "cannot open crypto interface \n");
            returnValue = -1;
        }
    }
    
    return returnValue;
}
#endif


typedef CRYPTO_DeviceSession* (*_crypto_Init)( void );
typedef int (*_crypto_Shutdown)( CRYPTO_DeviceSession *DevSess );
typedef int (*_crypto_Reset)( CRYPTO_DeviceSession *DevSess );
typedef int (*_crypto_Encrypt)(CRYPTO_DeviceSession *DevSess, Crypto_EncType encryptionType,
                               uint32 *dataIn, uint32 dataInLength,
                               uint32 *pKey, uint32 keyLength,
                               uint32 *dataOut, uint32 *dataOutLength );
typedef int (*_crypto_Decrypt)( CRYPTO_DeviceSession *DevSess, Crypto_EncType encryptionType,
                                uint32 *dataIn, uint32 dataInLength,
                                uint32 *pKey, uint32 keyLength,
                                uint32 *dataOut, uint32 *dataOutLength );
typedef int (*_crypto_SetStreamKey)( LCSM_DEVICE_HANDLE handle, unsigned keyHi, unsigned keyLo );

typedef struct 
{
    _crypto_Init            crypto_Init;
    _crypto_Shutdown        crypto_Shutdown;
    _crypto_Reset           crypto_Reset;
    _crypto_Encrypt         crypto_Encrypt;
    _crypto_Decrypt         crypto_Decrypt;
    _crypto_SetStreamKey    crypto_SetStreamKey;

} CRYPTO_INTERFACE;


#ifndef  _CRYPTO_INTERFACE_MODULE_
#include <dlfcn.h>


#define  crypto_Init( ) \
    cryptoInterface->crypto_Init( )

#define crypto_Shutdown( DevSess ) \
    cryptoInterface->crypto_Shutdown( DevSess )
    
#define crypto_Reset( DevSess ) \
    cryptoInterface->crypto_Reset( DevSess )
    
#define crypto_Encrypt( DevSess, encryptionType, dataIn, dataInLength, pKey, keyLength, dataOut, dataOutLength ) \
    cryptoInterface->crypto_Encrypt( DevSess, encryptionType, dataIn, dataInLength, pKey, keyLength, dataOut, dataOutLength )
    
#define crypto_Decrypt( DevSess, encryptionType, dataIn, dataInLength, pKey, keyLength, dataOut, dataOutLength ) \
    cryptoInterface->crypto_Decrypt( DevSess, encryptionType, dataIn, dataInLength, pKey, keyLength, dataOut, dataOutLength )
    
#define crypto_SetStreamKey( handle, keyHi, keyLo ) \
    cryptoInterface->crypto_SetStreamKey( handle, keyHi, keyLo )    

#else /* _CRYPTO_INTERFACE_MODULE_ */
    
// use these for cryto_user.c
CRYPTO_DeviceSession *crypto_Init( void );
int crypto_Shutdown( CRYPTO_DeviceSession *DevSess );
int crypto_Reset( CRYPTO_DeviceSession *DevSess );
int crypto_Encrypt( CRYPTO_DeviceSession *DevSess, Crypto_EncType encryptionType,
                    uint32 *dataIn, uint32 dataInLength,
                    uint32 *pKey, uint32 keyLength,
                    uint32 *dataOut, uint32 *dataOutLength );
int crypto_Decrypt( CRYPTO_DeviceSession *DevSess, Crypto_EncType encryptionType,
                    uint32 *dataIn, uint32 dataInLength,
                    uint32 *pKey, uint32 keyLength,
                    uint32 *dataOut, uint32 *dataOutLength );
// for 56 bit keys, keyHi[31:8] contains bits [24:0] of the key
// keyLo contains bits [55:25] of the key
int crypto_SetStreamKey( LCSM_DEVICE_HANDLE handle, unsigned keyHi, unsigned keyLo );


#endif /* _CRYPTO_INTERFACE_MODULE_ */


#ifdef __cplusplus
}
#endif 

#endif /* _CRYPTO_USER_H_ */

