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
 * @file rmf_oobService.cpp
 */

#include "rmf_oobService.h"
#include "rmf_oobsimgr.h"

/**
 * @brief This function creates and initialises the rmf_OobSiMgr instance if not already created.
 * As part of rmf_OobSiMgr instance creation, it also creates and initialises the outband SI cache
 * and OOB section filter.
 *
 * @return Returns RMF_SUCCESS if rmf_OobSiMgr is created successfully else returns RMF_OSAL_ENODATA.
 */
rmf_Error OobServiceInit()
{
	rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
	if( NULL == pOobSiMgr  ) return RMF_OSAL_ENODATA;
	return RMF_SUCCESS;				
}

/**
 * @brief This function gets the program information of the specified decimalSrcId from the SI cache.
 * The program information includes frequency, modulation, program number and the
 * service handle.
 *
 * @param[in] decimalSrcId Indicates unique value given to a service, against which the program information
 * needs to be populated.
 * @param[out] pInfo Pointer to populate the program information.
 *
 * @return Return values indicates the success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call is successful and the program information is fetched.
 * @retval RMF_OSAL_EINVAL Indicates the input parameter pInfo passed is NULL.
 * @retval RMF_SI_INVALID_PARAMETER Indicates invalid parameters passed to the functions which are called
 * internally.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 * @retval RMF_SI_NOT_FOUND Indicates any of the following condition.
 * <li> Indicates the entry corresponding to decimalSrcId is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT.
 * <li> The TS handle corresponding to decimalSrcId is invalid.
 */
rmf_Error OobGetProgramInfo(uint32_t decimalSrcId, Oob_ProgramInfo_t *pInfo)
{
	rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
	return pOobSiMgr->GetProgramInfo(decimalSrcId, pInfo);
}

/**
 * @brief This function gets the program information of the specified vcn from the SI cache.
 * The program information includes frequency, modulation, program number and the
 * service handle.
 *
 * @param[in] vcn Indicates the virtual channel number of a service, against which the program information
 * needs to be populated.
 * @param[out] pInfo Pointer to populate the program information.
 *
 * @return Return values indicates the success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call is successful and the program information is fetched.
 * @retval RMF_OSAL_EINVAL Indicates the input parameter pInfo passed is NULL.
 * @retval RMF_SI_INVALID_PARAMETER Indicates invalid parameters passed to the functions which are called
 * internally.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 * @retval RMF_SI_NOT_FOUND Indicates any of the following condition.
 * <li> Indicates the entry corresponding to decimalSrcId is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT.
 * <li> The TS handle corresponding to decimalSrcId is invalid.
 */
rmf_Error OobGetProgramInfoByVCN(uint32_t vcn, Oob_ProgramInfo_t *pInfo)
{
	rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
	return pOobSiMgr->GetProgramInfoByVCN(vcn, pInfo);
}

/**
 * @brief This function gets the DAC ID.
 *
 * @param[out] pDACId Pointer to populate the DAC ID.
 *
 * @return  Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the virtual channel number is populated and the call is successful.
 * @retval RMF_OSAL_EINVAL Indicates, the parameter pDACId passed is pointing to NULL.
 * @retval RMF_OSAL_ENODATA Indicates any of the below conditions.
 * <li> Indicates the entry corresponding to DAC_SOURCEID is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT and the SI is not up.
 * @retval RMF_SI_INVALID_PARAMETER Indicates the parameters passed to internal function calls are
 * invalid.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 */
rmf_Error OobGetDacId(uint16_t *pDACId)
{
	rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
	return pOobSiMgr->GetDACId(pDACId);
}

/**
 * @brief This function gets the channel map ID.
 *
 * @param[out] pChannelMapId Pointer to populate the channel map ID.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the virtual channel number is populated and the call is successful.
 * @retval RMF_OSAL_EINVAL Indicates, the parameter pChannelMapId passed is pointing to NULL.
 * @retval RMF_OSAL_ENODATA Indicates any of the below condition.
 * <li> Indicates the entry corresponding to sourceId is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT and SI is not up.
 * @retval RMF_SI_INVALID_PARAMETER Indicates the parameters passed to internal function calls are
 * invalid.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 */
rmf_Error OobGetChannelMapId(uint16_t *pChannelMapId)
{
	rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
	return pOobSiMgr->GetChannelMapId(pChannelMapId);
}

/**
 * @brief This function gets the virtual channel number corresponding to the specified decimalSrcId by using
 * rmf_OobSiMgr instance.
 *
 * @param[in] decimalSrcId Indicates unique value given to a service against which the virtual channel
 * number is required.
 * @param[out] pVCN Pointer to populate the virtual channel number.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the virtual channel number against the sourceId is foundC_SOURCEID.
 * @retval RMF_OSAL_EINVAL Indicates, the parameter pVCN passed is pointing to NULL.
 * @retval RMF_SI_INVALID_PARAMETER Indicate parameters passed to functions which are called
 * internally are invalid.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 * @retval RMF_OSAL_ENODATA Indicates any of the following condition.
 * <li> Indicates the entry corresponding to sourceId is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT.
 */
rmf_Error OobGetVirtualChannelNumber(uint32_t decimalSrcId, uint16_t *pVCN)
{
        rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
        return pOobSiMgr->GetVirtualChannelNumber(decimalSrcId, pVCN);
}

#ifdef ENABLE_TIME_UPDATE

/**
 * @brief This function gets the System Time Table acquired time in milliseconds only if the STT is already acquired. It uses the
 * rmf_OobSiMgr instance to get the STT acquired time.
 *
 * @param[out] pSysTime Pointer to populate the current time in milliseconds.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the current time is populated and the call is successful.
 * @retval RMF_OSAL_EINVAL Indicates the parameter pSysTime passed, points to NULL.
 * @retval RMF_OSAL_ENODATA Indicates STT is not acquired.
 */
rmf_Error OobGetSysTime(rmf_osal_TimeMillis *pSysTime)
{
        rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
	return pOobSiMgr->GetSysTime(pSysTime);
}

/**
 * @brief This function gets the time zone in hours and the day light saving information.
 * Time zone values can range from -12 to +13 hours.
 *
 * @param[out] pTimezoneinhours Pointer to populate time zone in hours.
 * @param[out] pDaylightflag Pointer to daylight flag. 0 indicates, not in daylight saving time and 1 indicates,
 * in daylight saving time.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call was successful.
 * @retval RMF_OSAL_EINVAL Indicates any one or both of the input parameters are pointing to NULL.
 * @retval RMF_OSAL_ENODATA Indicates STT is not acquired.
 */
rmf_Error OobGetTimeZone(int32_t *pTimezoneinhours, int32_t *pDaylightflag)
{
        rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
	return pOobSiMgr->GetTimeZone(pTimezoneinhours, pDaylightflag);
}
#endif

