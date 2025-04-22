 //
// Socket  File: socket.hpp
//
//
// Copyright 1998 Paul S. Hethmon
//

//
// Socket Class
//

#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <sys/types.h>

#ifdef __OS2__
  #include <os2.h>
  #define OS2        // This is needed for the socket header files
  #include <types.h>
  #include <netdb.h>
  #include <nerrno.h>
  #include <sys/socket.h>
  #include <sys/ioctl.h>
  #include <netinet/in_systm.h>
  #include <netinet/in.h>
  #include <netinet/ip.h>
  #include <utils.h>
  #ifdef TCPV40HDRS
    #define os2_select select
  #endif
  #ifndef TCPV40HDRS
    #include <unistd.h>
    #include <arpa/inet.h>
  #endif
#elif __WINDOWS__
  #include <windows.h>
  #include <winsock.h>
  #define soclose(x) closesocket(x)
#endif

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#define soclose(x) closesocket(x)
#endif

#ifndef _WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define soclose(x) close(x)
#endif

#include <memory.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>

#ifndef bzero
#define bzero(x,y) memset((x),'\0',(y))
#endif

// ------------------------------------------------------------------

#define FALSE 0
#define TRUE 1
// Define this to the size of the largest ascii line of
// data your application expects to receive.
#define SOCK_BUFSIZE     8192
//#define MAX_SOCK_BUFFER  48
#define MAX_SOCK_BUFFER  (16 * 1024)
#define NO_EOL           1
#define REUSE_PORT       1
#define MAX_SEND_BUFFER  32768

#ifdef __OS2__
  #define LOCAL_EOL        "\r\n"
#elif defined __WINDOWS__
  #define LOCAL_EOL        "\r\n"
#endif

class Socket
{
  public:

  Socket();
  ~Socket();
  int Create();                    // Allocate a socket for use
  int SetOob();                    // Set the OOBINLINE flag
  int SetLinger(int iTime);        // Set the linger time on the socket.
  int SetKeepAlive();              // Set the keepalive signal.
  int SetReusePort(int iFlag);     // Set the reuse port flag
  u_short GetPort()
    {
      return usPortUs;
    };
  char * GetIp()
    {
      return strdup(inet_ntoa(siUs.sin_addr));
    };
  int Passive()                    // Turn the socket into a passive socket
    {                              // Do not set SO_REUSEADDR
      return(Passive(0, 0));       // Let the stack assign a port
    }
  int Passive(u_short usPort)      // Turn the socket into a passive socket
    {                              // Do not set SO_REUSEADDR
      return(Passive(usPort, 0));
    }
  int Passive(u_short usPort,      // Passive mode and set reuse flag
              int iReuse)
    {
      return(Passive(usPort, iReuse, INADDR_ANY));
    };
  int Passive(u_short usPort,      // Turn the socket into a passive socket
              int iReuse,          // Allow setting of SO_REUSEADDR
              u_long ulAddr);      // Specify the address to use
  Socket * Accept();               // Listen for connections
  int Connect(char *szBuf,         // Connect the socket to the remote host
              u_short usRemotePort)
    {
      return(Connect(szBuf, usRemotePort, 0, INADDR_ANY));
    };
  int Connect(char *szBuf,         // Connect the socket to the remote host
              u_short usRemotePort,
              u_short usLocalPort)
    {
      return(Connect(szBuf, usRemotePort, usLocalPort, INADDR_ANY));
    };
  int Connect(char *szRemoteIp,    // Connect the socket to the remote host
              u_short usRemotePort,
              u_short usLocalPort,
              unsigned long ulAddr);
  int Recv()                       // Receive bytes on this socket
    {
      return(Recv(MAX_SOCK_BUFFER));
    }
  int Recv(int iBytes);            // Receive up to iBytes on this socket
  int RecvTeol()                   // Receive up to the telnet eol
    {
      return(RecvTeol(0));         // Include the telnet eol
    }
  int RecvTeol(int iToast);        // Receive up to the telnet eol
                                   // and possibly remove the telnet eol
  int Send(char *szBuf)            // Send the text buffer on this socket
    {
      return Send(szBuf, strlen(szBuf));
    };
  int Send(const char *szBuf)      // Send the text buffer on this socket
    {
      return Send((char *)szBuf, strlen(szBuf));
    };
  int Send(char *szBuf, int iLen); // Send this buffer with this length.
  int SendText(char *szFileName)   // Send this text file across the socket
    {
      return (SendText(szFileName, 0));
    };
  int SendText(char *szFileName, 
               u_long ulOffset);   // Send this text file across the socket
  int SendDotText(char *szFileName);  // Send this text file across the socket,
                                      // byte stuff with dots for transparency
  int SendBinary(char *szFileName) // Send this binary file across the socket
    {
      return(SendBinary(szFileName, 0));
    };
  int SendBinary(char *szFileName, 
                 u_long ulOffset); // Send this binary file across the socket
  int ResolveName();               // Look up the ip address and name of the peer
  int SetPeerName();               // Set the ip name to the ip address.
  int Close();                     // Close this socket

  int iSock;                       // The socket number allocated
  int iErr;                        // The last error code on a socket call
  int iEol;                        // Flag for finding eol
  char *szOutBuf;                  // Used to return data in
  char *szPeerName;                // The ip name of the peer connected.
  char *szPeerIp;                  // The ip address of the peer connected.
  unsigned long ulTimeout;         // The timeout for receives in seconds.
  struct sockaddr_in siUs;         // Our address
  struct sockaddr_in siThem;       // Their address
  int iTid;

  protected:

  u_short usPortUs;          // Our port
  u_short usPortThem;        // Their port
  int iLen;                  // The size of siUs and siThem
  int iBuf;                  // Active buffer flag.
  char *szBuf1, *szBuf2;     // Internal buffers.
  int iBeg1, iEnd1,          // Buffer markers.
      iBeg2, iEnd2;
};

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------

