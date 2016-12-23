#pragma once

#include <tuple>
#include <memory>
#include <type_traits>

#include "oqpi/empty_layer.hpp"
#include "oqpi/threads/thread_attributes.hpp"


namespace oqpi { namespace interface {

    //----------------------------------------------------------------------------------------------
    // Thread interface, all thread implementation need to comply to this interface
    template
    <
          typename _Impl
        // Platform specific implementation for a thread
        , template<typename> typename _Layer = empty_layer
        // Augmentation layer, needs to be templated and inherit from the implementation
    >
    class thread
        : public std::conditional<is_empty_layer<_Layer>::value, _Impl, _Layer<_Impl>>::type
    {
    public:
        //------------------------------------------------------------------------------------------
        // Whether the thread has augmented layer(s) or not
        static constexpr auto IsLean	= is_empty_layer<_Layer>::value;
        // The platform specific implementation
        using ThreadImpl				= _Impl;
        // The actual type taking into account the presence or absence of augmentation layer(s)
        using SelfType					= typename std::conditional<IsLean, _Impl, _Layer<_Impl>>::type;
        
    public:
        //------------------------------------------------------------------------------------------
        // Returns the number of hardware threads (logical cores)
        static unsigned int hardware_concurrency() { return ThreadImpl::hardware_concurrency(); }

    public:
        //------------------------------------------------------------------------------------------
        // Default constructible, constructs a non-joinable thread
        thread() noexcept = default;

        //------------------------------------------------------------------------------------------
        // Constructor from any callable object, creates a thread and runs the passed function
        // passing it the any arguments it needs. The arguments have to be provided.
        // See thread_attributes for more info on how to configure the thread.
        template<typename _Func, typename... _Args>
        explicit thread(const thread_attributes &attributes, _Func &&func, _Args &&...args)
        {
            launch(attributes, std::tuple<std::decay_t<_Func>, std::decay_t<_Args>...>(std::forward<_Func>(func), std::forward<_Args>(args)...));
        }

        //------------------------------------------------------------------------------------------
        // Creates a thread specifying only the its name. Uses default thread configuration.
        template<typename _Func, typename... _Args>
        explicit thread(const char *name, _Func &&func, _Args &&...args)
            : thread(thread_attributes(name), std::forward<_Func>(func), std::forward<_Args>(args)...)
        {}

        //------------------------------------------------------------------------------------------
        // Kills the thread if it's still joinable on destruction
        ~thread() noexcept
        {
            if (joinable())
            {
                std::terminate();
            }
        }

    public:
        //------------------------------------------------------------------------------------------
        // Movable
        thread(SelfType &&other)
            : ThreadImpl(std::move(other))
        {}
        //------------------------------------------------------------------------------------------
        SelfType& operator =(SelfType &&rhs)
        {
            if (this != &rhs)
            {
                if (joinable())
                {
                    std::terminate();
                }
                ThreadImpl::operator =(std::move(rhs));
            }
            return (*this);
        }

    private:
        //------------------------------------------------------------------------------------------
        // Not copyable
        thread(const SelfType &)                = delete;
        SelfType& operator =(const SelfType &)  = delete;


    public:
        //------------------------------------------------------------------------------------------
        // Forward native type  definitions
        using id                 = typename ThreadImpl::id;
        using native_handle_type = typename ThreadImpl::native_handle_type;

    public:
        //------------------------------------------------------------------------------------------
        // Public interface that needs to be implemented by the thread implementation
        id                  getId()                                     const   { return ThreadImpl::getId();                       }
        native_handle_type  getNativeHandle()                           const   { return ThreadImpl::getNativeHandle();             }

        bool                joinable()                                  const   { return ThreadImpl::joinable();                    }
        void                join()                                              { return ThreadImpl::join();                        }
        void                detach()                                            { return ThreadImpl::detach();                      }
                                        
        void                setCoreAffinityMask(core_affinity affinity)         { return ThreadImpl::setCoreAffinityMask(affinity); }
        core_affinity       getCoreAffinityMask()                       const   { return ThreadImpl::getCoreAffinityMask();         }

        void                setPriority(thread_priority priority)               { return ThreadImpl::setPriority(priority);         }
        thread_priority     getPriority()                               const   { return ThreadImpl::getPriority();                 }

    public:
        //------------------------------------------------------------------------------------------
        // Helpers
        void setCoreAffinity(int32_t coreNumber)
        {
            setCoreAffinityMask(core_affinity(1 << coreNumber));
        }

    private:
        //------------------------------------------------------------------------------------------
        // Intermediary structure used to launch a thread. It is templated by the tuple holding
        // the function as well as the needed parameters to pass to the said function.
        template<typename _Tuple>
        struct launcher
        {
            launcher(const thread_attributes &attributes, _Tuple &&t)
                : tuple_(std::move(t))
                , attributes_(attributes)
            {}

            inline void operator()()
            {
                this_thread::set_name(attributes_.name_);
                run(std::make_integer_sequence<size_t, std::tuple_size<typename _Tuple>::value>());
            }

            template<size_t... _Indices>
            void run(std::integer_sequence<size_t, _Indices...>)
            {
                std::invoke(std::move(std::get<_Indices>(tuple_))...);
            }

            const thread_attributes attributes_;
            _Tuple tuple_;
        };

        //------------------------------------------------------------------------------------------
        // This function is responsible for the actual thread creation and launch.
        // The template tuple packs the function to call alongside its needed parameters.
        template<typename _Tuple>
        void launch(const thread_attributes &attributes, _Tuple &&tuple)
        {
            // Actual type of the launcher
            using launcher_t = launcher<_Tuple>;
            // Create a launcher on the heap to be able to pass it to the thread.
            // The unique_ptr will make sure that the resource will be freed if anything goes wrong.
            auto upLauncher = std::make_unique<launcher_t>(attributes, std::forward<_Tuple>(tuple));

            // Actually creates the thread. Passed a pointer to the launcher as user data.
            // The implementation should take the ownership of the launcher if the thread creation succeeds.
            if (ThreadImpl::template create<launcher_t>(attributes, upLauncher.get()))
            {
                // If the thread creation succeeded, we transfer the ownership of the launcher to it.
                upLauncher.release();
            }
        }
    };

} /*interface*/ } /*oqpi*/