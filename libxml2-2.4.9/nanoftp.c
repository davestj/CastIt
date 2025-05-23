/*
 * nanoftp.c: basic FTP client support
 *
 *  Reference: RFC 959
 */

#ifdef TESTING
#define STANDALONE
#define HAVE_STDLIB_H
#define HAVE_UNISTD_H
#define HAVE_SYS_SOCKET_H
#define HAVE_NETINET_IN_H
#define HAVE_NETDB_H
#define HAVE_SYS_TIME_H
#else /* STANDALONE */
#define NEED_SOCKETS
#endif /* STANDALONE */

#include "libxml.h"

#ifdef LIBXML_FTP_ENABLED
#include <string.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h> 
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/nanoftp.h>
#include <libxml/globals.h>

/* #define DEBUG_FTP 1  */
#ifdef STANDALONE
#ifndef DEBUG_FTP
#define DEBUG_FTP 1
#endif
#endif

/**
 * A couple portability macros
 */
#ifndef _WINSOCKAPI_
#define closesocket(s) close(s)
#define SOCKET int
#endif
#if defined(VMS) || defined(__VMS)
#define SOCKLEN_T unsigned int
#endif

#define FTP_COMMAND_OK		200
#define FTP_SYNTAX_ERROR	500
#define FTP_GET_PASSWD		331
#define FTP_BUF_SIZE		512

typedef struct xmlNanoFTPCtxt {
    char *protocol;	/* the protocol name */
    char *hostname;	/* the host name */
    int port;		/* the port */
    char *path;		/* the path within the URL */
    char *user;		/* user string */
    char *passwd;	/* passwd string */
    struct sockaddr_in ftpAddr; /* the socket address struct */
    int passive;	/* currently we support only passive !!! */
    SOCKET controlFd;	/* the file descriptor for the control socket */
    SOCKET dataFd;	/* the file descriptor for the data socket */
    int state;		/* WRITE / READ / CLOSED */
    int returnValue;	/* the protocol return value */
    /* buffer for data received from the control connection */
    char controlBuf[FTP_BUF_SIZE + 1];
    int controlBufIndex;
    int controlBufUsed;
    int controlBufAnswer;
} xmlNanoFTPCtxt, *xmlNanoFTPCtxtPtr;

static int initialized = 0;
static char *proxy = NULL;	/* the proxy name if any */
static int proxyPort = 0;	/* the proxy port if any */
static char *proxyUser = NULL;	/* user for proxy authentication */
static char *proxyPasswd = NULL;/* passwd for proxy authentication */
static int proxyType = 0;	/* uses TYPE or a@b ? */

/**
 * xmlNanoFTPInit:
 *
 * Initialize the FTP protocol layer.
 * Currently it just checks for proxy informations,
 * and get the hostname
 */

void
xmlNanoFTPInit(void) {
    const char *env;
#ifdef _WINSOCKAPI_
    WSADATA wsaData;    
#endif

    if (initialized)
	return;

#ifdef _WINSOCKAPI_
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	return;
#endif

    proxyPort = 21;
    env = getenv("no_proxy");
    if (env != NULL)
	return;
    env = getenv("ftp_proxy");
    if (env != NULL) {
	xmlNanoFTPScanProxy(env);
    } else {
	env = getenv("FTP_PROXY");
	if (env != NULL) {
	    xmlNanoFTPScanProxy(env);
	}
    }
    env = getenv("ftp_proxy_user");
    if (env != NULL) {
	proxyUser = xmlMemStrdup(env);
    }
    env = getenv("ftp_proxy_password");
    if (env != NULL) {
	proxyPasswd = xmlMemStrdup(env);
    }
    initialized = 1;
}

/**
 * xmlNanoFTPCleanup:
 *
 * Cleanup the FTP protocol layer. This cleanup proxy informations.
 */

void
xmlNanoFTPCleanup(void) {
    if (proxy != NULL) {
	xmlFree(proxy);
	proxy = NULL;
    }
    if (proxyUser != NULL) {
	xmlFree(proxyUser);
	proxyUser = NULL;
    }
    if (proxyPasswd != NULL) {
	xmlFree(proxyPasswd);
	proxyPasswd = NULL;
    }
#ifdef _WINSOCKAPI_
    if (initialized)
	WSACleanup();
#endif
    initialized = 0;
}

/**
 * xmlNanoFTPProxy:
 * @host:  the proxy host name
 * @port:  the proxy port
 * @user:  the proxy user name
 * @passwd:  the proxy password
 * @type:  the type of proxy 1 for using SITE, 2 for USER a@b
 *
 * Setup the FTP proxy informations.
 * This can also be done by using ftp_proxy ftp_proxy_user and
 * ftp_proxy_password environment variables.
 */

void
xmlNanoFTPProxy(const char *host, int port, const char *user,
	        const char *passwd, int type) {
    if (proxy != NULL)
	xmlFree(proxy);
    if (proxyUser != NULL)
	xmlFree(proxyUser);
    if (proxyPasswd != NULL)
	xmlFree(proxyPasswd);
    if (host)
	proxy = xmlMemStrdup(host);
    if (user)
	proxyUser = xmlMemStrdup(user);
    if (passwd)
	proxyPasswd = xmlMemStrdup(passwd);
    proxyPort = port;
    proxyType = type;
}

/**
 * xmlNanoFTPScanURL:
 * @ctx:  an FTP context
 * @URL:  The URL used to initialize the context
 *
 * (Re)Initialize an FTP context by parsing the URL and finding
 * the protocol host port and path it indicates.
 */

static void
xmlNanoFTPScanURL(void *ctx, const char *URL) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    const char *cur = URL;
    char buf[4096];
    int indx = 0;
    int port = 0;

    if (ctxt->protocol != NULL) { 
        xmlFree(ctxt->protocol);
	ctxt->protocol = NULL;
    }
    if (ctxt->hostname != NULL) { 
        xmlFree(ctxt->hostname);
	ctxt->hostname = NULL;
    }
    if (ctxt->path != NULL) { 
        xmlFree(ctxt->path);
	ctxt->path = NULL;
    }
    if (URL == NULL) return;
    buf[indx] = 0;
    while (*cur != 0) {
        if ((cur[0] == ':') && (cur[1] == '/') && (cur[2] == '/')) {
	    buf[indx] = 0;
	    ctxt->protocol = xmlMemStrdup(buf);
	    indx = 0;
            cur += 3;
	    break;
	}
	buf[indx++] = *cur++;
    }
    if (*cur == 0) return;

    buf[indx] = 0;
    while (1) {
        if (cur[0] == ':') {
	    buf[indx] = 0;
	    ctxt->hostname = xmlMemStrdup(buf);
	    indx = 0;
	    cur += 1;
	    while ((*cur >= '0') && (*cur <= '9')) {
	        port *= 10;
		port += *cur - '0';
		cur++;
	    }
	    if (port != 0) ctxt->port = port;
	    while ((cur[0] != '/') && (*cur != 0)) 
	        cur++;
	    break;
	}
        if ((*cur == '/') || (*cur == 0)) {
	    buf[indx] = 0;
	    ctxt->hostname = xmlMemStrdup(buf);
	    indx = 0;
	    break;
	}
	buf[indx++] = *cur++;
    }
    if (*cur == 0) 
        ctxt->path = xmlMemStrdup("/");
    else {
        indx = 0;
        buf[indx] = 0;
	while (*cur != 0)
	    buf[indx++] = *cur++;
	buf[indx] = 0;
	ctxt->path = xmlMemStrdup(buf);
    }	
}

/**
 * xmlNanoFTPUpdateURL:
 * @ctx:  an FTP context
 * @URL:  The URL used to update the context
 *
 * Update an FTP context by parsing the URL and finding
 * new path it indicates. If there is an error in the 
 * protocol, hostname, port or other information, the
 * error is raised. It indicates a new connection has to
 * be established.
 *
 * Returns 0 if Ok, -1 in case of error (other host).
 */

int
xmlNanoFTPUpdateURL(void *ctx, const char *URL) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    const char *cur = URL;
    char buf[4096];
    int indx = 0;
    int port = 0;

    if (URL == NULL)
	return(-1);
    if (ctxt == NULL)
	return(-1);
    if (ctxt->protocol == NULL)
	return(-1);
    if (ctxt->hostname == NULL)
	return(-1);
    buf[indx] = 0;
    while (*cur != 0) {
        if ((cur[0] == ':') && (cur[1] == '/') && (cur[2] == '/')) {
	    buf[indx] = 0;
	    if (strcmp(ctxt->protocol, buf))
		return(-1);
	    indx = 0;
            cur += 3;
	    break;
	}
	buf[indx++] = *cur++;
    }
    if (*cur == 0)
	return(-1);

    buf[indx] = 0;
    while (1) {
        if (cur[0] == ':') {
	    buf[indx] = 0;
	    if (strcmp(ctxt->hostname, buf))
		return(-1);
	    indx = 0;
	    cur += 1;
	    while ((*cur >= '0') && (*cur <= '9')) {
	        port *= 10;
		port += *cur - '0';
		cur++;
	    }
	    if (port != ctxt->port)
		return(-1);
	    while ((cur[0] != '/') && (*cur != 0)) 
	        cur++;
	    break;
	}
        if ((*cur == '/') || (*cur == 0)) {
	    buf[indx] = 0;
	    if (strcmp(ctxt->hostname, buf))
		return(-1);
	    indx = 0;
	    break;
	}
	buf[indx++] = *cur++;
    }
    if (ctxt->path != NULL) {
	xmlFree(ctxt->path);
	ctxt->path = NULL;
    }

    if (*cur == 0) 
        ctxt->path = xmlMemStrdup("/");
    else {
        indx = 0;
        buf[indx] = 0;
	while (*cur != 0)
	    buf[indx++] = *cur++;
	buf[indx] = 0;
	ctxt->path = xmlMemStrdup(buf);
    }	
    return(0);
}

/**
 * xmlNanoFTPScanProxy:
 * @URL:  The proxy URL used to initialize the proxy context
 *
 * (Re)Initialize the FTP Proxy context by parsing the URL and finding
 * the protocol host port it indicates.
 * Should be like ftp://myproxy/ or ftp://myproxy:3128/
 * A NULL URL cleans up proxy informations.
 */

void
xmlNanoFTPScanProxy(const char *URL) {
    const char *cur = URL;
    char buf[4096];
    int indx = 0;
    int port = 0;

    if (proxy != NULL) { 
        xmlFree(proxy);
	proxy = NULL;
    }
    if (proxyPort != 0) { 
	proxyPort = 0;
    }
#ifdef DEBUG_FTP
    if (URL == NULL)
	xmlGenericError(xmlGenericErrorContext, "Removing FTP proxy info\n");
    else
	xmlGenericError(xmlGenericErrorContext, "Using FTP proxy %s\n", URL);
#endif
    if (URL == NULL) return;
    buf[indx] = 0;
    while (*cur != 0) {
        if ((cur[0] == ':') && (cur[1] == '/') && (cur[2] == '/')) {
	    buf[indx] = 0;
	    indx = 0;
            cur += 3;
	    break;
	}
	buf[indx++] = *cur++;
    }
    if (*cur == 0) return;

    buf[indx] = 0;
    while (1) {
        if (cur[0] == ':') {
	    buf[indx] = 0;
	    proxy = xmlMemStrdup(buf);
	    indx = 0;
	    cur += 1;
	    while ((*cur >= '0') && (*cur <= '9')) {
	        port *= 10;
		port += *cur - '0';
		cur++;
	    }
	    if (port != 0) proxyPort = port;
	    while ((cur[0] != '/') && (*cur != 0)) 
	        cur++;
	    break;
	}
        if ((*cur == '/') || (*cur == 0)) {
	    buf[indx] = 0;
	    proxy = xmlMemStrdup(buf);
	    indx = 0;
	    break;
	}
	buf[indx++] = *cur++;
    }
}

/**
 * xmlNanoFTPNewCtxt:
 * @URL:  The URL used to initialize the context
 *
 * Allocate and initialize a new FTP context.
 *
 * Returns an FTP context or NULL in case of error.
 */

void*
xmlNanoFTPNewCtxt(const char *URL) {
    xmlNanoFTPCtxtPtr ret;

    ret = (xmlNanoFTPCtxtPtr) xmlMalloc(sizeof(xmlNanoFTPCtxt));
    if (ret == NULL) return(NULL);

    memset(ret, 0, sizeof(xmlNanoFTPCtxt));
    ret->port = 21;
    ret->passive = 1;
    ret->returnValue = 0;
    ret->controlBufIndex = 0;
    ret->controlBufUsed = 0;

    if (URL != NULL)
	xmlNanoFTPScanURL(ret, URL);

    return(ret);
}

/**
 * xmlNanoFTPFreeCtxt:
 * @ctx:  an FTP context
 *
 * Frees the context after closing the connection.
 */

void
xmlNanoFTPFreeCtxt(void * ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    if (ctxt == NULL) return;
    if (ctxt->hostname != NULL) xmlFree(ctxt->hostname);
    if (ctxt->protocol != NULL) xmlFree(ctxt->protocol);
    if (ctxt->path != NULL) xmlFree(ctxt->path);
    ctxt->passive = 1;
    if (ctxt->controlFd >= 0) closesocket(ctxt->controlFd);
    ctxt->controlFd = -1;
    ctxt->controlBufIndex = -1;
    ctxt->controlBufUsed = -1;
    xmlFree(ctxt);
}

/**
 * xmlNanoFTPParseResponse:
 * @buf:  the buffer containing the response
 * @len:  the buffer length
 * 
 * Parsing of the server answer, we just extract the code.
 *
 * returns 0 for errors
 *     +XXX for last line of response
 *     -XXX for response to be continued
 */
static int
xmlNanoFTPParseResponse(char *buf, int len) {
    int val = 0;

    if (len < 3) return(-1);
    if ((*buf >= '0') && (*buf <= '9')) 
        val = val * 10 + (*buf - '0');
    else
        return(0);
    buf++;
    if ((*buf >= '0') && (*buf <= '9')) 
        val = val * 10 + (*buf - '0');
    else
        return(0);
    buf++;
    if ((*buf >= '0') && (*buf <= '9')) 
        val = val * 10 + (*buf - '0');
    else
        return(0);
    buf++;
    if (*buf == '-') 
        return(-val);
    return(val);
}

/**
 * xmlNanoFTPGetMore:
 * @ctx:  an FTP context
 *
 * Read more information from the FTP control connection
 * Returns the number of bytes read, < 0 indicates an error
 */
static int
xmlNanoFTPGetMore(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    int len;
    int size;

    if ((ctxt->controlBufIndex < 0) || (ctxt->controlBufIndex > FTP_BUF_SIZE)) {
#ifdef DEBUG_FTP
        xmlGenericError(xmlGenericErrorContext,
		"xmlNanoFTPGetMore : controlBufIndex = %d\n",
		ctxt->controlBufIndex);
#endif
	return(-1);
    }

    if ((ctxt->controlBufUsed < 0) || (ctxt->controlBufUsed > FTP_BUF_SIZE)) {
#ifdef DEBUG_FTP
        xmlGenericError(xmlGenericErrorContext,
		"xmlNanoFTPGetMore : controlBufUsed = %d\n",
		ctxt->controlBufUsed);
#endif
	return(-1);
    }
    if (ctxt->controlBufIndex > ctxt->controlBufUsed) {
#ifdef DEBUG_FTP
        xmlGenericError(xmlGenericErrorContext,
		"xmlNanoFTPGetMore : controlBufIndex > controlBufUsed %d > %d\n",
	       ctxt->controlBufIndex, ctxt->controlBufUsed);
#endif
	return(-1);
    }

    /*
     * First pack the control buffer
     */
    if (ctxt->controlBufIndex > 0) {
	memmove(&ctxt->controlBuf[0], &ctxt->controlBuf[ctxt->controlBufIndex],
		ctxt->controlBufUsed - ctxt->controlBufIndex);
	ctxt->controlBufUsed -= ctxt->controlBufIndex;
	ctxt->controlBufIndex = 0;
    }
    size = FTP_BUF_SIZE - ctxt->controlBufUsed;
    if (size == 0) {
#ifdef DEBUG_FTP
        xmlGenericError(xmlGenericErrorContext,
		"xmlNanoFTPGetMore : buffer full %d \n", ctxt->controlBufUsed);
#endif
	return(0);
    }

    /*
     * Read the amount left on the control connection
     */
    if ((len = recv(ctxt->controlFd, &ctxt->controlBuf[ctxt->controlBufIndex],
		    size, 0)) < 0) {
	closesocket(ctxt->controlFd); ctxt->controlFd = -1;
        ctxt->controlFd = -1;
        return(-1);
    }
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext,
	    "xmlNanoFTPGetMore : read %d [%d - %d]\n", len,
	   ctxt->controlBufUsed, ctxt->controlBufUsed + len);
#endif
    ctxt->controlBufUsed += len;
    ctxt->controlBuf[ctxt->controlBufUsed] = 0;

    return(len);
}

/**
 * xmlNanoFTPReadResponse:
 * @ctx:  an FTP context
 *
 * Read the response from the FTP server after a command.
 * Returns the code number
 */
static int
xmlNanoFTPReadResponse(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char *ptr, *end;
    int len;
    int res = -1, cur = -1;

get_more:
    /*
     * Assumes everything up to controlBuf[controlBufIndex] has been read
     * and analyzed.
     */
    len = xmlNanoFTPGetMore(ctx);
    if (len < 0) {
        return(-1);
    }
    if ((ctxt->controlBufUsed == 0) && (len == 0)) {
        return(-1);
    }
    ptr = &ctxt->controlBuf[ctxt->controlBufIndex];
    end = &ctxt->controlBuf[ctxt->controlBufUsed];

#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext,
	    "\n<<<\n%s\n--\n", ptr);
#endif
    while (ptr < end) {
        cur = xmlNanoFTPParseResponse(ptr, end - ptr);
	if (cur > 0) {
	    /*
	     * Successfully scanned the control code, scratch
	     * till the end of the line, but keep the index to be
	     * able to analyze the result if needed.
	     */
	    res = cur;
	    ptr += 3;
	    ctxt->controlBufAnswer = ptr - ctxt->controlBuf;
	    while ((ptr < end) && (*ptr != '\n')) ptr++;
	    if (*ptr == '\n') ptr++;
	    if (*ptr == '\r') ptr++;
	    break;
	}
	while ((ptr < end) && (*ptr != '\n')) ptr++;
	if (ptr >= end) {
	    ctxt->controlBufIndex = ctxt->controlBufUsed;
	    goto get_more;
	}
	if (*ptr != '\r') ptr++;
    }

    if (res < 0) goto get_more;
    ctxt->controlBufIndex = ptr - ctxt->controlBuf;
#ifdef DEBUG_FTP
    ptr = &ctxt->controlBuf[ctxt->controlBufIndex];
    xmlGenericError(xmlGenericErrorContext, "\n---\n%s\n--\n", ptr);
#endif

#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "Got %d\n", res);
#endif
    return(res / 100);
}

/**
 * xmlNanoFTPGetResponse:
 * @ctx:  an FTP context
 *
 * Get the response from the FTP server after a command.
 * Returns the code number
 */

int
xmlNanoFTPGetResponse(void *ctx) {
    int res;

    res = xmlNanoFTPReadResponse(ctx);

    return(res);
}

/**
 * xmlNanoFTPCheckResponse:
 * @ctx:  an FTP context
 *
 * Check if there is a response from the FTP server after a command.
 * Returns the code number, or 0
 */

int
xmlNanoFTPCheckResponse(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    fd_set rfd;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&rfd);
    FD_SET(ctxt->controlFd, &rfd);
    switch(select(ctxt->controlFd + 1, &rfd, NULL, NULL, &tv)) {
	case 0:
	    return(0);
	case -1:
#ifdef DEBUG_FTP
	    perror("select");
#endif
	    return(-1);
			
    }

    return(xmlNanoFTPReadResponse(ctx));
}

/**
 * Send the user authentification
 */

static int
xmlNanoFTPSendUser(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char buf[200];
    int len;
    int res;

    if (ctxt->user == NULL)
	sprintf(buf, "USER anonymous\r\n");
    else
	snprintf(buf, sizeof(buf), "USER %s\r\n", ctxt->user);
    buf[sizeof(buf) - 1] = 0;
    len = strlen(buf);
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
    res = send(ctxt->controlFd, buf, len, 0);
    if (res < 0) return(res);
    return(0);
}

/**
 * Send the password authentification
 */

static int
xmlNanoFTPSendPasswd(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char buf[200];
    int len;
    int res;

    if (ctxt->passwd == NULL)
	snprintf(buf, sizeof(buf), "PASS anonymous@\r\n");
    else
	snprintf(buf, sizeof(buf), "PASS %s\r\n", ctxt->passwd);
    buf[sizeof(buf) - 1] = 0;
    len = strlen(buf);
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
    res = send(ctxt->controlFd, buf, len, 0);
    if (res < 0) return(res);
    return(0);
}

/**
 * xmlNanoFTPQuit:
 * @ctx:  an FTP context
 *
 * Send a QUIT command to the server
 *
 * Returns -1 in case of error, 0 otherwise
 */


int
xmlNanoFTPQuit(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char buf[200];
    int len;
    int res;

    sprintf(buf, "QUIT\r\n");
    len = strlen(buf);
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "%s", buf); /* Just to be consistent, even though we know it can't have a % in it */
#endif
    res = send(ctxt->controlFd, buf, len, 0);
    return(0);
}

/**
 * xmlNanoFTPConnect:
 * @ctx:  an FTP context
 *
 * Tries to open a control connection
 *
 * Returns -1 in case of error, 0 otherwise
 */

int
xmlNanoFTPConnect(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    struct hostent *hp;
    int port;
    int res;

    if (ctxt == NULL)
	return(-1);
    if (ctxt->hostname == NULL)
	return(-1);

    /*
     * do the blocking DNS query.
     */
    if (proxy)
	hp = gethostbyname(proxy);
    else
	hp = gethostbyname(ctxt->hostname);
    if (hp == NULL)
        return(-1);

    /*
     * Prepare the socket
     */
    memset(&ctxt->ftpAddr, 0, sizeof(ctxt->ftpAddr));
    ctxt->ftpAddr.sin_family = AF_INET;
    memcpy(&ctxt->ftpAddr.sin_addr, hp->h_addr_list[0], hp->h_length);
    if (proxy) {
        port = proxyPort;
    } else {
	port = ctxt->port;
    }
    if (port == 0)
	port = 21;
    ctxt->ftpAddr.sin_port = htons(port);
    ctxt->controlFd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctxt->controlFd < 0)
        return(-1);

    /*
     * Do the connect.
     */
    if (connect(ctxt->controlFd, (struct sockaddr *) &ctxt->ftpAddr,
                sizeof(struct sockaddr_in)) < 0) {
        closesocket(ctxt->controlFd); ctxt->controlFd = -1;
        ctxt->controlFd = -1;
	return(-1);
    }

    /*
     * Wait for the HELLO from the server.
     */
    res = xmlNanoFTPGetResponse(ctxt);
    if (res != 2) {
        closesocket(ctxt->controlFd); ctxt->controlFd = -1;
        ctxt->controlFd = -1;
	return(-1);
    }

    /*
     * State diagram for the login operation on the FTP server
     *
     * Reference: RFC 959
     *
     *                       1
     * +---+   USER    +---+------------->+---+
     * | B |---------->| W | 2       ---->| E |
     * +---+           +---+------  |  -->+---+
     *                  | |       | | |
     *                3 | | 4,5   | | |
     *    --------------   -----  | | |
     *   |                      | | | |
     *   |                      | | | |
     *   |                 ---------  |
     *   |               1|     | |   |
     *   V                |     | |   |
     * +---+   PASS    +---+ 2  |  ------>+---+
     * |   |---------->| W |------------->| S |
     * +---+           +---+   ---------->+---+
     *                  | |   | |     |
     *                3 | |4,5| |     |
     *    --------------   --------   |
     *   |                    | |  |  |
     *   |                    | |  |  |
     *   |                 -----------
     *   |             1,3|   | |  |
     *   V                |  2| |  |
     * +---+   ACCT    +---+--  |   ----->+---+
     * |   |---------->| W | 4,5 -------->| F |
     * +---+           +---+------------->+---+
     *
     * Of course in case of using a proxy this get really nasty and is not
     * standardized at all :-(
     */
    if (proxy) {
        int len;
	char buf[400];

        if (proxyUser != NULL) {
	    /*
	     * We need proxy auth
	     */
	    snprintf(buf, sizeof(buf), "USER %s\r\n", proxyUser);
            buf[sizeof(buf) - 1] = 0;
            len = strlen(buf);
#ifdef DEBUG_FTP
	    xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
	    res = send(ctxt->controlFd, buf, len, 0);
	    if (res < 0) {
		closesocket(ctxt->controlFd);
		ctxt->controlFd = -1;
	        return(res);
	    }
	    res = xmlNanoFTPGetResponse(ctxt);
	    switch (res) {
		case 2:
		    if (proxyPasswd == NULL)
			break;
		case 3:
		    if (proxyPasswd != NULL)
			snprintf(buf, sizeof(buf), "PASS %s\r\n", proxyPasswd);
		    else
			snprintf(buf, sizeof(buf), "PASS anonymous@\r\n");
                    buf[sizeof(buf) - 1] = 0;
                    len = strlen(buf);
#ifdef DEBUG_FTP
		    xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
		    res = send(ctxt->controlFd, buf, len, 0);
		    if (res < 0) {
			closesocket(ctxt->controlFd);
			ctxt->controlFd = -1;
			return(res);
		    }
		    res = xmlNanoFTPGetResponse(ctxt);
		    if (res > 3) {
			closesocket(ctxt->controlFd);
			ctxt->controlFd = -1;
			return(-1);
		    }
		    break;
		case 1:
		    break;
		case 4:
		case 5:
		case -1:
		default:
		    closesocket(ctxt->controlFd);
		    ctxt->controlFd = -1;
		    return(-1);
	    }
	}

	/*
	 * We assume we don't need more authentication to the proxy
	 * and that it succeeded :-\
	 */
	switch (proxyType) {
	    case 0:
		/* we will try in seqence */
	    case 1:
		/* Using SITE command */
		snprintf(buf, sizeof(buf), "SITE %s\r\n", ctxt->hostname);
                buf[sizeof(buf) - 1] = 0;
                len = strlen(buf);
#ifdef DEBUG_FTP
		xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
		res = send(ctxt->controlFd, buf, len, 0);
		if (res < 0) {
		    closesocket(ctxt->controlFd); ctxt->controlFd = -1;
		    ctxt->controlFd = -1;
		    return(res);
		}
		res = xmlNanoFTPGetResponse(ctxt);
		if (res == 2) {
		    /* we assume it worked :-\ 1 is error for SITE command */
		    proxyType = 1;
		    break;
		}    
		if (proxyType == 1) {
		    closesocket(ctxt->controlFd); ctxt->controlFd = -1;
		    ctxt->controlFd = -1;
		    return(-1);
		}
	    case 2:
		/* USER user@host command */
		if (ctxt->user == NULL)
		    snprintf(buf, sizeof(buf), "USER anonymous@%s\r\n",
			           ctxt->hostname);
		else
		    snprintf(buf, sizeof(buf), "USER %s@%s\r\n",
			           ctxt->user, ctxt->hostname);
                buf[sizeof(buf) - 1] = 0;
                len = strlen(buf);
#ifdef DEBUG_FTP
		xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
		res = send(ctxt->controlFd, buf, len, 0);
		if (res < 0) {
		    closesocket(ctxt->controlFd); ctxt->controlFd = -1;
		    ctxt->controlFd = -1;
		    return(res);
		}
		res = xmlNanoFTPGetResponse(ctxt);
		if ((res == 1) || (res == 2)) {
		    /* we assume it worked :-\ */
		    proxyType = 2;
		    return(0);
		}    
		if (ctxt->passwd == NULL)
		    snprintf(buf, sizeof(buf), "PASS anonymous@\r\n");
		else
		    snprintf(buf, sizeof(buf), "PASS %s\r\n", ctxt->passwd);
                buf[sizeof(buf) - 1] = 0;
                len = strlen(buf);
#ifdef DEBUG_FTP
		xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
		res = send(ctxt->controlFd, buf, len, 0);
		if (res < 0) {
		    closesocket(ctxt->controlFd); ctxt->controlFd = -1;
		    ctxt->controlFd = -1;
		    return(res);
		}
		res = xmlNanoFTPGetResponse(ctxt);
		if ((res == 1) || (res == 2)) {
		    /* we assume it worked :-\ */
		    proxyType = 2;
		    return(0);
		}
		if (proxyType == 2) {
		    closesocket(ctxt->controlFd); ctxt->controlFd = -1;
		    ctxt->controlFd = -1;
		    return(-1);
		}
	    case 3:
		/*
		 * If you need support for other Proxy authentication scheme
		 * send the code or at least the sequence in use.
		 */
	    default:
		closesocket(ctxt->controlFd); ctxt->controlFd = -1;
		ctxt->controlFd = -1;
		return(-1);
	}
    }
    /*
     * Non-proxy handling.
     */
    res = xmlNanoFTPSendUser(ctxt);
    if (res < 0) {
        closesocket(ctxt->controlFd); ctxt->controlFd = -1;
        ctxt->controlFd = -1;
	return(-1);
    }
    res = xmlNanoFTPGetResponse(ctxt);
    switch (res) {
	case 2:
	    return(0);
	case 3:
	    break;
	case 1:
	case 4:
	case 5:
        case -1:
	default:
	    closesocket(ctxt->controlFd); ctxt->controlFd = -1;
	    ctxt->controlFd = -1;
	    return(-1);
    }
    res = xmlNanoFTPSendPasswd(ctxt);
    if (res < 0) {
        closesocket(ctxt->controlFd); ctxt->controlFd = -1;
        ctxt->controlFd = -1;
	return(-1);
    }
    res = xmlNanoFTPGetResponse(ctxt);
    switch (res) {
	case 2:
	    break;
	case 3:
	    xmlGenericError(xmlGenericErrorContext,
		    "FTP server asking for ACCNT on anonymous\n");
	case 1:
	case 4:
	case 5:
        case -1:
	default:
	    closesocket(ctxt->controlFd); ctxt->controlFd = -1;
	    ctxt->controlFd = -1;
	    return(-1);
    }

    return(0);
}

/**
 * xmlNanoFTPConnectTo:
 * @server:  an FTP server name
 * @port:  the port (use 21 if 0)
 *
 * Tries to open a control connection to the given server/port
 *
 * Returns an fTP context or NULL if it failed
 */

void*
xmlNanoFTPConnectTo(const char *server, int port) {
    xmlNanoFTPCtxtPtr ctxt;
    int res;

    xmlNanoFTPInit();
    if (server == NULL) 
	return(NULL);
    ctxt = (xmlNanoFTPCtxtPtr) xmlNanoFTPNewCtxt(NULL);
    ctxt->hostname = xmlMemStrdup(server);
    if (port != 0)
	ctxt->port = port;
    res = xmlNanoFTPConnect(ctxt);
    if (res < 0) {
	xmlNanoFTPFreeCtxt(ctxt);
	return(NULL);
    }
    return(ctxt);
}

/**
 * xmlNanoFTPCwd:
 * @ctx:  an FTP context
 * @directory:  a directory on the server
 *
 * Tries to change the remote directory
 *
 * Returns -1 incase of error, 1 if CWD worked, 0 if it failed
 */

int
xmlNanoFTPCwd(void *ctx, char *directory) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char buf[400];
    int len;
    int res;

    /*
     * Expected response code for CWD:
     *
     * CWD
     *     250
     *     500, 501, 502, 421, 530, 550
     */
    snprintf(buf, sizeof(buf), "CWD %s\r\n", directory);
    buf[sizeof(buf) - 1] = 0;
    len = strlen(buf);
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
    res = send(ctxt->controlFd, buf, len, 0);
    if (res < 0) return(res);
    res = xmlNanoFTPGetResponse(ctxt);
    if (res == 4) {
	return(-1);
    }
    if (res == 2) return(1);
    if (res == 5) {
	return(0);
    }
    return(0);
}

/**
 * xmlNanoFTPGetConnection:
 * @ctx:  an FTP context
 *
 * Try to open a data connection to the server. Currently only
 * passive mode is supported.
 *
 * Returns -1 incase of error, 0 otherwise
 */

int
xmlNanoFTPGetConnection(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char buf[200], *cur;
    int len, i;
    int res;
    unsigned char ad[6], *adp, *portp;
    unsigned int temp[6];
    struct sockaddr_in dataAddr;
    SOCKLEN_T dataAddrLen;

    ctxt->dataFd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctxt->dataFd < 0) {
        xmlGenericError(xmlGenericErrorContext,
		"xmlNanoFTPGetConnection: failed to create socket\n");
	return(-1);
    }
    dataAddrLen = sizeof(dataAddr);
    memset(&dataAddr, 0, dataAddrLen);
    dataAddr.sin_family = AF_INET;

    if (ctxt->passive) {
	sprintf(buf, "PASV\r\n");
        len = strlen(buf);
#ifdef DEBUG_FTP
	xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
	res = send(ctxt->controlFd, buf, len, 0);
	if (res < 0) {
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return(res);
	}
        res = xmlNanoFTPReadResponse(ctx);
	if (res != 2) {
	    if (res == 5) {
	        closesocket(ctxt->dataFd); ctxt->dataFd = -1;
		return(-1);
	    } else {
		/*
		 * retry with an active connection
		 */
	        closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	        ctxt->passive = 0;
	    }
	}
	cur = &ctxt->controlBuf[ctxt->controlBufAnswer]; 
	while (((*cur < '0') || (*cur > '9')) && *cur != '\0') cur++;
	if (sscanf(cur, "%u,%u,%u,%u,%u,%u", &temp[0], &temp[1], &temp[2],
	            &temp[3], &temp[4], &temp[5]) != 6) {
	    xmlGenericError(xmlGenericErrorContext,
		    "Invalid answer to PASV\n");
	    if (ctxt->dataFd != -1) {
		closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    }
	    return(-1);
	}
	for (i=0; i<6; i++) ad[i] = (unsigned char) (temp[i] & 0xff);
	memcpy(&dataAddr.sin_addr, &ad[0], 4);
	memcpy(&dataAddr.sin_port, &ad[4], 2);
	if (connect(ctxt->dataFd, (struct sockaddr *) &dataAddr, dataAddrLen) < 0) {
	    xmlGenericError(xmlGenericErrorContext,
		    "Failed to create a data connection\n");
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return (-1);
	}
    } else {
        getsockname(ctxt->dataFd, (struct sockaddr *) &dataAddr, &dataAddrLen);
	dataAddr.sin_port = 0;
	if (bind(ctxt->dataFd, (struct sockaddr *) &dataAddr, dataAddrLen) < 0) {
	    xmlGenericError(xmlGenericErrorContext,
		    "Failed to bind a port\n");
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return (-1);
	}
        getsockname(ctxt->dataFd, (struct sockaddr *) &dataAddr, &dataAddrLen);

	if (listen(ctxt->dataFd, 1) < 0) {
	    xmlGenericError(xmlGenericErrorContext,
		    "Could not listen on port %d\n",
	            ntohs(dataAddr.sin_port));
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return (-1);
	}
	adp = (unsigned char *) &dataAddr.sin_addr;
	portp = (unsigned char *) &dataAddr.sin_port;
	snprintf(buf, sizeof(buf), "PORT %d,%d,%d,%d,%d,%d\r\n",
	       adp[0] & 0xff, adp[1] & 0xff, adp[2] & 0xff, adp[3] & 0xff,
	       portp[0] & 0xff, portp[1] & 0xff);
        buf[sizeof(buf) - 1] = 0;
        len = strlen(buf);
#ifdef DEBUG_FTP
	xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif

	res = send(ctxt->controlFd, buf, len, 0);
	if (res < 0) {
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return(res);
	}
        res = xmlNanoFTPGetResponse(ctxt);
	if (res != 2) {
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return(-1);
        }
    }
    return(ctxt->dataFd);
    
}

/**
 * xmlNanoFTPCloseConnection:
 * @ctx:  an FTP context
 *
 * Close the data connection from the server
 *
 * Returns -1 incase of error, 0 otherwise
 */

int
xmlNanoFTPCloseConnection(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    int res;
    fd_set rfd, efd;
    struct timeval tv;

    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
    tv.tv_sec = 15;
    tv.tv_usec = 0;
    FD_ZERO(&rfd);
    FD_SET(ctxt->controlFd, &rfd);
    FD_ZERO(&efd);
    FD_SET(ctxt->controlFd, &efd);
    res = select(ctxt->controlFd + 1, &rfd, NULL, &efd, &tv);
    if (res < 0) {
#ifdef DEBUG_FTP
	perror("select");
#endif
	closesocket(ctxt->controlFd); ctxt->controlFd = -1;
	return(-1);
    }
    if (res == 0) {
#ifdef DEBUG_FTP
	xmlGenericError(xmlGenericErrorContext,
		"xmlNanoFTPCloseConnection: timeout\n");
#endif
	closesocket(ctxt->controlFd); ctxt->controlFd = -1;
    } else {
	res = xmlNanoFTPGetResponse(ctxt);
	if (res != 2) {
	    closesocket(ctxt->controlFd); ctxt->controlFd = -1;
	    return(-1);
	}
    }
    return(0);
}

/**
 * xmlNanoFTPParseList:
 * @list:  some data listing received from the server
 * @callback:  the user callback
 * @userData:  the user callback data
 *
 * Parse at most one entry from the listing. 
 *
 * Returns -1 incase of error, the length of data parsed otherwise
 */

static int
xmlNanoFTPParseList(const char *list, ftpListCallback callback, void *userData) {
    const char *cur = list;
    char filename[151];
    char attrib[11];
    char owner[11];
    char group[11];
    char month[4];
    int year = 0;
    int minute = 0;
    int hour = 0;
    int day = 0;
    unsigned long size = 0;
    int links = 0;
    int i;

    if (!strncmp(cur, "total", 5)) {
        cur += 5;
	while (*cur == ' ') cur++;
	while ((*cur >= '0') && (*cur <= '9'))
	    links = (links * 10) + (*cur++ - '0');
	while ((*cur == ' ') || (*cur == '\n')  || (*cur == '\r'))
	    cur++;
	return(cur - list);
    } else if (*list == '+') {
	return(0);
    } else {
	while ((*cur == ' ') || (*cur == '\n')  || (*cur == '\r'))
	    cur++;
	if (*cur == 0) return(0);
	i = 0;
	while (*cur != ' ') {
	    if (i < 10) 
		attrib[i++] = *cur;
	    cur++;
	    if (*cur == 0) return(0);
	}
	attrib[10] = 0;
	while (*cur == ' ') cur++;
	if (*cur == 0) return(0);
	while ((*cur >= '0') && (*cur <= '9'))
	    links = (links * 10) + (*cur++ - '0');
	while (*cur == ' ') cur++;
	if (*cur == 0) return(0);
	i = 0;
	while (*cur != ' ') {
	    if (i < 10) 
		owner[i++] = *cur;
	    cur++;
	    if (*cur == 0) return(0);
	}
	owner[i] = 0;
	while (*cur == ' ') cur++;
	if (*cur == 0) return(0);
	i = 0;
	while (*cur != ' ') {
	    if (i < 10) 
		group[i++] = *cur;
	    cur++;
	    if (*cur == 0) return(0);
	}
	group[i] = 0;
	while (*cur == ' ') cur++;
	if (*cur == 0) return(0);
	while ((*cur >= '0') && (*cur <= '9'))
	    size = (size * 10) + (*cur++ - '0');
	while (*cur == ' ') cur++;
	if (*cur == 0) return(0);
	i = 0;
	while (*cur != ' ') {
	    if (i < 3)
		month[i++] = *cur;
	    cur++;
	    if (*cur == 0) return(0);
	}
	month[i] = 0;
	while (*cur == ' ') cur++;
	if (*cur == 0) return(0);
        while ((*cur >= '0') && (*cur <= '9'))
	    day = (day * 10) + (*cur++ - '0');
	while (*cur == ' ') cur++;
	if (*cur == 0) return(0);
	if ((cur[1] == 0) || (cur[2] == 0)) return(0);
	if ((cur[1] == ':') || (cur[2] == ':')) {
	    while ((*cur >= '0') && (*cur <= '9'))
		hour = (hour * 10) + (*cur++ - '0');
	    if (*cur == ':') cur++;
	    while ((*cur >= '0') && (*cur <= '9'))
		minute = (minute * 10) + (*cur++ - '0');
	} else {
	    while ((*cur >= '0') && (*cur <= '9'))
		year = (year * 10) + (*cur++ - '0');
	}
	while (*cur == ' ') cur++;
	if (*cur == 0) return(0);
	i = 0;
	while ((*cur != '\n')  && (*cur != '\r')) {
	    if (i < 150)
		filename[i++] = *cur;
	    cur++;
	    if (*cur == 0) return(0);
	}
	filename[i] = 0;
	if ((*cur != '\n') && (*cur != '\r'))
	    return(0);
	while ((*cur == '\n')  || (*cur == '\r'))
	    cur++;
    }
    if (callback != NULL) {
        callback(userData, filename, attrib, owner, group, size, links,
		 year, month, day, hour, minute);
    }
    return(cur - list);
}

/**
 * xmlNanoFTPList:
 * @ctx:  an FTP context
 * @callback:  the user callback
 * @userData:  the user callback data
 * @filename:  optional files to list
 *
 * Do a listing on the server. All files info are passed back
 * in the callbacks.
 *
 * Returns -1 incase of error, 0 otherwise
 */

int
xmlNanoFTPList(void *ctx, ftpListCallback callback, void *userData,
	       char *filename) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char buf[4096 + 1];
    int len, res;
    int indx = 0, base;
    fd_set rfd, efd;
    struct timeval tv;

    if (filename == NULL) {
        if (xmlNanoFTPCwd(ctxt, ctxt->path) < 1)
	    return(-1);
	ctxt->dataFd = xmlNanoFTPGetConnection(ctxt);
	if (ctxt->dataFd == -1)
	    return(-1);
	sprintf(buf, "LIST -L\r\n");
    } else {
	if (filename[0] != '/') {
	    if (xmlNanoFTPCwd(ctxt, ctxt->path) < 1)
		return(-1);
	}
	ctxt->dataFd = xmlNanoFTPGetConnection(ctxt);
	if (ctxt->dataFd == -1)
	    return(-1);
	snprintf(buf, sizeof(buf), "LIST -L %s\r\n", filename);
    }
    buf[sizeof(buf) - 1] = 0;
    len = strlen(buf);
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
    res = send(ctxt->controlFd, buf, len, 0);
    if (res < 0) {
	closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	return(res);
    }
    res = xmlNanoFTPReadResponse(ctxt);
    if (res != 1) {
	closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	return(-res);
    }

    do {
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	FD_ZERO(&rfd);
	FD_SET(ctxt->dataFd, &rfd);
	FD_ZERO(&efd);
	FD_SET(ctxt->dataFd, &efd);
	res = select(ctxt->dataFd + 1, &rfd, NULL, &efd, &tv);
	if (res < 0) {
#ifdef DEBUG_FTP
	    perror("select");
#endif
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return(-1);
	}
	if (res == 0) {
	    res = xmlNanoFTPCheckResponse(ctxt);
	    if (res < 0) {
		closesocket(ctxt->dataFd); ctxt->dataFd = -1;
		ctxt->dataFd = -1;
		return(-1);
	    }
	    if (res == 2) {
		closesocket(ctxt->dataFd); ctxt->dataFd = -1;
		return(0);
	    }

	    continue;
	}

	if ((len = recv(ctxt->dataFd, &buf[indx], sizeof(buf) - (indx + 1), 0)) < 0) {
#ifdef DEBUG_FTP
	    perror("recv");
#endif
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    ctxt->dataFd = -1;
	    return(-1);
	}
#ifdef DEBUG_FTP
        write(1, &buf[indx], len);
#endif
	indx += len;
	buf[indx] = 0;
	base = 0;
	do {
	    res = xmlNanoFTPParseList(&buf[base], callback, userData);
	    base += res;
	} while (res > 0);

	memmove(&buf[0], &buf[base], indx - base);
	indx -= base;
    } while (len != 0);
    xmlNanoFTPCloseConnection(ctxt);
    return(0);
}

/**
 * xmlNanoFTPGetSocket:
 * @ctx:  an FTP context
 * @filename:  the file to retrieve (or NULL if path is in context).
 *
 * Initiate fetch of the given file from the server.
 *
 * Returns the socket for the data connection, or <0 in case of error
 */


int
xmlNanoFTPGetSocket(void *ctx, const char *filename) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char buf[300];
    int res, len;
    if ((filename == NULL) && (ctxt->path == NULL))
	return(-1);
    ctxt->dataFd = xmlNanoFTPGetConnection(ctxt);
    if (ctxt->dataFd == -1)
	return(-1);

    sprintf(buf, "TYPE I\r\n");
    len = strlen(buf);
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
    res = send(ctxt->controlFd, buf, len, 0);
    if (res < 0) {
	closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	return(res);
    }
    res = xmlNanoFTPReadResponse(ctxt);
    if (res != 2) {
	closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	return(-res);
    }
    if (filename == NULL)
	snprintf(buf, sizeof(buf), "RETR %s\r\n", ctxt->path);
    else
	snprintf(buf, sizeof(buf), "RETR %s\r\n", filename);
    buf[sizeof(buf) - 1] = 0;
    len = strlen(buf);
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "%s", buf);
#endif
    res = send(ctxt->controlFd, buf, len, 0);
    if (res < 0) {
	closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	return(res);
    }
    res = xmlNanoFTPReadResponse(ctxt);
    if (res != 1) {
	closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	return(-res);
    }
    return(ctxt->dataFd);
}

/**
 * xmlNanoFTPGet:
 * @ctx:  an FTP context
 * @callback:  the user callback
 * @userData:  the user callback data
 * @filename:  the file to retrieve
 *
 * Fetch the given file from the server. All data are passed back
 * in the callbacks. The last callback has a size of 0 block.
 *
 * Returns -1 incase of error, 0 otherwise
 */

int
xmlNanoFTPGet(void *ctx, ftpDataCallback callback, void *userData,
	      const char *filename) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;
    char buf[4096];
    int len = 0, res;
    fd_set rfd;
    struct timeval tv;

    if ((filename == NULL) && (ctxt->path == NULL))
	return(-1);
    if (callback == NULL)
	return(-1);
    if (xmlNanoFTPGetSocket(ctxt, filename) < 0)
	return(-1);

    do {
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	FD_ZERO(&rfd);
	FD_SET(ctxt->dataFd, &rfd);
	res = select(ctxt->dataFd + 1, &rfd, NULL, NULL, &tv);
	if (res < 0) {
#ifdef DEBUG_FTP
	    perror("select");
#endif
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return(-1);
	}
	if (res == 0) {
	    res = xmlNanoFTPCheckResponse(ctxt);
	    if (res < 0) {
		closesocket(ctxt->dataFd); ctxt->dataFd = -1;
		ctxt->dataFd = -1;
		return(-1);
	    }
	    if (res == 2) {
		closesocket(ctxt->dataFd); ctxt->dataFd = -1;
		return(0);
	    }

	    continue;
	}
	if ((len = recv(ctxt->dataFd, buf, sizeof(buf), 0)) < 0) {
	    callback(userData, buf, len);
	    closesocket(ctxt->dataFd); ctxt->dataFd = -1;
	    return(-1);
	}
	callback(userData, buf, len);
    } while (len != 0);

    return(xmlNanoFTPCloseConnection(ctxt));
}

/**
 * xmlNanoFTPRead:
 * @ctx:  the FTP context
 * @dest:  a buffer
 * @len:  the buffer length
 *
 * This function tries to read @len bytes from the existing FTP connection
 * and saves them in @dest. This is a blocking call.
 *
 * Returns the number of byte read. 0 is an indication of an end of connection.
 *         -1 indicates a parameter error.
 */
int
xmlNanoFTPRead(void *ctx, void *dest, int len) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;

    if (ctx == NULL) return(-1);
    if (ctxt->dataFd < 0) return(0);
    if (dest == NULL) return(-1);
    if (len <= 0) return(0);

    len = recv(ctxt->dataFd, dest, len, 0);
#ifdef DEBUG_FTP
    xmlGenericError(xmlGenericErrorContext, "Recvd %d bytes\n", len);
#endif
    if (len <= 0) {
	xmlNanoFTPCloseConnection(ctxt);
    }
    return(len);
}

/**
 * xmlNanoFTPOpen:
 * @URL: the URL to the resource
 *
 * Start to fetch the given ftp:// resource
 *
 * Returns an FTP context, or NULL 
 */

void*
xmlNanoFTPOpen(const char *URL) {
    xmlNanoFTPCtxtPtr ctxt;
    int sock;

    xmlNanoFTPInit();
    if (URL == NULL) return(NULL);
    if (strncmp("ftp://", URL, 6)) return(NULL);

    ctxt = (xmlNanoFTPCtxtPtr) xmlNanoFTPNewCtxt(URL);
    if (ctxt == NULL) return(NULL);
    if (xmlNanoFTPConnect(ctxt) < 0) {
	xmlNanoFTPFreeCtxt(ctxt);
	return(NULL);
    }
    sock = xmlNanoFTPGetSocket(ctxt, ctxt->path);
    if (sock < 0) {
	xmlNanoFTPFreeCtxt(ctxt);
	return(NULL);
    }
    return(ctxt);
}

/**
 * xmlNanoFTPClose:
 * @ctx: an FTP context
 *
 * Close the connection and both control and transport
 *
 * Returns -1 incase of error, 0 otherwise
 */

int
xmlNanoFTPClose(void *ctx) {
    xmlNanoFTPCtxtPtr ctxt = (xmlNanoFTPCtxtPtr) ctx;

    if (ctxt == NULL)
	return(-1);

    if (ctxt->dataFd >= 0) {
	closesocket(ctxt->dataFd);
	ctxt->dataFd = -1;
    }
    if (ctxt->controlFd >= 0) {
	xmlNanoFTPQuit(ctxt);
	closesocket(ctxt->controlFd);
	ctxt->controlFd = -1;
    }
    xmlNanoFTPFreeCtxt(ctxt);
    return(0);
}

#ifdef STANDALONE
/************************************************************************
 * 									*
 * 			Basic test in Standalone mode			*
 * 									*
 ************************************************************************/
void ftpList(void *userData, const char *filename, const char* attrib,
	     const char *owner, const char *group, unsigned long size, int links,
	     int year, const char *month, int day, int hour, int minute) {
    xmlGenericError(xmlGenericErrorContext,
	    "%s %s %s %ld %s\n", attrib, owner, group, size, filename);
}
void ftpData(void *userData, const char *data, int len) {
    if (userData == NULL) return;
    if (len <= 0) {
	fclose(userData);
	return;
    }	
    fwrite(data, len, 1, userData);
}

int main(int argc, char **argv) {
    void *ctxt;
    FILE *output;
    char *tstfile = NULL;

    xmlNanoFTPInit();
    if (argc > 1) {
	ctxt = xmlNanoFTPNewCtxt(argv[1]);
	if (xmlNanoFTPConnect(ctxt) < 0) {
	    xmlGenericError(xmlGenericErrorContext,
		    "Couldn't connect to %s\n", argv[1]);
	    exit(1);
	}
	if (argc > 2)
	    tstfile = argv[2];
    } else
	ctxt = xmlNanoFTPConnectTo("localhost", 0);
    if (ctxt == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"Couldn't connect to localhost\n");
        exit(1);
    }
    xmlNanoFTPList(ctxt, ftpList, NULL, tstfile);
    output = fopen("/tmp/tstdata", "w");
    if (output != NULL) {
	if (xmlNanoFTPGet(ctxt, ftpData, (void *) output, tstfile) < 0)
	    xmlGenericError(xmlGenericErrorContext,
		    "Failed to get file\n");
	
    }
    xmlNanoFTPClose(ctxt);
    xmlMemoryDump();
    exit(0);
}
#endif /* STANDALONE */
#else /* !LIBXML_FTP_ENABLED */
#ifdef STANDALONE
#include <stdio.h>
int main(int argc, char **argv) {
    xmlGenericError(xmlGenericErrorContext,
	    "%s : FTP support not compiled in\n", argv[0]);
    return(0);
}
#endif /* STANDALONE */
#endif /* LIBXML_FTP_ENABLED */
