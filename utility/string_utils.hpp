#pragma once
#include <string>
#include <iostream>
#include <vector>

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
}
