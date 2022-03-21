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


#ifndef TABLECVT_H
#define TABLECVT_H


/**
  *@author Eric S. Allen
  */

#include "cvtdownloadtypes.h"
#include "cvtfilterdata.h"
  
#define CVT_T1V1 0x9F9C02
#define CVT_T2Vx 0x9F9C05

// Descriptor types
#define VENDOR_ID             0x00
#define HARDWARE_VERSION_ID   0x01
#define HOST_PROPRIETARY_DATA 0x02    // CVT_T1Vx only
#define HOST_MAC_ADDR         0x02    // CVT_T2Vx only
#define HOST_ID               0x03    // CVT_T2Vx only

    
class TableCVT {
protected: 
    TableCVT();

  int type;
  int version;
  cvtdownload_data_t cvt_data;

public:
  virtual ~TableCVT();

  int getCVT_Type() { return type; };
  int getCVT_Version() { return version; }; 
  cvtdownload_data_t* getCVT_Data();
  cvtdownload_data_t* getCVT_DataForUpdate();
  virtual int filterCVT(cvtfilterdata_t *data) {return false;};
  
  static TableCVT* generateInstance(void *cvt, unsigned long len);

};

#endif
