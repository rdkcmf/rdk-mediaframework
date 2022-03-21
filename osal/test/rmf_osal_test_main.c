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
#include "rdk_debug.h"
#include "rmf_osal_init.h"

int rmf_osal_mutex_test();
int rmf_osal_event_test();
int rmf_osal_storage_test();
int rmf_osal_debug_test();
int rmf_osal_util_test();
int rmf_osal_mem_test();
int rmf_osal_file_test();
int rmf_osal_time_test();
int rmf_osal_cond_test();
int rmf_osal_thread_test();
int rmf_osal_socket_test();

int main()
{
	char option;
	int display_menu = 1;
	rmf_osal_init( NULL, NULL );
	while (1)
	{
		if (display_menu)
		{
			printf ("\n************RMF OSAL test application************\n");
			printf ("a : mutex test\n");
			printf ("b : event test\n");
			printf ("c : storage test\n");
			printf ("d : util test\n");
			printf ("f : mem test\n");
			printf ("g : file test\n");
			printf ("h : time test\n");
			printf ("i : condition variable test\n");
			printf ("j : thread test\n");
			printf ("k : socket test\n");
			printf ("x : exit \n");
			printf ("************RMF OSAL test application************\n");
		}
		else
		{
			display_menu = 1;
		}
		option = getchar();

		switch (option)
		{
		case 'a':
			rmf_osal_mutex_test();
			break;

		case 'b':
			rmf_osal_event_test();
			break;

		case 'c':
			rmf_osal_storage_test();
			break;

		case 'd':
			rmf_osal_util_test();
			break;

		case 'f':
			rmf_osal_mem_test();
			break;

		case 'g':
			rmf_osal_file_test();
			break;

		case 'h':
			rmf_osal_time_test();
			break;

		case 'i':
			rmf_osal_cond_test();
			break;

		case 'j':
			rmf_osal_thread_test();
			break;

		case 'k':
			rmf_osal_socket_test();
			break;

		case 'x':
			return 0;
			break;

		case '\n':
			display_menu = 0;
			break;

		default:
			printf("Invalid option %c \n" , option);
			break;
		}
	}
	return 0;
}
