#pragma once

#include <typeindex>

namespace utility
{
	template<class T> std::type_index type_of()
	{
		return typeid(T);
	}
}
