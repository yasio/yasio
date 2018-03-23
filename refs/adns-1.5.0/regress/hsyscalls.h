#ifndef HSYSCALLS_H_INCLUDED
#define HSYSCALLS_H_INCLUDED
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif
int Hselect(	int max , fd_set *rfds , fd_set *wfds , fd_set *efds , struct timeval *to 	);
#ifdef HAVE_POLL
int Hpoll(	struct pollfd *fds , int nfds , int timeout 	);
#endif
int Hsocket(	int domain , int type , int protocol 	);
int Hfcntl(	int fd , int cmd , ... 	);
int Hconnect(	int fd , const struct sockaddr *addr , int addrlen 	);
int Hbind(	int fd , const struct sockaddr *addr , int addrlen 	);
int Hlisten(	int fd , int backlog 	);
int Hclose(	int fd 	);
int Hsendto(	int fd , const void *msg , int msglen , unsigned int flags , const struct sockaddr *addr , int addrlen 	);
int Hrecvfrom(	int fd , void *buf , int buflen , unsigned int flags , struct sockaddr *addr , int *addrlen 	);
int Hread(	int fd , void *buf , size_t buflen 	);
int Hwrite(	int fd , const void *buf , size_t len 	);
int Hwritev(int fd, const struct iovec *vector, size_t count);
int Hgettimeofday(struct timeval *tv, struct timezone *tz);
pid_t Hgetpid(void);
void* Hmalloc(size_t sz);
void Hfree(void *ptr);
void* Hrealloc(void *op, size_t nsz);
void Hexit(int rv)NONRETURNING;
#endif
