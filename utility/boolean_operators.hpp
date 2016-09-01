#pragma once
#include "stdafx.h"

namespace utility
{

	template<class T> T aggregate(std::function<T(T, T)> f_, T&& a_, T&& b_)
	{
		return f_(std::forward<T>(a_), std::forward<T>(b_));
	}

	template<class T, class... Ts> T aggregate(std::function<T(T, T)> f_, T&& a_, T&& b_, Ts&&... ts_)
	{
		return aggregate(f_, aggregate(f_, std::forward<T>(a_), std::forward<T>(b_)), std::forward<Ts>(ts_)...);
	}

	template<class... Ts> bool and(Ts&&... ts_)
	{
		return aggregate<bool>([](bool a_, bool b_)
		{
			return a_ && b_;
		}, true, std::forward<Ts>(ts_)...);
	}

	template<class... Ts> bool or (Ts&&... ts_)
	{
		return aggregate<bool>([](bool a_, bool b_)
		{
			return a_ || b_;
		}, false, std::forward<Ts>(ts_)...);
	}
}