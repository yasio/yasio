// HALX99: 2015~2018 V2.0
#ifndef _FASTEST_CSV_PARSER_H_
#define _FASTEST_CSV_PARSER_H_
#include <set>
#include <map>
#include <vector>
#include <unordered_map>

class fastest_csv_parser
{
public:
    
    /*
    * op prototype: op(const char* v_start, const char* v_end)
    */
    template<typename _Fty> inline
        static	char* csv_parse_line(char* s, _Fty op)
    {
        // FSM
        enum {
            normal, // new field
            quote_field,
            quote_field_try_end,
            quote_field_end,
        } state;

        state = normal;

        char* _Start = s; // the start of every string
        char* _Ptr = s;   // source string iterator
        char* _Quote = nullptr;
        int skipCRLF = 1;

    _L_loop:
        {
			switch (*_Ptr)
			{
			case ',':
				switch (state) {
				case normal:
					if (_Start <= _Ptr)
						op(_Start, _Ptr);
					_Start = _Ptr + 1;
					break;
                case quote_field_try_end:
                    if (_Start < _Ptr)
                        op(_Start, _Ptr - 1);
                case quote_field_end:
					state = normal;
					_Start = _Ptr + 1; // skip the field end '\"'
					break;
				default:; // quote_field, do nothing
				}

				break;
			case '\"':
                switch (state) {
                case normal:
                    state = quote_field;
                    ++_Start; // skip left '\"'
                    break;
                case quote_field: // field end by right '\"'
                    _Quote = _Ptr;
                    state = quote_field_try_end; // delay 1 loop to detect whether is a inner quote, not end for field
                    break;
                case quote_field_try_end:
                    if (_Ptr - _Quote > 1) {
                        state = quote_field_end;
                        op(_Start, _Ptr);
                    }
                    else {
                        state = quote_field;
                    }
                    break;
                default:;
                }
                break;
            case '\r':
                skipCRLF = 2;
            case '\n':
            case '\0':
                switch (state)
                {
                case normal:
                    if (_Start <= _Ptr) 
                        op(_Start, _Ptr);
                    break;
                case quote_field_try_end:
                    if (_Start < _Ptr) 
                        op(_Start, _Ptr - 1);
                    break;
                default:; // other state, do nothing, quote_field, invalid csv format
                }
                goto _L_end;
            }
            ++_Ptr;
			
            goto _L_loop;
        }

    _L_end:
        return _Ptr + skipCRLF; // pointer to next line or after of null-termainted-charactor
    }
};

#endif

