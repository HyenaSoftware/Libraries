#include "stdafx.h"
#include "rv"

using namespace reactive_framework8;
using namespace std;

#define R(a) { return a; }

template<class T> weak_ptr<T> as_weak(shared_ptr<T> ptr_)
{
	return std::weak_ptr<T> { std::move(ptr_) };
}




rv_context::rv_context()
	: _thread_pool { 10 }
{

}

rv_debugger& rv_context::debugger()
{
	return _debugger;
}

utility::thread_pool& rv_context::thread_pool()
{
	return _thread_pool;
}




void function_one()
{

	auto add = [](int a_, int b_) R(a_ + b_);

	for(int i=0; i<100; ++i)
	{

		rv_context rc;

		rv<int> a, b;

		rc.debugger().set_name(a, "a");
		rc.debugger().set_name(b, "b");

		rv<int> c = rc.map(add, a, b );
		rc.debugger().set_name(c, "c");

		c.subscribe([](int v_)
		{
			int ans = v_;
		});

		a << 1;
		b << 2;
	}

	return;
}
