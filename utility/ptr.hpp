#pragma once
#include <utility\event_handler>

namespace utility
{
	//
	//	it does deap copy
	//
	template<class T> class scoped_holder
	{
	public:
		scoped_holder() = default;
		scoped_holder(T value_) : _value { std::move(value_) }
		{
		}

		scoped_holder(const scoped_holder& other_)
			: _value { value_ } // deep copy
			// on_destruction - do NOT copy it
		{
		}

		scoped_holder(scoped_holder&& other_) : _value{std::move(other_._value)}
		{
			on_destruction.swap(other_.on_destruction);
		}

		scoped_holder& operator=(scoped_holder other_)
		{
			swap(other_);

			return *this;
		}

		~scoped_holder()
		{
			on_destruction(this);
		}

		void swap(scoped_holder& other_)
		{
			std::swap(_value, other_._value);

			on_destruction.swap(other_.on_destruction);
		}

		T* operator->()
		{
			return _value;
		}

		const T* operator->() const
		{
			return _value;
		}

		T& operator*()
		{
			return _value;
		}

		const T& operator*() const
		{
			return _value;
		}

		event<scoped_holder*> on_destruction;

	private:
		T _value;
	};

	/*
		potential issue:

		ptr<A> p { A { } };		// target is on the heap
		A& ra = *p;
		ptr<A> q = ra;			// q considers &ra is on the stack

	*/

	template<class T> class ptr
	{
	public:
		ptr()
		{
		}

		ptr(const ptr& other_)
		{
		}

		ptr(ptr&& other_)
		{
		}

		ptr(scoped_holder<T>& value_on_stack_)
		{
		}

		explicit ptr(T value_)
		{
		}

		~ptr()
		{
		}

		ptr& operator=(ptr other)
		{
			return *this;
		}

		T& operator*() const
		{
			/*
			for stack object:
			- raise exception if the content is deleted

			for heap object:
			- keep the pointee alive
			*/
		}

		template<class U> operator ptr<U>() const
		{
		}

		operator bool () const
		{
		}

	private:
		enum E_TARGET_MEMORY_TYPE
		{
			HEAP,
			STACK
		};

		class shared_data
		{
		public:

		private:
			E_TARGET_MEMORY_TYPE _target_memory_type;
			T* _ptr;	// warning: it can point to heap or stack
			size_t _ref_cnt = 0;
		};

		shared_data* _data;
	};
}
