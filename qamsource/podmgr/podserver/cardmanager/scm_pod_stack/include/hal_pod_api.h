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


#ifndef POD_API_H
#define POD_API_H

#ifdef __cplusplus
extern "C" {
#endif


/*-------------------------------------------------------------------
   Include Files 
-------------------------------------------------------------------*/


/*-------------------------------------------------------------------
   Defines/Macros
-------------------------------------------------------------------*/
 
#define HAL_POD_MAX_PODS   (1)        /*  number of cablecards */
 



/*-------------------------------------------------------------------
   Types/Structs
-------------------------------------------------------------------*/
typedef enum 
{                   // Card Modes
    POD_MODE_SCARD,   // The cablecard is a single stream card
    POD_MODE_MCARD    // The cablecard can handle multiple streams
    
} POD_MODE_t;  


typedef enum
{
    HOST_LEGACY,
    HOST_MCARD_SUPPORTED


} HOST_MODE_t;


typedef enum 
{                   /* Card Status */  
    POD_CARD_INSERTED = 0,   /* The cablecard is present/inserted */
    POD_CARD_REMOVED = 1,    /* The cablecard is removed*/ 
    POD_CARD_ERROR = 2,      /* CableCard generated a error*/
    POD_PERSONALITY_CHANGE_COMPLETE = 3, /*CableCard personality change completed*/
    POD_DATA_AVAILABLE = 4
} POD_STATUS_t;  

typedef enum 
{
       
    HAL_POD_IIR,    
    HAL_POD_PCMCIA_RESET, // PCMCIA reset has been issued from within HAL - so current sessions have to be closed
       //refer SCTE28-2004 Table E.1-A  error handling
    HAL_POD_ERROR_READY_NOT_ASSERTED, //error 1
    HAL_POD_ERROR_INVALID_CIS,    //error 2
    HAL_POD_ERROR_INVALID_COR,    //error 3
    HAL_POD_ERROR_DATA_FR_BIT_TIMEOUT,    //error 4
    HAL_POD_ERROR_EXT_FR_BIT_TIMEOUT,    //error 5
    HAL_POD_ERROR_DATA_BUFFER_SIZE_NEGO,     //error 6, 7 
    HAL_POD_ERROR_EXT_BUFFER_SIZE_NEGO,     //error 8,9
     

} POD_ERROR_TYPE_t;


typedef enum
{
   
     POD_NOTIFY_CARD_DETECT,
     POD_NOTIFY_DATA_AVAILABLE,
     POD_NOTIFY_EXTCH_DATA_AVAILABLE,
     POD_NOTIFY_SEND_POD_POLL,
     POD_NOTIFY_ERROR_CONDITION
          
} POD_NOTIFY_TYPE_t;




typedef enum
{
    POD_DATA_CHANNEL_DATA,
    POD_EXT_CHANNEL_DATA

}POD_CHANNEL_TYPE_T;


typedef struct
{

    HOST_MODE_t   hostMode;
    

}POD_ATTRIBUTES_t; 


typedef struct
{
               /* Current POD settings. */
    
    POD_MODE_t    cardMode;
               
    
} POD_INFO_t;



typedef struct
{
    DEVICE_HANDLE_t hPODHandle;
    POD_MODE_t                  cardType;// 1 << TUNER_MODE_t
} POD_CAPABILITIES_INSTANCE_t;


typedef struct
{
    UINT8                           usNumPODs;
    POD_CAPABILITIES_INSTANCE_t   astInstanceCapabilities[HAL_POD_MAX_PODS];
} POD_CAPABILITIES_t;



typedef void (POD_NOTIFY_FUNC1_t) (DEVICE_HANDLE_t      hDevicehandle, 
                                    POD_STATUS_t       eStatus, 
                                    const void         *pData,
                                    UINT32             ulDataLength,
                                    void               *pulDataRcvd);
                    
                    
typedef void (POD_NOTIFY_FUNC2_t) (DEVICE_HANDLE_t      hDevicehandle, 
                                    const void         *pData);                            


typedef void (POD_NOTIFY_FUNC3_t) (DEVICE_HANDLE_t      hDevicehandle, 
                    T_TC        *pTPDU,
                                    const void         *pData);    
                    
                    
typedef void (POD_NOTIFY_FUNC4_t) (DEVICE_HANDLE_t      hDevicehandle, 
                    ULONG         lFlowId,
                                    const void          *pData,
                    unsigned short    wSize);                            
                                        
                    
typedef struct
{
    UINT8                      usIndex;
    BOOLEAN                    bFree;            // Whether the POD Module is free/already requested 
    POD_ATTRIBUTES_t           stAttributes;     // Current POD attributes
    pthread_cond_t             stTuneCond;
    pthread_mutex_t            stCondMutex;
    pthread_t                  stThreadID;
    pthread_mutex_t            stPODMutex;
    POD_NOTIFY_FUNC1_t            *card_detection_cb;
    POD_NOTIFY_FUNC1_t           *error_handler_cb;
    POD_NOTIFY_FUNC2_t           *send_pod_poll_cb;
    POD_NOTIFY_FUNC3_t           *data_available_cb;
    POD_NOTIFY_FUNC4_t           *extCh_data_available_cb;
    void*                      pulDataNotify;
    BOOLEAN                    IsPresent;
    DEVICE_HANDLE_t            hPODHandle;
    POD_MODE_t                  cardType;
    UINT8                    num_of_streams;
    UINT16                    data_ch_buffer_size;
    UINT16                    ext_ch_buffer_size;
    
    UINT8                    POD_mfg_string[20];
    void               *hCimax;
    
} POD_t;

/*
typedef struct
{
    BOOLEAN                 bIsInitialized;                 // Whether HAL POD is initialized
    HOST_MODE_t           hostMode;                //Host Type -got from GetAttributes
    POD_CAPABILITIES_t           stCapabilities;
    POD_t                   astPOD[HAL_POD_MAX_PODS]; // Array of POD modules
} POD_BASE_t;
*/



/*-------------------------------------------------------------------
   Global Data Declarations
-------------------------------------------------------------------*/
//extern POD_BASE_t stHalPODBase;

/*===================================================================
   FUNCTION PROTOTYPES
===================================================================*/


/*===================================================================
FUNCTION NAME: HAL_POD_<APIs>
DESCRIPTION:
   HAL POD APIs
DOCUMENTATION:
   "PDT Hardware Abstraction Layer" document
===================================================================*/
INT32 HAL_POD_GetAttributes   (DEVICE_HANDLE_t hTunerHandle, POD_ATTRIBUTES_t *pstPODAttributes);

INT32 HAL_POD_GetCapabilities (POD_CAPABILITIES_t *pstCapabilities);

INT32 HAL_POD_GetInfo         (DEVICE_HANDLE_t hPODHandle, POD_INFO_t *pstInfo);

INT32 HAL_POD_Release         (DEVICE_HANDLE_t hPODHandle);

INT32 HAL_POD_Request         (DEVICE_HANDLE_t hPODHandle);

INT32 HAL_POD_SetAttributes   (DEVICE_HANDLE_t hPODHandle, POD_ATTRIBUTES_t *pstPODAttributes); 

INT32 HAL_POD_SetNotify       (DEVICE_HANDLE_t hPODHandle, void *pfnNotifyFunc, POD_NOTIFY_TYPE_t val, void* pulData);

INT32 HAL_POD_Send_Data       (DEVICE_HANDLE_t hPODHandle, POD_BUFFER * stBufferHandle,  POD_CHANNEL_TYPE_T value, UINT8 *pData);

INT32 HAL_POD_TS_Control      (DEVICE_HANDLE_t hPODHandle, UINT32  *pData);
                        
INT32 HAL_POD_OOB_Control     (DEVICE_HANDLE_t hPODHandle, UINT32  *pData);

INT32 HAL_POD_IF_Control      (DEVICE_HANDLE_t hPODHandle, UINT32  *pData);

INT32 HAL_POD_POLL_ON_SND (DEVICE_HANDLE_t hPODHandle, UINT32  *pData); //POD Interface reset - by setting RS bit

INT32 HAL_POD_INTERFACE_RESET (DEVICE_HANDLE_t hPODHandle, UINT32  *pData); //POD Interface reset - by setting RS bit

INT32 HAL_POD_RESET_PCMCIA    (DEVICE_HANDLE_t hPODHandle, UINT32  *pData); //PCMCIA reset
INT32 HAL_POD_CONFIGURE_CIPHER(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *pStrPtr);
INT32 HAL_POD_INIT(void);//POD Init - should be called fro inside HAL , but being used temporarily

INT32 HomingCardFirmwareUpgradeSetState(INT32 state);
INT32 HomingCardFirmwareUpgradeGetState();
#ifdef __cplusplus
}
#endif

#endif //TUNER_API_H





