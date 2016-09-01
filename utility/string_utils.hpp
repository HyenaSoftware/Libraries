#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <tuple>

namespace utility
{
	inline std::ostream& operator << (std::ostream& os_, std::type_index ti_)
	{
		return os_ << ti_.name();
	}

	template<class T> std::ostream& operator << (std::ostream& os_, const std::vector<T>& vec_)
	{
		os_ << "[";
		for(auto& e : vec_)
		{
			os_ << e << ", ";
		}

		if (!vec_.empty())
		{
			os_.seekp(-2, ios_base::end);
		}

		return os_ << "]";
	}

	template<class... Ts> std::ostream& operator << (std::ostream& os_, const std::tuple<Ts...>& tpl_)
	{
		os_ << "[";

		utility::for_each(tpl_, [&os_](auto& e_)
		{
			os_ << e_ << ", ";
		});

		if (sizeof...(Ts) > 0)
		{
			os_.seekp(-2, ios_base::end);
		}

		return os_ << "]";
	}
}
