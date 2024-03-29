#pragma once

#include <chrono>

#include "oqpi/empty_layer.hpp"
#include "oqpi/synchronization/sync_common.hpp"


namespace oqpi { namespace itfc {

    //----------------------------------------------------------------------------------------------
    template
    <
        // Platform specific implementation for events
          typename _Impl
        // Augmentation layer, needs to be templated and inherit from the implementation
        , template<typename> typename _Layer
    >
    class event
        : public std::conditional<is_empty_layer<_Layer>::value, _Impl, _Layer<_Impl>>::type
    {
    public:
        //------------------------------------------------------------------------------------------
        // Whether the event has augmented layer(s) or not
        static constexpr auto is_lean   = is_empty_layer<_Layer>::value;
        // The platform specific implementation
        using event_impl                = _Impl;
        // The actual base type taking into account the presence or absence of augmentation layer(s)
        using base_type                 = std::conditional_t<is_lean, event_impl, _Layer<event_impl>>;
        // The actual type
        using self_type                 = event<event_impl, _Layer>;
        // Native handle
        using native_handle_type        = typename event_impl::native_handle_type;

    public:
        //------------------------------------------------------------------------------------------
        event()
            : base_type()
        {}

        //------------------------------------------------------------------------------------------
        explicit event(const std::string &name)
            : base_type(name, sync_object_creation_options::open_or_create)
        {}

        //------------------------------------------------------------------------------------------
        explicit event(const std::string &name, sync_object_creation_options creationOption)
            : base_type(name, creationOption)
        {}

    public:
        //------------------------------------------------------------------------------------------
        // Movable
        event(self_type &&rhs)
            : base_type(std::move(*(base_type *)&rhs))
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
        event(const self_type &)                    = delete;
        self_type& operator =(const self_type &)    = delete;

    public:
        //------------------------------------------------------------------------------------------
        // User interface
        native_handle_type  getNativeHandle()   const   { return base_type::getNativeHandle();  }
        inline bool         isValid()           const   { return base_type::isValid();          }
        inline void         notify()                    { return base_type::notify();           }
        inline bool         wait()                      { return base_type::wait();             }
        inline void         reset()                     { return base_type::reset();            }
        inline void         close()                     { return base_type::close();            }

        //------------------------------------------------------------------------------------------
        template<typename _Rep, typename _Period>
        inline bool waitFor(const std::chrono::duration<_Rep, _Period>& relTime)
        {
            return base_type::waitFor(relTime);
        }
        template<typename _Clock, typename _Duration>
        inline bool waitUntil(const std::chrono::time_point<_Clock, _Duration>& absTime)
        {
            return waitFor(absTime - _Clock::now());
        }
    };
    //----------------------------------------------------------------------------------------------

} /*itfc*/ } /*oqpi*/


namespace oqpi {
        
    //----------------------------------------------------------------------------------------------
    struct event_auto_reset_policy_impl
    {
        static bool is_manual_reset_enabled()
        {
            return false;
        }

        // CLANG/LLVM will evaluate a static_assert(false) even if the function is never called
        // So we must trick it to not evaluate it by templating the assert condition
        template<bool _UndefinedFunction = false>
        void reset()
        {
            static_assert(_UndefinedFunction, "reset() is not implemented for this event "
                "configuration, use oqpi::event instead of oqpi::auto_reset_event "
                "if you want to manually reset the event");
        }
    };
    //----------------------------------------------------------------------------------------------

} /*oqpi*/
