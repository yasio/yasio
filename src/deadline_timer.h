#ifndef _XXSOCKET_DEADLINE_TIMER_H_
#define _XXSOCKET_DEADLINE_TIMER_H_
#include <chrono>
#include <functional>

namespace purelib
{
    namespace inet {
        class deadline_timer {
        public:
            ~deadline_timer();

            void expires_from_now(const std::chrono::microseconds& duration, bool loop = false)
            {
                this->duration_ = duration;
                this->loop_ = loop;

                expire_time_ = std::chrono::steady_clock::now() + this->duration_;
            }

            void expires_from_now()
            {
                expire_time_ = std::chrono::steady_clock::now() + this->duration_;
            }

            void async_wait(const std::function<void(bool cancelled)>& callback);

            bool expired() const
            {
                return wait_duration().count() <= 0;
            }

            std::chrono::microseconds wait_duration() const
            {
                return std::chrono::duration_cast<std::chrono::microseconds>(expire_time_ - std::chrono::steady_clock::now());
            }

            bool loop_;
            std::chrono::microseconds duration_;
            std::chrono::time_point<std::chrono::steady_clock> expire_time_;
            std::function<void(bool cancelled)> callback_;
        };
    }
}

#endif
