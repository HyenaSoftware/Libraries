#include "stdafx.h"
#include "object_ref.hpp"

using namespace utility;




object_ref::object_ref(std::shared_ptr<void> ptr_, std::type_index real_type_, std::type_index ostensible_type_)
	: _ptr { ptr_ }
	, _real_type { real_type_ }
	, _ostensible_type { ostensible_type_ }
{
}


object_ref& object_ref::operator=(object_ref other_)
{
	swap(other_);

	return *this;
}


void object_ref::swap(object_ref& other_)
{
	_ptr.swap(other_._ptr);
	std::swap(_real_type, other_._real_type);
	std::swap(_ostensible_type, other_._ostensible_type);
}
