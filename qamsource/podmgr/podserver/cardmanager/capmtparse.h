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



#ifndef __CA_PMT_PARSE_H__
#define __CA_PMT_PARSE_H__
#include <stdio.h>
//#include <string.h>
#include "utilityMacros.h"
#include <vector>

#define VL_PMT_TBL_ID 0x02
#define VL_DESC_COND_ACCESS    0x09

typedef struct
{
    unsigned char* pCondAccessesRawBuff; 
    int            condAccessesRawBuffLen;
}vlcondAccessesDescsInfo;


class vlSimpleBitstream
{
public:

/**
 * Initialize the bit-getter to point to the specified buffer.
 */
  vlSimpleBitstream(unsigned char *buf, //!< Buffer to use.
                  unsigned long len   //!< Length of \e buf.
                  );

/**
 * Not implemented.
 */
  ~vlSimpleBitstream();

/** Extract the next 'n' bits and return them as an unsigned long.
   *
   * Jan-29-2008: Reimplemented this function with a faster algorithm...
 */
  unsigned long get_bits(int num);

/**
 * Set #curr_bit_index to #curr_bit_index + \e num.
 */
  void skip_bits(int num //!< Number of bits to skip.
                 );

/**
   * Check if the entire stream was processed
 */
    void warn(char * pszClass, char * pszObject);

/**
 * Mark the current position.
 */
    void mark() { marked_bit_index = curr_bit_index; }

/**
 * Rewind to Marked Position.
 */
    void rewind(int bitsOffset = 0) { curr_bit_index = marked_bit_index + bitsOffset; }

/**
 * \return #curr_bit_index.
 */
  unsigned long get_pos() { return curr_bit_index; }
  unsigned long get_buffer_length() { return bs_len; }

private:

/**
 * Bit stream bytes. Set by constructor.
 */
  unsigned char *bs;

/**
 * Bit stream length. Set by constructor.
 */
  unsigned long bs_len;

/**
 * Current bit index.
 */
  unsigned long curr_bit_index;
/**
 * Marked bit index.
 */
  unsigned long marked_bit_index;


};
class vlDescriptors
{
public:

  vlDescriptors();

 ~vlDescriptors();



    int ParseData(unsigned char *pData, 
              unsigned long Length 
            );


    int get_length(void *buf,
                   int count
                   );



    int    get_ca_desc_len(){ return raw_ca_desc_len;}


    unsigned char *get_ca_desc(){ return raw_ca_desc;}

    std::vector<vlcondAccessesDescsInfo*> getcondAccessesDescsList(){return m_condAccessesDescsList;};

private:

    unsigned char     *raw_ca_desc;
    int        raw_ca_desc_len;
    unsigned char *raw_desc;
    int raw_desc_len;
    int raw_desc_count;
    std::vector<vlcondAccessesDescsInfo*> m_condAccessesDescsList;
};






struct vlpmt_streams {
    int        stream_type; //!< Stream type.
    int        elem_pid;    //!< Element PID.
    int        es_info_len; //!< ES info length.
    vlDescriptors    *desc; //!< Descriptors.
};

struct vl_pmt_tbl {
    int        table_id;              //!< Table ID.
    int        section_len;           //!< Section length.
    int        program_num;           //!< Program number.
    int        version;               //!< Version.
    int        current_next;          //!< Current next.
    int        section_num;           //!< Section number.
    int        last_section_num;      //!< Last section number.
    int        pcr_pid;               //!< PCR PID.
    int        prog_info_len;         //!< Program info length.
    vlDescriptors    *desc;           //!< Descriptors.
    int        num_streams;           //!< Number of streams in #streams array.
    struct vlpmt_streams *streams; //!< Streams array.
    int        crc;                   //!< CRC.
};

/**
 * The PMT Table class.
 */
class vlTablePMT 
{
  public:


   vlTablePMT( );
  ~vlTablePMT();
   void dump();
   int get_streams_count(vlSimpleBitstream & bs, int nBytes);

 
  int ParseData(unsigned char  *pData, unsigned long);
  struct vl_pmt_tbl* GetPmtTable() {return pmt;  };
private:

// parse completed MPEG section, parse it as an PMT table.
 
    
     void put_pmt_tbl(struct vl_pmt_tbl* tbl);
    
    struct vl_pmt_tbl *pmt;


};

struct vlcaDetailInfo
{
    unsigned long ulSourceLTSID;
    int cardResourceHandle;
    struct vl_pmt_tbl* pPmtTable;
    void *stream_ptr;
    vlTablePMT *pPmtObj;
};

#endif //__CA_PMT_PARSE_H__
