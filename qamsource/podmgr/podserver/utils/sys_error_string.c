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




#include "hal_api.h"
#include "hutils_sys.h"
#include "hal_error.h"

/*----------------------------------------------------------------------------
 * Convert an Error Code to a string.
 *----------------------------------------------------------------------------*/

char *SYS_Error_String (int lErrorCode)
{
    char *szStr = "ERROR_HAL_DEFAULT";

    switch (lErrorCode)
    {
        case HAL_SUCCESS:
            szStr = "HAL_SUCCESS";
            break;

        case ERROR_HAL_GENERAL:
            szStr = "ERROR_HAL_GENERAL";
            break;
        case ERROR_HAL_UNKNOWN:
            szStr = "ERROR_HAL_UNKNOWN";
            break;
        case ERROR_HAL_NOT_SUPPORTED:
            szStr = "ERROR_HAL_NOT_SUPPORTED";
            break;
        case ERROR_HAL_NOT_INITIALIZED:
            szStr = "ERROR_HAL_NOT_INITIALIZED";
            break;
        case ERROR_HAL_NOT_FOUND:
            szStr = "ERROR_HAL_NOT_FOUND";
            break;
        case ERROR_HAL_NO_HARDWARE:
            szStr = "ERROR_HAL_NO_HARDWARE";
            break;
        case ERROR_HAL_OUT_OF_MEMORY:
            szStr = "ERROR_HAL_OUT_OF_MEMORY";
            break;
        case ERROR_HAL_TIMEOUT:
            szStr = "ERROR_HAL_TIMEOUT";
            break;
        case ERROR_HAL_NULL_PARAMETER:
            szStr = "ERROR_HAL_NULL_PARAMETER";
            break;
        case ERROR_HAL_INVALID_HANDLE:
            szStr = "ERROR_HAL_INVALID_HANDLE";
            break;
        case ERROR_HAL_DEVICE_NOT_AVAILABLE:
            szStr = "ERROR_HAL_DEVICE_NOT_AVAILABLE";
            break;
        case ERROR_HAL_SOURCE_NOT_AVAILABLE:
            szStr = "ERROR_HAL_SOURCE_NOT_AVAILABLE";
            break;
        case ERROR_HAL_NOT_REQUESTED:
            szStr = "ERROR_HAL_NOT_REQUESTED";
            break;
        case ERROR_HAL_INVALID_SOURCE_DEVICE:
            szStr = "ERROR_HAL_INVALID_SOURCE_DEVICE";
            break;
        case ERROR_HAL_DEVICE_CONNECTED:
            szStr = "ERROR_HAL_DEVICE_CONNECTED";
            break;
        case ERROR_HAL_NO_SOURCE:
            szStr = "ERROR_HAL_NO_SOURCE";
            break;
        case ERROR_HAL_STOPPED:
            szStr = "ERROR_HAL_STOPPED";
            break;
        case ERROR_HAL_STARTED:
            szStr = "ERROR_HAL_STARTED";
            break;
        case ERROR_HAL_NO_DATA:
            szStr = "ERROR_HAL_NO_DATA";
            break;
        case ERROR_HAL_INITIALIZATION_FAILED:
            szStr = "ERROR_HAL_INITIALIZATION_FAILED";
            break;
        case ERROR_HAL_ALREADY_INITIALIZED:
            szStr = "ERROR_HAL_ALREADY_INITIALIZED";
            break;
        case ERROR_HAL_POST_FAILED:
            szStr = "ERROR_HAL_POST_FAILED";
            break;
        case ERROR_HAL_DEVICE_NOT_CONFIGURED:
            szStr = "ERROR_HAL_DEVICE_NOT_CONFIGURED";
            break;
        case ERROR_HAL_INVALID_PID:
            szStr = "ERROR_HAL_INVALID_PID";
            break;
        case ERROR_HAL_PID_CHAN_ALLOC_FAILED:
            szStr = "ERROR_HAL_PID_CHAN_ALLOC_FAILED";
            break;
        case ERROR_HAL_NO_MASK:
            szStr = "ERROR_HAL_NO_MASK";
            break;
        case ERROR_HAL_SEMAPHORED:
            szStr = "ERROR_HAL_SEMAPHORED";
            break;
        case ERROR_HAL_OPERATION_FAILED:
            szStr = "ERROR_HAL_OPERATION_FAILED";
            break;
        case ERROR_HAL_OUT_OF_RANGE:
            szStr = "ERROR_HAL_OUT_OF_RANGE";
            break;

        case ERROR_HAL_OS_GENERAL:
            szStr = "ERROR_HAL_OS_GENERAL";
            break;
        case ERROR_HAL_OS_SEM_CREATE_FAILED:
            szStr = "ERROR_HAL_OS_SEM_CREATE_FAILED";
            break;
        case ERROR_HAL_OS_SEM_DESTROY_FAILED:
            szStr = "ERROR_HAL_OS_SEM_DESTROY_FAILED";
            break;
        case ERROR_HAL_OS_SEM_GET_FAILED:
            szStr = "ERROR_HAL_OS_SEM_GET_FAILED";
            break;
        case ERROR_HAL_OS_SEM_RELEASE_FAILED:
            szStr = "ERROR_HAL_OS_SEM_RELEASE_FAILED";
            break;

        case ERROR_ANAAUD_GENERAL:
            szStr = "ERROR_ANAAUD_GENERAL";
            break;

        case ERROR_ANAVID_GENERAL:
            szStr = "ERROR_ANAVID_GENERAL";
        break;

        case ERROR_CRYPTO_GENERAL:
            szStr = "ERROR_CRYPTO_GENERAL";
            break;

        case ERROR_DISP_GENERAL:
            szStr = "ERROR_DISP_GENERAL";
            break;
        case ERROR_DISP_FORMAT_NOT_SUPPORTED :
            szStr = "ERROR_DISP_FORMAT_NOT_SUPPORTED";
            break;
        case ERROR_DISP_ASPECT_RATIO_NOT_SUPPORTED :
            szStr = "ERROR_DISP_ASPECT_RATIO_NOT_SUPPORTED";
            break;
        case ERROR_DISP_INVALID_VIDEO_PORT :
            szStr = "ERROR_DISP_INVALID_VIDEO_PORT";
            break;
        case ERROR_DISP_INVALID_AUDIO_PORT :
            szStr = "ERROR_DISP_INVALID_AUDIO_PORT";
            break;
        case ERROR_DISP_COMPOSITE_NOT_AVAILABLE :
            szStr = "ERROR_DISP_COMPOSITE_NOT_AVAILABLE";
            break;
        case ERROR_DISP_WINDOW_NOT_AVAILABLE :
            szStr = "ERROR_DISP_WINDOW_NOT_AVAILABLE";
            break;

        case ERROR_EEPROM_GENERAL:
            szStr = "ERROR_EEPROM_GENERAL";
            break;
        case ERROR_EEPROM_INVALID_OFFSET:
            szStr = "ERROR_EEPROM_INVALID_OFFSET";
            break;

        case ERROR_FPD_GENERAL:
            szStr = "ERROR_FPD_GENERAL";
            break;
        case ERROR_FPD_INVALID_TIME:
            szStr = "ERROR_FPD_INVALID_TIME";
            break;
        case ERROR_FPD_INVALID_CHARS:
            szStr = "ERROR_FPD_INVALID_CHARS";
            break;

        case ERROR_GFX_GENERAL:
            szStr = "ERROR_GFX_GENERAL";
            break;
        case ERROR_GFX_PIXELFORMAT_NOT_SUPPORTED:
            szStr = "ERROR_GFX_PIXELFORMAT_NOT_SUPPORTED";
            break;
        case ERROR_GFX_OUT_OF_SURFACES:
            szStr = "ERROR_GFX_OUT_OF_SURFACES";
            break;
        case ERROR_GFX_NOT_CONNECTED_TO_DISPLAY:
            szStr = "ERROR_GFX_NOT_CONNECTED_TO_DISPLAY";
            break;
        case ERROR_GFX_OUT_OF_PALETTES:
            szStr = "ERROR_GFX_OUT_OF_PALETTES";
            break;

        case ERROR_HID_GENERAL:
            szStr = "ERROR_HID_GENERAL";
            break;

        case ERROR_IRB_GENERAL:
            szStr = "ERROR_IRB_GENERAL";
            break;

        case ERROR_MPAUD_GENERAL:
            szStr = "ERROR_MPAUD_GENERAL";
            break;
        case ERROR_MPAUD_AUDIO_NOT_SUPPORTED:
            szStr = "ERROR_MPAUD_AUDIO_NOT_SUPPORTED";
            break;

        case ERROR_MPENC_GENERAL:
            szStr = "ERROR_MPENC_GENERAL";
            break;

        case ERROR_MPVID_GENERAL:
            szStr = "ERROR_MPVID_GENERAL";
            break;

        case ERROR_OOB_GENERAL:
            szStr = "ERROR_OOB_GENERAL";
            break;

        case ERROR_PCM_GENERAL:
            szStr = "ERROR_PCM_GENERAL";
            break;

        case ERROR_POD_GENERAL:
            szStr = "ERROR_POD_GENERAL";
            break;

        case ERROR_RECODE_GENERAL:
            szStr = "ERROR_RECODE_GENERAL";
            break;

        case ERROR_SECTFLT_GENERAL:
            szStr = "ERROR_SECTFLT_GENERAL";
            break;

        case ERROR_SYS_GENERAL:
            szStr = "ERROR_SYS_GENERAL";
            break;

        case ERROR_TUNER_GENERAL:
            szStr = "ERROR_TUNER_GENERAL";
            break;
        case ERROR_TUNER_FREQ_OUT_OF_RANGE:
            szStr = "ERROR_TUNER_FREQ_OUT_OF_RANGE";
            break;
        case ERROR_TUNER_SR_OUT_OF_RANGE:
            szStr = "ERROR_TUNER_SR_OUT_OF_RANGE";
            break;
        case ERROR_TUNER_MODE_NOT_SUPPORTED:
            szStr = "ERROR_TUNER_MODE_NOT_SUPPORTED";
            break;

        /* VBI errors */
        case ERROR_VBI_INVALID_LINE:
            szStr = "ERROR_VBI_INVALID_LINE";
            break;
        case ERROR_VBI_INVALID_FIELD:
            szStr = "ERROR_VBI_INVALID_FIELD";
            break;
        case ERROR_VBI_INVALID_DATA_TYPE:
            szStr = "ERROR_VBI_INVALID_DATA_TYPE";
            break;

        /* Internal Errors */
        case ERROR_INTERNAL_GENERAL:
            szStr = "ERROR_INTERNAL_GENERAL";
            break;
        case ERROR_INVALID_DESTINATION:
            szStr = "ERROR_INVALID_DESTINATION";
            break;
        case ERROR_PID_NOT_IN_USE:
            szStr = "ERROR_PID_NOT_IN_USE";
            break;
        case ERROR_DEVICE_STILL_ACTIVE:
            szStr = "ERROR_DEVICE_STILL_ACTIVE";
            break;
        case ERROR_DEVICE_NOT_CONNECTED:
            szStr = "ERROR_DEVICE_NOT_CONNECTED";
            break;

        default:
            szStr = "Invalid Error Code";
            break;
    }
    return(szStr);
}



/* End of file */
