#include "stdafx.h"
#include "rv_debugger.hpp"



using namespace reactive_framework8;
using namespace std;


const std::unordered_map<E_DEBUGGER_EVENT, std::string> reactive_framework8::DEBUGGER_EVENT_TO_STRING
{
	{ E_DEBUGGER_EVENT::NODE_VALUE_CHANGED, "node value changed" }
};

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
	cout << event_ << " " << rv_name_ << " " << value_ <<endl;
}
