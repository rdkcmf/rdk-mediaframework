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



#ifndef CVTDOWNLOADTYPES_H
#define CVTDOWNLOADTYPES_H

#define     CVT2V2_MAX_OBJECTS        4
// Common Structures

typedef struct host_proprietary_data_struct  {
  int len;
  unsigned char data[16];
} host_proprietary_data_t;

typedef struct codefile_data_struct {
  int len;
  unsigned char* name;
} codefile_data_t;

typedef struct cvc_data_struct {
  int len;
  unsigned char* data;
} cvc_data_t;

typedef enum cvt2v2_object_type
{
    CVT2V2_FIRMWARE_OBJECT = 0,
    CVT2V2_APPLICATION_OBJECT,
    CVT2V2_DATA_OBJECT,
    CVT2V2_OTHER,

}cvt2v2_object_type_e;
// Download Commands
#define DOWNLOAD_NOW        0x01
#define DEFERRED_DOWNLOAD   0x02


// Modulation Types
#define QAM_64    0x01
#define QAM_256   0x02


// ---------- Type1 Version1 defines ---------- /

// Type1 Version1 download types
#define CVT_T1V1_ONEWAY_BROADCAST       0x01
#define CVT_T1V1_ALWAYS_ON_DEMAND       0x02
#define CVT_T1V1_DOWNLOAD_UNSUPPORTED   0x03

typedef struct cvt_t1v1_download_data {
  int                       downloadtype;
  int                       vendorId;
  int                       hardwareVersionId;
  host_proprietary_data_t   hostPropData;
  int                       download_command;
  int                       freq_vector;
  int                       transport_value;// named to match the spec;
                                            // as opposed to modulation_type
  int                       PID;
  codefile_data_t           codefile_data;
  cvc_data_t                cvc_data;
} cvt_t1v1_download_data_t;

// -------------------------------------------- /


// -------- Type2 Version1 defines ------------ /

// Type2 Version1 download types
#define CVT_T2Vx_INBAND_FAT             0x00
#define CVT_T2Vx_DSG_TUNNEL             0x01
#define CVT_T2Vx_TFTP                   0x02

// Type2 Version1 location types
#define CVT_T2Vx_LOC_SOURCE_ID          0x00
#define CVT_T2Vx_LOC_FREQ_PID           0x01
#define CVT_T2Vx_LOC_FREQ_PROG          0x02
#define CVT_T2Vx_LOC_BASIC_DSG          0x03
#define CVT_T2Vx_LOC_ADV_DSG            0x04
#define CVT_T2Vx_LOC_TFTP               0xFF  // location type does not exist in spec
                                              // but needed a type for delivery in the
                                              // return data.


// Location Type 0x00  Source ID
typedef struct location_t0 {
  int location_type;
  int sourceId;
} location_t0_t;


// Location Type 0x01  Freq and PID
typedef struct location_t1 {
  int location_type;
  int freq_vector;
  int modulation_type;
  int PID;
} location_t1_t;


// Location Type 0x02  Freq and Prog Num
typedef struct location_t2 {
  int location_type;
  int freq_vector;
  int modulation_type;
  int program_num;
} location_t2_t;


// Location Type 0x03  Basic DSG Tunnel
typedef struct location_t3 {
  int         location_type;
  long long   dsg_tunnel_addr;
  long long   src_ip_addr_upper;
  long long   src_ip_addr_lower;
  long long   dst_ip_addr_upper;
  long long   dst_ip_addr_lower;
  int         src_port;
  int         dst_port;
} location_t3_t;


// Location Type 0x04  Adv DSG Tunnel
typedef struct location_t4 {
  int location_type;
  int app_id;
} location_t4_t;


// Location type 0xFF   TFTP
typedef struct location_tftp {
  int location_type;
  char  tftp_addr_upper[8];
  char  tftp_addr_lower[8];
} location_tftp_t;


// Common location type
typedef union location_data {
  int             location_type;
  location_t0_t   location_type0_data;
  location_t1_t   location_type1_data;
  location_t2_t   location_type2_data;
  location_t3_t   location_type3_data;
  location_t4_t      location_type4_data;
  location_tftp_t location_typetftp_data;
} location_data_t;


typedef struct cvt_t2v1_download_data {
  int                       downloadtype;
  int                       vendorId;
  int                       hardwareVersionId;
  int                       protocol_version;
  int                       config_count_change;
  long long                 host_MAC_addr;
  long long                 host_id;
  host_proprietary_data_t   hostPropData;
  int                       download_command;
  int                       location_type;
  location_data_t           loc_data;
  codefile_data_t           codefile_data;
  int                       num_cvcs;
  cvc_data_t                cvc_data;
} cvt_t2v1_download_data_t;

typedef struct t2v2_objects
{
  int                       download_command;
  int                       downloadtype;
  int                       location_type;
  location_data_t           loc_data;
  

  cvt2v2_object_type_e        object_type;
  int                 object_data_length;
  unsigned char         *pObject_data_byte;
  codefile_data_t               codefile_data;
  int   downloadRequired;// This flag is set to "1" by the cdl manager to notify the download engine that download of the object image is required
  int   downloadComplete;// This flag is set to '1' by the download engine to notify the cdl manager that object dowload is complete
  int   downloadError;// This flag is set to '1' by the download engine to notify the cdl manager that Error occured while downloading 
  unsigned char *pDownloadedFileName;// This is the object image file name  with path to notify the cdl manager when download is complete
                    // memory will be freed by the cdl manager after its usage
  int   downloadedFileNameSize;// size of the above file path.
  int  downloadImageVerificationDone;//
  int  downloadImageExtractionDone;
  int  downloadParamsUpgradeDone;
  int  downloadImageUpgradeNotified;
}t2v2_objects_t;

typedef struct cvt_t2v2_download_data {
  
  int                       vendorId;
  int                       hardwareVersionId;
  int                       protocol_version;
  int                       config_count_change;
  long long                 host_MAC_addr;
  long long                 host_id;
  host_proprietary_data_t   hostPropData;

  int num_of_objects;
//{
  t2v2_objects_t     object_info[CVT2V2_MAX_OBJECTS];
//}
  int                       num_cvcs;
  cvc_data_t                cvc_data;
} cvt_t2v2_download_data_t;

// Common return type
typedef union cvtdownload_data {
  int                         downloadtype;
  cvt_t1v1_download_data_t    cvt_t1v1;
  cvt_t2v1_download_data_t    cvt_t2v1;
  cvt_t2v2_download_data_t    cvt_t2v2;

} cvtdownload_data_t;


#endif // CVTDOWNLOADTYPES_H
