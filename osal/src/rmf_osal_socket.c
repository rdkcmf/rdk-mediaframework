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
 * @file rmf_osal_socket.c
 * @brief The source file contains API for interacting with OSAL Socket.
 */
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>  /* getsockopt(2) */

/* gethostbyaddr(3), getpeername(2), getsockname(3), getsockopt(2) */
#include <sys/socket.h>

#include <sys/ioctl.h>

#include <unistd.h>     /* close(2), fcntl(2), gethostname(2) */
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <signal.h>		// sigignore()

#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>

#include <rmf_osal_types.h>
#include <rmf_osal_socket.h>
#include <rmf_osal_mem.h>
#include "rdk_debug.h"
//#include <rmf_osal_util.h>

//#include "vlMpeosPodIf.h"

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

/* BYTEORDER(3): htonl(), htons(), ntohl(), ntohs() */
#include <netinet/in.h>

#define RMF_OSAL_MEM_DEFAULT RMF_OSAL_MEM_NET

/**
 * @brief This function is used to get the MAC address of the current machine if no
 * display name is supplied. 
 *
 * If a network interface display name is supplied, find that
 * network interface and return the physical address associated with that interface.
 *
 * @param[in] displayName   name of specific network interface, if null, look for
 *            first interface which has an associated physical address.
 * @param[out] macAddress the buffer into which the MAC address should be written (returned).
 *            maybe null if no physical address is associated with specified interface
 *            name.
 *
 * @return 0 if successful, -1 if problems encountered
 */
int rmf_osal_socket_getMacAddress(char* displayName, char* macAddress)
{
    /* Get Ethernet interface information from the OS
     * (since we don't have a real RF interface on the Simulator)
     * Note: this string will be in the format "12:34:56:78:9A:BC" */

    struct ifreq ifr;
    struct ifreq *IFR;
    struct ifconf ifc;
    char buf[1024];
    int s, i;
    int ok = 0;
    char *NO_MAC = "00:00:00:00:00:00";

    s = socket(AF_INET, SOCK_DGRAM, 0);

    if (s != -1)
    {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (-1 == ioctl(s, SIOCGIFCONF, &ifc) )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.NET",
             "ioctl SIOCGIFCONF failed\n");
			close(s);
            return -1;
        }
        IFR = ifc.ifc_req;

        for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; IFR++)
        {
            strcpy(ifr.ifr_name, IFR->ifr_name);

            if (ioctl(s, SIOCGIFFLAGS, &ifr) == 0)
            {
                if (((!(ifr.ifr_flags & IFF_LOOPBACK)) && (displayName == NULL)) ||
                		((displayName != NULL) && (strcmp(displayName, ifr.ifr_name) == 0)))
                {
                    if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0)
                    {
                        ok = 1;
                        break;
                    }
                }
            }
        }

        close(s);

        if (ok)
        {
            // Get ethernet information.
            sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
                    (unsigned char) ifr.ifr_hwaddr.sa_data[0],
                    (unsigned char) ifr.ifr_hwaddr.sa_data[1],
                    (unsigned char) ifr.ifr_hwaddr.sa_data[2],
                    (unsigned char) ifr.ifr_hwaddr.sa_data[3],
                    (unsigned char) ifr.ifr_hwaddr.sa_data[4],
                    (unsigned char) ifr.ifr_hwaddr.sa_data[5]);
            strcpy(macAddress, buf);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.NET",
                    "rmf_osal_socket_getMacAddress: Unable to get mac\n");
            strcpy(macAddress, NO_MAC);
        }
    }

    return 0;
}

/**
 * @brief Get the type of the current machine if no display name is supplied.
 * If a network interface display name is supplied, find that network interface
 * and return the physical address associated with that interface.
 *
 * @param[in] displayName name of specific network interface, if null, look for
 *            first interface which has an associated physical address.
 * @param[in] type returns numeric representation of interface type the associated with specified
 *            interface name. Types are defined in org.ocap.hn.NetworkInterface as follows:
 * <ul>
 * <li>       MOCA = 1;
 * <li>       WIRED_ETHERNET = 2;
 * <li>       WIRELESS_ETHERNET = 3;
 * <li>       UNKNOWN = 0;
 * </ul>
 *
 * @retval 0 if successful, -1 if problems encountered
 * @retval -1 problems encountered
 */
int os_getNetworkInterfaceType(char* displayName, int* type)
{
    char buf[1024] =
    { 0 };
    struct ifconf ifc =
    { 0 };
    struct ifreq *ifr = NULL;
    int sock = 0;
    int nInterfaces = 0;
    int i = 0;

    /* Map OS network interface type to OCAP HN type */
    int WIRED_ETHERNET = 2;
    int WIRELESS_ETHERNET = 3;
    int UNKNOWN = 0;

    /* Get a socket handle. */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return -1;
    }

    /* Query available interfaces. */
    // GORP: what if buffer is too small??
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0)
    {
        close(sock);
    	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.NET",
    	"Problems getting interface info from socket\n");
        return -1;
    }

    /* Iterate through the list of interfaces. */
    ifr = ifc.ifc_req;
    nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
    for (i = 0; i < nInterfaces; i++)
    {
        struct ifreq *item = &ifr[i];

        if ((NULL != item->ifr_name) && (strcmp(item->ifr_name, displayName) == 0))
        {
            if (ioctl(sock, SIOCGIFHWADDR, item) < 0)
            {
                close(sock);
            	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.NET",
            	"Problems getting hardware address info from socket\n");
                return -1;
            }

        	// Determine this network interface type
        	switch (item->ifr_hwaddr.sa_family)
        	{
        	case ARPHRD_ETHER:
        	case ARPHRD_EETHER:
        		*type = WIRED_ETHERNET;
        		break;
        	case ARPHRD_IEEE802:
        		*type = WIRELESS_ETHERNET;
        		break;
        	default:
        		*type = UNKNOWN;
        		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET",
        			"os_getNetworkInterfaceType() - unrecognized interface type = %d\n",
        				item->ifr_hwaddr.sa_family);
        	}
        	break;
        }
    }

    close(sock);

    return 0;
}

/**
 * @brief This function is used to initialize the socket API. 
 * This must be called before any other function with a prefix of rmf_osal_socket().
 *
 * @return Upon successful completion, this function shall return TRUE. Otherwise, FALSE
 *   is returned to indicate that the socket subsystem could not be initialized.
 * @return None
 */
rmf_osal_Bool rmf_osal_socketInit(void)
{
    rmf_osal_Bool ret = TRUE;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "rmf_osal_socketInit() ...\n");

	/**
	 * Requests not to send SIGPIPE on errors on stream oriented
	 * sockets when the other end breaks the connection. The EPIPE 
	 * error is still returned. 
     */
#if 1
/*ericz, sigignore is obsoleted*/
    signal(SIGPIPE, SIG_IGN);
#else
    sigignore(SIGPIPE);
#endif
    return(ret); 
}

/**
 * @brief This function is used to terminate the socket API and this should be called when use of the socket API
 * (functions beginning with rmf_osal_socket) is no longer required.
 * @return None
 */
void rmf_osal_socketTerm(void)
{
}

/**
 * @brief This function is used to get the error status of the last socket function that failed.
 *
 * @return Error status of last socket function that failed
 */
int rmf_osal_socketGetLastError(void)
{
    return errno;
}

/* Jonathan Knudsen
 * May 21, 2009
 *
 * Most of the underlying socket functions set errno if something goes wrong.
 * However, gethostbyaddr() and gethostbyname() actually set h_errno instead. This
 * causes a problem in rmf_osal_socketGetLastError(), which simply returns the value
 * of errno.
 *
 * To fix this, the two functions that call gethostbyaddr() and gethostbyname()
 * will explicitly set errno when those functions fail. However, it is necesary
 * to translate between h_errno values and RMF_OSAL error values.
 *
 */
static int translate_h_errno_to_errno(int e) {
    int rmf_osal_error;
    
    switch (e) {
    case HOST_NOT_FOUND:
        rmf_osal_error = RMF_OSAL_SOCKET_EHOSTNOTFOUND;
        break;
    case NO_DATA:
        rmf_osal_error = RMF_OSAL_ENODATA;
        break;
    case NO_RECOVERY:
        rmf_osal_error = RMF_OSAL_SOCKET_ENORECOVERY;
        break;
    case TRY_AGAIN:
        rmf_osal_error = RMF_OSAL_SOCKET_ETRYAGAIN;
        break;
    case EBADF:
        rmf_osal_error = RMF_OSAL_SOCKET_EBADF;
        break;
    case EINVAL:
        rmf_osal_error = RMF_OSAL_EINVAL;
        break;
    case ENOTCONN:
        rmf_osal_error = RMF_OSAL_SOCKET_ENOTCONN;
        break;
    case ENOTSOCK:
        rmf_osal_error = RMF_OSAL_SOCKET_ENOTSOCK;
        break;
    case ENOBUFS:
        rmf_osal_error = RMF_OSAL_SOCKET_ENOBUFS;
        break;
    default:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "Unrecognized h_errno in rmf_osal_socketGetLastError()\n");
        rmf_osal_error = RMF_OSAL_EINVAL;
        break;
    }

	return rmf_osal_error;
}

/**
 * @brief This function is used to extract the first connection on the queue of pending connections, 
 * create a new socket with the same socket type protocol and address family as the specified 
 * <i>socket</i>, and allocate a new file descriptor for that socket.
 * <p>
 * If <i>address</i> is not a null pointer, the address of the peer for the accepted
 * connection shall be stored in the rmf_osal_SocketSockAddr structure pointed to by
 * <i>address</i>, and the length of this address shall be stored in the object pointed
 * to by <i>address_len</i>.
 * <p>
 * If the actual length of the address is greater than the length of the supplied
 * rmf_osal_SocketSockAddr structure, the stored address shall be truncated.
 * <p>
 * If the protocol permits connections by unbound clients, and the peer is not bound, then
 * the value stored in the object pointed to by <i>address</i> is unspecified.
 * <p>
 * If the listen queue is empty of connection requests this function shall block until a
 * connection is present.
 * <p>
 * The accepted socket cannot itself accept more connections. The original socket remains
 * open and can accept more connections.
 *
 * @param[in] socket specifies a socket that was created with rmf_osal_socketCreate(), has
 *          been bound to an address with rmf_osal_socketBind(), and has issued a
 *          successful call to rmf_osal_socketListen().
 * @param[in] address either a null pointer, or a pointer to a rmf_osal_SocketSockAddr structure
 *          where the address of the connecting socket shall be returned.
 * @param[in] address_len points to a length parameter which on input specifies the length of the
 *          supplied rmf_osal_SocketSockAddr structure, and on output specifies the length
 *          of the stored address.
 * @return Upon successful completion, this function shall return the file descriptor of the
 *          accepted socket. Otherwise, RMF_OSAL_SOCKET_INVALID_SOCKET shall be returned and one
 *          of the following error codes can be retrieved with rmf_osal_socketGetLastError():
 * @retval RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval RMF_OSAL_SOCKET_ECONNABORTED - A connection has been aborted.
 * @retval RMF_OSAL_SOCKET_EINTR - This function was interrupted by a signal that was caught before a valid connection arrived.
 * @retval RMF_OSAL_EINVAL - The socket is not accepting connections.
 * @retval RMF_OSAL_SOCKET_EMFILE - No more file descriptors are available for this process.
 * @retval RMF_OSAL_SOCKET_ENFILE - No more file descriptors are available for the system.
 * @retval RMF_OSAL_SOCKET_ENOBUFS - No buffer space is available.
 * @retval RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * @retval RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval RMF_OSAL_SOCKET_EOPNOTSUPP - The socket type of the specified socket does not support accepting connections.
 * @retval RMF_OSAL_SOCKET_EPROTO - A protocol error has occurred.
 * @retval RMF_OSAL_ETHREADDEATH - The thread executing this function has been marked for death.
 * @see POSIX function accept()
 */
rmf_osal_Socket rmf_osal_socketAccept(rmf_osal_Socket socket, rmf_osal_SocketSockAddr *address,
                              rmf_osal_SocketSockLen *address_len)
{
    return accept(socket, address, address_len);
}

/**
 * @brief This function is used to assign a local socket address to a socket identified 
 * by descriptor <i>socket</i> that has no local socket address assigned. Sockets created with 
 * the rmf_osal_socketCreate() function are initially unnamed; they are identified only by their 
 * address family.
 *
 * @param[in] socket specifies the file descriptor of the socket to be bound.
 * @param[in] address points to a rmf_osal_SocketSockAddr structure containing the address to be
 *          bound to the socket. The length and format of the address depend on the address
 *          family of the socket.
 * @param[in] address_len specifies the length of the rmf_osal_SocketSockAddr structure pointed
 *          to by the <i>address</i> argument.
 * @return Upon successful completion, this function shall return 0; otherwise, -1 shall be
 *          returned and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EACCES - The specified <i>address</i> is protected and the current user does
 *          not have permission to bind to it.
 * @retval  RMF_OSAL_SOCKET_EADDRINUSE - The specified <i>address</i> is already in use.
 * @retval  RMF_OSAL_SOCKET_EADDRNOTAVAIL - The specified <i>address</i> is not available from the local
 *          machine.
 * @retval  RMF_OSAL_SOCKET_EAFNOSUPPORT - The specified <i>address</i> is not a valid address for the address
 *          family of the specified socket.
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_EINVAL - The socket is already bound to an address, and the protocol does not
 *          support binding to a new address; or the socket has been shut down.
 * @retval  RMF_OSAL_EINVAL - The <i>address_len</i> argument is not a valid length for the address family.
 * @retval  RMF_OSAL_SOCKET_EISCONN - The socket is already connected.
 * @retval  RMF_OSAL_SOCKET_ELOOP - More than the maximum number of symbolic links were encountered during
 *          resolution of the pathname in <i>address</i>.
 * @retval  RMF_OSAL_SOCKET_ENAMETOOLONG - Pathname resolution of a symbolic link produced an intermediate
 *          result whose length exceeds the maximum allowed for a pathname.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available to complete the call.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval  RMF_OSAL_SOCKET_EOPNOTSUPP - The socket type of the specified socket does not support binding
 *          to an address.
 * @see POSIX function bind()
 */
int rmf_osal_socketBind(rmf_osal_Socket socket, const rmf_osal_SocketSockAddr *address,
                     rmf_osal_SocketSockLen address_len)
{
    return bind(socket, (rmf_osal_SocketSockAddr *)address, address_len);
}

/**
 * @brief This function is used to deallocate the file descriptor indicated by
 * <i>socket</i>. To deallocate means to make the file descriptor available for return by
 * subsequent calls to rmf_osal_socketCreate() or other functions that allocate file
 * descriptors.
 * <p>
 * If this function is interrupted by a signal that is to be caught, it shall return -1
 * and the state of <i>socket</i> is unspecified. In this case rmf_osal_socketGetLastError()
 * returns RMF_OSAL_SOCKET_EINTR.
 * <p>
 * If an I/O error occurred while reading from or writing to the socket during
 * rmf_osal_socketClose(), it may return -1 and the state of <i>socket</i> is unspecified.
 * In this case rmf_osal_socketGetLastError() returns RMF_OSAL_SOCKET_EIO.
 * <p>
 * If the socket is in connection-mode, and the RMF_OSAL_SOCKET_SO_LINGER option is set for the
 * socket with non-zero linger time, and the socket has untransmitted data, then this
 * function shall block for up to the current linger interval until all data is transmitted.
 *
 * @param[in] socket specifies the file descriptor associated with the socket to be closed.
 * @return Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned
 *          and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_SOCKET_EINTR - This function was interrupted by a signal.
 * @retval  RMF_OSAL_SOCKET_EIO - An I/O error occurred while reading from or writing to the socket.
 * @retval  RMF_OSAL_ETHREADDEATH - The thread executing this function has been marked for death.
 * @see POSIX function close()
 */
int rmf_osal_socketClose(rmf_osal_Socket socket)
{
    return close((int) socket);
}

/**
 * @brief This function is used to attempt to make a connection on a socket. If the socket has
 * not already been bound to a local address, this function shall bind it to an unused local address.
 * <p>
 * If the initiating socket is connection-mode, then this function shall set the socket's
 * peer address, and no connection is made. For RMF_OSAL_SOCKET_DATAGRAM sockets, the peer
 * address identifies where all datagrams are sent on subsequent rmf_osal_socketSend()
 * functions, and limits the remote sender for subsequent rmf_osal_socketRecv() functions. If
 * <i>address</i> is a null address for the protocol, the socket's peer address shall be reset.
 * <p>
 * If the initiating socket is non connection-mode, then this function shall attempt to establish
 * a connection to the address specified by the <i>address</i> argument. If the connection
 * cannot be established immediately this function shall block for up to an unspecified
 * timeout interval until the connection is established. If the timeout interval expires
 * before the connection is established, this function shall fail and the connection attempt
 * shall be aborted. If this function is interrupted by a signal that is caught while blocked
 * waiting to establish a connection, this function shall fail,
 * but the connection request shall not be aborted, and the connection shall be established
 * asynchronously. In this case rmf_osal_socketGetLastError() returns RMF_OSAL_SOCKET_EINTR.
 * <p>
 * When the connection has been established asynchronously, <i>rmf_osal_socketSelect()</i> shall
 * indicate that the file descriptor for the socket is ready for writing.
 *
 * @param[in] socket specifies the file descriptor associated with the socket.
 * @param[in] address points to a rmf_osal_SocketSockAddr structure containing the peer address.
 *          The length and format of the address depend on the address family of the socket.
 * @param[in] address_len specifies the length of the rmf_osal_SocketSockAddr structure pointed
 *          to by the <i>address</i> argument.
 * @return Upon successful completion, this function shall return 0; otherwise, -1 shall
 *          be returned and and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EACCES - Search permission is denied for a component of the path prefix;
 *          or write access to the named socket is denied.
 * @retval  RMF_OSAL_SOCKET_EADDRINUSE - Attempt to establish a connection that uses addresses that are
 *          already in use.
 * @retval  RMF_OSAL_SOCKET_EADDRNOTAVAIL - The specified <i>address</i> is not available from the local
 *          machine.
 * @retval  RMF_OSAL_SOCKET_EAFNOSUPPORT - The specified <i>address</i> is not a valid address for the
 *          address family of the specified socket.
 * @retval  RMF_OSAL_SOCKET_EALREADY - A connection request is already in progress for the specified
 *          socket.
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_SOCKET_ECONNREFUSED - The target address was not listening for connections or
 *          refused the connection request.
 * @retval  RMF_OSAL_SOCKET_ECONNRESET - Remote host reset the connection request.
 * @retval  RMF_OSAL_SOCKET_EHOSTUNREACH - The destination host cannot be reached (probably because the
 *          host is down or a remote router cannot reach it).
 * @retval  RMF_OSAL_SOCKET_EINTR - The attempt to establish a connection was interrupted by delivery of
 *          a signal that was caught; the connection shall be established asynchronously.
 * @retval  RMF_OSAL_EINVAL - The <i>address_len</i> argument is not a valid length for the address
 *          family; or invalid address family in the rmf_osal_SocketSockAddr structure.
 * @retval  RMF_OSAL_SOCKET_EISCONN - The specified socket is connection-mode and is already connected.
 * @retval  RMF_OSAL_SOCKET_ELOOP - More than the maximum number of symbolic links were encountered
 *          during resolution of the pathname in <i>address</i>.
 * @retval  RMF_OSAL_SOCKET_ENAMETOOLONG - Pathname resolution of a symbolic link produced an intermediate
 *          result whose length exceeds the maximum allowed for a pathname.
 * @retval  RMF_OSAL_SOCKET_ENETDOWN - The local network interface used to reach the destination is down.
 * @retval  RMF_OSAL_SOCKET_ENETUNREACH - No route to the network is present.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - No buffer space is available.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval  RMF_OSAL_SOCKET_EOPNOTSUPP - The socket is listening and cannot be connected.
 * @retval  RMF_OSAL_SOCKET_EPROTOTYPE - The specified <i>address</i> has a different type than the
 *          socket bound to the specified peer address.
 * @retval  RMF_OSAL_ETHREADDEATH - The thread executing this function has been marked for death.
 * @retval  RMF_OSAL_SOCKET_ETIMEDOUT - The attempt to connect timed out before a connection was made.
 * @see POSIX function connect()
 */
int rmf_osal_socketConnect(rmf_osal_Socket socket, const rmf_osal_SocketSockAddr *address,
                        rmf_osal_SocketSockLen address_len)
{
    return connect(socket, (rmf_osal_SocketSockAddr *)address, address_len);
}

/**
 * @brief This function is used to create an unbound socket in a communications
 * domain, and return a file descriptor that can be used in later function calls that
 * operate on sockets.
 * <p>
 * The <i>domain</i> argument specifies the address family used in the communications domain.
 * The only address families currently supported are RMF_OSAL_SOCKET_AF_INET4 and RMF_OSAL_SOCKET_AF_INET6.
 * <p>
 * The <i>type</i> argument specifies the socket type, which determines the semantics of
 * communication over the socket. The following socket types are currently supported:
 * <ul>
 * <li> RMF_OSAL_SOCKET_STREAM - provides sequenced, reliable, bidirectional, connection-mode
 *      byte streams.
 * <li> RMF_OSAL_SOCKET_DATAGRAM - provides datagrams, which are connectionless-mode, unreliable
 *      messages of fixed maximum length.
 * </ul>
 * <p>
 * If the <i>protocol</i> argument is non-zero, it shall specify a protocol that is supported
 * by the address family. If the <i>protocol</i> argument is zero, the default protocol for this
 * address family and type shall be used.
 *
 * @param[in] domain specifies the communications domain in which a socket is to be created.
 * @param[in] type specifies the type of socket to be created.
 * @param[in] protocol specifies a particular protocol to be used with the socket. Specifying
 *          a protocol of 0 causes this function to use an unspecified default protocol
 *          appropriate for the requested socket type.
 * @return Upon successful completion, this function shall return a valid socket descriptor.
 *          Otherwise, a value of RMF_OSAL_SOCKET_INVALID_SOCKET shall be returned and
 *          one of the following error codes can be retrieved with rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EACCES - The process does not have appropriate privileges.
 * @retval  RMF_OSAL_SOCKET_EAFNOSUPPORT - The implementation does not support the specified address family.
 * @retval  RMF_OSAL_SOCKET_EMFILE - No more file descriptors are available for this process.
 * @retval  RMF_OSAL_SOCKET_ENFILE - No more file descriptors are available for the system.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available in the system to perform the operation.
 * @retval  RMF_OSAL_ENOMEM - Insufficient memory was available to fulfill the request.
 * @retval  RMF_OSAL_SOCKET_EPROTONOSUPPORT - The <i>protocol</i> is not supported by the address
 *          family, or the <i>protocol</i> is not supported by the implementation.
 * @retval  RMF_OSAL_SOCKET_EPROTOTYPE - The socket <i>type</i> is not supported by the <i>protocol</i>.
 * @see POSIX function socket()
 */
rmf_osal_Socket rmf_osal_socketCreate(int domain, int type, int protocol)
{
    /*
     * Initiate fast boot if not in 2-way mode yet,
     * to insure that our network connection to the head-end is set-up
     */
    // DEBUG
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "CALLING rmf_osal_stbBoot() ...\n");
    // DEBUG
    // JAN-10-2011: Commented nop call to.. // rmf_osal_stbBoot(RMF_OSAL_BOOTMODE_FAST);
    // DEBUG
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "CALLING socket() ...\n");
    // DEBUG
    return socket(domain, type, protocol);
}

/**
 * @brief This function is used to remove the file descriptor 
 * <i>fd</i> from the set pointed to by <i>fdset</i>. If <i>fd</i> is not a member of this set, 
 * there shall be no effect on the set, nor will an error be returned. The behavior of this 
 * function is undefined if the <i>fd</i> argument is less than 0 or greater than or equal to 
 * RMF_OSAL_SOCKET_FD_SETSIZE, or if <i>fd</i> is not a valid file descriptor.
 *
 * @param[in] fd file descriptor to be removed from <i>fdset</i>
 * @param[in] fdset file descriptor set to be modified
 * @return None
 * @see POSIX macro FD_CLR()
 */
void rmf_osal_socketFDClear(rmf_osal_Socket fd, rmf_osal_SocketFDSet *fdset)
{
    if (fdset == NULL)
    {
        return;
    }
    FD_CLR(fd, fdset);
}

/**
 * @brief This function is used to determine whether the file descriptor 
 * <i>fd</i> is a member of the set pointed to by <i>fdset</i>. The behavior of this 
 * function is undefined if the <i>fd</i> argument is less than 0 or greater than or 
 * equal to RMF_OSAL_SOCKET_FD_SETSIZE, or if <i>fd</i> is not a valid file descriptor.
 *
 * @param[in] fd file descriptor within fdset to be checked
 * @param[in] fdset file descriptor set to be checked
 * @return A non-zero value if the bit for the file descriptor <i>fd</i> is set in
 *          the file descriptor set pointed to by <i>fdset</i>, and 0 otherwise.
 * @see POSIX macro FD_ISSET()
 */
int rmf_osal_socketFDIsSet(rmf_osal_Socket fd, rmf_osal_SocketFDSet *fdset)
{
    if (fdset == NULL)
    {
        return 0;
    }
    return FD_ISSET(fd, fdset);
}

/**
 * @brief This function is used to add the file descriptor <i>fd</i> to the set pointed
 * to by <i>fdset</i>. If the file descriptor <i>fd</i> is already in this set, there 
 * shall be no effect on the set, nor will an error be returned. The behavior of this 
 * function is undefined if the <i>fd</i> argument is less than 0 or greater than or 
 * equal to RMF_OSAL_SOCKET_FD_SETSIZE, or if <i>fd</i> is not a valid file descriptor.
 *
 * @param[in] fd file descriptor to be added to <i>fdset</i>
 * @param[in] fdset file descriptor set to be modified
 * @return None
 * @see POSIX macro FD_SET()
 */
void rmf_osal_socketFDSet(rmf_osal_Socket fd, rmf_osal_SocketFDSet *fdset)
{
    if (fdset == NULL)
    {
        return;
    }
    FD_SET(fd, fdset);
}

/**
 * @brief This function is used to initialize the descriptor set pointed to by 
 * <i>fdset</i> to the null set. No error is returned if the set is not empty at the time 
 * this function is invoked. The behavior of this function is undefined if the <i>fd</i>
 * argument is less than 0 or greater than or equal to RMF_OSAL_SOCKET_FD_SETSIZE, or if 
 * <i>fd</i> is not a valid file descriptor.
 *
 * @param[in] fdset file descriptor set to be initialized
 * @return None
 * @see POSIX macro FD_ZERO()
 */
void rmf_osal_socketFDZero(rmf_osal_SocketFDSet *fdset)
{
    if (fdset == NULL)
    {
        return;
    }
    FD_ZERO(fdset);
}

/**
 * @brief This function is used to return an entry containing addresses of address family type
 * for the host with address <i>addr</i>. The <i>len</i> argument contains the length of the
 * address pointed to by <i>addr</i>. This information is considered to be stored in a database
 * that can be accessed sequentially or randomly. Implementation of this database is unspecified.
 * <p>
 * The <i>addr</i> argument shall be an rmf_osal_SocketIPv4Addr structure when type is RMF_OSAL_SOCKET_AF_INET4 or
 * an rmf_osal_SocketIPv6Addr structure when type is RMF_OSAL_SOCKET_AF_INET6. It
 * contains a binary format (that is, not null-terminated) address in network byte order.
 * This function is not guaranteed to return addresses of address families other than
 * RMF_OSAL_SOCKET_AF_INET4 or RMF_OSAL_SOCKET_AF_INET6, even when such addresses exist in the database.
 * <p>
 * If this function returns successfully, then the h_addrtype field in the result shall
 * be the same as the <i>type</i> argument that was passed to the function, and the
 * h_addr_list field shall list a single address that is a copy of the <i>addr</i> argument
 * that was passed to the function.
 *
 * @param[in] addr pointer to the address in network byte order
 * @param[in] len length of the address
 * @param[in] type type of the address
 * @return Upon successful completion, this function shall return a pointer to a
 *          rmf_osal_SocketHostEntry structure if the requested entry was found. The caller must never
 *          attempt to modify this structure or to free any of its components. Furthermore,
 *          only one copy of this structure is allocated, so the caller should copy
 *          any information it needs before calling any other socket function. On error,
 *          a null pointer is returned and one of the following error codes can be retrieved
 *          with rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EHOSTNOTFOUND - No such host is known.
 * @retval  RMF_OSAL_ENODATA - The server recognized the request and the <i>name</i>, but no
 *          address is available. Another type of request to the name server for the
 *          domain might return an answer.
 * @retval  RMF_OSAL_SOCKET_ENORECOVERY - An unexpected server failure occurred which cannot be recovered.
 * @retval  RMF_OSAL_SOCKET_ETRYAGAIN - A temporary and possibly transient error occurred, such as a
 *          failure of a server to respond.
 * @see POSIX function gethostbyaddr()
 */
rmf_osal_SocketHostEntry *rmf_osal_socketGetHostByAddr(const void *addr,
        rmf_osal_SocketSockLen len, int type)
{
    rmf_osal_SocketHostEntry *hostEntry = NULL;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "****** Looking up host by address\n");
        
    /***
     *** does Linux impl need to perform a deep-copy of "struct hostent"
     ***        to rmf_osal_SocketHostEntry?
     ***/
    hostEntry = (rmf_osal_SocketHostEntry *) gethostbyaddr((const char *) addr,
            (int) len, type);

    if (hostEntry == NULL)
        errno = translate_h_errno_to_errno(h_errno); // Do this sormf_osal_socketGetLastError() works right
    return hostEntry;
}

/**
 * @brief This function is used return an entry containing addresses of address family 
 * RMF_OSAL_SOCKET_AF_INET4 or RMF_OSAL_SOCKET_AF_INET6 for the host with name <i>name</i>.
 * This information is considered to be stored in a database that can be accessed
 * sequentially or randomly. Implementation of this database is unspecified.
 * <p>
 * The <i>name</i> argument shall be a node name; the behavior of this function when
 * passed a numeric address string is unspecified. For IPv4, a numeric address string
 * shall be in dotted-decimal notation.
 * <p>
 * If <i>name</i> is not a numeric address string and is an alias for a valid host name,
 * then this function shall return information about the host name to which the alias
 * refers, and name shall be included in the list of aliases returned.
 * <p>
 * If <i>name</i> is identical to the local host name as returned by <i>rmf_osal_socketGetHostName() </i>,
 * the returned <i>rmf_osal_SocketHostEntry</i> structure shall contain a single address only.  That
 * address must identify the OCAP1.0 return channel or DOCSIS interface.
 *
 * @param[in] name the host name
 * @return Upon successful completion, this function shall return a pointer to a
 *          rmf_osal_SocketHostEntry structure if the requested entry was found. The caller must never
 *          attempt to modify this structure or to free any of its components. Furthermore,
 *          only one copy of this structure is allocated, so the caller should copy
 *          any information it needs before calling any other socket function. On error,
 *          a null pointer is returned and one of the following error codes can be retrieved
 *          with rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EHOSTNOTFOUND - No such host is known.
 * @retval  RMF_OSAL_ENODATA - The server recognized the request and the <i>name</i>, but no
 *          address is available. Another type of request to the name server for the
 *          domain might return an answer.
 * @retval  RMF_OSAL_SOCKET_ENORECOVERY - An unexpected server failure occurred which cannot be recovered.
 * @retval  RMF_OSAL_SOCKET_ETRYAGAIN - A temporary and possibly transient error occurred, such as a
 *          failure of a server to respond.
 * @see POSIX function gethostbyname()
 */
rmf_osal_SocketHostEntry *rmf_osal_socketGetHostByName(const char *name)
{
   char hostName[128];
    int ret = rmf_osal_socketGetHostName(hostName, 127);
    rmf_osal_SocketHostEntry *hostEntry = NULL;

    if(NULL == name)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.NET", "%s: NULL parameter passed.\n", __FUNCTION__);
        return NULL;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "%s(%s)\n", __func__, name);

    if ((0 == ret) && (0 == strcmp(name, hostName)))
    {
#if 0
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.NET",
                "getting host with ri_cablecard_gethostbyname NOW(%s)'\n", name);
        hostEntry = (rmf_osal_SocketHostEntry *) ri_cablecard_gethostbyname(name);
#else /* Avoiding call to ri_cablecard_gethostbyname()*/
        hostEntry = (rmf_osal_SocketHostEntry *) gethostbyname(name);
#endif
    }
    else
    {
        /***
         *** does Linux impl need to perform a deep-copy of
         ***        "struct hostent" to rmf_osal_SocketHostEntry?
         ***/
        hostEntry = (rmf_osal_SocketHostEntry *) gethostbyname(name);
    }

    if (hostEntry == NULL)
        errno = translate_h_errno_to_errno(h_errno); // Do this sormf_osal_socketGetLastError() works right
    return hostEntry;
}

/**
 * @brief This function is used to return the standard host name for the
 * current machine. The <i>namelen</i> argument shall specify the size of the array pointed
 * to by the <i>name</i> argument. The returned name is null-terminated and truncated if
 * insufficient space is provided.
 * <p>
 * Host names are limited to RMF_OSAL_SOCKET_MAXHOSTNAMELEN bytes.
 *
 * @param[in] name the buffer into which the host name should be written (returned).
 * @param[in] namelen the size of the buffer pointed to by <i>name</i>
 * @return Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned.
 * @see POSIX function gethostname()
 */
int rmf_osal_socketGetHostName(char *name, size_t namelen)
{   
    if (name == NULL)
        return -1;
    
    if (namelen == 0)
        return 0;
    
    int ret = gethostname(name, namelen);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "%d = %s(%s)\n", ret, __func__, name);
    return ret;
}

/**
 * @brief This function is used to retrieve the locally-bound name of the
 * specified socket, store this address in the rmf_osal_SocketSockAddr structure pointed to by
 * the <i>address</i> argument, and store the length of this address in the object pointed
 * to by the <i>address_len</i> argument.
 * <p>
 * If the actual length of the address is greater than the length of the supplied
 * rmf_osal_SocketSockAddr structure, the stored address shall be truncated.
 * <p>
 * If the socket has not been bound to a local name, the value stored in the object pointed
 * to by <i>address</i> is unspecified.
 *
 * @param[in] socket specifies the file descriptor associated with the socket.
 * @param[in] address points to a rmf_osal_SocketSockAddr structure where the name should be returned.
 * @param[in] address_len specifies the length of the rmf_osal_SocketSockAddr structure pointed
 *          to by the <i>address</i> argument. This is updated to reflect the length of the
 *          name actually copied to <i>address</i>.
 * @return Upon successful completion, 0 shall be returned, the <i>address</i> argument shall
 *          point to the address of the socket, and the <i>address_len</i> argument shall
 *          point to the length of the address. Otherwise, -1 shall be returned and one of
 *          the following error codes can be retrieved with rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_EINVAL - The socket has been shut down.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available in the system to complete
 *          the function.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval  RMF_OSAL_SOCKET_EOPNOTSUPP - The operation is not supported for this socket's protocol.
 * @see POSIX function getsockname()
 */
int rmf_osal_socketGetSockName(rmf_osal_Socket socket, rmf_osal_SocketSockAddr *address,
                            rmf_osal_SocketSockLen *address_len)
{
    return getsockname((int) socket, (struct sockaddr *) address,
                       (socklen_t *) address_len);
}

/**
 * @brief This function is used to manipulates options associated with a socket.
 * <p>
 * The rmf_osal_socketGetOpt() function shall retrieve the value for the option specified
 * by the <i>option_name</i> argument for the socket specified by the <i>socket</i> argument.
 * If the size of the option value is greater than <i>option_len</i>, the value stored in the
 * object pointed to by the <i>option_value</i> argument shall be silently truncated.
 * Otherwise, the object pointed to by the <i>option_len</i> argument shall be modified to
 * indicate the actual length of the value.
 * <p>
 * The <i>level</i> argument specifies the protocol level at which the option resides. To
 * retrieve options at the socket level, specify the <i>level</i> argument as RMF_OSAL_SOCKET_SOL_SOCKET. To
 * retrieve options at other levels, supply the appropriate level identifier for the protocol
 * controlling the option. For example, to indicate that an option is interpreted by the TCP
 * (Transmission Control Protocol), set <i>level</i> to RMF_OSAL_SOCKET_IPPROTO_TCP.
 * <p>
 * Valid values for <i>option_name</i> include the following where the datatype of the
 * option is in parenthesis:
 * <ul>
 * <li> RMF_OSAL_SOCKET_SO_BROADCAST - permit sending of broadcast datagrams (int)
 * <li> RMF_OSAL_SOCKET_SO_DEBUG - enable debug tracing (int)
 * <li> RMF_OSAL_SOCKET_SO_DONTROUTE - bypass routing table lookup (int)
 * <li> RMF_OSAL_SOCKET_SO_ERROR - get pending error and clear (int)
 * <li> RMF_OSAL_SOCKET_SO_KEEPALIVE - periodically test if connection still alive (int)
 * <li> RMF_OSAL_SOCKET_SO_LINGER - linger on close if data to send (rmf_osal_SocketLinger)
 * <li> RMF_OSAL_SOCKET_SO_OOBINLINE - leave received out-of-band data inline (int)
 * <li> RMF_OSAL_SOCKET_SO_RCVBUF - receive buffer size (int)
 * <li> RMF_OSAL_SOCKET_SO_SNDBUF - send buffer size (int)
 * <li> RMF_OSAL_SOCKET_SO_RCVLOWAT - receive buffer low-water mark (int)
 * <li> RMF_OSAL_SOCKET_SO_SNDLOWAT - send buffer low-water mark (int)
 * <li> RMF_OSAL_SOCKET_SO_RCVTIMEO - receive timeout (rmf_osal_TimeVal)
 * <li> RMF_OSAL_SOCKET_SO_SNDTIMEO - send timeout (rmf_osal_TimeVal)
 * <li> RMF_OSAL_SOCKET_SO_REUSEADDR - allow local address reuse (int)
 * <li> RMF_OSAL_SOCKET_SO_TYPE - get socket type (int)
 * <li> RMF_OSAL_SOCKET_IPV4_MULTICAST_IF - specify outgoing interface (rmf_osal_SocketIPv4Addr)
 * <li> RMF_OSAL_SOCKET_IPV4_MULTICAST_LOOP - specify loopback (unsigned char)
 * <li> RMF_OSAL_SOCKET_IPV4_MULTICAST_TTL - specify outgoing TTL (unsigned char)
 * <li> RMF_OSAL_SOCKET_IPV6_MULTICAST_IF - specify outgoing interface (rmf_osal_SocketIPv6Addr)
 * <li> RMF_OSAL_SOCKET_IPV6_MULTICAST_HOPS - specify outgoing hop limit (unsigned int)
 * <li> RMF_OSAL_SOCKET_IPV6_MULTICAST_LOOP - specify loopback (unsigned int)
 * <li> RMF_OSAL_SOCKET_TCP_NODELAY - disable Nagle algorithm (int)
 * </ul>
 *
 * @param[in] socket specifies the file descriptor associated with the socket.
 * @param[in] level is the protocol level at which the option resides
 * @param[in] option_name specifies a single option to be retrieved.
 * @param[in] option_value points to storage sufficient to hold the value of the option. For
 *          boolean options, a zero value indicates that the option is disabled and a
 *          non-zero value indicates that the option is enabled.
 * @param[in] option_len specifies the length of the buffer pointed to by <i>option_value</i>. It
 *          is updated by this function to indicate the number of bytes actually copied
 *          to <i>option_value</i>.
 * @return Upon successful completion, this function shall return 0; otherwise, -1 shall
 *          be returned and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EACCES - The calling process does not have the appropriate privileges.
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_EINVAL - The specified option is invalid at the specified socket level, or
 *          the socket has been shut down.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources are available in the system to complete
 *          the function.
 * @retval  RMF_OSAL_SOCKET_ENOPROTOOPT - The option is not supported by the protocol.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @see POSIX function getsockopt()
 */

int rmf_osal_socketGetOpt(rmf_osal_Socket socket, int level, int option_name,
        void *option_value, rmf_osal_SocketSockLen *option_len)
{
    return getsockopt((int) socket, level, option_name, option_value,
                      (socklen_t *) option_len);
}

/**
 * @brief The rmf_osal_socketGetPeerName() function shall retrieve the peer address of the specified
 * socket, store this address in the rmf_osal_SocketSockAddr structure pointed to by the <i>address</i>
 * argument, and store the length of this address in the object pointed to by the
 * <i>address_len</i> argument.
 * <p>
 * If the actual length of the address is greater than the length of the supplied
 * rmf_osal_SocketSockAddr structure, the stored address shall be truncated.
 * <p>
 * If the protocol permits connections by unbound clients, and the peer is not bound, then
 * the value stored in the object pointed to by <i>address</i> is unspecified.
 *
 * @param[in] socket specifies the file descriptor associated with the socket.
 * @param[in] address points to a rmf_osal_SocketSockAddr structure containing the peer address.
 * @param[in] address_len specifies the length of the rmf_osal_SocketSockAddr structure pointed
 *          to by the <i>address</i> argument.
 * @return Upon successful completion, this function shall return 0; otherwise, -1 shall
 *          be returned and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_EINVAL - The socket has been shut down.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available in the system to complete
 *          the call.
 * @retval  RMF_OSAL_SOCKET_ENOTCONN - The socket is not connected or otherwise has not had the peer
 *          pre-specified.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval  RMF_OSAL_SOCKET_EOPNOTSUPP - The operation is not supported for the socket protocol.
 * @see POSIX function getpeername()
 */
int rmf_osal_socketGetPeerName(rmf_osal_Socket socket, rmf_osal_SocketSockAddr *address,
                            rmf_osal_SocketSockLen *address_len)
{
    return getpeername((int) socket, (struct sockaddr *) address,
                       (socklen_t *) address_len);
}

/**
 * @brief This function is used to convert a 32-bit quantity from host byte order to network byte order.
 *
 * @param[in] hostlong the 32 bit value in host byte order
 * @return This function shall return <i>hostlong</i> converted to network byte order.
 * @see POSIX function htonl()
 */
uint32_t rmf_osal_socketHtoNL(uint32_t hostlong)
{
	return htonl(hostlong);
}

/**
 * @brief This function is used to convert a 16-bit quantity from host byte order to network byte order.
 *
 * @param[in] hostshort the 16 bit value in host byte order
 * @return This function shall return <i>hostshort</i> converted to network byte order.
 * @see POSIX function htons()
 */
uint16_t rmf_osal_socketHtoNS(uint16_t hostshort)
{
	return htons(hostshort);
}

/**
 * @brief The rmf_osal_socketNtoHL() function shall convert a 32-bit quantity from network byte order to
 * host byte order.
 *
 * @param[in] netlong the 32 bit value in network byte order
 * @return This function shall return <i>netlong</i> converted to host byte order.
 * @see POSIX function ntohl()
 */
uint32_t rmf_osal_socketNtoHL(uint32_t netlong)
{
	return ntohl(netlong);
}

/**
 * The rmf_osal_socketNtoHS() function shall convert a 16-bit quantity from network byte order to
 * host byte order.
 *
 * @param[in] netshort the 16 bit value in network byte order
 * @return This function shall return <i>netshort</i> converted to host byte order.
 * @see POSIX function ntohs()
 */
uint16_t rmf_osal_socketNtoHS(uint16_t netshort)
{
	return ntohs(netshort);
}

/**
 * The rmf_osal_socketIoctl() function shall perform a variety of control functions on a socket.
 * The <i>request</i> argument selects the control function to be performed. The <i>arg</i>
 * argument represents additional information that is needed to perform the requested function.
 * The type of <i>arg</i> depends upon the particular control request.
 * <p>
 * Valid values for <i>request</i> include:
 * <ul>
 * <li> RMF_OSAL_SOCKET_FIONBIO - The nonblocking flag for the socket is cleared or turned on, depending
 *      on whether the third argument points to a zero or nonzero integer value, respectively.
 * <li> RMF_OSAL_SOCKET_FIONREAD - Return in the integer pointed to by the third argument, the number of
 *      bytes currently in the socket receive buffer.
 * </ul>
 *
 * @param[in] socket specifies the file descriptor associated with the socket.
 * @param[in] request the control function to perform
 * @return Upon successful completion, this function shall return 0; otherwise, -1 shall
 *          be returned and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_SOCKET_EINTR - This function was interrupted by a signal that was caught, before any
 *          data was available.
 * @retval  RMF_OSAL_EINVAL - The <i>request</i> or <i>arg</i> argument is not valid for the
 *          specified socket.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @see POSIX function ioctl()
 */
int rmf_osal_socketIoctl(rmf_osal_Socket socket, int request, ...)
{
    int result, *iargp;
    unsigned long longarg;
    va_list ap;
    va_start(ap, request);
   
    // Processing depends on request code
    switch (request)
    {
        // Set or clear non-blocking flag
        case RMF_OSAL_SOCKET_FIONBIO:
            iargp = va_arg(ap, int*);
            longarg = *iargp;
            result = fcntl((int) socket, F_SETFL, (longarg == 0) ? 0 : O_NONBLOCK);
            break;

        // Get number of bytes ready
        case RMF_OSAL_SOCKET_FIONREAD:
            // Linux: FIONREAD == TIOCINQ
            result = ioctl((int) socket, TIOCINQ, (caddr_t)&longarg);	
            iargp = va_arg(ap, int*);
            *iargp = longarg;
            break;

        // Unsupported or unknown request codes
        default:
            result = RMF_OSAL_EINVAL;
    }
            
    va_end(ap);
    return result;
}

/**
 * @brief The rmf_osal_socketListen() function shall mark a connection-mode socket, specified
 * by the <i>socket</i> argument, as accepting connections.
 * <p>
 * The <i>backlog</i> argument provides a hint to the implementation which the implementation
 * shall use to limit the number of outstanding connections in the socket's listen queue.
 * Implementations may impose a limit on <i>backlog</i> and silently reduce the specified value.
 * Normally, a larger <i>backlog</i> argument value shall result in a larger or equal length of
 * the listen queue.
 * <p>
 * The implementation may include incomplete connections in its listen queue. The limits
 * on the number of incomplete connections and completed connections queued may be different.
 * <p>
 * The implementation may have an upper limit on the length of the listen queue-either
 * global or per accepting socket. If <i>backlog</i> exceeds this limit, the length of the listen
 * queue is set to the limit.
 * <p>
 * If this function is called with a <i>backlog</i> argument value that is less than 0, the
 * function behaves as if it had been called with a <i>backlog</i> argument value of 0.
 * <p>
 * A <i>backlog</i> argument of 0 may allow the socket to accept connections, in which case the
 * length of the listen queue may be set to an implementation-defined minimum value.
 *
 * @param[in] socket specifies the file descriptor associated with the socket.
 * @param[in] backlog the requested maximum number of connections to allow in the socket's
 *          listen queue.
 * @return Upon successful completion, this function shall return 0; otherwise, -1 shall
 *          be returned and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * <ul>
 * <li>     RMF_OSAL_SOCKET_EACCES - The calling process does not have the appropriate privileges.
 * <li>     RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * <li>     RMF_OSAL_SOCKET_EDESTADDRREQ - The socket is not bound to a local address, and the protocol
 *          does not support listening on an unbound socket.
 * <li>     RMF_OSAL_EINVAL - The socket is already connected or the socket has been shut down.
 * <li>     RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources are available in the system to complete the
 *          call.
 * <li>     RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * <li>     RMF_OSAL_SOCKET_EOPNOTSUPP - The socket protocol does not support <i>rmf_osal_socketListen()</i>.
 * </ul>
 * @see POSIX function listen()
 */
int rmf_osal_socketListen(rmf_osal_Socket socket, int backlog)
{
    return listen(socket, backlog);
}

/**
 * @brief The rmf_osal_socketAtoN() function interprets the specified character string as an Internet
 * address, placing the address into the structure provided.
 *
 * @param[in] strptr pointer to Internet address in dot notation
 * @param[in] addrptr pointer to structure to hold the address in numeric form
 * @return This function shall return 1 if the string was successfully interpreted; 0 if the
 *          string is invalid.
 */
int rmf_osal_socketAtoN(const char *strptr, rmf_osal_SocketIPv4Addr *addrptr)
{
#define IS_HEX_CHAR(hex_val) (( *hex_val - 'a')>=0 && (*hex_val -'a') <=5)
#define IS_DEC_CHAR(hex_val) ((*hex_val - '0')>=0 && (*hex_val - '0') <=9)

	uint8_t hex_string[9] = { 0, };
	const char s[2] = ".";
	char *token;
	int j = 0;
	unsigned int hex_result = 0;
	char strptr_dup[16] = { 0, };
	unsigned char *hex_val;
	if ( strlen( strptr ) < sizeof( strptr_dup ) )
	{
		memcpy( strptr_dup, strptr, strlen( strptr ) );
	}
	else
	{
		return 0;
	}

	token = strtok( strptr_dup, s );
	while ( token != NULL )
	{
		int result = 0, i;
		int len = strlen( token );
		for ( i = 0; i < len; i++ )
		{
			result = result * 10 + ( token[i] - '0' );
		}

		hex_string[j + 1] = "0123456789abcdef"[result % 16];
		result = result / 16;
		if ( result > 0 )
		{
			hex_string[j] = "0123456789abcdef"[result % 16];
		}
		else
		{
			hex_string[j] = '0';
		}
		j += 2;

		token = strtok( NULL, s );
	}


	hex_val = hex_string;

	while ( *hex_val )
	{
		hex_result = hex_result << 4;
		if ( IS_DEC_CHAR( hex_val ) )
		{
			hex_result |= *hex_val - '0';
			++hex_val;
			continue;
		}

		if ( IS_HEX_CHAR( hex_val ) )
		{
			hex_result |= ( *hex_val - 'a' + 10 );
			++hex_val;
		}
	}
	if ( addrptr )
		addrptr->s_addr = hex_result;

	return 1;
}


/**
 * @brief The rmf_osal_socketNtoA() function converts the Internet address stored in the <i>inaddr</i>
 * argument into an ASCII string representing the address in dot notation (for example, 127.0.0.1).
 *
 * @param[in] inaddr Internet host address
 * @return This function shall return a pointer to the network address in Internet standard
 *          dot notation.
 * @see POSIX function inet_ntoa()
 */
char *rmf_osal_socketNtoA(rmf_osal_SocketIPv4Addr inaddr)
{
    return inet_ntoa(inaddr);
}

/**
 * @brief The rmf_osal_socketRecv() function shall receive a message from a connection-mode or
 * connectionless-mode socket. It is normally used with connected sockets because it does
 * not permit the caller to retrieve the source address of received data.
 * <p>
 * This function shall return the length of the message written to the buffer pointed to
 * by the <i>buffer</i> argument. For message-based sockets, such as RMF_OSAL_SOCKET_DATAGRAM, the entire
 * message shall be read in a single operation. If a message is too long to fit in the
 * supplied buffer, and RMF_OSAL_SOCKET_MSG_PEEK is not set in the <i>flags</i> argument, the excess bytes
 * shall be discarded. For stream-based sockets, such as RMF_OSAL_SOCKET_STREAM, message
 * boundaries shall be ignored. In this case, data shall be returned to the user as
 * soon as it becomes available, and no data shall be discarded.
 * <p>
 * If no messages are available at the socket, this function shall block until a message
 * arrives.
 *
 * @param[in] socket specifies the socket file descriptor
 * @param[in] buffer points to a buffer where the message should be stored
 * @param[in] length specifies the length in bytes of the buffer pointed to by the <i>buffer</i>
 *          argument
 * @param[in] flags specifies the type of message reception. Values of this argument are formed
 *          by logically OR'ing zero or more of the following values:
 *          <ul>
 *          <li> RMF_OSAL_SOCKET_MSG_PEEK - Peeks at an incoming message. The data is treated as unread and
 *               the next rmf_osal_socketRecv() or similar function shall still return this
 *               data.
 *          <li> RMF_OSAL_SOCKET_MSG_OOB - Requests out-of-band data. The significance and semantics of
 *               out-of-band data are protocol-specific.
 *          </ul>
 * @return Upon successful completion, this function shall return the length of the message in
 *          bytes. If no messages are available to be received and the peer has performed an
 *          orderly shutdown, this function shall return 0. Otherwise, -1 shall be returned
 *          and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EAGAIN or RMF_OSAL_SOCKET_EWOULDBLOCK - The receive operation timed out and no data is waiting
 *          to be received; or RMF_OSAL_SOCKET_MSG_OOB is set and no out-of-band data is available and the
 *          socket does not support blocking to await out-of-band data.
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_SOCKET_ECONNRESET - A connection was forcibly closed by a peer.
 * @retval  RMF_OSAL_SOCKET_EINTR - This function was interrupted by a signal that was caught, before any
 *          data was available.
 * @retval  RMF_OSAL_EINVAL - The RMF_OSAL_SOCKET_MSG_OOB flag is set and no out-of-band data is available.
 * @retval  RMF_OSAL_SOCKET_EIO - An I/O error occurred while reading from or writing to the file system.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available in the system to perform the
 *          operation.
 * @retval  RMF_OSAL_ENOMEM - Insufficient memory was available to fulfill the request.
 * @retval  RMF_OSAL_SOCKET_ENOTCONN - A receive is attempted on a connection-mode socket that is not
 *          connected.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval  RMF_OSAL_SOCKET_EOPNOTSUPP - The specified <i>flags</i> are not supported for this socket
 *          type or protocol.
 * @retval  RMF_OSAL_ETHREADDEATH - The thread executing this function has been marked for death.
 * @retval  RMF_OSAL_SOCKET_ETIMEDOUT - The connection timed out during connection establishment, or
 *          due to a transmission timeout on active connection.
 * @see POSIX function recv()
 */
size_t rmf_osal_socketRecv(rmf_osal_Socket socket, void *buffer, size_t length,
        int flags)
{
    return recv(socket, buffer, length, flags);
}

/**
 * @brief The rmf_osal_socketRecvFrom() function shall receive a message from a connection-mode or
 * connectionless-mode socket. 
 *
 * It is normally used with connectionless-mode sockets because it permits the caller to retrieve
 * the source address of received data.
 * <p>
 * This function shall return the length of the message written to the buffer pointed to by
 * the <i>buffer</i> argument. For message-based sockets, such as RMF_OSAL_SOCKET_DATAGRAM, the entire message
 * shall be read in a single operation. If a message is too long to fit in the supplied buffer,
 * and RMF_OSAL_SOCKET_MSG_PEEK is not set in the <i>flags</i> argument, the excess bytes shall be discarded.
 * For stream-based sockets, such as RMF_OSAL_SOCKET_STREAM, message boundaries shall be ignored.
 * In this case, data shall be returned to the user as soon as it becomes available, and no
 * data shall be discarded.
 * <p>
 * Not all protocols provide the source address for messages. If the <i>address</i> argument is not
 * a null pointer and the protocol provides the source address of messages, the source
 * address of the received message shall be stored in the rmf_osal_SocketSockAddr structure pointed
 * to by the <i>address</i> argument, and the length of this address shall be stored in the object
 * pointed to by the <i>address_len</i> argument.
 * <p>
 * If the actual length of the address is greater than the length of the supplied rmf_osal_SocketSockAddr
 * structure, the stored address shall be truncated.
 * <p>
 * If the <i>address</i> argument is not a null pointer and the protocol does not provide the source
 * address of messages, the value stored in the object pointed to by <i>address</i> is unspecified.
 * <p>
 * If no messages are available at the socket this function shall block until a message
 * arrives.
 *
 * @param[in] socket specifies the socket file descriptor
 * @param[in] buffer points to the buffer where the message should be stored
 * @param[in] length specifies the length in bytes of the buffer pointed to by the <i>buffer</i>
 *          argument
 * @param[in] flags specifies the type of message reception. Values of this argument are formed
 *          by logically OR'ing zero or more of the following values:
 *          <ul>
 *          <li> RMF_OSAL_SOCKET_MSG_PEEK - Peeks at an incoming message. The data is treated as unread
 *               and the next rmf_osal_socketRecvFrom() or similar function shall still return
 *               this data.
 *          <li> RMF_OSAL_SOCKET_MSG_OOB - Requests out-of-band data. The significance and semantics of
 *               out-of-band data are protocol-specific.
 * @param[in] address a null pointer, or points to a rmf_osal_SocketSockAddr structure in which the
 *          sending address is to be stored. The length and format of the address depend
 *          on the address family of the socket.
 * @param[in] address_len specifies the length of the rmf_osal_SocketSockAddr structure pointed to by
 *          the <i>address</i> argument.
 * @return Upon successful completion, this function shall return the length of the message
 *          in bytes. If no messages are available to be received and the peer has performed
 *          an orderly shutdown, this function shall return 0. Otherwise, the function shall
 *          return -1 and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EAGAIN or RMF_OSAL_SOCKET_EWOULDBLOCK - The receive operation timed out and no data is waiting
 *          to be received; or RMF_OSAL_SOCKET_MSG_OOB is set and no out-of-band data is available and the
 *          socket does not support blocking to await out-of-band data.
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_SOCKET_ECONNRESET - A connection was forcibly closed by a peer.
 * @retval  RMF_OSAL_SOCKET_EINTR - This function was interrupted by a signal that was caught, before any
 *          data was available.
 * @retval  RMF_OSAL_EINVAL - The RMF_OSAL_SOCKET_MSG_OOB flag is set and no out-of-band data is available.
 * @retval  RMF_OSAL_SOCKET_EIO - An I/O error occurred while reading from or writing to the file system.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available in the system to perform the
 *          operation.
 * @retval  RMF_OSAL_ENOMEM - Insufficient memory was available to fulfill the request.
 * @retval  RMF_OSAL_SOCKET_ENOTCONN - A receive is attempted on a connection-mode socket that is not
 *          connected.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval  RMF_OSAL_SOCKET_EOPNOTSUPP - The specified <i>flags</i> are not supported for this socket
 *          type.
 * @retval  RMF_OSAL_ETHREADDEATH - The thread executing this function has been marked for death.
 * @retval  RMF_OSAL_SOCKET_ETIMEDOUT - The connection timed out during connection establishment, or
 *          due to a transmission timeout on active connection.
 * @see POSIX function recvfrom()
 */
size_t rmf_osal_socketRecvFrom(rmf_osal_Socket socket, void *buffer, size_t length,
        int flags, rmf_osal_SocketSockAddr *address, rmf_osal_SocketSockLen *address_len)
{
    return recvfrom(socket, buffer, length, flags, address, address_len);
}

/**
 * @brief The rmf_osal_socketSelect() function shall
 * examine the file descriptor sets whose addresses are passed in the <i>readfds</i>,
 * <i>writefds</i>, and <i>errorfds</i> parameters to see whether some of their descriptors
 * are ready for reading, are ready for writing, or have an exceptional condition pending,
 * respectively.
 * The behavior of this function on non-socket file descriptors is unspecified.
 * <p>
 * Upon successful completion, this function shall modify the objects pointed to by the
 * <i>readfds</i>, <i>writefds</i>, and <i>errorfds</i> arguments to indicate which file
 * descriptors are ready for reading, ready for writing, or have an error condition pending,
 * respectively, and shall return the total number of ready descriptors in all the output sets.
 * For each file descriptor less than <i>numfds</i>, the corresponding bit shall be set on
 * successful completion if it was set on input and the associated condition is true for
 * that file descriptor.
 * <p>
 * If none of the selected descriptors are ready for the requested operation, this function
 * shall block until at least one of the requested operations becomes ready, until the
 * timeout occurs, or until interrupted by a signal. The <i>timeout</i> parameter controls how
 * long this function shall take before timing out. If the <i>timeout</i> parameter is not a null
 * pointer, it specifies a maximum interval to wait for the selection to complete. If the
 * specified time interval expires without any requested operation becoming ready, the
 * function shall return. If the <i>timeout</i> parameter is a null pointer, then the call to this
 * function shall block indefinitely until at least one descriptor meets the specified
 * criteria. To effect a poll, the <i>timeout</i> parameter should not be a null pointer, and
 * should point to a zero-valued rmf_osal_TimeVal structure.
 * <p>
 * Implementations may place limitations on the maximum timeout interval supported. If
 * the <i>timeout</i> argument specifies a timeout interval greater than the implementation-defined
 * maximum value, the maximum value shall be used as the actual timeout value.
 * Implementations may also place limitations on the granularity of timeout intervals. If
 * the requested timeout interval requires a finer granularity than the implementation
 * supports, the actual timeout interval shall be rounded up to the next supported value.
 * <p>
 * If the <i>readfds</i>, <i>writefds</i>, and <i>errorfds</i> arguments are all null pointers
 * and the <i>timeout</i> argument is not a null pointer, this function shall block for the
 * time specified, or until interrupted by a signal. If the <i>readfds</i>, <i>writefds</i>,
 * and <i>errorfds</i> arguments are all null pointers and the <i>timeout</i> argument is a
 * null pointer, this function shall block until interrupted by a signal.
 * <p>
 * On failure, the objects pointed to by the <i>readfds</i>, <i>writefds</i>, and <i>errorfds</i>
 * arguments shall not be modified. If the timeout interval expires without the specified
 * condition being true for any of the specified file descriptors, the objects pointed to
 * by the <i>readfds</i>, <i>writefds</i>, and <i>errorfds</i> arguments shall have all bits
 * set to 0.
 *
 * @param[in] numfds the range of descriptors to be tested. The first <i>numfds</i> descriptors
 *          shall be checked in each set; that is, the descriptors from zero through
 *          <i>numfds</i>-1 in the descriptor sets shall be examined.
 * @param[in] readfds if the <i>readfds</i> argument is not a null pointer, it points to an object
 *          of type rmf_osal_SocketFDSet that on input specifies the file descriptors to be
 *          checked for being ready to read, and on output indicates which file descriptors
 *          are ready to read.
 * @param[in] writefds if the <i>writefds</i> argument is not a null pointer, it points to an object
 *          of type rmf_osal_SocketFDSet that on input specifies the file descriptors to be
 *          checked for being ready to write, and on output indicates which file descriptors
 *          are ready to write.
 * @param[in] errorfds if the <i>errorfds</i> argument is not a null pointer, it points to an object
 *          of type rmf_osal_SocketFDSet that on input specifies the file descriptors to be
 *          checked for error conditions pending, and on output indicates which file descriptors
 *          have error conditions pending.
 * @param[in] timeout the timeout period in seconds and microseconds.
 * @return Upon successful completion, this function shall return the total number of bits
 *          set in the bit masks. Otherwise, -1 shall be returned, and one of the following
 *          error codes can be retrieved with rmf_osal_socketGetLastError():
 * <ul>
 * <li>     RMF_OSAL_SOCKET_EBADF - One or more of the file descriptor sets specified a file descriptor
 *          that is not a valid open file descriptor.
 * <li>     RMF_OSAL_SOCKET_EINTR - The function was interrupted before any of the selected events
 *          occurred and before the timeout interval expired.
 * <li>     RMF_OSAL_EINVAL - An invalid timeout interval was specified; or the <i>nfds</i>
 *          argument is less than 0 or greater than RMF_OSAL_SOCKET_FD_SETSIZE.
 * <li>     RMF_OSAL_ETHREADDEATH - The thread executing this function has been marked for death.
 * </ul>
 * @see POSIX function select()
 */

int rmf_osal_socketSelect(int numfds, rmf_osal_SocketFDSet *readfds,
        rmf_osal_SocketFDSet *writefds, rmf_osal_SocketFDSet *errorfds,
        const rmf_osal_TimeVal *timeout)
{
    return select(numfds, readfds, writefds, errorfds, (rmf_osal_TimeVal *)timeout);
}

/**
 * @brief The rmf_osal_socketSend() function shall initiate transmission of a message from the
 * specified socket to its peer. This function shall send a message only when the socket
 * is connected (including when the peer of a connectionless socket has been set via
 * rmf_osal_socketConnect()).
 * <p>
 * The length of the message to be sent is specified by the <i>length</i> argument. If the
 * message is too long to pass through the underlying protocol, this function shall fail
 * and no data shall be transmitted.
 * <p>
 * Successful completion of a call to this function does not guarantee delivery of the
 * message. A return value of -1 indicates only locally-detected errors.
 * <p>
 * If space is not available at the sending socket to hold the message to be transmitted,
 * this function shall block until space is available. The rmf_osal_socketSelect() function
 * can be used to determine when it is possible to send more data.
 *
 * @param[in] socket specifies the socket file descriptor
 * @param[in] buffer points to the buffer containing the message to send
 * @param[in] length specifies the length of the message in bytes
 * @param[in] flags specifies the type of message transmission. Values of this argument are
 *          formed by logically OR'ing zero or more of the following flags:
 *          <ul>
 *          <li> RMF_OSAL_SOCKET_MSG_OOB - Sends out-of-band data on sockets that support
 *               out-of-band communications. The significance and semantics of
 *               out-of-band data are protocol-specific.
 *          </ul>
 * @return Upon successful completion, this function shall return the number of bytes
 *          sent. Otherwise, -1 shall be returned and one of the following error codes
 *          can be retrieved with rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EACCES - The calling process does not have the appropriate privileges.
 * @retval  RMF_OSAL_SOCKET_EAGAIN or RMF_OSAL_SOCKET_EWOULDBLOCK - The send operation timed out and no data was
 *          sent.
 * @retval     RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_SOCKET_ECONNRESET - A connection was forcibly closed by a peer.
 * @retval   RMF_OSAL_SOCKET_EDESTADDRREQ - The socket is not connection-mode and no peer address is set.
 * @retval  RMF_OSAL_SOCKET_EINTR - A signal interrupted this function before any data was transmitted.
 * @retval  RMF_OSAL_SOCKET_EIO - An I/O error occurred while reading from or writing to the file system.
 * @retval  RMF_OSAL_SOCKET_EMSGSIZE - The message is too large to be sent all at once, as the socket
 *          requires.
 * @retval  RMF_OSAL_SOCKET_ENETDOWN - The local network interface used to reach the destination is down.
 * @retval  RMF_OSAL_SOCKET_ENETUNREACH - No route to the network is present.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available in the system to perform the
 *          operation.
 * @retval  RMF_OSAL_SOCKET_ENOTCONN - The socket is not connected or otherwise has not had the peer
 *          pre-specified.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval  RMF_OSAL_SOCKET_EOPNOTSUPP - The <i>socket</i> argument is associated with a socket that does
 *          not support one or more of the values set in <i>flags</i>.
 * @retval  RMF_OSAL_SOCKET_EPIPE - The socket is shut down for writing, or the socket is connection-mode
 *          and is no longer connected. In the latter case, and if the socket is of type
 *          RMF_OSAL_SOCKET_STREAM, the RMF_OSAL_SIGPIPE signal is generated to the calling thread.
 * @retval  RMF_OSAL_ETHREADDEATH - The thread executing this function has been marked for death.
 * @see POSIX function send()
 */

size_t rmf_osal_socketSend(rmf_osal_Socket socket, const void *buffer, size_t length,
        int flags)
{
    /*
     * MSG_NOSIGNAL is OR'd into flags to insure that a client dropping a connection
     * does not raise a signal which is currently not handled and would cause the
     * ri to exit.
     */
    return send(socket, buffer, length, flags | MSG_NOSIGNAL);
}

/**
 * @brief The rmf_osal_socketSendTo() function shall send a message through a connection-mode or
 * connectionless-mode socket. If the socket is connectionless-mode, the message shall be
 * sent to the address specified by <i>dest_addr</i>. If the socket is connection-mode,
 * <i>dest_addr</i> shall be ignored.
 * <p>
 * If the socket protocol supports broadcast and the specified address is a broadcast
 * address for the socket protocol, this function shall fail if the RMF_OSAL_SOCKET_SO_BROADCAST
 * option is not set for the socket.
 * <p>
 * The <i>dest_addr</i> argument specifies the address of the target. The <i>length</i> argument
 * specifies the length of the message.
 * <p>
 * Successful completion of a call to this function does not guarantee delivery of the
 * message. A return value of -1 indicates only locally-detected errors.
 * <p>
 * If space is not available at the sending socket to hold the message to be transmitted,
 * this function shall block until space is available.
 *
 * @param[in] socket specifies the socket file descriptor
 * @param[in] message points to a buffer containing the message to be sent
 * @param[in] length specifies the size of the message in bytes
 * @param[in] flags specifies the type of message transmission. Values of this argument are
 *          formed by logically OR'ing zero or more of the following flags:
 *          <ul>
 *          <li> RMF_OSAL_SOCKET_MSG_OOB - Sends out-of-band data on sockets that support
 *               out-of-band data. The significance and semantics of out-of-band data
 *               are protocol-specific.
 *          </ul>
 * @param[in] dest_addr points to a rmf_osal_SocketSockAddr structure containing the destination
 *          address. The length and format of the address depend on the address
 *          family of the socket.
 * @param[in] dest_len specifies the length of the rmf_osal_SocketSockAddr structure pointed to by
 *          the <i>dest_addr</i> argument.
 * @return Upon successful completion, this function shall return the number of bytes sent.
 *          Otherwise, -1 shall be returned and one of the following error codes can be
 *          retrieved with rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EACCES - Search permission is denied for a component of the path prefix;
 *          or write access to the named socket is denied.
 * @retval  RMF_OSAL_SOCKET_EAFNOSUPPORT - Addresses in the specified address family cannot be used
 *          with this socket.
 * @retval  RMF_OSAL_SOCKET_EAGAIN or RMF_OSAL_SOCKET_EWOULDBLOCK - The send operation timed out and no data was
 *          sent.
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_SOCKET_ECONNRESET - A connection was forcibly closed by a peer.
 * @retval  RMF_OSAL_SOCKET_EDESTADDRREQ - The socket is not connection-mode and does not have its peer
 *          address set, and no destination address was specified.
 * @retval  RMF_OSAL_SOCKET_EHOSTUNREACH - The destination host cannot be reached (probably because the
 *          host is down or a remote router cannot reach it).
 * @retval  RMF_OSAL_SOCKET_EINTR - A signal interrupted this function before any data was transmitted.
 * @retval  RMF_OSAL_EINVAL - The <i>dest_len</i> argument is not a valid length for the address
 *          family.
 * @retval  RMF_OSAL_SOCKET_EIO - An I/O error occurred while reading from or writing to the file system.
 * @retval  RMF_OSAL_SOCKET_EISCONN - A destination address was specified and the socket is already
 *          connected. This error may or may not be returned for connection mode sockets.
 * @retval  RMF_OSAL_SOCKET_EMSGSIZE - The message is too large to be sent all at once, as the socket
 *          requires.
 * @retval  RMF_OSAL_SOCKET_ENETDOWN - The local network interface used to reach the destination is down.
 * @retval  RMF_OSAL_SOCKET_ENETUNREACH - No route to the network is present.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available in the system to perform the
 *          operation.
 * @retval  RMF_OSAL_ENOMEM - Insufficient memory was available to fulfill the request.
 * @retval  RMF_OSAL_SOCKET_ENOTCONN - The socket is connection-mode but is not connected.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @retval  RMF_OSAL_SOCKET_EOPNOTSUPP - The <i>socket</i> argument is associated with a socket that
 *          does not support one or more of the values set in <i>flags</i>.
 * @retval  RMF_OSAL_SOCKET_EPIPE - The socket is shut down for writing, or the socket is connection-mode
 *          and is no longer connected. In the latter case, and if the socket is of type
 *          RMF_OSAL_SOCKET_STREAM, the RMF_OSAL_SIGPIPE signal is generated to the calling thread.
 * @retval  RMF_OSAL_ETHREADDEATH - The thread executing this function has been marked for death.
 * @see POSIX function sendto()
 */

size_t rmf_osal_socketSendTo(rmf_osal_Socket socket, const void *message,
        size_t length, int flags, const rmf_osal_SocketSockAddr *dest_addr,
        rmf_osal_SocketSockLen dest_len)
{
   /*
     * MSG_NOSIGNAL is OR'd into flags to insure that a client dropping a connection
     * does not raise a signal which is currently not handled and would cause the
     * ri to exit.
     */
    return sendto(socket, message, length, flags | MSG_NOSIGNAL, dest_addr, dest_len);
}

/**
 * @brief The rmf_osal_socketSetOpt() function shall set the option specified by the <i>option_name</i>
 * argument, at the protocol level specified by the <i>level</i> argument, to the value pointed
 * to by the <i>option_value</i> argument for the socket associated with the file descriptor
 * specified by the <i>socket</i> argument.
 * <p>
 * The <i>level</i> argument specifies the protocol level at which the option resides. To set
 * options at the socket level, specify the <i>level</i> argument as RMF_OSAL_SOCKET_SOL_SOCKET. To set
 * options at other levels, supply the appropriate level identifier for the protocol
 * controlling the option. For example, to indicate that an option is interpreted by the
 * TCP (Transport Control Protocol), set <i>level</i> to RMF_OSAL_SOCKET_IPPROTO_TCP.
 * <p>
 * The <i>option_name</i> argument specifies a single option to set. The <i>option_name</i> argument and
 * any specified options are passed uninterpreted to the appropriate protocol module for
 * interpretations.
 * <p>
 * Valid values for <i>option_name</i> include:
 * <ul>
 * <li> RMF_OSAL_SOCKET_SO_BROADCAST - permit sending of broadcast datagrams (int)
 * <li> RMF_OSAL_SOCKET_SO_DEBUG - enable debug tracing (int)
 * <li> RMF_OSAL_SOCKET_SO_DONTROUTE - bypass routing table lookup (int)
 * <li> RMF_OSAL_SOCKET_SO_KEEPALIVE - periodically test if connection still alive (int)
 * <li> RMF_OSAL_SOCKET_SO_LINGER - linger on close if data to send (rmf_osal_SocketLinger)
 * <li> RMF_OSAL_SOCKET_SO_OOBINLINE - leave received out-of-band data inline (int)
 * <li> RMF_OSAL_SOCKET_SO_RCVBUF - receive buffer size (int)
 * <li> RMF_OSAL_SOCKET_SO_SNDBUF - send buffer size (int)
 * <li> RMF_OSAL_SOCKET_SO_RCVLOWAT - receive buffer low-water mark (int)
 * <li> RMF_OSAL_SOCKET_SO_SNDLOWAT - send buffer low-water mark (int)
 * <li> RMF_OSAL_SOCKET_SO_RCVTIMEO - receive timeout (rmf_osal_TimeVal)
 * <li> RMF_OSAL_SOCKET_SO_SNDTIMEO - send timeout (rmf_osal_TimeVal)
 * <li> RMF_OSAL_SOCKET_SO_REUSEADDR - allow local address reuse (int)
 * <li> RMF_OSAL_SOCKET_IPV4_ADD_MEMBERSHIP - join a multicast group (rmf_osal_SocketIPv4McastReq)
 * <li> RMF_OSAL_SOCKET_IPV4_DROP_MEMBERSHIP - leave a multicast group (rmf_osal_SocketIPv4McastReq)
 * <li> RMF_OSAL_SOCKET_IPV4_MULTICAST_IF - specify outgoing interface (rmf_osal_SocketIPv4Addr)
 * <li> RMF_OSAL_SOCKET_IPV4_MULTICAST_LOOP - specify loopback (unsigned char)
 * <li> RMF_OSAL_SOCKET_IPV4_MULTICAST_TTL - specify outgoing TTL (unsigned char)
 * <li> RMF_OSAL_SOCKET_IPV6_ADD_MEMBERSHIP - join a multicast group (rmf_osal_SocketIPv6McastReq)
 * <li> RMF_OSAL_SOCKET_IPV6_DROP_MEMBERSHIP - leave a multicast group (rmf_osal_SocketIPv4McastReq)
 * <li> RMF_OSAL_SOCKET_IPV6_MULTICAST_IF - specify outgoing interface (rmf_osal_SocketIPv6Addr)
 * <li> RMF_OSAL_SOCKET_IPV6_MULTICAST_HOPS - specify outgoing hop limit (unsigned int)
 * <li> RMF_OSAL_SOCKET_IPV6_MULTICAST_LOOP - specify loopback (unsigned int)
 * <li> RMF_OSAL_SOCKET_TCP_NODELAY - disable Nagle algorithm (int)
 * </ul>
 *
 * @param[in] socket specifies the file descriptor associated with the socket.
 * @param[in] level is the protocol level at which the option resides
 * @param[in] option_name specifies a single option to be set.
 * @param[in] option_value points to the new value for the option
 * @param[in] option_len specifies the length of option pointed to by <i>option_value</i>.
 * @return Upon successful completion, this function shall return 0; otherwise, -1 shall
 *          be returned and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_SOCKET_EDOM - The send and receive timeout values are too big to fit into the
 *          timeout fields in the socket structure.
 * @retval  RMF_OSAL_EINVAL - The specified option is invalid at the specified socket level or
 *          the socket has been shut down.
 * @retval  RMF_OSAL_SOCKET_EISCONN - The socket is already connected, and a specified option cannot
 *          be set while the socket is connected.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources are available in the system to complete
 *          the call.
 * @retval  RMF_OSAL_ENOMEM - There was insufficient memory available for the operation to
 *          complete.
 * @retval  RMF_OSAL_SOCKET_ENOPROTOOPT - The option is not supported by the protocol.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @see POSIX function setsockopt()
 */

int rmf_osal_socketSetOpt(rmf_osal_Socket socket, int level, int option_name,
    const void *option_value, rmf_osal_SocketSockLen option_len)
{
    return setsockopt(socket, level, option_name, (char *)option_value, 
        option_len);
}

/**
 * @brief The rmf_osal_socketShutdown() function shall cause all or part of a full-duplex connection
 * on the socket associated with the file descriptor <i>socket</i> to be shut down.
 * This function disables subsequent send and/or receive operations on a socket, depending
 * on the value of the <i>how</i> argument.
 *
 * @param[in] socket specifies the file descriptor of the socket
 * @param[in] how specifies the type of shutdown. The values are as follows:
 *          <ul>
 *          <li> RMF_OSAL_SOCKET_SHUT_RD - Disables further receive operations.
 *          <li> RMF_OSAL_SOCKET_SHUT_WR - Disables further send operations.
 *          <li> RMF_OSAL_SOCKET_SHUT_RDWR - Disables further send and receive operations.
 * @return Upon successful completion, this function shall return 0; otherwise, -1 shall
 *          be returned and one of the following error codes can be retrieved with
 *          rmf_osal_socketGetLastError():
 * @retval  RMF_OSAL_SOCKET_EBADF - The <i>socket</i> argument is not a valid file descriptor.
 * @retval  RMF_OSAL_EINVAL - The <i>how</i> argument is invalid.
 * @retval  RMF_OSAL_SOCKET_ENOBUFS - Insufficient resources were available in the system to perform
 *          the operation.
 * @retval  RMF_OSAL_SOCKET_ENOTCONN - The socket is not connected.
 * @retval  RMF_OSAL_SOCKET_ENOTSOCK - The <i>socket</i> argument does not refer to a socket.
 * @see POSIX function shutdown()
 */
int rmf_osal_socketShutdown(rmf_osal_Socket socket, int how)
{
    return shutdown(socket, how);
}

/**
 * @brief The rmf_osal_socketGetInterfaces() function acquires a pointer to a linked
 * list structure of the network interfaces and their associated addresses.
 * The addresses are also represented by a linked list structure.
 *
 * The <i>rmf_osal_SocketNetAddr</i> structure represents a
 * single network interface address in the linked list of addresses for an interface.
 *
 * The <i>rmf_osal_SocketNetIfList</i> structure returned by this function represents a
 * single network interface with its list of associated addresses.
 *
 * The pointer returned is to an internal buffer that should not be modified by the caller.
 *
 * @param[out] netIfList is a pointer for returning the address of the interface buffer.
 * @retval RMF_SUCCESS - if the operation succeeded.
 * @retval RMF_OSAL_EINVAL - invalid pointer argument.
 * @retval RMF_OSAL_ENODATA - if an error occurred attempting to acquire the list.
 * @retval RMF_OSAL_ENOMEM - if a memory allocation error occurred.
 */
rmf_Error rmf_osal_socketGetInterfaces(rmf_osal_SocketNetIfList **netIfList)
{
    extern rmf_osal_SocketNetIfList *getInterfaces(rmf_osal_SocketNetIfList *, int);

    /* Get the IPv4 interfaces only. */
    *netIfList = getInterfaces( NULL, AF_INET );

    /* Get the IPv6 interfaces only. */
    //*netIfList = getInterfaces( NULL, AF_INET6 );

    /* Get all interfaces. */
    //*netIfList = getInterfaces( NULL, AF_UNSPEC );

    /* Return interface list or error. */
    if (*netIfList != NULL)
    {
        return RMF_SUCCESS;
    }
    else
    {
        return RMF_OSAL_ENOMEM;
    }
}

/**
 * @brief This function is used to free the rmf_osal_socketNetIfList structure 
 * previously allocated via rmf_osal_socketGetInterfaces() call.
 *
 * @param[in] netIfList is a pointer to the structure previously allocated
 *                  via rmf_osal_socketGetInterfaces() call.
 * @return None
 */
void rmf_osal_socketFreeInterfaces(rmf_osal_SocketNetIfList *netIfList)
{
    rmf_osal_SocketNetIfList *curIf = netIfList;

    while (curIf != NULL)
    {
        rmf_osal_SocketNetIfList *iface = curIf;
        rmf_osal_SocketNetAddr *curAddr = curIf->if_addresses;
        while (curAddr != NULL)
        {
            rmf_osal_SocketNetAddr *addr = curAddr;
            curAddr = curAddr->if_next;
            free(addr->if_addr);
            addr->if_addr = NULL;
            free(addr);
            addr = NULL;
        }
        curIf = curIf->if_next;
        free(iface);
        iface = NULL;
    }
}

/*
 * Acquire the list of interfaces for the specified family and add each
 * interface and its addresses to the list of interfaces.
 */
rmf_osal_SocketNetIfList *getInterfaces(rmf_osal_SocketNetIfList *ifs, int family)
{
    extern rmf_osal_SocketNetIfList *addNetIf(rmf_osal_SocketNetIfList *, char *, int,
            int, rmf_osal_SocketSockAddr *, int);

    char buf[1024] =
    { 0 };
    struct ifconf ifc =
    { 0 };
    struct ifreq *ifr = NULL;
    int sock = 0;
    int nInterfaces = 0;
    int i = 0;

    /* Get a socket handle. */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return NULL;
    }

    /* Query available interfaces. */
    // GORP: what if buffer is too small??
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0)
    {
        close(sock);
        return NULL;
    }

    /* Iterate through the list of interfaces. */
    ifr = ifc.ifc_req;
    nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
    for (i = 0; i < nInterfaces; i++)
    {
        struct ifreq *item = &ifr[i];

        struct sockaddr *addr = &(item->ifr_addr);
        if ( NULL != item->ifr_name)
        {
            ifs = addNetIf(ifs, item->ifr_name, item->ifr_ifindex, family, addr,
                    sizeof(struct sockaddr));
        }
    }

    close(sock);
    return ifs;
}

/*
 * Add an interface to the list. If known interface just link
 * a new address onto the list. If new interface create new
 * rmf_osal_SocketNetIf structure.
 */
rmf_osal_SocketNetIfList *addNetIf(rmf_osal_SocketNetIfList *ifs, char *if_name,
        int if_index, int family, rmf_osal_SocketSockAddr *addr, int addrlen)
{
    rmf_osal_SocketNetIfList *currif = ifs;
    rmf_osal_SocketNetAddr *addrP;
    char physname[RMF_OSAL_SOCKET_IFNAMSIZ];
    int logicalOffset;
    char *unit;

    /*
     * If the interface name is longer than we can support,
     * return an un-updated list.
     */
    if (strlen(if_name) >= RMF_OSAL_SOCKET_IFNAMSIZ)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.NET",
                "addNetIf: Interface name '%s' is too long\n", if_name);
        return ifs;
    }

    strncpy(physname, if_name, RMF_OSAL_SOCKET_IFNAMSIZ);
    logicalOffset = (int)(strchr(physname, ':') - physname);
    if (logicalOffset > 0) 
    {
      physname[logicalOffset] = '\0';
    }

    if ((addrP = (rmf_osal_SocketNetAddr *) malloc(sizeof(rmf_osal_SocketNetAddr)))
            != NULL)
    {
        if ((addrP->if_addr = (rmf_osal_SocketSockAddr *) malloc(addrlen)) == NULL)
        {
            free(addrP);
            return ifs;
        }
    }
    else
    {
        return ifs;
    }
    memcpy(addrP->if_addr, addr, addrlen);
    addrP->if_family = family;

    /*
     * Make sure this is a new interface.
     */
    while (currif != NULL)
    {
        /* Compare interface names. */
        if (strcmp(physname, currif->if_name) == 0)
            break;
        currif = currif->if_next;
    }

    /*
     * If not found, create a new one.
     */
    if (currif == NULL)
    {
        /* Allocate a new interface structure. */
        if ((currif = (rmf_osal_SocketNetIfList *) malloc(
                sizeof(rmf_osal_SocketNetIfList))) != NULL)
        {
            /* Copy the interface name. */
            strcpy(currif->if_name, physname);
        }
        else
        {
            /* Free address structure on interface allocation error. */
            free(addrP->if_addr);
            free(addrP);
            return ifs;
        }
        /* Link new interface into the list. */
        currif->if_index = if_index;
        currif->if_addresses = NULL;
        currif->if_next = ifs;
        ifs = currif;
    }

    /*
     * Insert the address structure onto the interface list.
     */
    addrP->if_next = currif->if_addresses;
    currif->if_addresses = addrP;

    return ifs;
}
