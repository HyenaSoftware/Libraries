#pragma once
#include <exception>
#include <functional>
#include <string>
#include <vector>



namespace utility
{
	std::vector<std::exception_ptr> unwrap_exceptions(std::exception& e_);

	void rethrow_and_handle(std::exception_ptr e_, std::function<void(std::exception&)>);
}
