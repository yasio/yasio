#ifndef _SQLITE3PP_H_
#define _SQLITE3PP_H_

#define SQLITE3_MEMORY_FILENAME ":memory:"

struct sqlite3;

namespace sqlite3pp {

    struct sqlite3_result_set_t
    {
        char** result_;
        int    cur_row_;
        int    row_;
        int    column_;
    };

    // CLASS fwd
    class database;
    
    class result_set
    {
        friend class database;
    public:
        result_set(void);
        ~result_set(void);

        bool is_valid(void) const;

        int count_of_rows(void) const;

        int count_of_fields(void) const;

        /* return data as array of strings */
        char** fetch_row(void);

    private:
        sqlite3_result_set_t internal_;
    };

    class database
    {
    public:
        database(void);
        ~database(void);

        bool open(const char* filename);

        bool load(const char* filename, bool replace = false);

        bool save(const char* filename) const;

        bool is_open(void) const;

        void close(void);

        /*
        ** @brief: execute query SQL
        ** params: 
        **       sqlstr: SQL cmd to execute, such as: "select * from test where cout=19"
        **       resultset: output parameter to save resultset for a query
        **                  default: nullptr(ignore resultset)
        */
        bool  exec_query(const char* sqlstr);

        bool  exec_query(const char* sqlstr, result_set& results);

    private:
        static bool copy(sqlite3* from, sqlite3* to);

    private:
        sqlite3* db_;
        bool     temporary_;
    };
}

#endif

