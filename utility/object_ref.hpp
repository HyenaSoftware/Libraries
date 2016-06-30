#pragma once
#include <typeindex>
#include <memory>
#include <stdexcept>
#include "macros"

namespace utility
{
	template<class T> struct is_shared_ptr : std::false_type { };
	template<class T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type { };

	/*
		similar to any+any_ptr

		+ reference counted rsrc managment
		+ interchangeable with shared_ptr<T>
		+ can be
			* initialized by value
			* initialized by reference (==smart pointer)
	*/

	enum FORCE_USE_SHARED_PTR_AS_VALUE_TYPE { FORCE_USE_SHARED_PTR_AS_VALUE };
	enum STORE_SHARED_TARGET_TYPE { STORE_SHARED_TARGET };


	class object_ref
	{
	public:
		object_ref() = default;

		object_ref(const object_ref&) = default;

		object_ref(object_ref&&) = default;

		template<class T> object_ref(std::shared_ptr<T> ptr_, STORE_SHARED_TARGET_TYPE = STORE_SHARED_TARGET)
			: _ptr { ptr_ }
			, _real_type { typeid(*ptr_) }
			, _ostensible_type{ typeid(T) }
		{
		}

		template<class T,
			/* avoid to generate another copy or move ctor */ class = std::enable_if_t<!std::is_same<std::remove_reference_t<T>, object_ref>::value>
		>
		explicit object_ref(T&& val_, FORCE_USE_SHARED_PTR_AS_VALUE_TYPE = FORCE_USE_SHARED_PTR_AS_VALUE)
				//: object_ref { std::make_shared<std::remove_reference_t<T>>(std::forward<T>(val_)) }
				: object_ref { _make_shared_ptr(std::forward<T>(val_)) }
		{
		}


		explicit object_ref(std::shared_ptr<void>, std::type_index real_type_, std::type_index ostensible_type_);
		/*
			check the types as well, because in some rare situations:

			struct s1 { };
			struct s2 { s1 n; };
			
			s1 a;
			s2 b;

			assert(&a == &b);

			.. addresses of different object can be equals
		*/
		friend bool operator==(const object_ref& a_, const object_ref& b_)
		{
			return (a_._ptr.get() == b_._ptr.get())
				&& (a_._ostensible_type == b_._ostensible_type)
				&& (a_._real_type == b_._real_type);
		}

		friend bool operator!=(const object_ref& a_, const object_ref& b_)
		{
			return a_._ptr.get() != b_._ptr.get()
				|| (a_._ostensible_type != b_._ostensible_type)
				|| (a_._real_type != b_._real_type);
		}

		bool operator==(nullptr_t) const
		{
			return !static_cast<bool>(_ptr);
		}

		bool operator!=(nullptr_t) const
		{
			return static_cast<bool>(_ptr);
		}

		object_ref& operator=(object_ref other_);

		void swap(object_ref& other_);

		operator bool () const
		{
			return _ptr != nullptr;
		}

		std::type_index type() const
		{
			return _ostensible_type;
		}

		std::type_index real_type() const
		{
			return _real_type;
		}

		std::shared_ptr<void> pointer()
		{
			return _ptr;
		}

		const std::shared_ptr<void> pointer() const
		{
			return _ptr;
		}

		template<class T> explicit operator T& () const
		{
			return as<T>();
		}

		//
		//
		//
		template<class T> T& as() const
		{
			typedef std::remove_reference_t<T> value_t;

			auto ptr = as_pointer<value_t>();

			if(!ptr)
			{
				std::stringstream sb;
				sb << "Type " << _ostensible_type.name() << " (" << _real_type.name() << ") can not be converted to " << typeid(value_t).name() << ".";

				throw std::runtime_error { sb.str() };
			}

			return *ptr;
		}

		//
		//	it can be null
		//
		template<class T> std::shared_ptr<T> as_pointer() const
		{
			static_assert(!std::is_reference<T>::value, "");

			std::type_index expected_type{ typeid(T) };

			if (expected_type == _ostensible_type)
			{
				return std::static_pointer_cast<T>(_ptr);
			}

			return { };
		}

	private:
		std::shared_ptr<void> _ptr;
		std::type_index _real_type { typeid(nullptr_t) }, _ostensible_type { typeid(nullptr_t) };

		template<class T> static std::shared_ptr<T> _make_shared_ptr(T& val_)
		{
			return std::shared_ptr<T>
			{
				&val_,
				[](T*){}
			};
		}

		template<class T> static std::shared_ptr<T> _make_shared_ptr(T&& val_)
		{
			return std::make_shared<std::remove_reference_t<T>>(std::move(val_));
		}
	};
}


namespace std
{
	//
	//
	//
	template<> struct hash<utility::object_ref> : unary_function<utility::object_ref, size_t>
	{
		size_t operator()(const utility::object_ref& ptr_) const
		{
			hash<std::shared_ptr<void>> shared_ptr_hasher;

			size_t hc = shared_ptr_hasher(ptr_.pointer());

			return hc;
		}
	};

}
