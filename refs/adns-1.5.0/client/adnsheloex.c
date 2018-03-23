/*
 * adnsheloex.c
 * - look up the A record of hosts in an Exim log that failed HELO verification
 */
/*
 *  This file is
 *   Copyright (C) 2004 Tony Finch <dot@dotat.at>
 *
 *  It is part of adns, which is
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
 *
 *  This file is by Tony Finch, based on adnslogres.c.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "config.h"
#include "adns.h"
#include "client.h"

#ifdef ADNS_REGRESS_TEST
# include "hredirect.h"
#endif

/* maximum number of concurrent DNS queries */
#define MAXMAXPENDING 64000
#define DEFMAXPENDING 2000

/* maximum length of a line */
#define MAXLINE 1024

/* option flags */
#define OPT_DEBUG 1
#define OPT_POLL 2

static const char *const progname= "adnsheloex";
static const char *config_text;

#define guard_null(str) ((str) ? (str) : "")

#define sensible_ctype(type,ch) (type((unsigned char)(ch)))
  /* isfoo() functions from ctype.h can't safely be fed char - blech ! */

static void msg(const char *fmt, ...) {
  va_list al;

  fprintf(stderr, "%s: ", progname);
  va_start(al,fmt);
  vfprintf(stderr, fmt, al);
  va_end(al);
  fputc('\n',stderr);
}

static void aargh(const char *cause) {
  const char *why = strerror(errno);
  if (!why) why = "Unknown error";
  msg("%s: %s (%d)", cause, why, errno);
  exit(1);
}

typedef struct logline {
  struct logline *next;
  char *start, *name, *rest, *addr;
  adns_query query;
} logline;

static logline *readline(FILE *inf, adns_state adns, int opts) {
  static char buf[MAXLINE];
  char *str, *p, *q, *r;
  logline *line;

  if (fgets(buf, MAXLINE, inf)) {
    str= malloc(sizeof(*line) + strlen(buf) + 1);
    if (!str) aargh("malloc");
    line= (logline*)str;
    line->next= NULL;
    line->start= str+sizeof(logline);
    strcpy(line->start, buf);
    line->name= line->rest= line->addr= NULL;
    /* look for unverifiable HELO information matching the regex
       H=[a-z0-9.- ]*[(][a-z0-9.-]*[)] [[][0-9.]*[]] */
    for (p= strchr(line->start, ' '); p; p= strchr(p+1, ' ')) {
      if (!strncmp(p, " H=", 3)) {
	r= strchr(p, '[');
	if (!r) break;
	q= strchr(p, ')');
	if (!q || q>r) break;
	p= strchr(p, '(');
	if (!p || p>q) break;
	line->name= p+1;
	line->rest= q;
	line->addr= r+1;
	break;
      }
    }
    if (line->name) {
      *line->rest= '\0';
      if (opts & OPT_DEBUG)
	msg("submitting %s", line->name);
      if (adns_submit(adns, line->name, adns_r_a,
		      adns_qf_quoteok_query|adns_qf_quoteok_cname|adns_qf_cname_loose,
		      NULL, &line->query))
	aargh("adns_submit");
      *line->rest= ')';
    } else {
      if (opts & OPT_DEBUG)
	msg("no query");
      line->query= NULL;
    }
    return line;
  }
  if (!feof(inf))
    aargh("fgets");
  return NULL;
}

static void proclog(FILE *inf, FILE *outf, int maxpending, int opts) {
  int eof, err, len;
  adns_state adns;
  adns_answer *answer;
  logline *head, *tail, *line;
  adns_initflags initflags;

  initflags= (opts & OPT_DEBUG) ? adns_if_debug : 0;
  if (config_text) {
    errno= adns_init_strcfg(&adns, initflags, stderr, config_text);
  } else {
    errno= adns_init(&adns, initflags, 0);
  }
  if (errno) aargh("adns_init");
  head= tail= readline(inf, adns, opts);
  len= 1; eof= 0;
  while (head) {
    while (head) {
      if (head->query) {
	if (opts & OPT_DEBUG)
	  msg("%d in queue; checking %.*s", len,
	      (int)(head->rest-head->name), guard_null(head->name));
	if (eof || len >= maxpending) {
	  if (opts & OPT_POLL)
	    err= adns_wait_poll(adns, &head->query, &answer, NULL);
	  else
	    err= adns_wait(adns, &head->query, &answer, NULL);
	} else {
	  err= adns_check(adns, &head->query, &answer, NULL);
	}
	if (err == EAGAIN) break;
	if (err) {
	  fprintf(stderr, "%s: adns_wait/check: %s", progname, strerror(err));
	  exit(1);
	}
	if (answer->status == adns_s_ok) {
	  const char *addr;
	  int ok = 0;
	  fprintf(outf, "%.*s", (int)(head->rest-head->start), head->start);
	  while(answer->nrrs--) {
	    addr= inet_ntoa(answer->rrs.inaddr[answer->nrrs]);
	    ok |= !strncmp(addr, head->addr, strlen(addr));
	    fprintf(outf, " [%s]", addr);
	  }
	  fprintf(outf, "%s%s", ok ? " OK" : "", head->rest);
	} else {
	  if (opts & OPT_DEBUG)
	    msg("query failed");
	  fputs(head->start, outf);
	}
	free(answer);
	len--;
      } else {
	if (opts & OPT_DEBUG)
	  msg("%d in queue; no query on this line", len);
	fputs(head->start, outf);
      }
      line= head; head= head->next;
      free(line);
    }
    if (!eof) {
      line= readline(inf, adns, opts);
      if (line) {
        if (!head) head= line;
        else tail->next= line;
        tail= line;
	if (line->query) len++;
      } else {
	eof= 1;
      }
    }
  }
  adns_finish(adns);
}

static void printhelp(FILE *file) {
  fputs("usage: adnsheloex [<options>] [<logfile>]\n"
	"       adnsheloex --version|--help\n"
	"options: -c <concurrency>  set max number of outstanding queries\n"
	"         -p                use poll(2) instead of select(2)\n"
	"         -d                turn on debugging\n"
	"         -C <config>       use instead of contents of resolv.conf\n",
	stdout);
}

static void usage(void) {
  printhelp(stderr);
  exit(1);
}

int main(int argc, char *argv[]) {
  int c, opts, maxpending;
  extern char *optarg;
  FILE *inf;

  if (argv[1] && !strncmp(argv[1],"--",2)) {
    if (!strcmp(argv[1],"--help")) {
      printhelp(stdout);
    } else if (!strcmp(argv[1],"--version")) {
      fputs(VERSION_MESSAGE("adnsheloex"),stdout);
    } else {
      usage();
    }
    if (ferror(stdout) || fclose(stdout)) { perror("stdout"); exit(1); }
    exit(0);
  }

  maxpending= DEFMAXPENDING;
  opts= 0;
  while ((c= getopt(argc, argv, "c:C:dp")) != -1)
    switch (c) {
    case 'c':
      maxpending= atoi(optarg);
      if (maxpending < 1 || maxpending > MAXMAXPENDING) {
       fprintf(stderr, "%s: unfeasible concurrency %d\n", progname, maxpending);
       exit(1);
      }
      break;
    case 'C':
      config_text= optarg;
      break;
    case 'd':
      opts|= OPT_DEBUG;
      break;
    case 'p':
      opts|= OPT_POLL;
      break;
    default:
      usage();
    }

  argc-= optind;
  argv+= optind;

  inf= NULL;
  if (argc == 0)
    inf= stdin;
  else if (argc == 1)
    inf= fopen(*argv, "r");
  else
    usage();

  if (!inf)
    aargh("couldn't open input");

  proclog(inf, stdout, maxpending, opts);

  if (fclose(inf))
    aargh("fclose input");
  if (fclose(stdout))
    aargh("fclose output");

  return 0;
}
