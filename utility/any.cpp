#include "stdafx.h"
#include "any.hpp"

using namespace utility;
using namespace std;




bad_any_cast::bad_any_cast(type_index current_type_, type_index expected_type_)
	: runtime_error { _format_message(current_type_, expected_type_) }
{
}

string bad_any_cast::_format_message(type_index current_type_, type_index expected_type_)
{
	stringstream msg;

	msg << "Bad any cast: tried to get '" << expected_type_.name() << "' typed value, but the value of any is '"
		<< current_type_.name() << "'.";

	return msg.str();
}



any& any::operator=(any other_)
{
	swap(other_);

	return *this;
}

void any::swap(any& other_)
{
	_ptr_holder.swap(other_._ptr_holder);
}

const type_info& any::type() const
{
	return _ptr_holder->type();
}

bool any::is_const() const
{
	return _ptr_holder->is_const();
}

bool any::empty() const
{
	return _ptr_holder == nullptr;
}

void* any::raw_ptr()
{
	return _ptr_holder->raw_ptr();
}

const void* any::raw_ptr() const
{
	return _ptr_holder->raw_ptr();
}



any_ptr::any_ptr(any& a_)
	: _ptr { a_.raw_ptr() }
	, _ti { a_.type() }
{
}

any_ptr::any_ptr(void* ptr_, std::type_index ti_)
	: _ptr { ptr_ }
	, _ti { move(ti_) }
{
}

any_ptr::~any_ptr()
{
	_ptr = nullptr;
	_ti = typeid(nullptr_t);
}

any_ptr& any_ptr::operator=(any_ptr other_)
{
	swap(other_);

	return *this;
}

void any_ptr::swap(any_ptr& other_)
{
	std::swap(_ptr, other_._ptr);
	std::swap(_ti, other_._ti);
}

bool any_ptr::is_null() const
{
	return _ptr == nullptr;
}

void* any_ptr::raw_pointer() const
{
	return _ptr;
}

type_index any_ptr::type() const
{
	return _ti;
}
