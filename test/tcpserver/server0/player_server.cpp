#include "player_session.h"
#include "player_session_manager.h"
#include "game_hall_manager.h"
#include "auto_resources.h"
#include "server_config.h"
#include "mysql3c.h"
#include "player_server.h"

player_server::player_server(void)
{
    this->state_ = server_state_stopped;
    this->hall_manager_ = new game_hall_manager(*this, this->io_service_);
    this->session_manager_ = new player_session_manager(*this, this->io_service_);
}

player_server::~player_server(void)
{
    this->post_stop_signals();
    this->wait_all_workers();
    delete this->session_manager_;
    delete this->hall_manager_;
}

player_session_manager& player_server::get_session_manager(void)
{
    return *this->session_manager_;
}

game_hall_manager&   player_server::get_hall_manager(void)
{
    return *this->hall_manager_;
}

bool player_server::start(const char* addr, u_short port)
{
    this->tcp_acceptor_ = new tcp::acceptor(this->io_service_, 
        tcp::endpoint(boost::asio::ip::address::from_string(addr), port));

    this->prepare_accept(MAX_CONN_COUNT + 1);

    if(this->tcp_acceptor_->is_open())
    {
        this->state_ = server_state_starting;
    }

    if(server_state_starting == this->state_)
    {
        LOG_TRACE("start server sucessfully, listen at %s:%u...\n",
            this->tcp_acceptor_->local_endpoint().address().to_string().c_str(),
            this->tcp_acceptor_->local_endpoint().port());

        for(int i = 0; i < ar_server_config.thread_count_; ++i)
        {
            this->svc_worker_grp_.push_back( std::shared_ptr<boost::thread>(new boost::thread( boost::bind(&player_server::svc, this) ) ) );
        }
        this->state_ = server_state_started;
    }

    return server_state_started == this->state_;
}

void player_server::post_stop_signals(void)
{
    if(server_state_started == this->state_)
    {
        this->state_ = server_state_stopping;
        this->hall_manager_->cancel_all_games();
        this->session_manager_->cancel_session_timeout_check(); // stop all client session

        this->tcp_acceptor_->close();
        delete this->tcp_acceptor_;
        this->tcp_acceptor_ = nullptr;

        this->io_service_.stop();

        LOG_TRACE("send stop signals all thread of the server sucessfully.\n");
    }
}


void player_server::wait_all_workers(void)
{
    scoped_lock<thread_mutex> guard(this->state_mutex_);

    if(server_state_stopping == this->state_ || server_state_started == this->state_) {
        /*std::for_each(
            this->svc_worker_grp_.begin(),
            this->svc_worker_grp_.end(),
            [](std::shared_ptr<boost::thread>& thr) 
        {
            thr->join();
        });*/
        for(auto iter = this->svc_worker_grp_.begin(); iter != this->svc_worker_grp_.end(); ++iter)
        {   
            (*iter)->join();
        }  
        this->state_ = server_state_stopped;
    }
}

void  player_server::prepare_accept(size_t number)
{
    player_session* newSession = nullptr;
    for(size_t i = 0; i < number; ++i) {
        newSession = new(ar_conn_pool.get()) player_session(*this->session_manager_, this->io_service_);
        this->tcp_acceptor_->async_accept(newSession->get_socket(),
            boost::bind(&player_server::on_accept, 
            this,
            newSession, 
            boost::asio::placeholders::error
            )
            );
    }
}

void  player_server::on_accept(player_session* new_session, const boost::system::error_code& error)
{
    if(!error) 
    {
        assert(new_session->is_valid());
        this->session_manager_->add(new_session);
    }
}

void player_server::svc(void)
{
    try {
        this->io_service_.run();
    }
    catch(boost::exception& e)
    {
        LOG_TRACE_ALL("the thread: %#x exit exception, detail: %s", thr_self(), boost::diagnostic_information(e).c_str());
        return;
    }

    LOG_TRACE_ALL("the thread: %#x exit normally.\n", thr_self());
}

