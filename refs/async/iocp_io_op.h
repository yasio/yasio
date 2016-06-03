#ifndef _IOCP_IO_OP_H_
#define _IOCP_IO_OP_H_
#include "iocp_fwd.h"

class iocp_io_op : public OVERLAPPED
{
    typedef void (*func_type)(iocp_io_service*, 
        iocp_io_op*, 
        u_long ec, 
        size_t bytes_transferred);
public:
    iocp_io_op(func_type func) : func_(func)
    {
        this->reset();
    }

    void complete(iocp_io_service& owner, u_long ec, size_t bytes_transferred)
    {
        this->func_(&owner, this, ec, bytes_transferred);
    }

    void reset(void)
    {
        this->Internal = 0;
        this->InternalHigh = 0;
        this->Offset = 0;
        this->OffsetHigh = 0;
        this->hEvent = 0;
    }

private:
    func_type func_;
};


#endif
/*
* Copyright (c) 2012-2019 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

