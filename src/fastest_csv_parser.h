#ifndef _FASTEST_CSV_PARSER_H_
#define _FASTEST_CSV_PARSER_H_
#include <set>
#include <map>
#include <vector>
#include <unordered_map>
#include "xxfsutility.h"

class fastest_csv_parser
{
public:
    // test & usage for csv parse
    void parse_csv(const char* filename)
    {
        std::string buffer = fsutil::read_file_data(filename);

        if (buffer.empty())
            return;

        const char* newl = buffer.c_str();

        do {
            std::vector<std::string> record;
            newl = csv_parse_line(newl, [&record](const char* v_start, const char* v_end) {
                std::string temp = std::string(v_start, v_end);
                record.push_back(std::move(temp));
            });
            t_csv.push_back(std::move(record));
        } while ((newl - buffer.c_str()) < buffer.size());
    }
    std::vector<std::vector<std::string>> t_csv;

    /*
    * op prototype: op(const char* v_start, const char* v_end)
    */
    template<const char _Delim, typename _Fty> inline
        static const char* xsv_pase_line(const char* s, _Fty op)
    {
        enum {
            normal, // new field
            explicit_string_start,
            explicit_string_ending,
        } state;

        state = normal;

        const char* _Start = s; // the start of every string
        const char* _Ptr = s;   // source string iterator
        int _LLF = 1; // skip CRLF

    _L_loop:
        {
            switch (*_Ptr)
            {
            case _Delim:
                switch (state) {
                case normal:
                    if (_Start <= _Ptr)
                        op(_Start, _Ptr);
                    _Start = _Ptr + 1;
                    break;
                case explicit_string_ending:
                    state = normal;
                    _Start = _Ptr + 1;
                    break;
                default:; // explicit_string_start, do nothing
                }
                break;
            case '\"':
                if (state == normal) {
                    state = explicit_string_start;
                    ++_Start; // skip left '\"'
                }
                else { // field end by right '\"'
                    if (_Start <= _Ptr)
                        op(_Start, _Ptr);
                    state = explicit_string_ending;
                }
                break;

            case '\r':
                _LLF = 2;
            case '\n':
            case '\0':
                if (_Start <= _Ptr && state == normal) {
                    op(_Start, _Ptr);
                }
                goto _L_end;
                break;
            }
            ++_Ptr;
            goto _L_loop;
        }

    _L_end:
        return _Ptr + _LLF; // pointer to next line or after of null-termainted-charactor
    }

    /*
    * op prototype: op(const char* v_start, const char* v_end)
    */
    template<typename _Fty> inline
        static const char* csv_parse_line(const char* s, _Fty op)
    {
        return xsv_pase_line<',', _Fty>(s, op);
    }
};

#endif

