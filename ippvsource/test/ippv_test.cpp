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
/*
 * ippv_test.cpp
 *
 *  Created on:
 *      Author:
 */
#include <stdio.h>
#include <stdlib.h>
#include "ippvclient.h"
#include "rmf_osal_init.h"

int main()
{
	int option;
	int display_menu = 1;
	uint32_t eID = 0;
	uint16_t sourceID = 0;
	uint32_t tSrcId = 0;
	uint32_t response = 0;
	uint8_t isFreePreviewable, isAuthorized = 0;
	bool result = false;
	int size = 0;
	int date = 0;
	int time = 0;
	char UTCTime[10] = "";
	uint32_t hours = 0;
	uint32_t minutes = 0; 

	IppvClient* pIppvClient = new IppvClient();
	rmf_osal_init(NULL, NULL);
	
	while (1)
	{
		if (display_menu)
		{
			printf ("\n************IPPV test application************\n");
			printf ("a : Enter eID\n");	
			printf ("b : purchaseIppvEvent()\n");
			printf ("c : cancelIppvEvent()\n");
			printf ("d : isIppvEnabled()\n");
			printf ("e : getPurchaseStatus()\n");
			printf ("f : isIppvEventAuthorized()\n");
			printf ("g : exit \n");
			printf ("************IPPV test application************\n");
			display_menu = 0;
		}
		option = getchar();

		switch (option)
		{
		case 'a':
			printf("Enter sourceID:");

                        size = scanf("%x",&tSrcId);
                        if(0 == size)
                        {
                                printf("Read error..\n");
                        }
			sourceID = (uint16_t)tSrcId;	
			printf("Enter Program date:");

			size = scanf("%u",&date);
                        if(0 == size)
                        {
                                printf("Read error..\n");
                        }

			
			printf("Enter Program Start Time(UTC) in (HH:MM):");
			
			size = scanf("%s",UTCTime);
                        if(0 == size)
                        {
                                printf("Read error..\n");
                        }

			sscanf(UTCTime,"%u:%u",&hours,&minutes);
			time = hours*60 + minutes ;
			eID =sourceID * 0x10000 + date * 0x800 + time;
			printf("date:%u",date);
			printf("hours:%u\n",hours);
			printf("minutes:%u\n",minutes);
			printf("eID:%x\n",eID);
						
			break;
		
		case 'b':
			response = pIppvClient->purchaseIppvEvent(eID);
			printf("Response got:%d\n",response);
			break;

		case 'c':
			response = pIppvClient->cancelIppvEvent(eID);
			printf("Response got:%d\n",response);
			break;

		case 'd':
			result = pIppvClient->isIppvEnabled();
			if(1 == result)
			{
				printf("Ippv enabled\n");
			}
			else
			{
				printf("Ippv not enabled\n");
			}
			break;

		case 'e':
			response = pIppvClient->getPurchaseStatus(eID);
			printf("Response got:%d\n",response);
			break;

		case 'f':
			pIppvClient->isIppvEventAuthorized(eID, &isFreePreviewable, &isAuthorized);
			if(isFreePreviewable)
			{
				printf("isFreePreviewable\n");
			}
			else
			{
				printf("not FreePreviewable\n");
			}

			if(isAuthorized)
			{
				printf("isAuthorized\n");
			}
			else
			{
				printf("not Authorized\n");
			}
			
			break;

		case 'g':
			printf("Exiting....\n");
			exit(0);
			break;

		case '\n':
			break;

		default:
			printf("Invalid option %c \n" , option);
			break;
		}
		display_menu = 1;
	}
	return 0;
}


