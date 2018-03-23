/*
 * adh-query.c
 * - useful general-purpose resolver client program
 *   make queries and print answers
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

#include "adnshost.h"

adns_state ads;
struct outstanding_list outstanding;

static unsigned long idcounter;

void ensure_adns_init(void) {
  adns_initflags initflags;
  int r;
  
  if (ads) return;

  if (signal(SIGPIPE,SIG_IGN) == SIG_ERR) sysfail("ignore SIGPIPE",errno);

  initflags= adns_if_noautosys|adns_if_nosigpipe|ov_verbose;
  if (!ov_env) initflags |= adns_if_noenv;

  if (config_text) {
    r= adns_init_strcfg(&ads, initflags, stderr, config_text);
  } else {
    r= adns_init(&ads, initflags, 0);
  }
  if (r) sysfail("adns_init",r);

  if (ov_format == fmt_default)
    ov_format= ov_asynch ? fmt_asynch : fmt_simple;
}

void type_info(adns_rrtype type, const char **typename_r,
	       const void *datap, char **data_r) {
  static char buf[12];
  adns_status st;
  
  st= adns_rr_info(type, typename_r, 0,0, datap,data_r);
  if (st == adns_s_nomemory) sysfail("adns_rr_info failed",ENOMEM);
  assert(!st);
  if (typename_r && !*typename_r) {
    sprintf(buf,"TYPE%d", (int)(type & adns_rrt_typemask));
    *typename_r= buf;
  }
}

static void prep_query(struct query_node **qun_r, int *quflags_r) {
  struct query_node *qun;
  char idbuf[20];
  
  if (ov_pipe && !ads) usageerr("-f/--pipe not consistent with domains on command line");
  ensure_adns_init();
  
  qun= malloc(sizeof(*qun));
  qun->pqfr= ov_pqfr;
  if (ov_id) {
    qun->id= xstrsave(ov_id);
  } else {
    sprintf(idbuf,"%lu",idcounter++);
    idcounter &= 0x0fffffffflu;
    qun->id= xstrsave(idbuf);
  }

  *quflags_r=
    (ov_search ? adns_qf_search : 0) |
    (ov_tcp ? adns_qf_usevc : 0) |
    ((ov_pqfr.show_owner || ov_format == fmt_simple) ? adns_qf_owner : 0) |
    (ov_qc_query ? adns_qf_quoteok_query : 0) |
    (ov_qc_anshost ? adns_qf_quoteok_anshost : 0) |
    (ov_qc_cname ? 0 : adns_qf_quoteok_cname) |
    (ov_v6map ? adns_qf_ipv6_mapv4 : 0) |
    ov_cname | ov_afflags;
    
  *qun_r= qun;
}

static void sockaddr_aton(const char *text, adns_rr_addr *a) {
  int err;

  a->len= sizeof(a->addr);
  err= adns_text2addr(text,0,adns_qf_addrlit_scope_forbid,
		      &a->addr.sa,&a->len);
  if (err == EINVAL) usageerr("invalid IP address %s",text);
  else if (err) sysfail("adns_text2addr",err);
}
  
void of_ptr(const struct optioninfo *oi, const char *arg, const char *arg2) {
  struct query_node *qun;
  int quflags, r;
  adns_rr_addr a;

  sockaddr_aton(arg,&a);
  prep_query(&qun,&quflags);
  qun->owner= xstrsave(arg);
  r= adns_submit_reverse(ads, &a.addr.sa,
			 ov_type == adns_r_none ? adns_r_ptr : ov_type,
			 quflags,
			 qun,
			 &qun->qu);
  if (r) sysfail("adns_submit_reverse",r);

  LIST_LINK_TAIL(outstanding,qun);
}

void of_reverse(const struct optioninfo *oi, const char *arg, const char *arg2) {
  struct query_node *qun;
  int quflags, r;
  adns_rr_addr a;

  sockaddr_aton(arg,&a);
  prep_query(&qun,&quflags);
  qun->owner= xmalloc(strlen(arg) + strlen(arg2) + 2);
  sprintf(qun->owner, "%s %s", arg,arg2);
  r= adns_submit_reverse_any(ads, &a.addr.sa,arg2,
			     ov_type == adns_r_none ? adns_r_txt : ov_type,
			     quflags,
			     qun,
			     &qun->qu);
  if (r) sysfail("adns_submit_reverse",r);

  LIST_LINK_TAIL(outstanding,qun);
}

void query_do(const char *domain) {
  struct query_node *qun;
  int quflags, r;

  prep_query(&qun,&quflags);
  qun->owner= xstrsave(domain);
  r= adns_submit(ads, domain,
		 ov_type == adns_r_none ? adns_r_addr : ov_type,
		 quflags,
		 qun,
		 &qun->qu);
  if (r) sysfail("adns_submit",r);

  LIST_LINK_TAIL(outstanding,qun);
}

static void dequeue_query(struct query_node *qun) {
  LIST_UNLINK(outstanding,qun);
  free(qun->id);
  free(qun->owner);
  free(qun);
}

static void print_withspace(const char *str) {
  if (printf("%s ", str) == EOF) outerr();
}

static void print_ttl(struct query_node *qun, adns_answer *answer) {
  unsigned long ttl;
  time_t now;
  
  switch (qun->pqfr.ttl) {
  case tm_none:
    return;
  case tm_rel:
    if (time(&now) == (time_t)-1) sysfail("get current time",errno);
    ttl= answer->expires < now ? 0 : answer->expires - now;
    break;
  case tm_abs:
    ttl= answer->expires;
    break;
  default:
    abort();
  }
  if (printf("%lu ",ttl) == EOF) outerr();
}

static const char *owner_show(struct query_node *qun, adns_answer *answer) {
  return answer->owner ? answer->owner : qun->owner;
}

static void print_owner_ttl(struct query_node *qun, adns_answer *answer) {
  if (qun->pqfr.show_owner) print_withspace(owner_show(qun,answer));
  print_ttl(qun,answer);
}

static void check_status(adns_status st) {
  static const adns_status statuspoints[]= {
    adns_s_ok,
    adns_s_max_localfail, adns_s_max_remotefail, adns_s_max_tempfail,
    adns_s_max_misconfig, adns_s_max_misquery
  };

  const adns_status *spp;
  int minrcode;

  for (minrcode=0, spp=statuspoints;
       spp < statuspoints + (sizeof(statuspoints)/sizeof(statuspoints[0]));
       spp++)
    if (st > *spp) minrcode++;
  if (rcode < minrcode) rcode= minrcode;
}

static void print_status(adns_status st, struct query_node *qun, adns_answer *answer) {
  const char *statustypeabbrev, *statusabbrev, *statusstring;

  statustypeabbrev= adns_errtypeabbrev(st);
  statusabbrev= adns_errabbrev(st);
  statusstring= adns_strerror(st);
  assert(!strchr(statusstring,'"'));

  if (printf("%s %d %s ", statustypeabbrev, st, statusabbrev)
      == EOF) outerr();
  print_owner_ttl(qun,answer);
  if (qun->pqfr.show_cname)
    print_withspace(answer->cname ? answer->cname : "$");
  if (printf("\"%s\"\n", statusstring) == EOF) outerr();
}

static void print_dnsfail(adns_status st, struct query_node *qun, adns_answer *answer) {
  int r;
  const char *typename, *statusstring;
  
  if (ov_format == fmt_inline) {
    if (fputs("; failed ",stdout) == EOF) outerr();
    print_status(st,qun,answer);
    return;
  }
  assert(ov_format == fmt_simple);
  if (st == adns_s_nxdomain) {
    r= fprintf(stderr,"%s does not exist\n", owner_show(qun,answer));
  } else {
    type_info(answer->type, &typename, 0,0);
    if (st == adns_s_nodata) {
      r= fprintf(stderr,"%s has no %s record\n", owner_show(qun,answer), typename);
    } else {
      statusstring= adns_strerror(st);
      r= fprintf(stderr,"Error during DNS %s lookup for %s: %s\n",
		 typename, owner_show(qun,answer), statusstring);
    }
  }
  if (r == EOF) sysfail("write error message to stderr",errno);
}
    
void query_done(struct query_node *qun, adns_answer *answer) {
  adns_status st;
  int rrn, nrrs;
  const char *rrp, *realowner, *typename;
  char *datastr;

  st= answer->status;
  nrrs= answer->nrrs;
  if (ov_format == fmt_asynch) {
    check_status(st);
    if (printf("%s %d ", qun->id, nrrs) == EOF) outerr();
    print_status(st,qun,answer);
  } else {
    if (qun->pqfr.show_cname && answer->cname) {
      print_owner_ttl(qun,answer);
      if (qun->pqfr.show_type) print_withspace("CNAME");
      if (printf("%s\n", answer->cname) == EOF) outerr();
    }
    if (st) {
      check_status(st);
      print_dnsfail(st,qun,answer);
    }
  }
  if (qun->pqfr.show_owner) {
    realowner= answer->cname ? answer->cname : owner_show(qun,answer);
    assert(realowner);
  } else {
    realowner= 0;
  }
  if (nrrs) {
    for (rrn=0, rrp = answer->rrs.untyped;
	 rrn < nrrs;
	 rrn++, rrp += answer->rrsz) {
      if (realowner) print_withspace(realowner);
      print_ttl(qun,answer);
      type_info(answer->type,&typename, rrp,&datastr);
      if (qun->pqfr.show_type) print_withspace(typename);
      if (printf("%s\n",datastr) == EOF) outerr();
      free(datastr);
    }
  }
  if (fflush(stdout)) outerr();
  free(answer);
  dequeue_query(qun);
}

void of_asynch_id(const struct optioninfo *oi, const char *arg, const char *arg2) {
  free(ov_id);
  ov_id= xstrsave(arg);
}

void of_cancel_id(const struct optioninfo *oi, const char *arg, const char *arg2) {
  struct query_node *qun;

  for (qun= outstanding.head;
       qun && strcmp(qun->id,arg);
       qun= qun->next);
  if (!qun) return;
  adns_cancel(qun->qu);
  dequeue_query(qun);
}
