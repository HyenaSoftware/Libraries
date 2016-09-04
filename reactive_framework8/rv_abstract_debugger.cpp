#include "stdafx.h"
#include "rv_abstract_debugger.hpp"


using namespace reactive_framework8;
using namespace std;



void rv_abstract_debugger::set_name(void* ptr_urv_, string rv_name_)
{
	_urv_to_name.insert({ ptr_urv_, move(rv_name_) });
}

string rv_abstract_debugger::name_of(void* ptr_, type_index ti_)
{
	auto it = _urv_to_name.find(ptr_);

	if (it == _urv_to_name.end())
	{
		tie(it, std::ignore) = _urv_to_name.insert({ ptr_, generate_name(ptr_, ti_) });
	}

	return it->second;
}

string rv_abstract_debugger::generate_name(void* ptr_, type_index ti_)
{
	stringstream sb;
	sb << hex << "[0x" << ptr_ << "][" << ti_.name() << "]";

	return sb.str();
}
