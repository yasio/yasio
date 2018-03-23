/*
 * types.c
 * - RR-type-specific code, and the machinery to call it
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

#include <stddef.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "internal.h"

#define R_NOMEM       return adns_s_nomemory
#define CSP_ADDSTR(s) do {			\
    if (!adns__vbuf_appendstr(vb,(s))) R_NOMEM;	\
  } while (0)

/*
 * order of sections:
 *
 * _string                    (pap)
 * _textdata, _qstring        (csp)
 * _str                       (mf,cs)
 * _intstr                    (mf,csp,cs)
 * _manyistr                  (mf,cs)
 * _txt                       (pa)
 * _inaddr                    (pa,di,cs
 *				+search_sortlist, dip_genaddr, csp_genaddr)
 * _in6addr		      (pa,di,cs)
 * _addr                      (pap,pa,di,div,csp,cs,gsz,qs
 *				+search_sortlist_sa, dip_sockaddr,
 *				 addr_rrtypes, addr_submit, icb_addr)
 * _domain                    (pap,csp,cs)
 * _dom_raw		      (pa)
 * _host_raw                  (pa)
 * _hostaddr                  (pap,pa,dip,di,mfp,mf,csp,cs
 *				+pap_findaddrs, icb_hostaddr)
 * _mx_raw                    (pa,di)
 * _mx                        (pa,di)
 * _inthostaddr               (mf,cs)
 * _inthost		      (cs)
 * _ptr                       (ckl,pa +icb_ptr)
 * _strpair                   (mf)
 * _intstrpair                (mf)
 * _hinfo                     (pa)
 * _mailbox                   (pap,csp +pap_mailbox822)
 * _rp                        (pa,cs)
 * _soa                       (pa,mf,cs)
 * _srv*                      (ckl,(pap),pa*2,mf*2,di,(csp),cs*2,postsort)
 * _byteblock                 (mf)
 * _opaque                    (pa,cs)
 * _flat                      (mf)
 *
 * within each section:
 *    ckl_*
 *    pap_*
 *    pa_*
 *    dip_*
 *    di_*
 *    mfp_*
 *    mf_*
 *    csp_*
 *    cs_*
 *    gsz_*
 *    postsort_*
 *    qs_*
 */

/*
 * _qstring               (pap,csp)
 */

static adns_status pap_qstring(const parseinfo *pai, int *cbyte_io, int max,
			      int *len_r, char **str_r) {
  /* Neither len_r nor str_r may be null.
   * End of datagram (overrun) is indicated by returning adns_s_invaliddata;
   */
  const byte *dgram= pai->dgram;
  int l, cbyte;
  char *str;

  cbyte= *cbyte_io;

  if (cbyte >= max) return adns_s_invaliddata;
  GET_B(cbyte,l);
  if (cbyte+l > max) return adns_s_invaliddata;
  
  str= adns__alloc_interim(pai->qu, l+1);
  if (!str) R_NOMEM;
  
  str[l]= 0;
  memcpy(str,dgram+cbyte,l);

  *len_r= l;
  *str_r= str;
  *cbyte_io= cbyte+l;
  
  return adns_s_ok;
}

static adns_status csp_qstring(vbuf *vb, const char *dp, int len) {
  unsigned char ch;
  char buf[10];
  int cn;

  CSP_ADDSTR("\"");
  for (cn=0; cn<len; cn++) {
    ch= *dp++;
    if (ch == '\\') {
      CSP_ADDSTR("\\\\");
    } else if (ch == '"') {
      CSP_ADDSTR("\\\"");
    } else if (ch >= 32 && ch <= 126) {
      if (!adns__vbuf_append(vb,&ch,1)) R_NOMEM;
    } else {
      sprintf(buf,"\\x%02x",ch);
      CSP_ADDSTR(buf);
    }
  }
  CSP_ADDSTR("\"");
  
  return adns_s_ok;
}

/*
 * _str  (mf)
 */

static void mf_str(adns_query qu, void *datap) {
  char **rrp= datap;

  adns__makefinal_str(qu,rrp);
}

/*
 * _intstr  (mf)
 */

static void mf_intstr(adns_query qu, void *datap) {
  adns_rr_intstr *rrp= datap;

  adns__makefinal_str(qu,&rrp->str);
}

/*
 * _manyistr   (mf)
 */

static void mf_manyistr(adns_query qu, void *datap) {
  adns_rr_intstr **rrp= datap;
  adns_rr_intstr *te, *table;
  void *tablev;
  int tc;

  for (tc=0, te= *rrp; te->i >= 0; te++, tc++);
  tablev= *rrp;
  adns__makefinal_block(qu,&tablev,sizeof(*te)*(tc+1));
  *rrp= table= tablev;
  for (te= *rrp; te->i >= 0; te++)
    adns__makefinal_str(qu,&te->str);
}

/*
 * _txt   (pa,cs)
 */

static adns_status pa_txt(const parseinfo *pai, int cbyte,
			  int max, void *datap) {
  adns_rr_intstr **rrp= datap, *table, *te;
  const byte *dgram= pai->dgram;
  int ti, tc, l, startbyte;
  adns_status st;

  startbyte= cbyte;
  if (cbyte >= max) return adns_s_invaliddata;
  tc= 0;
  while (cbyte < max) {
    GET_B(cbyte,l);
    cbyte+= l;
    tc++;
  }
  if (cbyte != max || !tc) return adns_s_invaliddata;

  table= adns__alloc_interim(pai->qu,sizeof(*table)*(tc+1));
  if (!table) R_NOMEM;

  for (cbyte=startbyte, ti=0, te=table; ti<tc; ti++, te++) {
    st= pap_qstring(pai, &cbyte, max, &te->i, &te->str);
    if (st) return st;
  }
  assert(cbyte == max);

  te->i= -1;
  te->str= 0;
  
  *rrp= table;
  return adns_s_ok;
}

static adns_status cs_txt(vbuf *vb, const void *datap) {
  const adns_rr_intstr *const *rrp= datap;
  const adns_rr_intstr *current;
  adns_status st;
  int spc;

  for (current= *rrp, spc=0;  current->i >= 0;  current++, spc=1) {
    if (spc) CSP_ADDSTR(" ");
    st= csp_qstring(vb,current->str,current->i); if (st) return st;
  }
  return adns_s_ok;
}

/*
 * _hinfo   (cs)
 */

static adns_status cs_hinfo(vbuf *vb, const void *datap) {
  const adns_rr_intstrpair *rrp= datap;
  adns_status st;

  st= csp_qstring(vb,rrp->array[0].str,rrp->array[0].i);  if (st) return st;
  CSP_ADDSTR(" ");
  st= csp_qstring(vb,rrp->array[1].str,rrp->array[1].i);  if (st) return st;
  return adns_s_ok;
}

/*
 * _inaddr   (pa,di,cs +search_sortlist, dip_genaddr, csp_genaddr)
 */

static adns_status pa_inaddr(const parseinfo *pai, int cbyte,
			     int max, void *datap) {
  struct in_addr *storeto= datap;
  
  if (max-cbyte != 4) return adns_s_invaliddata;
  memcpy(storeto, pai->dgram + cbyte, 4);
  return adns_s_ok;
}

static int search_sortlist(adns_state ads, int af, const void *ad) {
  const struct sortlist *slp;
  struct in_addr a4;
  int i;
  int v6mappedp= 0;

  if (af == AF_INET6) {
    const struct in6_addr *a6= ad;
    if (IN6_IS_ADDR_V4MAPPED(a6)) {
      a4.s_addr= htonl(((unsigned long)a6->s6_addr[12] << 24) |
		       ((unsigned long)a6->s6_addr[13] << 16) |
		       ((unsigned long)a6->s6_addr[14] <<  8) |
		       ((unsigned long)a6->s6_addr[15] <<  0));
      v6mappedp= 1;
    }
  }

  for (i=0, slp=ads->sortlist;
       i<ads->nsortlist &&
	 !adns__addr_matches(af,ad, &slp->base,&slp->mask) &&
	 !(v6mappedp &&
	   adns__addr_matches(AF_INET,&a4, &slp->base,&slp->mask));
       i++, slp++);
  return i;
}

static int dip_genaddr(adns_state ads, int af, const void *a, const void *b) {
  int ai, bi;
  
  if (!ads->nsortlist) return 0;

  ai= search_sortlist(ads,af,a);
  bi= search_sortlist(ads,af,b);
  return bi<ai;
}

static int di_inaddr(adns_state ads,
		     const void *datap_a, const void *datap_b) {
  return dip_genaddr(ads,AF_INET,datap_a,datap_b);
}

static adns_status csp_genaddr(vbuf *vb, int af, const void *p) {
  char buf[ADNS_ADDR2TEXT_BUFLEN];
  int len= sizeof(buf);
  adns_rr_addr a;
  int err;

  memset(&a, 0, sizeof(a));
  a.addr.sa.sa_family= af;
  adns__addr_inject(p, &a.addr);
  err= adns_addr2text(&a.addr.sa,0, buf,&len, 0); assert(!err);
  CSP_ADDSTR(buf);
  return adns_s_ok;
}

static adns_status cs_inaddr(vbuf *vb, const void *datap) {
  return csp_genaddr(vb, AF_INET,datap);
}

/*
 * _in6addr   (pa,di,cs)
 */

static adns_status pa_in6addr(const parseinfo *pai, int cbyte,
			     int max, void *datap) {
  struct in6_addr *storeto= datap;

  if (max-cbyte != 16) return adns_s_invaliddata;
  memcpy(storeto->s6_addr, pai->dgram + cbyte, 16);
  return adns_s_ok;
}

static int di_in6addr(adns_state ads,
		     const void *datap_a, const void *datap_b) {
  return dip_genaddr(ads,AF_INET6,datap_a,datap_b);
}

static adns_status cs_in6addr(vbuf *vb, const void *datap) {
  return csp_genaddr(vb,AF_INET6,datap);
}

/*
 * _addr   (pap,pa,di,div,csp,cs,gsz,qs
 *		+search_sortlist_sa, dip_sockaddr, addr_rrtypes,
 *		 addr_submit, icb_addr)
 */

static const typeinfo tinfo_addrsub;

#define ADDR_RRTYPES(_) _(a) _(aaaa)

static const adns_rrtype addr_all_rrtypes[] = {
#define RRTY_CODE(ty) adns_r_##ty,
  ADDR_RRTYPES(RRTY_CODE)
#undef RRTY_CODE
};

enum {
#define RRTY_INDEX(ty) addr__ri_##ty,
  ADDR_RRTYPES(RRTY_INDEX)
#undef RRTY_INDEX
  addr_nrrtypes,
#define RRTY_FLAG(ty) addr_rf_##ty = 1 << addr__ri_##ty,
  ADDR_RRTYPES(RRTY_FLAG)
  addr__rrty_eat_final_comma
#undef RRTY_FLAG
};

static unsigned addr_rrtypeflag(adns_rrtype type) {
  int i;

  type &= adns_rrt_typemask;
  for (i=0; i<addr_nrrtypes; i++)
     if (type==addr_all_rrtypes[i])
       return 1 << i;
  return 0;
}

/* About CNAME handling in addr queries.
 *
 * A user-level addr query is translated into a number of protocol-level
 * queries, and its job is to reassemble the results.  This gets tricky if
 * the answers aren't consistent.  In particular, if the answers report
 * inconsistent indirection via CNAME records (e.g., different CNAMEs, or
 * some indirect via a CNAME, and some don't) then we have trouble.
 *
 * Once we've received an answer, even if it was NODATA, we set
 * adns__qf_addr_answer on the parent query.  This will let us detect a
 * conflict between a no-CNAME-with-NODATA reply and a subsequent CNAME.
 *
 * If we detect a conflict of any kind, then at least one answer came back
 * with a CNAME record, so we pick the first such answer (somewhat
 * arbitrarily) as being the `right' canonical name, and set this in the
 * parent query's answer->cname slot.  We discard address records from the
 * wrong name.  And finally we cancel the outstanding child queries, and
 * resubmit address queries for the address families we don't yet have, with
 * adns__qf_addr_cname set so that we know that we're in the fixup state.
 */

static adns_status pap_addr(const parseinfo *pai, int in_rrty, size_t out_rrsz,
			    int *cbyte_io, int cbyte_max, adns_rr_addr *out) {
  int in_addrlen;
  int out_af, out_salen;
  struct in6_addr v6map;

  const void *use_addr= pai->dgram + *cbyte_io;

  switch (in_rrty) {
  case adns_r_a:    in_addrlen= 4;  out_af= AF_INET;  break;
  case adns_r_aaaa: in_addrlen= 16; out_af= AF_INET6; break;
  default: abort();
  }

  if ((*cbyte_io + in_addrlen) != cbyte_max) return adns_s_invaliddata;

  if (out_af==AF_INET &&
      (pai->qu->flags & adns_qf_ipv6_mapv4) &&
      (pai->qu->answer->type & adns__qtf_bigaddr)) {
    memset(v6map.s6_addr +  0, 0x00,    10);
    memset(v6map.s6_addr + 10, 0xff,     2);
    memcpy(v6map.s6_addr + 12, use_addr, 4);
    use_addr= v6map.s6_addr;
    out_af= AF_INET6;
  }

  switch (out_af) {
  case AF_INET:  out_salen= sizeof(out->addr.inet);  break;
  case AF_INET6: out_salen= sizeof(out->addr.inet6); break;
  default: abort();
  }

  assert(offsetof(adns_rr_addr, addr) + out_salen <= out_rrsz);

  memset(&out->addr, 0, out_salen);
  out->len= out_salen;
  out->addr.sa.sa_family= out_af;
  adns__addr_inject(use_addr, &out->addr);

  *cbyte_io += in_addrlen;
  return adns_s_ok;
}

static adns_status pa_addr(const parseinfo *pai, int cbyte,
			   int max, void *datap) {
  int err= pap_addr(pai, pai->qu->answer->type & adns_rrt_typemask,
		    pai->qu->answer->rrsz, &cbyte, max, datap);
  if (err) return err;
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static int search_sortlist_sa(adns_state ads, const struct sockaddr *sa) {
  const void *pa = adns__sockaddr_addr(sa);
  return search_sortlist(ads, sa->sa_family, pa);
}

static int dip_sockaddr(adns_state ads,
			const struct sockaddr *sa,
			const struct sockaddr *sb) {
  if (!ads->sortlist) return 0;
  return search_sortlist_sa(ads, sa) > search_sortlist_sa(ads, sb);
}

static int di_addr(adns_state ads, const void *datap_a, const void *datap_b) {
  const adns_rr_addr *ap= datap_a, *bp= datap_b;
  return dip_sockaddr(ads, &ap->addr.sa, &bp->addr.sa);
}

static int div_addr(void *context, const void *datap_a, const void *datap_b) {
  const adns_state ads= context;

  return di_addr(ads, datap_a, datap_b);
}		     

static adns_status csp_addr(vbuf *vb, const adns_rr_addr *rrp) {
  char buf[ADNS_ADDR2TEXT_BUFLEN];
  int len= sizeof(buf);
  int err;

  switch (rrp->addr.inet.sin_family) {
  case AF_INET:
    CSP_ADDSTR("INET ");
    goto a2t;
  case AF_INET6:
    CSP_ADDSTR("INET6 ");
    goto a2t;
  a2t:
    err= adns_addr2text(&rrp->addr.sa,0, buf,&len, 0); assert(!err);
    CSP_ADDSTR(buf);
    break;
  default:
    sprintf(buf,"AF=%u",rrp->addr.sa.sa_family);
    CSP_ADDSTR(buf);
    break;
  }
  return adns_s_ok;
}

static adns_status cs_addr(vbuf *vb, const void *datap) {
  const adns_rr_addr *rrp= datap;

  return csp_addr(vb,rrp);
}

static int gsz_addr(const typeinfo *typei, adns_rrtype type) {
  return type & adns__qtf_bigaddr ?
    sizeof(adns_rr_addr) : sizeof(adns_rr_addr_v4only);
}

static unsigned addr_rrtypes(adns_state ads, adns_rrtype type,
			     adns_queryflags qf) {
  /* Return a mask of addr_rf_... flags indicating which address families are
   * wanted, given a query type and flags.
   */

  adns_queryflags permitaf= 0;
  unsigned want= 0;

  if (!(type & adns__qtf_bigaddr))
    qf= (qf & ~adns_qf_want_allaf) | adns_qf_want_ipv4;
  else {
    if (!(qf & adns_qf_want_allaf)) {
      qf |= (type & adns__qtf_manyaf) ?
	adns_qf_want_allaf : adns_qf_want_ipv4;
    }
    if (ads->iflags & adns_if_permit_ipv4) permitaf |= adns_qf_want_ipv4;
    if (ads->iflags & adns_if_permit_ipv6) permitaf |= adns_qf_want_ipv6;
    if (qf & permitaf) qf &= permitaf | ~adns_qf_want_allaf;
  }

  if (qf & adns_qf_want_ipv4) want |= addr_rf_a;
  if (qf & adns_qf_want_ipv6) want |= addr_rf_aaaa;

  return want;
}

static void icb_addr(adns_query parent, adns_query child);

static void addr_subqueries(adns_query qu, struct timeval now,
			    adns_queryflags qf_extra,
			    const byte *qd_dgram, int qd_dglen) {
  int i, err, id;
  adns_query cqu;
  adns_queryflags qf= (qu->flags & ~adns_qf_search) | qf_extra;
  adns_rrtype qtf= qu->answer->type & adns__qtf_deref;
  unsigned which= qu->ctx.tinfo.addr.want & ~qu->ctx.tinfo.addr.have;
  qcontext ctx;

  memset(&ctx, 0, sizeof(ctx));
  ctx.callback= icb_addr;
  for (i=0; i<addr_nrrtypes; i++) {
    if (!(which & (1 << i))) continue;
    err= adns__mkquery_frdgram(qu->ads, &qu->vb, &id, qd_dgram,qd_dglen,
			       DNS_HDRSIZE, addr_all_rrtypes[i], qf);
    if (err) goto x_error;
    err= adns__internal_submit(qu->ads, &cqu, qu, &tinfo_addrsub,
			       addr_all_rrtypes[i] | qtf,
			       &qu->vb, id, qf, now, &ctx);
    if (err) goto x_error;
    cqu->answer->rrsz= qu->answer->rrsz;
  }
  qu->state= query_childw;
  LIST_LINK_TAIL(qu->ads->childw, qu);
  return;

x_error:
  adns__query_fail(qu, err);
}

static adns_status addr_submit(adns_query parent, adns_query *query_r,
			       vbuf *qumsg_vb, int id, unsigned want,
			       adns_queryflags flags, struct timeval now,
			       qcontext *ctx) {
  /* This is effectively a substitute for adns__internal_submit, intended for
   * the case where the caller (possibly) only wants a subset of the
   * available record types.  The memory management and callback rules are
   * the same as for adns__internal_submit.
   *
   * Some differences: the query is linked onto the parent's children
   * list before exit (though the parent's state is not changed, and
   * it is not linked into the childw list queue); and we set the
   * `tinfo' portion of the context structure (yes, modifying *ctx),
   * since this is, in fact, the main purpose of this function.
   */

  adns_state ads= parent->ads;
  adns_query qu;
  adns_status err;
  adns_rrtype type= ((adns_r_addr & adns_rrt_reprmask) |
		     (parent->answer->type & ~adns_rrt_reprmask));

  ctx->tinfo.addr.want= want;
  ctx->tinfo.addr.have= 0;
  err= adns__internal_submit(ads, &qu, parent, adns__findtype(adns_r_addr),
			     type, qumsg_vb, id, flags, now, ctx);
  if (err) return err;

  *query_r= qu;
  return adns_s_ok;
}

static adns_status append_addrs(adns_query qu, size_t rrsz,
				adns_rr_addr **dp, int *dlen,
				const adns_rr_addr *sp, int slen) {
  /* Append a vector of slen addr records, each of size rrsz, starting at ap,
   * to a vector starting at *dp, of length *dlen.  On successful completion,
   * *dp and *dlen are updated.
   */

  size_t drrsz= *dlen*rrsz, srrsz= slen*rrsz;
  byte *p;

  if (!slen) return adns_s_ok;
  p= adns__alloc_interim(qu, drrsz + srrsz);
  if (!p) R_NOMEM;
  if (*dlen) {
    memcpy(p, *dp, drrsz);
    adns__free_interim(qu, *dp);
  }
  memcpy(p + drrsz, sp, srrsz);
  *dlen += slen;
  *dp= (adns_rr_addr *)p;
  return adns_s_ok;
}

static void propagate_ttl(adns_query to, adns_query from)
  { if (to->expires > from->expires) to->expires= from->expires; }

static adns_status copy_cname_from_child(adns_query parent, adns_query child) {
  adns_answer *pans= parent->answer, *cans= child->answer;
  size_t n= strlen(cans->cname) + 1;

  pans->cname= adns__alloc_preserved(parent, n);
  if (!pans->cname) R_NOMEM;
  memcpy(pans->cname, cans->cname, n);
  return adns_s_ok;
}

static void done_addr_type(adns_query qu, adns_rrtype type) {
  unsigned f= addr_rrtypeflag(type);
  assert(f); qu->ctx.tinfo.addr.have |= f;
}

static void icb_addr(adns_query parent, adns_query child) {
  adns_state ads= parent->ads;
  adns_answer *pans= parent->answer, *cans= child->answer;
  struct timeval now;
  adns_status err;
  adns_queryflags qf;
  int id, r;

  propagate_ttl(parent, child);

  if (!(child->flags & adns__qf_addr_cname) &&
      (parent->flags & adns__qf_addr_answer) &&
      (!!pans->cname != !!cans->cname ||
       (pans->cname && strcmp(pans->cname, cans->cname)))) {
    /* We've detected an inconsistency in CNAME records, and must deploy
     * countermeasures.
     */

    if (!pans->cname) {
      /* The child has a CNAME record, but the parent doesn't.  We must
       * discard all of the parent's addresses, and substitute the child's.
       */

      assert(pans->rrsz == cans->rrsz);
      adns__free_interim(parent, pans->rrs.bytes);
      adns__transfer_interim(child, parent, cans->rrs.bytes);
      pans->rrs.bytes= cans->rrs.bytes;
      pans->nrrs= cans->nrrs;
      parent->ctx.tinfo.addr.have= 0;
      done_addr_type(parent, cans->type);
      err= copy_cname_from_child(parent, child); if (err) goto x_err;
    }

    /* We've settled on the CNAME (now) associated with the parent, which
     * already has appropriate address records.  Build a query datagram for
     * this name so that we can issue child queries for the missing address
     * families.  The child's vbuf looks handy for this.
     */
    err= adns__mkquery(ads, &child->vb, &id, pans->cname,
		       strlen(pans->cname), &tinfo_addrsub,
		       adns_r_addr, parent->flags);
    if (err) goto x_err;

    /* Now cancel the remaining children, and try again with the CNAME we've
     * settled on.
     */
    adns__cancel_children(parent);
    r= gettimeofday(&now, 0);  if (r) goto x_gtod;
    qf= adns__qf_addr_cname;
    if (!(parent->flags & adns_qf_cname_loose)) qf |= adns_qf_cname_forbid;
    addr_subqueries(parent, now, qf, child->vb.buf, child->vb.used);
    return;
  }

  if (cans->cname && !pans->cname) {
    err= copy_cname_from_child(parent, child);
    if (err) goto x_err;
  }

  if ((parent->flags & adns_qf_search) &&
      !pans->cname && cans->status == adns_s_nxdomain) {
    /* We're searching a list of suffixes, and the name doesn't exist.  Try
     * the next one.
     */

    adns__cancel_children(parent);
    adns__free_interim(parent, pans->rrs.bytes);
    pans->rrs.bytes= 0; pans->nrrs= 0;
    r= gettimeofday(&now, 0);  if (r) goto x_gtod;
    adns__search_next(ads, parent, now);
    return;
  }

  if (cans->status && cans->status != adns_s_nodata)
    { err= cans->status; goto x_err; }

  assert(pans->rrsz == cans->rrsz);
  err= append_addrs(parent, pans->rrsz,
		    &pans->rrs.addr, &pans->nrrs,
		    cans->rrs.addr, cans->nrrs);
  if (err) goto x_err;
  done_addr_type(parent, cans->type);

  if (parent->children.head) LIST_LINK_TAIL(ads->childw, parent);
  else if (!pans->nrrs) adns__query_fail(parent, adns_s_nodata);
  else adns__query_done(parent);
  parent->flags |= adns__qf_addr_answer;
  return;

x_gtod:
  /* We have our own error handling, because adns__must_gettimeofday
   * handles errors by calling adns_globalsystemfailure, which would
   * reenter the query processing logic. */
  adns__diag(ads, -1, parent, "gettimeofday failed: %s", strerror(errno));
  err= adns_s_systemfail;
  goto x_err;

x_err:
  adns__query_fail(parent, err);
}

static void qs_addr(adns_query qu, struct timeval now) {
  if (!qu->ctx.tinfo.addr.want) {
    qu->ctx.tinfo.addr.want= addr_rrtypes(qu->ads, qu->answer->type,
					  qu->flags);
    qu->ctx.tinfo.addr.have= 0;
  }
  addr_subqueries(qu, now, 0, qu->query_dgram, qu->query_dglen);
}

/*
 * _domain      (pap,csp,cs)
 * _dom_raw     (pa)
 */

static adns_status pap_domain(const parseinfo *pai, int *cbyte_io, int max,
			      char **domain_r, parsedomain_flags flags) {
  adns_status st;
  char *dm;
  
  st= adns__parse_domain(pai->qu->ads, pai->serv, pai->qu, &pai->qu->vb, flags,
			 pai->dgram,pai->dglen, cbyte_io, max);
  if (st) return st;
  if (!pai->qu->vb.used) return adns_s_invaliddata;

  dm= adns__alloc_interim(pai->qu, pai->qu->vb.used+1);
  if (!dm) R_NOMEM;

  dm[pai->qu->vb.used]= 0;
  memcpy(dm,pai->qu->vb.buf,pai->qu->vb.used);
  
  *domain_r= dm;
  return adns_s_ok;
}

static adns_status csp_domain(vbuf *vb, const char *domain) {
  CSP_ADDSTR(domain);
  if (!*domain) CSP_ADDSTR(".");
  return adns_s_ok;
}

static adns_status cs_domain(vbuf *vb, const void *datap) {
  const char *const *domainp= datap;
  return csp_domain(vb,*domainp);
}

static adns_status pa_dom_raw(const parseinfo *pai, int cbyte,
			      int max, void *datap) {
  char **rrp= datap;
  adns_status st;

  st= pap_domain(pai, &cbyte, max, rrp, pdf_quoteok);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

/*
 * _host_raw   (pa)
 */

static adns_status pa_host_raw(const parseinfo *pai, int cbyte,
			       int max, void *datap) {
  char **rrp= datap;
  adns_status st;

  st= pap_domain(pai, &cbyte, max, rrp,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

/*
 * _hostaddr   (pap,pa,dip,di,mfp,mf,csp,cs +pap_findaddrs, icb_hostaddr)
 */

static adns_status pap_findaddrs(const parseinfo *pai, adns_rr_hostaddr *ha,
				 unsigned *want_io, size_t addrsz,
				 int *cbyte_io, int count, int dmstart) {
  int rri, naddrs;
  unsigned typef, want= *want_io, need= want;
  int type, class, rdlen, rdend, rdstart, ownermatched;
  unsigned long ttl;
  adns_status st;
  
  for (rri=0, naddrs=0; rri<count; rri++) {
    st= adns__findrr_anychk(pai->qu, pai->serv, pai->dgram,
			    pai->dglen, cbyte_io,
			    &type, &class, &ttl, &rdlen, &rdstart,
			    pai->dgram, pai->dglen, dmstart, &ownermatched);
    if (st) return st;
    if (!ownermatched || class != DNS_CLASS_IN) continue;
    typef= addr_rrtypeflag(type);
    if (!(want & typef)) continue;
    need &= ~typef;
    if (!adns__vbuf_ensure(&pai->qu->vb, (naddrs+1)*addrsz)) R_NOMEM;
    adns__update_expires(pai->qu,ttl,pai->now);
    rdend= rdstart + rdlen;
    st= pap_addr(pai, type, addrsz, &rdstart, rdend,
		 (adns_rr_addr *)(pai->qu->vb.buf + naddrs*addrsz));
    if (st) return st;
    if (rdstart != rdend) return adns_s_invaliddata;
    naddrs++;
  }
  if (naddrs > 0) {
    st= append_addrs(pai->qu, addrsz, &ha->addrs, &ha->naddrs,
		     (const adns_rr_addr *)pai->qu->vb.buf, naddrs);
    if (st) return st;
    ha->astatus= adns_s_ok;

    if (!need) {
      adns__isort(ha->addrs, naddrs, addrsz, pai->qu->vb.buf,
		  div_addr, pai->ads);
    }
  }
  *want_io= need;
  return adns_s_ok;
}

static void icb_hostaddr(adns_query parent, adns_query child) {
  adns_answer *cans= child->answer;
  adns_rr_hostaddr *rrp= child->ctx.pinfo.hostaddr;
  adns_state ads= parent->ads;
  adns_status st;
  size_t addrsz= gsz_addr(0, parent->answer->type);

  st= cans->status == adns_s_nodata ? adns_s_ok : cans->status;
  if (st) goto done;
  propagate_ttl(parent, child);

  assert(addrsz == cans->rrsz);
  st= append_addrs(parent, addrsz,
		   &rrp->addrs, &rrp->naddrs,
		   cans->rrs.addr, cans->nrrs);
  if (st) goto done;
  if (!rrp->naddrs) { st= adns_s_nodata; goto done; }

  if (!adns__vbuf_ensure(&parent->vb, addrsz))
    { st= adns_s_nomemory; goto done; }
  adns__isort(rrp->addrs, rrp->naddrs, addrsz, parent->vb.buf,
	      div_addr, ads);

done:
  if (st) {
    adns__free_interim(parent, rrp->addrs);
    rrp->naddrs= (st>0 && st<=adns_s_max_tempfail) ? -1 : 0;
  }

  rrp->astatus= st;
  if (parent->children.head) {
    LIST_LINK_TAIL(ads->childw,parent);
  } else {
    adns__query_done(parent);
  }
}

static adns_status pap_hostaddr(const parseinfo *pai, int *cbyte_io,
				int max, adns_rr_hostaddr *rrp) {
  adns_status st;
  int dmstart, cbyte;
  qcontext ctx;
  int id;
  adns_query nqu;
  adns_queryflags nflags;
  unsigned want;
  size_t addrsz= gsz_addr(0, pai->qu->answer->type);

  dmstart= cbyte= *cbyte_io;
  st= pap_domain(pai, &cbyte, max, &rrp->host,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  *cbyte_io= cbyte;

  rrp->astatus= adns_s_ok;
  rrp->naddrs= 0;
  rrp->addrs= 0;

  cbyte= pai->nsstart;

  want= addr_rrtypes(pai->ads, pai->qu->answer->type, pai->qu->flags);

  st= pap_findaddrs(pai, rrp, &want, addrsz, &cbyte, pai->nscount, dmstart);
  if (st) return st;
  if (!want) return adns_s_ok;

  st= pap_findaddrs(pai, rrp, &want, addrsz, &cbyte, pai->arcount, dmstart);
  if (st) return st;
  if (!want) return adns_s_ok;

  st= adns__mkquery_frdgram(pai->ads, &pai->qu->vb, &id,
			    pai->dgram, pai->dglen, dmstart,
			    adns_r_addr, adns_qf_quoteok_query);
  if (st) return st;

  ctx.ext= 0;
  ctx.callback= icb_hostaddr;
  ctx.pinfo.hostaddr= rrp;
  
  nflags= adns_qf_quoteok_query | (pai->qu->flags & (adns_qf_want_allaf |
						     adns_qf_ipv6_mapv4));
  if (!(pai->qu->flags & adns_qf_cname_loose)) nflags |= adns_qf_cname_forbid;
  
  st= addr_submit(pai->qu, &nqu, &pai->qu->vb, id, want,
		  nflags, pai->now, &ctx);
  if (st) return st;

  return adns_s_ok;
}

static adns_status pa_hostaddr(const parseinfo *pai, int cbyte,
			       int max, void *datap) {
  adns_rr_hostaddr *rrp= datap;
  adns_status st;

  st= pap_hostaddr(pai, &cbyte, max, rrp);
  if (st) return st;
  if (cbyte != max) return adns_s_invaliddata;

  return adns_s_ok;
}

static int dip_hostaddr(adns_state ads,
			const adns_rr_hostaddr *ap, const adns_rr_hostaddr *bp) {
  if (ap->astatus != bp->astatus) return ap->astatus;
  if (ap->astatus) return 0;

  return dip_sockaddr(ads, &ap->addrs[0].addr.sa, &bp->addrs[0].addr.sa);
}

static int di_hostaddr(adns_state ads,
		       const void *datap_a, const void *datap_b) {
  const adns_rr_hostaddr *ap= datap_a, *bp= datap_b;

  return dip_hostaddr(ads, ap,bp);
}

static void mfp_hostaddr(adns_query qu, adns_rr_hostaddr *rrp) {
  void *tablev;
  size_t addrsz= gsz_addr(0, qu->answer->type);

  adns__makefinal_str(qu,&rrp->host);
  tablev= rrp->addrs;
  adns__makefinal_block(qu, &tablev, rrp->naddrs*addrsz);
  rrp->addrs= tablev;
}

static void mf_hostaddr(adns_query qu, void *datap) {
  adns_rr_hostaddr *rrp= datap;

  mfp_hostaddr(qu,rrp);
}

static adns_status csp_hostaddr(vbuf *vb, const adns_rr_hostaddr *rrp) {
  const char *errstr;
  adns_status st;
  char buf[20];
  int i;

  st= csp_domain(vb,rrp->host);  if (st) return st;

  CSP_ADDSTR(" ");
  CSP_ADDSTR(adns_errtypeabbrev(rrp->astatus));

  sprintf(buf," %d ",rrp->astatus);
  CSP_ADDSTR(buf);

  CSP_ADDSTR(adns_errabbrev(rrp->astatus));
  CSP_ADDSTR(" ");

  errstr= adns_strerror(rrp->astatus);
  st= csp_qstring(vb,errstr,strlen(errstr));  if (st) return st;
  
  if (rrp->naddrs >= 0) {
    CSP_ADDSTR(" (");
    for (i=0; i<rrp->naddrs; i++) {
      CSP_ADDSTR(" ");
      st= csp_addr(vb,&rrp->addrs[i]);
    }
    CSP_ADDSTR(" )");
  } else {
    CSP_ADDSTR(" ?");
  }
  return adns_s_ok;
}

static adns_status cs_hostaddr(vbuf *vb, const void *datap) {
  const adns_rr_hostaddr *rrp= datap;

  return csp_hostaddr(vb,rrp);
}

/*
 * _mx_raw   (pa,di)
 */

static adns_status pa_mx_raw(const parseinfo *pai, int cbyte,
			     int max, void *datap) {
  const byte *dgram= pai->dgram;
  adns_rr_intstr *rrp= datap;
  adns_status st;
  int pref;

  if (cbyte+2 > max) return adns_s_invaliddata;
  GET_W(cbyte,pref);
  rrp->i= pref;
  st= pap_domain(pai, &cbyte, max, &rrp->str,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static int di_mx_raw(adns_state ads, const void *datap_a, const void *datap_b) {
  const adns_rr_intstr *ap= datap_a, *bp= datap_b;

  if (ap->i < bp->i) return 0;
  if (ap->i > bp->i) return 1;
  return 0;
}

/*
 * _mx   (pa,di)
 */

static adns_status pa_mx(const parseinfo *pai, int cbyte,
			 int max, void *datap) {
  const byte *dgram= pai->dgram;
  adns_rr_inthostaddr *rrp= datap;
  adns_status st;
  int pref;

  if (cbyte+2 > max) return adns_s_invaliddata;
  GET_W(cbyte,pref);
  rrp->i= pref;
  st= pap_hostaddr(pai, &cbyte, max, &rrp->ha);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static int di_mx(adns_state ads, const void *datap_a, const void *datap_b) {
  const adns_rr_inthostaddr *ap= datap_a, *bp= datap_b;

  if (ap->i < bp->i) return 0;
  if (ap->i > bp->i) return 1;
  return dip_hostaddr(ads, &ap->ha, &bp->ha);
}

/*
 * _inthostaddr  (mf,cs)
 */

static void mf_inthostaddr(adns_query qu, void *datap) {
  adns_rr_inthostaddr *rrp= datap;

  mfp_hostaddr(qu,&rrp->ha);
}

static adns_status cs_inthostaddr(vbuf *vb, const void *datap) {
  const adns_rr_inthostaddr *rrp= datap;
  char buf[10];

  sprintf(buf,"%u ",rrp->i);
  CSP_ADDSTR(buf);

  return csp_hostaddr(vb,&rrp->ha);
}

/*
 * _inthost  (cs)
 */

static adns_status cs_inthost(vbuf *vb, const void *datap) {
  const adns_rr_intstr *rrp= datap;
  char buf[10];

  sprintf(buf,"%u ",rrp->i);
  CSP_ADDSTR(buf);
  return csp_domain(vb,rrp->str);
}

/*
 * _ptr   (ckl,pa +icb_ptr)
 */

static adns_status ckl_ptr(adns_state ads, adns_queryflags flags,
			   union checklabel_state *cls, qcontext *ctx,
			   int labnum, const char *dgram,
			   int labstart, int lablen) {
  if (lablen) {
    if (!adns__revparse_label(&cls->ptr, labnum, dgram,labstart,lablen))
      return adns_s_querydomainwrong;
  } else {
    if (!adns__revparse_done(&cls->ptr, dgram, labnum,
			     &ctx->tinfo.ptr.rev_rrtype,
			     &ctx->tinfo.ptr.addr))
      return adns_s_querydomainwrong;
  }
  return adns_s_ok;
}

static void icb_ptr(adns_query parent, adns_query child) {
  adns_answer *cans= child->answer;
  const adns_sockaddr *queried;
  const unsigned char *found;
  adns_state ads= parent->ads;
  int i;

  if (cans->status == adns_s_nxdomain || cans->status == adns_s_nodata) {
    adns__query_fail(parent,adns_s_inconsistent);
    return;
  } else if (cans->status) {
    adns__query_fail(parent,cans->status);
    return;
  }

  queried= &parent->ctx.tinfo.ptr.addr;
  for (i=0, found=cans->rrs.bytes; i<cans->nrrs; i++, found+=cans->rrsz) {
    if (adns__addrs_equal_raw(&queried->sa,
			  parent->ctx.tinfo.ptr.addr.sa.sa_family,found)) {
      if (!parent->children.head) {
	adns__query_done(parent);
	return;
      } else {
	LIST_LINK_TAIL(ads->childw,parent);
	return;
      }
    }
  }

  adns__query_fail(parent,adns_s_inconsistent);
}

static adns_status pa_ptr(const parseinfo *pai, int dmstart,
			  int max, void *datap) {
  char **rrp= datap;
  adns_status st;
  adns_rrtype rrtype= pai->qu->ctx.tinfo.ptr.rev_rrtype;
  int cbyte, id;
  adns_query nqu;
  qcontext ctx;

  cbyte= dmstart;
  st= pap_domain(pai, &cbyte, max, rrp,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  if (cbyte != max) return adns_s_invaliddata;

  st= adns__mkquery_frdgram(pai->ads, &pai->qu->vb, &id,
			    pai->dgram, pai->dglen, dmstart,
			    rrtype, adns_qf_quoteok_query);
  if (st) return st;

  ctx.ext= 0;
  ctx.callback= icb_ptr;
  memset(&ctx.pinfo,0,sizeof(ctx.pinfo));
  memset(&ctx.tinfo,0,sizeof(ctx.tinfo));
  st= adns__internal_submit(pai->ads, &nqu, pai->qu,
			    adns__findtype(rrtype),
			    rrtype, &pai->qu->vb, id,
			    adns_qf_quoteok_query, pai->now, &ctx);
  if (st) return st;

  return adns_s_ok;
}

/*
 * _strpair   (mf)
 */

static void mf_strpair(adns_query qu, void *datap) {
  adns_rr_strpair *rrp= datap;

  adns__makefinal_str(qu,&rrp->array[0]);
  adns__makefinal_str(qu,&rrp->array[1]);
}

/*
 * _intstrpair   (mf)
 */

static void mf_intstrpair(adns_query qu, void *datap) {
  adns_rr_intstrpair *rrp= datap;

  adns__makefinal_str(qu,&rrp->array[0].str);
  adns__makefinal_str(qu,&rrp->array[1].str);
}

/*
 * _hinfo   (pa)
 */

static adns_status pa_hinfo(const parseinfo *pai, int cbyte,
			    int max, void *datap) {
  adns_rr_intstrpair *rrp= datap;
  adns_status st;
  int i;

  for (i=0; i<2; i++) {
    st= pap_qstring(pai, &cbyte, max, &rrp->array[i].i, &rrp->array[i].str);
    if (st) return st;
  }

  if (cbyte != max) return adns_s_invaliddata;
  
  return adns_s_ok;
}

/*
 * _mailbox   (pap,cs +pap_mailbox822)
 */

static adns_status pap_mailbox822(const parseinfo *pai,
				  int *cbyte_io, int max, char **mb_r) {
  int lablen, labstart, i, needquote, c, r, neednorm;
  const unsigned char *p;
  char *str;
  findlabel_state fls;
  adns_status st;
  vbuf *vb;

  vb= &pai->qu->vb;
  vb->used= 0;
  adns__findlabel_start(&fls, pai->ads,
			-1, pai->qu,
			pai->dgram, pai->dglen, max,
			*cbyte_io, cbyte_io);
  st= adns__findlabel_next(&fls,&lablen,&labstart);
  if (!lablen) {
    adns__vbuf_appendstr(vb,".");
    goto x_ok;
  }

  neednorm= 1;
  for (i=0, needquote=0, p= pai->dgram+labstart; i<lablen; i++) {
    c= *p++;
    if ((c&~128) < 32 || (c&~128) == 127) return adns_s_invaliddata;
    if (c == '.' && !neednorm) neednorm= 1;
    else if (c==' ' || c>=127 || ctype_822special(c)) needquote++;
    else neednorm= 0;
  }

  if (needquote || neednorm) {
    r= adns__vbuf_ensure(vb, lablen+needquote+4); if (!r) R_NOMEM;
    adns__vbuf_appendq(vb,"\"",1);
    for (i=0, needquote=0, p= pai->dgram+labstart; i<lablen; i++, p++) {
      c= *p;
      if (c == '"' || c=='\\') adns__vbuf_appendq(vb,"\\",1);
      adns__vbuf_appendq(vb,p,1);
    }
    adns__vbuf_appendq(vb,"\"",1);
  } else {
    r= adns__vbuf_append(vb, pai->dgram+labstart, lablen); if (!r) R_NOMEM;
  }

  r= adns__vbuf_appendstr(vb,"@"); if (!r) R_NOMEM;

  st= adns__parse_domain_more(&fls,pai->ads, pai->qu,vb,0, pai->dgram);
  if (st) return st;

 x_ok:
  str= adns__alloc_interim(pai->qu, vb->used+1); if (!str) R_NOMEM;
  memcpy(str,vb->buf,vb->used);
  str[vb->used]= 0;
  *mb_r= str;
  return adns_s_ok;
}

static adns_status pap_mailbox(const parseinfo *pai, int *cbyte_io, int max,
			       char **mb_r) {
  if (pai->qu->typei->typekey & adns__qtf_mail822) {
    return pap_mailbox822(pai, cbyte_io, max, mb_r);
  } else {
    return pap_domain(pai, cbyte_io, max, mb_r, pdf_quoteok);
  }
}

static adns_status csp_mailbox(vbuf *vb, const char *mailbox) {
  return csp_domain(vb,mailbox);
}

/*
 * _rp   (pa,cs)
 */

static adns_status pa_rp(const parseinfo *pai, int cbyte,
			 int max, void *datap) {
  adns_rr_strpair *rrp= datap;
  adns_status st;

  st= pap_mailbox(pai, &cbyte, max, &rrp->array[0]);
  if (st) return st;

  st= pap_domain(pai, &cbyte, max, &rrp->array[1], pdf_quoteok);
  if (st) return st;

  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static adns_status cs_rp(vbuf *vb, const void *datap) {
  const adns_rr_strpair *rrp= datap;
  adns_status st;

  st= csp_mailbox(vb,rrp->array[0]);  if (st) return st;
  CSP_ADDSTR(" ");
  st= csp_domain(vb,rrp->array[1]);  if (st) return st;

  return adns_s_ok;
}  

/*
 * _soa   (pa,mf,cs)
 */

static adns_status pa_soa(const parseinfo *pai, int cbyte,
			  int max, void *datap) {
  adns_rr_soa *rrp= datap;
  const byte *dgram= pai->dgram;
  adns_status st;
  int msw, lsw, i;

  st= pap_domain(pai, &cbyte, max, &rrp->mname,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;

  st= pap_mailbox(pai, &cbyte, max, &rrp->rname);
  if (st) return st;

  if (cbyte+20 != max) return adns_s_invaliddata;
  
  for (i=0; i<5; i++) {
    GET_W(cbyte,msw);
    GET_W(cbyte,lsw);
    (&rrp->serial)[i]= (msw<<16) | lsw;
  }

  return adns_s_ok;
}

static void mf_soa(adns_query qu, void *datap) {
  adns_rr_soa *rrp= datap;

  adns__makefinal_str(qu,&rrp->mname);
  adns__makefinal_str(qu,&rrp->rname);
}

static adns_status cs_soa(vbuf *vb, const void *datap) {
  const adns_rr_soa *rrp= datap;
  char buf[20];
  int i;
  adns_status st;
  
  st= csp_domain(vb,rrp->mname);  if (st) return st;
  CSP_ADDSTR(" ");
  st= csp_mailbox(vb,rrp->rname);  if (st) return st;

  for (i=0; i<5; i++) {
    sprintf(buf," %lu",(&rrp->serial)[i]);
    CSP_ADDSTR(buf);
  }

  return adns_s_ok;
}

/*
 * _srv*  (ckl,(pap),pa*2,mf*2,di,(csp),cs*2,postsort)
 */

static adns_status ckl_srv(adns_state ads, adns_queryflags flags,
			   union checklabel_state *cls, qcontext *ctx,
			   int labnum, const char *dgram,
			   int labstart, int lablen) {
  const char *label = dgram+labstart;
  if (labnum < 2) {
    if (flags & adns_qf_quoteok_query) return adns_s_ok;
    if (!lablen || label[0] != '_') return adns_s_querydomaininvalid;
    return adns_s_ok;
  }
  return adns__ckl_hostname(ads,flags, cls,ctx, labnum, dgram,labstart,lablen);
}

static adns_status pap_srv_begin(const parseinfo *pai, int *cbyte_io, int max,
				 adns_rr_srvha *rrp
				   /* might be adns_rr_srvraw* */) {
  const byte *dgram= pai->dgram;
  int ti, cbyte;

  cbyte= *cbyte_io;
  if ((*cbyte_io += 6) > max) return adns_s_invaliddata;
  
  rrp->priority= GET_W(cbyte, ti);
  rrp->weight=   GET_W(cbyte, ti);
  rrp->port=     GET_W(cbyte, ti);
  return adns_s_ok;
}

static adns_status pa_srvraw(const parseinfo *pai, int cbyte,
			     int max, void *datap) {
  adns_rr_srvraw *rrp= datap;
  adns_status st;

  st= pap_srv_begin(pai,&cbyte,max,datap);
  if (st) return st;
  
  st= pap_domain(pai, &cbyte, max, &rrp->host,
		 pai->qu->flags & adns_qf_quoteok_anshost ? pdf_quoteok : 0);
  if (st) return st;
  
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static adns_status pa_srvha(const parseinfo *pai, int cbyte,
			    int max, void *datap) {
  adns_rr_srvha *rrp= datap;
  adns_status st;

  st= pap_srv_begin(pai,&cbyte,max,datap);       if (st) return st;
  st= pap_hostaddr(pai, &cbyte, max, &rrp->ha);  if (st) return st;
  if (cbyte != max) return adns_s_invaliddata;
  return adns_s_ok;
}

static void mf_srvraw(adns_query qu, void *datap) {
  adns_rr_srvraw *rrp= datap;
  adns__makefinal_str(qu, &rrp->host);
}

static void mf_srvha(adns_query qu, void *datap) {
  adns_rr_srvha *rrp= datap;
  mfp_hostaddr(qu,&rrp->ha);
}

static int di_srv(adns_state ads, const void *datap_a, const void *datap_b) {
  const adns_rr_srvraw *ap= datap_a, *bp= datap_b;
    /* might be const adns_rr_svhostaddr* */

  if (ap->priority < bp->priority) return 0;
  if (ap->priority > bp->priority) return 1;
  return 0;
}

static adns_status csp_srv_begin(vbuf *vb, const adns_rr_srvha *rrp
				   /* might be adns_rr_srvraw* */) {
  char buf[30];
  sprintf(buf,"%u %u %u ", rrp->priority, rrp->weight, rrp->port);
  CSP_ADDSTR(buf);
  return adns_s_ok;
}

static adns_status cs_srvraw(vbuf *vb, const void *datap) {
  const adns_rr_srvraw *rrp= datap;
  adns_status st;
  
  st= csp_srv_begin(vb,(const void*)rrp);  if (st) return st;
  return csp_domain(vb,rrp->host);
}

static adns_status cs_srvha(vbuf *vb, const void *datap) {
  const adns_rr_srvha *rrp= datap;
  adns_status st;

  st= csp_srv_begin(vb,(const void*)datap);  if (st) return st;
  return csp_hostaddr(vb,&rrp->ha);
}

static void postsort_srv(adns_state ads, void *array, int nrrs,int rrsz,
			 const struct typeinfo *typei) {
  /* we treat everything in the array as if it were an adns_rr_srvha
   * even though the array might be of adns_rr_srvraw.  That's OK
   * because they have the same prefix, which is all we access.
   * We use rrsz, too, rather than naive array indexing, of course.
   */
  char *workbegin, *workend, *search, *arrayend;
  const adns_rr_srvha *rr;
  union { adns_rr_srvha ha; adns_rr_srvraw raw; } rrtmp;
  int cpriority, totalweight, runtotal;
  long randval;

  assert(rrsz <= sizeof(rrtmp));
  for (workbegin= array, arrayend= workbegin + rrsz * nrrs;
       workbegin < arrayend;
       workbegin= workend) {
    cpriority= (rr=(void*)workbegin)->priority;
    
    for (workend= workbegin, totalweight= 0;
	 workend < arrayend && (rr=(void*)workend)->priority == cpriority;
	 workend += rrsz) {
      totalweight += rr->weight;
    }

    /* Now workbegin..(workend-1) incl. are exactly all of the RRs of
     * cpriority.  From now on, workbegin points to the `remaining'
     * records: we select one record at a time (RFC2782 `Usage rules'
     * and `Format of the SRV RR' subsection `Weight') to place at
     * workbegin (swapping with the one that was there, and then
     * advance workbegin. */
    for (;
	 workbegin + rrsz < workend; /* don't bother if just one */
	 workbegin += rrsz) {
      
      randval= nrand48(ads->rand48xsubi);
      randval %= (totalweight + 1);
        /* makes it into 0..totalweight inclusive; with 2^10 RRs,
	 * totalweight must be <= 2^26 so probability nonuniformity is
	 * no worse than 1 in 2^(31-26) ie 1 in 2^5, ie
	 *  abs(log(P_intended(RR_i) / P_actual(RR_i)) <= log(2^-5).
	 */

      for (search=workbegin, runtotal=0;
	   (runtotal += (rr=(void*)search)->weight) < randval;
	   search += rrsz);
      assert(search < arrayend);
      totalweight -= rr->weight;
      if (search != workbegin) {
	memcpy(&rrtmp, workbegin, rrsz);
	memcpy(workbegin, search, rrsz);
	memcpy(search, &rrtmp, rrsz);
      }
    }
  }
  /* tests:
   *  dig -t srv _srv._tcp.test.iwj.relativity.greenend.org.uk.
   *   ./adnshost_s -t srv- _sip._udp.voip.net.cam.ac.uk.
   *   ./adnshost_s -t srv- _jabber._tcp.jabber.org
   */
}

/*
 * _byteblock   (mf)
 */

static void mf_byteblock(adns_query qu, void *datap) {
  adns_rr_byteblock *rrp= datap;
  void *bytes= rrp->data;
  adns__makefinal_block(qu,&bytes,rrp->len);
  rrp->data= bytes;
}

/*
 * _opaque   (pa,cs)
 */

static adns_status pa_opaque(const parseinfo *pai, int cbyte,
			     int max, void *datap) {
  adns_rr_byteblock *rrp= datap;

  rrp->len= max - cbyte;
  rrp->data= adns__alloc_interim(pai->qu, rrp->len);
  if (!rrp->data) R_NOMEM;
  memcpy(rrp->data, pai->dgram + cbyte, rrp->len);
  return adns_s_ok;
}

static adns_status cs_opaque(vbuf *vb, const void *datap) {
  const adns_rr_byteblock *rrp= datap;
  char buf[10];
  int l;
  unsigned char *p;

  sprintf(buf,"\\# %d",rrp->len);
  CSP_ADDSTR(buf);
  
  for (l= rrp->len, p= rrp->data;
       l>=4;
       l -= 4, p += 4) {
    sprintf(buf," %02x%02x%02x%02x",p[0],p[1],p[2],p[3]);
    CSP_ADDSTR(buf);
  }
  for (;
       l>0;
       l--, p++) {
    sprintf(buf," %02x",*p);
    CSP_ADDSTR(buf);
  }
  return adns_s_ok;
}
  
/*
 * _flat   (mf)
 */

static void mf_flat(adns_query qu, void *data) { }

/*
 * Now the table.
 */

#define TYPESZ_M(member)           (sizeof(*((adns_answer*)0)->rrs.member))

#define DEEP_TYPE(code,rrt,fmt,memb,parser,comparer,/*printer*/...)	\
 { adns_r_##code&adns_rrt_reprmask, rrt,fmt,TYPESZ_M(memb), mf_##memb,	\
     GLUE(cs_, CAR(__VA_ARGS__)),pa_##parser,di_##comparer,		\
     adns__ckl_hostname, 0, adns__getrrsz_default, adns__query_send,	\
     CDR(__VA_ARGS__) }
#define FLAT_TYPE(code,rrt,fmt,memb,parser,comparer,/*printer*/...)	\
 { adns_r_##code&adns_rrt_reprmask, rrt,fmt,TYPESZ_M(memb), mf_flat,	\
     GLUE(cs_, CAR(__VA_ARGS__)),pa_##parser,di_##comparer,		\
     adns__ckl_hostname, 0, adns__getrrsz_default, adns__query_send,	\
     CDR(__VA_ARGS__) }

#define di_0 0

static const typeinfo typeinfos[] = {
/* Must be in ascending order of rrtype ! */
/* mem-mgmt code  rrt     fmt   member   parser      comparer  printer */

FLAT_TYPE(a,      "A",     0,   inaddr,    inaddr,  inaddr,inaddr          ),
DEEP_TYPE(ns_raw, "NS",   "raw",str,       host_raw,0,     domain          ),
DEEP_TYPE(cname,  "CNAME", 0,   str,       dom_raw, 0,     domain          ),
DEEP_TYPE(soa_raw,"SOA",  "raw",soa,       soa,     0,     soa             ),
DEEP_TYPE(ptr_raw,"PTR",  "raw",str,       host_raw,0,     domain          ),
DEEP_TYPE(hinfo,  "HINFO", 0,   intstrpair,hinfo,   0,     hinfo           ),
DEEP_TYPE(mx_raw, "MX",   "raw",intstr,    mx_raw,  mx_raw,inthost         ),
DEEP_TYPE(txt,    "TXT",   0,   manyistr,  txt,     0,     txt             ),
DEEP_TYPE(rp_raw, "RP",   "raw",strpair,   rp,      0,     rp              ),
FLAT_TYPE(aaaa,	  "AAAA",  0,   in6addr,   in6addr, in6addr,in6addr        ),
DEEP_TYPE(srv_raw,"SRV",  "raw",srvraw ,   srvraw,  srv,   srvraw,
			      .checklabel= ckl_srv, .postsort= postsort_srv),

FLAT_TYPE(addr,   "A",  "addr", addr,      addr,    addr,  addr,
				   .getrrsz= gsz_addr, .query_send= qs_addr),
DEEP_TYPE(ns,     "NS", "+addr",hostaddr,  hostaddr,hostaddr,hostaddr      ),
DEEP_TYPE(ptr,    "PTR","checked",str,     ptr,     0,     domain,
						       .checklabel= ckl_ptr),
DEEP_TYPE(mx,     "MX", "+addr",inthostaddr,mx,     mx,    inthostaddr,    ),
DEEP_TYPE(srv,    "SRV","+addr",srvha,     srvha,   srv,   srvha,
			      .checklabel= ckl_srv, .postsort= postsort_srv),

DEEP_TYPE(soa,    "SOA","822",  soa,       soa,     0,     soa             ),
DEEP_TYPE(rp,     "RP", "822",  strpair,   rp,      0,     rp              ),
};

static const typeinfo tinfo_addrsub =
FLAT_TYPE(none,	  "<addr>","sub",addr,	   addr,    0,	   addr,
							 .getrrsz= gsz_addr);

static const typeinfo typeinfo_unknown=
DEEP_TYPE(unknown,0, "unknown",byteblock,opaque,  0,     opaque            );

const typeinfo *adns__findtype(adns_rrtype type) {
  const typeinfo *begin, *end, *mid;

  if (type & ~(adns_rrtype)0x63ffffff)
    /* 0x60000000 is reserved for `harmless' future expansion */
    return 0;

  if (type & adns_r_unknown) return &typeinfo_unknown;
  type &= adns_rrt_reprmask;

  begin= typeinfos;  end= typeinfos+(sizeof(typeinfos)/sizeof(typeinfo));

  while (begin < end) {
    mid= begin + ((end-begin)>>1);
    if (mid->typekey == type) return mid;
    if (type > mid->typekey) begin= mid+1;
    else end= mid;
  }
  return 0;
}
