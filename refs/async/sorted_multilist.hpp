
        template<typename _Ty, typename _Pr = std::less<_Ty>>
        class sorted_multilist
        {
            struct _List_entry
            {
                _Ty      value;
                _List_entry* next;
#if 1
                static void * operator new(size_t /*size*/)
                {
                    return get_pool().get();
                }

                static void operator delete(void *p)
                {
                    get_pool().release(p);
                }

                static purelib::gc::object_pool<_List_entry>& get_pool()
                {
                    static purelib::gc::object_pool<_List_entry> s_pool;
                    return s_pool;
                }
#endif
            };

        public:
            sorted_multilist() : _Mysize(0), _Myhead(nullptr)
            {
            }

            ~sorted_multilist()
            {
                if (!empty()) {
                    clear();
                }
            }

            bool empty() const
            {
                return _Mysize == 0;
            }

            size_t size() const
            {
                return _Mysize;
            }

            void clear()
            {
                for (auto ptr = &_Myhead; *ptr;)
                {
                    auto entry = *ptr;
                    *ptr = entry->next;
                    delete (entry);
                }
                _Myhead = nullptr;
                _Mysize = 0;
            }

            template<typename _Fty>
            size_t remove_if(const _Fty& cond)
            {
                size_t count = 0;
                for (auto ptr = &_Myhead; *ptr;)
                {
                    auto entry = *ptr;
                    if (cond(entry->value)) {
                        *ptr = entry->next;
                        delete (entry);
                        ++count;
                    }
                    else {
                        ptr = &entry->next;
                    }
                }
                _Mysize -= count;
                return count;
            }

            bool remove(const _Ty& value)
            {
                for (auto ptr = &_Myhead; *ptr;)
                {
                    auto entry = *ptr;
                    if (value == entry->value) {
                        *ptr = entry->next;
                        delete (entry);
                        --_Mysize;
                        return true;
                    }
                    else {
                        ptr = &entry->next;
                    }
                }
                return false;
            }

            template<typename _Fty>
            void foreach(const _Fty& callback)
            {
                for (auto ptr = _Myhead; ptr != nullptr; ptr = ptr->next)
                {
                    callback(ptr->value);
                }
            }

            template<typename _Fty>
            void foreach(const _Fty& callback) const
            {
                for (auto ptr = _Myhead; ptr != nullptr; ptr = ptr->next) 
                {
                    callback(ptr->value);
                }
            }

            const _Ty& front() const
            {
                assert(!empty());
                return _Myhead->value;
            }

            _Ty& front()
            {
                assert(!empty());
                return _Myhead->value;
            }

            void pop_front()
            {
                assert(!empty());
                auto deleting = _Myhead;
                _Myhead = _Myhead->next;
                delete deleting;
                --_Mysize;
            }

            void insert(const _Ty& value)
            {
                const _Pr comparator;
                if (_Myhead != nullptr) {
                    auto ptr = _Myhead;
                    while (comparator(ptr->value, value) && ptr->next != nullptr)
                    {
                        ptr = ptr->next;
                    }

                    // if (value == ptr->value)
                    // { // duplicate
                    //    return 0;
                    // }

                    auto newNode = _Buynode(value);

                    if (ptr->next != nullptr)
                    {
                        newNode->next = ptr->next;
                        ptr->next = newNode;
                    }
                    else
                    {
                        ptr->next = newNode;
                    }

                    if (comparator(value, ptr->value))
                    {
                        std::swap(ptr->value, newNode->value);
                    }
                }
                else {
                    _Myhead = _Buynode(value);
                }
                ++_Mysize;
            }
        private:
            _List_entry* _Buynode(const _Ty& value)
            {
                auto _Newnode = new _List_entry;
                _Newnode->value = value;
                _Newnode->next = nullptr;
                return _Newnode;
            }
        private:
            _List_entry* _Myhead;
            size_t       _Mysize;
        };
