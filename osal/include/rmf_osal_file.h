/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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

/**
 * @file rmf_osal_file.h
 * @brief The header file contains all the prototypes for file handling such as
 * open, close, read dir, create dir, remove dir, etc. The internal implementation
 * is defined in the OSAL library.
 */

/**
 * @defgroup OSAL_FILE OSAL File Handling
 * This module perform various file handling operations such as open, close, read dir,
 * create dir, remove dir, etc. The internal implementation is defined in the OSAL library.
 * @ingroup OSAL
 *
 * @defgroup OSAL_FILE_API OSAL File Handling API list
 * RMF OSAL File module defines interfaces for creation and management of files.
 * @ingroup OSAL_FILE
 *
 * @addtogroup OSAL_FILE_TYPES OSAL File Handling Data Types
 * Describe the details about Data Structure used by OSAL File Handling.
 * @ingroup OSAL_FILE
 */

#if !defined(_RMF_OSAL_FILE_H)
#define _RMF_OSAL_FILE_H

#include <rmf_osal_types.h>
#include <rmf_osal_error.h>

#include <rmf_osal_time.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup OSAL_FILE_TYPES
 * @{
 */

/* file return error codes */
#define RMF_OSAL_FS_ERROR_FAILURE            (RMF_ERRBASE_OSAL_FS + 1)
#define RMF_OSAL_FS_ERROR_ALREADY_EXISTS     (RMF_ERRBASE_OSAL_FS + 2)
#define RMF_OSAL_FS_ERROR_NOT_FOUND          (RMF_ERRBASE_OSAL_FS + 3)
#define RMF_OSAL_FS_ERROR_EOF                (RMF_ERRBASE_OSAL_FS + 4)
#define RMF_OSAL_FS_ERROR_DEVICE_FAILURE     (RMF_ERRBASE_OSAL_FS + 5)
#define RMF_OSAL_FS_ERROR_INVALID_STATE      (RMF_ERRBASE_OSAL_FS + 6)
#define RMF_OSAL_FS_ERROR_READ_ONLY          (RMF_ERRBASE_OSAL_FS + 7)
#define RMF_OSAL_FS_ERROR_NO_MOUNT           (RMF_ERRBASE_OSAL_FS + 8)
#define RMF_OSAL_FS_ERROR_UNSUPPORT          (RMF_ERRBASE_OSAL_FS + 9)
#define RMF_OSAL_FS_ERROR_OUT_OF_MEM          (RMF_ERRBASE_OSAL_FS + 11)
#define RMF_OSAL_FS_ERROR_TIMEOUT            (RMF_ERRBASE_OSAL_FS + 12)
#define RMF_OSAL_FS_ERROR_INVALID_PARAMETER   (RMF_ERRBASE_OSAL_FS + 13)

/* file seek position parameters */
typedef enum rmf_osal_FileSeekMode_e
{
    RMF_OSAL_FS_SEEK_SET  =               (1),
    RMF_OSAL_FS_SEEK_CUR  =               (2),
    RMF_OSAL_FS_SEEK_END  =               (3)
} rmf_osal_FileSeekMode;


/* file open attribute bitfields */
#define RMF_OSAL_FS_OPEN_READ                (0x0001)
#define RMF_OSAL_FS_OPEN_WRITE               (0x0002)
#define RMF_OSAL_FS_OPEN_READWRITE           (0x0004)
#define RMF_OSAL_FS_OPEN_CAN_CREATE          (0x0008)
#define RMF_OSAL_FS_OPEN_MUST_CREATE         (0x0010)
#define RMF_OSAL_FS_OPEN_TRUNCATE            (0x0020)
#define RMF_OSAL_FS_OPEN_APPEND              (0x0040)

/* file types */
#define RMF_OSAL_FS_TYPE_UNKNOWN             (0x0000)
#define RMF_OSAL_FS_TYPE_FILE                (0x0001)
#define RMF_OSAL_FS_TYPE_DIR                 (0x0002)
#define RMF_OSAL_FS_TYPE_OTHER               (0xffff)

/* file stat info */
typedef enum rmf_osal_FileStatMode_e
{
     RMF_OSAL_FS_STAT_SIZE           =     (1),
     RMF_OSAL_FS_STAT_TYPE           =     (3),
     RMF_OSAL_FS_STAT_CREATEDATE     =     (7),
     RMF_OSAL_FS_STAT_MODDATE        =     (8)
}rmf_osal_FileStatMode;

/* os-specific parameters */
#define RMF_OSAL_FS_MAX_PATH                 1024
#define RMF_OSAL_FS_SEPARATION_STRING        "/"
#define RMF_OSAL_FS_SEPARATION_CHAR          RMF_OSAL_FS_SEPARATION_STRING[0]

/* file system type mappings */

typedef void* rmf_osal_File;
typedef uint32_t rmf_osal_FileOpenMode;

typedef void* rmf_osal_Dir;

typedef struct rmf_osal_DirEntry_s
{
    char name[RMF_OSAL_FS_MAX_PATH + 1];
    uint64_t fileSize;
    rmf_osal_Bool isDir;
} rmf_osal_DirEntry;

typedef struct rmf_osal_FileInfo_s
{
    uint64_t size; /* file size */
    uint16_t type; /* see RMF_OSAL_FS_TYPE_xxx */
    rmf_osal_Time createDate; /* creation date */
    rmf_osal_Time modDate; /* last modified date */
}rmf_osal_FileInfo;

/** @{ */ //end of doxygen tag OSAL_FILE_TYPES

/**
 * @addtogroup OSAL_FILE_API
 * @{
 */

/**
 * @brief This function is used to performs any low-level initialization required to set up the OS-level file subsystem.  
 * @return None
 */
void rmf_osal_fileInit();

/**
 * @brief This function is used to open a file in the file system namespace.
 *
 * @param[in] name The path to the file to open.
 * @param[in] openMode The type of access requested (read, write, create, truncate, append).
 * @param[in] handle A pointer to an rmf_osal_File handle, through which the opened file is returned.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileOpen(const char* name, rmf_osal_FileOpenMode openMode,
        rmf_osal_File* handle);

rmf_Error rmf_osal_fileOpenBuffered(const char* name, rmf_osal_FileOpenMode openMode,
        rmf_osal_File* handle);
/**
 * @brief This function is used to close a file previously opened with rmf_osal_FileOpen().
 *
 * @param[in] handle An rmf_osal_File handle, previously returned by rmf_osal_FileOpen().
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileClose(rmf_osal_File handle);

/**
 * @brief This function is used to read data from a file previously opened with rmf_osal_FileOpen().
 *
 * @param[in] handle An rmf_osal_File handle, previously returned by rmf_osal_FileOpen().
 * @param[in] count A pointer to a byte count.  On entry, this must point to the number of bytes to
 *                  read.  On exit, this will indicate the number of bytes actually read.
 * @param[in] buffer A pointer to a buffer to receive the data.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileRead(rmf_osal_File handle, uint32_t *count,
                                   void* buffer);

/**
 * @brief This function is used to write data to a file previously opened with rmf_osal_FileOpen().
 *
 * @param[in] handle An rmf_osal_File handle, previously returned by rmf_osal_FileOpen().
 * @param[in] count A pointer to a byte count.  On entry, this must point to the number of bytes to
 *                  write.  On exit, this will indicate the number of bytes actually written.
 * @param[in] buffer A pointer to a buffer containing the data to send.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileWrite(rmf_osal_File handle, uint32_t *count,
                                    void* buffer);

/**
 * @brief This function is used to changes and reports the current position within a file previously opened with rmf_osal_FileOpen().
 *
 * @param[in] handle An rmf_osal_File handle, previously returned by rmf_osal_FileOpen().
 * @param[in] seekMode A seek mode constant indicating whether the offset value should be considered
 *                  relative to the start, end, or current position within the file.
 * @param[in] offset A pointer to a file position offset.  On entry, this should indicate the number
 *                  of bytes to seek, offset from the seekMode.  On exit, this will indicate the
 *                  new absolute position within the file.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileSeek(rmf_osal_File handle, rmf_osal_FileSeekMode seekMode,
                                   int64_t* offset);

/**
 * @brief This function is used to synchronizes the contents of a file previously opened with rmf_osal_FileOpen().
 *  This will write any data that is pending.  Pending data is data that has been written to a file, but which hasn't
 *  been flushed to the storage device yet.
 *
 * @param[in] handle An rmf_osal_File handle, previously returned by rmf_osal_FileOpen().
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileSync(rmf_osal_File handle);

/**
 * @brief This function is use to retrieve some file status info on a file.
 *
 * @param[in] fileName The path to the file on which to retrieve information
 * @param[in] mode The specific file stat to get.
 * @param[in] info A pointer to the buffer in which to return the indicated file stat info.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileGetStat(const char* fileName,
        rmf_osal_FileStatMode mode, rmf_osal_FileInfo *info);

/**
 * @brief This function is used to set some file status info on a file.
 *
 * @param[in] fileName The path to the file on which to update its status information.
 * @param[in] mode The specific file stat to set.
 * @param[in] info A pointer to the buffer from which to set the indicated file stat info.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileSetStat(const char* fileName,
        rmf_osal_FileStatMode mode, rmf_osal_FileInfo *info);

/**
 * @brief This function is used to retrieve some file status info on a file previously opened with rmf_osal_FileOpen().
 *
 * @param[in] handle An rmf_osal_File handle, previously returned by rmf_osal_FileOpen().
 * @param[in] mode The specific file stat to get.
 * @param[in] info A pointer to the buffer in which to return the indicated file stat info.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileGetFStat(rmf_osal_File handle, rmf_osal_FileStatMode mode,
                                       rmf_osal_FileInfo *info);

/**
 * @brief This function is used to set some file status info on a file previously opened with rmf_osal_FileOpen().
 *
 * @param[in] handle An rmf_osal_File handle, previously returned by rmf_osal_FileOpen().
 * @param[in] mode The specific file stat to get.
 * @param[in] info A pointer to the buffer from which to set the indicated file stat info.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileSetFStat(rmf_osal_File handle, rmf_osal_FileStatMode mode,
        rmf_osal_FileInfo *info);

/**
 * @brief This function is used to delete the specific file from the the file system namespace.
 *
 * @param[in] fileName The path to the file to delete.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileDelete(const char* fileName);

/**
 * @brief This function is used to rename or moves the specific file in the file system namespace.
 *
 * @param[in] oldName The path to the file to rename or move.
 * @param[in] newName The new path and/or name for the file.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileRename(const char* oldName, const char* newName);

/**
 * @brief This function is used to open a directory in the file system namespace.
 *
 * @param[in] name The path to the directory to open.
 * @param[in] handle An pointer to an rmf_osal_Dir handle, through which the opened
 *   directory is returned.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_dirOpen(const char *name, rmf_osal_Dir* handle);


/**
 * @brief This function is used to read the contents of a directory previously opened with rmf_osal_DirOpen.
 * This can be used to iterate through the contents a directory in the file system namespace.
 *
 * @param[in] handle An rmf_osal_Dir handle, previously returned by rmf_osal_DirOpen().
 * @param[in] dirEnt A pointer to a rmf_osal_DirEntry object.  On return, this will contain
 *                   data about a directory entry.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_dirRead (rmf_osal_Dir handle, rmf_osal_DirEntry* dirEnt);

/**
 * @brief This function is used to close a directory previously opened with rmf_osal_DirOpen.
 *
 * @param[in] handle An rmf_osal_Dir handle, previously returned by rmf_osal_DirOpen().
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_dirClose(rmf_osal_Dir handle);

/**
 * @brief This function is use to create the specific directory in the file system namespace.
 *
 * @param[in] dirName The path to the directory to create.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */

rmf_Error rmf_osal_dirCreate(const char* dirName);

/**
 * @brief This function is used to delete a directory from the file system namespace.
 *
 * @param[in] dirName The path to the directory to delete.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_dirDelete(const char* dirName);

/**
 * @brief This function is used to rename or moves the specific directory in the file system namespace.
 *
 * @param[in] oldName The path to the directory to rename or move.
 * @param[in] newName The new path and/or name for the directory.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_dirRename(const char* oldName, const char* newName);


/**
 * @brief This function is used to retrieve default directory.
 *
 * @param[in] buf Pointer to retrieve path of default directory.
 * @param[in] bufSz Size of the buf paramerter.
 *
 * @return Returns RMF_SUCCESS on success, else returns the error code.
 */
rmf_Error rmf_osal_fileGetDefaultDir(char *buf, uint32_t bufSz);

/** @} */ //end of doxygen tag OSAL_FILE_API

#ifdef __cplusplus
}
#endif

#endif /* _RMF_OSAL_FILE_H */


