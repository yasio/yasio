#ifdef _WIN32
#include <algorithm>
#include "container_helper.h"
#include "object_pool.h"
#include "iocp_io_service.h"

using namespace thelib::container_helper;

iocp_io_service::iocp_io_service(void)
{
    iocp_.handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
}

iocp_io_service::~iocp_io_service(void)
{
    this->stop(true);
    clear_c(this->op_worker_grp);
}

// Initialise the task. Nothing to do here.
void iocp_io_service::start(int task_number)
{
    for(size_t i = 0; i < task_number; ++i)
    {
        this->op_worker_grp.push_back( new std::thread(
            std::bind(&iocp_io_service::run, this) ) );
    }
}

// Destroy all user-defined handler objects owned by the service.
void iocp_io_service::stop(bool force)
{
    ::CloseHandle(this->iocp_.handle);
    if(!force) {
        this->wait_all();
    }
    this->iocp_.handle = nullptr;
}

void iocp_io_service::wait_all(void)
{
    std::for_each(this->op_worker_grp.begin(), 
        this->op_worker_grp.end(),
        std::mem_fun(&std::thread::join));
}


void iocp_io_service::run(void)
{
    for (;;)
    {
        DWORD bytes_transferred = 0;
        ULONG_PTR completion_key = 0;
        LPOVERLAPPED overlapped = 0;
        ::SetLastError(0);
        BOOL ok = ::GetQueuedCompletionStatus(iocp_.handle, &bytes_transferred,
            &completion_key, &overlapped, INFINITE);
        DWORD last_error = ::GetLastError();

        if (overlapped)
        {
            iocp_io_op* op = static_cast<iocp_io_op*>(overlapped);
            op->complete(*this, last_error, bytes_transferred);
            iocp_op_gc.release(op);
            continue;
        }
        else if (!ok)
        {
            if (last_error != WAIT_TIMEOUT)
            {
                log_error("iocp error occured, error code: %d\n", last_error);
                break;
            }
            continue;
        }
    }
}

thelib::gc::object_pool<op_max_capacity> iocp_op_gc;

#endif

