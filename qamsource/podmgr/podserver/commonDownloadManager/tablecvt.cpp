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


#include "tablecvt.h"
#include "tablecvt_t1v1.h"
#include "tablecvt_t2v1.h"
#include "bits_cdl.h"
#include <string.h>
#include <stdlib.h>
#include <rdk_debug.h>

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif

//#include <vlMemCpy.h>
TableCVT::TableCVT(){
this->type = 0;
this->version = 0;
}
TableCVT::~TableCVT(){
}

TableCVT* TableCVT::generateInstance(void *cvt, unsigned long len)
{
  TableCVT *out = NULL;
  unsigned int cvt_tag;
  
  SimpleBitstream b((unsigned char*)cvt, len);

  cvt_tag = b.get_bits(24);

  switch(cvt_tag)
  {
    case CVT_T1V1:
      out = new TableCVT_T1V1(cvt, len);
      break;
    case CVT_T2Vx:
      out = new TableCVT_T2Vx(cvt, len);
      break;

    default:
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," TableCVT::generateInstance() data passed in not of a known CVT type\n");
      break;
  }

  return out;
}

cvtdownload_data_t* TableCVT::getCVT_Data()
{
  cvtdownload_data_t* data;
                                                              
  data = (cvtdownload_data_t*)malloc(sizeof(cvtdownload_data_t));
  memcpy(data, &cvt_data, sizeof(cvtdownload_data_t));

  return data;
}

cvtdownload_data_t* TableCVT::getCVT_DataForUpdate()
{
  //cvtdownload_data_t* data;
                                                              
  //data = (cvtdownload_data_t*)malloc(sizeof(cvtdownload_data_t));
  //memcpy(data, &cvt_data, sizeof(cvtdownload_data_t));

  return &cvt_data;
}
