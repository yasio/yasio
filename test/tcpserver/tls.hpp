#ifndef _TCPSERVER_TLS_H_
#define _TCPSERVER_TLS_H_

void tls_ctx_init();
void tls_ctx_end();

void* tls_get_connection();
void tls_release_connection(void*);

void* tls_get_buffer(size_t size);
void tls_release_buffer(void*);

void gls_ctx_init();
void gls_ctx_end();

void* gls_get_connection();
void gls_release_connection(void*);

void* gls_get_buffer(size_t size);
void gls_release_buffer(void*p);

#endif

