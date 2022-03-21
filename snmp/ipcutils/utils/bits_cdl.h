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


#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__


//[Vinod] : Removed VLOCAP Stack specific headers
//Eric May add the required header files to complile this class
#include <stdio.h>


/** Abstract a given buffer as a stream of bits.
 *
 */
class SimpleBitstream
{
public:

/**
 * Initialize the bit-getter to point to the specified buffer.
 */
  SimpleBitstream(unsigned char *buf, //!< Buffer to use.
                  unsigned long len   //!< Length of \e buf.
                  );

/**
 * Not implemented.
 */
  ~SimpleBitstream();

/** Extract the next 'n' bits and return them as an unsigned long.
 *
 * Reimplement this function with a faster algorithm.
 */
      unsigned long get_bits(int num);

/**
 * Set #curr_bit_index to #curr_bit_index + \e num.
 */
  void skip_bits(int num //!< Number of bits to skip.
                 );

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
};

#endif //__BITSTREAM_H__
