#include "stdafx.h"
#include "rv_debugger.hpp"

using namespace reactive_framework8;
using namespace std;

namespace std
{
	ostream& operator<<(ostream& os_, E_DEBUGGER_EVENT event_)
	{
		return os_ << DEBUGGER_EVENT_TO_STRING.at(event_);
	}
}


rv_debugger::~rv_debugger()
{
	int stop = 0;
}

void rv_debugger::notify(E_DEBUGGER_EVENT event_, string rv_name_, string value_)
{
	std::lock_guard<std::mutex> l{ _mtx_print };
	cout << event_ << ": " << rv_name_ << " to " << value_ <<endl;
}


void rv_debugger::add_edge(rv_abstract_operator& op_, void* ptr_, std::type_index ti_)
{
	std::lock_guard<std::mutex> l{ _mtx_print };
	std::cout << "new edge: " << name_of(ptr_, ti_) << " -> " << typeid(op_).name() << endl;
}


void rv_debugger::add_edge(void* ptr_, std::type_index ti_, rv_abstract_operator& op_)
{
	std::lock_guard<std::mutex> l{ _mtx_print };
	std::cout << "new edge: " << ti_.name() << " -> " << name_of(ptr_, ti_) << endl;
}
