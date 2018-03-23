/*
 * internal.h
 * - declarations of private objects with external linkage (adns__*)
 * - definitons of internal macros
 * - comments regarding library data structures
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

#ifndef ADNS_INTERNAL_H_INCLUDED
#define ADNS_INTERNAL_H_INCLUDED

#include "config.h"
typedef unsigned char byte;

#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/time.h>

#define ADNS_FEATURE_MANYAF
#include "adns.h"
#include "dlist.h"

#ifdef ADNS_REGRESS_TEST
# include "hredirect.h"
#endif

/* Configuration and constants */

#define MAXSERVERS 5
#define MAXSORTLIST 15
#define UDPMAXRETRIES 15
#define UDPRETRYMS 2000
#define TCPWAITMS 30000
#define TCPCONNMS 14000
#define TCPIDLEMS 30000
#define MAXTTLBELIEVE (7*86400) /* any TTL > 7 days is capped */

#define DNS_PORT 53
#define DNS_MAXUDP 512
#define DNS_MAXLABEL 63
#define DNS_MAXDOMAIN 255
#define DNS_HDRSIZE 12
#define DNS_IDOFFSET 0
#define DNS_CLASS_IN 1

#define MAX_POLLFDS  ADNS_POLLFDS_RECOMMENDED

/* Some preprocessor hackery */

#define GLUE(x, y) GLUE_(x, y)
#define GLUE_(x, y) x##y

/* C99 macro `...' must match at least one argument, so the naive definition
 * `#define CAR(car, ...) car' won't work.  But it's easy to arrange for the
 * tail to be nonempty if we're just going to discard it anyway. */
#define CAR(...) CAR_(__VA_ARGS__, _)
#define CAR_(car, ...) car

/* Extracting the tail of an argument list is rather more difficult.  The
 * following trick is based on one by Laurent Deniau to count the number of
 * arguments to a macro, simplified in two ways: (a) it only handles up to
 * eight arguments, and (b) it only needs to distinguish the one-argument
 * case from many arguments. */
#define CDR(...) CDR_(__VA_ARGS__, m, m, m, m, m, m, m, 1, _)(__VA_ARGS__)
#define CDR_(_1, _2, _3, _4, _5, _6, _7, _8, n, ...) CDR_##n
#define CDR_1(_)
#define CDR_m(_, ...) __VA_ARGS__

typedef enum {
  cc_user,
  cc_entex,
  cc_freq
} consistency_checks;

typedef enum {
  rcode_noerror,
  rcode_formaterror,
  rcode_servfail,
  rcode_nxdomain,
  rcode_notimp,
  rcode_refused
} dns_rcode;

enum {
  adns__qf_addr_answer= 0x01000000,/* addr query received an answer */
  adns__qf_addr_cname = 0x02000000 /* addr subquery performed on cname */
};

/* Shared data structures */

typedef struct {
  int used, avail;
  byte *buf;
} vbuf;

typedef struct {
  adns_state ads;
  adns_query qu;
  int serv;
  const byte *dgram;
  int dglen, nsstart, nscount, arcount;
  struct timeval now;
} parseinfo;

#define MAXREVLABELS 34		/* keep in sync with addrfam! */
struct revparse_state {
  uint16_t labstart[MAXREVLABELS];
  uint8_t lablen[MAXREVLABELS];
};

union checklabel_state {
  struct revparse_state ptr;
};

typedef struct {
  void *ext;
  void (*callback)(adns_query parent, adns_query child);

  union {
    struct {
      adns_rrtype rev_rrtype;
      adns_sockaddr addr;
    } ptr;
    struct {
      unsigned want, have;
    } addr;
  } tinfo; /* type-specific state for the query itself: zero-init if you
	    * don't know better. */

  union {
    adns_rr_hostaddr *hostaddr;
  } pinfo; /* state for use by parent's callback function */
} qcontext;

typedef struct typeinfo {
  adns_rrtype typekey;
  const char *rrtname;
  const char *fmtname;
  int fixed_rrsz;

  void (*makefinal)(adns_query qu, void *data);
  /* Change memory management of *data.
   * Previously, used alloc_interim, now use alloc_final.
   */

  adns_status (*convstring)(vbuf *vb, const void *data);
  /* Converts the RR data to a string representation in vbuf.
   * vbuf will be appended to (it must have been initialised),
   * and will not be null-terminated by convstring.
   */

  adns_status (*parse)(const parseinfo *pai, int cbyte,
		       int max, void *store_r);
  /* Parse one RR, in dgram of length dglen, starting at cbyte and
   * extending until at most max.
   *
   * The RR should be stored at *store_r, of length qu->typei->getrrsz().
   *
   * If there is an overrun which might indicate truncation, it should set
   * *rdstart to -1; otherwise it may set it to anything else positive.
   *
   * nsstart is the offset of the authority section.
   */

  int (*diff_needswap)(adns_state ads,const void *datap_a,const void *datap_b);
  /* Returns !0 if RR a should be strictly after RR b in the sort order,
   * 0 otherwise.  Must not fail.
   */

  adns_status (*checklabel)(adns_state ads, adns_queryflags flags,
			    union checklabel_state *cls, qcontext *ctx,
			    int labnum, const char *dgram,
			    int labstart, int lablen);
  /* Check a label from the query domain string.  The label is not
   * necessarily null-terminated.  The hook can refuse the query's submission
   * by returning a nonzero status.  State can be stored in *cls between
   * calls, and useful information can be stashed in ctx->tinfo, to be stored
   * with the query (e.g., it will be available to the parse hook).  This
   * hook can detect a first call because labnum is zero, and a final call
   * because lablen is zero.
   */

  void (*postsort)(adns_state ads, void *array, int nrrs, int rrsz,
		   const struct typeinfo *typei);
  /* Called immediately after the RRs have been sorted, and may rearrange
   * them.  (This is really for the benefit of SRV's bizarre weighting
   * stuff.)  May be 0 to mean nothing needs to be done.
   */

  int (*getrrsz)(const struct typeinfo *typei, adns_rrtype type);
  /* Return the output resource-record element size; if this is null, then
   * the rrsz member can be used.
   */

  void (*query_send)(adns_query qu, struct timeval now);
  /* Send the query to nameservers, and hook it into the appropriate queue.
   * Normal behaviour is to call adns__query_send, but this can be overridden
   * for special effects.
   */
} typeinfo;

adns_status adns__ckl_hostname(adns_state ads, adns_queryflags flags,
			       union checklabel_state *cls,
			       qcontext *ctx, int labnum,
			       const char *dgram, int labstart, int lablen);
  /* implemented in query.c, used by types.c as default
   * and as part of implementation for some fancier types
   * doesn't require any state */

typedef struct allocnode {
  struct allocnode *next, *back;
  size_t sz;
} allocnode;

union maxalign {
  byte d[1];
  struct in_addr ia;
  long l;
  void *p;
  void (*fp)(void);
  union maxalign *up;
} data;

struct adns__query {
  adns_state ads;
  enum { query_tosend, query_tcpw, query_childw, query_done } state;
  adns_query back, next, parent;
  struct { adns_query head, tail; } children;
  struct { adns_query back, next; } siblings;
  struct { allocnode *head, *tail; } allocations;
  int interim_allocd, preserved_allocd;
  void *final_allocspace;

  const typeinfo *typei;
  byte *query_dgram;
  int query_dglen;

  vbuf vb;
  /* General-purpose messing-about buffer.
   * Wherever a `big' interface is crossed, this may be corrupted/changed
   * unless otherwise specified.
   */

  adns_answer *answer;
  /* This is allocated when a query is submitted, to avoid being unable
   * to relate errors to queries if we run out of memory.  During
   * query processing status, rrs is 0.  cname is set if
   * we found a cname (this corresponds to cname_dgram in the query
   * structure).  type is set from the word go.  nrrs and rrs
   * are set together, when we find how many rrs there are.
   * owner is set during querying unless we're doing searchlist,
   * in which case it is set only when we find an answer.
   */

  byte *cname_dgram;
  int cname_dglen, cname_begin;
  /* If non-0, has been allocated using . */

  vbuf search_vb;
  int search_origlen, search_pos, search_doneabs;
  /* Used by the searching algorithm.  The query domain in textual form
   * is copied into the vbuf, and _origlen set to its length.  Then
   * we walk the searchlist, if we want to.  _pos says where we are
   * (next entry to try), and _doneabs says whether we've done the
   * absolute query yet (0=not yet, 1=done, -1=must do straight away,
   * but not done yet).  If flags doesn't have adns_qf_search then
   * the vbuf is initialised but empty and everything else is zero.
   */

  int id, flags, retries;
  int udpnextserver;
  unsigned long udpsent; /* bitmap indexed by server */
  struct timeval timeout;
  time_t expires; /* Earliest expiry time of any record we used. */

  qcontext ctx;

  /* Possible states:
   *
   *  state   Queue   child  id   nextudpserver  udpsent     tcpfailed
   *
   *  tosend  NONE    null   >=0  0              zero        zero
   *  tosend  udpw    null   >=0  any            nonzero     zero
   *  tosend  NONE    null   >=0  any            nonzero     zero
   *
   *  tcpw    tcpw    null   >=0  irrelevant     any         any
   *
   *  child   childw  set    >=0  irrelevant     irrelevant  irrelevant
   *  child   NONE    null   >=0  irrelevant     irrelevant  irrelevant
   *  done    output  null   -1   irrelevant     irrelevant  irrelevant
   *
   * Queries are only not on a queue when they are actually being processed.
   * Queries in state tcpw/tcpw have been sent (or are in the to-send buffer)
   * iff the tcp connection is in state server_ok.
   *
   * Internal queries (from adns__submit_internal) end up on intdone
   * instead of output, and the callbacks are made on the way out of
   * adns, to avoid reentrancy hazards.
   *
   *			      +------------------------+
   *             START -----> |      tosend/NONE       |
   *			      +------------------------+
   *                         /                       |\  \
   *        too big for UDP /             UDP timeout  \  \ send via UDP
   *        send via TCP   /              more retries  \  \
   *        when conn'd   /                  desired     \  \
   *                     |     	       	       	       	  |  |
   *                     v				  |  v
   *              +-----------+         	    	+-------------+
   *              | tcpw/tcpw | ________                | tosend/udpw |
   *              +-----------+         \	    	+-------------+
   *                 |    |              |     UDP timeout | |
   *                 |    |              |      no more    | |
   *                 |    |              |      retries    | |
   *                  \   | TCP died     |      desired    | |
   *                   \   \ no more     |                 | |
   *                    \   \ servers    | TCP            /  |
   *                     \   \ to try    | timeout       /   |
   *                  got \   \          v             |_    | got
   *                 reply \   _| +------------------+      / reply
   *   	       	       	    \  	  | done/output FAIL |     /
   *                         \    +------------------+    /
   *                          \                          /
   *                           _|                      |_
   *                             (..... got reply ....)
   *                              /                   \
   *        need child query/ies /                     \ no child query
   *                            /                       \
   *                          |_                         _|
   *		   +---------------+		       +----------------+
   *               | childw/childw | ----------------> | done/output OK |
   *               +---------------+  children done    +----------------+
   */
};

struct query_queue { adns_query head, tail; };

#define MAXUDP 2

struct adns__state {
  adns_initflags iflags;
  adns_logcallbackfn *logfn;
  void *logfndata;
  int configerrno;
  struct query_queue udpw, tcpw, childw, output, intdone;
  adns_query forallnext;
  int nextid, tcpsocket;
  struct udpsocket { int af; int fd; } udpsockets[MAXUDP];
  int nudpsockets;
  vbuf tcpsend, tcprecv;
  int nservers, nsortlist, nsearchlist, searchndots, tcpserver, tcprecv_skip;
  enum adns__tcpstate {
    server_disconnected, server_connecting,
    server_ok, server_broken
  } tcpstate;
  struct timeval tcptimeout;
  /* This will have tv_sec==0 if it is not valid.  It will always be
   * valid if tcpstate _connecting.  When _ok, it will be nonzero if
   * we are idle (ie, tcpw queue is empty), in which case it is the
   * absolute time when we will close the connection.
   */
  struct sigaction stdsigpipe;
  sigset_t stdsigmask;
  struct pollfd pollfds_buf[MAX_POLLFDS];
  adns_rr_addr servers[MAXSERVERS];
  struct sortlist {
    adns_sockaddr base, mask;
  } sortlist[MAXSORTLIST];
  char **searchlist;
  unsigned config_report_unknown:1;
  unsigned short rand48xsubi[3];
};

/* From addrfam.c: */

extern int adns__addrs_equal_raw(const struct sockaddr *a,
				 int bf, const void *b);
/* Returns nonzero a's family is bf and a's protocol address field
 * refers to the same protocol address as that stored at ba.
 */

extern int adns__addrs_equal(const adns_sockaddr *a,
			     const adns_sockaddr *b);
/* Returns nonzero if the two refer to the same protocol address
 * (disregarding port, IPv6 scope, etc).
 */

extern int adns__sockaddrs_equal(const struct sockaddr *sa,
				 const struct sockaddr *sb);
/* Return nonzero if the two socket addresses are equal (in all significant
 * respects).
 */

extern int adns__addr_width(int af);
/* Return the width of addresses of family af, in bits. */

extern void adns__prefix_mask(adns_sockaddr *sa, int len);
/* Stores in sa's protocol address field an address mask for address
 * family af, whose first len bits are set and the remainder are
 * clear.  On entry, sa's af field must be set.  This is what you want
 * for converting a prefix length into a netmask.
 */

extern int adns__guess_prefix_length(const adns_sockaddr *addr);
/* Given a network base address, guess the appropriate prefix length based on
 * the appropriate rules for the address family (e.g., for IPv4, this uses
 * the old address classes).
 */

extern int adns__addr_matches(int af, const void *addr,
			      const adns_sockaddr *base,
			      const adns_sockaddr *mask);
/* Return nonzero if the protocol address specified by af and addr
 * lies within the network specified by base and mask.
 */

extern void adns__addr_inject(const void *a, adns_sockaddr *sa);
/* Injects the protocol address *a into the socket adress sa.  Assumes
 * that sa->sa_family is already set correctly.
 */

extern const void *adns__sockaddr_addr(const struct sockaddr *sa);
/* Returns the address of the protocol address field in sa.
 */

char *adns__sockaddr_ntoa(const struct sockaddr *sa, char *buf);
/* Converts sa to a string, and writes it to buf, which must be at
 * least ADNS_ADDR2TEXT_BUFLEN bytes long (unchecked).  Returns buf;
 * can't fail.
 */

extern int adns__make_reverse_domain(const struct sockaddr *sa,
				     const char *zone,
				     char **buf_io, size_t bufsz,
				     char **buf_free_r);
/* Construct a reverse domain string, given a socket address and a parent
 * zone.  If zone is null, then use the standard reverse-lookup zone for the
 * address family.  If the length of the resulting string is no larger than
 * bufsz, then the result is stored starting at *buf_io; otherwise a new
 * buffer is allocated is used, and a pointer to it is stored in both *buf_io
 * and *buf_free_r (the latter of which should be null on entry).  If
 * something goes wrong, then an errno value is returned: ENOSYS if the
 * address family of sa isn't recognized, or ENOMEM if the attempt to
 * allocate an output buffer failed.
 */

extern bool adns__revparse_label(struct revparse_state *rps, int labnum,
				 const char *dgram,
				 int labstart, int lablen);
/* Parse a label in a reverse-domain name, given its index labnum (starting
 * from zero), a pointer to its contents (which need not be null-terminated),
 * and its length.  The state in *rps is initialized implicitly when labnum
 * is zero.
 *
 * Returns 1 if the parse is proceeding successfully, 0 if the domain
 * name is definitely invalid and the parse must be abandoned.
 */

extern bool adns__revparse_done(struct revparse_state *rps,
				const char *dgram, int nlabels,
				adns_rrtype *rrtype_r, adns_sockaddr *addr_r);
/* Finishes parsing a reverse-domain name, given the total number of
 * labels in the name.  On success, fills in the af and protocol
 * address in *addr_r, and the forward query type in *rrtype_r
 * (because that turns out to be useful).  Returns 1 if the parse
 * was successful.
 */

/* From setup.c: */

int adns__setnonblock(adns_state ads, int fd); /* => errno value */

/* From general.c: */

void adns__vlprintf(adns_state ads, const char *fmt, va_list al);
void adns__lprintf(adns_state ads, const char *fmt,
		   ...) PRINTFFORMAT(2,3);

void adns__vdiag(adns_state ads, const char *pfx, adns_initflags prevent,
		 int serv, adns_query qu, const char *fmt, va_list al);

void adns__debug(adns_state ads, int serv, adns_query qu,
		 const char *fmt, ...) PRINTFFORMAT(4,5);
void adns__warn(adns_state ads, int serv, adns_query qu,
		const char *fmt, ...) PRINTFFORMAT(4,5);
void adns__diag(adns_state ads, int serv, adns_query qu,
		const char *fmt, ...) PRINTFFORMAT(4,5);

int adns__vbuf_ensure(vbuf *vb, int want);
int adns__vbuf_appendstr(vbuf *vb, const char *data); /* doesn't include nul */
int adns__vbuf_append(vbuf *vb, const byte *data, int len);
/* 1=>success, 0=>realloc failed */
void adns__vbuf_appendq(vbuf *vb, const byte *data, int len);
void adns__vbuf_init(vbuf *vb);
void adns__vbuf_free(vbuf *vb);

const char *adns__diag_domain(adns_state ads, int serv, adns_query qu,
			      vbuf *vb,
			      const byte *dgram, int dglen, int cbyte);
/* Unpicks a domain in a datagram and returns a string suitable for
 * printing it as.  Never fails - if an error occurs, it will
 * return some kind of string describing the error.
 *
 * serv may be -1 and qu may be 0.  vb must have been initialised,
 * and will be left in an arbitrary consistent state.
 *
 * Returns either vb->buf, or a pointer to a string literal.  Do not modify
 * vb before using the return value.
 */

int adns__getrrsz_default(const typeinfo *typei, adns_rrtype type);
/* Default function for the `getrrsz' type hook; returns the `fixed_rrsz'
 * value from the typeinfo entry.
 */

void adns__isort(void *array, int nobjs, int sz, void *tempbuf,
		 int (*needswap)(void *context, const void *a, const void *b),
		 void *context);
/* Does an insertion sort of array which must contain nobjs objects
 * each sz bytes long.  tempbuf must point to a buffer at least
 * sz bytes long.  needswap should return !0 if a>b (strictly, ie
 * wrong order) 0 if a<=b (ie, order is fine).
 */

void adns__sigpipe_protect(adns_state);
void adns__sigpipe_unprotect(adns_state);
/* If SIGPIPE protection is not disabled, will block all signals except
 * SIGPIPE, and set SIGPIPE's disposition to SIG_IGN.  (And then restore.)
 * Each call to _protect must be followed by a call to _unprotect before
 * any significant amount of code gets to run, since the old signal mask
 * is stored in the adns structure.
 */

/* From transmit.c: */

adns_status adns__mkquery(adns_state ads, vbuf *vb, int *id_r,
			  const char *owner, int ol,
			  const typeinfo *typei, adns_rrtype type,
			  adns_queryflags flags);
/* Assembles a query packet in vb.  A new id is allocated and returned.
 */

adns_status adns__mkquery_frdgram(adns_state ads, vbuf *vb, int *id_r,
				  const byte *qd_dgram, int qd_dglen,
				  int qd_begin,
				  adns_rrtype type, adns_queryflags flags);
/* Same as adns__mkquery, but takes the owner domain from an existing datagram.
 * That domain must be correct and untruncated.
 */

void adns__querysend_tcp(adns_query qu, struct timeval now);
/* Query must be in state tcpw/tcpw; it will be sent if possible and
 * no further processing can be done on it for now.  The connection
 * might be broken, but no reconnect will be attempted.
 */

struct udpsocket *adns__udpsocket_by_af(adns_state ads, int af);
/* Find the UDP socket structure in ads which has the given address family.
 * Return null if there isn't one.
 *
 * This is used during initialization, so ads is only partially filled in.
 * The requirements are that nudp is set, and that udpsocket[i].af are
 * defined for 0<=i<nudp.
 */

void adns__query_send(adns_query qu, struct timeval now);
/* Query must be in state tosend/NONE; it will be moved to a new state,
 * and no further processing can be done on it for now.
 * (Resulting state is one of udp/timew, tcpwait/timew (if server not
 * connected), tcpsent/timew, child/childw or done/output.)
 * __query_send may decide to use either UDP or TCP depending whether
 * _qf_usevc is set (or has become set) and whether the query is too
 * large.
 */

/* From query.c: */

adns_status adns__internal_submit(adns_state ads, adns_query *query_r,
				  adns_query parent,
				  const typeinfo *typei, adns_rrtype type,
				  vbuf *qumsg_vb, int id,
				  adns_queryflags flags, struct timeval now,
				  qcontext *ctx);
/* Submits a query (for internal use, called during external submits).
 *
 * The new query is returned in *query_r, or we return adns_s_nomemory.
 *
 * The query datagram should already have been assembled in qumsg_vb;
 * the memory for it is _taken over_ by this routine whether it
 * succeeds or fails (if it succeeds, the vbuf is reused for qu->vb).
 *
 * *ctx is copied byte-for-byte into the query.  Before doing this, its tinfo
 * field may be modified by type hooks.
 *
 * When the child query is done, ctx->callback will be called.  The
 * child will already have been taken off both the global list of
 * queries in ads and the list of children in the parent.  The child
 * will be freed when the callback returns.  The parent will have been
 * taken off the global childw queue.
 *
 * The callback should either call adns__query_done, if it is
 * complete, or adns__query_fail, if an error has occurred, in which
 * case the other children (if any) will be cancelled.  If the parent
 * has more unfinished children (or has just submitted more) then the
 * callback may choose to wait for them - it must then put the parent
 * back on the childw queue.
 */

void adns__search_next(adns_state ads, adns_query qu, struct timeval now);
/* Walks down the searchlist for a query with adns_qf_search.
 * The query should have just had a negative response, or not had
 * any queries sent yet, and should not be on any queue.
 * The query_dgram if any will be freed and forgotten and a new
 * one constructed from the search_* members of the query.
 *
 * Cannot fail (in case of error, calls adns__query_fail).
 */

void *adns__alloc_interim(adns_query qu, size_t sz);
void *adns__alloc_preserved(adns_query qu, size_t sz);
/* Allocates some memory, and records which query it came from
 * and how much there was.
 *
 * If an error occurs in the query, all the memory from _interim is
 * simply freed.  If the query succeeds, one large buffer will be made
 * which is big enough for all these allocations, and then
 * adns__alloc_final will get memory from this buffer.
 *
 * _alloc_interim can fail (and return 0).
 * The caller must ensure that the query is failed.
 *
 * The memory from _preserved is is kept and transferred into the
 * larger buffer - unless we run out of memory, in which case it too
 * is freed.  When you use _preserved you have to add code to the
 * x_nomem error exit case in adns__makefinal_query to clear out the
 * pointers you made to those allocations, because that's when they're
 * thrown away; you should also make a note in the declaration of
 * those pointer variables, to note that they are _preserved rather
 * than _interim.  If they're in the answer, note it here:
 *  answer->cname and answer->owner are _preserved.
 */

void adns__transfer_interim(adns_query from, adns_query to, void *block);
/* Transfers an interim allocation from one query to another, so that
 * the `to' query will have room for the data when we get to makefinal
 * and so that the free will happen when the `to' query is freed
 * rather than the `from' query.
 *
 * It is legal to call adns__transfer_interim with a null pointer; this
 * has no effect.
 *
 * _transfer_interim also ensures that the expiry time of the `to' query
 * is no later than that of the `from' query, so that child queries'
 * TTLs get inherited by their parents.
 */

void adns__free_interim(adns_query qu, void *p);
/* Forget about a block allocated by adns__alloc_interim.
 */

void *adns__alloc_mine(adns_query qu, size_t sz);
/* Like _interim, but does not record the length for later
 * copying into the answer.  This just ensures that the memory
 * will be freed when we're done with the query.
 */

void *adns__alloc_final(adns_query qu, size_t sz);
/* Cannot fail, and cannot return 0.
 */

void adns__makefinal_block(adns_query qu, void **blpp, size_t sz);
void adns__makefinal_str(adns_query qu, char **strp);

void adns__reset_preserved(adns_query qu);
/* Resets all of the memory management stuff etc. to take account of
 * only the _preserved stuff from _alloc_preserved.  Used when we find
 * an error somewhere and want to just report the error (with perhaps
 * CNAME, owner, etc. info), and also when we're halfway through RRs
 * in a datagram and discover that we need to retry the query.
 */

void adns__cancel(adns_query qu);
void adns__query_done(adns_query qu);
void adns__query_fail(adns_query qu, adns_status st);
void adns__cancel_children(adns_query qu);

void adns__returning(adns_state ads, adns_query qu);
/* Must be called before returning from adns any time that we have
 * progressed (including made, finished or destroyed) queries.
 *
 * Might reenter adns via internal query callbacks, so
 * external-faciing functions which call adns__returning should
 * normally be avoided in internal code. */

/* From reply.c: */

void adns__procdgram(adns_state ads, const byte *dgram, int len,
		     int serv, int viatcp, struct timeval now);
/* This function is allowed to cause new datagrams to be constructed
 * and sent, or even new queries to be started.  However,
 * query-sending functions are not allowed to call any general event
 * loop functions in case they accidentally call this.
 *
 * Ie, receiving functions may call sending functions.
 * Sending functions may NOT call receiving functions.
 */

/* From types.c: */

const typeinfo *adns__findtype(adns_rrtype type);

/* From parse.c: */

typedef struct {
  adns_state ads;
  adns_query qu;
  int serv;
  const byte *dgram;
  int dglen, max, cbyte, namelen;
  int *dmend_r;
} findlabel_state;

void adns__findlabel_start(findlabel_state *fls, adns_state ads,
			   int serv, adns_query qu,
			   const byte *dgram, int dglen, int max,
			   int dmbegin, int *dmend_rlater);
/* Finds labels in a domain in a datagram.
 *
 * Call this routine first.
 * dmend_rlater may be null.  ads (and of course fls) may not be.
 * serv may be -1, qu may be null - they are for error reporting.
 */

adns_status adns__findlabel_next(findlabel_state *fls,
				 int *lablen_r, int *labstart_r);
/* Then, call this one repeatedly.
 *
 * It will return adns_s_ok if all is well, and tell you the length
 * and start of successive labels.  labstart_r may be null, but
 * lablen_r must not be.
 *
 * After the last label, it will return with *lablen_r zero.
 * Do not then call it again; instead, just throw away the findlabel_state.
 *
 * *dmend_rlater will have been set to point to the next part of
 * the datagram after the label (or after the uncompressed part,
 * if compression was used).  *namelen_rlater will have been set
 * to the length of the domain name (total length of labels plus
 * 1 for each intervening dot).
 *
 * If the datagram appears to be truncated, *lablen_r will be -1.
 * *dmend_rlater, *labstart_r and *namelen_r may contain garbage.
 * Do not call _next again.
 *
 * There may also be errors, in which case *dmend_rlater,
 * *namelen_rlater, *lablen_r and *labstart_r may contain garbage.
 * Do not then call findlabel_next again.
 */

typedef enum {
  pdf_quoteok= 0x001
} parsedomain_flags;

adns_status adns__parse_domain(adns_state ads, int serv, adns_query qu,
			       vbuf *vb, parsedomain_flags flags,
			       const byte *dgram, int dglen, int *cbyte_io,
			       int max);
/* vb must already have been initialised; it will be reset if necessary.
 * If there is truncation, vb->used will be set to 0; otherwise
 * (if there is no error) vb will be null-terminated.
 * If there is an error vb and *cbyte_io may be left indeterminate.
 *
 * serv may be -1 and qu may be 0 - they are used for error reporting only.
 */

adns_status adns__parse_domain_more(findlabel_state *fls, adns_state ads,
				    adns_query qu, vbuf *vb,
				    parsedomain_flags flags,
				    const byte *dgram);
/* Like adns__parse_domain, but you pass it a pre-initialised findlabel_state,
 * for continuing an existing domain or some such of some kind.  Also, unlike
 * _parse_domain, the domain data will be appended to vb, rather than replacing
 * the existing contents.
 */

adns_status adns__findrr(adns_query qu, int serv,
			 const byte *dgram, int dglen, int *cbyte_io,
			 int *type_r, int *class_r, unsigned long *ttl_r,
			 int *rdlen_r, int *rdstart_r,
			 int *ownermatchedquery_r);
/* Finds the extent and some of the contents of an RR in a datagram
 * and does some checks.  The datagram is *dgram, length dglen, and
 * the RR starts at *cbyte_io (which is updated afterwards to point
 * to the end of the RR).
 *
 * The type, class, TTL and RRdata length and start are returned iff
 * the corresponding pointer variables are not null.  type_r, class_r
 * and ttl_r may not be null.  The TTL will be capped.
 *
 * If ownermatchedquery_r != 0 then the owner domain of this
 * RR will be compared with that in the query (or, if the query
 * has gone to a CNAME lookup, with the canonical name).
 * In this case, *ownermatchedquery_r will be set to 0 or 1.
 * The query datagram (or CNAME datagram) MUST be valid and not truncated.
 *
 * If there is truncation then *type_r will be set to -1 and
 * *cbyte_io, *class_r, *rdlen_r, *rdstart_r and *eo_matched_r will be
 * undefined.
 *
 * qu must obviously be non-null.
 *
 * If an error is returned then *type_r will be undefined too.
 */

adns_status adns__findrr_anychk(adns_query qu, int serv,
				const byte *dgram, int dglen, int *cbyte_io,
				int *type_r, int *class_r,
				unsigned long *ttl_r,
				int *rdlen_r, int *rdstart_r,
				const byte *eo_dgram, int eo_dglen,
				int eo_cbyte, int *eo_matched_r);
/* Like adns__findrr_checked, except that the datagram and
 * owner to compare with can be specified explicitly.
 *
 * If the caller thinks they know what the owner of the RR ought to
 * be they can pass in details in eo_*: this is another (or perhaps
 * the same datagram), and a pointer to where the putative owner
 * starts in that datagram.  In this case *eo_matched_r will be set
 * to 1 if the datagram matched or 0 if it did not.  Either
 * both eo_dgram and eo_matched_r must both be non-null, or they
 * must both be null (in which case eo_dglen and eo_cbyte will be ignored).
 * The eo datagram and contained owner domain MUST be valid and
 * untruncated.
 */

void adns__update_expires(adns_query qu, unsigned long ttl,
			  struct timeval now);
/* Updates the `expires' field in the query, so that it doesn't exceed
 * now + ttl.
 */

bool adns__labels_equal(const byte *a, int al, const byte *b, int bl);

/* From event.c: */

void adns__tcp_broken(adns_state ads, const char *what, const char *why);
/* what and why may be both 0, or both non-0. */

void adns__tcp_tryconnect(adns_state ads, struct timeval now);

void adns__autosys(adns_state ads, struct timeval now);
/* Make all the system calls we want to if the application wants us to.
 * Must not be called from within adns internal processing functions,
 * lest we end up in recursive descent !
 */

void adns__must_gettimeofday(adns_state ads, const struct timeval **now_io,
			     struct timeval *tv_buf);
/* Call with care - might reentrantly cause queries to be completed! */

int adns__pollfds(adns_state ads, struct pollfd pollfds_buf[MAX_POLLFDS]);
void adns__fdevents(adns_state ads,
		    const struct pollfd *pollfds, int npollfds,
		    int maxfd, const fd_set *readfds,
		    const fd_set *writefds, const fd_set *exceptfds,
		    struct timeval now, int *r_r);
int adns__internal_check(adns_state ads,
			 adns_query *query_io,
			 adns_answer **answer,
			 void **context_r);

void adns__timeouts(adns_state ads, int act,
		    struct timeval **tv_io, struct timeval *tvbuf,
		    struct timeval now);
/* If act is !0, then this will also deal with the TCP connection
 * if previous events broke it or require it to be connected.
 */

/* From check.c: */

void adns__consistency(adns_state ads, adns_query qu, consistency_checks cc);

/* Useful static inline functions: */

static inline int ctype_whitespace(int c) {
  return c==' ' || c=='\n' || c=='\t';
}
static inline int ctype_digit(int c) { return c>='0' && c<='9'; }
static inline int ctype_alpha(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static inline int ctype_toupper(int c) {
  return ctype_alpha(c) ? (c & ~32) : c;
}
static inline int ctype_822special(int c) {
  return strchr("()<>@,;:\\\".[]",c) != 0;
}
static inline int ctype_domainunquoted(int c) {
  return ctype_alpha(c) || ctype_digit(c) || (strchr("-_/+",c) != 0);
}

static inline int errno_resources(int e) { return e==ENOMEM || e==ENOBUFS; }

/* Useful macros */

#define MEM_ROUND(sz)						\
  (( ((sz)+sizeof(union maxalign)-1) / sizeof(union maxalign) )	\
   * sizeof(union maxalign) )

#define GETIL_B(cb) (((dgram)[(cb)++]) & 0x0ff)
#define GET_B(cb,tv) ((tv)= GETIL_B((cb)))
#define GET_W(cb,tv) ((tv)=0,(tv)|=(GETIL_B((cb))<<8), (tv)|=GETIL_B(cb), (tv))
#define GET_L(cb,tv) ( (tv)=0,				\
		       (tv)|=(GETIL_B((cb))<<24),	\
		       (tv)|=(GETIL_B((cb))<<16),	\
		       (tv)|=(GETIL_B((cb))<<8),	\
		       (tv)|=GETIL_B(cb),		\
		       (tv) )

#endif
