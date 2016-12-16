#pragma once

#include <memory>
#include <type_traits>
#include "oqpi/empty_layer.hpp"
#include "oqpi/threads/thread_attributes.hpp"


namespace oqpi { namespace interface {

    // Configurable thread class with a C++11 like interface
    template<typename _Impl, template<typename> typename _Layer = empty_layer>
    class thread
        : public std::conditional<is_empty_layer<_Layer>::value, _Impl, _Layer<_Impl>>::type
    {
    public:
        static constexpr auto IsLean	= is_empty_layer<_Layer>::value;
        using ThreadImpl				= _Impl;
        using SelfType					= typename std::conditional<IsLean, _Impl, _Layer<_Impl>>::type;

    public:
        thread() noexcept
        {}

        // Constructors with argument binding
        template<typename _Func, typename... _Args>
        explicit thread(const thread_attributes &attributes, _Func &&func, _Args &&...args)
        {
            launch(attributes, std::bind(std::forward<_Func>(func), std::forward<_Args>(args)...));
        }

        template<typename _Func, typename... _Args>
        explicit thread(const char *pName, _Func &&func, _Args &&...args)
            : thread(thread_attributes{pName}, std::forward<_Func>(func), std::forward<_Args>(args)...)
        {}

        template<typename _Func, typename... _Args>
        explicit thread(_Func &&func, _Args &&...args)
            : thread("<unnamed thread>", std::forward<_Func>(func), std::forward<_Args>(args)...)
        {}

    public:
        using native_handle = typename ThreadImpl::native_handle;
        using id            = typename ThreadImpl::id;

    public:
        // Public interface
        bool            joinable()          const { return ThreadImpl::joinable();        }
        void            join()                    { return ThreadImpl::join();            }
        void            detach()                  { return ThreadImpl::detach();          }
                                                  
        native_handle   getNativeHandle()   const { return ThreadImpl::getNativeHandle(); }
        id              getId()             const { return ThreadImpl::getId();           }
        thread_priority getPriority()       const { return ThreadImpl::getPriority();     }
        core_affinity   getCoreAffinity()   const { return ThreadImpl::getCoreAffinity(); }
        uint32_t        getStackSize()      const { return ThreadImpl::getStackSize();    }

    private:
        template<typename _Target>
        void launch(const thread_attributes &attributes, _Target &&target)
        {
            auto upTarget = std::make_unique<_Target>(std::forward<_Target>(target));

            if (ThreadImpl::create(attributes, nullptr))
            {
                upTarget.release();
            }
        }
    };

} /*interface*/ } /*oqpi*/