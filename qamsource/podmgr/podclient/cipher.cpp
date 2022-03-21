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
#ifdef USE_POD
#ifndef NEXUS_PLATFORM
/*-------------------------------------------------------------------
   Include Files
-------------------------------------------------------------------*/
#include <cipher.h>
#include <stdio.h>
#include "ismd_demux.h"
#include "sys_init.h" 

#include "rmfqamsrc.h"
#include "rmf_osal_util.h"
#include "rmf_osal_thread.h"
#include "rdk_debug.h"
#include "ismd_global_defs.h"
#include "sec.h"


#ifdef MPOD_SUPPORT

int get_bool_env(int result, const char *key)
{
    const char *value = rmf_osal_envGet(key);

	if (value != NULL)
	{
        result = (strcasecmp(value, "TRUE") == 0);
	}

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: key=%s, result=%d", __FUNCTION__, key, result);

    return result;
}
int configureCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *ptr)
{
	int i;
	ismd_dev_handle_t	demux_handle;
    ismd_demux_crypto_info_t key_data;
    ismd_demux_crypto_info_t iv;
    bool  backDoorLoading = FALSE; // Figure out how to determine the new SKU
    backDoorLoading = get_bool_env(false, "BACKDOOR.KEY.LOADING");

   RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
		"%s : Starts \n", __FUNCTION__ );
   
    memset(&iv, 0, sizeof(iv));
    if(backDoorLoading)
    {

        //0x0123456789ABCDEF0123456789ABCDEF  -- ORIGINAL KEY
        //0x89ABCDEF 0x01234567 0x89ABCDEF 0x01234567 -- BACK-DOOR 
	key_data.crypto_info[0] = (DesKeyAHi >> 24) & 0xFF;
	key_data.crypto_info[1] = (DesKeyAHi >> 16) & 0xFF;
	key_data.crypto_info[2] = (DesKeyAHi >>  8) & 0xFF;
	key_data.crypto_info[3] = (DesKeyAHi >>  0) & 0xFF;
	
	key_data.crypto_info[4] = (DesKeyALo >> 24) & 0xFF;
	key_data.crypto_info[5] = (DesKeyALo >> 16) & 0xFF;
	key_data.crypto_info[6] = (DesKeyALo >>  8) & 0xFF;
	key_data.crypto_info[7] = (DesKeyALo >>  0) & 0xFF;

    /* Odd video */

	key_data.crypto_info[8] = (DesKeyBHi >> 24) & 0xFF; 
	key_data.crypto_info[9] = (DesKeyBHi >> 16) & 0xFF;
	key_data.crypto_info[10] = (DesKeyBHi >> 8) & 0xFF;
	key_data.crypto_info[11] = (DesKeyBHi >> 0) & 0xFF;
	
	key_data.crypto_info[12] = (DesKeyBLo >> 24) & 0xFF;
	key_data.crypto_info[13] = (DesKeyBLo >> 16) & 0xFF;
	key_data.crypto_info[14] = (DesKeyBLo >>  8) & 0xFF;
	key_data.crypto_info[15] = (DesKeyBLo >>  0) & 0xFF;
    }
    else 
    {
	key_data.crypto_info[7] = (DesKeyAHi >> 24) & 0xFF;
    key_data.crypto_info[6] = (DesKeyAHi >> 16) & 0xFF;
    key_data.crypto_info[5] = (DesKeyAHi >>  8) & 0xFF;
    key_data.crypto_info[4] = (DesKeyAHi >>  0) & 0xFF;
	
    key_data.crypto_info[3] = (DesKeyALo >> 24) & 0xFF;
    key_data.crypto_info[2] = (DesKeyALo >> 16) & 0xFF;
    key_data.crypto_info[1] = (DesKeyALo >>  8) & 0xFF;
    key_data.crypto_info[0] = (DesKeyALo >>  0) & 0xFF;

    /* Odd video */

	key_data.crypto_info[15] = (DesKeyBHi >> 24) & 0xFF; 
	key_data.crypto_info[14] = (DesKeyBHi >> 16) & 0xFF;
	key_data.crypto_info[13] = (DesKeyBHi >> 8) & 0xFF;
	key_data.crypto_info[12] = (DesKeyBHi >> 0) & 0xFF;
	
	key_data.crypto_info[11] = (DesKeyBLo >> 24) & 0xFF;
	key_data.crypto_info[10] = (DesKeyBLo >> 16) & 0xFF;
	key_data.crypto_info[9] = (DesKeyBLo >>  8) & 0xFF;
	key_data.crypto_info[8] = (DesKeyBLo >>  0) & 0xFF;
    }
      
    
    if(!get_bool_env(false, "FEATURE.CIPHER.SYNC"))
    {

       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Waiting for PID Filter configuration \n", __FUNCTION__);
       rmf_osal_threadSleep(500, 0);  
    }

  rmf_transport_path_info txPathInfo;
  if(rmf_qamsrc_getTransportPathInfoForLTSID(ltsid,  &txPathInfo) != 0)
  {
      RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.POD", "%s: FAILED: Could not get the Transport Path Info for LTSID %d <<<<<<<<<<<< \n", __FUNCTION__, ltsid);
      return -1;
  }
  if(!backDoorLoading)
  {
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Using front door key loaing FFFFFF \n", __FUNCTION__);

    for(i=0; i < numpids;i++)
    {
	ismd_result_t ret;

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " >>>>>>>>>>>> calling ismd_demux_ts_load_key() Even <<<<<<<<<<<< Pid:0x%x\n",decodePid[i]);
	ret = ismd_demux_ts_load_key(txPathInfo.demux_handle, decodePid[i], ISMD_DEMUX_3DES_DECRYPT_ECB, ISMD_DEMUX_TEXT_STEALING_RESIDUAL, ISMD_DEMUX_KEY_TYPE_EVEN, key_data, 16, iv, 0);
        if(ret != ISMD_SUCCESS)
        {
          RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.POD", " ismd_demux_ts_load_key returned ERROR (Error Code %d) while loading KEY_TYPE_EVEN >>>>>>>>>>> \n",ret );
        }
        else
        {
          //LOGINFO("LOG.RDK.POD", " ismd_demux_ts_load_key returned SUCCESS while loading KEY_TYPE_EVEN >>>>>>>>>>> \n");
        }

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " >>>>>>>>>>>> calling ismd_demux_ts_load_key() Odd <<<<<<<<<<<< \n");
	ret = ismd_demux_ts_load_key(txPathInfo.demux_handle, decodePid[i], ISMD_DEMUX_3DES_DECRYPT_ECB, ISMD_DEMUX_TEXT_STEALING_RESIDUAL, ISMD_DEMUX_KEY_TYPE_ODD, key_data, 16, iv, 0);
        if(ret != ISMD_SUCCESS)
        {
          RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.POD", " ismd_demux_ts_load_key returned ERROR (Error Code %d) while loading KEY_TYPE_ODD >>>>>>>>>>> \n",ret );
        }
        else
        {
          //LOGINFO("LOG.RDK.POD", " ismd_demux_ts_load_key returned SUCCESS while loading KEY_TYPE_ODD >>>>>>>>>>> \n");
        }
    }

  }
  else
  {
    //SRINI. fill it up.
//Back door
						
/*
MAPPING OF SEC SLOTS TO tsd SLOTS:

SEC SLOT	TSD SLOT
CW1			0
CW2			1
CW3			2
CW4			3
CW5			4
CW6			5
CW7			6
CW8			7
CCW1		8
CCW2		9
CCW3		10
CCW4		11
*/
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: Using back door key loaing BBBBBB \n", __FUNCTION__);

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s ltsid=%d context = %04X  TSID = %02X demux_handle = 0x%08X\n",
		  __FUNCTION__, 
		  ltsid,
		  txPathInfo.hSecContextFor3DES, 
		  txPathInfo.tsd_key_location,
		  txPathInfo.demux_handle
 		);
    if(txPathInfo.hSecContextFor3DES == 0xFF)
    {
	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s context = 0xFF RETURNING!!!\n");
	return -1;
    }
    for(i=0; i < numpids;i++)
    {
	ismd_result_t ret;

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " >>>>>>>>>>>> calling ismd_demux_ts_load_key() Even <<<<<<<<<<<< Pid:0x%x\n",decodePid[i]);
	ret = ismd_demux_ts_set_crypto_params( txPathInfo.demux_handle,						//Handle
					decodePid[i],						//PID
					ISMD_DEMUX_3DES_DECRYPT_ECB,		//algo type
					ISMD_DEMUX_TEXT_STEALING_RESIDUAL,	//redisual type
					iv, 								//iv
					16);								//key length
	ret = (ismd_result_t)sec_set_internal_key(txPathInfo.hSecContextFor3DES,	// contexdt,
				&key_data,  	// key data,
				16,         // key length bytes
				SEC_CCW1,   // sec_key_cache_slot_t
				txPathInfo.tsd_key_location,		    // TSD Slot
				SEC_TSD_KEYATTR_3DES);	//The cipher to assign to the key in the TSD key store location
	
	ismd_demux_key_params_t key_params;
	memset (&key_params, 0x0, sizeof(ismd_demux_key_params_t));

	key_params.key_load_mode        = ISMD_DEMUX_KEY_LOAD_INDIRECT;
	key_params.odd_or_even          = ISMD_DEMUX_KEY_TYPE_ODD;
	key_params.indirect.key_src     = ISMD_DEMUX_SEC_KEY_SRC_FW;                                                               
	key_params.indirect.key_type    = ISMD_DEMUX_SEC_CPCW_KEYS;
	key_params.indirect.key_decode  = ISMD_DEMUX_KEY_DECODE_SEC;
	key_params.indirect.key_sel_sec = txPathInfo.tsd_key_location; //TSD Slot
	
	ret = ismd_demux_descramble_set_key(txPathInfo.demux_handle,
					    decodePid[i],
					    &key_params);
	
        if(ret != ISMD_SUCCESS)
        {
          RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.POD", " ismd_demux_descramble_set_key returned ERROR (Error Code %d) while loading KEY_TYPE_EVEN, demux_handle = %x,tsd_key_location= %d  >>>>>>>>>>> \n",ret, txPathInfo.demux_handle,  txPathInfo.tsd_key_location);
        }
        else
        {
          RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " ismd_demux_descramble_set_key returned SUCCESS while loading KEY_TYPE_EVEN >>>>>>>>>>> \n");
        }

	key_params.odd_or_even          = ISMD_DEMUX_KEY_TYPE_EVEN;
	ret = ismd_demux_descramble_set_key(txPathInfo.demux_handle,
					    decodePid[i],
					    &key_params);

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " >>>>>>>>>>>> calling ismd_demux_ts_load_key() Odd <<<<<<<<<<<< \n");
        if(ret != ISMD_SUCCESS)
        {
          RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.POD", " ismd_demux_descramble_set_key returned ERROR (Error Code %d) while loading KEY_TYPE_ODD >>>>>>>>>>> \n",ret );
        }
        else
        {
          RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",  " ismd_demux_descramble_set_key returned SUCCESS while loading KEY_TYPE_ODD >>>>>>>>>>> \n");
        }

	}

  }
  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
		"%s : Finished successfully \n", __FUNCTION__ );
  return 0;	

}
#endif // MPOD_SUPPORT
//#else
//#error "USE_POD is not defined.."


#else
#include <cipher.h>

int configureCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *ptr)
{ return 0;}
#endif

#endif // USE_POD
