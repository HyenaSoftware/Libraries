#include "stdafx.h"
#include "exception_utils.hpp"

using namespace utility;
using namespace std;



vector<exception_ptr> _unwrap_exceptions_impl(exception& e_, vector<exception_ptr> vec_)
{
	try
	{
		vec_.push_back(make_exception_ptr(e_));

		rethrow_if_nested(e_);
	}
	catch (std::exception& nested_e_)
	{
		return _unwrap_exceptions_impl(nested_e_, std::move(vec_));
	}

	return std::move(vec_);
}


vector<exception_ptr> utility::unwrap_exceptions(exception& e_)
{
	return _unwrap_exceptions_impl(e_, {});
}

void utility::rethrow_and_handle(exception_ptr e_, function<void(exception&)> handler_)
{
	try
	{
		rethrow_exception(e_);
	}
	catch (exception& e_)
	{
		handler_(e_);
	}
}
