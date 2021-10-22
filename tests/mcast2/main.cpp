// Please reference to: https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancedmulticast9a.html
#include <stdio.h>
#include "yasio/xxsocket.hpp"

// Function: PrintAddress
// Description:
//    This routine takes a SOCKADDR structure and its length and prints
//    converts it to a string representation. This string is printed
//    to the console via stdout.
int PrintAddress(SOCKADDR* sa, int salen)
{

  char host[NI_MAXHOST], serv[NI_MAXSERV];
  int hostlen = NI_MAXHOST, servlen = NI_MAXSERV, rc;
  rc = getnameinfo(sa, salen, host, hostlen, serv, servlen, NI_NUMERICHOST | NI_NUMERICSERV);
  if (rc != 0)
  {
    fprintf(stderr, "%s: getnameinfo failed with error code %d\n", __FILE__, rc);
    return rc;
  }

  // If the port is zero then don't print it
  if (strcmp(serv, "0") != 0)
  {
    if (sa->sa_family == AF_INET)
      printf("[%s]:%s", host, serv);
    else
      printf("%s:%s", host, serv);
  }
  else
    printf("%s", host);
  return NO_ERROR;
}

// Function: FormatAddress
// Description:
//    This is similar to the PrintAddress function except that instead of
//    printing the string address to the console, it is formatted into
//    the supplied string buffer.
int FormatAddress(SOCKADDR* sa, int salen, char* addrbuf, int addrbuflen)
{
  char host[NI_MAXHOST], serv[NI_MAXSERV];
  int hostlen = NI_MAXHOST, servlen = NI_MAXSERV, rc;
  rc = getnameinfo(sa, salen, host, hostlen, serv, servlen, NI_NUMERICHOST | NI_NUMERICSERV);
  if (rc != 0)
  {
    fprintf(stderr, "%s: getnameinfo failed: %d\n", __FILE__, rc);
    return rc;
  }

  if ((strlen(host) + strlen(serv) + 1) > (unsigned)addrbuflen)
    return WSAEFAULT;

  if (sa->sa_family == AF_INET)
    sprintf_s(addrbuf, sizeof(addrbuf), "%s:%s", host, serv);
  else if (sa->sa_family == AF_INET6)
    sprintf_s(addrbuf, sizeof(addrbuf), "[%s]:%s", host, serv);
  else
    addrbuf[0] = '\0';
  return NO_ERROR;
}

// Function: ResolveAddress
// Description:
//    This routine resolves the specified address and returns a list of addrinfo
//    structure containing SOCKADDR structures representing the resolved addresses.
//    Note that if 'addr' is non-NULL, then getaddrinfo will resolve it whether
//    it is a string literal address or a hostname.
struct addrinfo* ResolveAddress(const char* addr, const char* port, int af, int type, int proto)
{
  struct addrinfo hints, *res = NULL;

  int rc;
  memset(&hints, 0, sizeof(hints));

  hints.ai_flags    = ((addr) ? 0 : AI_PASSIVE);
  hints.ai_family   = af;
  hints.ai_socktype = type;
  hints.ai_protocol = proto;

  rc = getaddrinfo(addr, port, &hints, &res);

  if (rc != 0)
  {
    printf("Invalid address %s, getaddrinfo failed: %d\n", addr, rc);
    return NULL;
  }

  return res;
}

// Sample: IPv4/6 Multicasting using setsockopt()
// Purpose:
//    This sample illustrates IP multicasting using the Winsock 1
//    method of joining and leaving an multicast group.  This sample
//    may be invoked as either a sender or a receiver. This sample works
//    with both IPv4 and IPv6 multicasting but does not include support
//    for the IPv4 source multicasting.
//
//    One of the advantages of using the setsockopt over WSAJoinLeaf is
//    the ability to join multiple multicast groups on a single socket
//    which is not possible with WSAJoinLeaf.
//
//    Also note that because we include winsock2.h we must link with
//    ws2_32.lib and not with wsock32.lib.
//
// Command Line Options/Parameters
//    mcastws1.exe [-s] [-m str] [-p int] [-i str] [-b str] [-l] [-n int]
//       -b str    Local interface to bind to
//       -c        Connect to multicast address before sending?
//       -i str    Local interface to use for the multicast join
//       -j        Don't join the multicast group (sender only)
//       -l        Disable the loopback
//       -m str    Dotted decimal IP multicast address to join
//       -n int    Number of messages to send or receive
//       -p int    Port number to use
//       -s        Act as sender; otherwise receive data
//       -t int    Multicast TTL
//       -z int    Buffer size (in bytes)
//
#define MCASTADDRV4 "239.0.0.19"
#define MCASTADDRV6 "ff12::1"
#define MCASTPORT "22016"
#define BUFSIZE 4
#define DEFAULT_COUNT 500
#define DEFAULT_TTL 128

BOOL bSender        = FALSE,         // Act as sender?
    bConnect        = FALSE,         // Connect before sending?
    bLoopBack       = FALSE,         // Loopback parameter specified?
    bDontJoin       = FALSE;         // Specifies whether to join the multicast group
int gSocketType     = SOCK_DGRAM,    // datagram
    gProtocol       = IPPROTO_UDP,   // UDP
    gLoopBack       = 0,             // Disable loopback?
    gCount          = DEFAULT_COUNT, // Number of messages to send/receive
    gTtl            = DEFAULT_TTL,   // Multicast TTL value
    gBufferSize     = BUFSIZE;       // Buffer size for send/recv
char *gBindAddr     = NULL,          // Address to bind socket to (default is 0.0.0.0 or ::)
    *gInterface     = NULL;          // Interface to join the multicast group on
const char *gMulticast   = MCASTADDRV4;   // Multicast group to join
const char *gPort        = MCASTPORT;     // Port number to use

// Function: usage

// Description: Print usage information and exit.

int usage(char* progname)
{
  printf("Usage: %s -s -m str -p int -i str -l -n int\n", progname);
  printf(" -b str String address to bind to\n");
  printf(" -c     Connect before sending?\n");
  printf(" -i str Local interface to join groups\n");
  printf(" -j     Don't join the multicast group\n");
  printf(" -l 0/1 Turn on/off loopback\n");
  printf(" -m str Dotted decimal multicast IP address to join\n");
  printf(" -n int Number of messages to send/receive\n");
  printf(" -p int Port number to use. The default port is: %s\n", MCASTPORT);
  printf(" -s     Act as server (send data); otherwise receive data.\n");
  printf(" -t int Set multicast TTL\n");
  printf(" -z int Size of the send/recv buffer\n");
  return 0;
}

// Function: ValidateArgs
// Description
//    Parse the command line arguments and set some global flags
//    depending on the values.
void ValidateArgs(int argc, char** argv)
{
  int i;
  for (i = 1; i < argc; i++)
  {
    if ((argv[i][0] == '-') || (argv[i][0] == '/'))
    {
      switch (tolower(argv[i][1]))
      {
        case 'b': // Address to bind to
          if (i + 1 >= argc)
            usage(argv[0]);
          gBindAddr = argv[++i];
          break;

        case 'c': // Connect socket
          bConnect = TRUE;
          break;

        case 'i': // local interface to use
          if (i + 1 >= argc)
            usage(argv[0]);
          gInterface = argv[++i];
          break;

        case 'j': // Don't join multicast group
          bDontJoin = TRUE;
          break;

        case 'l': // Disable loopback?
          if (i + 1 >= argc)
            usage(argv[0]);

          bLoopBack = TRUE;
          gLoopBack = atoi(argv[++i]);
          break;

        case 'm': // multicast group to join
          if (i + 1 >= argc)
            usage(argv[0]);

          gMulticast = argv[++i];
          break;

        case 'n': // Number of messages to send/recv
          if (i + 1 >= argc)
            usage(argv[0]);

          gCount = atoi(argv[++i]);
          break;

        case 'p': // Port number to use
          if (i + 1 >= argc)
            usage(argv[0]);
          gPort = argv[++i];
          break;

        case 's': // sender
          bSender = TRUE;
          break;

        case 't': // Multicast ttl
          if (i + 1 >= argc)
            usage(argv[0]);

          gTtl = atoi(argv[++i]);
          break;

        case 'z': // Buffer size for send/recv
          if (i + 1 >= argc)
            usage(argv[0]);
          gBufferSize = atol(argv[++i]);
          break;

        default:
          usage(argv[0]);
          break;
      }
    }
  }

  return;
}

// Function: JoinMulticastGroup
// Description:
//    This function joins the multicast socket on the specified multicast
//    group. The structures for IPv4 and IPv6 multicast joins are slightly
//    different which requires different handlers. For IPv6 the scope-ID
//    (interface index) is specified for the local interface whereas for IPv4
//    the actual IPv4 address of the interface is given.
int JoinMulticastGroup(SOCKET s, struct addrinfo* group, struct addrinfo* iface)
{
  struct ip_mreq mreqv4;
  struct ipv6_mreq mreqv6;
  char* optval = NULL;
  int optlevel, option, optlen, rc;
  rc = NO_ERROR;
  if (group->ai_family == AF_INET)
  {
    // Setup the v4 option values and ip_mreq structure
    optlevel                    = IPPROTO_IP;
    option                      = IP_ADD_MEMBERSHIP;
    optval                      = (char*)&mreqv4;
    optlen                      = sizeof(mreqv4);
    mreqv4.imr_multiaddr.s_addr = ((SOCKADDR_IN*)group->ai_addr)->sin_addr.s_addr;
    mreqv4.imr_interface.s_addr = ((SOCKADDR_IN*)iface->ai_addr)->sin_addr.s_addr;
  }
  else if (group->ai_family == AF_INET6)
  {
    // Setup the v6 option values and ipv6_mreq structure
    optlevel                = IPPROTO_IPV6;
    option                  = IPV6_ADD_MEMBERSHIP;
    optval                  = (char*)&mreqv6;
    optlen                  = sizeof(mreqv6);
    mreqv6.ipv6mr_multiaddr = ((SOCKADDR_IN6*)group->ai_addr)->sin6_addr;
    mreqv6.ipv6mr_interface = ((SOCKADDR_IN6*)iface->ai_addr)->sin6_scope_id;
  }
  else
  {
    fprintf(stderr, "Attempting to join multicast group for invalid address family!\n");
    rc = SOCKET_ERROR;
  }

  if (rc != SOCKET_ERROR)
  {
    // Join the group
    rc = setsockopt(s, optlevel, option, optval, optlen);
    if (rc == SOCKET_ERROR)
    {
      printf("JoinMulticastGroup: setsockopt failed with error code %d\n", WSAGetLastError());
    }
    else
    {
      printf("Joined group: ");
      PrintAddress(group->ai_addr, group->ai_addrlen);
      printf("\n");
    }
  }

  return rc;
}

// Function: SetSendInterface
// Description:
//    This routine sets the send (outgoing) interface of the socket.
//    Again, for v4 the IP address is used to specify the interface while
//    for v6 its the scope-ID.
int SetSendInterface(SOCKET s, struct addrinfo* iface)
{
  char* optval = NULL;
  int optlevel, option, optlen, rc;
  rc = NO_ERROR;
  if (iface->ai_family == AF_INET)
  {
    // Setup the v4 option values
    optlevel = IPPROTO_IP;
    option   = IP_MULTICAST_IF;
    optval   = (char*)&((SOCKADDR_IN*)iface->ai_addr)->sin_addr.s_addr;
    optlen   = sizeof(((SOCKADDR_IN*)iface->ai_addr)->sin_addr.s_addr);
  }
  else if (iface->ai_family == AF_INET6)
  {

    // Setup the v6 option values
    optlevel = IPPROTO_IPV6;
    option   = IPV6_MULTICAST_IF;
    optval   = (char*)&((SOCKADDR_IN6*)iface->ai_addr)->sin6_scope_id;
    optlen   = sizeof(((SOCKADDR_IN6*)iface->ai_addr)->sin6_scope_id);
  }

  else
  {
    fprintf(stderr, "Attempting to set sent interface for invalid address family!\n");
    rc = SOCKET_ERROR;
  }

  // Set send IF
  if (rc != SOCKET_ERROR)
  {
    // Set the send interface
    rc = setsockopt(s, optlevel, option, optval, optlen);
    if (rc == SOCKET_ERROR)
    {
      printf("setsockopt() failed with error code %d\n", WSAGetLastError());
    }
    else
    {
      printf("Set sending interface to: ");
      PrintAddress(iface->ai_addr, iface->ai_addrlen);
      printf("\n");
    }
  }

  return rc;
}

// Function: SetMulticastTtl
// Description: This routine sets the multicast TTL value for the socket.
int SetMulticastTtl(SOCKET s, int af, int ttl)
{
  char* optval = NULL;
  int optlevel, option, optlen, rc;
  rc = NO_ERROR;
  if (af == AF_INET)
  {
    // Set the options for V4
    optlevel = IPPROTO_IP;
    option   = IP_MULTICAST_TTL;
    optval   = (char*)&ttl;
    optlen   = sizeof(ttl);
  }

  else if (af == AF_INET6)
  {
    // Set the options for V6
    optlevel = IPPROTO_IPV6;
    option   = IPV6_MULTICAST_HOPS;
    optval   = (char*)&ttl;
    optlen   = sizeof(ttl);
  }
  else
  {
    fprintf(stderr, "Attempting to set TTL for invalid address family!\n");
    rc = SOCKET_ERROR;
  }

  if (rc != SOCKET_ERROR)
  {
    // Set the TTL value
    rc = setsockopt(s, optlevel, option, optval, optlen);
    if (rc == SOCKET_ERROR)
    {
      fprintf(stderr, "SetMulticastTtl: setsockopt() failed with error code %d\n", WSAGetLastError());
    }
    else
    {
      printf("Set multicast ttl to: %d\n", ttl);
    }
  }
  return rc;
}

// Function: SetMulticastLoopBack()
// Description:
//    This function enabled or disables multicast loopback. If loopback is enabled
//    (and the socket is a member of the destination multicast group) then the
//    data will be placed in the receive queue for the socket such that if a
//    receive is posted on the socket its own data will be read. For this sample
//    it doesn't really matter as if invoked as the sender, no data is read.
int SetMulticastLoopBack(SOCKET s, int af, int loopval)
{
  char* optval = NULL;
  int optlevel, option, optlen, rc;
  rc = NO_ERROR;
  if (af == AF_INET)
  {
    // Set the v4 options
    optlevel = IPPROTO_IP;
    option   = IP_MULTICAST_LOOP;
    optval   = (char*)&loopval;
    optlen   = sizeof(loopval);
  }
  else if (af == AF_INET6)
  {
    // Set the v6 options
    optlevel = IPPROTO_IPV6;
    option   = IPV6_MULTICAST_LOOP;
    optval   = (char*)&loopval;
    optlen   = sizeof(loopval);
  }
  else
  {
    fprintf(stderr, "Attempting to set multicast loopback for invalid address family!\n");
    rc = SOCKET_ERROR;
  }

  if (rc != SOCKET_ERROR)
  {
    // Set the multipoint loopback
    rc = setsockopt(s, optlevel, option, optval, optlen);
    if (rc == SOCKET_ERROR)
    {
      fprintf(stderr, "SetMulticastLoopBack: setsockopt() failed with error code %d\n", WSAGetLastError());
    }
    else
    {
      printf("Setting multicast loopback to: %d\n", loopval);
    }
  }

  return rc;
}

// Function: main
// Description:
//    Parse the command line arguments, load the Winsock library,
//    create a socket and join the multicast group. If set as a
//    sender then begin sending messages to the multicast group;
//    otherwise, call recvfrom() to read messages send to the group.
int main(int argc, char** argv)
{
  WSADATA wsd;
  SOCKET s;
  struct addrinfo *resmulti = NULL, *resbind = NULL, *resif = NULL;
  char* buf = NULL;

  int rc, i = 0;
  if (argc < 2)
  {
    usage(argv[0]);
    exit(1);
  }

  // Parse the command line
  ValidateArgs(argc, argv);
  if (WSAStartup(MAKEWORD(1, 1), &wsd) != 0)
  {
    printf("WSAStartup failed\n");
    return -1;
  }

  // Resolve the multicast address
  resmulti = ResolveAddress(gMulticast, gPort, AF_UNSPEC, gSocketType, gProtocol);
  if (resmulti == NULL)
  {
    fprintf(stderr, "Unable to convert multicast address '%s': Error %d\n", gMulticast, WSAGetLastError());
    return -1;
  }

  // Resolve the binding address
  resbind = ResolveAddress(gBindAddr, (bSender ? "0" : gPort), resmulti->ai_family, resmulti->ai_socktype, resmulti->ai_protocol);
  if (resbind == NULL)
  {
    fprintf(stderr, "Unable to convert bind address '%s': Error %d\n", gBindAddr, WSAGetLastError());
    return -1;
  }

  // Resolve the multicast interface
  resif = ResolveAddress(gInterface, "0", resmulti->ai_family, resmulti->ai_socktype, resmulti->ai_protocol);
  if (resif == NULL)
  {
    fprintf(stderr, "Unable to convert interface address '%s': Error %d\n", gInterface, WSAGetLastError());
    return -1;
  }

  // Create the socket. In Winsock 1 you don't need any special
  // flags to indicate multicasting.
  s = socket(resmulti->ai_family, resmulti->ai_socktype, resmulti->ai_protocol);
  if (s == INVALID_SOCKET)
  {
    printf("socket() failed with with error code %d\n", WSAGetLastError());
    return -1;
  }
  printf("socket handle = 0x%p\n", s);

  // Bind the socket to the local interface. This is done so we can receive data
  rc = bind(s, resbind->ai_addr, resbind->ai_addrlen);
  if (rc == SOCKET_ERROR)
  {
    printf("bind() failed with error code %d\n", WSAGetLastError());
    return -1;
  }

  printf("Binding to ");
  PrintAddress(resbind->ai_addr, resbind->ai_addrlen);
  printf("\n");

  // Join the multicast group if specified
  if (bDontJoin == FALSE)
  {
    rc = JoinMulticastGroup(s, resmulti, resif);
    if (rc == SOCKET_ERROR)
    {
      return -1;
    }
  }

  // Set the send (outgoing) interface
  rc = SetSendInterface(s, resif);
  if (rc == SOCKET_ERROR)
  {
    return -1;
  }

  // Set the TTL to something else. The default TTL is one.

  rc = SetMulticastTtl(s, resmulti->ai_family, gTtl);
  if (rc == SOCKET_ERROR)
  {
    return -1;
  }

  // Disable the loopback if selected. Note that on NT4 and Win95 you cannot disable it.
  if (bLoopBack)
  {
    rc = SetMulticastLoopBack(s, resmulti->ai_family, gLoopBack);
    if (rc == SOCKET_ERROR)
    {
      return -1;
    }
  }

  // When using sendto on an IPv6 multicast socket, the scope id needs to be zero
  if ((bSender) && (resmulti->ai_family == AF_INET6))
    ((SOCKADDR_IN6*)resmulti->ai_addr)->sin6_scope_id = 0;
  if (bConnect)
  {
    rc = connect(s, resmulti->ai_addr, resmulti->ai_addrlen);
    if (rc == SOCKET_ERROR)
    {
      printf("connect() failed with error code %d\n", WSAGetLastError());
      return -1;
    }
  }

  buf = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, gBufferSize);
  if (buf == NULL)
  {
    fprintf(stderr, "HeapAlloc() failed with error code %d\n", GetLastError());
    return -1;
  }

  if (!bSender) // receiver
  {
    SOCKADDR_STORAGE safrom;
    int fromlen;
    for (i = 0; i < gCount; i++)
    {
      fromlen = sizeof(safrom);
      rc      = recvfrom(s, buf, gBufferSize, 0, (SOCKADDR*)&safrom, &fromlen);
      if (rc == SOCKET_ERROR)
      {
        printf("recvfrom() failed with error code %d\n", WSAGetLastError());
        break;
      }

      printf("read %d bytes from <", rc);
      PrintAddress((SOCKADDR*)&safrom, fromlen);
      printf(">\n");
    }
  }
  else // sender
  {
    memset(buf, '$', gBufferSize);
    // Send some data
    for (i = 0; i < gCount; i++)
    {
      rc = sendto(s, buf, gBufferSize, 0, resmulti->ai_addr, resmulti->ai_addrlen);
      if (rc == SOCKET_ERROR)
      {
        printf("sendto() failed with error code %d\n", WSAGetLastError());
        return -1;
      }

      printf("Sent %d bytes to ", rc);
      PrintAddress(resmulti->ai_addr, resmulti->ai_addrlen);
      printf("\n");

      Sleep(500);
    }
  }

  freeaddrinfo(resmulti);
  freeaddrinfo(resbind);
  freeaddrinfo(resif);
  closesocket(s);
  WSACleanup();
  return 0;
}
