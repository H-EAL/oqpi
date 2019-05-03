#pragma once

#include <chrono>

#include "oqpi/empty_layer.hpp"
#include "oqpi/synchronization/sync_common.hpp"


namespace oqpi { namespace itfc {

    //----------------------------------------------------------------------------------------------
    template
    <
        // Platform specific implementation for semaphores
          typename _Impl
        // Augmentation layer, needs to be templated and inherit from the implementation
        , template<typename> typename _Layer
    >
    class mutex
        : public std::conditional<is_empty_layer<_Layer>::value, _Impl, _Layer<_Impl>>::type
    {
    public:
        //------------------------------------------------------------------------------------------
        // Whether the object has augmented layer(s) or not
        static constexpr auto is_lean   = is_empty_layer<_Layer>::value;
        // The platform specific implementation
        using mutex_impl                = _Impl;
        // The actual base type taking into account the presence or absence of augmentation layer(s)
        using base_type                 = typename std::conditional_t<is_lean, mutex_impl, _Layer<mutex_impl>>;
        // The actual type
        using self_type                 = mutex<mutex_impl, _Layer>;
        // Native handle
        using native_handle_type        = typename mutex_impl::native_handle_type;

    public:
        //------------------------------------------------------------------------------------------
        mutex()
            : base_type()
        {}

        //------------------------------------------------------------------------------------------
        explicit mutex(const std::string &name)
            : base_type(name, sync_object_creation_options::open_or_create)
        {}

        //------------------------------------------------------------------------------------------
        mutex(const std::string &name, sync_object_creation_options creationOption)
            : base_type(name, creationOption)
        {}

    public:
        //------------------------------------------------------------------------------------------
        // Movable
        mutex(self_type &&rhs)
            : base_type(std::move(rhs))
        {}

        //------------------------------------------------------------------------------------------
        inline self_type& operator =(self_type &&rhs)
        {
            if (this != &rhs)
            {
                base_type::operator =(std::move(rhs));
            }
            return (*this);
        }

        //------------------------------------------------------------------------------------------
        // Not copyable
        mutex(const self_type &)                    = delete;
        self_type& operator =(const self_type &)    = delete;

    public:
        //------------------------------------------------------------------------------------------
        // User interface
        native_handle_type  getNativeHandle()       const   { return base_type::getNativeHandle();  }
        bool                isValid()               const   { return base_type::isValid();          }   
        bool                tryLock()                       { return base_type::tryLock();          }

        //------------------------------------------------------------------------------------------
        // STL BasicLockable requirements:  https://en.cppreference.com/w/cpp/named_req/BasicLockable
        bool                lock()                          { return base_type::lock();             }
        void                unlock()                        { return base_type::unlock();           }

        //------------------------------------------------------------------------------------------
        // STL Lockable requirements:       https://en.cppreference.com/w/cpp/named_req/Lockable
        bool                try_lock()                      { return tryLock();                     }

        //------------------------------------------------------------------------------------------
        // STL TimedLockable requirements:  https://en.cppreference.com/w/cpp/named_req/TimedLockable
        template<typename _Rep, typename _Period>
        bool try_lock_for(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            return tryLockFor(relTime);
        }
        //------------------------------------------------------------------------------------------
        template<typename _Clock, typename _Duration>
        bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& absTime)
        {
            return tryLockUntil(absTime);
        }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        inline bool tryLockFor(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            return base_type::tryLockFor(relTime);
        }
        //------------------------------------------------------------------------------------------
        template<typename _Clock, typename _Duration>
        inline bool tryLockUntil(const std::chrono::time_point<_Clock, _Duration>& absTime)
        {
            return tryLockFor(absTime - _Clock::now());
        }
    };

} /*itfc*/ } /*oqpi*/
