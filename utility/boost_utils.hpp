#pragma once
#include <boost\none_t.hpp>

namespace utility
{
	inline bool operator==(const boost::none_t&, const boost::none_t&)
	{
		return true;
	}

	inline bool operator!=(const boost::none_t&, const boost::none_t&)
	{
		return false;
	}
}

