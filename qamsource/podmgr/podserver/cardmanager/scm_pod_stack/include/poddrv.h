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



#ifndef _PODDRV_H_
#define _PODDRV_H_

#ifdef __cplusplus
extern "C" {
#endif

# ifdef __KERNEL__

/*! Open the POD for read/write

    A single user-space client is allowed to open the POD in order to read/write
    its memory-mapped EBI addresses
    @precondition POD is inserted.
    NB: this requirement may be removed if too difficult for client to satisfy
    @param inode [IN] Standard Linux pointer 
    @param file [IN] Standard Linux pointer
    @return EOK if successful, 
            -ENODEV if POD initialization failed or POD not inserted
            -ENOMEM if out of memory
            -ENOACCESS if POD already opened
    @version 12/30/03 - prototype
 */
int     pod_drv_open ( struct inode * inode, struct file * file );

/*! Close the POD for read/write

    Release the single-owner POD from being accessed
    @precondition POD was opened by caller via pod_drv_open
    @param inode [IN] Standard Linux pointer 
    @param file [IN] Standard Linux pointer
    @return EOK if successful, 
            -ENODEV if POD failed to cleanup
            -ENOMEM if out of memory
            -ENOACCESS if POD not open
    @version 12/30/03 - prototype
 */
int     pod_drv_close( struct inode * inode, struct file * file );

/*! Read from the POD via memory-mapped EBI address(es)

    Not implemented - mmap'd instead
    @precondition POD was opened by caller via pod_drv_open
    @param file [IN] Standard Linux pointer
    @param pBuf [OUT] User-space buffer to fill
    @param count [IN] Count of bytes to read into 'pBuf'
    @param offset [IN] ??
    @return 'count' (>= 0) if read successful,
            -EFAULT if error
    @version 12/30/03 - prototype
 */
//ssize_t pod_drv_read ( struct file * file, char * pBuf, size_t count, loff_t * offset );

/*! Write to the POD via memory-mapped EBI address(es)

    Not implemented - mmap'd instead
    @precondition POD was opened by caller via pod_drv_open
    @param file [IN] Standard Linux pointer
    @param pBuf [IN] User-space buffer to empty
    @param count [IN] Count of bytes to write from 'pBuf'
    @param offset [IN] ??
    @return 'count' (>= 0) if write successful, 
            -EFAULT if error
    @version 12/30/03 - prototype
 */
//ssize_t pod_drv_write( struct file * file, const char * pBuf, size_t count, loff_t * offset );

    // not needed
//loff_t  pod_drv_llseek( struct file * file, loff_t offset, int origin );

/*! Generic I/O control capability

    @precondition POD was opened by caller via pod_drv_open
    @param inode [IN] Standard Linux pointer
    @param file [IN] Standard Linux pointer
    @param int [IN] ??
    @param long [IN] ??
    @return Status of IOCTL (EOK indicates success)
            -EFAULT if error
    @version 12/30/03 - prototype
 */

int     pod_drv_ioctl( struct inode * inode, struct file * file, unsigned int cmd,
                       unsigned long arg );

# endif // __KERNEL__

                                      //  Vcc  Vpp
    // an ioctl  'cmd' can be:        //  ---  ---
#define PODDRV_POWER_OFF            1 //   0    0
#define PODDRV_POWER_33V            2 //  3.3  3.3  default
#define PODDRV_POWER_5V             3 //  3.3   5
#define PODDRV_OOB_ON               4 //    N/A     Turn OOB channel to POD ON
#define PODDRV_OOB_OFF              5 //    N/A     Turn OOB channel to POD OFF
#define PODDRV_PARALLEL_INPUT_ON    6 //    N/A     Turn Parallel to POD ON
#define PODDRV_PARALLEL_INPUT_OFF   7 //    N/A     Turn Parallel to POD ON
#define PODDRV_CIMAX_RESET_ON       8 //    N/A     Set CIMAX reset pin high
#define PODDRV_CIMAX_RESET_OFF      9 //    N/A     Set CIMAX reset pin low

extern int  Pod_fd;     // declared in main.c; init'd in initPodUserSpaceThread

#ifdef __cplusplus
}
#endif

#endif // _PODDRV_H_

