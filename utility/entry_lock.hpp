#pragma once


namespace utility
{

	class flag_mutex
	{
	public:
		inline operator bool() const { return _allow; }

		inline void set() { _allow = false; }
		inline void clear() { _allow = true; }

	private:
		bool _allow = true;
	};

	class flag_lock
	{
	public:
		inline flag_lock(flag_mutex& mtx_) : _mtx{ mtx_ }
		{
			_mtx.set();
		}

		inline ~flag_lock()
		{
			_mtx.clear();
		}

	private:
		flag_mutex& _mtx;
	};


}
