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



#include "capmtparse.h"
#include <rdk_debug.h>
#include "rmf_osal_mem.h"
#include <strings.h>
#include <string.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


// Constructor - initialize the bit-getter to point to the specified buffer.
vlSimpleBitstream::vlSimpleBitstream(unsigned char *buf, unsigned long len)
{
    curr_bit_index = 0;
    marked_bit_index = 0;
    bs = buf;
    bs_len = len * 8;
}

vlSimpleBitstream::~vlSimpleBitstream()
{
}

/* get_bits - extract the next 'n' bits and return them as an unsigned long.
 * Must be preceeded by a call to init_bits() to point to the right buffer.
 * Jan-29-2008: Reimplemented this function with a faster algorithm...
 */
unsigned long vlSimpleBitstream::get_bits(int n)
{
    // local vars
    unsigned long long out = 0;
    unsigned long nOrgBitIndex = curr_bit_index;

    // assimilation logic
    while(curr_bit_index < (nOrgBitIndex + n))
    {
        // check for buffer overrun
        if(curr_bit_index >= bs_len)
        {
            // buffer overrun
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SimpleBitstream::get_bits::Attempting to read beyond end of bitstream buffer!\n");
            return 0;
        }
        // calculate the byte pointer
        int byte = curr_bit_index / 8;
        // calculate the bit pointer
        int bit = 7 - (curr_bit_index % 8);    /* msb first */
        int bitsRemaining = (nOrgBitIndex+n)-curr_bit_index;

        // begin optimized code
        // check if the bit pointer is byte aligned
        if((7 == bit) && (0 == (bitsRemaining%8)) && ((bitsRemaining/8) <= 4))
        {
            // optimized code (performs around 32 times faster for longs)
            if(bitsRemaining >= 32)
            {
                // assimilate the byte as a whole
                out = (out<<32) | (VL_VALUE_4_FROM_ARRAY(bs+byte));
                // update bit pointer
                curr_bit_index += 32;
                // finished this round
                continue;
            }
            // optimized code (performs around 24 times faster for 3 byte longs)
            else if(bitsRemaining >= 24)
            {
                // assimilate the byte as a whole
                out = (out<<24) | (VL_VALUE_3_FROM_ARRAY(bs+byte));
                // update bit pointer
                curr_bit_index += 24;
                // finished this round
                continue;
            }
            // optimized code (performs around 16 times faster for shorts)
            else if(bitsRemaining >= 16)
            {
                // assimilate the byte as a whole
                out = (out<<16) | (VL_VALUE_2_FROM_ARRAY(bs+byte));
                // update bit pointer
                curr_bit_index += 16;
                // finished this round
                continue;
            }
            // optimized code (performs around 8 times faster for bytes)
            else if(bitsRemaining >= 8)
            {
                // assimilate the byte as a whole
                out = (out<<8) | (bs[byte]);
                // update bit pointer
                curr_bit_index += 8;
                // finished this round
                continue;
            }
        }
        else // perform an unaligned assimilation
        {
            // optimized code for bits (performs between 1..8 times faster)
            // calculate the bits to read
            int bits_to_read = bitsRemaining;
            // default mask
            unsigned char mask = 0xFF;
            // to avoid endian issues lets not cross byte boundaries
            if( bits_to_read > (bit+1)) bits_to_read = (bit+1);
            // make space for the incoming bits
            out <<= bits_to_read;
            // assimilate the incoming bits
            out |= ((bs[byte]>>((bit+1)-bits_to_read)) & (mask>>(8-bits_to_read)));
            // update bit pointer
            curr_bit_index += bits_to_read;
            // finished this round
            continue;
        }
        // end optimized code
    }
    // return result
    return ((unsigned long)(out));
}

void vlSimpleBitstream::skip_bits(int n)
{
    if(curr_bit_index + n >= bs_len)
    {
        // buffer overrun exception
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SimpleBitstream::skip_bits::Attempting to skip beyond end of bitstream buffer!\n");
    }

    curr_bit_index += n;
}

void vlSimpleBitstream::warn(char * pszClass, char * pszObject)
{
    if(curr_bit_index > bs_len)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","%s::SimpleBitstream:: Overflow occurred for '%s'\n", pszClass, pszObject);
    }
    else if(curr_bit_index < bs_len)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","%s::SimpleBitstream:: Incomplete usage occurred for '%s'\n", pszClass, pszObject);
    }
}


int vlTablePMT::get_streams_count(vlSimpleBitstream & bs, int nBytes)
{
    int nCount = 0;
    int es_info_len = 0;
    int stream_type = 0;
    int elem_pid = 0;

    bs.mark();
    while(nBytes > 0)
    {
        stream_type = bs.get_bits(8);
        bs.skip_bits(3);
        elem_pid = bs.get_bits(13);
        bs.skip_bits(4);
        es_info_len =     bs.get_bits(12);//es_info_len
        // Parse ES_info descriptors
        if(es_info_len > 0)
        {
            bs.skip_bits(es_info_len*8);
        }
        nBytes -= (5 + es_info_len);
        nCount++;
    }
    bs.rewind();
    return nCount;
}


vlTablePMT::vlTablePMT()
{
  pmt = NULL;
}
vlTablePMT::~vlTablePMT()
{
  if(pmt)
     put_pmt_tbl(pmt);
  
  
}
void vlTablePMT::put_pmt_tbl(struct vl_pmt_tbl *tbl)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlTablePMT::put_pmt_tbl Entered ... \n");
    if(tbl) 
    {
        if(tbl->desc)
        {
            std::vector<vlcondAccessesDescsInfo*> Vec;
            vlcondAccessesDescsInfo* pCaDesc;
            
            Vec = tbl->desc->getcondAccessesDescsList();
            for(std::vector<vlcondAccessesDescsInfo*>::iterator Res_iter = Vec.begin(); Res_iter != Vec.end(); Res_iter++)
            {
                pCaDesc = *Res_iter;
                if(pCaDesc != NULL)
                {
                    if( NULL != pCaDesc->pCondAccessesRawBuff)
                    {
						rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pCaDesc->pCondAccessesRawBuff);
                    }
					rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pCaDesc);
                }
            }
            
            Vec.clear();
            
            //VLFREE_INFO2(tbl->desc);
            delete tbl->desc;
        }
        if(tbl->streams)
        {
            for (int i = 0; i < tbl->num_streams; i++)
            {
                if (tbl->streams[i].desc)
                {
                    //VLFREE_INFO2(tbl->streams[i].desc);
                    delete tbl->streams[i].desc;
            }
            }
            //VLFREE_INFO2(tbl->streams);
            delete [] tbl->streams;
        }
        //VLFREE_INFO2(tbl);
        delete tbl;
    }
}

int vlTablePMT::ParseData(unsigned char  *pData, unsigned long Length)
{
   
    int ii;
    unsigned char *buf;
    int noOfStreams;
    int validLength = 0;
     
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Entered ............vlTablePMT::ParseData \n");

    if( (NULL == pData) || (Length == 0) )
    {
      return -1;
    }
	
    buf = pData;
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlTablePMT::ParseData  Length :%d \n", Length);
    
    vlSimpleBitstream b(buf, Length);
     struct vl_pmt_tbl *tbl = new struct vl_pmt_tbl;


   unsigned char *s = (unsigned char *)buf;

    bzero(tbl, sizeof(*tbl));

    tbl->table_id =         b.get_bits(8);    b.skip_bits(4);
    tbl->section_len =        b.get_bits(12);
    tbl->program_num =         b.get_bits(16);    b.skip_bits(2);
    tbl->version =            b.get_bits(5);
    tbl->current_next =        b.get_bits(1);
    tbl->section_num =         b.get_bits(8);
    tbl->last_section_num =    b.get_bits(8);
    b.skip_bits(3);
    tbl->pcr_pid = b.get_bits(13);
    b.skip_bits(4);
    tbl->prog_info_len = b.get_bits(12);

    validLength = 3 + tbl->section_len;
  
  
    // Discard tables with the 'next' indicator set, or for other errors
    if(tbl->current_next == 0)
    {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlTablePMT::ParseData tbl->current_next :%d Error Returning !!! .... \n", tbl->current_next);
          //VLFREE_INFO2(tbl);
        delete tbl;
        tbl = NULL;
        return -1;
    }

    if(tbl->table_id != VL_PMT_TBL_ID)
    {
    
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlTablePMT::ParseData tbl->table_id :%d Error Returning !!! .... \n", tbl->table_id);
        //VLFREE_INFO2(tbl);
        delete tbl;
        tbl = NULL;
        return -1;
    }

   
    if(tbl->prog_info_len > 0)
    {
        tbl->desc = new vlDescriptors();


        tbl->desc->ParseData((unsigned char *)&s[b.get_pos()/8],(unsigned long) tbl->prog_info_len);
        b.skip_bits(tbl->prog_info_len*8);
    }

    noOfStreams = get_streams_count(b,(validLength-4)-(b.get_pos()/8));
  
    if(noOfStreams == 0)
    {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlTablePMT::ParseData   Error !!  noOfStreams :%d  returning... \n", noOfStreams);
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlTablePMT::ParseData    noOfStreams :%d  \n", noOfStreams);
    
    tbl->streams = new struct vlpmt_streams[noOfStreams];

    
    bzero(tbl->streams, sizeof(struct vlpmt_streams) * noOfStreams);
   
    ii = 0;

   while((b.get_pos()/8) < (validLength-4))
   {
        struct vlpmt_streams *prog = &tbl->streams[ii];
        prog->stream_type =     b.get_bits(8);    b.skip_bits(3);
        prog->elem_pid     =     b.get_bits(13);    b.skip_bits(4);
        prog->es_info_len=     b.get_bits(12);

        // Parse ES_info descriptors
        if(prog->es_info_len > 0)
        {
            prog->desc = new vlDescriptors;


            prog->desc->ParseData((unsigned char *)&s[b.get_pos()/8],(unsigned long) prog->es_info_len);
            b.skip_bits(prog->es_info_len*8);
        }

        ii++;
    }
    tbl->num_streams = ii;
    
    pmt = tbl;
    
    return 0;
   
}

vlDescriptors::vlDescriptors()
{
	raw_ca_desc = NULL;
	raw_ca_desc_len = 0;
	raw_desc = NULL;
	raw_desc_len = 0;
	raw_desc_count = 0;
}

vlDescriptors::~vlDescriptors()
{
}

int vlDescriptors::ParseData(unsigned char *pData, 
              unsigned long Length )
{
    unsigned long len = Length;
    unsigned char *buf = pData;
    unsigned char tag, size, count = 0;
    unsigned char *ptr;
    vlcondAccessesDescsInfo *pCondAccessesDescsInfo = NULL;
    unsigned char *s = (unsigned char *)buf;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Entered vlDescriptors::ParseData... \n");
    

    vlSimpleBitstream b((unsigned char*)buf, len);
    
    
    while(b.get_pos() < (len*8))
    {

        // Point to start of this descriptor.  'size' doesn't include the
        // tag and size fields.
        tag =             b.get_bits(8);
        size =             b.get_bits(8);
        ptr =             &s[b.get_pos()/8];
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlDescriptors::ParseData tag:0x%X size:%d count:%d\n",tag,size,count);
        count++;

        switch(tag)
        {
            
            case VL_DESC_COND_ACCESS:
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlDescriptors::ParseData parsing CA Descriptor \n");
            
				rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(vlcondAccessesDescsInfo),(void **)&pCondAccessesDescsInfo);
				memset(pCondAccessesDescsInfo, 0, sizeof(vlcondAccessesDescsInfo));

                if (pCondAccessesDescsInfo != NULL)
                {
                pCondAccessesDescsInfo->condAccessesRawBuffLen = size + 2;
                
				rmf_osal_memAllocP(RMF_OSAL_MEM_POD, pCondAccessesDescsInfo->condAccessesRawBuffLen,(void **)&pCondAccessesDescsInfo->pCondAccessesRawBuff);
                if(pCondAccessesDescsInfo->pCondAccessesRawBuff == NULL)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," vlDescriptors::ParseData .......... Error mem allocation failed!! \n");
			        rmf_osal_memFreeP(RMF_OSAL_MEM_POD, (void *)pCondAccessesDescsInfo );
                    return -1;
                }
		        memset (pCondAccessesDescsInfo->pCondAccessesRawBuff, 0, pCondAccessesDescsInfo->condAccessesRawBuffLen);
                pCondAccessesDescsInfo->pCondAccessesRawBuff[0] = tag;
                pCondAccessesDescsInfo->pCondAccessesRawBuff[1] = size;
                
                memcpy(&pCondAccessesDescsInfo->pCondAccessesRawBuff[2], ptr, size);
                if(pCondAccessesDescsInfo)
                    m_condAccessesDescsList.push_back(pCondAccessesDescsInfo);
                }    
                

            }
            break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlDescriptors::ParseData  ... Not a CA Descriptor  skip this desc...\n");
                break;
        }
        b.skip_bits(size*8);// skip data
    }

    
    return 0;
}
