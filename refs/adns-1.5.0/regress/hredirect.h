#ifndef HREDIRECT_H_INCLUDED
#define HREDIRECT_H_INCLUDED
#include "hsyscalls.h"
#undef select
#define select Hselect
#ifdef HAVE_POLL
#undef poll
#define poll Hpoll
#endif
#undef socket
#define socket Hsocket
#undef fcntl
#define fcntl Hfcntl
#undef connect
#define connect Hconnect
#undef bind
#define bind Hbind
#undef listen
#define listen Hlisten
#undef close
#define close Hclose
#undef sendto
#define sendto Hsendto
#undef recvfrom
#define recvfrom Hrecvfrom
#undef read
#define read Hread
#undef write
#define write Hwrite
#undef writev
#define writev Hwritev
#undef gettimeofday
#define gettimeofday Hgettimeofday
#undef getpid
#define getpid Hgetpid
#undef malloc
#define malloc Hmalloc
#undef free
#define free Hfree
#undef realloc
#define realloc Hrealloc
#undef exit
#define exit Hexit
#endif
