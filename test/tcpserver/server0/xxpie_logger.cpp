#include <stdarg.h>
#include "xxpie_logger.h"

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

xxpie_logger::xxpie_logger(const char* filename, const char* err, const char* proto)
{
    if(filename != nullptr) 
    {
        this->out_ = fopen(filename, "w+");
    }
    if(err != nullptr)
    {
        this->err_ = fopen(err, "w+");
    }
    if(proto != nullptr)
    {
        this->proto_ = fopen(proto, "w+");
    }
}

xxpie_logger::~xxpie_logger(void)
{
    if(this->proto_ != nullptr) fclose(this->proto_);
    if(this->err_ != nullptr) fclose(this->err_);
    if(this->out_ != nullptr) fclose(this->out_);
}

void xxpie_logger::log(const char* format, ...)
{
    va_list ap;  
    va_start(ap, format);  

    this->mutex_.lock();
    
    if(this->out_) vfprintf(this->out_, format, ap);
    fflush(this->out_); 

    this->mutex_.unlock();

    va_end(ap); 
}

void xxpie_logger::log_as_error(const char* format, ...)
{
    va_list ap;  
    va_start(ap, format);  

    this->mutex_.lock();
    
    if(this->err_) vfprintf(this->err_, format, ap);
    fflush(this->err_); 

    this->mutex_.unlock();

    va_end(ap); 
}

void xxpie_logger::log_all(const char* format, ...)
{
    va_list ap;  
    va_start(ap, format);  

    this->mutex_.lock();
    
    vfprintf(stdout, format, ap);
    if(this->out_) 
    {
        va_start(ap, format);
        vfprintf(this->out_, format, ap);
        fflush(this->out_); 
    }

    this->mutex_.unlock();

    va_end(ap); 
}

void xxpie_logger::log_all_as_error(const char* format, ...)
{
    va_list ap;  
    va_start(ap, format);  

    this->mutex_.lock();

    vfprintf(stderr, format, ap);
    if(this->err_) 
    {
        va_start(ap, format);
        vfprintf(this->err_, format, ap);
        fflush(this->err_); 
    }

    this->mutex_.unlock();

    va_end(ap); 
}

void xxpie_logger::proto_log(const char* format, ...)
{
    va_list ap;  
    va_start(ap, format);  

    this->mutex_.lock();
    
    if(this->proto_) vfprintf(this->proto_, format, ap);
    fflush(this->err_); 

    this->mutex_.unlock();

    va_end(ap); 
}

void xxpie_logger::dump_hex(const char* prompt, const char* hex, int len)
{
    this->mutex_.lock();
    dump_hex_i(prompt, hex, len, this->out_);
    this->mutex_.unlock();
}

