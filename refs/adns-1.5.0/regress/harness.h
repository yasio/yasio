#ifndef HARNESS_H_INCLUDED
#define HARNESS_H_INCLUDED
#include "internal.h"
#include "hsyscalls.h"
/* There is a Q function (Q for Question) for each such syscall;
 * it constructs a string representing the call, and calls Q_str
 * on it, or constructs it in vb and calls Q_vb;
 */
void Qselect(	int max , const fd_set *rfds , const fd_set *wfds , const fd_set *efds , struct timeval *to 	);
#ifdef HAVE_POLL
void Qpoll(	const struct pollfd *fds , int nfds , int timeout 	);
#endif
void Qsocket(	int domain , int type 	);
void Qfcntl(	int fd , int cmd , long arg 	);
void Qconnect(	int fd , const struct sockaddr *addr , int addrlen 	);
void Qbind(	int fd , const struct sockaddr *addr , int addrlen 	);
void Qlisten(	int fd , int backlog 	);
void Qclose(	int fd 	);
void Qsendto(	int fd , const void *msg , int msglen , const struct sockaddr *addr , int addrlen 	);
void Qrecvfrom(	int fd , int buflen , int addrlen 	);
void Qread(	int fd , size_t buflen 	);
void Qwrite(	int fd , const void *buf , size_t len 	);
void Q_vb(void);
extern void Tshutdown(void);
/* General help functions */
void Tfailed(const char *why);
void Toutputerr(void);
void Tnomem(void);
void Tfsyscallr(const char *fmt, ...) PRINTFFORMAT(1,2);
void Tensurerecordfile(void);
void Tmust(const char *call, const char *arg, int cond);
void Tvbf(const char *fmt, ...) PRINTFFORMAT(1,2);
void Tvbvf(const char *fmt, va_list al);
void Tvbfdset(int max, const fd_set *set);
void Tvbpollfds(const struct pollfd *fds, int nfds);
void Tvbaddr(const struct sockaddr *addr, int addrlen);
void Tvbbytes(const void *buf, int len);
void Tvberrno(int e);
void Tvba(const char *str);
/* Shared globals */
extern vbuf vb;
extern struct timeval currenttime;
extern const struct Terrno { const char *n; int v; } Terrnos[];
#endif
