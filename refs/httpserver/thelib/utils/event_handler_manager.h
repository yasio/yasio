#ifndef _EVENT_HANDLER_MANAGER_H_
#define _EVENT_HANDLER_MANAGER_H_
#include <map>
#include <set>
#include "container_helper.h"

namespace thelib {

namespace privacy{ // reusable event handler manager templates
template<typename event_trigger, typename event_id, typename event_arg>
class event_handler_manager
{
public:
    class event_handler
    {
    public:
        virtual void invoke(event_trigger* trigger, const event_arg&) = 0;
    };

public:
    ~event_handler_manager(void)
    {
        container_helper::clear_cc(this->ec, [](std::set<event_handler*>& eh_set) {
            container_helper::clear_c(eh_set);
        });
    }

    event_handler* register_handler(event_id eid, event_handler* eh)
    {
        static const std::set<event_handler*> nil_eh_set;
        auto target_ehpr = this->ec.find(eid);
        if(this->ec.end() == target_ehpr)
        {
            target_ehpr = this->ec.insert(std::make_pair(eid, nil_eh_set)).first;
        }
        target_ehpr->second.insert(eh);
        return eh;
    }

    void unregister_handler(event_id eid, event_handler* specific_handler)
    {
        auto deleter = this->ec.find(eid);
        if(deleter != this->ec.end())
        {
            auto ehiter = deleter->second.find(specific_handler);
            if(ehiter != deleter->second.end())
            {
                delete specific_handler;
                deleter->second.erase(specific_handler);
            }
        }
    }

    void unregister_handler(event_id eid)
    {
        auto deleter = this->ec.find(eid);
        if(deleter != this->ec.end())
        {
            container_helper::clear_c(deleter->second);
        }
    }

    void invoke(event_id eid, event_trigger* trigger, const event_arg& arg)
    {
		auto invoke_pr = this->ec.find(eid);
        if(invoke_pr != this->ec.end())
        {
            std::for_each(invoke_pr->second.begin(), invoke_pr->second.end(), [&](event_handler* eh) {
                eh->invoke(trigger, arg);
            });
        }
    }

private:

    template<typename _Fty, int _ArgFlags = 2>
    class event_handler_impl : public event_handler
    {
    public:
        event_handler_impl(const _Fty& _Func) : callee(_Func)
        {
        }
        virtual void invoke(event_trigger* trigger, const event_arg& arg)
        {
            this->callee(trigger, arg);
        }

    private:
        _Fty callee;
    };

    template<typename _Fty>
    class event_handler_impl<_Fty, 1> : public event_handler
    {
    public:
        event_handler_impl(const _Fty& _Func) : callee(_Func)
        {
        }
        virtual void invoke(event_trigger*, const event_arg& arg)
        {
            this->callee(arg);
        }

    private:
        _Fty callee;
    };

    template<typename _Fty>
    class event_handler_impl<_Fty, 0> : public event_handler
    {
    public:
        event_handler_impl(const _Fty& _Func) : callee(_Func)
        {
        }

        virtual void invoke(event_trigger*, const event_arg&)
        {
            this->callee();
        }

    private:
        _Fty callee;
    };

    template<typename _Fty>
    struct event_handler_factory
    {
        static event_handler* create(const _Fty& _Func)
        {
            return create_event_handler(_Func, &_Fty::operator());
        }

    private:
        static event_handler* create_event_handler(const _Fty& _Func, void (_Fty::*)(event_trigger*,const event_arg&))
        {
            return (new event_handler_impl<_Fty>(_Func));
        }

        static event_handler* create_event_handler(const _Fty& _Func, void (_Fty::*)(event_trigger*,const event_arg&) const)
        {
            return (new event_handler_impl<_Fty, 2>(_Func));
        }

        static event_handler* create_event_handler(const _Fty& _Func, void (_Fty::*)(const event_arg&))
        {
            return (new event_handler_impl<_Fty, 1>(fo));
        }

        static event_handler* create_event_handler(const _Fty& _Func, void (_Fty::*)(const event_arg&) const)
        {
            return (new event_handler_impl<_Fty, 1>(_Func));
        }

        static event_handler* create_event_handler(const _Fty& _Func, void (_Fty::*)(void))
        {
            return (new event_handler_impl<_Fty, 0>(_Func));
        }

        static event_handler* create_event_handler(const _Fty& _Func, void (_Fty::*)(void) const)
        {
            return (new event_handler_impl<_Fty, 0>(_Func));
        }
    };

    template<typename _Return>
    class event_handler_factory<_Return (*)(event_trigger*,const event_arg&)>
    {
    public:
        static event_handler* create(_Return (*pf)(event_trigger*,const event_arg&))
        {
            return (new event_handler_impl<_Return (*)(event_trigger*,const event_arg&), 2>(pf));
        }
    };

    template<typename _Return>
    class event_handler_factory<_Return (*)(const event_arg&)>
    {
    public:
        static event_handler* create(_Return (*pf)(const event_arg&))
        {
            return (new event_handler_impl<_Return (*)(const event_arg&), 1>(pf));
        }
    };

    template<typename _Return>
    class event_handler_factory<_Return (*)(void)>
    {
    public:
        static event_handler* create(_Return (*pf)(void))
        {
            return (new event_handler_impl<_Return (*)(void), 0>(pf));
        }
    };

public:
    /// new a event handler
    template<typename _Fty>
    static event_handler* new_event_handler(const _Fty& handler_function)
    {
        return event_handler_factory<_Fty>::create(handler_function);
    }

private:
    std::map<event_id, std::set<event_handler*> > ec;
};

}; // namespace: thelib::privacy
}; // namespace: thelib

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

