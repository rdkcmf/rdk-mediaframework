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

#ifndef _RMF_OSAL_SOCKET_PRIV_H_
#define _RMF_OSAL_SOCKET_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
#if !defined(RMF_OSAL_BIG_ENDIAN) && !defined(RMF_OSAL_LITTLE_ENDIAN)
#error Either RMF_OSAL_BIG_ENDIAN or RMF_OSAL_LITTLE_ENDIAN must be defined
#endif
*/

#include <rmf_osal_types.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <asm/ioctls.h>
#include <netdb.h>

/****************************************************************************
 *
 * PROTOCOL INDEPENDENT DEFINITIONS (See mpeos_socket.h for details)
 *
 ***************************************************************************/

#define OS_SOCKET_FD_SETSIZE    			FD_SETSIZE
#define OS_SOCKET_MAXHOSTNAMELEN			MAXHOSTNAMELEN
#define OS_SOCKET_MSG_OOB       			MSG_OOB
#define OS_SOCKET_MSG_PEEK      			MSG_PEEK
#define OS_SOCKET_SHUTDOWN_RD  				0
#define OS_SOCKET_SHUTDOWN_RDWR				2
#define OS_SOCKET_SHUTDOWN_WR  				1
#define OS_SOCKET_DATAGRAM					SOCK_DGRAM
#define OS_SOCKET_STREAM					SOCK_STREAM
#define OS_SOCKET_INVALID_SOCKET           -1
#define OS_SOCKET_FIONBIO 					FIONBIO
#define OS_SOCKET_FIONREAD					FIONREAD
typedef int                         		os_Socket;
typedef socklen_t				os_SocketSockLen;
typedef fd_set                      		os_SocketFDSet;
typedef struct linger               		os_SocketLinger;
typedef u_char								os_SocketSaFamily;
typedef struct sockaddr             		os_SocketSockAddr;

/****************************************************************************
 *
 * IPv4 SPECIFIC DEFINITIONS (See mpeos_socket.h for details)
 *
 ***************************************************************************/

#define OS_SOCKET_AF_INET4                  AF_INET
#define OS_SOCKET_IN4ADDR_ANY               INADDR_ANY
#define OS_SOCKET_IN4ADDR_LOOPBACK          INADDR_LOOPBACK
#define OS_SOCKET_INET4_ADDRSTRLEN          INET_ADDRSTRLEN
typedef struct in_addr              		os_SocketIPv4Addr;
typedef struct ip_mreq             		    os_SocketIPv4McastReq;
typedef struct sockaddr_in          		os_SocketIPv4SockAddr;

/****************************************************************************
 *
 * IPv6 SPECIFIC DEFINITIONS (See mpeos_socket.h for details)
 *
 ***************************************************************************/

#ifdef RMF_OSAL_FEATURE_IPV6

#define OS_SOCKET_AF_INET6             		AF_INET6
#define OS_SOCKET_IN6ADDR_ANY_INIT     		IN6ADDR_ANY_INIT
#define OS_SOCKET_IN6ADDR_LOOPBACK_INIT		IN6ADDR_LOOPBACK_INIT
#define OS_SOCKET_INET6_ADDRSTRLEN     		INET6_ADDRSTRLEN
typedef struct in6_addr    					os_SocketIPv6Addr;
typedef struct ipv6_mreq   					os_SocketIPv6McastReq;
typedef struct sockaddr_in6					os_SocketIPv6SockAddr;

#endif

/****************************************************************************
 *
 * SOCKET LEVEL OPTIONS (See mpeos_socket.h for details)
 *
 ***************************************************************************/

#define OS_SOCKET_SOL_SOCKET               SOL_SOCKET
#define OS_SOCKET_SO_BROADCAST             SO_BROADCAST
#define OS_SOCKET_SO_DEBUG                 SO_DEBUG
#define OS_SOCKET_SO_DONTROUTE             SO_DONTROUTE
#define OS_SOCKET_SO_ERROR                 SO_ERROR
#define OS_SOCKET_SO_KEEPALIVE             SO_KEEPALIVE
#define OS_SOCKET_SO_LINGER                SO_LINGER
#define OS_SOCKET_SO_OOBINLINE             SO_OOBINLINE
#define OS_SOCKET_SO_RCVBUF                SO_RCVBUF
#define OS_SOCKET_SO_RCVLOWAT              SO_RCVLOWAT
#define OS_SOCKET_SO_RCVTIMEO              SO_RCVTIMEO
#define OS_SOCKET_SO_REUSEADDR             SO_REUSEADDR
#define OS_SOCKET_SO_SNDBUF                SO_SNDBUF
#define OS_SOCKET_SO_SNDLOWAT              SO_SNDLOWAT
#define OS_SOCKET_SO_SNDTIMEO              SO_SNDTIMEO
#define OS_SOCKET_SO_TYPE                  SO_TYPE

/****************************************************************************
 *
 * IPv4 LEVEL OPTIONS (See mpeos_socket.h for details)
 *
 ***************************************************************************/

#define OS_SOCKET_IPPROTO_IPV4				IPPROTO_IP
#define OS_SOCKET_IPPROTO_TCP              	IPPROTO_TCP
#define OS_SOCKET_IPPROTO_UDP              	IPPROTO_UDP
#define OS_SOCKET_IPV4_ADD_MEMBERSHIP 		IP_ADD_MEMBERSHIP
#define OS_SOCKET_IPV4_DROP_MEMBERSHIP		IP_DROP_MEMBERSHIP
#define OS_SOCKET_IPV4_MULTICAST_IF   		IP_MULTICAST_IF
#define OS_SOCKET_IPV4_MULTICAST_LOOP 		IP_MULTICAST_LOOP
#define OS_SOCKET_IPV4_MULTICAST_TTL  		IP_MULTICAST_TTL

/****************************************************************************
 *
 * IPv6 LEVEL OPTIONS (See mpeos_socket.h for details)
 *
 ***************************************************************************/

#ifdef RMF_OSAL_FEATURE_IPV6

#define OS_SOCKET_IPPROTO_IPV6        		IPPROTO_IPV6
#define OS_SOCKET_IPV6_ADD_MEMBERSHIP 		IPV6_ADD_MEMBERSHIP
#define OS_SOCKET_IPV6_DROP_MEMBERSHIP		IPV6_DROP_MEMBERSHIP
#define OS_SOCKET_IPV6_MULTICAST_IF   		IPV6_MULTICAST_IF
#define OS_SOCKET_IPV6_MULTICAST_HOPS 		IPV6_MULTICAST_HOPS
#define OS_SOCKET_IPV6_MULTICAST_LOOP 		IPV6_MULTICAST_LOOP

#endif

/****************************************************************************
 *
 * TCP LEVEL OPTIONS (See mpeos_socket.h for details)
 *
 ***************************************************************************/

#define OS_SOCKET_TCP_NODELAY				TCP_NODELAY

/****************************************************************************
 *
 * SOCKET ERROR CODES (See mpeos_socket.h for details)
 *
 ***************************************************************************/

#define OS_RMF_OSAL_SOCKET_EACCES        		EACCES
#define OS_RMF_OSAL_SOCKET_EADDRINUSE    		EADDRINUSE
#define OS_RMF_OSAL_SOCKET_EADDRNOTAVAIL 		EADDRNOTAVAIL
#define OS_RMF_OSAL_SOCKET_EAFNOSUPPORT  		EAFNOSUPPORT
#define OS_RMF_OSAL_SOCKET_EAGAIN        		EAGAIN
#define OS_RMF_OSAL_SOCKET_EALREADY      		EALREADY
#define OS_RMF_OSAL_SOCKET_EBADF         		EBADF
#define OS_RMF_OSAL_SOCKET_ECONNABORTED  		ECONNABORTED
#define OS_RMF_OSAL_SOCKET_ECONNREFUSED  		ECONNREFUSED
#define OS_RMF_OSAL_SOCKET_ECONNRESET    		ECONNRESET
#define OS_RMF_OSAL_SOCKET_EDESTADDRREQ  		EDESTADDRREQ
#define OS_RMF_OSAL_SOCKET_EDOM          		EDOM
#define OS_RMF_OSAL_SOCKET_EHOSTNOTFOUND 		-1 /* FINISH HOST_NOT_FOUND */
#define OS_RMF_OSAL_SOCKET_EHOSTUNREACH  		EHOSTUNREACH
#define OS_RMF_OSAL_SOCKET_EINTR         		EINTR
#define OS_RMF_OSAL_SOCKET_EIO           		EIO
#define OS_RMF_OSAL_SOCKET_EINPROGRESS   		EINPROGRESS
#define OS_RMF_OSAL_SOCKET_EISCONN       		EISCONN
#define OS_RMF_OSAL_SOCKET_ELOOP         		ELOOP
#define OS_RMF_OSAL_SOCKET_EMFILE        		EMFILE
#define OS_RMF_OSAL_SOCKET_EMSGSIZE      		EMSGSIZE
#define OS_RMF_OSAL_SOCKET_ENAMETOOLONG  		ENAMETOOLONG
#define OS_RMF_OSAL_SOCKET_ENFILE        		ENFILE
#define OS_RMF_OSAL_SOCKET_ENETDOWN      		ENETDOWN
#define OS_RMF_OSAL_SOCKET_ENETUNREACH   		ENETUNREACH
#define OS_RMF_OSAL_SOCKET_ENOBUFS       		ENOBUFS
#define OS_RMF_OSAL_SOCKET_ENOPROTOOPT   		ENOPROTOOPT
#define OS_RMF_OSAL_SOCKET_ENORECOVERY   		-1 /* FINISH WSANO_RECOVERY */
#define OS_RMF_OSAL_SOCKET_ENOSPC        		NOSPC
#define OS_RMF_OSAL_SOCKET_ENOTCONN      		ENOTCONN
#define OS_RMF_OSAL_SOCKET_ENOTSOCK      		ENOTSOCK
#define OS_RMF_OSAL_SOCKET_EOPNOTSUPP    		EOPNOTSUPP
#define OS_RMF_OSAL_SOCKET_EPIPE         		EPIPE
#define OS_RMF_OSAL_SOCKET_EPROTO        		EPROTO
#define OS_RMF_OSAL_SOCKET_EPROTONOSUPPORT		EPROTONOSUPPORT
#define OS_RMF_OSAL_SOCKET_EPROTOTYPE     		EPROTOTYPE
#define OS_RMF_OSAL_SOCKET_ETIMEDOUT      		ETIMEDOUT
#define OS_RMF_OSAL_SOCKET_ETRYAGAIN      		-1 /* FINISH WSATRY_AGAIN */
#define OS_RMF_OSAL_SOCKET_EWOULDBLOCK    		EWOULDBLOCK


/****************************************************************************
 *
 * Linux uses bundled DNS resolver support rather than the ARES DNS resolver.
 *
 ***************************************************************************/

typedef struct hostent	os_SocketHostEntry;

#ifdef __cplusplus
}
#endif
#endif

