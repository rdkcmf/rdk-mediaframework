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
#include "rmf_osal_mem.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define RMF_OSAL_ASSERT_MEM(ret,msg) {\
	if (RMF_SUCCESS != ret) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}
#define MEM_TEST_ALLOC_SIZE 16
#define MEM_TEST_REALLOC_SIZE (MEM_TEST_ALLOC_SIZE+16)
int rmf_osal_mem_test()
{
	char *a, *b, *c;
	int i;
	rmf_Error ret;

	ret  = rmf_osal_memAllocP( RMF_OSAL_MEM_TEMP, MEM_TEST_ALLOC_SIZE, (void**)&a);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memAllocP Failed");

	ret  = rmf_osal_memAllocP( RMF_OSAL_MEM_TEMP, MEM_TEST_ALLOC_SIZE, (void**) &b);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memAllocP Failed");

	ret  = rmf_osal_memAllocP( RMF_OSAL_MEM_TEMP, MEM_TEST_ALLOC_SIZE, (void**) &c);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memAllocP Failed");

	printf("Memory after alloc a=0x%x  b=0x%x  c=0x%x \n" , (unsigned int )a, (unsigned int )b, (unsigned int )c);

	for ( i=0; i< MEM_TEST_ALLOC_SIZE; i++)
	{
		*(a+i) = i;
		*(b+i) = i;
		*(c+i) = i;
	}
	ret  = rmf_osal_memReallocP( RMF_OSAL_MEM_TEMP, MEM_TEST_REALLOC_SIZE, (void**)&a);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memAllocP Failed");

	ret  = rmf_osal_memReallocP( RMF_OSAL_MEM_TEMP, MEM_TEST_REALLOC_SIZE, (void**) &b);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memAllocP Failed");

	ret  = rmf_osal_memReallocP( RMF_OSAL_MEM_TEMP, MEM_TEST_REALLOC_SIZE, (void**) &c);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memAllocP Failed");

	printf("Memory after realloc a=0x%x  b=0x%x  c=0x%x \n" , (unsigned int )a, (unsigned int )b, (unsigned int )c);

	for ( i=MEM_TEST_ALLOC_SIZE; i< MEM_TEST_REALLOC_SIZE; i++)
	{
		*(a+i) = i;
		*(b+i) = i;
		*(c+i) = i;
	}
	for ( i=0; i< MEM_TEST_REALLOC_SIZE; i++)
	{
		printf("a[%d]=%d b[%d]=%d c[%d]=%d\n", i,a[i],i,b[i],i,c[i]);
	}
	ret  = rmf_osal_memFreeP( RMF_OSAL_MEM_TEMP, a);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memFreeP a Failed");
	ret  = rmf_osal_memFreeP( RMF_OSAL_MEM_TEMP, b);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memFreeP b Failed");
	ret  = rmf_osal_memFreeP( RMF_OSAL_MEM_TEMP, c);
	RMF_OSAL_ASSERT_MEM(ret, "rmf_osal_memFreeP c Failed");
#ifdef DEBUG_ALLOC
	print_alloc_info();
#endif
	return 0;
}
