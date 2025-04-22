//
// Socket  File: socket.cpp
//
// Copyright 1998 Paul S. Hethmon
//
#ifdef _WIN32
#define socklen_t int
#endif

#include "socket.hpp"

// ------------------------------------------------------------------

Socket::Socket()
{
  iSock = -1;
  iLen = sizeof(siUs);
  iErr = 0;
  iEol = FALSE;
  szOutBuf = new char[MAX_SOCK_BUFFER];
  szBuf1 = new char[MAX_SOCK_BUFFER];
  szBuf2 = new char[MAX_SOCK_BUFFER];
  iBeg1 = iEnd1 = iBeg2 = iEnd2 = 0;
  iBuf = 1;
  szPeerIp = NULL;
  szPeerName = NULL;
  ulTimeout = 5 * 60;  // 5 minutes default.
}

// ------------------------------------------------------------------

Socket::~Socket()
{
  if (iSock > -1) soclose(iSock);
  delete [] szOutBuf;
  delete [] szBuf1;
  delete [] szBuf2;
  if (szPeerIp) delete [] szPeerIp;
  if (szPeerName) delete [] szPeerName;
}

// ------------------------------------------------------------------

int
Socket::Create()                     // Allocate a socket for use
{
  iSock = socket(AF_INET, SOCK_STREAM, 0);
  return iSock;
}

// ------------------------------------------------------------------

int
Socket::SetOob()  // Set SO_OOBINLINE
{
  int optval = 1;

  iErr = setsockopt(iSock, SOL_SOCKET, SO_OOBINLINE, (char *) &optval, sizeof(int));

  return iErr;
}

// ------------------------------------------------------------------

int
Socket::SetLinger(int iTime)  // Set the SO_LINGER option
{
  struct linger L;

  L.l_onoff = 1;
  L.l_linger = 20; // linger for 20 seconds to make sure buffer flushed
  iErr = setsockopt(iSock, SOL_SOCKET, SO_LINGER, (char *) &L, sizeof(L));

  return iErr;
}

// ------------------------------------------------------------------

int
Socket::SetKeepAlive()  // Set SO_KEEPALIVE
{
  int optval = 1;

  // Tell the stack to ping the other host to make sure the
  // connection doesn't drop.
  iErr = setsockopt(iSock, SOL_SOCKET, SO_KEEPALIVE, (char *) &optval, sizeof(int));

  return iErr;
}

// ------------------------------------------------------------------

int
Socket::SetReusePort(int iReuse)  // Set SO_REUSEADDR
{
  int optval;

  if (iReuse == REUSE_PORT)
    {
      optval = 1;
    }
  else
    {
      optval = 0;
    }

  iErr = setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(int));

  return iErr;
}

// ------------------------------------------------------------------
//
// Passive
//
// Place the socket into passive mode.
//

int
Socket::Passive(u_short usPort, int iReuse, u_long ulAddr)
{
  int optval = 1;

  if (iReuse > 0)  // Force reuse of the address.
    {
      setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(int));
    }

  usPortUs = usPort;

  bzero((void *)&siUs, iLen);              // make sure everything zero
  siUs.sin_family = AF_INET;
  siUs.sin_port = htons(usPortUs);
  siUs.sin_addr.s_addr = ulAddr;

  // Bind to the given port
  iErr = bind(iSock, (struct sockaddr *) &siUs, iLen);
  if (iErr < 0)
    {
      return iErr;
    }

  // change to passive socket
  iErr = listen(iSock, SOMAXCONN);
  if (iErr < 0)
    {
      return iErr;
    }

  if (usPortUs == 0)  // Look up the port we got.
    {
      bzero((void *)&siUs, iLen);              // make sure everything zero
      iErr = getsockname(iSock, (sockaddr *)&siUs, (socklen_t *)&iLen);
      if (iErr < 0)
        {
          return iErr;
        }
      usPortUs = ntohs(siUs.sin_port);
    }

  return 0;
}

// ------------------------------------------------------------------
//
// Connect
//
// Connect to the specified remote host.
//

int
Socket::Connect(char *szRemoteIp, u_short usRemotePort, 
                u_short usLocalPort, u_long ulAddr)
{
  struct hostent *heHost;
  u_long ulClientAddr;

  heHost = NULL;
  heHost = gethostbyname(szRemoteIp);
  if (heHost == NULL)
    {
      ulClientAddr = inet_addr(szRemoteIp);
      if (ulClientAddr == -1)
        {
          iErr = -1;
          return (iErr);
        }
    }
  else
    {
      ulClientAddr = *((u_long *)heHost->h_addr);
    }

  bzero((void *)&siUs, iLen);              // make sure everything zero
  siThem.sin_family = AF_INET;
  siThem.sin_port = htons(usRemotePort);
  siThem.sin_addr.s_addr = ulClientAddr;

  bzero((void *)&siUs, sizeof(siUs));
  siUs.sin_family = AF_INET;
  // Use the specified domain name address
  siUs.sin_addr.s_addr = ulAddr;
  // Always use the user defined port
  usPortUs = usLocalPort;
  siUs.sin_port = htons(usPortUs);

  iErr = bind(iSock, (struct sockaddr *)&siUs, iLen);
  if (iErr != 0)
    {
      return iErr;
    }

  iErr = connect(iSock, (sockaddr *)&siThem, iLen);
  if (iErr < 0) {
	  return iErr;
  }
  if (usPortUs == 0)  // Look up the port we got.
    {
      bzero((void *)&siUs, iLen);              // make sure everything zero
      iErr = getsockname(iSock, (sockaddr *)&siUs, (socklen_t *)&iLen);
      if (iErr < 0)
        {
          return iErr;
        }
      usPortUs = ntohs(siUs.sin_port);
    }
  return (iErr);
}

// ------------------------------------------------------------------
//
// Accept
//
// Accept an incoming connection on the socket.
//

Socket *
Socket::Accept()
{
  Socket *sSock;

  sSock = new Socket();

  bzero(&siThem, iLen);
  sSock->iSock = accept(iSock, (struct sockaddr *)&(sSock->siThem), (socklen_t *)&iLen);
  if (sSock->iSock < 0)
    {
      iErr = sSock->iSock;
      delete sSock;
      return NULL;
    }

  sSock->szPeerIp = new char[128];
  strncpy(sSock->szPeerIp, inet_ntoa(sSock->siThem.sin_addr), 128);

  return sSock;
}

// ------------------------------------------------------------------
//
// Send
//
// Send the specified buffer across the socket.
//

int 
Socket::Send(char *szBuf, int iLen)
{
#ifdef __OS2__
  int fdsSocks[1];
#elif __WINDOWS__
  fd_set fdsSocks;
  struct timeval stTimeout;
#endif

#ifdef __OS2__
  fdsSocks[0] = iSock;
  iErr = os2_select(fdsSocks, 0, 1, 0, ulTimeout * 1000);
  if (iErr < 1) // Error occured.
    {
      return iErr;
    }
#endif
  iErr = send(iSock, szBuf, iLen, 0);
  return iErr;
}

// ------------------------------------------------------------------
//
// SendText
//
// Send the specified file across the socket. This assumes
// a text file.
//

int
Socket::SendText(char *szFileName, u_long ulOffset)
{
#ifdef __OS2__
  int fdsSocks[1];
#elif __WINDOWS__
  fd_set fdsSocks;
  struct timeval stTimeout;
#endif
  ifstream ifIn;
  char *szBuf;

  ifIn.open(szFileName);
  if (! ifIn)
    {
      return -1;
    }

  if (ulOffset > 0)
    {
      // Start at the position in the class they asked for
      ifIn.seekg(ulOffset);
    }

  szBuf = new char[SOCK_BUFSIZE];
  iErr = 0;
  do
    {
      memset(szBuf, 0, SOCK_BUFSIZE);
      ifIn.getline(szBuf, SOCK_BUFSIZE, '\n');
#ifdef __OS2__
      fdsSocks[0] = iSock;
      iErr = os2_select(fdsSocks, 0, 1, 0, ulTimeout * 1000);
      if (iErr < 1) // Error occured.
        {
          ifIn.close();
          delete [] szBuf;
          return iErr;
        }
#endif
      iErr += send(iSock, szBuf, strlen(szBuf), 0);    // The line.
      if ( ifIn.eof() ) break;                         // The last line
                                                       // doesn't get an
                                                       // eol appended.
      iErr += send(iSock, "\r\n", strlen("\r\n"), 0);  // The eol.
    }
  while ( ! ifIn.eof() );

  ifIn.close();
  delete [] szBuf;
  return iErr;
}

// ------------------------------------------------------------------
//
// SendDotText
//
// Send the specified file across the socket. This assumes
// a text file. The text file is byte stuffed as necessary
// to allow transfer in an SMTP or POP3 session.
//

int
Socket::SendDotText(char *szFileName)
{
#ifdef __OS2__
  int fdsSocks[1];
#elif __WINDOWS__
  fd_set fdsSocks;
  struct timeval stTimeout;
#endif
  ifstream ifIn;
  char *szBuf,
       *szSendBuf;


  ifIn.open(szFileName);
  if (! ifIn)
    {
      return -1;
    }

  szBuf = new char[SOCK_BUFSIZE];
  szSendBuf = new char[MAX_SEND_BUFFER];
  szSendBuf[0] = '\000';
  iErr = 0;
  do
    {
      memset(szBuf, 0, SOCK_BUFSIZE);
      ifIn.getline(szBuf, SOCK_BUFSIZE, '\n');
#ifdef __OS2__
      fdsSocks[0] = iSock;
      iErr = os2_select(fdsSocks, 0, 1, 0, ulTimeout * 1000);
      if (iErr < 1) // Error occured.
        {
          ifIn.close();
          delete [] szBuf;
          delete [] szSendBuf;
          return iErr;
        }
#endif
      if ( (strlen(szSendBuf) + strlen(szBuf) + 32) < MAX_SEND_BUFFER)
        {
          if (szBuf[0] == '.')
            {
              strcat(szSendBuf, ".");
            }
          strcat(szSendBuf, szBuf);
          strcat(szSendBuf, "\r\n");
        }
      else // Buffer full, send now
        {
          iErr += send(iSock, szSendBuf, strlen(szSendBuf), 0);
          szSendBuf[0] = '\000'; // empty buffer
          // Add last line read to send buffer
          if (szBuf[0] == '.')
            {
              strcat(szSendBuf, ".");
            }
          strcat(szSendBuf, szBuf);
          strcat(szSendBuf, "\r\n");
        }
    }
  while ( ! ifIn.eof() );

  if (strlen(szSendBuf) > 0)
    {
      iErr += send(iSock, szSendBuf, strlen(szSendBuf), 0);
    }
  iErr += send(iSock, ".\r\n", strlen(".\r\n"), 0);

  ifIn.close();
  delete [] szBuf;
  delete [] szSendBuf;
  return iErr;
}

// ------------------------------------------------------------------
//
// SendBinary
//
// Send the specified file across the socket. This assumes
// a binary file.
//

int
Socket::SendBinary(char *szFileName, u_long ulOffset)
{
#ifdef __OS2__
  int fdsSocks[1];
#elif __WINDOWS__
  fd_set fdsSocks;
  struct timeval stTimeout;
#endif
  ifstream ifIn;
  char *szBuf;
  iErr = 0;

  ifIn.open(szFileName, ios::binary);
  if (! ifIn)
    {
      return -1;
    }

  if (ulOffset > 0)
    {
      // Start at the position in the class they asked for
      ifIn.seekg(ulOffset);
    }

  szBuf = new char[SOCK_BUFSIZE];

  while ( ! ifIn.eof() )
    {
      ifIn.read(szBuf, SOCK_BUFSIZE);
#ifdef __OS2__
      fdsSocks[0] = iSock;
      iErr = os2_select(fdsSocks, 0, 1, 0, ulTimeout * 1000);
      if (iErr < 1) // Error occured.
        {
          ifIn.close();
          delete [] szBuf;
          return iErr;
        }
#endif
      iErr = send(iSock, szBuf, ifIn.gcount(), 0);    // The line
    }

  ifIn.close();
  delete [] szBuf;
  return iErr;
}

// ------------------------------------------------------------------
//
// Recv
//
// Receive up to iBytes on this socket.
//

int
Socket::Recv(int iBytes)
{
#ifdef __OS2__
  int fdsSocks[1];
#elif __WINDOWS__
  fd_set fdsSocks;
  struct timeval stTimeout;
#endif

  memset(szOutBuf, 0, MAX_SOCK_BUFFER);
  iErr = 0;

  if ((iBuf == 1) && (iEnd1 != 0))  // Copy the contents of buf 1.
    {
      if (iBytes >= (iEnd1 - iBeg1))  // Copy all the bytes.
        {
          memcpy(szOutBuf, szBuf1 + iBeg1, iEnd1 - iBeg1);
          iErr = iEnd1 - iBeg1;
          iBeg1 = iEnd1 = 0;
          iBuf = 2;
        }
      else  // Only copy the requested number.
        {
          memcpy(szOutBuf, szBuf1 + iBeg1, iBytes);
          iErr = iBytes;    // This many bytes sent back.
          iBeg1 += iBytes;  // Advance to this location.
        }
    }      
  else if ((iBuf == 2) && (iEnd2 != 0))  // Copy the contents of buf 2.
    {
      if (iBytes >= (iEnd2 - iBeg2))
        {
          memcpy(szOutBuf, szBuf2 + iBeg2, iEnd2 - iBeg2);
          iErr = iEnd2 - iBeg2;
          iBeg2 = iEnd2 = 0;
          iBuf = 1;
        }
      else
        {
          memcpy(szOutBuf, szBuf2 + iBeg2, iBytes);
          iErr = iBytes;
          iBeg1 += iBytes;
        }
    }      
  else
    {
#ifdef __OS2__
      fdsSocks[0] = iSock;
      iErr = os2_select(fdsSocks, 1, 0, 0, ulTimeout * 1000);
      if (iErr < 1) // Error occured.
        {
          return -1;
        }
#elif __WINDOWS__
      FD_ZERO(&fdsSocks);
      FD_SET(iSock, &fdsSocks);
      stTimeout.tv_sec = ulTimeout;
      iErr = select(1, &fdsSocks, 0, 0, &stTimeout);
      if (iErr < 1) // Error occured.
        {
          return -1;
        }
#endif
      iErr = recv(iSock, szOutBuf, iBytes, 0);
      if (iErr == 0) return -1;
    }

  return iErr;
}

// ------------------------------------------------------------------
//
// RecvTeol
//
// Receive a line delimited nominally by the telnet end-of-line
// sequence -- CRLF. This one also accepts just CR or just LF
// also.
//
// A return value of less than 0 indicates an error with the connection.
//

int
Socket::RecvTeol(int iToast)
{
  int iState = 1,
      idx = 0;
#ifdef __OS2__
  int fdsSocks[1];
#elif __WINDOWS__
  fd_set fdsSocks;
  struct timeval stTimeout;
#endif

  memset(szOutBuf, 0, MAX_SOCK_BUFFER);
  iEol = FALSE;
  iErr = 0;

  while (iState != 0)
    {
      switch (iState)
        {
          case 1:  // Figure out where to start.
            {
              if ((iEnd1 == 0) && (iEnd2 == 0)) // Both buffers empty.
                {
                  iState = 2;
                }
              else
                {
                  iState = 3;
                }
              break;
            }
          case 2:  // Fill the buffers with data.
            {
#ifdef __OS2__
              fdsSocks[0] = iSock;
              iErr = os2_select(fdsSocks, 1, 0, 0, ulTimeout * 1000);
              if (iErr < 1) // Error occured.
                {
                  iState = 0;
                  iErr = -1;
                  break;
                }
#elif __WINDOWS__
              FD_ZERO(&fdsSocks);
              FD_SET(iSock, &fdsSocks);
              stTimeout.tv_sec = ulTimeout;
              iErr = select(1, &fdsSocks, 0, 0, &stTimeout);
              if (iErr < 1) // Error occured.
                {
                  iState = 0;
                  iErr = -1;
                  break;
                }
#endif
              iErr = recv(iSock, szBuf1, MAX_SOCK_BUFFER - 1, 0);
              if (iErr < 1)
                {
                  iState = 0;
                  iErr = -1;
                  break;
                }
              iBeg1 = 0;
              iEnd1 = iErr;
              if (iErr == (MAX_SOCK_BUFFER - 1))  // Filled up Buffer 1.
                {
#ifdef __OS2__
                  fdsSocks[0] = iSock;
                  iErr = os2_select(fdsSocks, 1, 0, 0, ulTimeout * 1000);
                  if (iErr < 1) // Error occured.
                    {
                      iState = 0;
                      iErr = -1;
                      break;
                    }
#elif __WINDOWS__
                  FD_ZERO(&fdsSocks);
                  FD_SET(iSock, &fdsSocks);
                  stTimeout.tv_sec = ulTimeout;
                  iErr = select(1, &fdsSocks, 0, 0, &stTimeout);
                  if (iErr < 1) // Error occured.
                    {
                      iState = 0;
                      iErr = -1;
                      break;
                    }
#endif
                  iErr = recv(iSock, szBuf2, MAX_SOCK_BUFFER - 1, 0);
                  if (iErr < 1)
                    {
                      iState = 0;
                      iErr = -1;
                      break;
                    }
                  iBeg2 = 0;
                  iEnd2 = iErr;
                }
              iBuf = 1;
              iState = 3;  // Advance to the next state.
              break;
            }
          case 3:  // Look for the EOL sequence.
            {
              if ((iBuf == 1) && (iEnd1 != 0))  // Use Buffer 1 first.
                {
                  for ( ; iBeg1 < iEnd1; iBeg1++)
                    {
                      szOutBuf[idx] = szBuf1[iBeg1];   // Copy.
                      if ((szOutBuf[idx - 1] == '\r') && (szOutBuf[idx] == '\n'))
                        {
                          iBeg1++;                     // Count the char just read.
                          if (iBeg1 == iEnd1)
                            {
                              iBeg1 = iEnd1 = 0;   // Reset.
                              iBuf = 2;
                            }
                          szOutBuf[idx + 1] = '\0';    // Done. Null line.
                          iState = 4;                  // Goto cleanup & exit.
                          iEol = TRUE;
                          break;                       // Break from for loop.
                        }
                      else if (szOutBuf[idx - 1] == '\r')
                        {
                          // A real bad implementation found, bare CRs in the stream
                          szOutBuf[idx-1] = ' ';
                        }
                      else if (szOutBuf[idx] == '\n')
                        {
                          iBeg1++;                     // Count the char just read.
                          if (iBeg1 == iEnd1)
                            {
                              iBeg1 = iEnd1 = 0;   // Reset.
                              iBuf = 2;
                            }
                          szOutBuf[idx + 1] = '\0';    // Done. Null line.
                          iState = 4;                  // Goto cleanup & exit.
                          iEol = TRUE;
                          break;                       // Break from for loop.
                        }
                      idx++;                           // Advance to next spot.
                      if ((idx+1) == MAX_SOCK_BUFFER)     // Out of room.
                        {
                          if (szOutBuf[idx - 1] == '\r')
                            {
                              // push it back onto the buffer.
                              idx--;
                            }
                          else
                            {
                              iBeg1++;
                            }
                          if (iBeg1 == iEnd1)
                            {
                              iBeg1 = iEnd1 = 0;   // Reset.
                              iBuf = 2;
                            }
                          szOutBuf[idx] = '\0';
                          iState = 4;
                          break;
                        }
                    }
                  if (iBeg1 == iEnd1) iBeg1 = iEnd1 = 0;   // Reset.
                  if (iState == 3)    iBuf = 2;            // EOL not found yet.
                }
              else if ((iBuf == 2) && (iEnd2 != 0))    // Use Buffer 2.
                {
                  for ( ; iBeg2 < iEnd2; iBeg2++)
                    {
                      szOutBuf[idx] = szBuf2[iBeg2];   // Copy.
                      if ((szOutBuf[idx - 1] == '\r') && (szOutBuf[idx] == '\n'))
                        {
                          iBeg2++;                     // Count the char just read
                          if (iBeg2 == iEnd2)
                            {
                              iBeg2 = iEnd2 = 0;   // Reset.
                              iBuf = 1;
                            }
                          szOutBuf[idx + 1] = '\0';    // Done. Null line.
                          iState = 4;                  // Goto cleanup & exit.
                          iEol = TRUE;
                          break;                       // Break from for loop.
                        }
                      else if (szOutBuf[idx - 1] == '\r')
                        {
                          // A real bad implementation found, bare CRs in the stream
                          szOutBuf[idx-1] = ' ';
                        }
                      else if (szOutBuf[idx] == '\n')
                        {
                          iBeg2++;
                          if (iBeg2 == iEnd2)
                            {
                              iBeg2 = iEnd2 = 0;   // Reset.
                              iBuf = 1;
                            }
                          szOutBuf[idx + 1] = '\0';    // Done. Null line.
                          iState = 4;                  // Goto cleanup & exit.
                          iEol = TRUE;
                          break;                       // Break from for loop.
                        }
                      idx++;                           // Advance to next spot.
                      if ((idx+1) == MAX_SOCK_BUFFER)     // Out of room.
                        {
                          if (szOutBuf[idx - 1] == '\r')
                            {
                              // push it back onto the buffer.
                              idx--;
                            }
                          else
                            {
                              iBeg2++;
                            }
                          if (iBeg2 == iEnd2)
                            {
                              iBeg2 = iEnd2 = 0;   // Reset.
                              iBuf = 1;
                            }
                          szOutBuf[idx] = '\0';
                          iState = 4;
                          break;
                        }
                    }
                  if (iBeg2 == iEnd2) iBeg2 = iEnd2 = 0;   // Reset.
                  if (iState == 3)    iBuf = 1;            // EOL not found yet.
                }
              else  // Both buffers empty and still no eol.
                {
                  if (idx < MAX_SOCK_BUFFER)
                    {
                      iState = 2;  // Still room. Refill the buffers.
                    }
                  else
                    {
                      iState = 4;  // Out of room. Return.
                    }
                }
              break;
            }
          case 4:  // Cleanup and exit.
            {
              iState = 0;
              break;
            }
        } // End of switch statement.
    } // End of while loop.

  if (iToast > 0)  // Remove the telnet end of line before returning.
    {
      while (( (szOutBuf[idx] == '\r') || (szOutBuf[idx] == '\n') ) && (idx > -1))
        {
          szOutBuf[idx] = '\0';
          idx--;
        }
    }

  if (iErr < 0) return iErr;
  
  // Must add 1 to idx since idx is an index into the actual
  // array, not the number of bytes returned.
  return (idx + 1);
}
              
// ------------------------------------------------------------------
//
// ResolveName
//
// Look up the name of the peer connected to this socket.
//

int
Socket::ResolveName()
{
  struct hostent *hePeer;

  if (szPeerIp == NULL)  // Only if we don't have it already.
    {
      szPeerIp = new char[128];
      strncpy(szPeerIp, inet_ntoa(siThem.sin_addr), 128);
    }
  szPeerName = new char[128];
  hePeer = gethostbyaddr((char *)&(siThem.sin_addr),
                          sizeof(struct in_addr), AF_INET);
  if (hePeer != NULL)  // We found the ip name.
    {
      strncpy(szPeerName, hePeer->h_name, 128);
      iErr = 0;        // Good return.
    }
  else                 // No name available for this host.
    {
      strncpy(szPeerName, szPeerIp, 128);
      iErr = -1;       // Bad return.
    }

  return iErr;
}          

// ------------------------------------------------------------------
//
// SetPeerName
//
// Set the name by using the ip address instead of dns lookup.
//

int
Socket::SetPeerName()
{
  if (szPeerIp == NULL)  // Only if we don't have it already.
    {
      szPeerIp = new char[128];
      strncpy(szPeerIp, inet_ntoa(siThem.sin_addr), 128);
    }
  szPeerName = new char[128];
  sprintf(szPeerName, "[%s]", szPeerIp);
  iErr = 0;

  return iErr;
}          

// ------------------------------------------------------------------

int
Socket::Close()                      // Close this socket
{
  iBeg1 = iEnd1 = iBeg2 = iEnd2 = 0;
  iBuf = 1;
  memset(szOutBuf, 0, MAX_SOCK_BUFFER);
  memset(szBuf1, 0, MAX_SOCK_BUFFER/2);
  memset(szBuf2, 0, MAX_SOCK_BUFFER/2);
  if (szPeerIp) delete [] szPeerIp;
  if (szPeerName) delete [] szPeerName;
  szPeerIp = NULL;
  szPeerName = NULL;
  ulTimeout = 5 * 60;  // 5 minutes default.
  usPortUs = 0;
  usPortThem = 0;

  iErr = soclose(iSock);
  iSock = -1;
  return iErr;
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------

