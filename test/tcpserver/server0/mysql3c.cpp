#include <string.h>
#include "mysql3c.h"
#ifdef _WIN32
#pragma comment(lib, "libmysql.lib")
#endif
using namespace mysql3c;

mysql3c::result_set::result_set( void ) : res(nullptr)
{
}

mysql3c::result_set::~result_set( void )
{
    if(this->is_valid()) {
        mysql_free_result(res);
        res = nullptr;
    }
}

bool mysql3c::result_set::is_valid( void ) const
{
    return this->res != nullptr;
}

my_ulonglong mysql3c::result_set::count_of_rows( void )
{
    return mysql_num_rows(this->res);
}

my_ulonglong mysql3c::result_set::count_of_fields( void )
{
    return mysql_num_fields(this->res);
}

char** mysql3c::result_set::fetch_row( void )
{
    return mysql_fetch_row(this->res);
}


mysql3c::statement::statement( void ) : stmt(NULL), rowbuf(NULL), column_count(0)
{

}

mysql3c::statement::~statement( void )
{
    this->release();
}

int mysql3c::statement::prepare( const char* sqlstr, enum_field_types types[], int count )
{
    this->reset();
    if(0 == mysql_stmt_prepare(this->stmt, sqlstr, strlen(sqlstr)))
    {
        this->column_count = mysql_stmt_param_count(this->stmt);
    }
    if(this->column_count > 0 && (size_t)count == this->column_count)
    {
        this->rowbuf = (user_data*)calloc(this->column_count, sizeof(user_data));

        for(int i = 0; i < count; ++i)
        {
            this->rowbuf[i].buffer_type = types[i];
            this->rowbuf[i].buffer_length = 16;
            this->rowbuf[i].buffer = malloc(16);
        }

        int ret = mysql_stmt_bind_param(this->stmt, this->rowbuf);
        return ret;
    }
    return -1;
}

void mysql3c::statement::fill( const void* value, unsigned long valuelen, int index )
{
    if(valuelen >  this->rowbuf[index].buffer_length)
    {
        this->rowbuf[index].buffer = realloc(this->rowbuf[index].buffer, valuelen);
        this->rowbuf[index].buffer_length = valuelen;
    }
    memcpy(this->rowbuf[index].buffer, value, valuelen);
}

int mysql3c::statement::execute( void )
{
    return mysql_stmt_execute(this->stmt);
}

int mysql3c::statement::commit( void )
{
    return mysql_commit(this->stmt->mysql);
}

void mysql3c::statement::reset( void )
{
    if(this->rowbuf != NULL) {
        for(size_t i = 0; i < this->column_count; ++i)
        {
            free(this->rowbuf[i].buffer);
        }
        free(this->rowbuf);
        this->rowbuf = NULL;
        mysql_stmt_reset(this->stmt);
    }
}

void mysql3c::statement::release( void )
{
    if(this->stmt != NULL)
    {
        this->reset();
        mysql_stmt_close(this->stmt);
        this->stmt = NULL;
    }
}

database::database(void)
{
    mysql_init(&this->handle);
}


database::~database(void)
{
    this->close();
}

bool database::open(const char* dbname, const char* addr, u_short port, const char* user, const char* password)
{
    bool succeed = mysql_real_connect(&this->handle, addr, user, password, dbname, port, NULL, 0) != NULL;
    this->handle.reconnect = true;
    return succeed;
}

bool database::is_open(void) const
{
    return this->handle.db != NULL;
}

void database::close(void)
{
    if(this->is_open()) {
        mysql_close(&this->handle);
    }
}

bool database::get_statement(statement& stmt)
{
    stmt.stmt = mysql_stmt_init(&this->handle);
    return stmt.stmt != nullptr;
}

bool database::exec_query(const char* sqlstr, result_set* resultset)
{
    mtx.lock();
    bool retval = ( 0 == mysql_query(&this->handle, sqlstr) );
    if(retval && resultset != nullptr)
    {
        resultset->res = mysql_store_result(&this->handle);
        mtx.unlock();
        return retval;
    }
    mtx.unlock();
    return retval;
}

