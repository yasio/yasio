#include <string.h>
#include "mysqlcc.h"
#ifdef _WIN32
#ifdef _DEBUG
#pragma comment(lib, "libmysqld.lib")
#else
#pragma comment(lib, "libmysql.lib")
#endif
#endif
using namespace mysqlcc;

mysqlcc::result_set::result_set( void ) : res(nullptr)
{
}

mysqlcc::result_set::result_set( MYSQL_RES* theRes ) : res(theRes)
{
}

mysqlcc::result_set::~result_set( void )
{
    this->destory();
}

void mysqlcc::result_set::assign(MYSQL_RES* res)
{
    this->destory();
    this->res = res;
}

void mysqlcc::result_set::destory(void)
{
	 if(this->validated()) {
        mysql_free_result(res);
        res = nullptr;
    }
}

MYSQL_RES* mysqlcc::result_set::release( void )
{
	MYSQL_RES* tmp = this->res;
	this->res = nullptr;
	return tmp;
}
 
bool mysqlcc::result_set::has_on_least(void)
{
	return count_of_rows() >= 1;
}

bool mysqlcc::result_set::has_one(void)
{
	return count_of_rows() == 1;
}

bool mysqlcc::result_set::validated( void ) const
{
    return this->res != nullptr;
}

my_ulonglong mysqlcc::result_set::count_of_rows( void )
{
    return mysql_num_rows(this->res);
}

my_ulonglong mysqlcc::result_set::count_of_fields( void )
{
    return mysql_num_fields(this->res);
}

char** mysqlcc::result_set::fetch_row( void )
{
    return mysql_fetch_row(this->res);
}


mysqlcc::statement::statement( void ) : stmt(NULL), rowbuf(NULL), column_count(0)
{

}

mysqlcc::statement::~statement( void )
{
    this->release();
}

int mysqlcc::statement::prepare( const char* sqlstr, enum_field_types types[], int count )
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

void mysqlcc::statement::fill( const void* value, unsigned long valuelen, int index )
{
    if(valuelen >  this->rowbuf[index].buffer_length)
    {
        this->rowbuf[index].buffer = realloc(this->rowbuf[index].buffer, valuelen);
        this->rowbuf[index].buffer_length = valuelen;
    }
    memcpy(this->rowbuf[index].buffer, value, valuelen);
}

int mysqlcc::statement::execute( void )
{
    return mysql_stmt_execute(this->stmt);
}

int mysqlcc::statement::commit( void )
{
    return mysql_commit(this->stmt->mysql);
}

void mysqlcc::statement::reset( void )
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

void mysqlcc::statement::release( void )
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
}

database::~database(void)
{
    this->close();
}

bool database::open(const char* dbname, const char* addr, u_short port, const char* user, const char* password)
{
    mysql_init(&this->handle);

    bool succeed = mysql_real_connect(&this->handle, addr, user, password, dbname, port, NULL, 0) != NULL;
    if(succeed) {
        this->handle.reconnect = true;
    }
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
	//mysql_set_character_set(&this->handle, "UTF-8");
	//mysql_options(&this->handle, MYSQL_SET_CHARSET_NAME, "gbk"); 
	//character_set_results(
	//mysql_options(&this->handle, 
	//const char* name = mysql_character_set_name(&this->handle);
    // mtx.lock();
    bool retval = ( 0 == mysql_query(&this->handle, sqlstr) );
    if(retval && resultset != nullptr)
    {
        resultset->res = mysql_store_result(&this->handle);
//        mtx.unlock();
        return retval;
    }
    
    // mtx.unlock();
    return retval;
}

my_ulonglong database::get_insert_id(void)
{
	return mysql_insert_id(&this->handle);
}

const char* database::get_last_error(void)
{
    return mysql_error(&this->handle);
}

int database::get_last_errno(void)
{
	return mysql_errno(&this->handle);
}

