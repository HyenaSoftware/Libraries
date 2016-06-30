#pragma once
#include <typeindex>
//#include <utility\ptr.hpp>
#include <utility\rtti_aux.hpp>
#include <memory>

namespace utility
{

	//
	// shallow copy version of boost::any (that does deep copy)
	//
	//
	//
	//
	/*
		shared_ptr<T> p = ...;

		any a = p;	// any<shared_ptr<T>>

		any_cast<T>(a);	// bad_any_cast

		any b = map(a, ???);

		any_cast<T>(b);

		//
		int n = 8;

		any a = n;

		any_ptr r1 = a;		// any_ptr(any&)
		any_ptr r2 = &n;	// any_ptr(T*)

		any b = r1;		// b[8]; r1[ptr] -> a[8]
		any c = r2;		// c[8]; r2[ptr] -> n[8]


		any
			T

		// since it uses shallow copy, the constness of the holder does matter
	*/

	class bad_any_cast : public std::runtime_error
	{
	public:
		bad_any_cast(std::type_index, std::type_index);

	private:
		static std::string _format_message(std::type_index, std::type_index);
	};


	class any
	{
		template<class T> friend T  any_cast(const any&);
		template<class T> friend T* any_cast(any*);

		template<class T> using remove_cv_ref = std::remove_cv_t<std::remove_reference_t<T>>;

	public:
		any() = default;

		any(const any& other_) = default;
		any(any&& other_) = default;

		template<class T> any(T value_)
			: _ptr_holder{ std::make_shared<typed_holder<remove_cv_ref<T>>>( std::move(value_) ) }
		{
		}

		any& operator=(any other_);

		friend bool operator==(const any& a_, const any& b_)
		{
			return a_.raw_ptr() == b_.raw_ptr();
		}

		friend bool operator!=(const any& a_, const any& b_)
		{
			return a_.raw_ptr() != b_.raw_ptr();
		}

		void swap(any& other_);

		const std::type_info& type() const;

		bool is_const() const;

		bool empty() const;

		void* raw_ptr();

		const void* raw_ptr() const;

	protected:
		struct holder
		{
			virtual const std::type_info& type() const = 0;

			virtual bool is_const() const = 0;

			virtual void* raw_ptr() = 0;

			virtual const void* raw_ptr() const = 0;
		};

		template<class T> struct typed_holder : public holder
		{
			typedef std::remove_reference_t<T> value_type;
			typedef T contained_type;

			typed_holder(T&& value_) : value{ std::forward<T>(value_) }
			{
			}

			const std::type_info& type() const override
			{
				return typeid(value_type);
			}

			bool is_const() const override
			{
				return std::is_const<T>::value;
			}

			void* raw_ptr()
			{
				return &value;
			}

			const void* raw_ptr() const
			{
				return &value;
			}

			T value;
		};

		std::shared_ptr<holder> _ptr_holder;
	};

	/*
		ptr-like
		it's a "typed-void*" pointer

		planned:
			non-null	- how any_ptr can be default constructible?
	*/
	class any_ptr
	{
	public:
		any_ptr() = default;

		any_ptr(any& a_);

		template<class T> any_ptr(T* ptr_)
			: _ptr { reinterpret_cast<void*>(ptr_) }
			, _ti { typeid(T) }
		{}

		any_ptr(void* ptr_, std::type_index ti_);

		~any_ptr();

		any_ptr& operator=(any_ptr other);

		template<class T> any_ptr& operator=(T* ptr_)
		{
			any_ptr copy { ptr_ };

			swap(copy);

			return *this;
		}

		void swap(any_ptr& other_);

		bool is_null() const;

		void* raw_pointer() const;

		std::type_index type() const;

		template<class T> operator T () const
		{
			static_assert(std::is_pointer<T>::value, "Type T must be a pointer.");

			typedef std::remove_pointer_t<T> direct_t;

			std::type_index target_type { typeid(direct_t) };

			if(target_type != _ti)
			{
				throw std::runtime_error { "bad cast" };
			}

			return reinterpret_cast<T>(_ptr);
		}

	private:
		void* _ptr;
		std::type_index _ti = typeid(nullptr_t);
	};


	/*
		any_cast	any				is valid?	to create any holder sig.
		T			T				YES			= T
		T			const T			NO			n/a
		[int*]		const [int*]	NO			n/a

		T&			T				YES			= remove_reference_t<T>
		T&			const T			NO			n/a

		const T&	T				YES			= remove_const_t<remove_reference_t<T>>
		const T&	const T			YES			= remove_reference_t<T>
	*/
	template<class T> T* any_cast(any* any_)
	{
		bool type_is_matching = type_of<T>() == any_->type();

		if(!type_is_matching)
		{
			return nullptr;
		}

		typedef std::remove_cv_t<T> held_t;

		return &static_cast<any::typed_holder<held_t>*>(any_->_ptr_holder.get())->value;
	}

	template<class T> inline const T* any_cast(const any* any_)
	{
		return any_cast<T>(const_cast<any*>(any_));
	}

	template<class T> T& any_cast(any& any_)
	{
		typedef std::remove_reference_t<T> value_type;

		value_type* ptr = any_cast<value_type>(&any_);

		if(!ptr)
		{
			throw bad_any_cast { any_.type(), typeid(value_type) };
		}

		typedef std::add_lvalue_reference_t<value_type> ref_type;

		return static_cast<ref_type>(*ptr);
	}

	template<class T> T any_cast(any&& any_)
	{
		typedef std::remove_reference_t<T> value_type;

		value_type* ptr = any_cast<value_type>(&any_);

		if (!ptr)
		{
			throw bad_any_cast{ any_.type(), typeid(value_type) };
		}

		return std::move(*ptr);
	}

	template<class T> inline T& any_cast(const any& any_)
	{
		typedef std::remove_reference_t<T> value_t;

		return any_cast<const value_t&>(const_cast<any&>(any_));
	}


}

namespace std
{
	//
	//
	//
	template<> struct hash<utility::any> : unary_function<utility::any, size_t>
	{
		size_t operator()(const utility::any& ptr_) const
		{
			size_t hc = reinterpret_cast<size_t>(ptr_.raw_ptr());

			return hc;
		}
	};



	//
	//
	//
	template<> struct hash<utility::any_ptr> : unary_function<utility::any_ptr, size_t>
	{
		size_t operator()(const utility::any_ptr& ptr_) const
		{
			size_t hc = reinterpret_cast<size_t>(ptr_.raw_pointer());

			return hc;
		}
	};
}