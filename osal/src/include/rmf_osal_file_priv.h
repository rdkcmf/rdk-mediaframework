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

#ifndef _RMF_OSAL_FILE_PRIV_H_
#define _RMF_OSAL_FILE_PRIV_H_

/* max path length (os-specific) */
#define MAX_PATH                        1024
#define OS_FS_MAX_PATH					MAX_PATH

#define OS_FS_DEFAULT_SYS_DIR			"."
#define OS_FS_ENV_DEFAULT_SYS_DIR		"FS.DEFSYSDIR"
#define OS_FS_ENV_ROMFS_LIB_DIR			"FS.ROMFS.LIBDIR"
#define OS_FS_ROMFS_DLL_PREFIX			"romfs_"
#define OS_FS_ROMFS_DLL_SUFFIX			".so"

/* async thread polling period (for BFS version change checking, etc.) */
#define OS_FS_ASYNC_POLL_PERIOD_MS		(500)

#define RMF_OSAL_FS_ENV_DEFAULT_SYS_DIR      "FS.DEFSYSDIR"

/* Creates the directory name given, including any missing parent dirs */
int createFullPath(const char* pathname);

rmf_Error convertName(const char *name, char **newName);

#define rmf_osal_fileHDInit rmf_osal_fileInit
#define rmf_osal_fileHDDirOpen rmf_osal_dirOpen
#define rmf_osal_fileHDFileOpen rmf_osal_fileOpen
#define rmf_osal_fileHDFileGetFStat rmf_osal_fileGetFStat
#define rmf_osal_fileHDFileSetFStat rmf_osal_fileSetFStat
#define rmf_osal_fileHDFileGetStat rmf_osal_fileGetStat
#define rmf_osal_fileHDFileSetStat rmf_osal_fileSetStat
#define rmf_osal_fileHDFileClose rmf_osal_fileClose
#define rmf_osal_fileHDFileRename rmf_osal_fileRename
#define rmf_osal_fileHDDirRead rmf_osal_dirRead
#define rmf_osal_fileHDDirClose rmf_osal_dirClose
#define rmf_osal_fileHDFileWrite rmf_osal_fileWrite
#define rmf_osal_fileHDFileRead rmf_osal_fileRead
#define rmf_osal_fileHDFileSeek  rmf_osal_fileSeek 
#define rmf_osal_fileHDFileDelete  rmf_osal_fileDelete
#define rmf_osal_fileHDDirDelete rmf_osal_dirDelete
#define rmf_osal_fileHDDirCreate rmf_osal_dirCreate
#define rmf_osal_fileHDDirRename rmf_osal_dirRename

#endif /* _RMF_OSAL_FILE_PRIV_H_ */

