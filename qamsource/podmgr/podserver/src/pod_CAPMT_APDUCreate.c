/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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

#include <rdk_debug.h>
#include <rmf_osal_types.h>
#include <rmf_osal_mem.h>
#include <podimpl.h> 
#include <stdlib.h>
#include <string.h>
#include <podmgr.h>

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

static uint8_t *assemble_CA_PMT_APDU( uint32_t * apdu_size,
									  uint8_t program_index,
									  uint8_t transaction_id, uint8_t ltsid,
									  uint16_t program_number,
									  uint16_t source_id,
									  uint8_t ca_pmt_cmd_id,
									  uint16_t ca_system_id,
									  uint16_t program_info_length,
									  uint8_t * program_info,
									  uint16_t stream_info_length,
									  uint8_t * stream_info )
{
#define PUT_NEXT_BYTE( xx_byteValue ) apdu_buffer[index++] = (xx_byteValue)

	uint8_t *apdu_buffer = NULL;
	uint32_t payload = 0;
	uint32_t index = 0;

	if ( *apdu_size < 127 )
	{
		/* we can handle up to 127, so we need to insure that there is
		 * room for the single byte storing the length
		 */

		*apdu_size += 1;
	}
	else
	{
		/* adjusted to handle the 3 bytes required to convey the length of
		 * this message (using ASN.1)
		 */
		*apdu_size += 3;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "assemble_CA_PMT_APDU: allocating storage for actual APDU (%d bytes)\n",
			 *apdu_size );

	( void ) rmf_osal_memAllocP( RMF_OSAL_MEM_POD, *apdu_size,
								 ( void ** ) &apdu_buffer );
	if ( apdu_buffer == NULL )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "assemble_CA_PMT_APDU: failed to allocate memory.\n" );
		return NULL;
	}

	// ca_pmt_tag
	PUT_NEXT_BYTE( ( RMF_PODMGR_CA_PMT_TAG >> 16 ) & 0xFF );
	PUT_NEXT_BYTE( ( RMF_PODMGR_CA_PMT_TAG >> 8 ) & 0xFF );
	PUT_NEXT_BYTE( RMF_PODMGR_CA_PMT_TAG & 0xFF );

	// handle APDU size appropriately
	if ( *apdu_size <= 127 )
	{
		payload = *apdu_size - 4;
		PUT_NEXT_BYTE( payload );
	}
	else
	{
		PUT_NEXT_BYTE( 0x82 );	// size indicator == 1 (0x80) and length value == 2

		payload = *apdu_size - 6;

		PUT_NEXT_BYTE( payload >> 8 );	// high-order byte
		PUT_NEXT_BYTE( payload & 0xFF );		// low-order byte
	}

	PUT_NEXT_BYTE( program_index );

	// transaction_id
	PUT_NEXT_BYTE( transaction_id );

	// ltsid
	PUT_NEXT_BYTE( ltsid );

	PUT_NEXT_BYTE( program_number >> 8 );
	PUT_NEXT_BYTE( program_number & 0xFF );

	PUT_NEXT_BYTE( source_id >> 8 );
	PUT_NEXT_BYTE( source_id & 0xFF );

	// ca_pmt_cmd_id
	PUT_NEXT_BYTE( ca_pmt_cmd_id );

	// program_info_length
	PUT_NEXT_BYTE( ( program_info_length >> 8 ) & 0xF );		// top 4 bits are reserved
	PUT_NEXT_BYTE( program_info_length & 0xFF );

	if ( ( program_info_length & 0xFFF ) > 0 )
	{
		rmf_osal_memcpy( &apdu_buffer[index], program_info, program_info_length,
			(*apdu_size-index), program_info_length );
		index += program_info_length;
	}

	// now add the stream information
	if ( stream_info_length > 0 )
	{
		rmf_osal_memcpy( &apdu_buffer[index], stream_info, stream_info_length,
			(*apdu_size-index), stream_info_length );
		index += stream_info_length;
	}

	return apdu_buffer;
}

/**
 * This function will create a CA_PMT APDU as defined in CCIF-2.0-I18 for the given service
 * with key values provided by the caller.
 *
 * @param service_handle: A read-locked SIDB service handle
 *
 * @param session:
 *
 * @param apdu_buffer:	  A pointer to pointer to contain the address of an allocated memory buffer allocated by this method.
 *						  It is the callers responsibility to free with rmf_osal_memFreeP().
 *
 * @param apdu_size:	  A pointer to an uint32_t varible to contain the size of apdu_buffer, in bytes.
 *						  Upon return, this function will write the number of bytes in the
 *						  generated APDU to this location.
 *
 * @return RMF_SUCCESS if a CA_PMT APDU is created, RMF_OSAL_ENODATA if no CA descriptors were found
 *		   in the service, and a type-specific error code otherwise.
 */

rmf_Error podmgrCreateCAPMT_APDU( parsedDescriptors * pPodSIDescriptors,
								  uint8_t programIdx, uint8_t transactionId,
								  uint8_t ltsid, uint16_t ca_system_id,
								  uint8_t ca_pmt_cmd_id,
								  uint8_t ** apdu_buffer,
								  uint32_t * apdu_buffer_size )
{
	rmf_Error err;
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "podmgrCreateCAPMT_APDU(service 0x%08x, pgm_index %d, trans %d, ltsid %d, ca_pmt_cmd_id %d) - Enter...\n",
			 pPodSIDescriptors, programIdx, transactionId, ltsid,
			 ca_pmt_cmd_id );

/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
// VL updated POD implementation	 
	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			 "podmgrCreateCAPMT_APDU: program_number:%d sourceId:0x%X\n",
			 pPodSIDescriptors->prgNo, pPodSIDescriptors->srcId );
#endif
/* Standard code deviation: ext: X1 end */
	if ( pPodSIDescriptors->streamInfoLen == 0
		 && pPodSIDescriptors->ProgramInfoLen == 0 )
	{
		err = RMF_OSAL_ENODATA;
		goto FAILED_CA_PMT_CREATION;
	}

	/*
	 * The APDU size is computed from a 3 byte TAG, followed by a 1 or 3 byte
	 * length (computed by the called method below) plus 3 bytes for program_index,
	 * transaction_id, and ltsid respectively plus 2 bytes for program_number plus
	 * 2 bytes for source_id plus 1 byte for the CA_PMT_Command_id plus
	 * pPodSIDescriptors->ProgramInfoLen plus program info plus pPodSIDescriptors->pStreamInfo.
	 */

	*apdu_buffer_size =
		3 + 3 + 2 + 2 + 1 + 2 + pPodSIDescriptors->ProgramInfoLen +
		pPodSIDescriptors->streamInfoLen;

	*apdu_buffer =
		assemble_CA_PMT_APDU( apdu_buffer_size, programIdx, transactionId,
							  ltsid, pPodSIDescriptors->prgNo,
							  pPodSIDescriptors->srcId, ca_pmt_cmd_id,
							  ca_system_id, pPodSIDescriptors->ProgramInfoLen,
							  pPodSIDescriptors->pProgramInfo,
							  pPodSIDescriptors->streamInfoLen,
							  pPodSIDescriptors->pStreamInfo );

	if ( NULL == *apdu_buffer )
		return RMF_OSAL_ENODATA;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "podmgrCreateCAPMT_APDU: Successfully created %d byte CA_PMT APDU\n",
			 *apdu_buffer_size );

	return RMF_SUCCESS;

	//
	// Error recovery/cleanup
	//

  FAILED_CA_PMT_CREATION:

	// Assert: err set by the caller
	return err;

}								// END podmgrCreateCAPMT_APDU()




