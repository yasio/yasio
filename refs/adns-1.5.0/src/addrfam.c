/*
 * addrfam.c
 * - address-family specific code
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

#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "internal.h"

/*
 * General address-family operations.
 */

#define SIN(cnst, sa) ((void)(sa)->sa_family, (cnst struct sockaddr_in *)(sa))
#define SIN6(cnst, sa) ((void)(sa)->sa_family, (cnst struct sockaddr_in6 *)(sa))

static void unknown_af(int af) NONRETURNING;
static void unknown_af(int af) {
  fprintf(stderr, "ADNS INTERNAL: unknown address family %d\n", af);
  abort();
}

/*
 * SOCKADDR_IN_IN6(CNST, struct sockaddr *sa, SIN, {
 *     // struct sockaddr_in *const SIN; // implicitly
 *     code for inet;
 *   }, {
 *     // struct sockaddr_in6 *const SIN6; // implicitly
 *     code for inet6;
 *   })
 *
 * SOCKADDR_IN_IN6_PAIR(CNST, struct sockaddr *sa, SINA,
 *                            struct sockaddr *sb, SINB, {
 *     // struct sockaddr_in *const SINA; // implicitly
 *     // struct sockaddr_in *const SINB; // implicitly
 *     code for inet;
 *   },{
 *     // struct sockaddr_in6 *const SINA6; // implicitly
 *     // struct sockaddr_in6 *const SINB6; // implicitly
 *     code for inet6;
 *   });
 *
 * SOCKADDR_IN_IN6_OTHER(CNST, struct sockaddr *sa, SIN, { in }, { in6 }, {
 *     code for other address family
 *   })
 *
 * AF_IN_IN6_OTHER(af, { in }, { in6 }, { other })
 *
 * Executes the first or second block according to the AF in sa.  CNST
 * may be `const' or empty.  For _PAIR, sa and sb must be same AF.
 *
 * All except _OTHER handle unknown AFs with unknown_af.
 *
 * Code blocks may not contain , outside parens.
 */
#define AF_IN_IN6_OTHER(af, for_inet, for_inet6, other)	\
  if ((af) == AF_INET) {					\
    for_inet							\
  } else if ((af) == AF_INET6) {				\
    for_inet6							\
  } else {							\
    other							\
  }
#define SOCKADDR_IN_IN6_OTHER(cnst, sa, sin, for_inet, for_inet6, other) \
  AF_IN_IN6_OTHER((sa)->sa_family, {					\
      cnst struct sockaddr_in *const sin = SIN(cnst,(sa));		\
      for_inet								\
  }, {									\
      cnst struct sockaddr_in6 *const sin##6 = SIN6(cnst,(sa));		\
      for_inet6								\
  },									\
    other								\
  )
#define SOCKADDR_IN_IN6(cnst, sa, sin, for_inet, for_inet6)		\
  SOCKADDR_IN_IN6_OTHER(cnst, sa, sin, for_inet, for_inet6, {	\
      unknown_af((sa)->sa_family);					\
  })
#define SOCKADDR_IN_IN6_PAIR(cnst, sa, sina, sb, sinb, for_inet, for_inet6) \
  do{									\
    assert((sa)->sa_family == (sb)->sa_family);				\
    SOCKADDR_IN_IN6(cnst, sa, sina, {					\
        cnst struct sockaddr_in *const sinb = SIN(cnst,(sb));		\
	for_inet							\
      }, {								\
        cnst struct sockaddr_in6 *const sinb##6 = SIN6(cnst,(sb));	\
	for_inet6							\
    });									\
  }while(0)

int adns__addrs_equal_raw(const struct sockaddr *a,
			 int bf, const void *b) {
  if (a->sa_family != bf) return 0;

  SOCKADDR_IN_IN6(const, a, sin, {
    return sin->sin_addr.s_addr == ((const struct in_addr*)b)->s_addr;
  }, {
    return !memcmp(&sin6->sin6_addr, b, sizeof(struct in6_addr));
  });
}

int adns__addrs_equal(const adns_sockaddr *a, const adns_sockaddr *b) {
  return adns__addrs_equal_raw(&a->sa, b->sa.sa_family,
			       adns__sockaddr_addr(&b->sa));
}

int adns__sockaddrs_equal(const struct sockaddr *sa,
			  const struct sockaddr *sb) {
  if (!adns__addrs_equal_raw(sa, sb->sa_family, adns__sockaddr_addr(sb)))
    return 0;
  SOCKADDR_IN_IN6_PAIR(const, sa, sina, sb, sinb, {
      return sina->sin_port == sinb->sin_port;
    }, {
      return sina6->sin6_port == sinb6->sin6_port &&
             sina6->sin6_scope_id == sinb6->sin6_scope_id;
    });
}

int adns__addr_width(int af) {
  AF_IN_IN6_OTHER(af, {
      return 32;
    }, {
      return 128;
    }, {
      unknown_af(af);
    });
}

void adns__prefix_mask(adns_sockaddr *sa, int len) {
  SOCKADDR_IN_IN6(, &sa->sa, sin, {
      assert(len <= 32);
      sin->sin_addr.s_addr= htonl(!len ? 0 : 0xffffffff << (32-len));
    }, {
      int i= len/8;
      int j= len%8;
      unsigned char *m= sin6->sin6_addr.s6_addr;
      assert(len <= 128);
      memset(m, 0xff, i);
      if (j) m[i++]= (0xff << (8-j)) & 0xff;
      memset(m+i, 0, 16-i);
    });
}

int adns__guess_prefix_length(const adns_sockaddr *sa) {
  SOCKADDR_IN_IN6(const, &sa->sa, sin, {
      unsigned a= (ntohl(sin->sin_addr.s_addr) >> 24) & 0xff;
      if (a < 128) return 8;
      else if (a < 192) return 16;
      else if (a < 224) return 24;
      else return -1;
    }, {
      (void)sin6;
      return 64;
    });
}

int adns__addr_matches(int af, const void *addr,
		       const adns_sockaddr *base, const adns_sockaddr *mask)
{
  if (af != base->sa.sa_family) return 0;
  SOCKADDR_IN_IN6_PAIR(const, &base->sa, sbase, &mask->sa, smask, {
      const struct in_addr *v4 = addr;
      return (v4->s_addr & smask->sin_addr.s_addr)
	== sbase->sin_addr.s_addr;
    }, {
      int i;
      const char *a= addr;
      const char *b= sbase6->sin6_addr.s6_addr;
      const char *m= smask6->sin6_addr.s6_addr;
      for (i = 0; i < 16; i++)
	if ((a[i] & m[i]) != b[i]) return 0;
      return 1;
    });
}

const void *adns__sockaddr_addr(const struct sockaddr *sa) {
  SOCKADDR_IN_IN6(const, sa, sin, {
      return &sin->sin_addr;
    }, {
      return &sin6->sin6_addr;
    });
}

void adns__addr_inject(const void *a, adns_sockaddr *sa) {
  SOCKADDR_IN_IN6( , &sa->sa, sin, {
      memcpy(&sin->sin_addr, a, sizeof(sin->sin_addr));
    }, {
      memcpy(&sin6->sin6_addr, a, sizeof(sin6->sin6_addr));
    });
}

/*
 * addr2text and text2addr
 */

#define ADDRFAM_DEBUG
#ifdef ADDRFAM_DEBUG
static void af_debug_func(const char *fmt, ...) {
  int esave= errno;
  va_list al;
  va_start(al,fmt);
  vfprintf(stderr,fmt,al);
  va_end(al);
  errno= esave;
}
# define af_debug(fmt,...) \
  (af_debug_func("%s: " fmt "\n", __func__, __VA_ARGS__))
#else
# define af_debug(fmt,...) ((void)("" fmt "", __VA_ARGS__))
#endif

static bool addrtext_our_errno(int e) {
  return
    e==EAFNOSUPPORT ||
    e==EINVAL ||
    e==ENOSPC ||
    e==ENOSYS;
}

static bool addrtext_scope_use_ifname(const struct sockaddr *sa) {
  const struct in6_addr *in6= &SIN6(const,sa)->sin6_addr;
  return
    IN6_IS_ADDR_LINKLOCAL(in6) ||
    IN6_IS_ADDR_MC_LINKLOCAL(in6);
}

static int textaddr_check_qf(adns_queryflags flags) {
  if (flags & ~(adns_queryflags)(adns_qf_addrlit_scope_forbid|
				 adns_qf_addrlit_scope_numeric|
				 adns_qf_addrlit_ipv4_quadonly|
				 0x40000000))
    return ENOSYS;
  return 0;
}

int adns_text2addr(const char *text, uint16_t port, adns_queryflags flags,
		   struct sockaddr *sa, socklen_t *salen_io) {
  int r, af;
  char copybuf[INET6_ADDRSTRLEN];
  const char *parse=text;
  const char *scopestr=0;
  socklen_t needlen;
  void *dst;
  uint16_t *portp;

  r= textaddr_check_qf(flags);  if (r) return r;

#define INVAL(how) do{				\
  af_debug("invalid: %s: `%s'", how, text);	\
  return EINVAL;				\
}while(0)

#define AFCORE(INETx,SINx,sinx)			\
    af= AF_##INETx;				\
    dst = &SINx(,sa)->sinx##_addr;		\
    portp = &SINx(,sa)->sinx##_port;		\
    needlen= sizeof(*SINx(,sa));

  if (!strchr(text, ':')) { /* INET */

    AFCORE(INET,SIN,sin);

  } else { /* INET6 */

    AFCORE(INET6,SIN6,sin6);

    const char *percent= strchr(text, '%');
    if (percent) {
      ptrdiff_t lhslen = percent - text;
      if (lhslen >= INET6_ADDRSTRLEN) INVAL("scoped addr lhs too long");
      memcpy(copybuf, text, lhslen);
      copybuf[lhslen]= 0;

      parse= copybuf;
      scopestr= percent+1;

      af_debug("will parse scoped addr `%s' %% `%s'", parse, scopestr);
    }

  }

#undef AFCORE

  if (scopestr && (flags & adns_qf_addrlit_scope_forbid))
    INVAL("scoped addr but _scope_forbid");

  if (*salen_io < needlen) {
    *salen_io = needlen;
    return ENOSPC;
  }

  memset(sa, 0, needlen);

  sa->sa_family= af;
  *portp = htons(port);

  if (af == AF_INET && !(flags & adns_qf_addrlit_ipv4_quadonly)) {
    /* we have to use inet_aton to deal with non-dotted-quad literals */
    int r= inet_aton(parse,&SIN(,sa)->sin_addr);
    if (!r) INVAL("inet_aton rejected");
  } else {
    int r= inet_pton(af,parse,dst);
    if (!r) INVAL("inet_pton rejected");
    assert(r>0);
  }

  if (scopestr) {
    errno=0;
    char *ep;
    unsigned long scope= strtoul(scopestr,&ep,10);
    if (errno==ERANGE) INVAL("numeric scope id too large for unsigned long");
    assert(!errno);
    if (!*ep) {
      if (scope > ~(uint32_t)0)
	INVAL("numeric scope id too large for uint32_t");
    } else { /* !!*ep */
      if (flags & adns_qf_addrlit_scope_numeric)
	INVAL("non-numeric scope but _scope_numeric");
      if (!addrtext_scope_use_ifname(sa)) {
	af_debug("cannot convert non-numeric scope"
		 " in non-link-local addr `%s'", text);
	return ENOSYS;
      }
      errno= 0;
      scope= if_nametoindex(scopestr);
      if (!scope) {
	/* RFC3493 says "No errors are defined".  It's not clear
	 * whether that is supposed to mean if_nametoindex "can't
	 * fail" (other than by the supplied name not being that of an
	 * interface) which seems unrealistic, or that it conflates
	 * all its errors together by failing to set errno, or simply
	 * that they didn't bother to document the errors.
	 *
	 * glibc, FreeBSD and OpenBSD all set errno (to ENXIO when
	 * appropriate).  See Debian bug #749349.
	 *
	 * We attempt to deal with this by clearing errno to start
	 * with, and then perhaps mapping the results. */
	af_debug("if_nametoindex rejected scope name (errno=%s)",
		 strerror(errno));
	if (errno==0) {
	  return ENXIO;
	} else if (addrtext_our_errno(errno)) {
	  /* we use these for other purposes, urgh. */
	  perror("adns: adns_text2addr: if_nametoindex"
		 " failed with unexpected error");
	  return EIO;
	} else {
	  return errno;
	}
      } else { /* ix>0 */
	if (scope > ~(uint32_t)0) {
	  fprintf(stderr,"adns: adns_text2addr: if_nametoindex"
		  " returned an interface index >=2^32 which will not fit"
		  " in sockaddr_in6.sin6_scope_id");
	  return EIO;
	}
      }
    } /* else; !!*ep */

    SIN6(,sa)->sin6_scope_id= scope;
  } /* if (scopestr) */

  *salen_io = needlen;
  return 0;
}

int adns_addr2text(const struct sockaddr *sa, adns_queryflags flags,
		   char *buffer, int *buflen_io, int *port_r) {
  const void *src;
  int r, port;

  r= textaddr_check_qf(flags);  if (r) return r;

  if (*buflen_io < ADNS_ADDR2TEXT_BUFLEN) {
    *buflen_io = ADNS_ADDR2TEXT_BUFLEN;
    return ENOSPC;
  }

  SOCKADDR_IN_IN6_OTHER(const, sa, sin, {
      src= &sin->sin_addr;    port= sin->sin_port;
    }, {
      src= &sin6->sin6_addr;  port= sin6->sin6_port;
    }, {
      return EAFNOSUPPORT;
    });

  const char *ok= inet_ntop(sa->sa_family, src, buffer, *buflen_io);
  assert(ok);

  if (sa->sa_family == AF_INET6) {
    uint32_t scope = SIN6(const,sa)->sin6_scope_id;
    if (scope) {
      if (flags & adns_qf_addrlit_scope_forbid)
	return EINVAL;
      int scopeoffset = strlen(buffer);
      int remain = *buflen_io - scopeoffset;
      char *scopeptr =  buffer + scopeoffset;
      assert(remain >= IF_NAMESIZE+1/*%*/);
      *scopeptr++= '%'; remain--;
      bool parsedname = 0;
      af_debug("will print scoped addr `%.*s' %% %"PRIu32"",
	       scopeoffset,buffer, scope);
      if (scope <= UINT_MAX /* so we can pass it to if_indextoname */
	  && !(flags & adns_qf_addrlit_scope_numeric)
	  && addrtext_scope_use_ifname(sa)) {
	parsedname = if_indextoname(scope, scopeptr);
	if (!parsedname) {
	  af_debug("if_indextoname rejected scope (errno=%s)",
		   strerror(errno));
	  if (errno==ENXIO) {
	    /* fair enough, show it as a number then */
	  } else if (addrtext_our_errno(errno)) {
	    /* we use these for other purposes, urgh. */
	    perror("adns: adns_addr2text: if_indextoname"
		   " failed with unexpected error");
	    return EIO;
	  } else {
	    return errno;
	  }
	}
      }
      if (!parsedname) {
	int r = snprintf(scopeptr, remain,
			 "%"PRIu32"", scope);
	assert(r < *buflen_io - scopeoffset);
      }
      af_debug("printed scoped addr `%s'", buffer);
    }
  }

  if (port_r) *port_r= ntohs(port);
  return 0;
}

char *adns__sockaddr_ntoa(const struct sockaddr *sa, char *buf) {
  int err;
  int len= ADNS_ADDR2TEXT_BUFLEN;

  err= adns_addr2text(sa, 0, buf, &len, 0);
  if (err == EIO)
    err= adns_addr2text(sa, adns_qf_addrlit_scope_numeric, buf, &len, 0);
  assert(!err);
  return buf;
}

/*
 * Reverse-domain parsing and construction.
 */

int adns__make_reverse_domain(const struct sockaddr *sa, const char *zone,
			      char **buf_io, size_t bufsz,
			      char **buf_free_r) {
  size_t req;
  char *p;
  unsigned c, y;
  unsigned long aa;
  const unsigned char *ap;
  int i, j;

  AF_IN_IN6_OTHER(sa->sa_family, {
      req= 4 * 4;
      if (!zone) zone= "in-addr.arpa";
    }, {
      req = 2 * 32;
      if (!zone) zone= "ip6.arpa";
    }, {
      return ENOSYS;
    });

  req += strlen(zone) + 1;
  if (req <= bufsz)
    p= *buf_io;
  else {
    p= malloc(req); if (!p) return errno;
    *buf_free_r = p;
  }

  *buf_io= p;
  SOCKADDR_IN_IN6(const, sa, sin, {
      aa= ntohl(sin->sin_addr.s_addr);
      for (i=0; i<4; i++) {
	p += sprintf(p, "%d", (int)(aa & 0xff));
	*p++= '.';
	aa >>= 8;
      }
    }, {
      ap= sin6->sin6_addr.s6_addr + 16;
      for (i=0; i<16; i++) {
	c= *--ap;
	for (j=0; j<2; j++) {
	  y= c & 0xf;
	  *p++= (y < 10) ? y + '0' : y - 10 + 'a';
	  c >>= 4;
	  *p++= '.';
	}
      }
    });

  strcpy(p, zone);
  return 0;
}


#define REVPARSE_P_L(labnum)			\
  const char *p= dgram + rps->labstart[labnum];	\
  int l= rps->lablen[labnum]
  /*
   * REVPARSE_P_L(int labnum);
   *   expects:
   *     const char *dgram;
   *     const struct revparse_state *rps;
   *   produces:
   *     const char *p; // start of label labnum in dgram
   *     int l; // length of label in dgram
   */

static bool revparse_check_tail(struct revparse_state *rps,
				const char *dgram, int nlabels,
				int bodylen, const char *inarpa) {
  int i;

  if (nlabels != bodylen+2) return 0;
  for (i=0; i<2; i++) {
    REVPARSE_P_L(bodylen+i);
    const char *want= !i ? inarpa : "arpa";
    if (!adns__labels_equal(p,l, want,strlen(want))) return 0;
  }
  return 1;
}

static bool revparse_atoi(const char *p, int l, int base,
			  unsigned max, unsigned *v_r) {
  if (l>3) return 0;
  if (l>1 && p[0]=='0') return 0;
  unsigned v=0;
  while (l-- > 0) {
    int tv;
    int c= ctype_toupper(*p++);
    if ('0'<=c && c<='9') tv = c-'0';
    else if ('A'<=c && c<='Z') tv = c-'A'+10;
    else return 0;
    if (tv >= base) return 0;
    v *= base;
    v += tv;
  }
  if (v>max) return 0;
  *v_r= v;
  return 1;
}

static bool revparse_inet(struct revparse_state *rps,
			  const char *dgram, int nlabels,
			  adns_rrtype *rrtype_r, adns_sockaddr *addr_r) {
  if (!revparse_check_tail(rps,dgram,nlabels,4,"in-addr")) return 0;

  uint32_t a=0;
  int i;
  for (i=3; i>=0; i--) {
    REVPARSE_P_L(i);
    unsigned v;
    if (!revparse_atoi(p,l,10,255,&v)) return 0;
    a <<= 8;
    a |= v;
  }
  *rrtype_r= adns_r_a;
  addr_r->inet.sin_family= AF_INET;
  addr_r->inet.sin_addr.s_addr= htonl(a);
  return 1;
}

static bool revparse_inet6(struct revparse_state *rps,
			   const char *dgram, int nlabels,
			   adns_rrtype *rrtype_r, adns_sockaddr *addr_r) {
  if (!revparse_check_tail(rps,dgram,nlabels,32,"ip6")) return 0;

  int i, j;
  memset(addr_r,0,sizeof(*addr_r));
  unsigned char *a= addr_r->inet6.sin6_addr.s6_addr+16;
  for (i=0; i<32; ) { /* i incremented in inner loop */
    unsigned b=0;
    for (j=0; j<2; j++, i++) {
      REVPARSE_P_L(i);
      unsigned v;
      if (!revparse_atoi(p,l,16,15,&v)) return 0;
      b >>= 4;
      b |= v << 4;
    }
    *--a= b;
  }
  *rrtype_r= adns_r_aaaa;
  addr_r->inet.sin_family= AF_INET6;
  return 1;
}

bool adns__revparse_label(struct revparse_state *rps, int labnum,
			  const char *dgram, int labstart, int lablen) {
  if (labnum >= MAXREVLABELS)
    return 0;

  assert(labstart <= 65535);
  assert(lablen <= 255);
  rps->labstart[labnum] = labstart;
  rps->lablen[labnum] = lablen;
  return 1;
}

bool adns__revparse_done(struct revparse_state *rps,
			 const char *dgram, int nlabels,
			 adns_rrtype *rrtype_r, adns_sockaddr *addr_r) {
  return
    revparse_inet(rps,dgram,nlabels,rrtype_r,addr_r) ||
    revparse_inet6(rps,dgram,nlabels,rrtype_r,addr_r);
}
