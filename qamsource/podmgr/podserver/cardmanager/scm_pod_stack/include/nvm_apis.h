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



#ifndef _nvm_apis_h_
#define _nvm_apis_h_

#include "pdt_Common.h"



#ifndef VAR_EXTERN
#define VAR_EXTERN extern
#endif /*VAR_EXTERN*/

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include <stdio.h>

#ifndef _nvmmbid_h_
#include "nvmm_bid.h"
#endif /*_nvmmbid_h_*/



//@{ 
/** \defgroup NVRAM NVRAM_Sub_System 
 * The <i>NVRAM_Sub_System</i> is a collection of APIs that provide
 * persistant storage for the Media Receiver project.  Users of the NVRAM
 * subsystem should register their individual uses in one of the following
 * header files:\n
 * \n
 * NVRAM_BB_Users.h // Reserved for Burbank\n
 * NVRAM_SD_Users.h // Reserved for San Diego\n
 * NVRAM_DVB_Users.h // Reserved for Digital Video Board\n
 * NVRAM_MB_Users.h // Reserved for Main Board\n
 * \n
 * Each NVRAM user group is allocated a range of BLOCKIDs.  There should be
 * no duplicate BLOCKIDs for the entire Media Receiver project.  Following 
 * the established declaration paradigm should guarantee no duplicates.\n
 * \n
 * Use the generated enumerated values to reference BLOCKIDs.  Do not use
 * hard coded numbers.  The MAC_ALL_XXX() and MAC() macros will generate
 * enumerated identifies with the following consistant format:\n
 * \n
 *    NVRAM_BB_ + MeaningfulName \n
 *    NVRAM_SD_ + MeaningfulName \n
 *    NVRAM_DVB_ + MeaningfulName \n
 *    NVRAM_MB_ + MeaningfulName \n
*/
//@{ 

#define DEFAULT_NVRAM_BLOCK_SIZE 0x0400

#define NVRAM_BB_UsersBlockSize    DEFAULT_NVRAM_BLOCK_SIZE
#define NVRAM_SD_UsersBlockSize    DEFAULT_NVRAM_BLOCK_SIZE
#define NVRAM_DVB_UsersBlockSize   DEFAULT_NVRAM_BLOCK_SIZE
#define NVRAM_MB_UsersBlockSize    DEFAULT_NVRAM_BLOCK_SIZE

/* Generate a master NVRAM usage list with the following naming conventions:
    NVRAM_BB_ + MeaningfulName
    NVRAM_SD_ + MeaningfulName
    NVRAM_DVB_ + MeaningfulName
    NVRAM_MB_ + MeaningfulName
*/
typedef enum eNVM_BLOCKID {
    
    NVRAM_BB_Users       = 0x1000,

    #include "NVRAM_BB_Users.h"
    #undef MAC
    #define MAC(base) NVRAM_BB_ ##   base,
        MAC_ALL_BB(MAC)

    NVRAM_SD_Users       = 0x4000,

    #include "NVRAM_SD_Users.h"
    #undef MAC
    #define MAC(base) NVRAM_SD_ ##   base,
        MAC_ALL_SD(MAC)

    NVRAM_DVB_Users      = 0x0000,

    #include "NVRAM_DVB_Users.h"
    #undef MAC
    #define MAC(base) NVRAM_DVB_ ##   base,
        MAC_ALL_DVB(MAC)

    NVRAM_MB_Users       = 0x6000,

    #include "NVRAM_MB_Users.h"
    #undef MAC
    #define MAC(base) NVRAM_MB_ ##   base,
        MAC_ALL_MB(MAC)

    NVRAM_LAST_NumBlockIDs
} eNVM_BLOCKID;


// API return codes
typedef enum eNvmm_ReturnCode
{
    // nvmm_CreateBlock()
    kNvmm_BlockCreated,
    kNvmm_BlockResized,
    kNvmm_BlockExists,
    
    // nvmm_OpenBlock()
    kNvmm_BlockRemoved,
    kNvmm_BlockOpen
} eNvmm_ReturnCode;


// exception error codes
typedef enum eNvmm_ErrorCodes
{
    kNvmm_NoErr                    =   0,
    kNvmm_DirectoryCorruptErr   = - 1,
    kNvmm_AccessIDErr            = - 2,
    kNvmm_BlockIDErr            = - 3,
    kNvmm_InvalidSizeErr        = - 4,
    kNvmm_InsufficientNVMErr    = - 5,
    kNvmm_DeviceReadErr            = - 6,
    kNvmm_DeviceWriteErr        = - 7,
    kNvmm_WriteAccessErr        = - 8
} eNvmm_ErrorCodes;


// These are for use w/nvmm_rawRead
typedef enum {
    NVRAM_Hardware_Version_Number    = 0x000,
    NVRAM_Hardware_Serial_Number    = 0x002,
    NVRAM_Version_Serial_Number_CRC16   = 0x005,
    NVRAM_VCXO_Calibration            = 0x00C,
    NVRAM_VCXO_Calibration_CRC16        = 0x00E,
    NVRAM_IEEE1394_DTCP_Key             = 0x010,
    NVRAM_IEEE1394_DTCP_Key_CRC16       = 0x3F0,
    NVRAM_IEEE1394_Chip_ID              = 0x3F2,
    NVRAM_IEEE1394_Model_Name           = 0x3F7,
    NVRAM_IEEE1394_Model_Chip_CRC16     = 0x407,
    NVRAM_IEEE1394_SRM                  = 0x409,
    NVRAM_IEEE1394_SRM_CRC16            = 0x809,
    NVRAM_IEEE1394_Stock                = 0x80B,
    NVRAM_IEEE1394_Stock_CRC16            = 0x823,
    NVRAM_CFE_Version                   = 0x839,
    NVRAM_Read_Write_Test_Byte            = 0xBFF,
    NVRAM_CP_AUTH_STATUS                = 0x7000,
} FactoryKeys;



#define kNvmm_InvalidAccessID NULL

// nvmm_CreateBlock() - bit field flags for creating logical blocks
#define kNvmm_Resize                0x00000001

// nvmm_OpenBlock() - bit field flags for opening logical blocks
#define kNvmm_DisableCache            0x00000001
#define kNvmm_EnableWrite            0x00000002


//The Flags for nvmm_CreateBlock() and nvmm_OpenBlock are:
//    #define kNvmm_DisableCache       0x00000001
//    #define kNvmm_EnableWrite        0x00000002
typedef long  Nvmm_BlockID;
typedef struct Nvmm_BlockAccess    Nvmm_BlockAccess;  // forward declaration of internal structure
typedef FILE* Nvmm_AccessID;


void compilerCheckForDuplicates(void);


/*
**  This function allows raw regions of NV RAM to be read
**  Used to get factory keys
**  returns number of data actually read
*/
unsigned nvmm_rawRead( unsigned nvRamOffset, unsigned length, unsigned char *localBuffer ); 

/*
**  Allows raw regions of NVRAM to be written
 */ 
unsigned nvmm_rawWrite( unsigned nvRamOffset, unsigned length, unsigned char *localBuffer )  ;

/*
**
**  Used to Force Initialization of NV_RAM
**
*/
extern void  nvmm_ForcedInitialization( void );


/** <i>nvmm_CreateBlock</i> creates a new block in the nvm.\
    *Requires the blockID and the size of the block you wish to create.

    @param blockID Block ID to create
    @param size size of the block to create
    @param flags kNvmm_Resize or \n
                 0x00
    @return kNvmm_BlockCreated if the block was created\n
        kNvmm_BlockExists if the block exists and kNvmm_Resize\n
        was not specified (Also returned on generic errors, for now)\n
 */
extern eNvmm_ReturnCode nvmm_CreateBlock ( Nvmm_BlockID blockID , Uint32 size , Uint32 flags );

/** <i>nvmm_RemoveBlock</i> removes a block previously allocated from nvm
    *Requires the blockID that you wish to remove.

    @param blockID Block ID to remove
    @return kNvmm_BlockCreated if the block was removed\n
 */
extern eNvmm_ReturnCode nvmm_RemoveBlock ( Nvmm_BlockID blockID );

/** <i>nvmm_OpenBlock</i> opens block a previously allocated from nvm
    *Requires the blockID that you wish to open.
    *Returns Status_DATA_NOT_AVAILABLE if it can't access the data.

    @param blockID Block ID to create
    @param flags any of the following ORed together\n
                kNvmm_DisableCache,\n
                kNvmm_EnableWrite, or \n
        0x00 for neither
    @return Nvvmm_AccessID used to access block or\n
            NULL on failure\n
 */
extern Nvmm_AccessID nvmm_OpenBlock ( Nvmm_BlockID blockID , Uint32 flags );

/** <i>nvmm_CloseBlock</i> closes block a previously allocated from nvm
    *Requires the acessID that you wish to close.
    *The acessID is a FILE pointer of the file to the corressponding blockID

    @param accessID Nvmm_AccessID previously returned by nvmm_OpenBlock
    @return void
 */
extern void nvmm_CloseBlock ( Nvmm_AccessID accessID );

/** <i>nvmm_Read</i> reads from a block previously allocated from nvm
    *Requires the acessID, offset of the location in the file, and the count that you wish to read.
    *The acessID is a FILE pointer of the file to the corressponding blockID
    *The count refers to the number of bytes you wish to read into the buffer
    *Returns the number of bytes read

    @param accessID Nvmm_AccessID previously returned by nvmm_OpenBlock
    @param offset offset from begining of block at which to start reading
    @param count number of bytes to read
    @param buffer buffer in which to read
    @return number of bytes read or\n
        kNvmm_InsufficientNVMErr if the block did not contain enough data or\n
        kNvmm_DeviceReadErr on general errors\n
 */
extern Int32 nvmm_Read ( Nvmm_AccessID accessID , Uint32 offset , Uint32 count , Uint8 * buffer );

/** <i>nvmm_Write</i> writes to a block previously allocated from nvm
    *Requires the acessID, offset of the location in the file, and the count that you wish to write.
    *The acessID is a FILE pointer of the file to the corressponding blockID
    *The count refers to the number of bytes you wish to write into the buffer
    *Returns the number of bytes written

    @param accessID Nvmm_AccessID previously returned by nvmm_OpenBlock
    @param offset offset from begining of block at which to start writing
    @param count number of bytes to write
    @param buffer buffer from which to write
    @return number of bytes written or\n
        kNvmm_WriteAccessErr if the file is not writable\n
        kNvmm_DeviceWriteErr on general errors\n
 */
extern Int32 nvmm_Write ( Nvmm_AccessID accessID , Uint32 offset , Uint32 count , const Uint8 * buffer );

/** <i>nvmm_Flush</i> calls fflush

    If the given stream has been opened for writing the output buffer is written to the file
    If the given stream has been opened for reading the content of the input buffer is cleared
    The stream remains open after this call
    If the given file has been closed all the buffers associated with it are automatically flushed
    and this function should **NOT** be called

    @param accessID Nvmm_AccessID previously returned by nvmm_OpenBlock
    @return void
*/
extern void nvmm_Flush ( Nvmm_AccessID accessID );


/* end functions */


/* start variables */
/* end variables */

/*@} // defgroup NVRAM end */
/*@} // addtogroup Seachange Media Receiver Platform System  end */

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*_nvmm_h_*/

