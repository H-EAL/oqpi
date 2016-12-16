#pragma once

#include "oqpi/empty_layer.hpp"
#include "oqpi/threads/thread_attributes.hpp"


namespace oqpi { namespace interface {

	// Configurable thread class with a C++11 like interface
	template<typename _Impl, template<typename> typename _Layer = empty_layer>
	class thread
		: public std::conditional<is_empty_layer_v<_Layer>, _Impl, _Layer<_Impl>>::type
	{
	public:
		static constexpr IsAugmented	= !is_empty_layer_v<_Layer>;
		using SelfType					= std::conditional<IsAugmented, _Impl, _Layer<_Impl>>::type;
		using ThreadImpl				= _Impl;

	public:
		thread() noexcept = default;

		template<typename _Func, typename... _Args>
		thread(const thread_attributes &attributes, _Func &&func, _Args &&...args)
		{
			launch(attributes, std::bind(func, std::forward<_Args...>(args)))
		}

		template<typename _Func, typename... _Args>
		thread(const char *pName, _Func &&func, _Args &&...args)
			: thread(thread_attributes{pName}, std::forward<_Func>(func), std::forward<_Args...>(args))
		{}

	private:
		template<typename _Target>
		void launch(const thread_attributes &attributes, _Target &&target)
		{

			auto upTarget = std::make_unique<_Target>(std::forward<_Target>(target));

			ThreadImpl::create(attributes, )
		}
	};

} /*interface*/ } /*oqpi*/