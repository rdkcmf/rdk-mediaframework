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
#include <unistd.h>
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>

#endif
//#include "pfcresource.h"
#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "core_events.h"
#include "cardmanager.h"

// Constructor - initialize the bit-getter to point to the specified buffer.
cSimpleBitstream::cSimpleBitstream(unsigned char *buf, unsigned long len)
{
    curr_bit_index = 0;
    bs = buf;
    bs_len = len * 8;
}

cSimpleBitstream::~cSimpleBitstream()
{
}

/* get_bits - extract the next 'n' bits and return them as an unsigned long.
 * Must be preceeded by a call to init_bits() to point to the right buffer.
 * reimplement this function with a faster algorithm...
 */
unsigned long cSimpleBitstream::get_bits(int n)
{
    int i;
    unsigned long out = 0;
    
    for(i=0; i<n; i++) {
        if(curr_bit_index >= bs_len) {
            //RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "Attempting to read beyond end of bitstream buffer!");
            return 0;
        }
        int byte = curr_bit_index / 8;
        int bit = 7 - (curr_bit_index % 8);    /* msb first */

        out <<= 1;
        out |= (unsigned long)((bs[byte]>>bit) & 1);
        curr_bit_index++;
    }
    return out;
}

void cSimpleBitstream::skip_bits(int n)
{
    curr_bit_index += n;
}

