#include "stdafx.h"
#include "rv_debugger.hpp"

using namespace reactive_framework8;
using namespace std;


rv_debugger::~rv_debugger()
{
	int stop = 0;
}

void rv_debugger::notify_value_change(string rv_name_, string value_)
{
	std::lock_guard<std::mutex> l{ _mtx_print };
	cout << "value changeing: " << rv_name_ << " to " << value_ <<endl;
}

void rv_debugger::notify_rv_assigned_to(std::string rv_name_)
{
	std::lock_guard<std::mutex> l{ _mtx_print };
	cout << "new value assigned to the context: " << rv_name_ << endl;
}

void rv_debugger::notify_new_operator(std::string op_name_)
{
	std::lock_guard<std::mutex> l{ _mtx_print };
	std::cout << "new operator: " << op_name_ << endl;
}

void rv_debugger::add_edge_from(void* node_ptr_, std::type_index node_type_, void* operator_ptr_, std::type_index operator_type_)
{
	std::lock_guard<std::mutex> l{ _mtx_print };
	std::cout << "new edge: " << name_of(node_ptr_, node_type_) << " -> " << name_of(operator_ptr_, operator_type_) << endl;
}

void rv_debugger::add_edge_to(void* operator_ptr_, std::type_index operator_type_, void* node_ptr_, std::type_index node_type_)
{
	std::lock_guard<std::mutex> l{ _mtx_print };
	std::cout << "new edge: " << name_of(operator_ptr_, operator_type_) << " -> " << name_of(node_ptr_, node_type_) << endl;
}
