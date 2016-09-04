#include "stdafx.h"
#include "rv"
#include "rv_debugger.hpp"
#include "rv_remote_debugger.hpp"

using namespace reactive_framework8;
using namespace std;


namespace reactive_framework8
{
	namespace graph
	{
		class inotifiable;
	}
}


#define R(a) { return a; }

template<class T> weak_ptr<T> as_weak(shared_ptr<T> ptr_)
{
	return std::weak_ptr<T> { std::move(ptr_) };
}

class rv_context::rv_context_impl
{
public:
	std::unordered_set<std::shared_ptr<graph::inotifiable>> intermediate_nodes;

	virtual rv_abstract_debugger& debugger() = 0;

	virtual void reset_debugger(std::unique_ptr<rv_abstract_debugger> debugger_) = 0;

	virtual void submit(std::function<void()> task_) = 0;
	
};

class rv_context_impl_multithreaded : public rv_context::rv_context_impl
{
public:
	rv_context_impl_multithreaded()
		: _thread_pool{ 10 }
		, _debugger{ make_unique<rv_debugger>() }
	{
	}

	rv_abstract_debugger& debugger()
	{
		return *_debugger;
	}

	void reset_debugger(std::unique_ptr<rv_abstract_debugger> debugger_)
	{
		_debugger.swap(debugger_);
	}

	void submit(std::function<void()> task_)
	{
		_thread_pool.submit(move(task_));
	}

private:
	//	1st - allocation 
	//	2nd - deallocation - make sure no other thread keep locked its mutex
	std::unique_ptr<rv_abstract_debugger> _debugger;

	//	2nd - allocation
	//	1st - deallocation - terminate running threads
	utility::thread_pool _thread_pool;
};

class rv_context_impl_unittest : public rv_context::rv_context_impl
{
public:
	rv_abstract_debugger& debugger()
	{
		return _debugger;
	}

	void reset_debugger(std::unique_ptr<rv_abstract_debugger>)
	{
	}

	void submit(std::function<void()> task_)
	{
		task_();
	}
private:
	rv_debugger _debugger;
};





rv_context::rv_context() : _impl { make_unique<rv_context_impl_unittest>() }
{
}

rv_context::~rv_context()
{
}

rv_abstract_debugger& rv_context::debugger()
{
	return _impl->debugger();
}

void rv_context::reset_debugger(std::unique_ptr<rv_abstract_debugger> debugger_)
{
	//_impl->reset_debugger(move(debugger_));
}

void rv_context::submit(std::function<void()> task_)
{
	_impl->submit(move(task_));
}


void rv_context::_hold_this_internal_nodes(std::shared_ptr<graph::inotifiable> ptr_)
{
	_impl->intermediate_nodes.insert(std::move(ptr_));
}


void function_one()
{

	auto add = [](int a_, int b_) R(a_ + b_);

	for(int i=0; i<1; ++i)
	{

		rv_context rc;
		rc.reset_debugger(make_unique<rv_remote_debugger>());

		rv<int> a, b;

		rc.debugger().set_name(a, "a");
		rc.debugger().set_name(b, "b");

		rv<int> c = rc.map(add, a, b );
		rc.debugger().set_name(c, "c");

		c.subscribe([](int v_)
		{
			int ans = v_;
		});

		cout << "a=1" << endl;
		//system("pause");
		
		a << 1;

		cout << "b=2" << endl;
		//system("pause");

		b << 2;

		cout << "a=10" << endl;
		//system("pause");

		a << 10;

		system("pause");
	}

	return;
}
