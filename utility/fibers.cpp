#include "stdafx.h"
#include "fibers.hpp"

using namespace utility;
using namespace std;


//
//
//
fiber::fiber(fiber_context& master_fiber_, LPVOID this_fiber_)
	: _master{ master_fiber_ }
	, _this_fiber{ this_fiber_ }
{
}

fiber::fiber(fiber&& other_)
	: _master{ other_._master }
	, _this_fiber{ other_._this_fiber }
{
	other_._this_fiber = nullptr;
}

void fiber::switch_to()
{
	// make sure the current thread is converted to secondary_fiber
	//this_fiber::get_fiber();

	//
	SwitchToFiber(_this_fiber);
}

//
//
//
secondary_fiber::secondary_fiber(fiber_context& master_fiber_) : secondary_fiber{ master_fiber_, [] {} }
{
}

secondary_fiber::secondary_fiber(fiber_context& master_fiber_, std::function<void()> handler_)
	: fiber{ master_fiber_, CreateFiber(0, &secondary_fiber::_dispatcher, this) }
	, _handler{ std::move(handler_) }
{
}

secondary_fiber::secondary_fiber(secondary_fiber&& other_)
	: fiber(other_._master, CreateFiber(0, &secondary_fiber::_dispatcher, this))
	// do not swap the fiber, as it's associated to *this* object
{
	_handler.swap(other_._handler);
}

secondary_fiber::~secondary_fiber()
{
	if (_this_fiber)
	{
		DeleteFiber(_this_fiber);
		_this_fiber = nullptr;
	}
}

void secondary_fiber::reset_handler(std::function<void()> handler_)
{
	_handler.swap(handler_);
}

void __stdcall secondary_fiber::_dispatcher(LPVOID parameter_)
{
	auto ptr_fiber = static_cast<secondary_fiber*>(parameter_);
	ptr_fiber->_dispatcher();
}

void secondary_fiber::_dispatcher()
{
	_handler();

	// merge it back automatically
	_master.switch_to_primary();
}


//
//
//
primary_fiber::primary_fiber(fiber_context& master_fiber_)
	: fiber { master_fiber_, ConvertThreadToFiber(nullptr) }
{
}

primary_fiber::primary_fiber(primary_fiber&& other_) : fiber(std::move(other_))
{
}

primary_fiber::~primary_fiber()
{
	if (_this_fiber)
	{
		auto status = ConvertFiberToThread();
		_this_fiber = nullptr;
	}
}

//
//
//
void fiber_context::switch_to(int i_)
{
	if (i_ == 0)
	{
		switch_to_primary();
	}
	else
	{
		auto it = _slave_fibers.find(i_);
		if (it != _slave_fibers.end())
		{
			it->second.switch_to();
		}
	}
}

void fiber_context::switch_to_primary()
{
	_primary_fiber.switch_to();
}

bool fiber_context::erase(int i_)
{
	if (i_ == 0)
		throw std::out_of_range{ "primary fiber cannot be deleted explicity" };

	auto it = _slave_fibers.find(i_);
	bool has_it = it != _slave_fibers.end();

	if (has_it)
	{
		_slave_fibers.erase(it);
	}

	return has_it;
}

fiber& fiber_context::get_or_create(int i_, std::function<void()> func_)
{
	if (i_ == 0)
		return _primary_fiber;

	return get_or_create(i_, std::move(func_));
}

secondary_fiber& fiber_context::get_or_create_secondary(int i_, std::function<void()> func_)
{
	auto it = _slave_fibers.find(i_);

	if (it == _slave_fibers.end())
	{
		// use emplace, instead of insert to avoid to use the move-ctor
		std::tie(it, std::ignore) = _slave_fibers.emplace(
			std::piecewise_construct,
			std::make_tuple(i_),
			std::make_tuple(std::ref(*this), std::move(func_)));
	}

	return it->second;
}

secondary_fiber& fiber_context::get_or_create_secondary(int i_)
{
	return get_or_create_secondary(i_, [] {});
}

fiber& fiber_context::get_or_create(int i_)
{
	return get_or_create(i_, [] {});
}

