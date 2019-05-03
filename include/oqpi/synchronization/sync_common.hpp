#pragma once

#include <string>


namespace oqpi {

    //----------------------------------------------------------------------------------------------
    enum class sync_object_creation_options
    {
        create_if_nonexistent,
        open_existing,
        open_or_create,
    };


    //----------------------------------------------------------------------------------------------
    template<typename _Impl>
    class local_sync_object
        : protected _Impl
    {
    protected:
        //------------------------------------------------------------------------------------------
        template<typename... _Args>
        local_sync_object(_Args &&...args)
            : _Impl("", sync_object_creation_options::open_or_create, std::forward<_Args>(args)...)
        {}

        //------------------------------------------------------------------------------------------
        template<typename... _Args>
        local_sync_object(const std::string &, sync_object_creation_options, _Args &&...args)
            : local_sync_object(std::forward<_Args>(args)...)
        {}
    };


    //----------------------------------------------------------------------------------------------
    template<typename _Impl>
    class global_sync_object
        : protected _Impl
    {
    protected:
        //------------------------------------------------------------------------------------------
        global_sync_object()
        {
            static_assert(false, "You must name the global synchronization object.");
        }

        //------------------------------------------------------------------------------------------
        template<typename... _Args>
        global_sync_object(const std::string &name, sync_object_creation_options creationOption, _Args &&...args)
            : _Impl(name, creationOption, std::forward<_Args>(args)...)
            , name_(name)
        {}

    public:
        //------------------------------------------------------------------------------------------
        const auto& getName() const { return name_; }

    private:
        //------------------------------------------------------------------------------------------
        std::string name_;
    };

} /*oqpi*/
