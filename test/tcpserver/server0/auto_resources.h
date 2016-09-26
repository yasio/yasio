#ifndef _AUTO_RESOURCES_H_
#define _AUTO_RESOURCES_H_
#include "xxpiedef.h"

typedef std::map<unsigned long, ngx_pool_t*> thread_mpool_table_t;

class server_config;
class xxpie_logger;
class game_widget_table;
struct level_exp_pr;
namespace mysql3c {
class database;
};

namespace auto_resources
{
    extern server_config        ar_server_config;
    extern game_widget_table    ar_game_widget_table;
    extern ngx_pool_t*          ar_ngx_pool;
    extern thread_mutex         ar_ngx_pool_mutex;
    extern mysql3c::database    ar_db;
    extern unsigned int         ar_random_seed;
	extern exx::gc::object_pool<player_session, MAX_CONN_COUNT + 1> ar_conn_pool;
};

using namespace auto_resources;

#define mysqltc ar_db_pool.find(thr_self())->second

#define xmpool_alloc(size) ngx_palloc(ar_ngx_pool, size)
#define xmpool_free(ptr)   ngx_pfree(ar_ngx_pool, ptr)
#define mpool_alloc(size)  xmpool_alloc_atom(size)
#define mpool_free(ptr)    xmpool_free_atom(ptr)

extern void* xmpool_alloc_atom(size_t size);
extern void  xmpool_free_atom(void* ptr);
extern int   next_rand_int(void);


extern int64_t eval_exp_by_level(double level);
extern double eval_level_by_exp(int64_t exp);

extern void xxpie_server_initialize(void);
#endif

