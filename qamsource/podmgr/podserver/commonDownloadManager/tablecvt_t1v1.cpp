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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tablecvt_t1v1.h"
#include "cvtdownloadtypes.h"
#include "bits_cdl.h"
#include <rdk_debug.h>

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

//#include <vlMemCpy.h>
TableCVT_T1V1::TableCVT_T1V1(void *buf, unsigned long in_len){
  cvt = buf;
  len = in_len;

  bzero(&cvt_data, sizeof(cvtdownload_data_t));
  type = CVT_T1V1;

  parseCVT();
}

TableCVT_T1V1::~TableCVT_T1V1(){
}

void TableCVT_T1V1::parseCVT()
{
  int size_indicator;
  int length_value;
  int length_field_size;
  int num_descriptors;
  int des_tag, des_len;
  int cvc_start_bit;
  int cvc_len_bits;
  int cvc_len_bytes;
  int codefile_name_len;
  char codefile_name[256];

  cvt_t1v1_download_data_t* cvtdata = (cvt_t1v1_download_data_t*)&cvt_data;
  
  SimpleBitstream b((unsigned char*)cvt, len);

  b.skip_bits(24);  // skip the CVT tag as it was already looked at before.

  // length_field()
  size_indicator = b.get_bits(1);
  if(size_indicator == 0)
  {
    length_value = b.get_bits(7);
  }
  else // size_indicator == 1
  {
    length_field_size = b.get_bits(7);
    length_value = b.get_bits(length_field_size*8);
  }

  num_descriptors = b.get_bits(8);
  for(int i = 0; i < num_descriptors; i++)
  {
    des_tag = b.get_bits(8);
    des_len = b.get_bits(8);

    switch(des_tag)
    {
      case VENDOR_ID:
        cvtdata->vendorId = b.get_bits(des_len*8); // throw exception if des_len != 3
        break;
      case HARDWARE_VERSION_ID:
        // throw exception if des_len !=4
        cvtdata->hardwareVersionId = b.get_bits(des_len*8);
        break;
      case HOST_PROPRIETARY_DATA:
        // throw exception if des_len > 16
        cvtdata->hostPropData.len = des_len;
        for(int j = 0; j < des_len; j++)
        {
          cvtdata->hostPropData.data[j] = (unsigned char)b.get_bits(8);
        }
        break;
      default:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","TABLECVT_T1V1::parseCVT()  Unknown descriptor type of [0x%X]\n", des_tag);
        b.skip_bits(des_len*8);
        break;
    }
  }

  cvtdata->downloadtype       = b.get_bits(4);
  cvtdata->download_command   = b.get_bits(4);
  cvtdata->freq_vector        = b.get_bits(16);
  cvtdata->transport_value    = b.get_bits(8);
                                b.skip_bits(3); // reserved
  cvtdata->PID                = b.get_bits(13);
  codefile_name_len           = b.get_bits(8);
  
  for(int k = 0; k < codefile_name_len; k++)
  {
    codefile_name[k] = (char)b.get_bits(8);
  }

  unsigned char* name;
  name = (unsigned char*)malloc(codefile_name_len + 1);
 
  memcpy(name, codefile_name, codefile_name_len);
  cvtdata->codefile_data.len = codefile_name_len;
  cvtdata->codefile_data.name = name;
  name[codefile_name_len] = 0;

  cvc_start_bit = b.get_pos();
  cvc_len_bits = (len * 8) - cvc_start_bit;

  cvc_len_bytes = cvc_len_bits / 8;

  // check if there is a remainder, and add to the bytes size if so.
  if((cvc_len_bits % 8))
  {
    cvc_len_bytes++;
  }

  int bit_len;
  cvtdata->cvc_data.data = (unsigned char*)malloc(cvc_len_bytes);
  for(int l = 0; l < cvc_len_bytes; l++)
  {
    if(l*8 > cvc_len_bits)
    {
      bit_len = cvc_len_bits - ((l-1)*8);
    }
    else
    {
      bit_len = 8;
    }

    cvtdata->cvc_data.data[l] = b.get_bits(bit_len);
    cvtdata->cvc_data.len = cvc_len_bytes;
  }
}


int TableCVT_T1V1::filterCVT(cvtfilterdata_t *data)
{
  int ret = false;

  cvt_t1v1_filterdata_t* fltData = (cvt_t1v1_filterdata_t *)data;
  cvt_t1v1_download_data_t* cvtdata = (cvt_t1v1_download_data_t*)&cvt_data;

  if((fltData->vendorId == cvtdata->vendorId) &&
     (fltData->hardwareVersionId == cvtdata->hardwareVersionId))
  {
    ret = true;
  }

  return ret;
}
