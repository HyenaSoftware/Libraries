#pragma once
#include <tuple>

namespace utility
{
	template<class T, T V> struct value_holder { };

	template<class T> struct get_value_of;
	template<class T, T V> struct get_value_of<value_holder<T, V>>
	{
		static const T value = V;
	};
}
