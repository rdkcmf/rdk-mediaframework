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
 * @file rmf_osal_socket.h
 * @brief The RMF OSAL Sockets API. This API provides a consistent interface to BSD style sockets
 * regardless of the underlying operating system.
 * Define RMF_OSAL_FEATURE_IPV6 prior to including this file to turn on support for IPv6
 *
 */

/**
 * @defgroup OSAL_SOCKET OSAL Socket
 * Described the details about OSAL Socket API specifications. These API provides a
 * consistent interface to BSD style sockets regardless of the underlying operating system.
 * @ingroup OSAL
 *
 * @defgroup OSAL_SOCKET_API OSAL Socket API list
 * Described the details about OSAL Socket Interface functions.
 * These API provides a consistent interface to BSD style sockets regardless of the underlying operating system. 
 * @ingroup OSAL_SOCKET
 *
 * @addtogroup OSAL_SOCKET_TYPES OSAL Socket Data Types
 * Describe the details about Data Structure used by OSAL Socket.
 * @ingroup OSAL_SOCKET
 */


#ifndef _RMF_OSAL_SOCKET_H_
#define _RMF_OSAL_SOCKET_H_

#include <rmf_osal_error.h>
#include <rmf_osal_time.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup OSAL_SOCKET_TYPES
 * @{
 */


/****************************************************************************************
 *
 * PROTOCOL INDEPENDENT DEFINITIONS
 *
 ***************************************************************************************/

/**
 * The maximum number of file descriptors represented in the rmf_osal_SocketFDSet datatype.
 * @see POSIX definition of FD_SETSIZE
 */
#define RMF_OSAL_SOCKET_FD_SETSIZE FD_SETSIZE

/**
 * The maximum length of a host name in bytes (including the terminating NULL byte).
 */
#define RMF_OSAL_SOCKET_MAXHOSTNAMELEN MAXHOSTNAMELEN

/**
 * Size of the out of band data transfering and it protocol specific
 */
#define RMF_OSAL_SOCKET_MSG_OOB MSG_OOB

/**
 * Number of bytes peek into the received packet and leave the packet
 * as unread
 */
#define RMF_OSAL_SOCKET_MSG_PEEK MSG_PEEK

/**
 * Used with rmf_osal_socketShutdown() to disables further receive operations.
 */
#define RMF_OSAL_SOCKET_SHUT_RD 0

/**
 * Used with rmf_osal_socketShutdown() to disables further send and receive operations.
 */
#define RMF_OSAL_SOCKET_SHUT_RDWR 2

/**
 * Used with rmf_osal_socketShutdown() to disables further send operations.
 */
#define RMF_OSAL_SOCKET_SHUT_WR 1

/**
 * A datagram based socket
 */
#define RMF_OSAL_SOCKET_DATAGRAM SOCK_DGRAM

/**
 * A stream based socket
 */
#define RMF_OSAL_SOCKET_STREAM SOCK_STREAM

/**
 * A value which indicates that an rmf_osal_Socket descriptor is not valid.
 */
#define RMF_OSAL_SOCKET_INVALID_SOCKET -1

/**
 * Used with rmf_osal_socketIoctl() to clear or turn on the non-blocking flag for the socket.
 */
#define RMF_OSAL_SOCKET_FIONBIO FIONBIO

/**
 * Used with rmf_osal_socketIoctl() to return the number of bytes currently in the receive
 * buffer for the socket.
 */
#define RMF_OSAL_SOCKET_FIONREAD FIONREAD

/**
 * This is a numeric type used to reference an open socket file descriptor
 */
typedef int rmf_osal_Socket;

/**
 * This type defines a length of a socket structure
 */
typedef socklen_t rmf_osal_SocketSockLen;

/**
 * This is an opaque structure used by rmf_osal_socketSelect() to define a set of file
 * descriptors
 */
typedef fd_set rmf_osal_SocketFDSet;

/**
 * The host entry returned by calls to rmf_osal_socketGetHostByAddr() and rmf_osal_socketGetHostByName().
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     char *h_name - official (canonical) name of host
 * <li>     char **h_aliases - pointer to array of pointers to alias names
 * <li>     int h_addrtype - host address type
 * <li>     int h_length - length of address
 * <li>     char **h_addr_list - pointer to array of pointers with IPv4 or IPv6 addresses
 * </ul>
 * @see POSIX definition of hostent
 */
typedef struct hostent rmf_osal_SocketHostEntry;

/**
 * Linger structure.
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     int l_onoff - 0=off, nonzero=on
 * <li>     int l_linger - linger time in seconds
 * </ul>
 * @see POSIX definition of linger
 */
typedef struct linger  rmf_osal_SocketLinger;

/**
 * Socket address family (RMF_OSAL_SOCKET_AF_INET4 or RMF_OSAL_SOCKET_AF_INET6)
 * @see POSIX definition of sa_family_t
 */
typedef u_char rmf_osal_SocketSaFamily;

/**
 * Protocol independent socket address structure.
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     rmf_osal_SocketSaFamily sa_family - address family
 * </ul>
 * @see POSIX definition of sockaddr
 */
typedef struct sockaddr rmf_osal_SocketSockAddr;

/**
 * Platform independent structure definitions used to return a list of network
 * interfaces of the system.  The <i>rmf_osal_SocketNetAddr</i> structure represents a
 * single network interface address in the linked list of addresses for an interface.
 * The <i>rmf_osal_SocketNetIfList</i> structure represents a single network interface
 * with its list of associated addresses.
 */
#define RMF_OSAL_SOCKET_IFNAMSIZ (64)

typedef struct _rmf_osal_SocketNetAddr
{
    int if_family; /* Inet family. */
    rmf_osal_SocketSockAddr *if_addr; /* IPv4 or IPv4 address. */
    struct _rmf_osal_SocketNetAddr *if_next; /* Pointer to next address. */
} rmf_osal_SocketNetAddr;

typedef struct _rmf_osal_SocketNetIfList
{
    char if_name[RMF_OSAL_SOCKET_IFNAMSIZ]; /* Interface name */
    uint32_t if_index; /* Interface index. */
    rmf_osal_SocketNetAddr *if_addresses; /* List of addresses. */
    struct _rmf_osal_SocketNetIfList *if_next; /* Next interface in list. */
} rmf_osal_SocketNetIfList;

/****************************************************************************************
 *
 * IPv4 SPECIFIC DEFINITIONS
 *
 ***************************************************************************************/

/**
 * The address family for IPv4 sockets
 * @see POSIX definition of AF_INET
 */
#define RMF_OSAL_SOCKET_AF_INET4 AF_INET

/**
 * The IPv4 protocol
 * @see POSIX definition of IPPROTO_IP
 */
#define RMF_OSAL_SOCKET_IPPROTO_IPV4 IPPROTO_IP

/**
 * The TCP/IP protocol
 * @see POSIX definition of IPPROTO_TCP
 */
#define RMF_OSAL_SOCKET_IPPROTO_TCP IPPROTO_TCP

/**
 * The UDP/IP protocol
 * @see POSIX definition of IPPROTO_UDP
 */
#define RMF_OSAL_SOCKET_IPPROTO_UDP IPPROTO_UDP

/**
 * Wildcard IPv4 address (matches any address)
 * @see POSIX definition of INADDR_ANY
 */
#define RMF_OSAL_SOCKET_IN4ADDR_ANY INADDR_ANY

/**
 * IPv4 loopback address
 * @see POSIX definition of INADDR_LOOPBACK
 */
#define RMF_OSAL_SOCKET_IN4ADDR_LOOPBACK INADDR_LOOPBACK

/**
 * Maximum length of an IPv4 address string (includes the null terminator)
 * @see POSIX definition of INET_ADDRSTRLEN
 */
#define RMF_OSAL_SOCKET_INET4_ADDRSTRLEN INET_ADDRSTRLEN

/**
 * IPv4 address.
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     uint32_t s_addr - 32 bit IPv4 address in network byte order
 * </ul>
 * @see POSIX definition of in_addr
 */
typedef struct in_addr rmf_osal_SocketIPv4Addr;

/**
 * IPv4 multicast request structure.
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     rmf_osal_SocketIPv4Addr imr_multiaddr - IPv4 class D multicast address
 * <li>     rmf_osal_SocketIPv4Addr imr_interface - IPv4 address of local interface
 * </ul>
 * @see POSIX definition of ip_mreq
 */
typedef struct ip_mreq rmf_osal_SocketIPv4McastReq;

/**
 * IPv4 socket address structure.
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     rmf_osal_SocketSaFamily sin_family - address family (RMF_OSAL_SOCKET_AF_INET4)
 * <li>     uint16_t sin_port - 16 bit TCP or UDP port number
 * <li>     rmf_osal_SocketIPv4Addr sin_addr - IPv4 address
 * </ul>
 * @see POSIX definition of sockaddr_in
 */
typedef struct sockaddr_in rmf_osal_SocketIPv4SockAddr;

/****************************************************************************************
 *
 * IPv6 SPECIFIC DEFINITIONS
 *
 ***************************************************************************************/

#ifdef RMF_OSAL_FEATURE_IPV6

/**
 * The address family for IPv6 sockets. This constant is only defined when RMF_OSAL_FEATURE_IPV6
 * is defined.
 * @see POSIX definition of AF_INET6
 */
#define RMF_OSAL_SOCKET_AF_INET6 AF_INET6

/**
 * The IPv6 protocol
 * @see POSIX definition of IPPROTO_IPV6
 */
#define RMF_OSAL_SOCKET_IPPROTO_IPV6 IPPROTO_IPV6

/**
 * Wildcard IPv6 address (matches any address)
 * @see POSIX definition of IN6ADDR_ANY_INIT
 */
#define RMF_OSAL_SOCKET_IN6ADDR_ANY_INIT IN6ADDR_ANY_INIT

/**
 * IPv6 loopback address
 * @see POSIX definition of IN6ADDR_LOOPBACK_INIT
 */
#define RMF_OSAL_SOCKET_IN6ADDR_LOOPBACK_INIT IN6ADDR_LOOPBACK_INIT

/**
 * Maximum length of an IPv6 address string
 * @see POSIX definition of IN6ADDR_ADDRSTRLEN
 */
#define RMF_OSAL_SOCKET_INET6_ADDRSTRLEN INET6_ADDRSTRLEN

/**
 * IPv6 address. This structure is only defined when RMF_OSAL_FEATURE_IPV6 is defined.
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     uint8_t s6_addr[16] - 128 bit IPv6 address in network byte order
 * </ul>
 * @see POSIX definition of in6_addr
 */
typedef struct in6_addr rmf_osal_SocketIPv6Addr;

/**
 * IPv6 multicast request structure. This structure is only defined when RMF_OSAL_FEATURE_IPV6
 * is defined.
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     rmf_osal_SocketIPv6Addr ipv6mr_multiaddr - IPv6 multicast address
 * <li>     rmf_osal_SocketIPv6Addr ipv6mr_interface - Interface index, or 0
 * </ul>
 * @see POSIX definition of ip6_mreq
 */
typedef struct ipv6_mreq rmf_osal_SocketIPv6McastReq;

/**
 * IPv6 socket address structure. This structure is only defined when RMF_OSAL_FEATURE_IPV6
 * is defined.
 * The following members may be accessed directly and must be present in the underlying
 * platform dependent structure:
 * <ul>
 * <li>     rmf_osal_SocketSaFamily sin6_family - address family (RMF_OSAL_SOCKET_AF_INET6)
 * <li>     uint16_t sin6_port - transport layer port number in network byte order
 * <li>     uint32_t sin6_flowinfo - priority & flow label in network byte order
 * <li>     rmf_osal_SocketIPv6Addr sin6_addr - IPv6 address
 * </ul>
 * @see POSIX definition of sockaddr_in6
 */
typedef struct sockaddr_in6 rmf_osal_SocketIPv6SockAddr;

#endif

/****************************************************************************************
 *
 * SOCKET LEVEL OPTIONS
 *
 ***************************************************************************************/

/**
 * Used with rmf_osal_socketSetOpt() and rmf_osal_socketGetOpt() to specify a socket level
 * option.
 * @see POSIX definition of SOL_SOCKET
 */
#define RMF_OSAL_SOCKET_SOL_SOCKET SOL_SOCKET

/**
 * Controls whether transmission of broadcast messages is supported, if this is supported
 * by the protocol. This boolean option shall store an int value.
 * @see POSIX definition of SO_BROADCAST
 */
#define RMF_OSAL_SOCKET_SO_BROADCAST SO_BROADCAST

/**
 * Controls whether debugging information is being recorded. This boolean option shall store
 * an int value.
 * @see POSIX definition of SO_DEBUG
 */
#define RMF_OSAL_SOCKET_SO_DEBUG SO_DEBUG

/**
 * Change the default rule based on this flag, if interface is directly connected to network
 * it is will go through default routing path.
 * @see POSIX definition of SO_DONTROUTE
 */
#define RMF_OSAL_SOCKET_SO_DONTROUTE SO_DONTROUTE

/**
 * Reports about error status and clears it when used with rmf_osal_socketGetOpt().
 * @see POSIX definition of SO_ERROR
 */
#define RMF_OSAL_SOCKET_SO_ERROR SO_ERROR

/**
 * Set the connection as keepalive option socket option
 * @see POSIX definition of SO_KEEPALIVE
 */
#define RMF_OSAL_SOCKET_SO_KEEPALIVE SO_KEEPALIVE

/**
 * This option is used to set the SO_LINGER socket option to set the time out
 * If linger onoff is set it will wait for the timeout before closing the connection
 * This option shall store a rmf_osal_SocketLinger structure.
 * @see POSIX definition of SO_LINGER
 */
#define RMF_OSAL_SOCKET_SO_LINGER SO_LINGER

/**
 * To mark the data urgent in oob inline data received through this socket
 * @see POSIX definition of SO_OOBINLINE
 */
#define RMF_OSAL_SOCKET_SO_OOBINLINE SO_OOBINLINE

/**
 * Controls the receive buffer size. This option shall store an int value.
 * @see POSIX definition of SO_RCVBUF
 */
#define RMF_OSAL_SOCKET_SO_RCVBUF SO_RCVBUF

/**
 * Set the lower water mark of the socket data received
 */
#define RMF_OSAL_SOCKET_SO_RCVLOWAT SO_RCVLOWAT

/**
 * Set the socket option to wait timeout, when this value sets socket will wait before reporting
 * timeout error to application
 */
#define RMF_OSAL_SOCKET_SO_RCVTIMEO SO_RCVTIMEO

/**
 * Socket option to allow to reuse the local addres to bind
 */
#define RMF_OSAL_SOCKET_SO_REUSEADDR SO_REUSEADDR

/**
 * Controls the send buffer size. This option shall store an int value.
 * @see POSIX definition of SO_SNDBUF
 */
#define RMF_OSAL_SOCKET_SO_SNDBUF SO_SNDBUF

/**
 * Set low water mark for processing data to output through socket
 */
#define RMF_OSAL_SOCKET_SO_SNDLOWAT SO_SNDLOWAT

/**
 * Set socket send timeout, if this socket option is set, before reporting error socket
 * will wait and report error
 */
#define RMF_OSAL_SOCKET_SO_SNDTIMEO SO_SNDTIMEO

/**
 * Reports the socket type when used with rmf_osal_socketGetOpt(). This option cannot be set.
 * This option shall store an int value.
 * @see POSIX definition of SO_TYPE
 */
#define RMF_OSAL_SOCKET_SO_TYPE SO_TYPE

/****************************************************************************************
 *
 * IPv4 LEVEL OPTIONS
 *
 ***************************************************************************************/

/**
 * Use this option to join IPv4 multicast group
 */
#define RMF_OSAL_SOCKET_IPV4_ADD_MEMBERSHIP IP_ADD_MEMBERSHIP

/**
 * Use this option to drop from IPv4 multicast group
 */
#define RMF_OSAL_SOCKET_IPV4_DROP_MEMBERSHIP IP_DROP_MEMBERSHIP

/**
 * To specify interface to use outgoing multicast datagrams in socket option
 */
#define RMF_OSAL_SOCKET_IPV4_MULTICAST_IF IP_MULTICAST_IF

/**
 * To enable or disable local loopback of multicast datagrams. By default loopback is enabled:
 */
#define RMF_OSAL_SOCKET_IPV4_MULTICAST_LOOP IP_MULTICAST_LOOP

/**
 * Sets the time-to-live value for multicast packet sending through that socket.
 */
#define RMF_OSAL_SOCKET_IPV4_MULTICAST_TTL IP_MULTICAST_TTL

/****************************************************************************************
 *
 * IPv6 LEVEL OPTIONS
 *
 ***************************************************************************************/

#ifdef RMF_OSAL_FEATURE_IPV6

/**
 * Join a multicast group on a specified local interface. Argument is a rmf_osal_SocketIPv6McastReq
 * structure. <i>ipv6mr_multiaddr</i> contains the address of the multicast group the
 * caller wants to join or leave. It must be a valid multicast address.
 * <i>ipv6mr_interface</i> is the address of the local interface with which the system
 * should join the multicast group; if it is equal to RMF_OSAL_SOCKET_IN6ADDR_ANY_INIT an appropriate
 * interface is chosen by the system.
 * <p>
 * More than one join is allowed on a given socket but each join must be for a different
 * multicast address, or for the same multicast address but on a different interface from
 * previous joins for that address on this socket. This can be used on a multihomed host
 * where, for example, one socket is created and then for each interface a join is
 * performed for a given multicast address.
 * @see POSIX definition of IPV6_ADD_MEMBERSHIP
 */
#define RMF_OSAL_SOCKET_IPV6_ADD_MEMBERSHIP IPV6_ADD_MEMBERSHIP

/**
 * Leave a multicast group. Argument is a rmf_osal_SocketIPv6McastReq structure similar to
 * RMF_OSAL_SOCKET_IPV6_ADD_MEMBERSHIP. If the local interface is not specified (that is, the value is
 * IN6ADDR_ANY_INIT), the first matching multicasting group membership is dropped.
 * <p>
 * If a process joins a group but never explicitly leaves the group, when the socket is
 * closed (either explicitly or on process termination), the membership is dropped
 * automatically. It is possible for multiple processes on a host to each join the same
 * group, in which case the host remains a member of that group until the last process
 * leaves the group.
 * @see POSIX definition of IPV6_DROP_MEMBERSHIP
 */
#define RMF_OSAL_SOCKET_IPV6_DROP_MEMBERSHIP IPV6_DROP_MEMBERSHIP

/**
 * Specify the interface for outgoing multicast datagrams sent on this socket. This
 * interface is specified as a rmf_osal_SocketIPv6Addr structure. If the value specified is
 * IN6ADDR_ANY_INIT, this removes any interface previously assigned by this socket option,
 * and the system will choose the interface each time a datagram is sent.
 * <p>
 * Be careful to distinguish between the local interface specified (or chosen) when a
 * process joins a group (the interface on which arriving multicast datagrams will be
 * received), and the local interface specified (or chosen) when a multicast datagram is
 * output.
 * @see POSIX definition of IPV6_MULTICAST_IF
 */
#define RMF_OSAL_SOCKET_IPV6_MULTICAST_IF IPV6_MULTICAST_IF

/**
 * Sets or reads the hop limit of outgoing multicast datagrams for this socket. It
 * is very important for multicast packets to set the smallest hop limit possible. The default
 * is 1 which means that multicast packets don't leave the local network unless the
 * user program explicitly requests it. This option shall store an unsigned int value.
 * @see POSIX definition of IPV6_MULTICAST_HOPS
 */
#define RMF_OSAL_SOCKET_IPV6_MULTICAST_HOPS IPV6_MULTICAST_HOPS

/**
 * Enable or disable local loopback of multicast datagrams of that socket
 */
#define RMF_OSAL_SOCKET_IPV6_MULTICAST_LOOP IPV6_MULTICAST_LOOP

#endif

/****************************************************************************************
 *
 * TCP LEVEL OPTIONS
 *
 ***************************************************************************************/

/**
 * If set, this option disables TCP's <i>Nagle algorithm</i>. By default this algorithm is
 * enabled. This option shall store an int value.
 * @see POSIX definition of TCP_NODELAY
 */
#define RMF_OSAL_SOCKET_TCP_NODELAY TCP_NODELAY

/****************************************************************************************
 *
 * The following error codes are used by functions below but are defined in rmf_osal_error.h
 * so they do not need to be mapped here.
 *
 * RMF_OSAL_EINVAL
 * RMF_OSAL_ENODATA
 * RMF_OSAL_ENOMEM
 * RMF_OSAL_ETHREADDEATH
 *
 ***************************************************************************************/

#define RMF_OSAL_SOCKET_EACCES        		EACCES
#define RMF_OSAL_SOCKET_EADDRINUSE    		EADDRINUSE
#define RMF_OSAL_SOCKET_EADDRNOTAVAIL 		EADDRNOTAVAIL
#define RMF_OSAL_SOCKET_EAFNOSUPPORT  		EAFNOSUPPORT
#define RMF_OSAL_SOCKET_EAGAIN        		EAGAIN
#define RMF_OSAL_SOCKET_EALREADY      		EALREADY
#define RMF_OSAL_SOCKET_EBADF         		EBADF
#define RMF_OSAL_SOCKET_ECONNABORTED  		ECONNABORTED
#define RMF_OSAL_SOCKET_ECONNREFUSED  		ECONNREFUSED
#define RMF_OSAL_SOCKET_ECONNRESET    		ECONNRESET
#define RMF_OSAL_SOCKET_EDESTADDRREQ  		EDESTADDRREQ
#define RMF_OSAL_SOCKET_EDOM          		EDOM
#define RMF_OSAL_SOCKET_EHOSTNOTFOUND 		-1 /* FINISH HOST_NOT_FOUND */
#define RMF_OSAL_SOCKET_EHOSTUNREACH  		EHOSTUNREACH
#define RMF_OSAL_SOCKET_EINTR         		EINTR
#define RMF_OSAL_SOCKET_EIO           		EIO
#define RMF_OSAL_SOCKET_EINPROGRESS   		EINPROGRESS
#define RMF_OSAL_SOCKET_EISCONN       		EISCONN
#define RMF_OSAL_SOCKET_ELOOP         		ELOOP
#define RMF_OSAL_SOCKET_EMFILE        		EMFILE
#define RMF_OSAL_SOCKET_EMSGSIZE      		EMSGSIZE
#define RMF_OSAL_SOCKET_ENAMETOOLONG  		ENAMETOOLONG
#define RMF_OSAL_SOCKET_ENFILE        		ENFILE
#define RMF_OSAL_SOCKET_ENETDOWN      		ENETDOWN
#define RMF_OSAL_SOCKET_ENETUNREACH   		ENETUNREACH
#define RMF_OSAL_SOCKET_ENOBUFS       		ENOBUFS
#define RMF_OSAL_SOCKET_ENOPROTOOPT   		ENOPROTOOPT
#define RMF_OSAL_SOCKET_ENORECOVERY   		-1 /* FINISH WSANO_RECOVERY */
#define RMF_OSAL_SOCKET_ENOSPC        		NOSPC
#define RMF_OSAL_SOCKET_ENOTCONN      		ENOTCONN
#define RMF_OSAL_SOCKET_ENOTSOCK      		ENOTSOCK
#define RMF_OSAL_SOCKET_EOPNOTSUPP    		EOPNOTSUPP
#define RMF_OSAL_SOCKET_EPIPE         		EPIPE
#define RMF_OSAL_SOCKET_EPROTO        		EPROTO
#define RMF_OSAL_SOCKET_EPROTONOSUPPORT		EPROTONOSUPPORT
#define RMF_OSAL_SOCKET_EPROTOTYPE     		EPROTOTYPE
#define RMF_OSAL_SOCKET_ETIMEDOUT      		ETIMEDOUT
#define RMF_OSAL_SOCKET_ETRYAGAIN      		-1 /* FINISH WSATRY_AGAIN */
#define RMF_OSAL_SOCKET_EWOULDBLOCK    		EWOULDBLOCK

/** @} */ //end of doxygen tag OSAL_SOCKET_TYPES

/****************************************************************************************
 *
 * SOCKET FUNCTIONS
 *
 ***************************************************************************************/

/**
 * @addtogroup OSAL_SOCKET_API
 * @{
 */

rmf_osal_Bool rmf_osal_socketInit(void);

void rmf_osal_socketTerm(void);

int rmf_osal_socketGetLastError(void);

rmf_osal_Socket rmf_osal_socketAccept(rmf_osal_Socket socket, rmf_osal_SocketSockAddr *address,
        rmf_osal_SocketSockLen *address_len);

int rmf_osal_socketBind(rmf_osal_Socket socket, const rmf_osal_SocketSockAddr *address,
        rmf_osal_SocketSockLen address_len);

int rmf_osal_socketClose(rmf_osal_Socket socket);

int rmf_osal_socketConnect(rmf_osal_Socket socket, const rmf_osal_SocketSockAddr *address,
        rmf_osal_SocketSockLen address_len);

rmf_osal_Socket rmf_osal_socketCreate(int domain, int type, int protocol);

void rmf_osal_socketFDClear(rmf_osal_Socket fd, rmf_osal_SocketFDSet *fdset);

int rmf_osal_socketFDIsSet(rmf_osal_Socket fd, rmf_osal_SocketFDSet *fdset);

void rmf_osal_socketFDSet(rmf_osal_Socket fd, rmf_osal_SocketFDSet *fdset);

void rmf_osal_socketFDZero(rmf_osal_SocketFDSet *fdset);

rmf_osal_SocketHostEntry *rmf_osal_socketGetHostByAddr(const void *addr,
        rmf_osal_SocketSockLen len, int type);

rmf_osal_SocketHostEntry *rmf_osal_socketGetHostByName(const char *name);

int rmf_osal_socketGetHostName(char *name, size_t namelen);

int rmf_osal_socketGetSockName(rmf_osal_Socket socket, rmf_osal_SocketSockAddr *address,
        rmf_osal_SocketSockLen *address_len);

int rmf_osal_socketGetOpt(rmf_osal_Socket socket, int level, int option_name,
        void *option_value, rmf_osal_SocketSockLen *option_len);

int rmf_osal_socketGetPeerName(rmf_osal_Socket socket, rmf_osal_SocketSockAddr *address,
        rmf_osal_SocketSockLen *address_len);

uint32_t rmf_osal_socketHtoNL(uint32_t hostlong);

uint16_t rmf_osal_socketHtoNS(uint16_t hostshort);

uint32_t rmf_osal_socketNtoHL(uint32_t netlong);

uint16_t rmf_osal_socketNtoHS(uint16_t netshort);

int rmf_osal_socketIoctl(rmf_osal_Socket socket, int request, ...);

int rmf_osal_socketListen(rmf_osal_Socket socket, int backlog);

int rmf_osal_socketAtoN(const char *strptr, rmf_osal_SocketIPv4Addr *addrptr);

char *rmf_osal_socketNtoA(rmf_osal_SocketIPv4Addr inaddr);

#ifdef RMF_OSAL_FEATURE_IPV6
/**
 * The rmf_osal_socketNtoP() function shall convert a numeric address into a text string
 * suitable for presentation. 
 *
 * @param af address family
 * @param src the source form of the address
 * @param dst the destination form of the address
 * @param size the size of the buffer pointed to by <i>dst</i>
 * @return This function shall return a pointer to the buffer containing the text string
 *          if the conversion succeeds, and NULL otherwise. In the latter case one of the
 *          following error codes can be retrieved with rmf_osal_socketGetLastError():
 * <ul>
 * <li>     RMF_OSAL_SOCKET_EAFNOSUPPORT - The <i>af</i> argument is invalid.
 * <li>     RMF_OSAL_SOCKET_ENOSPC - The size of the result buffer is inadequate.
 * </ul>
 * @see POSIX function inet_ntop()
 */
const char *rmf_osal_socketNtoP(int af, const void *src, char *dst, size_t size);
#endif

#ifdef RMF_OSAL_FEATURE_IPV6
/**
 * The rmf_osal_socketPtoN() function shall convert an address in its standard text presentation
 * form into its numeric binary form. 
 *
 * @param af address family
 * @param src the source form of the address
 * @param dst the destination form of the address
 * @return This function shall return 1 if the conversion succeeds, with the address pointed
 *          to by <i>dst</i> in network byte order. It shall return 0 if the input is not a
 *          valid IPv4 dotted-decimal string or a valid IPv6 address string, or -1 on error.
 *          In the latter case one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * <ul>
 * <li>     RMF_OSAL_SOCKET_EAFNOSUPPORT - The <i>af</i> argument is invalid.
 * </ul>
 * @see POSIX function inet_pton()
 * @see A more extensive description of the standard representations of IPv6 addresses can
 *          be found in RFC 2373.
 */
int rmf_osal_socketPtoN(int af, const char *src, void *dst);
#endif

size_t rmf_osal_socketRecv(rmf_osal_Socket socket, void *buffer, size_t length,
        int flags);

size_t rmf_osal_socketRecvFrom(rmf_osal_Socket socket, void *buffer, size_t length,
        int flags, rmf_osal_SocketSockAddr *address, rmf_osal_SocketSockLen *address_len);

int rmf_osal_socketSelect(int numfds, rmf_osal_SocketFDSet *readfds,
        rmf_osal_SocketFDSet *writefds, rmf_osal_SocketFDSet *errorfds,
        const rmf_osal_TimeVal *timeout);

size_t rmf_osal_socketSend(rmf_osal_Socket socket, const void *buffer, size_t length,
        int flags);

size_t rmf_osal_socketSendTo(rmf_osal_Socket socket, const void *message,
        size_t length, int flags, const rmf_osal_SocketSockAddr *dest_addr,
        rmf_osal_SocketSockLen dest_len);

int rmf_osal_socketSetOpt(rmf_osal_Socket socket, int level, int option_name,
        const void *option_value, rmf_osal_SocketSockLen option_len);

int rmf_osal_socketShutdown(rmf_osal_Socket socket, int how);

rmf_Error rmf_osal_socketGetInterfaces(rmf_osal_SocketNetIfList **netIfList);

void rmf_osal_socketFreeInterfaces(rmf_osal_SocketNetIfList *netIfList);

int rmf_osal_socket_getMacAddress(char* displayName, char* macAddress);

/** @} */ //end of doxygen tag OSAL_SOCKET_API

#ifdef __cplusplus
}
#endif
#endif
