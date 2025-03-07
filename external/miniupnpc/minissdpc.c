
 /* LICENCE file provided */
/*#include <syslog.h>*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#if defined(_WIN32) || defined(__amigaos__) || defined(__amigaos4__)
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <winsock.h>
#include <stdint.h>
#endif
#if defined(__amigaos__) || defined(__amigaos4__)
#include <sys/socket.h>
#endif
#if defined(__amigaos__)
#define uint16_t unsigned short
#endif
/* Hack */
#define UNIX_PATH_LEN   108
struct sockaddr_un {
  uint16_t sun_family;
  char     sun_path[UNIX_PATH_LEN];
};
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include "minissdpc.h"
#include "miniupnpc.h"

#include "codelength.h"

struct UPNPDev *
getDevicesFromMiniSSDPD(const char * devtype, const char * socketpath)
{
	struct UPNPDev * tmp;
	struct UPNPDev * devlist = NULL;
	unsigned char buffer[2048];
	ssize_t n;
	unsigned char * p;
	unsigned char * url;
	unsigned int i;
	unsigned int urlsize, stsize, usnsize, l;
	int s;
	struct sockaddr_un addr;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s < 0)
	{
		perror("socket(unix)");
		return NULL;
	}
	addr.sun_family = AF_UNIX;

	// Assicurarsi che la destinazione sia terminata correttamente
	strncpy(addr.sun_path, socketpath, sizeof(addr.sun_path) - 1);
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

	// Tentativo di connessione
	if(connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0)
	{
		close(s);
		return NULL;
	}
	stsize = strlen(devtype);
	buffer[0] = 1; /* request type 1 : request devices/services by type */
	p = buffer + 1;
	l = stsize;	
	CODELENGTH(l, p);
	if(p + stsize > buffer + sizeof(buffer))
	{
		/* devtype is too long! */
		close(s);
		return NULL;
	}
	memcpy(p, devtype, stsize);
	p += stsize;
	if(write(s, buffer, p - buffer) < 0)
	{
		perror("minissdpc.c: write()");
		close(s);
		return NULL;
	}
	n = read(s, buffer, sizeof(buffer));
	if(n <= 0)
	{
		perror("minissdpc.c: read()");
		close(s);
		return NULL;
	}
	p = buffer + 1;
	for(i = 0; i < buffer[0]; i++)
	{
		if(p + 2 >= buffer + sizeof(buffer))
			break;
		DECODELENGTH(urlsize, p);
		if(p + urlsize + 2 >= buffer + sizeof(buffer))
			break;
		url = p;
		p += urlsize;
		DECODELENGTH(stsize, p);
		if(p + stsize + 2 >= buffer + sizeof(buffer))
			break;
		tmp = (struct UPNPDev *)malloc(sizeof(struct UPNPDev) + urlsize + stsize);
		tmp->pNext = devlist;
		tmp->descURL = tmp->buffer;
		tmp->st = tmp->buffer + 1 + urlsize;
		memcpy(tmp->buffer, url, urlsize);
		tmp->buffer[urlsize] = '\0';
		memcpy(tmp->buffer + urlsize + 1, p, stsize);
		p += stsize;
		tmp->buffer[urlsize + 1 + stsize] = '\0';
		devlist = tmp;

		// Per compatibilità con versioni recenti di MiniSSDPd
		DECODELENGTH(usnsize, p);
		p += usnsize;
		if(p > buffer + sizeof(buffer))
			break;
	}
	close(s);
	return devlist;
}