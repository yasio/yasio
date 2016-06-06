#include <stdarg.h>
#include "server_logger.hpp"

enum {MAX_LEN = 1024 * 16};

static void dump_hex_i(const char* prompt, const char* hex, int len, FILE* fp = stdout, char delim = ',', int line_feed = 16)
{
    fprintf(fp, "%s\n", prompt);
    for(int i = 0; i < len; ++i)
    {
        if( i + 1 != len) {
            if( (i + 1) % line_feed != 0 ) {
                fprintf(fp, "0x%02x%c ", (unsigned char)hex[i], delim);
            }
            else {
                fprintf(fp, "0x%02x%c\n", (unsigned char)hex[i], delim);
            }
        }
        else {
            fprintf(fp, "0x%02x\n", (unsigned char)hex[i]);
        }
    }
    fprintf(fp, "\n");
}

server_logger::server_logger(const char* filename)
{
    if(filename != nullptr) 
    {
        this->log_ = fopen(filename, "w+");
    }
}

server_logger::~server_logger(void)
{
    if(this->log_ != nullptr) fclose(this->log_);
}

void server_logger::log(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);

    this->mutex_.lock();

    // output debug string
#if defined(_WIN32) && defined(_DEBUG)
    char szBuf[MAX_LEN];
    vsnprintf_s(szBuf, MAX_LEN, MAX_LEN, format, ap);
    OutputDebugStringA(szBuf);
#endif
    
    if(this->log_) vfprintf(this->log_, format, ap);
    fflush(this->log_); 

    this->mutex_.unlock();

    va_end(ap); 
}

void server_logger::log_all(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);

    this->mutex_.lock();
    
    // output debug string
#if defined(_WIN32) && defined(_DEBUG)
    char szBuf[MAX_LEN];
    vsnprintf_s(szBuf, MAX_LEN, MAX_LEN, format, ap);
    OutputDebugStringA(szBuf);
#endif

    vfprintf(stdout, format, ap);
    if(this->log_) 
    {
        va_start(ap, format);
        vfprintf(this->log_, format, ap);
        fflush(this->log_); 
    }

    this->mutex_.unlock();

    va_end(ap); 
}

void server_logger::dump_hex(const char* prompt, const char* hex, int len)
{
    this->mutex_.lock();
    dump_hex_i(prompt, hex, len, this->log_);
    this->mutex_.unlock();
}

