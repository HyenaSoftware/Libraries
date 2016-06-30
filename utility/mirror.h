#pragma once
#include "stdafx.h"

//#include <boost\any.hpp>

#include "any.hpp"
#include "hash.hpp"
#include "tuple_utils.hpp"
#include "star_map.hpp"
#include "object_ref.hpp"
#include "memory_utils.hpp"

namespace utility
{
	class class_not_found_exception : public std::runtime_error
	{
	public:
		class_not_found_exception(std::string msg_);
	}; 


	class member_not_found_exception : public std::runtime_error
	{
	public:
		member_not_found_exception(std::type_index type_, std::string member_name_);

	private:
		static std::string _format_message(std::type_index type_, std::string member_name_);
	};

	//
	//struct undefined_t { };

	//
	template<class T> struct type_tag { typedef T type; };


	// forward declaration
	class base_runtime_database;

	//
	class mirrored_object
	{
	public:
		mirrored_object(std::string name_, std::type_index type_);

		std::string name() const;

		std::type_index type() const;

		virtual ~mirrored_object() = default;

		virtual void set_runtime(const base_runtime_database&);

	private:
		std::string _name;
		std::type_index _type;

	protected:
		const base_runtime_database* _ptr_runtime { nullptr };
	};


	namespace detail
	{
		class class_like_t : public mirrored_object
		{
		public:
			using mirrored_object::mirrored_object;

			virtual const std::unordered_map<std::type_index, std::shared_ptr<class_like_t>>& base_classes() const = 0;
			virtual class_like_t& set_base_class(std::shared_ptr<class_like_t>) = 0;			
		};

		std::vector<any_ptr> to_any_ptr_vec(std::vector<object_ref>);
	}


	//
	class member_t : public mirrored_object
	{
	public:
		bool is_static() const;

	protected:
		member_t(std::string name_, std::type_index, bool is_static_);

	private:
		bool _is_static;
	};


	class method_like_t : public member_t
	{
	public:
		std::vector<std::type_index> signature() const;
		std::type_index return_type() const;

	protected:
		class method_holder
		{
		public:
			virtual std::type_index expected_object_type() const = 0;
			virtual object_ref apply(std::vector<object_ref> args_) const = 0;
			virtual ~method_holder() = default;
		};

		method_like_t(std::string name_, std::type_index, bool is_static_, std::type_index return_type_, std::vector<std::type_index> signature_, std::shared_ptr<method_holder> method_);

		std::shared_ptr<method_holder> _method;

	private:
		std::vector<std::type_index> _signature;
		std::type_index _return_type;
	};

	//
	class constructor_t : public method_like_t
	{
	public:
		template<class T, class... As> constructor_t(type_tag<T>, type_tag<As> ...)
			: method_like_t { "#ctor", typeid(T(As...)), false, typeid(T), { typeid(As)... }, std::make_shared<ctor_holder<T, As...>>() }
		{
			static_assert(sizeof...(As) > 0 || is_default_constructible<T>::value, "If ctor signature is empty, type T must be default constructible");
		}

		object_ref create(std::vector<object_ref> args_ = {}) const;

	private:
		//
		template<class T, class... As> class ctor_holder : public method_holder
		{
		public:
			object_ref apply(std::vector<object_ref> args_) const override
			{
				//return std::make_shared<T>(_method(detail::to_any_ptr_vec(args_)));
				return std::make_shared<T>(_method(std::move(args_)));
			}

			std::type_index expected_object_type() const override
			{
				return typeid(T);
			}

		private:
			any_arged_function<T, As...> _method { &_ctor_proxy };

			static T _ctor_proxy(As... args_)
			{
				return T { std::move(args_) ... };
			}
		};
	};


	template<class T, class... As> constructor_t make_constructor()
	{
		return constructor_t
		{
			type_tag<T> { }, type_tag<As> { } ...
		};
	}


	//
	class method_t : public method_like_t
	{
		struct const_t : std::true_type { };
		struct non_const_t : std::false_type { };
	public:
		template<class R, class T, class... As> method_t(std::string name_, R(T::*ptr_method_)(As...))
			: method_like_t { move(name_), typeid(R(T::*)(As...)), false, typeid(R), { typeid(As)... }, std::make_shared<typed_method_holder<non_const_t, R, T, As...>>(ptr_method_) }
		{
		}

		template<class R, class T, class... As> method_t(std::string name_, R(T::*ptr_method_)(As...) const)
			: method_like_t{ move(name_), typeid(R(T::*)(As...)), false, typeid(R),{ typeid(As)... }, std::make_shared<typed_method_holder<const_t, R, T, As...>>(ptr_method_) }
		{
		}

		template<class R, class... As> method_t(std::string name_, R(*ptr_static_method_)(As...))
			: method_like_t{ std::move(name_), typeid(R(*)(As...)), true, typeid(R), { typeid(As)... }, std::make_shared<typed_static_method_holder<R, As...>>(ptr_static_method_) }
		{
		}

		//
		object_ref invoke(object_ref obj_, std::vector<object_ref> args_) const;

		//object_ref invoke(object_ref obj_, std::vector<any_ptr> args_) const;

		//
		template<class R, class... As> R invoke_static(As... args_) const
		{
			auto val = invoke_static({ object_ref { args_ } ... });

			return val.as<R>();
		}

		object_ref invoke_static(std::vector<object_ref> args_ = { }) const;

	private:

		template<class IS_CONST, class R, class T, class... As> class typed_method_holder : public method_holder
		{
		public:
			enum { arity = sizeof...(As) };

			typed_method_holder(R(T::*ptr_method_)(As...))
				: _method { _make_function(ptr_method_, std::make_index_sequence<arity> { }) }
			{
			}

			typed_method_holder(R(T::*ptr_method_)(As...) const)
				: _method{ _make_function(ptr_method_, std::make_index_sequence<arity> { }) }
			{
			}

			object_ref apply(std::vector<object_ref> args_) const override
			{
				std::shared_ptr<T> obj = args_.front().as_pointer<T>();
				args_.front() = object_ref { obj };

				return _method_proxy<R>(std::move(args_));
				//return _method_proxy<R>(detail::to_any_ptr_vec(args_));
			}

			std::type_index expected_object_type() const override
			{
				return typeid(T);
			}

		private:
			typedef std::conditional_t<IS_CONST::value, const T, T> type_t;
			any_arged_function<R, type_t&, As...> _method;

			template<size_t... Is>
			static std::function<R(T&, As...)> _make_function(R(T::*ptr_method_)(As...), std::index_sequence<Is...>)
			{
				// make sure these are treated as placeholders
				static_assert(std::is_placeholder<std::_Ph<2>>::value, "It must be a placeholder");
				return std::bind<R>(ptr_method_, placeholders::_1, std::_Ph<Is + 2> { } ...);
			}

			template<size_t... Is>
			static std::function<R(const T&, As...)> _make_function(R(T::*ptr_method_)(As...) const, std::index_sequence<Is...>)
			{
				// make sure these are treated as placeholders
				static_assert(std::is_placeholder<std::_Ph<2>>::value, "It must be a placeholder");
				return std::bind<R>(ptr_method_, placeholders::_1, std::_Ph<Is + 2> { } ...);
			}

			template<class X> std::enable_if_t<		std::is_same<void, X>::value, object_ref> _method_proxy(std::vector<object_ref> args_) const
			{
				_method(std::move(args_));

				return { };
			}

			template<class X> std::enable_if_t< NOT std::is_same<void, X>::value, object_ref> _method_proxy(std::vector<object_ref> args_) const
			{
				return object_ref { _method(std::move(args_)) };
			}
		};

		template<class R, class... As> class typed_static_method_holder : public method_holder
		{
		public:
			enum { arity = sizeof...(As) };

			typed_static_method_holder(R(*ptr_static_method_)(As...))
				: _method{ ptr_static_method_ }
			{
			}

			object_ref apply(std::vector<object_ref> args_) const override
			{
				return std::make_shared<R>(_method(std::move(args_)));
				//return std::make_shared<R>(_method(detail::to_any_ptr_vec(args_)));
			}

			std::type_index expected_object_type() const override
			{
				return typeid(void);
			}

		private:
			any_arged_function<R, As...> _method;
		};
		
		template<class... As> class typed_static_method_holder<void, As...> : public method_holder
		{
		public:
			enum { arity = sizeof...(As) };

			typed_static_method_holder(void(*ptr_static_method_)(As...))
				: _method { ptr_static_method_ }
			{
			}

			object_ref apply(std::vector<object_ref> args_) const override
			{
				_method(std::move(args_));
				//_method(detail::to_any_ptr_vec(args_));

				return { };
			}

			std::type_index expected_object_type() const override
			{
				return typeid(void);
			}

		private:
			any_arged_function<void, As...> _method;
		};
	};


	//
	class field_t : public member_t
	{
	public:
		template<class T, class U> field_t(std::string name_, T U::*ptr_) : member_t { std::move(name_), typeid(T U::*), false }
		{
			_ptr_field = std::make_shared<typed_field_holder<T, U>>(ptr_);
		}

		template<class T> field_t(std::string name_, T *ptr_) : member_t{ std::move(name_), typeid(T*), true }
		{
			_ptr_field = std::make_shared<typed_static_field_holder<T>>(ptr_);
		}

		//
		//	getters
		//	Q:	is there any use case to pass object by value ?
		//
		object_ref value(object_ref obj_) const;

		object_ref value() const;


		//
		//	setters
		//	Q:	is there any use case to pass object by value ?
		//	A:	yes, move semantics
		//
		void set_value(object_ref obj_, object_ref value_) const;

		// for static fields
		void set_value(object_ref value_) const;


		std::type_index held_type() const;

	private:
		struct abstract_field_holder
		{
			virtual std::type_index held_type() const = 0;
		};

		struct field_holder : abstract_field_holder
		{
			virtual object_ref value(object_ref object_) const = 0;
			virtual void set_value(object_ref object_, object_ref value_) const = 0;

		};

		struct static_field_holder : abstract_field_holder
		{
			virtual object_ref value() const = 0;
			virtual void set_value(object_ref value_) const = 0;
		};

		template<class T, class U> class typed_field_holder : public field_holder
		{
		public:
			typed_field_holder(T U::*ptr_) : _ptr { ptr_ } { }

			/*
				what it should do ?
				a) return with a reference to that field
					object_ref { shared_ptr<T> -> &field }
			
				b) return with the value of that field
					object_ref { shared_ptr<T> -> &T }
			*/
			object_ref value(object_ref object_) const
			{
				auto ref = _field_ref_from(object_);
				return ref;
			}

			void set_value(object_ref object_, object_ref value_) const
			{
				//auto ref_obj = _field_ref_from(object_);

#pragma message(HERE"move() or swap() should be used here to avoid unnecessary copying of objects")

				T val { value_ };

				U& ref_obj{ object_.operator U& () };

				std::swap(ref_obj.*_ptr, val);
			}

			std::type_index held_type() const override
			{
				return utility::type_of<T>();
			}

		private:
			T U::*_ptr;

			object_ref _field_ref_from(object_ref object_) const
			{
				const std::type_index THIS_TYPE{ object_.type() };

				if (THIS_TYPE == type_of<U>())
				{
					U& ref_obj { object_.operator U& () };
					T* ptr_field = &(ref_obj.*_ptr);

					return std::shared_ptr<T> { object_.pointer(), ptr_field };
				}

				// raise exception
				stringstream msg;
				msg << "Type of any's content must be <Type> or <Type>*. Type '" << object_.type().name() << " is not allowed.";

				throw std::runtime_error { msg.str() };
			}
		};

		template<class T> class typed_static_field_holder : public static_field_holder
		{
		public:
			typed_static_field_holder(T *ptr_) : _ptr { ptr_ } { }

			object_ref value() const override
			{
				return std::shared_ptr<T> { detail::eternal_shared_ptr, _ptr };
			}

			void set_value(object_ref value_) const override
			{
				#pragma message(HERE"move() or swap() should be used here to avoid unnecessary copying of objects")

				T val { value_ };
				std::swap(*_ptr, val);
			}

			std::type_index held_type() const override
			{
				return utility::type_of<T>();
			}

		private:
			T *_ptr;
		};

		std::shared_ptr<abstract_field_holder> _ptr_field;
	};


	//
	class class_converter_t
	{
	public:
		template<class B, class D> class_converter_t(type_tag<B>, type_tag<D>)
			: _base_type { typeid(B) }
			, _derived_type { typeid(D) }
		{
			_cast_from[_derived_type] = [](object_ref obj_) -> object_ref
			{
				return _cast<B>(obj_.as_pointer<D>());
			};

			_cast_from[_base_type] = [](object_ref obj_) -> object_ref
			{
				return _cast<D>(obj_.as_pointer<B>());
			};
		}

		std::type_index base_type() const;
		std::type_index derived_type() const;

		object_ref convert(object_ref) const;

		friend bool operator==(const class_converter_t&, const class_converter_t&);

		size_t hash_code() const;

		std::function<object_ref(object_ref)> upcast_functor() const;
		std::function<object_ref(object_ref)> downcast_functor() const;

	private:
		std::type_index _base_type, _derived_type;
		std::unordered_map<std::type_index, std::function<object_ref(object_ref)>> _cast_from;

		template<class B, class A> static std::shared_ptr<B> _cast(std::shared_ptr<A> ptr_, std::enable_if_t< std::is_polymorphic<A>::value>* = nullptr)
		{
			return std::dynamic_pointer_cast<B>(std::move(ptr_));
		}

		template<class B, class A> static std::shared_ptr<B> _cast(std::shared_ptr<A> ptr_, std::enable_if_t<!std::is_polymorphic<A>::value>* = nullptr)
		{
			return std::static_pointer_cast<B>(std::move(ptr_));
		}
	};

	class base_runtime_database;
	
	namespace detail
	{
		class ctor_key_t
		{
		public:
			virtual ~ctor_key_t() = default;

			friend std::ostream& operator<<(std::ostream&, const ctor_key_t&);

		protected:
			virtual std::ostream& operator>>(std::ostream&) const = 0;
		};

		class method_key_t
		{
		public:
			virtual ~method_key_t() = default;

			friend std::ostream& operator<<(std::ostream&, const method_key_t&);

		protected:
			virtual std::ostream& operator>>(std::ostream&) const = 0;
		};
		
		class field_key_t
		{
		public:
			virtual ~field_key_t() = default;

			friend std::ostream& operator<<(std::ostream&, const field_key_t&);

		protected:
			virtual std::ostream& operator>>(std::ostream&) const = 0;
		};

		class class_imp
		{
		public:
			//
			virtual constructor_t& ctor(ctor_key_t&&) = 0;
			virtual void set_ctor(constructor_t ctor_) = 0;
			virtual std::vector<constructor_t> ctors() const = 0;

			//
			virtual method_t& method(method_key_t&&) = 0;
			virtual void set_method(method_t) = 0;
			virtual std::vector<method_t> methods() const = 0;

			// 
			virtual field_t& field(std::string) = 0;
			virtual void set_field(field_t) = 0;
			virtual std::vector<field_t> fields() const = 0;

			//
			virtual void set_member(method_t) = 0;
			virtual void set_member(constructor_t) = 0;
			virtual void set_member(field_t) = 0;

			virtual void set_owner(base_runtime_database*) = 0;
			virtual std::vector<std::type_index> base_classes() const = 0;
		};

		//


		class cpp_ctor_key_t : public ctor_key_t
		{
		public:
			cpp_ctor_key_t(std::vector<std::type_index> vec_tis_);

			template<class... Ts> cpp_ctor_key_t() : cpp_ctor_key_t { { type_of<Ts>()... } }
			{
			}

			const std::vector<std::type_index>& value() const;

			friend bool operator==(const cpp_ctor_key_t& a_, const cpp_ctor_key_t& b_);

		private:
			std::vector<std::type_index> _vec_tis;

			std::ostream& operator>>(std::ostream&) const override;
		};

		class cpp_method_key_t : public method_key_t
		{
		public:
			template<class... Ts> cpp_method_key_t(std::string name_, type_tag<Ts>...)
				: cpp_method_key_t { std::move(name_), { type_of<Ts>()... } }
			{
			}

			cpp_method_key_t(std::string name_, std::vector<std::type_index> vec_tis_);

			const std::tuple<std::string, std::vector<std::type_index>>& value();

			friend bool operator==(const cpp_method_key_t& a_, const cpp_method_key_t& b_);

		private:
			std::ostream& operator>>(std::ostream&) const override;
			std::string _name;
			std::vector<std::type_index> _vec_tis;
		};

		class cpp_field_key_t : public field_key_t
		{
		public:
			cpp_field_key_t(std::string);

			const std::string value() const;

			friend bool operator==(const cpp_field_key_t& a_, const cpp_field_key_t& b_);

		private:
			std::string _name;

			std::ostream& operator>>(std::ostream&) const override;
		};
	}
}


namespace std
{
	template<> struct hash<utility::detail::cpp_ctor_key_t> : unary_function<utility::detail::cpp_ctor_key_t, size_t>
	{
		size_t operator()(const utility::detail::cpp_ctor_key_t& key_) const
		{
			return { };
		}
	};

	template<> struct hash<utility::detail::cpp_field_key_t> : unary_function<utility::detail::cpp_field_key_t, size_t>
	{
		size_t operator()(const utility::detail::cpp_field_key_t& key_) const
		{
			return{};
		}
	};

	template<> struct hash<utility::detail::cpp_method_key_t> : unary_function<utility::detail::cpp_method_key_t, size_t>
	{
		size_t operator()(const utility::detail::cpp_method_key_t& key_) const
		{
			return{};
		}
	};

	template<> struct hash<utility::class_converter_t> : unary_function<utility::class_converter_t, size_t>
	{
		size_t operator()(const utility::class_converter_t& key_) const
		{
			return key_.hash_code();
		}
	};
}

namespace utility
{

	//
	//	const reference semantics, use immutable objects
	//
	class class_t : public mirrored_object
	{
	public:
		template<class T> class_t(std::string name_, type_tag<T>)
			: class_t { std::move(name_), type_of<T>() }
		{
			_poly_type_extractor = [](object_ref obj_)
			{
				return type_index { typeid( obj_.as<T>() ) };
			};
		}

		class_t(std::string, std::type_index);

		virtual const constructor_t& ctor(detail::ctor_key_t&&) const = 0;
		virtual std::vector<constructor_t> ctors() const = 0;

		virtual const method_t& method(detail::method_key_t&&) const = 0;
		virtual std::vector<method_t> methods() const = 0;

		virtual const field_t& field(detail::field_key_t&&) const = 0;
		virtual std::vector<field_t> fields() const = 0;

		std::type_index get_polymorphic_type(object_ref) const;

		//
		//	make it pure virtual again, as a class implementataion must take care of the registration of its components
		//
		void set_runtime(const base_runtime_database&) override final;

	private:

		virtual void _set_runtime() = 0;

		std::function<std::type_index(object_ref)> _poly_type_extractor;
	};

	//
	class cpp_class_t : public class_t
	{
	public:
		typedef std::string												field_key_t;
		typedef std::tuple<std::string, std::vector<std::type_index>>	method_key_t;
		typedef std::vector<std::type_index>							ctor_key_t;

		//
		template<class... Ms> cpp_class_t(std::string name_, std::type_index ti_, Ms... ms_)
			: class_t { std::move(name_), std::move(ti_) }
		{
			make_tuple(set_member(ms_) ...);
		}


		//
		const constructor_t& ctor(detail::ctor_key_t&&) const override;
		std::vector<constructor_t> ctors() const override;

		//
		const method_t& method(detail::method_key_t&&) const override;
		std::vector<method_t> methods() const override;


		// 
		const field_t& field(detail::field_key_t&&) const override;
		std::vector<field_t> fields() const override;

		//class_t& set_base_class(std::shared_ptr<detail::class_like_t>) override;
		//const std::unordered_map<std::type_index, std::shared_ptr<detail::class_like_t>>& base_classes() const override;

	private:
		std::unordered_map<detail::cpp_method_key_t,	method_t>		_methods;
		std::unordered_map<detail::cpp_field_key_t,		field_t>		_fields;
		std::unordered_map<detail::cpp_ctor_key_t,		constructor_t>	_ctors;

		//
		typedef int nothing_t;
		
		//
		nothing_t set_member(method_t);
		nothing_t set_member(constructor_t);
		nothing_t set_member(field_t);
		nothing_t set_member(class_converter_t);

		void _set_runtime() override;
	};
	

	//
	class enum_const_t : public member_t
	{
	public:
		template<class T> enum_const_t(std::string name_, T value_)
			: member_t { std::move(name_), type_of<T>(), true }
			, _integral_value{ static_cast<int>(value_) }
			, _value { std::move(value_) }
		{
		}

		int integral_value() const;

		object_ref value() const;

	private:
		int _integral_value;
		object_ref _value;
	};

	//
	class enum_t : public mirrored_object
	{
	public:
		typedef std::unordered_map<int, enum_const_t>::iterator iterator;
		typedef std::unordered_map<int, enum_const_t>::const_iterator const_iterator;

		enum_t(std::string name_, std::type_index type_, std::initializer_list<enum_const_t>);

		std::vector<enum_const_t> values() const;

		const enum_const_t& at(int integral_value_) const;

		const_iterator find(int) const;

		iterator begin();
		iterator end();

		const_iterator begin() const;
		const_iterator end() const;

	private:
		std::unordered_map<int, enum_const_t> _values;
	};


	namespace detail
	{
		//
		class base_runtime_database_impl;
	}

	//
	class base_runtime_database
	{
	public:

		base_runtime_database();
		base_runtime_database(const base_runtime_database&) = delete;
		base_runtime_database(base_runtime_database&&) = delete;

		~base_runtime_database();

		//
		bool has_class(std::string class_name_);
		bool has_class(std::type_index ti_);

		class_t& get_polymorph_class(object_ref obj_) const;
		class_t& get_class(std::string class_name_) const;
		class_t& get_class(std::type_index ti_) const;

		std::vector<class_t*> classes() const;

		//
		base_runtime_database& add(std::shared_ptr<class_t> class_);
		base_runtime_database& add(std::shared_ptr<enum_t> enum_);
		base_runtime_database& add(class_converter_t base_class_);

		enum_t& enums(std::string enum_name_) const;
		enum_t& enums(std::type_index type_) const;
		std::vector<enum_t*> enums() const;

		object_ref convert_to(object_ref obj_, std::type_index target_type_) const;
		std::vector<class_converter_t> bases_of(std::type_index ti_) const;

	private:
		utility::table<std::string, std::type_index, std::shared_ptr<class_t>> _known_classes;
		utility::table<std::string, std::type_index, std::shared_ptr<enum_t>> _known_enums;
		std::unordered_multimap<std::type_index, class_converter_t> _base_classes, _derived_classes;

		//std::unordered_map<std::tuple<std::type_index, std::type_index>, std::function<object_ref(object_ref)>> _type_converters;

		std::unique_ptr<detail::base_runtime_database_impl> _ptr_impl;
	};



	//
	template<class T> class generic_runtime_database
	{
	public:
		typedef T class_t;

		generic_runtime_database() = default;
		generic_runtime_database(generic_runtime_database&& other_)
		{
			_inner_db.swap(other_._inner_db);
		}

		bool has_class(std::string class_name_)
		{
			return _inner_db->has_class(move(class_name_));
		}

		bool has_class(std::type_index ti_)
		{
			return _inner_db->has_class(move(ti_));
		}

		class_t& get_polymorph_class(object_ref obj_) const
		{
			auto& ref = _inner_db->get_polymorph_class(std::move(obj_));

			return static_cast<class_t&>(ref);
		}

		class_t& get_class(std::string class_name_)
		{
			auto& ref = _inner_db->get_class(std::move(class_name_));

			return static_cast<class_t&>(ref);
		}

		class_t& get_class(std::type_index ti_)
		{
			auto& ref = _inner_db->get_class(std::move(ti_));

			return static_cast<class_t&>(ref);
		}

		std::vector<class_t*> classes() const
		{
			std::vector<class_t*> result;

			for(auto e : _inner_db->classes())
			{
				auto ptr = static_cast<class_t*>(e);
				result.push_back(ptr);
			}

			return result;
		}

		generic_runtime_database& add(class_t class_)
		{
			_inner_db->add(std::make_shared<class_t>(std::move(class_)));

			return *this;
		}

		generic_runtime_database& add(enum_t enum_)
		{
			_inner_db->add(std::make_shared<enum_t>(std::move(enum_)));
			
			return *this;
		}

		generic_runtime_database& add(class_converter_t base_)
		{
			_inner_db->add(std::move(base_));

			return *this;
		}

		std::vector<enum_t*> enums()
		{
			return _inner_db->enums();
		}

		std::vector<class_converter_t> bases_of(std::type_index ti_) const
		{
			return _inner_db->bases_of(ti_);
		}

	private:
		std::unique_ptr<base_runtime_database> _inner_db { std::make_unique<base_runtime_database>() };
	};


	typedef generic_runtime_database<cpp_class_t> runtime_database;



	template<class T> class generic_runtime_database_builder
	{
	public:
		typedef T class_t;

		typedef generic_runtime_database_builder<class_t> self_t;
		typedef generic_runtime_database<class_t> runtime_database_t;

		self_t add(class_t class_) &&
		{
			_db.add(std::move(class_));
			//_classes.push_back(std::move(class_));

			return std::move(*this);
		}

		self_t add(enum_t enum_) &&
		{
			_db.add(std::move(enum_));
			//_enums.push_back(std::move(enum_));

			return std::move(*this);
		}

		self_t add(class_converter_t base_classes_) &&
		{
			_db.add(std::move(base_classes_));
			//_base_classes.push_back(std::move(base_classes_));

			return std::move(*this);
		}

		runtime_database_t build() &&
		{
			//runtime_database_t runtime;
			//runtime.add()

			return std::move(_db);
		}

		std::shared_ptr<runtime_database_t> build_ptr() &&
		{
			return std::make_shared<runtime_database_t>(std::move(*this).build());
		}

	private:
		//std::vector<class_converter_t> _base_classes;
		//std::vector<enum_t> _enums;
		//std::vector<class_t> _classes;

		runtime_database_t _db;
	};
}

