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



#ifndef _VL_HOST_GENERIC_FEATURES_H_
#define _VL_HOST_GENERIC_FEATURES_H_

#include "pod_types.h"

typedef enum _VL_HOST_GENERIC_FEATURES_RESULT
{
    VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS,
    VL_HOST_GENERIC_FEATURES_RESULT_NOT_IMPLEMENTED,
    VL_HOST_GENERIC_FEATURES_RESULT_NULL_PARAM,
    VL_HOST_GENERIC_FEATURES_RESULT_ERROR,

    VL_HOST_GENERIC_FEATURES_RESULT_INVALID

}VL_HOST_GENERIC_FEATURES_RESULT;

#ifdef __cplusplus
extern "C" {
#endif
VL_HOST_GENERIC_FEATURES_RESULT vlPodGetGenericFeatureListWithVer(unsigned char Ver,VL_POD_HOST_FEATURE_ID_LIST * pList);
VL_HOST_GENERIC_FEATURES_RESULT vlPodGetGenericFeatureList(VL_POD_HOST_FEATURE_ID_LIST * pStruct);
VL_HOST_GENERIC_FEATURES_RESULT vlPodGetGenericFeatureData(VL_HOST_FEATURE_PARAM eFeature, void * _pvStruct);
VL_HOST_GENERIC_FEATURES_RESULT vlPodSetGenericFeatureData(VL_HOST_FEATURE_PARAM eFeature, void * _pvStruct, bool sendEventToJava);
VL_HOST_GENERIC_FEATURES_RESULT vlPodGetGenericFeatureHexData(VL_HOST_FEATURE_PARAM eFeature, unsigned char * pData, int * pnBytes, int nByteCapacity);
VL_HOST_GENERIC_FEATURES_RESULT vlPodSetGenericFeatureHexData(VL_HOST_FEATURE_PARAM eFeature, const unsigned char * pData, int nBytes, bool sendEventToJava);
const char * vlPodGetTimeZoneString();
void vlPodSetTimeZoneString();
    
#ifdef __cplusplus
}
#endif

#endif // _VL_HOST_GENERIC_FEATURES_H_
