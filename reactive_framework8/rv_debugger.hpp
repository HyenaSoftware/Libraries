#pragma once
#include "stdafx.h"
#include "rv_abstract_debugger.hpp"

namespace reactive_framework8
{
	class rv_debugger : public rv_abstract_debugger
	{
	public:
		~rv_debugger();

		void notify(E_DEBUGGER_EVENT event_, std::string rv_name_, std::string value_);

		void add_edge(void* ptr_, std::type_index, rv_abstract_operator& op_);
		void add_edge(rv_abstract_operator& op_, void* ptr_, std::type_index);
		

	private:
		std::mutex _mtx_print;
	};
}
