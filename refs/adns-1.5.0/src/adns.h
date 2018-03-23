/*
 * adns.h
 * - adns user-visible API
 */
/*
 *
 *  This file is
 *    Copyright (C) 1997-2000,2003,2006,2014 Ian Jackson
 *
 *  It is part of adns, which is
 *    Copyright (C) 1997-2000,2003,2006,2014 Ian Jackson
 *    Copyright (C) 1999-2000,2003,2006 Tony Finch
 *    Copyright (C) 1991 Massachusetts Institute of Technology
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
 * 
 *  For the benefit of certain LGPL'd `omnibus' software which
 *  provides a uniform interface to various things including adns, I
 *  make the following additional licence.  I do this because the GPL
 *  would otherwise force either the omnibus software to be GPL'd or
 *  the adns-using part to be distributed separately.
 *  
 *  So: you may also redistribute and/or modify adns.h (but only the
 *  public header file adns.h and not any other part of adns) under the
 *  terms of the GNU Library General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *  
 *  Note that adns itself is GPL'd.  Authors of adns-using applications
 *  with GPL-incompatible licences, and people who distribute adns with
 *  applications where the whole distribution is not GPL'd, are still
 *  likely to be in violation of the GPL.  Anyone who wants to do this
 *  should contact Ian Jackson.  Please note that to avoid encouraging
 *  people to infringe the GPL as it applies to the body of adns, Ian
 *  thinks that if you take advantage of the special exception to
 *  redistribute just adns.h under the LGPL, you should retain this
 *  paragraph in its place in the appropriate copyright statements.
 *
 *
 *  You should have received a copy of the GNU General Public License,
 *  or the GNU Library General Public License, as appropriate, along
 *  with this program; if not, write to the Free Software Foundation.
 *
 */

#ifndef ADNS_H_INCLUDED
#define ADNS_H_INCLUDED

#include <stdio.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C" { /* I really dislike this - iwj. */
#endif

/* Whether to support address families other than IPv4 in responses which use
 * the `adns_rr_addr' structure.  This is a source-compatibility issue: old
 * clients may not expect to find address families other than AF_INET in
 * their query results.  There's a separate binary compatibility issue to do
 * with the size of the `adns_rr_addr' structure, but we'll assume you can
 * cope with that because you have this header file.  Define
 * `ADNS_FEATURE_IPV4ONLY' if you only want to see AF_INET addresses by
 * default, or `ADNS_FEATURE_MANYAF' to allow multiple address families; the
 * default is currently to stick with AF_INET only, but this is likely to
 * change in a later release of ADNS.  Note that any adns_qf_want_... flags
 * in your query are observed: this setting affects only the default address
 * families.
 */
#if !defined(ADNS_FEATURE_IPV4ONLY) && !defined(ADNS_FEATURE_MANYAF)
#  define ADNS_FEATURE_IPV4ONLY
#elif defined(ADNS_FEATURE_IPV4ONLY) && defined(ADNS_FEATURE_MANYAF)
#  error "Feature flags ADNS_FEATURE_IPV4ONLY and ..._MANYAF are incompatible"
#endif

/* All struct in_addr anywhere in adns are in NETWORK byte order. */

typedef struct adns__state *adns_state;
typedef struct adns__query *adns_query;

typedef enum { /* In general, or together the desired flags: */
 adns_if_none=        0x0000,/* no flags.  nicer than 0 for some compilers */
 adns_if_noenv=       0x0001,/* do not look at environment */
 adns_if_noerrprint=  0x0002,/* never print to stderr (_debug overrides) */
 adns_if_noserverwarn=0x0004,/* do not warn to stderr about duff servers etc */
 adns_if_debug=       0x0008,/* enable all output to stderr plus debug msgs */
 adns_if_logpid=      0x0080,/* include pid in diagnostic output */
 adns_if_noautosys=   0x0010,/* do not make syscalls at every opportunity */
 adns_if_eintr=       0x0020,/* allow _wait and _synchronous to return EINTR */
 adns_if_nosigpipe=   0x0040,/* applic has SIGPIPE ignored, do not protect */
 adns_if_checkc_entex=0x0100,/* consistency checks on entry/exit to adns fns */
 adns_if_checkc_freq= 0x0300,/* consistency checks very frequently (slow!) */

 adns_if_permit_ipv4= 0x0400,/* allow _addr queries to return IPv4 addresses  */
 adns_if_permit_ipv6= 0x0800,/* allow _addr queries to return IPv6 addresses */
 adns_if_afmask=      0x0c00,
   /* These are policy flags, and overridden by the adns_af:... option in
    * resolv.conf.  If the adns_qf_want_... query flags are incompatible with
    * these settings (in the sense that no address families are permitted at
    * all) then the query flags take precedence; otherwise only records which
    * satisfy all of the stated requirements are allowed.
    */
 adns__if_sizeforce= 0x7fff,
} adns_initflags;

typedef enum { /* In general, or together the desired flags: */
 adns_qf_none=           0x00000000,/* no flags */
 adns_qf_search=         0x00000001,/* use the searchlist */
 adns_qf_usevc=          0x00000002,/* use a virtual circuit (TCP conn) */
 adns_qf_owner=          0x00000004,/* fill in the owner field in the answer */
 adns_qf_quoteok_query=  0x00000010,/* allow special chars in query domain */
 adns_qf_quoteok_cname=  0x00000000,/*  ... in CNAME we go via (now default) */
 adns_qf_quoteok_anshost=0x00000040,/*  ... in things supposedly hostnames */
 adns_qf_quotefail_cname=0x00000080,/* refuse if quote-req chars in CNAME we go via */
 adns_qf_cname_loose=    0x00000100,/* allow refs to CNAMEs - without, get _s_cname */
 adns_qf_cname_strict=   0x00010000,/* forbid CNAME refs (default, currently) */
 adns_qf_cname_forbid=   0x00000200,/* don't follow CNAMEs, instead give _s_cname */

 adns_qf_want_ipv4=	 0x00000400,/* try to return IPv4 addresses */
 adns_qf_want_ipv6=	 0x00000800,/* try to return IPv6 addresses */
 adns_qf_want_allaf=	 0x00000c00,/* all the above flag bits */
   /* Without any of the _qf_want_... flags, _qtf_deref queries try to return
    * all address families permitted by _if_permit_... (as overridden by the
    * `adns_af:...'  configuration option).  Set flags to restrict the
    * returned address families to the ones selected.
    */
 adns_qf_ipv6_mapv4=	 0x00001000,/*  ... return IPv4 addresses as v6-mapped */

 adns_qf_addrlit_scope_forbid=0x00002000,/* forbid %<scope> in IPv6 literals */
 adns_qf_addrlit_scope_numeric=0x00004000,/* %<scope> may only be numeric */
 adns_qf_addrlit_ipv4_quadonly=0x00008000,/* reject non-dotted-quad ipv4 */

 adns__qf_internalmask=  0x0ff00000,
 adns__qf_sizeforce=     0x7fffffff
} adns_queryflags;

typedef enum {
 adns_rrt_typemask=  0x0ffff,
 adns_rrt_reprmask= 0xffffff,
 adns__qtf_deref_bit=0x10000,/* internal version of ..._deref below */
 adns__qtf_mail822=  0x20000,/* return mailboxes in RFC822 rcpt field fmt   */

 adns__qtf_bigaddr=0x1000000,/* use the new larger sockaddr union */
 adns__qtf_manyaf= 0x2000000,/* permitted to return multiple address families */

 adns__qtf_deref=    adns__qtf_deref_bit|adns__qtf_bigaddr
#ifdef ADNS_FEATURE_MANYAF
		     |adns__qtf_manyaf
#endif
			    ,/* dereference domains; perhaps get extra data */

 adns_r_unknown=     0x40000,
   /* To use this, ask for records of type   <rr-type-code>|adns_r_unknown.
    * adns will not process the RDATA - you'll get adns_rr_byteblocks,
    * where the int is the length and the unsigned char* points to the
    * data.  String representation of the RR data (by adns_rrinfo) is as in
    * RFC3597.  adns_rr_info will not return the type name in *rrtname_r
    * (due to memory management problems); *fmtname_r will be set to
    * "unknown".
    *
    * Do not specify adns_r_unknown along with a known RR type which
    * requires domain name uncompression (see RFC3597 s4); domain names
    * will not be uncompressed and the resulting data would be useless.
    * Asking for meta-RR types via adns_r_unknown will not work properly
    * either and may make adns complain about server misbehaviour, so don't
    * do that.
    *
    * Don't forget adns_qf_quoteok if that's what you want. */

 adns_r_none=             0,
 		     
 adns_r_a=                1,
 		     
 adns_r_ns_raw=           2,
 adns_r_ns=                  adns_r_ns_raw|adns__qtf_deref,
 		     
 adns_r_cname=            5,
 		     
 adns_r_soa_raw=          6,
 adns_r_soa=                 adns_r_soa_raw|adns__qtf_mail822, 
 		     
 adns_r_ptr_raw=         12, /* do not mind PTR with wrong or missing addr */
 adns_r_ptr=                 adns_r_ptr_raw|adns__qtf_deref,
 		     
 adns_r_hinfo=           13,  
 		     
 adns_r_mx_raw=          15,
 adns_r_mx=                  adns_r_mx_raw|adns__qtf_deref,
 		     
 adns_r_txt=             16,
 		     
 adns_r_rp_raw=          17,
 adns_r_rp=                  adns_r_rp_raw|adns__qtf_mail822,

 adns_r_aaaa=		 28,

 /* For SRV records, query domain without _qf_quoteok_query must look
  * as expected from SRV RFC with hostname-like Name.  _With_
  * _quoteok_query, any query domain is allowed. */
 adns_r_srv_raw=         33,
 adns_r_srv=                 adns_r_srv_raw|adns__qtf_deref,
		     
 adns_r_addr=                adns_r_a|adns__qtf_deref,

 adns__rrt_sizeforce= 0x7fffffff,
 
} adns_rrtype;

/*
 * In queries without qf_quoteok_*, all domains must have standard
 * legal syntax, or you get adns_s_querydomainvalid (if the query
 * domain contains bad characters) or adns_s_answerdomaininvalid (if
 * the answer contains bad characters).
 * 
 * In queries _with_ qf_quoteok_*, domains in the query or response
 * may contain any characters, quoted according to RFC1035 5.1.  On
 * input to adns, the char* is a pointer to the interior of a "
 * delimited string, except that " may appear in it unquoted.  On
 * output, the char* is a pointer to a string which would be legal
 * either inside or outside " delimiters; any character which isn't
 * legal in a hostname (ie alphanumeric or hyphen) or one of _ / +
 * (the three other punctuation characters commonly abused in domain
 * names) will be quoted, as \X if it is a printing ASCII character or
 * \DDD otherwise.
 *
 * If the query goes via a CNAME then the canonical name (ie, the
 * thing that the CNAME record refers to) is usually allowed to
 * contain any characters, which will be quoted as above.  With
 * adns_qf_quotefail_cname you get adns_s_answerdomaininvalid when
 * this happens.  (This is a change from version 0.4 and earlier, in
 * which failing the query was the default, and you had to say
 * adns_qf_quoteok_cname to avoid this; that flag is now deprecated.)
 *
 * In version 0.4 and earlier, asking for _raw records containing
 * mailboxes without specifying _qf_quoteok_anshost was silly.  This
 * is no longer the case.  In this version only parts of responses
 * that are actually supposed to be hostnames will be refused by
 * default if quote-requiring characters are found.
 */

/*
 * If you ask for an RR which contains domains which are actually
 * encoded mailboxes, and don't ask for the _raw version, then adns
 * returns the mailbox formatted suitably for an RFC822 recipient
 * header field.  The particular format used is that if the mailbox
 * requires quoting according to the rules in RFC822 then the
 * local-part is quoted in double quotes, which end at the next
 * unescaped double quote (\ is the escape char, and is doubled, and
 * is used to escape only \ and ").  If the local-part is legal
 * without quoting according to RFC822, it is presented as-is.  In any
 * case the local-part is followed by an @ and the domain.  The domain
 * will not contain any characters not legal in hostnames.
 *
 * Unquoted local-parts may contain any printing 7-bit ASCII
 * except the punctuation characters ( ) < > @ , ; : \ " [ ]
 * I.e. they may contain alphanumerics, and the following
 * punctuation characters:  ! # % ^ & * - _ = + { } .
 *
 * adns will reject local parts containing control characters (byte
 * values 0-31, 127-159, and 255) - these appear to be legal according
 * to RFC822 (at least 0-127) but are clearly a bad idea.  RFC1035
 * syntax does not make any distinction between a single RFC822
 * quoted-string containing full stops, and a series of quoted-strings
 * separated by full stops; adns will return anything that isn't all
 * valid atoms as a single quoted-string.  RFC822 does not allow
 * high-bit-set characters at all, but adns does allow them in
 * local-parts, treating them as needing quoting.
 *
 * If you ask for the domain with _raw then _no_ checking is done
 * (even on the host part, regardless of adns_qf_quoteok_anshost), and
 * you just get the domain name in master file format.
 *
 * If no mailbox is supplied the returned string will be `.' in either
 * case.
 */

typedef enum {
 adns_s_ok,

 /* locally induced errors */
 adns_s_nomemory,
 adns_s_unknownrrtype,
 adns_s_systemfail,

 adns_s_max_localfail= 29,
 
 /* remotely induced errors, detected locally */
 adns_s_timeout,
 adns_s_allservfail,
 adns_s_norecurse,
 adns_s_invalidresponse,
 adns_s_unknownformat,

 adns_s_max_remotefail= 59,
 
 /* remotely induced errors, reported by remote server to us */
 adns_s_rcodeservfail,
 adns_s_rcodeformaterror,
 adns_s_rcodenotimplemented,
 adns_s_rcoderefused,
 adns_s_rcodeunknown,

 adns_s_max_tempfail= 99,

 /* remote configuration errors */
 adns_s_inconsistent, /* PTR gives domain whose addr is missing or mismatch */
 adns_s_prohibitedcname, /* CNAME, but eg A expected (not if _qf_cname_loose) */
 adns_s_answerdomaininvalid,
 adns_s_answerdomaintoolong,
 adns_s_invaliddata,
 
 adns_s_max_misconfig= 199,

 /* permanent problems with the query */
 adns_s_querydomainwrong,
 adns_s_querydomaininvalid,
 adns_s_querydomaintoolong,
 
 adns_s_max_misquery= 299,

 /* permanent errors */
 adns_s_nxdomain,
 adns_s_nodata,

 adns_s_max_permfail= 499
 
} adns_status;

typedef union {
  struct sockaddr sa;
  struct sockaddr_in inet;
} adns_sockaddr_v4only;

typedef union {
  struct sockaddr sa;
  struct sockaddr_in inet;
  struct sockaddr_in6 inet6;
} adns_sockaddr;

typedef struct {
  int len;
  adns_sockaddr addr;
} adns_rr_addr;

typedef struct {
  /* the old v4-only structure; handy if you have complicated binary
   * compatibility problems. */
  int len;
  adns_sockaddr_v4only addr;
} adns_rr_addr_v4only;

typedef struct {
  char *host;
  adns_status astatus;
  int naddrs; /* temp fail => -1, perm fail => 0, s_ok => >0 */
  adns_rr_addr *addrs;
} adns_rr_hostaddr;

typedef struct {
  char *(array[2]);
} adns_rr_strpair;

typedef struct {
  int i;
  adns_rr_hostaddr ha;
} adns_rr_inthostaddr;

typedef struct {
  /* Used both for mx_raw, in which case i is the preference and str
   * the domain, and for txt, in which case each entry has i for the
   * `text' length, and str for the data (which will have had an extra
   * nul appended so that if it was plain text it is now a
   * null-terminated string).
   */
  int i;
  char *str;
} adns_rr_intstr;

typedef struct {
  adns_rr_intstr array[2];
} adns_rr_intstrpair;

typedef struct {
  char *mname, *rname;
  unsigned long serial, refresh, retry, expire, minimum;
} adns_rr_soa;

typedef struct {
  int priority, weight, port;
  char *host;
} adns_rr_srvraw;

typedef struct {
  int priority, weight, port;
  adns_rr_hostaddr ha;
} adns_rr_srvha;

typedef struct {
  int len;
  unsigned char *data;
} adns_rr_byteblock;

typedef struct {
  adns_status status;
  char *cname; /* always NULL if query was for CNAME records */
  char *owner; /* only set if req'd in query flags; maybe 0 on error anyway */
  adns_rrtype type; /* guaranteed to be same as in query */
  time_t expires;/*abs time.  def only if _s_ok, nxdomain or nodata. NOT TTL!*/
  int nrrs, rrsz; /* nrrs is 0 if an error occurs */
  union {
    void *untyped;
    unsigned char *bytes;
    char *(*str);                    /* ns_raw, cname, ptr, ptr_raw */
    adns_rr_intstr *(*manyistr);     /* txt (list strs ends with i=-1, str=0)*/
    adns_rr_addr *addr;              /* addr */
    struct in_addr *inaddr;          /* a */
    struct in6_addr *in6addr;	     /* aaaa */
    adns_rr_hostaddr *hostaddr;      /* ns */
    adns_rr_intstrpair *intstrpair;  /* hinfo */
    adns_rr_strpair *strpair;        /* rp, rp_raw */
    adns_rr_inthostaddr *inthostaddr;/* mx */
    adns_rr_intstr *intstr;          /* mx_raw */
    adns_rr_soa *soa;                /* soa, soa_raw */
    adns_rr_srvraw *srvraw;          /* srv_raw */
    adns_rr_srvha *srvha;/* srv */
    adns_rr_byteblock *byteblock;    /* ...|unknown */
  } rrs;
} adns_answer;

/* Memory management:
 *  adns_state and adns_query are actually pointers to malloc'd state;
 *  On submission questions are copied, including the owner domain;
 *  Answers are malloc'd as a single piece of memory; pointers in the
 *  answer struct point into further memory in the answer.
 * query_io:
 *  Must always be non-null pointer;
 *  If *query_io is 0 to start with then any query may be returned;
 *  If *query_io is !0 adns_query then only that query may be returned.
 *  If the call is successful, *query_io, *answer_r, and *context_r
 *  will all be set.
 * Errors:
 *  Return values are 0 or an errno value.
 *
 *  For _init, _init_strcfg, _submit and _synchronous, system errors
 *  (eg, failure to create sockets, malloc failure, etc.) return errno
 *  values.  EINVAL from _init et al means the configuration file
 *  is erroneous and cannot be parsed.
 * 
 *  For _wait and _check failures are reported in the answer
 *  structure, and only 0, ESRCH or (for _check) EAGAIN is
 *  returned: if no (appropriate) requests are done adns_check returns
 *  EAGAIN; if no (appropriate) requests are outstanding both
 *  adns_query and adns_wait return ESRCH.
 *
 *  Additionally, _wait can return EINTR if you set adns_if_eintr.
 *
 *  All other errors (nameserver failure, timed out connections, &c)
 *  are returned in the status field of the answer.  After a
 *  successful _wait or _check, if status is nonzero then nrrs will be
 *  0, otherwise it will be >0.  type will always be the type
 *  requested.
 */

/* Threads:
 *  adns does not use any static modifiable state, so it
 *   is safe to call adns_init several times and then use the
 *   resulting adns_states concurrently.
 *  However, it is NOT safe to make simultaneous calls into
 *   adns using the same adns_state; a single adns_state must be used
 *   only by one thread at a time.  You can solve this problem by
 *   having one adns_state per thread, or if that isn't feasible, you
 *   could maintain a pool of adns_states.  Unfortunately neither of
 *   these approaches has optimal performance.
 */

int adns_init(adns_state *newstate_r, adns_initflags flags,
	      FILE *diagfile /*0=>stderr*/);

int adns_init_strcfg(adns_state *newstate_r, adns_initflags flags,
		     FILE *diagfile /*0=>discard*/, const char *configtext);

typedef void adns_logcallbackfn(adns_state ads, void *logfndata,
				const char *fmt, va_list al);
  /* Will be called perhaps several times for each message; when the
   * message is complete, the string implied by fmt and al will end in
   * a newline.  Log messages start with `adns debug:' or `adns
   * warning:' or `adns:' (for errors), or `adns debug [PID]:'
   * etc. if adns_if_logpid is set. */

int adns_init_logfn(adns_state *newstate_r, adns_initflags flags,
		    const char *configtext /*0=>use default config files*/,
		    adns_logcallbackfn *logfn /*0=>logfndata is a FILE* */,
		    void *logfndata /*0 with logfn==0 => discard*/);

/* Configuration:
 *  adns_init reads /etc/resolv.conf, which is expected to be (broadly
 *  speaking) in the format expected by libresolv, and then
 *  /etc/resolv-adns.conf if it exists.  adns_init_strcfg is instead
 *  passed a string which is interpreted as if it were the contents of
 *  resolv.conf or resolv-adns.conf.  In general, configuration which
 *  is set later overrides any that is set earlier.
 *
 * Standard directives understood in resolv[-adns].conf:
 * 
 *  nameserver <address>
 *   Must be followed by the IP address of a nameserver.  Several
 *   nameservers may be specified, and they will be tried in the order
 *   found.  There is a compiled in limit, currently 5, on the number
 *   of nameservers.  (libresolv supports only 3 nameservers.)
 *
 *  search <domain> ...
 *   Specifies the search list for queries which specify
 *   adns_qf_search.  This is a list of domains to append to the query
 *   domain.  The query domain will be tried as-is either before all
 *   of these or after them, depending on the ndots option setting
 *   (see below).
 *
 *  domain <domain>
 *   This is present only for backward compatibility with obsolete
 *   versions of libresolv.  It should not be used, and is interpreted
 *   by adns as if it were `search' - note that this is subtly
 *   different to libresolv's interpretation of this directive.
 *
 *  sortlist <addr>/<mask> ...
 *   Should be followed by a sequence of IP-address and netmask pairs,
 *   separated by spaces.  They may be specified as
 *   eg. 172.30.206.0/24 or 172.30.206.0/255.255.255.0.  Currently up
 *   to 15 pairs may be specified (but note that libresolv only
 *   supports up to 10).
 *
 *  options
 *   Should followed by one or more options, separated by spaces.
 *   Each option consists of an option name, followed by optionally
 *   a colon and a value.  Options are listed below.
 *
 * Non-standard directives understood in resolv[-adns].conf:
 *
 *  clearnameservers
 *   Clears the list of nameservers, so that further nameserver lines
 *   start again from the beginning.
 *
 *  include <filename>
 *   The specified file will be read.
 *
 * Additionally, adns will ignore lines in resolv[-adns].conf which
 * start with a #.
 *
 * Standard options understood:
 *
 *  debug
 *   Enables debugging output from the resolver, which will be written
 *   to stderr.
 *
 *  ndots:<count>
 *   Affects whether queries with adns_qf_search will be tried first
 *   without adding domains from the searchlist, or whether the bare
 *   query domain will be tried last.  Queries which contain at least
 *   <count> dots will be tried bare first.  The default is 1.
 *
 * Non-standard options understood:
 *
 *  adns_checkc:none
 *  adns_checkc:entex
 *  adns_checkc:freq
 *   Changes the consistency checking frequency; this overrides the
 *   setting of adns_if_check_entex, adns_if_check_freq, or neither,
 *   in the flags passed to adns_init.
 *
 *  adns_af:{ipv4,ipv6},...  adns_af:any
 *   Determines which address families ADNS looks up (either as an
 *   adns_r_addr query, or when dereferencing an answer yielding hostnames
 *   (e.g., adns_r_mx).  The argument is a comma-separated list: only the
 *   address families listed will be looked up.  The default is `any'.
 *   Lookups occur (logically) concurrently; use the `sortlist' directive to
 *   control the relative order of addresses in answers.  This option
 *   overrides the corresponding init flags (covered by adns_if_afmask).
 *
 *  adns_ignoreunkcfg
 *   Ignore unknown options and configuration directives, rather than
 *   logging them.  To be effective, appear in the configuration
 *   before the unknown options.  ADNS_RES_OPTIONS is generally early
 *   enough.
 * 
 * There are a number of environment variables which can modify the
 * behaviour of adns.  They take effect only if adns_init is used, and
 * the caller of adns_init can disable them using adns_if_noenv.  In
 * each case there is both a FOO and an ADNS_FOO; the latter is
 * interpreted later so that it can override the former.  Unless
 * otherwise stated, environment variables are interpreted after
 * resolv[-adns].conf are read, in the order they are listed here.
 *
 *  RES_CONF, ADNS_RES_CONF
 *   A filename, whose contets are in the format of resolv.conf.
 *
 *  RES_CONF_TEXT, ADNS_RES_CONF_TEXT
 *   A string in the format of resolv.conf.
 *
 *  RES_OPTIONS, ADNS_RES_OPTIONS
 *   These are parsed as if they appeared in the `options' line of a
 *   resolv.conf.  In addition to being parsed at this point in the
 *   sequence, they are also parsed at the very beginning before
 *   resolv.conf or any other environment variables are read, so that
 *   any debug option can affect the processing of the configuration.
 *
 *  LOCALDOMAIN, ADNS_LOCALDOMAIN
 *   These are interpreted as if their contents appeared in a `search'
 *   line in resolv.conf.
 */

int adns_synchronous(adns_state ads,
		     const char *owner,
		     adns_rrtype type,
		     adns_queryflags flags,
		     adns_answer **answer_r);

/* NB: if you set adns_if_noautosys then _submit and _check do not
 * make any system calls; you must use some of the asynch-io event
 * processing functions to actually get things to happen.
 */

int adns_submit(adns_state ads,
		const char *owner,
		adns_rrtype type,
		adns_queryflags flags,
		void *context,
		adns_query *query_r);

/* The owner should be quoted in master file format. */

int adns_check(adns_state ads,
	       adns_query *query_io,
	       adns_answer **answer_r,
	       void **context_r);

int adns_wait(adns_state ads,
	      adns_query *query_io,
	      adns_answer **answer_r,
	      void **context_r);

/* same as adns_wait but uses poll(2) internally */
int adns_wait_poll(adns_state ads,
		   adns_query *query_io,
		   adns_answer **answer_r,
		   void **context_r);

void adns_cancel(adns_query query);

/* The adns_query you get back from _submit is valid (ie, can be
 * legitimately passed into adns functions) until it is returned by
 * adns_check or adns_wait, or passed to adns_cancel.  After that it
 * must not be used.  You can rely on it not being reused until the
 * first adns_submit or _transact call using the same adns_state after
 * it became invalid, so you may compare it for equality with other
 * query handles until you next call _query or _transact.
 *
 * _submit and _synchronous return ENOSYS if they don't understand the
 * query type.
 */

int adns_submit_reverse(adns_state ads,
			const struct sockaddr *addr,
			adns_rrtype type,
			adns_queryflags flags,
			void *context,
			adns_query *query_r);
/* type must be _r_ptr or _r_ptr_raw.  _qf_search is ignored.
 * addr->sa_family must be AF_INET or you get ENOSYS.
 */

int adns_submit_reverse_any(adns_state ads,
			    const struct sockaddr *addr,
			    const char *rzone,
			    adns_rrtype type,
			    adns_queryflags flags,
			    void *context,
			    adns_query *query_r);
/* For RBL-style reverse `zone's; look up
 *   <reversed-address>.<zone>
 * Any type is allowed.  _qf_search is ignored.
 * addr->sa_family must be AF_INET or you get ENOSYS.
 */

void adns_finish(adns_state ads);
/* You may call this even if you have queries outstanding;
 * they will be cancelled.
 */

#define ADNS_ADDR2TEXT_BUFLEN					\
  (INET6_ADDRSTRLEN + 1/*%*/					\
  + ((IF_NAMESIZE-1) > 9 ? (IF_NAMESIZE-1) : 9/*uint32*/)	\
  + 1/* nul; included in IF_NAMESIZE */)

int adns_text2addr(const char *text, uint16_t port, adns_queryflags flags,
		   struct sockaddr *sa_r,
		   socklen_t *salen_io /* updated iff OK or ENOSPC */);
int adns_addr2text(const struct sockaddr *sa, adns_queryflags flags,
		   char *buffer, int *buflen_io /* updated ONLY on ENOSPC */,
		   int *port_r /* may be 0 */);
  /*
   * port is always in host byte order and is simply copied to and
   * from the appropriate sockaddr field (byteswapped as necessary).
   *
   * The only flags supported are adns_qf_addrlit_...
   *
   * Error return values are:
   *
   *  ENOSPC    Output buffer is too small.  Can only happen if
   *            *buflen_io < ADNS_ADDR2TEXT_BUFLEN or
   *            *salen_io < sizeof(adns_sockaddr).  On return,
   *            *buflen_io or *salen_io has been updated by adns.
   *
   *  EINVAL    text has invalid syntax.
   *
   *            text represents an address family not supported by
   *            this version of adns.
   *
   *            Scoped address supplied (text contained "%" or
   *            sin6_scope_id nonzero) but caller specified
   *            adns_qf_addrlit_scope_forbid.
   *
   *            Scope name (rather than number) supplied in text but
   *            caller specified adns_qf_addrlit_scope_numeric.
   *
   *  EAFNOSUPPORT   sa->sa_family is not supported (addr2text only).
   *
   *  ENOSYS    Unsupported flags set.
   *
   * Only if neither adns_qf_addrlit_scope_forbid nor
   * adns_qf_addrlit_scope_numeric are set:
   *
   *  ENOSYS    Scope name supplied in text but IPv6 address part of
   *            sockaddr is not a link local address.
   *
   *  ENXIO     Scope name supplied in text but if_nametoindex
   *            said it wasn't a valid local interface name.
   *
   *  EIO       Scoped address supplied but if_nametoindex failed
   *            in an unexpected way; adns has printed a message to
   *            stderr.
   *
   *  any other   if_nametoindex failed in a more-or-less expected way.
   */

void adns_forallqueries_begin(adns_state ads);
adns_query adns_forallqueries_next(adns_state ads, void **context_r);
/* Iterator functions, which you can use to loop over the outstanding
 * (submitted but not yet successfuly checked/waited) queries.
 *
 * You can only have one iteration going at once.  You may call _begin
 * at any time; after that, an iteration will be in progress.  You may
 * only call _next when an iteration is in progress - anything else
 * may coredump.  The iteration remains in progress until _next
 * returns 0, indicating that all the queries have been walked over,
 * or ANY other adns function is called with the same adns_state (or a
 * query in the same adns_state).  There is no need to explicitly
 * finish an iteration.
 *
 * context_r may be 0.  *context_r may not be set when _next returns 0.
 */

void adns_checkconsistency(adns_state ads, adns_query qu);
/* Checks the consistency of adns's internal data structures.
 * If any error is found, the program will abort().
 * You may pass 0 for qu; if you pass non-null then additional checks
 * are done to make sure that qu is a valid query.
 */

/*
 * Example expected/legal calling sequence for submit/check/wait:
 *  adns_init
 *  adns_submit 1
 *  adns_submit 2
 *  adns_submit 3
 *  adns_wait 1
 *  adns_check 3 -> EAGAIN
 *  adns_wait 2
 *  adns_wait 3
 *  ....
 *  adns_finish
 */

/*
 * Entrypoints for generic asynch io:
 * (these entrypoints are not very useful except in combination with *
 * some of the other I/O model calls which can tell you which fds to
 * be interested in):
 *
 * Note that any adns call may cause adns to open and close fds, so
 * you must call beforeselect or beforepoll again just before
 * blocking, or you may not have an up-to-date list of it's fds.
 */

int adns_processany(adns_state ads);
/* Gives adns flow-of-control for a bit.  This will never block, and
 * can be used with any threading/asynch-io model.  If some error
 * occurred which might cause an event loop to spin then the errno
 * value is returned.
 */

int adns_processreadable(adns_state ads, int fd, const struct timeval *now);
int adns_processwriteable(adns_state ads, int fd, const struct timeval *now);
int adns_processexceptional(adns_state ads, int fd, const struct timeval *now);
/* Gives adns flow-of-control so that it can process incoming data
 * from, or send outgoing data via, fd.  Very like _processany.  If it
 * returns zero then fd will no longer be readable or writeable
 * (unless of course more data has arrived since).  adns will _only_
 * use that fd and only in the manner specified, regardless of whether
 * adns_if_noautosys was specified.
 *
 * adns_processexceptional should be called when select(2) reports an
 * exceptional condition, or poll(2) reports POLLPRI.
 *
 * It is fine to call _processreabable or _processwriteable when the
 * fd is not ready, or with an fd that doesn't belong to adns; it will
 * then just return 0.
 *
 * If some error occurred which might prevent an event loop to spin
 * then the errno value is returned.
 */

void adns_processtimeouts(adns_state ads, const struct timeval *now);
/* Gives adns flow-of-control so that it can process any timeouts
 * which might have happened.  Very like _processreadable/writeable.
 *
 * now may be 0; if it isn't, *now must be the current time, recently
 * obtained from gettimeofday.
 */

void adns_firsttimeout(adns_state ads,
		       struct timeval **tv_mod, struct timeval *tv_buf,
		       struct timeval now);
/* Asks adns when it would first like the opportunity to time
 * something out.  now must be the current time, from gettimeofday.
 * 
 * If tv_mod points to 0 then tv_buf must be non-null, and
 * _firsttimeout will fill in *tv_buf with the time until the first
 * timeout, and make *tv_mod point to tv_buf.  If adns doesn't have
 * anything that might need timing out it will leave *tv_mod as 0.
 *
 * If *tv_mod is not 0 then tv_buf is not used.  adns will update
 * *tv_mod if it has any earlier timeout, and leave it alone if it
 * doesn't.
 *
 * This call will not actually do any I/O, or change the fds that adns
 * is using.  It always succeeds and never blocks.
 */

void adns_globalsystemfailure(adns_state ads);
/* If serious problem(s) happen which globally affect your ability to
 * interact properly with adns, or adns's ability to function
 * properly, you or adns can call this function.
 *
 * All currently outstanding queries will be made to fail with
 * adns_s_systemfail, and adns will close any stream sockets it has
 * open.
 *
 * This is used by adns, for example, if gettimeofday() fails.
 * Without this the program's event loop might start to spin !
 *
 * This call will never block.
 */

/*
 * Entrypoints for select-loop based asynch io:
 */

void adns_beforeselect(adns_state ads, int *maxfd, fd_set *readfds,
		       fd_set *writefds, fd_set *exceptfds,
		       struct timeval **tv_mod, struct timeval *tv_buf,
		       const struct timeval *now);
/* Find out file descriptors adns is interested in, and when it would
 * like the opportunity to time something out.  If you do not plan to
 * block then tv_mod may be 0.  Otherwise, tv_mod and tv_buf are as
 * for adns_firsttimeout.  readfds, writefds, exceptfds and maxfd_io may
 * not be 0.
 *
 * If tv_mod is 0 on entry then this will never actually do any I/O,
 * or change the fds that adns is using or the timeouts it wants.  In
 * any case it won't block, and it will set the timeout to zero if a
 * query finishes in _beforeselect.
 */

void adns_afterselect(adns_state ads, int maxfd, const fd_set *readfds,
		      const fd_set *writefds, const fd_set *exceptfds,
		      const struct timeval *now);
/* Gives adns flow-of-control for a bit; intended for use after
 * select.  This is just a fancy way of calling adns_processreadable/
 * writeable/timeouts as appropriate, as if select had returned the
 * data being passed.  Always succeeds.
 */

/*
 * Example calling sequence:
 *
 *  adns_init _noautosys
 *  loop {
 *   adns_beforeselect
 *   select
 *   adns_afterselect
 *   ...
 *   adns_submit / adns_check
 *   ...
 *  }
 */

/*
 * Entrypoints for poll-loop based asynch io:
 */

struct pollfd;
/* In case your system doesn't have it or you forgot to include
 * <sys/poll.h>, to stop the following declarations from causing
 * problems.  If your system doesn't have poll then the following
 * entrypoints will not be defined in libadns.  Sorry !
 */

int adns_beforepoll(adns_state ads, struct pollfd *fds,
		    int *nfds_io, int *timeout_io,
		    const struct timeval *now);
/* Finds out which fd's adns is interested in, and when it would like
 * to be able to time things out.  This is in a form suitable for use
 * with poll(2).
 * 
 * On entry, usually fds should point to at least *nfds_io structs.
 * adns will fill up to that many structs will information for poll,
 * and record in *nfds_io how many structs it filled.  If it wants to
 * listen for more structs then *nfds_io will be set to the number
 * required and _beforepoll will return ERANGE.
 *
 * You may call _beforepoll with fds==0 and *nfds_io 0, in which case
 * adns will fill in the number of fds that it might be interested in
 * in *nfds_io, and always return either 0 (if it is not interested in
 * any fds) or ERANGE (if it is).
 *
 * NOTE that (unless now is 0) adns may acquire additional fds
 * from one call to the next, so you must put adns_beforepoll in a
 * loop, rather than assuming that the second call (with the buffer
 * size requested by the first) will not return ERANGE.
 *
 * adns only ever sets POLLIN, POLLOUT and POLLPRI in its pollfd
 * structs, and only ever looks at those bits.  POLLPRI is required to
 * detect TCP Urgent Data (which should not be used by a DNS server)
 * so that adns can know that the TCP stream is now useless.
 *
 * In any case, *timeout_io should be a timeout value as for poll(2),
 * which adns will modify downwards as required.  If the caller does
 * not plan to block then *timeout_io should be 0 on entry, or
 * alternatively, timeout_io may be 0.  (Alternatively, the caller may
 * use _beforeselect with timeout_io==0 to find out about file
 * descriptors, and use _firsttimeout is used to find out when adns
 * might want to time something out.)
 *
 * adns_beforepoll will return 0 on success, and will not fail for any
 * reason other than the fds buffer being too small (ERANGE).
 *
 * This call will never actually do any I/O.  If you supply the
 * current time it will not change the fds that adns is using or the
 * timeouts it wants.
 *
 * In any case this call won't block.
 */

#define ADNS_POLLFDS_RECOMMENDED 3
/* If you allocate an fds buf with at least RECOMMENDED entries then
 * you are unlikely to need to enlarge it.  You are recommended to do
 * so if it's convenient.  However, you must be prepared for adns to
 * require more space than this.
 */

void adns_afterpoll(adns_state ads, const struct pollfd *fds, int nfds,
		    const struct timeval *now);
/* Gives adns flow-of-control for a bit; intended for use after
 * poll(2).  fds and nfds should be the results from poll().  pollfd
 * structs mentioning fds not belonging to adns will be ignored.
 */


adns_status adns_rr_info(adns_rrtype type,
			 const char **rrtname_r, const char **fmtname_r,
			 int *len_r,
			 const void *datap, char **data_r);
/*
 * Get information about a query type, or convert reply data to a
 * textual form.  type must be specified, and the official name of the
 * corresponding RR type will be returned in *rrtname_r, and
 * information about the processing style in *fmtname_r.  The length
 * of the table entry in an answer for that type will be returned in
 * in *len_r.  Any or all of rrtname_r, fmtname_r and len_r may be 0.
 * If fmtname_r is non-null then *fmtname_r may be null on return,
 * indicating that no special processing is involved.
 *
 * data_r be must be non-null iff datap is.  In this case *data_r will
 * be set to point to a string pointing to a representation of the RR
 * data in master file format.  (The owner name, timeout, class and
 * type will not be present - only the data part of the RR.)  The
 * memory will have been obtained from malloc() and must be freed by
 * the caller.
 *
 * Usually this routine will succeed.  Possible errors include:
 *  adns_s_nomemory
 *  adns_s_rrtypeunknown
 *  adns_s_invaliddata (*datap contained garbage)
 * If an error occurs then no memory has been allocated,
 * and *rrtname_r, *fmtname_r, *len_r and *data_r are undefined.
 *
 * There are some adns-invented data formats which are not official
 * master file formats.  These include:
 *
 * Mailboxes if __qtf_mail822: these are just included as-is.
 *
 * Addresses (adns_rr_addr): these may be of pretty much any type.
 * The representation is in two parts: first, a word for the address
 * family (ie, in AF_XXX, the XXX), and then one or more items for the
 * address itself, depending on the format.  For an IPv4 address the
 * syntax is INET followed by the dotted quad (from inet_ntoa).
 * Currently only IPv4 is supported.
 *
 * Text strings (as in adns_rr_txt) appear inside double quotes, and
 * use \" and \\ to represent " and \, and \xHH to represent
 * characters not in the range 32-126.
 *
 * Hostname with addresses (adns_rr_hostaddr): this consists of the
 * hostname, as usual, followed by the adns_status value, as an
 * abbreviation, and then a descriptive string (encoded as if it were
 * a piece of text), for the address lookup, followed by zero or more
 * addresses enclosed in ( and ).  If the result was a temporary
 * failure, then a single ?  appears instead of the ( ).  If the
 * result was a permanent failure then an empty pair of parentheses
 * appears (which a space in between).  For example, one of the NS
 * records for greenend.org.uk comes out like
 *  ns.chiark.greenend.org.uk ok "OK" ( INET 195.224.76.132 )
 * an MX referring to a nonexistent host might come out like:
 *  50 sun2.nsfnet-relay.ac.uk nxdomain "No such domain" ( )
 * and if nameserver information is not available you might get:
 *  dns2.spong.dyn.ml.org timeout "DNS query timed out" ?
 */

const char *adns_strerror(adns_status st);
const char *adns_errabbrev(adns_status st);
const char *adns_errtypeabbrev(adns_status st);
/* Like strerror but for adns_status values.  adns_errabbrev returns
 * the abbreviation of the error - eg, for adns_s_timeout it returns
 * "timeout".  adns_errtypeabbrev returns the abbreviation of the
 * error class: ie, for values up to adns_s_max_XXX it will return the
 * string XXX.  You MUST NOT call these functions with status values
 * not returned by the same adns library.
 */

#ifdef __cplusplus
} /* end of extern "C" */
#endif
#endif
