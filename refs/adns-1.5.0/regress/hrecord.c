#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "harness.h"
static FILE *Toutputfile;
void Tshutdown(void) {
}
static void R_recordtime(void) {
  int r;
  struct timeval tv, tvrel;
  Tensurerecordfile();
  r= gettimeofday(&tv,0); if (r) Tfailed("gettimeofday syscallbegin");
  tvrel.tv_sec= tv.tv_sec - currenttime.tv_sec;
  tvrel.tv_usec= tv.tv_usec - currenttime.tv_usec;
  if (tv.tv_usec < 0) { tvrel.tv_usec += 1000000; tvrel.tv_sec--; }
  Tvbf("\n +%ld.%06ld",(long)tvrel.tv_sec,(long)tvrel.tv_usec);
  currenttime= tv;
}
void Tensurerecordfile(void) {
  const char *fdstr;
  int fd, r;
  if (Toutputfile) return;
  Toutputfile= stdout;
  fdstr= getenv("ADNS_TEST_OUT_FD");
  if (fdstr) {
    fd= atoi(fdstr);
    Toutputfile= fdopen(fd,"a"); if (!Toutputfile) Tfailed("fdopen ADNS_TEST_OUT_FD");
  }
  r= gettimeofday(&currenttime,0); if (r) Tfailed("gettimeofday syscallbegin");
  if (fprintf(Toutputfile," start %ld.%06ld\n",
	      (long)currenttime.tv_sec,(long)currenttime.tv_usec) == EOF) Toutputerr();
}
void Q_vb(void) {
  if (!adns__vbuf_append(&vb,"",1)) Tnomem();
  Tensurerecordfile();
  if (fprintf(Toutputfile," %s\n",vb.buf) == EOF) Toutputerr();
  if (fflush(Toutputfile)) Toutputerr();
}
static void R_vb(void) {
  Q_vb();
}
int Hselect(	int max , fd_set *rfds , fd_set *wfds , fd_set *efds , struct timeval *to 	) {
 int r, e;
 Qselect(	max , rfds , wfds , efds , to 	);
 r= select(	max , rfds , wfds , efds , to 	);
 e= errno;
 vb.used= 0;
 Tvba("select=");
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tvbf("%d",r);
	Tvba(" rfds="); Tvbfdset(max,rfds); 
	Tvba(" wfds="); Tvbfdset(max,wfds); 
	Tvba(" efds="); Tvbfdset(max,efds); 
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
#ifdef HAVE_POLL
int Hpoll(	struct pollfd *fds , int nfds , int timeout 	) {
 int r, e;
 Qpoll(	fds , nfds , timeout 	);
 r= poll(	fds , nfds , timeout 	);
 e= errno;
 vb.used= 0;
 Tvba("poll=");
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tvbf("%d",r);
        Tvba(" fds="); Tvbpollfds(fds,nfds); 
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
#endif
int Hsocket(	int domain , int type , int protocol 	) {
 int r, e;
  Tmust("socket","domain",domain==AF_INET || domain==AF_INET6); 
  Tmust("socket","type",type==SOCK_STREAM || type==SOCK_DGRAM); 
 Qsocket(	domain , type 	);
 r= socket(	domain , type , protocol 	);
 e= errno;
 vb.used= 0;
 Tvba("socket=");
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tvbf("%d",r);
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hfcntl(	int fd , int cmd , ... 	) {
 int r, e;
	va_list al; long arg; 
  Tmust("fcntl","cmd",cmd==F_SETFL || cmd==F_GETFL);
  if (cmd == F_SETFL) {
    va_start(al,cmd); arg= va_arg(al,long); va_end(al);
  } else {
    arg= 0;
  } 
 Qfcntl(	fd , cmd , arg 	);
 r= fcntl(	fd , cmd , arg 	);
 e= errno;
 vb.used= 0;
 Tvba("fcntl=");
  if (r==-1) { Tvberrno(e); goto x_error; }
  if (cmd == F_GETFL) {
    Tvbf(r & O_NONBLOCK ? "O_NONBLOCK|..." : "~O_NONBLOCK&...");
  } else {
    if (cmd == F_SETFL) {
      Tmust("fcntl","return",!r);
    } else {
      Tmust("cmd","F_GETFL/F_SETFL",0);
    }
    Tvba("OK");
  }
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hconnect(	int fd , const struct sockaddr *addr , int addrlen 	) {
 int r, e;
 Qconnect(	fd , addr , addrlen 	);
 r= connect(	fd , addr , addrlen 	);
 e= errno;
 vb.used= 0;
 Tvba("connect=");
  if (r) { Tvberrno(e); goto x_error; }
  Tvba("OK");
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hbind(	int fd , const struct sockaddr *addr , int addrlen 	) {
 int r, e;
 Qbind(	fd , addr , addrlen 	);
 r= bind(	fd , addr , addrlen 	);
 e= errno;
 vb.used= 0;
 Tvba("bind=");
  if (r) { Tvberrno(e); goto x_error; }
  Tvba("OK");
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hlisten(	int fd , int backlog 	) {
 int r, e;
 Qlisten(	fd , backlog 	);
 r= listen(	fd , backlog 	);
 e= errno;
 vb.used= 0;
 Tvba("listen=");
  if (r) { Tvberrno(e); goto x_error; }
  Tvba("OK");
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hclose(	int fd 	) {
 int r, e;
 Qclose(	fd 	);
 r= close(	fd 	);
 e= errno;
 vb.used= 0;
 Tvba("close=");
  if (r) { Tvberrno(e); goto x_error; }
  Tvba("OK");
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hsendto(	int fd , const void *msg , int msglen , unsigned int flags , const struct sockaddr *addr , int addrlen 	) {
 int r, e;
	Tmust("sendto","flags",flags==0); 
 Qsendto(	fd , msg , msglen , addr , addrlen 	);
 r= sendto(	fd , msg , msglen , flags , addr , addrlen 	);
 e= errno;
 vb.used= 0;
 Tvba("sendto=");
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tvbf("%d",r);
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hrecvfrom(	int fd , void *buf , int buflen , unsigned int flags , struct sockaddr *addr , int *addrlen 	) {
 int r, e;
	Tmust("recvfrom","flags",flags==0); 
	Tmust("recvfrom","*addrlen",*addrlen>=sizeof(struct sockaddr_in)); 
 Qrecvfrom(	fd , buflen , *addrlen 	);
 r= recvfrom(	fd , buf , buflen , flags , addr , addrlen 	);
 e= errno;
 vb.used= 0;
 Tvba("recvfrom=");
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tmust("recvfrom","return",r<=buflen);
  Tvba("OK");
	Tvba(" addr="); Tvbaddr(addr,*addrlen); 
	Tvbbytes(buf,r); 
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hread(	int fd , void *buf , size_t buflen 	) {
 int r, e;
 Qread(	fd , buflen 	);
 r= read(	fd , buf , buflen 	);
 e= errno;
 vb.used= 0;
 Tvba("read=");
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tmust("read","return",r<=buflen);
  Tvba("OK");
	Tvbbytes(buf,r); 
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
int Hwrite(	int fd , const void *buf , size_t len 	) {
 int r, e;
 Qwrite(	fd , buf , len 	);
 r= write(	fd , buf , len 	);
 e= errno;
 vb.used= 0;
 Tvba("write=");
  if (r==-1) { Tvberrno(e); goto x_error; }
  Tvbf("%d",r);
 x_error:
 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
