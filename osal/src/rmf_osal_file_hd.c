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


#include <string.h>
#include <sys/types.h> /* fstat(2),lseek(2),open(2) */
#include <sys/sysmacros.h>
#include <sys/stat.h>  /* fstat(2),open(2) */
#include <sys/sysmacros.h> /* major(3), minor(3) */
#include <fcntl.h>     /* open(2) */
#include <unistd.h>    /* close(2),lseek(2),read(2) */
#include <dirent.h>     // directory stuff
#include <errno.h>
#include <stdio.h>

#include <rmf_osal_file.h>
#include <rmf_osal_file_priv.h>
#include <rmf_osal_error.h>
#include <rmf_osal_mem.h>
#include <rmf_osal_time.h>
#include "rdk_debug.h"
#include <rmf_osal_mem.h>
#include <rmf_osal_sync.h>
#include <rmf_osal_util.h>

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define RMF_OSAL_MEM_DEFAULT RMF_OSAL_MEM_FILE

#define MAX_OPEN_DIRECTORIES 10

#if 0
rmf_osal_filesys_ftable_t port_fileHDFTable;
#endif

static rmf_osal_Mutex g_hdMutex;
typedef struct
{
    rmf_osal_Bool opened;         // 1=yes 0=no
    char path[RMF_OSAL_FS_MAX_PATH+1];   // place to keep the name of the directory
    DIR *  fsHandle;    // file system handle from opendur
} DirHandle;

static DirHandle gOpenDirs[ MAX_OPEN_DIRECTORIES ];

static DirHandle *getDirHandle( DIR* dp, char *path )
{
    int i;

    if (NULL == path || strlen(path) > RMF_OSAL_FS_MAX_PATH)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Invalid params\n", __FUNCTION__);
        return NULL;
    }
    if ( rmf_osal_mutexAcquire(g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not grab mutex\n", __FUNCTION__);
        return NULL;
    }

    for (i=0; i < MAX_OPEN_DIRECTORIES; i++)
    {
        if (gOpenDirs[i].opened == FALSE)
        {
            gOpenDirs[i].fsHandle = dp;
            gOpenDirs[i].opened = TRUE;
            memset(gOpenDirs[i].path, '\0', sizeof(gOpenDirs[i].path) );
            strncpy(gOpenDirs[i].path, path, sizeof(gOpenDirs[i].path) -1 );
            break;  
        }
    }

    if ( rmf_osal_mutexRelease(g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not release mutex\n", __FUNCTION__);
        return NULL;
    }

    if (i == MAX_OPEN_DIRECTORIES)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.FILESYS",
                "%s() - Could not open directory - max=%d\n", __FUNCTION__, i);
        return NULL;
    }

    return &gOpenDirs[i];
}

static void freeDirHandle( DirHandle *dp )
{
    if ( rmf_osal_mutexAcquire(g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not grab mutex\n", __FUNCTION__);
        return ;
    }
    dp->opened = FALSE;
    if ( rmf_osal_mutexRelease(g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not release mutex\n", __FUNCTION__);
        return ;
    }
}

typedef struct rmf_osal_fileHD
{
    rmf_osal_Bool isBuffered;
    union{
        int fd;
        FILE *fp;
    }data;
} rmf_osal_fileHD;

typedef struct rmf_osal_dirHD
{
    int fd;
} rmf_osal_dirHD;

// RMF_OSAL private function called by other mpeos modules for this port
// Ensures that the full path specified exists
int createFullPath(const char* pathname)
{
    char mypath[RMF_OSAL_FS_MAX_PATH];
    const char* walker = pathname;
    rmf_Error err;

    // Skip past the root
    if (pathname[0] == RMF_OSAL_FS_SEPARATION_CHAR)
        walker ++;
    
    // Walk the string looking for path elements and ensure that each
    // is created
    while ((walker = strchr(walker,RMF_OSAL_FS_SEPARATION_CHAR)) != NULL)
    {
        strncpy(mypath,pathname,walker-pathname);
        mypath[walker-pathname] = '\0';

        err = rmf_osal_dirCreate(mypath);
        if (err != RMF_SUCCESS && err != RMF_OSAL_FS_ERROR_ALREADY_EXISTS)
        {
            return -1;
        }
        walker++;
    }

    // Finally, create the last part of the path
    if (pathname[strlen(pathname)-1] != RMF_OSAL_FS_SEPARATION_CHAR)
    {
        err = rmf_osal_dirCreate(pathname);
        if (err != RMF_SUCCESS && err != RMF_OSAL_FS_ERROR_ALREADY_EXISTS)
        {
            return -1;
        }
    }
    
    return 0;
}

/*
 * Public Functions
 */

/*
 * Some filesystems require that we prefix the original pathname
 * (e.g., "/itfs").  This is not necessary (at this time) for this
 * POSIX1 implementation.
 */
rmf_Error convertName(const char *name, char **newName)
{
    char *full_name;
    int mpeRet;
    size_t newNameSz;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:convertName() name: '%s'\n", name);

    newNameSz = strlen(name) + 1;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:convertName() allocating storage for %d characters\n", 
             newNameSz);

    mpeRet = rmf_osal_memAllocPGen( RMF_OSAL_MEM_FILE,  newNameSz, (void **) &full_name);
    if ( RMF_SUCCESS != mpeRet )
    {
        return RMF_OSAL_FS_ERROR_OUT_OF_MEM;
    }

    (void) strcpy(full_name, name);

    if ( '/' == full_name[strlen(full_name)-1] )
    {
        full_name[strlen(full_name)-1] = '\0';
    }

    *newName = full_name;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:convertName() *newName: '%s'\n", *newName);
    return RMF_SUCCESS;
}

/*
 * File-Scope Functions.  Those that map to function-pointer
 * table entries are listed first and in the order in which
 * they appear in the function table.
 */

/*
 * <i>rmf_osal_fileHDInit()</i>
 *
 * Implementation complete.
 */
void rmf_osal_fileHDInit()
{
    int i;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDInit() \n");

    if (rmf_osal_mutexNew(&g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not create overall mutex\n", __FUNCTION__);
        return;
    }


    if ( rmf_osal_mutexAcquire(g_hdMutex) != RMF_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not grab mutex\n", __FUNCTION__);
        return;
    }

    for (i=0; i < MAX_OPEN_DIRECTORIES; i++)
        gOpenDirs[i].opened = FALSE;

    if ( rmf_osal_mutexRelease(g_hdMutex) != RMF_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not release mutex\n", __FUNCTION__);
        return;
    }

    return;
}

/*
 * <i>rmf_osal_fileHDFileOpen()</i>
 *
 * Opens a file from a writable persistent file system.</br>
 * Wraps POSIX.1 <i>open(2)</i> function.
 *
 * @param name Pathname of the file to be opened.
 * @param openMode Type of access requested (read, write, create, truncate, append).
 * @param handle Upon success, points to the opened file.
 *
 * @return The error code if the operation fails, otherwise
 *     <i>RMF_SUCCESS</i> is returned.  Note well that an error code
 *     is returned if the specified file is a directory.  In that case,
 *     <i>rmf_osal_fileHDDirOpen()</i> should be used to open the directory.
 */
rmf_Error rmf_osal_fileHDFileOpen(const char* name, rmf_osal_FileOpenMode openMode,
        rmf_osal_File* handle)
{
    struct stat buf;
    rmf_Error ec;
    char *full_name;
    rmf_osal_fileHD *hdHandle;
    int localFd;
    int oflag = 0;
    int statRet;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileOpen()     name: '%s'\n", name);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileOpen() openMode: 0x%x\n", openMode);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileOpen()   handle: %p (addr)\n", handle);

    ec = convertName(name, &full_name);
    if ( RMF_SUCCESS != ec )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileOpen() convertName() failed: %d\n",
                ec);
        return ec;
    }

    if ( openMode & RMF_OSAL_FS_OPEN_MUST_CREATE)
    {
        oflag |= O_CREAT;
        oflag |= O_EXCL;
    }
    else if ( openMode & RMF_OSAL_FS_OPEN_CAN_CREATE)
    {
        oflag |= O_CREAT;
    }

    if ((openMode & RMF_OSAL_FS_OPEN_READWRITE) || ((openMode & RMF_OSAL_FS_OPEN_READ)
           && (openMode & RMF_OSAL_FS_OPEN_WRITE)) )
    {
        oflag |= O_RDWR;
    }
    else if ( openMode & RMF_OSAL_FS_OPEN_WRITE )
    {
        oflag |= O_WRONLY;
    }
    else
    {
        oflag |= O_RDONLY;
    }

    if (openMode & RMF_OSAL_FS_OPEN_TRUNCATE)
    {
        oflag |= O_TRUNC;
    }

    if (openMode & RMF_OSAL_FS_OPEN_APPEND)
    {
         oflag |= O_APPEND;
    } 

    ec = rmf_osal_memAllocPGen( RMF_OSAL_MEM_FILE,  sizeof(rmf_osal_fileHD), (void **) &hdHandle);
    if ( RMF_SUCCESS != ec )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileOpen() RMF_OSAL_FS_ERROR_OUT_OF_MEM\n");
        rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, full_name);
        return RMF_OSAL_FS_ERROR_OUT_OF_MEM;
    }

    localFd = open((const char *) full_name, oflag, S_IRUSR|S_IWUSR|S_IRGRP
           | S_IWGRP|S_IROTH|S_IWOTH);
    rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, full_name);
    if ( -1 == localFd )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileOpen() open(%s) failed errno: %s\n",
                name, strerror(errno));
        rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, hdHandle);
        if (errno == EEXIST)
            return RMF_OSAL_FS_ERROR_ALREADY_EXISTS;
        
        return RMF_OSAL_FS_ERROR_FAILURE;
    }

    /*
     * We must return an error condition if the specified file is
     * a directory.  This is not explicitly stated in any document
     * I can find, but appears to be a "de facto" requirement
     * (confirmed by phone conversation with Tom Brasier 051116).
     *
     * If the call to fstat() fails, we ignore the error.
     */
    statRet = fstat(localFd, &buf);
    if ( -1 == statRet )
    {
        RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileOpen() dir-check fstat() failed; ignoring error ...\n");
    }
    else
    {
        if ( S_ISDIR(buf.st_mode) )
        {
            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.FILESYS",
                    "HDFILESYS:rmf_osal_fileHDFileOpen() specified file is a directory; return error code ...\n");
            close(localFd);
            rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, hdHandle);
            return RMF_OSAL_FS_ERROR_FAILURE;
        }
    }

    // Make sure that the file descriptor is not inherited by child processes
    if (fcntl(localFd, F_SETFD, FD_CLOEXEC) != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                  "HDFILESYS:rmf_osal_fileHDFileOpen() failed to set FD_CLOEXEC flag on fd: %d\n",
                  localFd);
        close(localFd);
        rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, hdHandle);
        return RMF_OSAL_FS_ERROR_FAILURE;
    }

    hdHandle->data.fd = localFd;
    hdHandle->isBuffered = FALSE;
    *handle = (rmf_osal_File *) hdHandle;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileOpen() open successful (descriptor %d)\n",
            hdHandle->data.fd);

    return RMF_SUCCESS;
}

const char* convertFileAttributes(rmf_osal_FileOpenMode openMode)
{
    if(openMode & RMF_OSAL_FS_OPEN_APPEND)
    {
        if((openMode & RMF_OSAL_FS_OPEN_READWRITE) || (openMode & RMF_OSAL_FS_OPEN_READ))
        {
            return "a+";
        }
        else
        {
            return "a";
        }
    }
    else if((openMode & RMF_OSAL_FS_OPEN_READWRITE) ||
            ((openMode & RMF_OSAL_FS_OPEN_READ) && (openMode & RMF_OSAL_FS_OPEN_WRITE)))
    {
        return "r+";
    }
    else if(openMode & RMF_OSAL_FS_OPEN_WRITE)
    {
        return "w";
    }
    else
    {
        //Assume openMode = RMF_OSAL_FS_OPEN_READ 
        return "r";
    }
}

rmf_Error rmf_osal_fileOpenBuffered(const char* name, rmf_osal_FileOpenMode openMode,
        rmf_osal_File* handle)
{
    int fd = 0;
    rmf_Error ret = rmf_osal_fileHDFileOpen(name, openMode, handle);
    if(RMF_SUCCESS != ret)
    {
        return ret;
    }

    rmf_osal_fileHD *hdl = (rmf_osal_fileHD *)(*handle);
    fd = hdl->data.fd;
    hdl->data.fp = fdopen(fd , convertFileAttributes(openMode));
    if(NULL == hdl->data.fp)
    {
        int myerr = errno;

        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.FILESYS",
                "rmf_osal_fileOpenBuffered() failed in fdopen(fd %d, err 0x%x, %s). Will continue unbuffered.\n", fd, myerr, strerror(myerr));
        hdl->data.fd = fd;
        return RMF_SUCCESS;
    }

    hdl->isBuffered = TRUE;
    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileOpenBuffered() open successful (fp 0x%x)\n",
            hdl->data.fd);

    return RMF_SUCCESS;
}


/*
 * <i>rmf_osal_fileHDFileClose</i>
 *
 * Closes a file previously opened with <i>rmf_osal_fileHDFileOpen()</i>.</br>
 * Wraps POSIX.1 <i>close(2)</i> function.
 *
 * @param handle  RMF_OSAL HD file handle, which must have been provided by
 *                a previous call to <i>rmf_osal_fileHDFileOpen()</i>.
 *
 * @return The error code if the operation fails, otherwise
 *     <i>RMF_SUCCESS</i> is returned.
 */
rmf_Error rmf_osal_fileHDFileClose(rmf_osal_File handle)
{
    int closeRet;
    int mpeRet;
    rmf_osal_fileHD *hdHandle = (rmf_osal_fileHD*) handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileClose() handle: %p (addr)\n", handle);

    if ( NULL == handle )
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileClose() RMF_OSAL_FS_ERROR_INVALID_PARAMETER (NULL handle)\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }
	
    if(TRUE == hdHandle->isBuffered)
    {
        closeRet = fclose(hdHandle->data.fp);
    }
    else
    {
        closeRet = close(hdHandle->data.fd);
    }

    mpeRet = rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, handle);
    if ( RMF_SUCCESS != mpeRet )
    {
        return RMF_OSAL_FS_ERROR_FAILURE;
    }

    if ( -1 == closeRet )
    {
        return RMF_OSAL_FS_ERROR_FAILURE;
    }

    handle = NULL;
    return RMF_SUCCESS;
}
/*
 * <i>rmf_osal_fileHDFileRead</i>
 *
 * Reads data from specified hard-disk file.</br>
 * Wraps POSIX.1 <i>read(2)</i> function.
 *
 * @param handle  RMF_OSAL HD file handle, which must have been provided by
 *                a previous call to <i>rmf_osal_fileHDFileOpen()</i>.
 * @param count  A pointer to a byte-count.  On entry, this must point to
 *               the number of bytes to attempt to read.  Upon return,
 *               this will point to the number of bytes actually read.
 * @param buffer A pointer to the buffer that will receive the data.
 *               The buffer size must be greater than or equal to the
 *               entry-point value of <i>count</i>.
 *
 *
 */
rmf_Error rmf_osal_fileHDFileRead(rmf_osal_File handle, uint32_t *count,
                                   void* buffer)
{
    int bytesRead = 0;
    rmf_osal_fileHD *hdHandle = (rmf_osal_fileHD*) handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileRead() handle: %p (addr)\n", handle);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileRead()  count: %p (addr)\n", count);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileRead() buffer: %p (addr)\n", buffer);

    if ( (NULL == handle) || (NULL == count) || (NULL == buffer) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileRead() RMF_OSAL_FS_ERROR_INVALID_PARAMETER\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    if ( 0 == *count )
    {
        return RMF_SUCCESS;
    }
    
    if(TRUE == hdHandle->isBuffered)
    {
        bytesRead = fread(buffer, 1, (size_t)*count, hdHandle->data.fp);
    }
    else
    {
        bytesRead = read(hdHandle->data.fd, buffer, (unsigned int) *count);
    }
    if ( -1 == bytesRead )
    {
        if ( EISDIR == errno )
        {
            RDK_LOG(
                    RDK_LOG_ERROR,
                    "LOG.RDK.FILESYS",
                    "HDFILESYS:rmf_osal_fileHDFileRead() read() from a directory ... (errno: %d) ... do not treat as an error ...\n",
                    errno);
            bytesRead = 0;
        }
        else
        {
            RDK_LOG(
                    RDK_LOG_ERROR,
                    "LOG.RDK.FILESYS",
                    "HDFILESYS:rmf_osal_fileHDFileRead() read() failed (errno: %d)\n",
                    errno);
            return RMF_OSAL_FS_ERROR_DEVICE_FAILURE;
        }
    }
    *count = (uint32_t) bytesRead;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileRead() successful read (*count: %d)\n",
            *count);
    return RMF_SUCCESS;
}
/*
 *  - <i>rmf_osal_fileHDFileWrite</i>
 */
rmf_Error rmf_osal_fileHDFileWrite(rmf_osal_File handle, uint32_t *count,
                                    void* buffer)
{
    int bytesWritten = 0;
    rmf_osal_fileHD *hdHandle = (rmf_osal_fileHD*) handle;
    rmf_Error retval = RMF_SUCCESS;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileWrite\n\thandle:%p (addr)\n\tcount: %p (addr)\n\tbuffer: %p (addr)\n",
            handle, count, buffer);

    if ( (NULL == handle) || (NULL == count) || (NULL == buffer) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileWrite() RMF_OSAL_FS_ERROR_INVALID_PARAMETER\n");
        retval = RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }
    else if ( 0 != *count )
    {
        if(TRUE == hdHandle->isBuffered)
        {
            bytesWritten = fwrite(buffer, 1, (size_t)*count, hdHandle->data.fp);
        }
        else
        {
            bytesWritten = write(hdHandle->data.fd, buffer, (unsigned int) *count);
        }

        if ( -1 != bytesWritten )
        {
            *count = (uint32_t) bytesWritten;
            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.FILESYS",
                    "HDFILESYS:rmf_osal_fileHDFileWrite() successful write (*count: %d)\n",
                    *count);
        }
        else
        {
            RDK_LOG(
                    RDK_LOG_ERROR,
                    "LOG.RDK.FILESYS",
                    "HDFILESYS:rmf_osal_fileHDFileWrite() write failed (errno: %d)\n",
                    errno);
            *count = 0;
            retval = RMF_OSAL_FS_ERROR_DEVICE_FAILURE;
        }
    }
    return(retval);
}

/*
 * <i>rmf_osal_fileHDFileSeek</i>
 *
 * Repositions read/write file offset.</br>
 * Wraps POSIX.1 <i>lseek(2)</i> function.
 *
 * @param handle  RMF_OSAL HD file handle, which must have been provided by
 *                a previous call to <i>rmf_osal_fileHDFileOpen()</i>.
 * @param seekMode  One of:  <i>RMF_OSAL_FS_SEEK_CUR</i>, <i>RMF_OSAL_FS_SEEK_END</i>,
                    <i>RMF_OSAL_FS_SEEK_SET</i>.
 * @param offset  New offset.
 *
 * @return The error code if the operation fails, otherwise
 *         <i>RMF_SUCCESS</i> is returned.
 */
rmf_Error rmf_osal_fileHDFileSeek(rmf_osal_File handle, rmf_osal_FileSeekMode seekMode,
                                   int64_t* offset)
{
    rmf_osal_fileHD *hdHandle = (rmf_osal_fileHD*) handle;
    off_t lseekOffset = 0;
    off_t lseekRet;
    int lseekWhence;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileSeek()      handle: %p (addr)\n", handle);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileSeek()    seekMode: 0x%x\n", seekMode);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileSeek()      offset: %p (addr)\n", offset);

    if ( NULL == handle )
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileSeek() RMF_OSAL_FS_ERROR_INVALID_PARAMETER (NULL handle)\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    switch ( seekMode )
    {
    case RMF_OSAL_FS_SEEK_CUR:
        lseekOffset=(off_t)*offset; 
        lseekWhence = SEEK_CUR;
        break;
    case RMF_OSAL_FS_SEEK_END:
        lseekWhence = SEEK_END;
        break;
    case RMF_OSAL_FS_SEEK_SET:
        lseekOffset = (off_t) *offset;
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileSeek() current offset: 0x%x\n",
                *offset);
        lseekWhence = SEEK_SET;
        break;
    default:
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileSeek() RMF_OSAL_FS_ERROR_INVALID_PARAMETER (invalid seekMode)\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    lseekRet = lseek(hdHandle->data.fd, lseekOffset, lseekWhence);
    if ( (off_t) -1 == lseekRet )
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileSeek() lseek() failed (errno: %d)\n",
                errno);
        return RMF_OSAL_FS_ERROR_DEVICE_FAILURE;
    }

    *offset = lseekRet;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileSeek() new offset: 0x%x\n", *offset);

    return RMF_SUCCESS;
}


rmf_Error rmf_osal_fileHDFileSync(rmf_osal_File handle)
{
    rmf_Error retval = RMF_SUCCESS;

            printf("HDFILESYS:rmf_osal_fileHDFileSync() STUB\n");
    return (retval);
}

/*
 * Gets information about the specified file.</br>
 * Wraps POSIX.1 <i>stat(2)</i> function.
 *
 * @param fileName  Pathname of file to research
 * @param mode  To be decide
 * @param info A pointer to storage for file status information.
 *
 * @return The error code if the operation fails, otherwise
 *         <i>RMF_SUCCESS</i> is returned.
 */
rmf_Error rmf_osal_fileHDFileGetStat(const char* fileName,
        rmf_osal_FileStatMode mode, rmf_osal_FileInfo *info)
{
    struct stat buf;
    rmf_Error retval = RMF_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetStat() fileName: %s\n", fileName);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetStat()     mode: 0x%x\n", mode);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetStat()     info: %p (addr)\n", info);

    if (fileName == NULL || info == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileGetStat() RMF_OSAL_FS_ERROR_INVALID_PARAMETER\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    // Ensure the file or directory exists
    if (stat(fileName, &buf) == -1)
        return RMF_OSAL_FS_ERROR_NOT_FOUND;

    switch((int) mode)
    {
    case RMF_OSAL_FS_STAT_TYPE:
        if (S_ISDIR(buf.st_mode))
            info->type = RMF_OSAL_FS_TYPE_DIR;
        else if (S_ISREG(buf.st_mode))
            info->type = RMF_OSAL_FS_TYPE_FILE;
        else
            info->type = RMF_OSAL_FS_TYPE_OTHER;
        break;

    case RMF_OSAL_FS_STAT_SIZE:
        info->size = (uint64_t) buf.st_size;
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileGetStat() info->size: %d (dec)\n",
                info->size);
        break;

    case RMF_OSAL_FS_STAT_MODDATE:
        info->modDate = buf.st_mtime;
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileGetStat()   info->modDate: 0x%lu\n",
                info->modDate);
        break;

    case RMF_OSAL_FS_STAT_CREATEDATE:
        info->createDate = buf.st_ctime;
        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileGetStat()   info->createDate: 0x%lu\n",
                info->createDate);
        break;

    default:
        printf("HDFILESYS:rmf_osal_fileHDFileGetStat STUB: unknown mode: %d\n",
                mode);
        retval = RMF_OSAL_FS_ERROR_UNSUPPORT;
        break;
    }

    return (retval);
}

rmf_Error rmf_osal_fileHDFileSetFStat(rmf_osal_File handle, rmf_osal_FileStatMode mode,
        rmf_osal_FileInfo *info)
{
    rmf_osal_fileHD *hdHandle = (rmf_osal_fileHD*) handle;

    if (info == NULL || hdHandle == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileSetFStat() RMF_OSAL_FS_ERROR_INVALID_PARAMETER\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    switch (mode)
    {
    /* set file size */
    case RMF_OSAL_FS_STAT_SIZE:
        if (ftruncate64(hdHandle->data.fd, info->size) == -1)
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileSetFStat() ftruncate64 failed %s\n", strerror(errno));
              return RMF_OSAL_FS_ERROR_FAILURE;
        }
        break;

    default:
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileSetFStat() Unsupported stat (%d)\n",
                mode);
        return RMF_OSAL_FS_ERROR_FAILURE;
    }

    return RMF_SUCCESS;
}

rmf_Error rmf_osal_fileHDFileSetStat(const char* fileName,
        rmf_osal_FileStatMode mode, rmf_osal_FileInfo *info)
{
	rmf_Error err = RMF_SUCCESS;
    struct stat buf;
    
    if (fileName == NULL || info == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileGetStat() RMF_OSAL_FS_ERROR_INVALID_PARAMETER\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    // Ensure the file or directory exists
    if (stat(fileName, &buf) == -1)
        return RMF_OSAL_FS_ERROR_NOT_FOUND;

	switch(mode)
	{
        /* set file type */
	    case RMF_OSAL_FS_STAT_TYPE:
            /* The user should not be able to set this */
			err = RMF_OSAL_FS_ERROR_READ_ONLY;
            break; 
#if 0
		/* set file known (parent directory loaded) status */
		case RMF_OSAL_FS_STAT_ISKNOWN:
			err = RMF_OSAL_FS_ERROR_READ_ONLY;
			break; 
#endif
		/* set file create date */
		case RMF_OSAL_FS_STAT_CREATEDATE:
			err = RMF_OSAL_FS_ERROR_READ_ONLY;
			break; 

		/* set file last-modified date */
		case RMF_OSAL_FS_STAT_MODDATE:
			err = RMF_OSAL_FS_ERROR_READ_ONLY;
			break; 

		/* unknown setstat call */
		default:
		{
			rmf_osal_File h;

			/* open a handle to the requested file (for now) */
			if ((err = rmf_osal_fileHDFileOpen( fileName, RMF_OSAL_FS_OPEN_WRITE, &h))
			    == RMF_SUCCESS)
			{
				/* call SetFStat */
				err = rmf_osal_fileHDFileSetFStat(h,mode,info);
				(void)rmf_osal_fileHDFileClose(h);
			}
		}
		break; 
	}

	return err; 
}

/*
 * <i>rmf_osal_fileHDFileGetFStat</i>
 *
 * Gets the size of the specified file.</br>
 * Wraps POSIX.1 <i>fstat(2)</i> function.
 *
 * @param handle  RMF_OSAL HD file handle, which must have been provided by
 *                a previous call to <i>rmf_osal_fileHDFileOpen()</i>.
 * @param mode  Must be:  <i>RMF_OSAL_FS_STAT_SIZE</i>.
 * @param info A pointer to storage for file status information.
 *
 * @return The error code if the operation fails, otherwise
 *         <i>RMF_SUCCESS</i> is returned.
 */
rmf_Error rmf_osal_fileHDFileGetFStat(rmf_osal_File handle, rmf_osal_FileStatMode mode,
                                       rmf_osal_FileInfo *info)
{
    struct stat buf;
    rmf_osal_fileHD *hdHandle = (rmf_osal_fileHD*) handle;
    int statRet;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat() handle: %p (addr)\n", handle);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()   mode: 0x%x\n", mode);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()   info: %p (addr)\n", info);

    if ( (NULL == handle) || (NULL == info) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDFileGetFStat() RMF_OSAL_FS_ERROR_INVALID_PARAMETER\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    if ( RMF_OSAL_FS_STAT_SIZE != mode )
    {
        return RMF_OSAL_FS_ERROR_UNSUPPORT;
    }

    statRet = fstat(hdHandle->data.fd, &buf);
    if ( -1 == statRet )
    {
        return RMF_OSAL_FS_ERROR_FAILURE;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()     buf.st_dev: major(0x%x) minor(0x%x)\n",
              major(buf.st_dev), minor(buf.st_dev));
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()     buf.st_ino: 0x%lx\n",
            buf.st_ino);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()    buf.st_mode: 0x%x\n",
            buf.st_mode);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()   buf.st_nlink: 0x%x\n",
            buf.st_nlink);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()     buf.st_uid: 0x%x\n",
            buf.st_uid);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()     buf.st_gid: 0x%x\n",
            buf.st_gid);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()    buf.st_rdev: major(0x%x) minor(0x%x)\n",
              major(buf.st_rdev), minor(buf.st_rdev));
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()    buf.st_size: %lu (dec)\n",
            buf.st_size);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()   buf.st_atime: 0x%lx\n",
            buf.st_atime);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()   buf.st_mtime: 0x%lx\n",
            buf.st_mtime);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()   buf.st_ctime: 0x%lx\n",
            buf.st_ctime);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat() buf.st_blksize: 0x%lx\n",
            buf.st_blksize);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat()  buf.st_blocks: 0x%lx\n",
            buf.st_blocks);

    info->size = (uint64_t) buf.st_size;
    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileGetFStat() successful info->size: %llu (dec)\n",
            info->size);
    return RMF_SUCCESS;
}


rmf_Error rmf_osal_fileHDFileDelete(const char* fileName)
{
    rmf_Error retval = RMF_SUCCESS;
    if (0 == fileName)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS::rmf_osal_fileHDFileDelete - NULL fileName!\n");
        return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileDelete() fileName: \"%s\"\n", fileName);
    errno = 0;
    if (0 != unlink(fileName))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS::rmf_osal_fileHDFileDelete - unlink() set errno %d (%s)\n",
                errno, strerror(errno));
        retval = RMF_OSAL_FS_ERROR_FAILURE;
    }
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileDelete(\"%s\") returns: %d\n", fileName,
            retval);

    return (retval);
}

rmf_Error rmf_osal_fileHDFileRename(const char* oldName, const char* newName)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDFileRename(<%s>,<%s>\n", oldName, newName);

    if ( -1 == rename( oldName, newName ) )
    {
        int ccode = errno;
        switch ( ccode )
        {
        default:
            return RMF_OSAL_FS_ERROR_INVALID_PARAMETER;

        case EBUSY:
        case ENOSPC:
            return RMF_OSAL_FS_ERROR_DEVICE_FAILURE;

        case ENOENT:
        case ENOTDIR:
            return RMF_OSAL_FS_ERROR_NOT_FOUND;

        case ENOMEM: 
            return RMF_OSAL_FS_ERROR_OUT_OF_MEM;

        }
    }

    return RMF_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * <i>rmf_osal_fileHDDirOpen()</i>
 *
 * Opens a directory from a writable persistent file system.</br>
 * Wraps POSIX.1 <i>open(2)</i> function.
 *
 * @param name Pathname of the directory to be opened.
 * @param handle Upon success, points to the opened file.
 *
 * @return The error code if the operation fails, otherwise
 *     <i>RMF_SUCCESS</i> is returned.
 */
rmf_Error rmf_osal_fileHDDirOpen(const char *name, rmf_osal_Dir* handle)
{
    rmf_Error retval = RMF_SUCCESS;
    int ccode;
    char *full_name = 0;
    DIR *pDir = 0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDDirOpen() name:<%s> handle@%p\n", name,
            handle);

    retval = convertName(name, &full_name);
    if ( RMF_SUCCESS == retval )
    {
        if ( (pDir = opendir( full_name )) != 0 )
        {
// 20061206 - ABD fixed bug while working on BCM97456 DVR Playback
//            rmf_osal_memFreeP(RMF_OSAL_MEM_FILE, full_name);
// 20061206 - ABD fixed bug while working on BCM97456 DVR Playback
            *handle = (rmf_osal_File *)getDirHandle( pDir, full_name );
            if ( *handle==0 )
            {
                RDK_LOG(
                        RDK_LOG_ERROR,
                        "LOG.RDK.FILESYS",
                        "HDFILESYS:rmf_osal_fileHDDirOpen() getDirHandle failed - %d concurrently open directories max\n",
                        MAX_OPEN_DIRECTORIES);
                retval = RMF_OSAL_FS_ERROR_OUT_OF_MEM;
            }
            else
            {
                RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.FILESYS",
                        "HDFILESYS:rmf_osal_fileHDDirOpen() open successful (descriptor %p)\n",
                        *handle);
            }
        }
        else
        {
            ccode = errno;
            switch ( ccode )
            {
            case ENOTDIR:
            case ENOENT:
                retval = RMF_OSAL_FS_ERROR_NOT_FOUND;
                break;

            case ENOMEM:
                retval = RMF_OSAL_FS_ERROR_OUT_OF_MEM;
                break;

            case EACCES:
            case EMFILE:
            case ENFILE:
            default:
                retval = RMF_OSAL_FS_ERROR_DEVICE_FAILURE;
                break;
            }

            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",

            "HDFILESYS:rmf_osal_fileHDDirOpen() diropen(%s) failed: %d\n",
                    full_name, errno);
        }

        rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, full_name);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDDirOpen() convertName() failed: %d\n",
                retval);
    }

    return(retval);
}

rmf_Error rmf_osal_fileHDDirRead (rmf_osal_Dir handle, rmf_osal_DirEntry* dirEnt)
{
    rmf_Error retval = RMF_SUCCESS;
    struct dirent *pde = 0;
    DirHandle *dhp = (DirHandle *)handle;
    struct stat fileStat;
    int ccode;
    unsigned int uiBufferDirNameLen4Cat =0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDDirRead() handle@%p\n", handle);

/***
 *** 20061213 Porting-Projects BUG#16
 *** NOTE WELL: when returning the filename to the caller, we must remove
 ***     the directory prefix required by the call to stat(2)
 ***/

    if ( rmf_osal_mutexAcquire(g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not grab mutex\n", __FUNCTION__);
        return RMF_OSAL_FS_ERROR_FAILURE;
    }
    // prep the path for the full filename
    memset (dirEnt->name, '\0', sizeof(dirEnt->name) );
    strncpy( dirEnt->name, dhp->path,sizeof(dirEnt->name) -1 );
    if ( rmf_osal_mutexRelease(g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not release mutex\n", __FUNCTION__);
        return RMF_OSAL_FS_ERROR_FAILURE;
    }
    uiBufferDirNameLen4Cat = ((sizeof(dirEnt->name) - strnlen(dirEnt->name, sizeof(dirEnt->name))) > 1) ? (sizeof(dirEnt->name) - strnlen(dirEnt->name, sizeof(dirEnt->name))) : 1;
    strncat( dirEnt->name, RMF_OSAL_FS_SEPARATION_STRING, (uiBufferDirNameLen4Cat - 1));

    /* 
     * We want to remove any file attribute file names from the directory listing
     * so keep reading entries until we are either done or have found a
     * non-attribute file
     */
    const char * metadataFilePrefix = rmf_osal_envGet("OCAP.persistent.metadataFilePrefix");

    if(NULL != metadataFilePrefix)
    {
      while (1)
      {
          if ( rmf_osal_mutexAcquire(g_hdMutex) != RMF_SUCCESS)
          {
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                      "%s() - Could not grab mutex\n", __FUNCTION__);
              return RMF_OSAL_FS_ERROR_FAILURE;
          }
          pde = readdir( (DIR*) dhp->fsHandle );  // get the next file from this directory
          if ( rmf_osal_mutexRelease(g_hdMutex) != RMF_SUCCESS)
          {
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                      "%s() - Could not release mutex\n", __FUNCTION__);
              return RMF_OSAL_FS_ERROR_FAILURE;
          }

          if (pde > 0)
          {
              /* Skip .. and . entries */
              /* Also skip persistent file attributes file */
              if (strcmp(pde->d_name,"..") == 0 ||
                      strcmp(pde->d_name,".") == 0 ||
                      strstr(pde->d_name,metadataFilePrefix) != NULL)
              {
                  continue;
              }

              strcat (dirEnt->name, pde->d_name); // add the filename to the path for a complete path
              ccode = stat( dirEnt->name, &fileStat );
              if ( ccode == 0 )
              {
                  dirEnt->fileSize = fileStat.st_size;
                  dirEnt->isDir = S_ISDIR(fileStat.st_mode) ? TRUE : FALSE;
                  break;
              }
              else
              {
                  // failed to stat the file
                  ccode = errno;
                  switch ( ccode )
                  {
                      case EBADF:
                      case EFAULT:
                      case ELOOP:  
                      case ENAMETOOLONG:
                      case ENOTDIR:
                      case ENOENT: 
                          retval = RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
                          break;

                      case ENOMEM: 
                          retval = RMF_OSAL_FS_ERROR_OUT_OF_MEM;
                          break;

                      default: 
                          retval = RMF_OSAL_FS_ERROR_DEVICE_FAILURE;
                          break;

                  }
                  RDK_LOG(
                        RDK_LOG_ERROR,
                        "LOG.RDK.FILESYS",
                        "HDFILESYS:rmf_osal_fileHDDirRead() stat <%s> failed %d\n",
                        dirEnt->name, ccode);
                  break;
              }
          }
          else
          {
              retval = RMF_OSAL_FS_ERROR_NOT_FOUND;
              break;
          }
      }
    }
    else
    {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
             "HDFILESYS:rmf_osal_fileHDDirRead(): OCAP.persistent.metadataFilePrefix flag does not exist in rmfconfig.ini file \n");

      retval = RMF_OSAL_FS_ERROR_DEVICE_FAILURE;
    }
    
    if ( retval == RMF_SUCCESS )
    {
        dirEnt->name[0] = '\0';
        strcat (dirEnt->name, pde->d_name); // caller receives only the "short" filename
    }
    else
    {
        dirEnt->name[0] = '\0';
        dirEnt->fileSize = 0;
        dirEnt->isDir = FALSE;
    }

    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDDirRead(\"%s\") returns dirEnt->name: \"%s\"(0x%x)\n",
            dhp->path, dirEnt->name,  (unsigned int)dirEnt->name);

    return(retval);
}

/*
 * <i>rmf_osal_fileHDDirClose</i>
 *
 * Closes a directory previously opened with <i>rmf_osal_fileHDDirOpen()</i>.</br>
 * Wraps POSIX.1 <i>close(2)</i> function.
 *
 * @param handle  RMF_OSAL HD file handle, which must have been provided by
 *                a previous call to <i>rmf_osal_fileHDDirOpen()</i>.
 *
 * @return The error code if the operation fails, otherwise
 *     <i>RMF_SUCCESS</i> is returned.
 */
rmf_Error rmf_osal_fileHDDirClose(rmf_osal_Dir handle)
{
    rmf_Error retval = RMF_SUCCESS;
    DirHandle *dhp = (DirHandle *)handle;
    int ccode=0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDDirClose() handle: %p (addr)\n", handle);
    if ( rmf_osal_mutexAcquire(g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not grab mutex\n", __FUNCTION__);
        return RMF_OSAL_FS_ERROR_FAILURE;
    }
    if( dhp->opened )
    {
        ccode = closedir( dhp->fsHandle );
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS","HDFILESYS:rmf_osal_fileHDDirClose() WARNING staled DirHandle: %s\n",dhp->path);
    }
    if ( ccode == 0 )
    {
        freeDirHandle( dhp );
    }
    else
    {
        retval = RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "HDFILESYS:rmf_osal_fileHDDirClose() failed\n");
    }
    if ( rmf_osal_mutexRelease(g_hdMutex) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILESYS",
                "%s() - Could not release mutex\n", __FUNCTION__);
        return RMF_OSAL_FS_ERROR_FAILURE;
    }
    return(retval);
}

/*
 * <i>rmf_osal_fileHDDirDelete</i>
 *
*/
rmf_Error rmf_osal_fileHDDirDelete(const char* dirName)
{
    rmf_Error retval = RMF_SUCCESS;
    char *full_name = 0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDDirDelete<%s>\n",dirName);

    retval = convertName(dirName, &full_name);

    if (rmdir(full_name) != 0)
    {
        int errCode = errno;
       
        if (errCode != ENOENT)
        {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
                    "HDFILESYS:rmf_osal_fileHDDirDelete<%s> -- error = %d\n",
                    dirName, errCode);
            
            retval = RMF_OSAL_FS_ERROR_FAILURE;
        }
    }

    rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, full_name);

    return(retval);
}

rmf_Error rmf_osal_fileHDDirRename(const char* oldName, const char* newName)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDDirRename(<%s>,<%s>\n", oldName, newName);
    return rmf_osal_fileHDFileRename(oldName, newName);
}

rmf_Error rmf_osal_fileHDDirCreate(const char* dirName)
{
    rmf_Error retval = RMF_SUCCESS;
    char *full_name = 0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILESYS",
            "HDFILESYS:rmf_osal_fileHDDirCreate <%s>\n", dirName);

    retval = convertName(dirName, &full_name);
    if ( retval == RMF_SUCCESS )
    {
        if ( -1 == mkdir( full_name, 0755 ) )
        {
            int ccode = errno;
            switch ( ccode )
            {
            case ENOSPC:
                retval = RMF_OSAL_FS_ERROR_DEVICE_FAILURE;
                break;

            case ENOENT:
            case ENOTDIR:
                retval = RMF_OSAL_FS_ERROR_NOT_FOUND;
                break;

            case ENOMEM: 
                retval = RMF_OSAL_FS_ERROR_OUT_OF_MEM;
                break;

            case EEXIST:
                retval = RMF_OSAL_FS_ERROR_ALREADY_EXISTS;
                break;

            default:
                retval = RMF_OSAL_FS_ERROR_INVALID_PARAMETER;
                break;

            } // end error switch
        }
        rmf_osal_memFreePGen(RMF_OSAL_MEM_FILE, full_name);
    }

    return(retval);
}


