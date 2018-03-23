/*
  some test cases


 ./addrtext_s fe80::1%wlanx
 ./addrtext_s fe80::1%wlan0
 ./addrtext_s fe80::1%23
 ./addrtext_s fe80::1%1
 ./addrtext_s 2001:ba8:1e3::%wlan0
 ./addrtext_s 2001:ba8:1e3::%23
 ./addrtext_s 2001:ba8:1e3::%1   # normally lo
 ./addrtext_s 127.0.0.1x
 ./addrtext_s 172.18.45.6
 ./addrtext_s 12345


  */

/*
 * addrtext.c
 * - test program for address<->string conversion, not part of the library
 */
/*
 *  This file is part of adns, which is
 *    Copyright (C) 1997-2000,2003,2006,2014  Ian Jackson
 *    Copyright (C) 2014  Mark Wooding
 *    Copyright (C) 1999-2000,2003,2006  Tony Finch
 *    Copyright (C) 1991 Massachusetts Institute of Technology
 *  (See the file INSTALL for full details.)
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "config.h"
#include "adns.h"

#define PORT 1234

#define STRING(x) STRING2(x)
#define STRING2(x) #x

static int fails;

static void hex(const void *data_v, int l) {
  const uint8_t *data= data_v;
  int i;
  for (i=0; i<l; i++)
    printf("%02x",data[i]);
}

static void dump(const char *pfx, struct sockaddr *sa, socklen_t salen) {
  int i;
  printf(" %s: ",pfx);
  hex(sa, salen);

  for (i=0; i<salen; i++)
    printf("%02x",((const uint8_t*)sa)[i]);

  printf(" %d ", sa->sa_family);

  switch (sa->sa_family) {
  case AF_INET: {
    const struct sockaddr_in *s = (const void*)sa;
    printf(".port=%d .addr=%08"PRIx32"",
	   ntohs(s->sin_port),
	   (uint32_t)ntohl(s->sin_addr.s_addr));
    break;
  }
  case AF_INET6: {
    const struct sockaddr_in6 *s = (const void*)sa;
    printf(".port=%d .flowinfo=%08"PRIx32" .scope_id=%08"PRIx32" .addr=",
	   ntohs(s->sin6_port),
	   (uint32_t)ntohl(s->sin6_flowinfo),
	   (uint32_t)ntohl(s->sin6_scope_id));
    hex(&s->sin6_addr, sizeof(s->sin6_addr));
    break;
  }
  }
  printf("\n");
}

static void dotest(const char *input) {
  adns_sockaddr ours;
  socklen_t socklen;
  struct addrinfo aip;
  struct addrinfo *air=0;
  char ourbuf[ADNS_ADDR2TEXT_BUFLEN];
  char theirbuf[ADNS_ADDR2TEXT_BUFLEN];

  memset(&ours,0,sizeof(ours));

  socklen= sizeof(ours);
  int our_r= adns_text2addr(input,PORT,0,&ours.sa,&socklen);

  memset(&aip,0,sizeof(aip));
  aip.ai_flags= AI_NUMERICHOST|AI_NUMERICSERV;
  aip.ai_socktype= SOCK_DGRAM;
  aip.ai_protocol= IPPROTO_UDP;
  air= 0;
  int libc_r= getaddrinfo(input,STRING(PORT),&aip,&air);
  printf("`%s': us %s; libc %s, air=%p",
	 input, strerror(our_r), libc_r ? gai_strerror(libc_r) : "0", air);
  if (air)
    printf(" .family=%d .socklen=%ld .addr=%p .next=%p",
	   air->ai_family, (long)air->ai_addrlen, air->ai_addr, air->ai_next);
  printf(":");

  if (libc_r==EAI_NONAME && !air) {
    if (strchr(input,'%') && (our_r==ENOSYS || our_r==ENXIO)) {
      printf(" bad-scope");
      goto ok;
    }
    if (strchr(input,'%') && our_r==ENOSYS) {
      printf(" bad-scope");
      goto ok;
    }
    if (our_r==EINVAL) {
      printf(" invalid");
      goto ok;
    }
  }
  printf(" valid");

#define FAIL do{ printf(" | FAIL\n"); fails++; }while(0)
#define WANT(x) if (!(x)) { printf(" not %s",STRING(x)); FAIL; return; } else;

  WANT(!our_r);
  WANT(!libc_r);
  WANT(air);
  WANT(air->ai_addr);
  WANT(!air->ai_next);
  if (air->ai_addrlen!=socklen || memcmp(&ours,air->ai_addr,socklen)) {
    printf(" mismatch");
    FAIL;
    dump("ours",&ours.sa,socklen);
    dump("libc",air->ai_addr,air->ai_addrlen);
    return;
  }

  printf(" |");

  int ourbuflen= sizeof(ourbuf);
  int ourport;
  our_r= adns_addr2text(&ours.sa,0, ourbuf,&ourbuflen, &ourport);

  printf(" us %s",strerror(our_r));
  if (!our_r)
    printf(" `%s'",ourbuf);

  size_t theirbuflen= sizeof(theirbuf);
  libc_r= getnameinfo(&ours.sa,socklen, theirbuf,theirbuflen, 0,0,
		      NI_NUMERICHOST|NI_NUMERICSERV);
  printf("; libc %s", libc_r ? gai_strerror(libc_r) : "0");
  if (!libc_r)
    printf(" `%s'",theirbuf);

  printf(":");

  WANT(!our_r);
  WANT(!libc_r);
  WANT(ourport==PORT);
  if (strcmp(ourbuf,theirbuf)) {
    printf(" mismatch");
    FAIL;
    return;
  }

 ok:
  printf(" | PASS\n");
}

int main(int argc, char **argv) {
  const char *arg;
  while ((arg= *++argv)) {
    dotest(arg);
  }
  return !!fails;
}
