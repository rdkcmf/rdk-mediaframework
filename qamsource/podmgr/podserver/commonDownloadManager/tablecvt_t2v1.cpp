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


#include <stdlib.h>
#include <string.h>
#include "tablecvt_t2v1.h"
#include "bits_cdl.h"
#include <rdk_debug.h>

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

//#include <vlMemCpy.h>
TableCVT_T2Vx::TableCVT_T2Vx(void *buf, unsigned long in_len){
 int size_indicator;
int length_value;
int length_field_size;    

  cvt = buf;
  len = in_len;

  bzero(&cvt_data, sizeof(cvtdownload_data_t));
  type = CVT_T2Vx;


   SimpleBitstream b((unsigned char*)buf, len);

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

  version = b.get_bits(8); // not usable until T2V2 comes to existance
 if(version ==  1)
 {
  parseCVTVer1();
 }
 else if(version ==  2)
 {
  parseCVTVer2();
 }
 else
 {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," TableCVT_T2Vx:  Error Version:%d is not correct \n",version);
 }
}

TableCVT_T2Vx::~TableCVT_T2Vx(){
}


void TableCVT_T2Vx::parseCVTVer1()
{
  int size_indicator;
  int length_value;
  int length_field_size;
  int num_descriptors;
  int des_tag, des_len;
  int codefile_name_len;
  char codefile_name[256];
  int cvc_start_bit;
  int cvc_len_bits;
  int cvc_len_bytes;
  
  cvt_t2v1_download_data_t* cvtdata = (cvt_t2v1_download_data_t*)&cvt_data;

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

  cvtdata->protocol_version = b.get_bits(8); // not usable until T2V2 comes to existance
  cvtdata->config_count_change = b.get_bits(8);

  num_descriptors = b.get_bits(8);
  for(int i = 0; i < num_descriptors; i++)
  {
    des_tag = b.get_bits(8);
    des_len = b.get_bits(8);

    switch(des_tag)
    {
      case VENDOR_ID:
        cvtdata->vendorId = b.get_bits(des_len*8); //  throw exception if des_len != 3
        break;
      case HARDWARE_VERSION_ID:
        //  throw exception if des_len !=4
        cvtdata->hardwareVersionId = b.get_bits(des_len*8);
        break;
      case HOST_MAC_ADDR:
        // throw exception if des_len > 6
        cvtdata->host_MAC_addr = ((long long)b.get_bits(16)) << 32;
        cvtdata->host_MAC_addr = cvtdata->host_MAC_addr | ((long long)b.get_bits(32));
        break;
      case HOST_ID:
        // throw exception if des_len > 5
        cvtdata->host_id = ((long long)b.get_bits(8)) << 32;
        cvtdata->host_id = cvtdata->host_id | ((long long)b.get_bits(32));
        break;
      default:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","TABLECVT_T2Vx::parseCVT()  Unknown descriptor type of [0x%X]\n", des_tag);
        b.skip_bits(des_len*8);
        break;
    }
  }

  cvtdata->downloadtype       = b.get_bits(4);
  cvtdata->download_command   = b.get_bits(4);

  switch(cvtdata->downloadtype)
  {
    case CVT_T2Vx_INBAND_FAT:
    {
      cvtdata->location_type = b.get_bits(8);
      switch(cvtdata->location_type)
      {
        case CVT_T2Vx_LOC_SOURCE_ID:
        {
          location_t0_t* locdata = (location_t0_t*)&(cvtdata->loc_data);
          locdata->sourceId = b.get_bits(16);
          break;
        }
        case CVT_T2Vx_LOC_FREQ_PID:
        {
          location_t1_t* locdata = (location_t1_t*)&(cvtdata->loc_data);
          locdata->freq_vector      = b.get_bits(16);
          locdata->modulation_type  = b.get_bits(8);
                                      b.skip_bits(3);
          locdata->PID              = b.get_bits(13);
          break;
        }
        case CVT_T2Vx_LOC_FREQ_PROG:
        {
          location_t2_t* locdata = (location_t2_t*)&(cvtdata->loc_data);
          locdata->freq_vector      = b.get_bits(16);
          locdata->modulation_type  = b.get_bits(8);
          locdata->program_num      = b.get_bits(16);
          break;
        }
      }
      break;
    }
    case CVT_T2Vx_DSG_TUNNEL:
    {
      cvtdata->location_type = b.get_bits(8);
      switch(cvtdata->location_type)
      {
        case CVT_T2Vx_LOC_BASIC_DSG:
        {
          location_t3_t* locdata = (location_t3_t*)&(cvtdata->loc_data);
          locdata->dsg_tunnel_addr    = ((long long)b.get_bits(16)) << 32;
          locdata->dsg_tunnel_addr    = locdata->dsg_tunnel_addr | b.get_bits(32);
          locdata->src_ip_addr_upper  = ((long long)b.get_bits(32)) << 32;
          locdata->src_ip_addr_upper  = locdata->src_ip_addr_upper | b.get_bits(32);
          locdata->src_ip_addr_lower  = ((long long)b.get_bits(32)) << 32;
          locdata->src_ip_addr_lower  = locdata->src_ip_addr_lower | b.get_bits(32);
          locdata->dst_ip_addr_upper  = ((long long)b.get_bits(32)) << 32;
          locdata->dst_ip_addr_upper  = locdata->dst_ip_addr_upper | b.get_bits(32);
          locdata->dst_ip_addr_lower  = ((long long)b.get_bits(32)) << 32;
          locdata->dst_ip_addr_lower  = locdata->dst_ip_addr_lower | b.get_bits(32);
          locdata->src_port           = b.get_bits(16);
          locdata->dst_port           = b.get_bits(16);
          break;
        }
        case CVT_T2Vx_LOC_ADV_DSG:
        {
          location_t4_t* locdata = (location_t4_t*)&(cvtdata->loc_data);
          locdata->app_id   = b.get_bits(16);
          break;
        }
      }
      break;
    }
    case CVT_T2Vx_TFTP:
    {
      cvtdata->location_type = CVT_T2Vx_LOC_TFTP;
      location_tftp_t* locdata = (location_tftp_t*)&(cvtdata->loc_data);
#if 0
      locdata->tftp_addr_upper = ((long long)b.get_bits(32)) << 32;
      locdata->tftp_addr_upper = locdata->tftp_addr_upper | b.get_bits(32);
      locdata->tftp_addr_lower = ((long long)b.get_bits(32)) << 32;
      locdata->tftp_addr_lower = locdata->tftp_addr_lower | b.get_bits(32);
#endif
    for(int k = 0; k < 8; k++)
      {
    locdata->tftp_addr_upper[k] = (char)b.get_bits(8);
      }
    for(int k = 0; k < 8; k++)
      {
    locdata->tftp_addr_lower[k] = (char)b.get_bits(8);
      }
      break;
    }
  }

  codefile_name_len           = b.get_bits(8);
  for(int k = 0; k < codefile_name_len; k++)
  {
    codefile_name[k] = (char)b.get_bits(8);
  }

  unsigned char* name;
  name = (unsigned char*)malloc(codefile_name_len+1);
  memcpy(name, codefile_name, codefile_name_len);
  cvtdata->codefile_data.len = codefile_name_len;
  cvtdata->codefile_data.name = name;
  name[codefile_name_len] = 0;

  cvtdata->num_cvcs = b.get_bits(8);

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


int TableCVT_T2Vx::filterCVT(cvtfilterdata_t *data)
{
  int ret = false;
  int ven_hardId = false;
  int macAddr = false;
  int hostId = false;

  cvt_t2v1_filterdata_t* fltData = (cvt_t2v1_filterdata_t *)data;
  cvt_t2v1_download_data_t* cvtdata = (cvt_t2v1_download_data_t*)&cvt_data;

  if((fltData->vendorId == cvtdata->vendorId) &&
     (fltData->hardwareVersionId == cvtdata->hardwareVersionId))
  {
    ven_hardId = true;
  }

  if((fltData->host_MAC_addr == cvtdata->host_MAC_addr) || (cvtdata->host_MAC_addr == 0))
  {
    macAddr = true;
  }

  if((fltData->host_id == cvtdata->host_id) || (cvtdata->host_id == 0))
  {
    hostId = true;
  }

  if(ven_hardId && macAddr && hostId)
  {
    ret = true;
  }

  return ret;
}


void TableCVT_T2Vx::parseCVTVer2()
{
  int size_indicator;
  int length_value;
  int length_field_size;
  int num_descriptors;
  int des_tag, des_len;
  int codefile_name_len;
  char codefile_name[256];
  int cvc_start_bit;
  int cvc_len_bits;
  int cvc_len_bytes;
  
  cvt_t2v2_download_data_t* cvtdata = (cvt_t2v2_download_data_t*)&cvt_data;

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

  cvtdata->protocol_version = b.get_bits(8); // not usable until T2V2 comes to existance
  cvtdata->config_count_change = b.get_bits(8);

  num_descriptors = b.get_bits(8);
  if(num_descriptors < 2)
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","TableCVT_T2Vx::parseCVTVer2: Error in CVT num_descriptors:%d \n",num_descriptors);
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
        //  throw exception if des_len !=4
        cvtdata->hardwareVersionId = b.get_bits(des_len*8);
        break;
      case HOST_MAC_ADDR:
        // throw exception if des_len > 6
        cvtdata->host_MAC_addr = ((long long)b.get_bits(16)) << 32;
        cvtdata->host_MAC_addr = cvtdata->host_MAC_addr | ((long long)b.get_bits(32));
        break;
      case HOST_ID:
        // throw exception if des_len > 5
        cvtdata->host_id = ((long long)b.get_bits(8)) << 32;
        cvtdata->host_id = cvtdata->host_id | ((long long)b.get_bits(32));
        break;
      default:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","TABLECVT_T2Vx::parseCVT()  Unknown descriptor type of [0x%X]\n", des_tag);
        b.skip_bits(des_len*8);
        break;
    }
  }
  cvtdata->num_of_objects =  b.get_bits(8);
    
  if(cvtdata->num_of_objects > CVT2V2_MAX_OBJECTS)
  {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","TableCVT_T2Vx::parseCVTVer2: Error in CVT2 cvtdata->num_of_objects :%d \n",cvtdata->num_of_objects);
  }
  for(int i = 0; i < cvtdata->num_of_objects; i++)
  {    
      cvtdata->object_info[i].downloadtype       = b.get_bits(4);
      cvtdata->object_info[i].download_command   = b.get_bits(4);

      switch(cvtdata->object_info[i].downloadtype)
      {
            case CVT_T2Vx_INBAND_FAT:
            {
                  cvtdata->object_info[i].location_type = b.get_bits(8);
                  switch(cvtdata->object_info[i].location_type)
                  {
                    case CVT_T2Vx_LOC_SOURCE_ID:
                    {
                          location_t0_t* locdata = (location_t0_t*)&(cvtdata->object_info[i].loc_data);
                          locdata->sourceId = b.get_bits(16);
                          break;
                    }
                    case CVT_T2Vx_LOC_FREQ_PID:
                    {
                          location_t1_t* locdata = (location_t1_t*)&(cvtdata->object_info[i].loc_data);
                          locdata->freq_vector      = b.get_bits(16);
                          locdata->modulation_type  = b.get_bits(8);
                                         b.skip_bits(3);
                          locdata->PID              = b.get_bits(13);
                          break;
                       }
                       case CVT_T2Vx_LOC_FREQ_PROG:
                    {
                    location_t2_t* locdata = (location_t2_t*)&(cvtdata->object_info[i].loc_data);
                    locdata->freq_vector      = b.get_bits(16);
                    locdata->modulation_type  = b.get_bits(8);
                    locdata->program_num      = b.get_bits(16);
                    break;
                }
                  }
                  break;
            }
        case CVT_T2Vx_DSG_TUNNEL:
            {
                 cvtdata->object_info[i].location_type = b.get_bits(8);
                  switch(cvtdata->object_info[i].location_type)
                  {
                    case CVT_T2Vx_LOC_BASIC_DSG:
                       {
                    location_t3_t* locdata = (location_t3_t*)&(cvtdata->object_info[i].loc_data);
                    locdata->dsg_tunnel_addr    = ((long long)b.get_bits(16)) << 32;
                    locdata->dsg_tunnel_addr    = locdata->dsg_tunnel_addr | b.get_bits(32);
                    locdata->src_ip_addr_upper  = ((long long)b.get_bits(32)) << 32;
                    locdata->src_ip_addr_upper  = locdata->src_ip_addr_upper | b.get_bits(32);
                    locdata->src_ip_addr_lower  = ((long long)b.get_bits(32)) << 32;
                    locdata->src_ip_addr_lower  = locdata->src_ip_addr_lower | b.get_bits(32);
                    locdata->dst_ip_addr_upper  = ((long long)b.get_bits(32)) << 32;
                    locdata->dst_ip_addr_upper  = locdata->dst_ip_addr_upper | b.get_bits(32);
                    locdata->dst_ip_addr_lower  = ((long long)b.get_bits(32)) << 32;
                    locdata->dst_ip_addr_lower  = locdata->dst_ip_addr_lower | b.get_bits(32);
                    locdata->src_port           = b.get_bits(16);
                    locdata->dst_port           = b.get_bits(16);
                    break;
                }
                    case CVT_T2Vx_LOC_ADV_DSG:
                {
                    location_t4_t* locdata = (location_t4_t*)&(cvtdata->object_info[i].loc_data);
                    locdata->app_id   = b.get_bits(16);
                    break;
                }
                  }
                  break;
            }
            case CVT_T2Vx_TFTP:
            {
                  cvtdata->object_info[i].location_type = CVT_T2Vx_LOC_TFTP;
                  location_tftp_t* locdata = (location_tftp_t*)&(cvtdata->object_info[i].loc_data);
            for(int k = 0; k < 8; k++)
            {
                locdata->tftp_addr_upper[k] = (char)b.get_bits(8);
            }
            for(int k = 0; k < 8; k++)
            {
                locdata->tftp_addr_lower[k] = (char)b.get_bits(8);
            }
            break;
        }
        
        default:
        break;
      }
    cvtdata->object_info[i].object_type = (cvt2v2_object_type_e)b.get_bits(16);
    cvtdata->object_info[i].object_data_length = (int)b.get_bits(8);
    cvtdata->object_info[i].pObject_data_byte = (unsigned char*)malloc(cvtdata->object_info[i].object_data_length);
    if(cvtdata->object_info[i].pObject_data_byte == NULL)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","TableCVT_T2Vx::parseCVTVer2: Error in Memory Allocation Failed \n");
        return;
    }
    for(int j = 0; j < cvtdata->object_info[i].object_data_length; j++)
    {
        cvtdata->object_info[i].pObject_data_byte[j] = (unsigned char)b.get_bits(8);
    }

      codefile_name_len           = b.get_bits(8);
      for(int k = 0; k < codefile_name_len; k++)
      {
            codefile_name[k] = (char)b.get_bits(8);
      }

      unsigned char* name;
      name = (unsigned char*)malloc(codefile_name_len+1);
    memcpy(name, codefile_name, codefile_name_len);
      cvtdata->object_info[i].codefile_data.len = codefile_name_len;
      cvtdata->object_info[i].codefile_data.name = name;
      name[codefile_name_len] = 0;
  }

  cvtdata->num_cvcs = b.get_bits(8);

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


