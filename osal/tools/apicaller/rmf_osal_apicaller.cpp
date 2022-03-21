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
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <math.h>


#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define RMF_OSAL_API_CALLER_CALL_PORT    54128
#define RMF_OSAL_API_CALLER_MAX_CHARS    256

typedef int SOCKET;

#define VLABS(n)    (((n) < 0) ? -(n) : (n))
static char * vlg_pszThisAppName = NULL;

#define DECREMENT_AND_RETURN_ON_ZERO(value, dec) {\
    value = value -dec;\
    if ( 0 >= value)\
    {\
       printf("Error, value reached 0\n");\
       close(sockfd);\
       return -1;\
    }\
}

int main(int argc, char * argv[])
{
    int i = 0;
    
    vlg_pszThisAppName = strrchr(argv[0], '/');
    
    if(NULL == vlg_pszThisAppName)
    {
        vlg_pszThisAppName = argv[0];
    }
    else
    {
        vlg_pszThisAppName++;
    }
    
    if(argc < 2)
    {
        printf("%s: Usage: %s < API > [ param1, param2, ... ]\n", vlg_pszThisAppName, vlg_pszThisAppName);
        return 0;
    }
    
    long sockfd = 0;
    int error = 0;

    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("%s: Error opening socket \n", vlg_pszThisAppName);
        return 0;
    }

    if(sockfd > 0)
    {
        char aSzFunctionCall[RMF_OSAL_API_CALLER_MAX_CHARS];
        char * pszFunctionCall = aSzFunctionCall;
        int bytesRemaining = sizeof(aSzFunctionCall);

        struct sockaddr_in6 serv_addr;

        /* Setting all values in a buffer to zero */
        memset((char *) &serv_addr, 0, sizeof(serv_addr));
        /* Code for address family */
        serv_addr.sin6_family = AF_INET;
        /* IP address of the host */
        inet_pton(serv_addr.sin6_family, "127.0.0.1", &(serv_addr.sin6_addr));
        /* Port number */
        serv_addr.sin6_port = htons(RMF_OSAL_API_CALLER_CALL_PORT);
        
        strncpy(aSzFunctionCall, "\n;\n;", bytesRemaining);
        DECREMENT_AND_RETURN_ON_ZERO( bytesRemaining, sizeof("\n;\n;"));
        strncat(aSzFunctionCall, argv[1], bytesRemaining);
        DECREMENT_AND_RETURN_ON_ZERO( bytesRemaining ,strlen(argv[1]));
        strncat(aSzFunctionCall, "(", bytesRemaining);
        DECREMENT_AND_RETURN_ON_ZERO( bytesRemaining, 1);
        
        printf("%s: Calling %s(", vlg_pszThisAppName, argv[1]);
        
        for(i = 2; i < argc; i++)
        {
            strncat(aSzFunctionCall, argv[i], bytesRemaining);
            DECREMENT_AND_RETURN_ON_ZERO(bytesRemaining , strlen(argv[i]));
            strncat(aSzFunctionCall, ",", bytesRemaining);
            DECREMENT_AND_RETURN_ON_ZERO( bytesRemaining, 1);
        }

        printf(");\n");
        strncat(aSzFunctionCall, ");\n;\n", bytesRemaining);
        printf("aSzFunctionCall: %s\n", aSzFunctionCall);
        if (-1 == sendto(sockfd, pszFunctionCall, strlen(pszFunctionCall), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
        {
            printf("%s: Error sendig data \n", vlg_pszThisAppName);
        }

        close(sockfd);
    }
    
    return 0;
}
