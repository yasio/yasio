//#include <io.h>
#include <string.h>
#include "sqlite3.h"
#include "sqlite3pp.h"

#ifndef _WIN32
#define nullptr 0
#endif

using namespace sqlite3pp;

result_set::result_set(void)
{
    memset(&this->internal_, 0x0, sizeof(this->internal_));
}

result_set::~result_set(void)
{
    if(is_valid())
    {
        sqlite3_free_table(this->internal_.result_);
    }
}

bool result_set::is_valid(void) const
{
    return this->internal_.result_ != nullptr;
}

int result_set::count_of_rows(void) const
{
    return this->internal_.row_;
}

int result_set::count_of_fields(void) const
{
    return this->internal_.column_;
}

/* return data as array of strings */
char** result_set::fetch_row(void)
{
    this->internal_.cur_row_ += this->internal_.column_;
    return this->internal_.result_ + this->internal_.cur_row_;
}

/*--------------------------- friendly divisior line ---------------------------*/

database::database(void) : db_(nullptr), temporary_(false)
{
}

database::~database(void)
{
    this->close();
}

bool database::open(const char* filename)
{
    int retc = sqlite3_open(filename, &this->db_);

    if(SQLITE_OK == retc)
    {
        this->temporary_ = (0 == memcmp(filename, SQLITE3_MEMORY_FILENAME, sizeof(SQLITE3_MEMORY_FILENAME)));
        return true;
    }

    return false;
}

bool database::load(const char* filename, bool replace)
{
    if(!this->temporary_)
    {
        return false;
    }

    if(this->is_open())
    {
        if(replace) { this->close(); }
        else { return false; }
    }

    sqlite3* from = nullptr;
    if(SQLITE_OK == sqlite3_open(filename, &from))
    {
        bool succeed = database::copy(from, this->db_); 
        sqlite3_close(from);
        return succeed;
    }

    return false;
}

bool database::save(const char* filename) const
{
    if(!this->temporary_ || !this->is_open())
    {
        return false;
    }
      
    sqlite3* to = nullptr;  
    if(SQLITE_OK == sqlite3_open(filename, &to))
    {
        bool succeed = database::copy(this->db_, to);
        sqlite3_close(to);
        return succeed;
    }
    return false;
}

bool database::is_open(void) const
{
    return this->db_ != nullptr;
}

void database::close(void)
{
    if(this->is_open())
    {
        sqlite3_close(this->db_);
        this->db_ = nullptr;
    }
}

bool  database::exec_query(const char* sqlstr)
{
    return SQLITE_OK == sqlite3_exec(this->db_, sqlstr, nullptr, nullptr, nullptr);
}

bool  database::exec_query(const char* sqlstr, result_set& results)
{
    return SQLITE_OK == sqlite3_get_table(this->db_, 
        sqlstr,
        &(results.internal_.result_), 
        &(results.internal_.row_), 
        &(results.internal_.column_), 
        nullptr);
}

bool database::copy(sqlite3* from, sqlite3* to)
{
    sqlite3_backup* bak = sqlite3_backup_init(to, "main", from, "main");  

    if(bak != nullptr) 
    {
        (void)sqlite3_backup_step(bak, -1);  
        (void)sqlite3_backup_finish(bak); 

        return SQLITE_OK == sqlite3_errcode(to);
    }

    return false;
}

