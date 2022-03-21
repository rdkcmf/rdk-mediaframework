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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rmf_osal_file.h"

#define RMF_OSAL_ASSERT_FILE(ret,msg) {\
	if (RMF_SUCCESS != ret) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}
#define TST_DIR "test_dir"
#define TST_FILE "testfile"
#define TST_FILE_PATH TST_DIR RMF_OSAL_FS_SEPARATION_STRING TST_FILE
#define TST_DIR1 "test_dir1"
#define TST_FILE1 "testfile1"
#define TST_FILE_PATH1_BEFORE_REN TST_DIR1 RMF_OSAL_FS_SEPARATION_STRING TST_FILE
#define TST_FILE_PATH1 TST_DIR1 RMF_OSAL_FS_SEPARATION_STRING TST_FILE1
#define TST_BUF_LEN 12

int rmf_osal_file_test()
{
	rmf_osal_File file_handle;
	rmf_osal_Dir dir_handle;
	rmf_Error ret;
	int64_t size;
	rmf_osal_FileInfo fileinfo;
	static rmf_osal_DirEntry dirEnt;
	int i;
	uint32_t count = TST_BUF_LEN;
	char write_buf[TST_BUF_LEN] = {0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF};
	char read_buf[TST_BUF_LEN] ;
	char default_dir[RMF_OSAL_FS_MAX_PATH];

	ret = rmf_osal_fileGetDefaultDir( default_dir, RMF_OSAL_FS_MAX_PATH);
	RMF_OSAL_ASSERT_FILE( ret, "rmf_osal_fileGetDefaultDir Failed"); 
	printf("Default directory is %s  path separator string is %s path separator char is %c\n", default_dir, RMF_OSAL_FS_SEPARATION_STRING, RMF_OSAL_FS_SEPARATION_CHAR);

	ret = rmf_osal_dirCreate( TST_DIR);
	RMF_OSAL_ASSERT_FILE( ret, "rmf_osal_dirCreate Failed"); 

	ret = rmf_osal_fileOpen ( TST_FILE_PATH ,RMF_OSAL_FS_OPEN_WRITE|RMF_OSAL_FS_OPEN_CAN_CREATE, &file_handle);
	RMF_OSAL_ASSERT_FILE( ret, "rmf_osal_fileOpen Failed"); 

	ret = rmf_osal_fileWrite( file_handle, &count, (void*)write_buf);
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileWrite Failed"); 
	printf("Written %d bytes to " TST_FILE_PATH"\n",count);

	ret = rmf_osal_fileClose( file_handle);
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileClose Failed"); 

	ret = rmf_osal_dirRename( TST_DIR , TST_DIR1);
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_dirRename Failed");

	ret = rmf_osal_fileRename ( TST_FILE_PATH1_BEFORE_REN ,TST_FILE_PATH1);
	RMF_OSAL_ASSERT_FILE( ret, "rmf_osal_fileRename Failed"); 

	ret = rmf_osal_fileOpen ( TST_FILE_PATH1 ,RMF_OSAL_FS_OPEN_READ|RMF_OSAL_FS_OPEN_WRITE, &file_handle);
	RMF_OSAL_ASSERT_FILE( ret, "rmf_osal_fileOpen Failed"); 

	ret = rmf_osal_fileRead( file_handle, &count, (void*)read_buf);
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileWrite Failed"); 
	printf("Read %d bytes to " TST_FILE_PATH1"\n",count);

	for ( i= 0; i< count; i++)
	{
		if (write_buf[i] != read_buf [i] )
		{
			printf("Error on verifying read write data\n");
			abort();
		}
	}
	printf("Verified read data against written data.\n"); 

	ret = rmf_osal_fileGetFStat( file_handle, RMF_OSAL_FS_STAT_SIZE, &fileinfo );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileSeek Failed"); 
	printf("Size got from fstat = %d\n", fileinfo.size);

	ret = rmf_osal_fileSeek( file_handle, RMF_OSAL_FS_SEEK_END, &size );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileSeek Failed"); 
	printf("Size got from seeking = %d\n", size);

	fileinfo.size = 11;
	ret = rmf_osal_fileSetFStat( file_handle, RMF_OSAL_FS_STAT_SIZE, &fileinfo );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileSetFStat Failed"); 
	printf("Changed size to %d using fstat \n", fileinfo.size);

	fileinfo.size = 0;
	ret = rmf_osal_fileGetFStat( file_handle, RMF_OSAL_FS_STAT_SIZE, &fileinfo );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileSeek Failed"); 
	printf("Size got from fstat = %d\n", fileinfo.size);

	ret = rmf_osal_fileClose( file_handle);
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileClose Failed"); 

	ret = rmf_osal_fileGetStat( TST_FILE_PATH1, RMF_OSAL_FS_STAT_MODDATE, &fileinfo );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileGetStat Failed"); 
	printf("Mod Date from stat = %d\n", fileinfo.modDate);

	ret = rmf_osal_fileGetStat( TST_FILE_PATH1, RMF_OSAL_FS_STAT_CREATEDATE, &fileinfo );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileGetStat Failed"); 
	printf("Create Date from stat = %d\n", fileinfo.createDate);

	ret = rmf_osal_fileGetStat( TST_FILE_PATH1, RMF_OSAL_FS_STAT_TYPE, &fileinfo );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileGetStat Failed"); 
	printf("Type from stat = %d\n", fileinfo.type);

	fileinfo.size = 10;
	ret = rmf_osal_fileSetStat( TST_FILE_PATH1, RMF_OSAL_FS_STAT_SIZE, &fileinfo );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileGetStat Failed"); 
	printf("Changed size to %d using stat \n", fileinfo.size);


	fileinfo.size = 0;
	ret = rmf_osal_fileGetStat( TST_FILE_PATH1, RMF_OSAL_FS_STAT_SIZE, &fileinfo );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileGetStat Failed"); 
	printf("Size from stat = %d\n", fileinfo.size);

	ret = rmf_osal_dirOpen( TST_DIR1, &dir_handle );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_dirOpen Failed"); 
       memset(dirEnt.name,0,sizeof(dirEnt.name));
	printf("Going to call dir read. Address of name is  (0x%x)\n",  (unsigned int)dirEnt.name);
	ret = rmf_osal_dirRead( dir_handle, &dirEnt);
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_dirRead Failed"); 
	printf("Dir read returned size = %lld isDir= %d name = %s (0x%x)\n", 
		dirEnt.fileSize, dirEnt.isDir, dirEnt.name,  (unsigned int)dirEnt.name);

	ret = rmf_osal_dirClose( dir_handle );
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_dirClose Failed"); 

	printf("Deleting file and dir\n"); 
	ret = rmf_osal_fileDelete( TST_FILE_PATH1);
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_fileDelete Failed"); 
	ret = rmf_osal_dirDelete( TST_DIR1);
	RMF_OSAL_ASSERT_FILE(ret, "rmf_osal_dirDelete Failed"); 
	
	return 0;
}
