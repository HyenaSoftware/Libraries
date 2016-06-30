#include "stdafx.h"
#include "thread_pool"


using namespace std;
using namespace utility;


struct utility::thread_pool_impl
{
	thread_pool_impl(size_t count_of_threads_)
	{
		_threads.reserve(count_of_threads_);

		for(size_t i=0; i<count_of_threads_; ++i)
		{
			_threads.emplace_back(std::bind(&thread_pool_impl::_worker, this));
		}
	}

	~thread_pool_impl()
	{
		{
			unique_lock<mutex> ql{ _mtx_queue_change };
			_terminating = true;
		}

		_cv_queue_change.notify_all();

		for (auto& th : _threads)
		{
			th.join();
		}
	}

	size_t size() const
	{
		return _threads.size();
	}

	void submit(function<void()> task_)
	{
		{
			unique_lock<mutex> l { _mtx_queue_change };

			_task_queue.push(move(task_));
		}

		_cv_queue_change.notify_one();
	}

private:
	void _worker()
	{
		for(;;)
		{
			function<void()> task;
			{
				unique_lock<mutex> ql { _mtx_queue_change };

				_cv_queue_change.wait(ql, [this]
				{
					return !_task_queue.empty() || _terminating;
				});

				if(_terminating)
				{
					return;
				}

				_task_queue.front().swap(task);
				_task_queue.pop();
			}

			task();
		}
	}

	//thread_pool_impl& operator=(thread_pool_impl&&) = delete;
	//thread_pool_impl& operator=(const thread_pool_impl&) = delete;

	vector<thread> _threads;
	queue<function<void()>> _task_queue;

	bool _terminating = false;
	mutex _mtx_queue_change;
	condition_variable _cv_queue_change;

};


thread_pool::thread_pool(int count_of_threads_) : _pimpl { make_unique<thread_pool_impl>(count_of_threads_) }
{
}

thread_pool::thread_pool(const thread_pool& other_)
	: thread_pool { other_._pimpl->size() }
{
}

thread_pool::thread_pool(thread_pool&& other_)
{
	swap(other_);
}

void thread_pool::swap(thread_pool& other_)
{
	_pimpl.swap(other_._pimpl);
}

thread_pool& thread_pool::operator=(const thread_pool& other_)
{
	// call copy-ctor directly
	*this = thread_pool { other_ };

	return *this;
}

thread_pool& thread_pool::operator=(thread_pool&& other_)
{
	swap(other_);

	return *this;
}

thread_pool::~thread_pool() = default;

void thread_pool::submit(function<void()> task_)
{
	_pimpl->submit(task_);
}


