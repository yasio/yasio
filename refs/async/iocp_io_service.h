#ifndef _IOCP_IO_SERVCIE_H_
#define _IOCP_IO_SERVCIE_H_
#include "iocp_io_op.h"
#include "iocp_accept_op.h"

class iocp_io_service
{
public:

    // Constructor. Specifies a concurrency hint that is passed through to the
    // underlying I/O completion port.
    iocp_io_service(void);

    ~iocp_io_service(void);

    // Initialise the task. Nothing to do here.
    void start(int);

    // Destroy all user-defined handler objects owned by the service.
    void stop(bool force = false);

    void wait_all(void);

    // Register a handle with the IO completion port.
    template<typename _Handle>
    bool register_descriptor(_Handle handle)
    {
        return (::CreateIoCompletionPort((HANDLE)handle, iocp_.handle, 0, 0) != nullptr);
    }

    // Helper class for managing a HANDLE.
    struct auto_handle
    {
        HANDLE handle;
        auto_handle() : handle(0) {}
        ~auto_handle() { if (handle) ::CloseHandle(handle); }
    };

private:
    void run(void);

    // The IO completion port used for queueing operations.
    auto_handle iocp_;

    std::list<std::thread*>    op_worker_grp;
};

#endif
/*
* Copyright (c) 2012 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/
