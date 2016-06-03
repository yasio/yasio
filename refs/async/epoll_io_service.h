/*********************************************************************
* conn_job_manager.h - Class conn_job_manager
*                Copyright (c) 2012 pholang.com. All rights reserved.
*
* Author: xml3see
*
* Purpose: create new job for a player connection, and register job evnet
*          by call 'register_ev' of connection event listener.
* 
* Version: 1.0.0
*
*********************************************************************/
#ifndef _EPOLL_IO_SERVICE_H_
#define _EPOLL_IO_SERVICE_H_
#include "epoll_io_op.h"

class epoll_io_service
{
    
public:
    epoll_io_service(void);

    ~epoll_io_service(void);

    bool register_descriptor(socket_handle fd, epoll_io_op* op);

    bool unregister_descriptor(socket_handle fd, epoll_io_op* op);

    // Initialise the task. Nothing to do here.
    void start(size_t task_number);

    void stop(bool force);

    void wait_all(void);

    void run(void) ;

    void io_perfmon_entry(void);

   
private:
    bool                       initialized_;      // initialized

    /// events listener
    int                        evq_;              // event listenning queue, control monitor behavior.
    struct epoll_event         evop_;             // resource: ev for op
    
    std::list<epoll_io_op*>    op_queue_;               // op queue
    thread_mutex               op_queue_mtx_;         // mutex for job manager
    thread_cond                op_queue_cv_; 

    /// io event operator
    // std::list<std::thread*>    op_worker_grp;     // threads for complete io operation
    
};

#endif

