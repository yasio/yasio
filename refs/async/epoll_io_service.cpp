#ifdef __GNUC__
#include "container_helper.h" 
#include "epoll_io_service.h"

epoll_io_service::epoll_io_service(void)
{
    // create event queue fd
    this->evq = epoll_create(EPOLL_MAX_EVENTS);
    if(-1 == this->evq) {
        log_error(
            "epoll_io_service: create event trigger failed, errno: %d\n",
            errno);
        return;
    }
    this->initialized = true;
}

epoll_io_service::~epoll_io_service(void)
{
    this->stop(true);
    if(this->evq != -1)
    {
        ::close(this->evq);
        exx::container_helper::clear_c(this->op_worker_grp);
    }
}

bool epoll_io_service::register_descriptor(socket_handle fd, epoll_io_op* op)
{
    epoll_event ev = { 0, { 0 } };
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLPRI | EPOLLET; // 边缘触发模式完整标识
    return ::epoll_ctl(this->evq, EPOLL_CTL_ADD, fd, &ev) != -1;
}

bool epoll_io_service::resume_ev(socket_handle fd, epoll_io_op* op, int type) // 仅仅当EPOLLONESHOT标识设置后才需调用此接口
{
    this->evop.events = type;
    this->evop.data.ptr = op;
    return ::epoll_ctl(this->evq, EPOLL_CTL_MOD, fd, &this->evop) != -1;
}

// Initialise the task. Nothing to do here.
void epoll_io_service::start(size_t task_number)
{
    for(size_t i = 0; i < task_number; ++i)
    {
        this->op_worker_grp.push_back( new std::thread(&epoll_io_service::io_perfmon_entry, this) );
    }

    this->op_worker_grp.push_front(new std::thread(&epoll_io_service::svc, this));
}

void epoll_io_service::stop(bool force)
{
    if(this->initialized) {
        this->initialized = false;

        // notify all thread to stop working.
        this->opq_nonempty_cond.broadcast();

        // wait all worker threads exit.
        if(!force) {
            this->wait_all();
        }
    }
}

void epoll_io_service::wait_all(void)
{
    std::for_each(this->op_worker_grp.begin(), 
        this->op_worker_grp.end(),
        std::mem_fun(&std::thread::join));
}

void epoll_io_service::svc(void) 
{
    log_trace("listen thread started, LWP: %ld\n", getlwpid());

    int nfds = 0;
    int timeo = 10;
    for(;this->initialized;)
    {
        epoll_event events[128];
        nfds = epoll_wait(this->evq, events, 128, timeo);

        if(nfds < 0) {
            if(EINTR == errno) { // interrupted by high privilege syscall
                printf("conn_job_manager: wait events, errinfo: %s\n", 
                    strerror(errno));
                continue;
            }
            fprintf(stderr, "conn_job_manager: wait events failed, return: %d, errno: %s\n",
                nfds,
                strerror(errno) );
            break;
        }

        for(int i = 0; i < nfds; ++i)
        {
            /* put a read ready op into the work list */
            this->opq_mutex.lock();
            // log_trace("put a new ready descroptor to queue of io worker group.\n");

            this->opq.push_back(static_cast<epoll_io_op*>(events[i].data.ptr));
            this->opq_mutex.unlock();
            this->opq_nonempty_cond.signal();
        }
    }
}

void epoll_io_service::io_perfmon_entry(void)
{
    epoll_io_op* op = nullptr;
    log_trace("io perfmon thread started, LWP: %ld\n", getlwpid());
L_Loop:
    this->opq_mutex.lock();
    while(this->opq.empty() && this->initialized)
    {
        this->opq_nonempty_cond.wait(this->opq_mutex);
    }

    if(!this->initialized) {
        this->opq_mutex.unlock(); // release the lock
        log_trace("epoll_io_service: the thread LWP: %#lx will exit.\n", getlwpid());
        return; // exit this thread
    }

    // dequeue from head of service queue and release lock immediately
    op = *this->opq.begin();
    this->opq.pop_front();
    this->opq_mutex.unlock();

    // execute the head of the service queue, consider the infinite-loop
    op->complete(*this, 0, 0);

    // go on to execute next service
    goto L_Loop;
}

object_pool<op_max_capacity> epoll_op_gc;


#endif

