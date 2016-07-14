#pragma once
#include "stdafx.h"

namespace utility
{
	class singleton { public: virtual ~singleton() = default; };


	class storage
	{
	public:
		template<class T> T& get_singleton_of()
		{
			static_assert(std::is_base_of<singleton, T>::value, "Type T must be a derived class of utility::singleton");

			unique_ptr<T> (*T_ALLOCATOR)() = &std::make_unique<T>;

			return dynamic_cast<T&>(get_singleton_of(typeid(T), T_ALLOCATOR));
		}
		
		singleton& get_singleton_of(std::type_index, std::function<std::unique_ptr<singleton>()> allocator_);

	private:
		std::unordered_map<std::type_index, std::unique_ptr<singleton>> _instances;
	};

	storage& gl_storage();
}
