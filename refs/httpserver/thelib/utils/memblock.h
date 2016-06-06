#ifndef _THELIB_MEMBLOCK_H_
#define _THELIB_MEMBLOCK_H_
#include "simpleppdef.h"

namespace thelib {
namespace gc {

class memblock 
{
    typedef struct free_link_node
    { 
        free_link_node* next; 
    } *free_link;

    typedef struct chunk_link_node
    {
        chunk_link_node* next; /* at start of data */
        char            data[0];
    } *chunk_link;

    memblock(const memblock&);
    void operator= (const memblock&);

public:
    memblock(size_t elemsize, size_t elemcount);

    ~memblock(void);

    void cleanup(void);

    void purge(void);

    size_t count(void) const;

    // if the type is not pod, you may be use placement new to call the constructor,
    // for example: _Ty* obj = new(pool.get()) _Ty(arg1,arg2,...);
    void* get(void) ;

    void release(void* _Ptr);

private:
    void _Enlarge(void);

private:
    free_link        _Myhead; // link to free head
    chunk_link       _Mychunk; // chunk link
    size_t           _Mycount; // allocated count 
    const size_t     _MyElemSize; // elem size
    const size_t     _MyElemCount; // elem count
}; /* equalize size block */

};
};


#endif
