#include "player_server.h"
#include "player_session.h"
#include "auto_resources.h"
#include "server_config.h"
#include "player_session_manager.h"

player_session_manager::player_session_manager(player_server& owner, boost::asio::io_service& io_service) : owner_(owner), session_check_timer_(io_service)
{
    this->post_session_timeout_check();
}

player_session_manager::~player_session_manager(void)
{
}

void player_session_manager::add(player_session* session)
{
    if(this->session_set_unlogin_.size() >= (size_t)MAX_CONN_COUNT)
    {
        LOG_TRACE("The server threshold already arrived, current connection count: %u\n", this->session_set_unlogin_.size());
        session->close();
        ar_conn_pool.release(session);
        this->owner_.prepare_accept();
    }

    session->init();

    scoped_wlock<thread_rwlock> guard(this->session_set_unlogin_rwlock_);
    this->session_set_unlogin_.insert(session);
    LOG_TRACE("A session income, detail: %s, current total connection count: %u, address: %08x\n", session->get_addr_info(), this->session_set_unlogin_.size(), session);

    session->start();
}

bool player_session_manager::tryregister(player_session* first)
{
    scoped_wlock<thread_rwlock> tab_guard(this->session_tab_login_rwlock_);
    return this->session_tab_login_.insert(std::make_pair(first->get_id(), first)).second;
}

player_session*  player_session_manager::reregister(player_session* later)
{
    uint32_t user_id = later->get_id();
    player_session* previous = nullptr;
    
    scoped_wlock<thread_rwlock> tab_guard(this->session_tab_login_rwlock_);
    auto target = this->session_tab_login_.find(user_id);
    if(target != this->session_tab_login_.end())
    { // login succeed, remove from unlogin set and replace previously in the login table.
        previous = target->second;
        target->second = later;
    }
    else {
        // previously session already logout, login directly: also remove from unlogin set firstly.
        this->session_tab_login_.insert(std::make_pair(user_id, later));
    }
    return previous;
}

void player_session_manager::unregister(player_session* session)
{
    this->session_tab_login_rwlock_.lock_exclusive();

    auto target = this->session_tab_login_.end();
    if( (target = session_tab_login_.find(session->get_id()) ) != this->session_tab_login_.end() )
    {
        if(!session->is_kicked()) 
        { // only when it is not kicked, erase from loggin table;
            this->session_tab_login_.erase(target);
        }
    }

    this->session_tab_login_rwlock_.unlock_exclusive();
}

void  player_session_manager::cleanup(player_session* session)
{
    scoped_wlock<thread_rwlock> guard(this->session_set_unlogin_rwlock_);

    auto unlogin_iter = this->session_set_unlogin_.find(session);
    if(unlogin_iter != this->session_set_unlogin_.end())
    {
        this->session_set_unlogin_.erase(unlogin_iter);

        LOG_TRACE("A session leave, detail: %s, current total connection count: %u\n", session->get_addr_info(), this->session_set_unlogin_.size());

        ar_conn_pool.release(session);
        this->owner_.prepare_accept();
    }
}

void  player_session_manager::post_session_timeout_check(void)
{
    this->session_check_timer_.expires_from_now(boost::posix_time::seconds(ar_server_config.heartbeat_check_interval_));
    this->session_check_timer_.async_wait(
        boost::bind(
        &player_session_manager::on_session_timeout_checking,
        this,
        boost::asio::placeholders::error)
        );
}

void player_session_manager::on_session_timeout_checking(const boost::system::error_code& ec)
{
    if(!ec)
    {
        scoped_rlock<thread_rwlock> shared_guard(this->session_set_unlogin_rwlock_);

        std::for_each(this->session_set_unlogin_.begin(), this->session_set_unlogin_.end(), [](player_session* session) {
            (void)session->execute_timeout_check();
        });

        this->post_session_timeout_check();
    }
}

void player_session_manager::cancel_session_timeout_check(void)
{
    this->session_check_timer_.cancel();
}



