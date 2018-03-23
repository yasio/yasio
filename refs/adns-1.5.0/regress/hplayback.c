#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "harness.h"
static FILE *Tinputfile, *Treportfile;
static vbuf vb2;
extern void Tshutdown(void) {
  adns__vbuf_free(&vb2);
}
static void Tensurereportfile(void) {
  const char *fdstr;
  int fd;
  if (Treportfile) return;
  Treportfile= stderr;
  fdstr= getenv("ADNS_TEST_REPORT_FD"); if (!fdstr) return;
  fd= atoi(fdstr);
  Treportfile= fdopen(fd,"a"); if (!Treportfile) Tfailed("fdopen ADNS_TEST_REPORT_FD");
}
static void Psyntax(const char *where) {
  fprintf(stderr,"adns test harness: syntax error in test log input file: %s\n",where);
  exit(-1);
}
static void Pcheckinput(void) {
  if (ferror(Tinputfile)) Tfailed("read test log input file");
  if (feof(Tinputfile)) Psyntax("eof at syscall reply");
}
void Tensurerecordfile(void) {
  const char *fdstr;
  int fd;
  int chars;
  unsigned long sec, usec;
  if (Tinputfile) return;
  Tinputfile= stdin;
  fdstr= getenv("ADNS_TEST_IN_FD");
  if (fdstr) {
    fd= atoi(fdstr);
    Tinputfile= fdopen(fd,"r"); if (!Tinputfile) Tfailed("fdopen ADNS_TEST_IN_FD");
  }
  setvbuf(Tinputfile,0,_IONBF,0);
  if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
  fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
  chars= -1;
  sscanf(vb2.buf," start %lu.%lu%n",&sec,&usec,&chars);
  if (chars==-1) Psyntax("start time invalid");
  currenttime.tv_sec= sec;
  currenttime.tv_usec= usec;
  if (vb2.buf[chars] != '\n') Psyntax("not newline after start time");
}
static void Parg(const char *argname) {
  int l;
  if (vb2.buf[vb2.used++] != ' ') Psyntax("not a space before argument");
  l= strlen(argname);
  if (memcmp(vb2.buf+vb2.used,argname,l)) Psyntax("argument name wrong");
  vb2.used+= l;
  if (vb2.buf[vb2.used++] != '=') Psyntax("not = after argument name");
}
static int Pstring_maybe(const char *string) {
  int l;
  l= strlen(string);
  if (memcmp(vb2.buf+vb2.used,string,l)) return 0;
  vb2.used+= l;
  return 1;
}
static void Pstring(const char *string, const char *emsg) {
  if (Pstring_maybe(string)) return;
  Psyntax(emsg);
}
static int Perrno(const char *stuff) {
  const struct Terrno *te;
  int r;
  char *ep;
  for (te= Terrnos; te->n && strcmp(te->n,stuff); te++);
  if (te->n) return te->v;
  r= strtoul(stuff+2,&ep,10);
  if (*ep) Psyntax("errno value not recognised, not numeric");
  return r;
}
static void P_updatetime(void) {
  int chars;
  unsigned long sec, usec;
  if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
  fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
  chars= -1;
  sscanf(vb2.buf," +%lu.%lu%n",&sec,&usec,&chars);
  if (chars==-1) Psyntax("update time invalid");
  currenttime.tv_sec+= sec;
  currenttime.tv_usec+= usec;
  if (currenttime.tv_usec > 1000000) {
    currenttime.tv_sec++;
    currenttime.tv_usec -= 1000000;
  }
  if (vb2.buf[chars] != '\n') Psyntax("not newline after update time");
}
static void Pfdset(fd_set *set, int max) {
  int r, c;
  char *ep;
  if (vb2.buf[vb2.used++] != '[') Psyntax("fd set start not [");
  FD_ZERO(set);
  if (vb2.buf[vb2.used] == ']') { vb2.used++; return; }
  for (;;) {
    r= strtoul(vb2.buf+vb2.used,&ep,10);
    if (r>=max) Psyntax("fd set member > max");
    if (ep == (char*)vb2.buf+vb2.used) Psyntax("empty entry in fd set");
    FD_SET(r,set);
    vb2.used= ep - (char*)vb2.buf;
    c= vb2.buf[vb2.used++];
    if (c == ']') break;
    if (c != ',') Psyntax("fd set separator not ,");
  }
}
#ifdef HAVE_POLL
static int Ppollfdevents(void) {
  int events;
  if (Pstring_maybe("0")) return 0;
  events= 0;
  if (Pstring_maybe("POLLIN")) {
    events |= POLLIN;
    if (!Pstring_maybe("|")) return events;
  }
  if (Pstring_maybe("POLLOUT")) {
    events |= POLLOUT;
    if (!Pstring_maybe("|")) return events;
  }
  Pstring("POLLPRI","pollfdevents PRI?");
  return events;
}
static void Ppollfds(struct pollfd *fds, int nfds) {
  int i;
  char *ep;
  const char *comma= "";
  if (vb2.buf[vb2.used++] != '[') Psyntax("pollfds start not [");
  for (i=0; i<nfds; i++) {
    Pstring("{fd=","{fd= in pollfds");
    fds->fd= strtoul(vb2.buf+vb2.used,&ep,10);
    vb2.used= ep - (char*)vb2.buf;    
    Pstring(", events=",", events= in pollfds");
    fds->events= Ppollfdevents();
    Pstring(", revents=",", revents= in pollfds");
    fds->revents= Ppollfdevents();
    Pstring("}","} in pollfds");
    Pstring(comma,"separator in pollfds");
    comma= ", ";
  }
  if (vb2.buf[vb2.used++] != ']') Psyntax("pollfds end not ]");
}
#endif
static void Paddr(struct sockaddr *addr, int *lenr) {
  adns_rr_addr a;
  char *p, *q, *ep;
  int err;
  unsigned long ul;
  p= vb2.buf+vb2.used;
  if (*p!='[') {
    q= strchr(p,':');
    if (!q) Psyntax("missing :");
    *q++= 0;
  } else {
    p++;
    q= strchr(p,']');
    if (!q) Psyntax("missing ]");
    *q++= 0;
    if (*q!=':') Psyntax("expected : after ]");
    q++;
  }
  ul= strtoul(q,&ep,10);
  if (*ep && *ep != ' ') Psyntax("invalid port (bad syntax)");
  if (ul >= 65536) Psyntax("port too large");
  a.len= sizeof(a.addr);
  err= adns_text2addr(p, (int)ul, 0, &a.addr.sa,&a.len);
  if (err) Psyntax("invalid address");
  assert(*lenr >= a.len);
  memcpy(addr, &a.addr, a.len);
  *lenr= a.len;
  vb2.used= ep - (char*)vb2.buf;
}
static int Pbytes(byte *buf, int maxlen) {
  static const char hexdigits[]= "0123456789abcdef";
  int c, v, done;
  const char *pf;
  done= 0;
  for (;;) {
    c= getc(Tinputfile); Pcheckinput();
    if (c=='\n' || c==' ' || c=='\t') continue;
    if (c=='.') break;
    pf= strchr(hexdigits,c); if (!pf) Psyntax("invalid first hex digit");
    v= (pf-hexdigits)<<4;
    c= getc(Tinputfile); Pcheckinput();
    pf= strchr(hexdigits,c); if (!pf) Psyntax("invalid second hex digit");
    v |= (pf-hexdigits);
    if (maxlen<=0) Psyntax("buffer overflow in bytes");
    *buf++= v;
    maxlen--; done++;
  }
  for (;;) {
    c= getc(Tinputfile); Pcheckinput();
    if (c=='\n') return done;
  }
}
void Q_vb(void) {
  const char *nl;
  Tensurerecordfile();
  if (!adns__vbuf_ensure(&vb2,vb.used+2)) Tnomem();
  fread(vb2.buf,1,vb.used+2,Tinputfile);
  if (feof(Tinputfile)) {
    fprintf(stderr,"adns test harness: input ends prematurely; program did:\n %.*s\n",
           vb.used,vb.buf);
    exit(-1);
  }
  Pcheckinput();
  if (vb2.buf[0] != ' ') Psyntax("not space before call");
  if (memcmp(vb.buf,vb2.buf+1,vb.used) ||
      vb2.buf[vb.used+1] != '\n') {
    fprintf(stderr,
            "adns test harness: program did unexpected:\n %.*s\n"
            "was expecting:\n %.*s\n",
            vb.used,vb.buf, vb.used,vb2.buf+1);
    exit(1);
  }
  Tensurereportfile();
  nl= memchr(vb.buf,'\n',vb.used);
  fprintf(Treportfile," %.*s\n", (int)(nl ? nl - (const char*)vb.buf : vb.used), vb.buf);
}
int Hselect(	int max , fd_set *rfds , fd_set *wfds , fd_set *efds , struct timeval *to 	) {
 int r, amtread;
 char *ep;
 Qselect(	max , rfds , wfds , efds , to 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," select=",8)) Psyntax("syscall reply mismatch");
 if (vb2.buf[8] == 'E') {
  int e;
  e= Perrno(vb2.buf+8);
  P_updatetime();
  errno= e;
  return -1;
 }
  r= strtoul(vb2.buf+8,&ep,10);
  if (*ep && *ep!=' ') Psyntax("return value not E* or positive number");
  vb2.used= ep - (char*)vb2.buf;
	Parg("rfds"); Pfdset(rfds,max); 
	Parg("wfds"); Pfdset(wfds,max); 
	Parg("efds"); Pfdset(efds,max); 
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
#ifdef HAVE_POLL
int Hpoll(	struct pollfd *fds , int nfds , int timeout 	) {
 int r, amtread;
 char *ep;
 Qpoll(	fds , nfds , timeout 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," poll=",6)) Psyntax("syscall reply mismatch");
 if (vb2.buf[6] == 'E') {
  int e;
  e= Perrno(vb2.buf+6);
  P_updatetime();
  errno= e;
  return -1;
 }
  r= strtoul(vb2.buf+6,&ep,10);
  if (*ep && *ep!=' ') Psyntax("return value not E* or positive number");
  vb2.used= ep - (char*)vb2.buf;
        Parg("fds"); Ppollfds(fds,nfds); 
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
#endif
int Hsocket(	int domain , int type , int protocol 	) {
 int r, amtread;
 char *ep;
  Tmust("socket","domain",domain==AF_INET || domain==AF_INET6); 
  Tmust("socket","type",type==SOCK_STREAM || type==SOCK_DGRAM); 
 Qsocket(	domain , type 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," socket=",8)) Psyntax("syscall reply mismatch");
 if (vb2.buf[8] == 'E') {
  int e;
  e= Perrno(vb2.buf+8);
  P_updatetime();
  errno= e;
  return -1;
 }
  r= strtoul(vb2.buf+8,&ep,10);
  if (*ep && *ep!=' ') Psyntax("return value not E* or positive number");
  vb2.used= ep - (char*)vb2.buf;
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
int Hfcntl(	int fd , int cmd , ... 	) {
 int r, amtread;
	va_list al; long arg; 
  Tmust("fcntl","cmd",cmd==F_SETFL || cmd==F_GETFL);
  if (cmd == F_SETFL) {
    va_start(al,cmd); arg= va_arg(al,long); va_end(al);
  } else {
    arg= 0;
  } 
 Qfcntl(	fd , cmd , arg 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," fcntl=",7)) Psyntax("syscall reply mismatch");
 if (vb2.buf[7] == 'E') {
  int e;
  e= Perrno(vb2.buf+7);
  P_updatetime();
  errno= e;
  return -1;
 }
  r= 0;
  if (cmd == F_GETFL) {
    if (!memcmp(vb2.buf+7,"O_NONBLOCK|...",14)) {
      r= O_NONBLOCK;
      vb2.used= 7+14;
    } else if (!memcmp(vb2.buf+7,"~O_NONBLOCK&...",15)) {
      vb2.used= 7+15;
    } else {
      Psyntax("fcntl flags not O_NONBLOCK|... or ~O_NONBLOCK&...");
    }
  } else if (cmd == F_SETFL) {
  if (memcmp(vb2.buf+7,"OK",2)) Psyntax("success/fail not E* or OK");
  vb2.used= 7+2;
  r= 0;
  } else {
    Psyntax("fcntl not F_GETFL or F_SETFL");
  }
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
int Hconnect(	int fd , const struct sockaddr *addr , int addrlen 	) {
 int r, amtread;
 Qconnect(	fd , addr , addrlen 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," connect=",9)) Psyntax("syscall reply mismatch");
 if (vb2.buf[9] == 'E') {
  int e;
  e= Perrno(vb2.buf+9);
  P_updatetime();
  errno= e;
  return -1;
 }
  if (memcmp(vb2.buf+9,"OK",2)) Psyntax("success/fail not E* or OK");
  vb2.used= 9+2;
  r= 0;
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
int Hbind(	int fd , const struct sockaddr *addr , int addrlen 	) {
 int r, amtread;
 Qbind(	fd , addr , addrlen 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," bind=",6)) Psyntax("syscall reply mismatch");
 if (vb2.buf[6] == 'E') {
  int e;
  e= Perrno(vb2.buf+6);
  P_updatetime();
  errno= e;
  return -1;
 }
  if (memcmp(vb2.buf+6,"OK",2)) Psyntax("success/fail not E* or OK");
  vb2.used= 6+2;
  r= 0;
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
int Hlisten(	int fd , int backlog 	) {
 int r, amtread;
 Qlisten(	fd , backlog 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," listen=",8)) Psyntax("syscall reply mismatch");
 if (vb2.buf[8] == 'E') {
  int e;
  e= Perrno(vb2.buf+8);
  P_updatetime();
  errno= e;
  return -1;
 }
  if (memcmp(vb2.buf+8,"OK",2)) Psyntax("success/fail not E* or OK");
  vb2.used= 8+2;
  r= 0;
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
int Hclose(	int fd 	) {
 int r, amtread;
 Qclose(	fd 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," close=",7)) Psyntax("syscall reply mismatch");
 if (vb2.buf[7] == 'E') {
  int e;
  e= Perrno(vb2.buf+7);
  P_updatetime();
  errno= e;
  return -1;
 }
  if (memcmp(vb2.buf+7,"OK",2)) Psyntax("success/fail not E* or OK");
  vb2.used= 7+2;
  r= 0;
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
int Hsendto(	int fd , const void *msg , int msglen , unsigned int flags , const struct sockaddr *addr , int addrlen 	) {
 int r, amtread;
 char *ep;
	Tmust("sendto","flags",flags==0); 
 Qsendto(	fd , msg , msglen , addr , addrlen 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," sendto=",8)) Psyntax("syscall reply mismatch");
 if (vb2.buf[8] == 'E') {
  int e;
  e= Perrno(vb2.buf+8);
  P_updatetime();
  errno= e;
  return -1;
 }
  r= strtoul(vb2.buf+8,&ep,10);
  if (*ep && *ep!=' ') Psyntax("return value not E* or positive number");
  vb2.used= ep - (char*)vb2.buf;
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
int Hrecvfrom(	int fd , void *buf , int buflen , unsigned int flags , struct sockaddr *addr , int *addrlen 	) {
 int r, amtread;
	Tmust("recvfrom","flags",flags==0); 
	Tmust("recvfrom","*addrlen",*addrlen>=sizeof(struct sockaddr_in)); 
 Qrecvfrom(	fd , buflen , *addrlen 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," recvfrom=",10)) Psyntax("syscall reply mismatch");
 if (vb2.buf[10] == 'E') {
  int e;
  e= Perrno(vb2.buf+10);
  P_updatetime();
  errno= e;
  return -1;
 }
  if (memcmp(vb2.buf+10,"OK",2)) Psyntax("success/fail not E* or OK");
  vb2.used= 10+2;
  r= 0;
	Parg("addr"); Paddr(addr,addrlen); 
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
	r= Pbytes(buf,buflen); 
 P_updatetime();
 return r;
}
int Hread(	int fd , void *buf , size_t buflen 	) {
 int r, amtread;
 Qread(	fd , buflen 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," read=",6)) Psyntax("syscall reply mismatch");
 if (vb2.buf[6] == 'E') {
  int e;
  e= Perrno(vb2.buf+6);
  P_updatetime();
  errno= e;
  return -1;
 }
  if (memcmp(vb2.buf+6,"OK",2)) Psyntax("success/fail not E* or OK");
  vb2.used= 6+2;
  r= 0;
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
	r= Pbytes(buf,buflen); 
 P_updatetime();
 return r;
}
int Hwrite(	int fd , const void *buf , size_t len 	) {
 int r, amtread;
 char *ep;
 Qwrite(	fd , buf , len 	);
 if (!adns__vbuf_ensure(&vb2,1000)) Tnomem();
 fgets(vb2.buf,vb2.avail,Tinputfile); Pcheckinput();
 Tensurereportfile();
 fprintf(Treportfile,"%s",vb2.buf);
 amtread= strlen(vb2.buf);
 if (amtread<=0 || vb2.buf[--amtread]!='\n')
  Psyntax("badly formed line");
 vb2.buf[amtread]= 0;
 if (memcmp(vb2.buf," write=",7)) Psyntax("syscall reply mismatch");
 if (vb2.buf[7] == 'E') {
  int e;
  e= Perrno(vb2.buf+7);
  P_updatetime();
  errno= e;
  return -1;
 }
  r= strtoul(vb2.buf+7,&ep,10);
  if (*ep && *ep!=' ') Psyntax("return value not E* or positive number");
  vb2.used= ep - (char*)vb2.buf;
 assert(vb2.used <= amtread);
 if (vb2.used != amtread) Psyntax("junk at end of line");
 P_updatetime();
 return r;
}
