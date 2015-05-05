#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <typeindex>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <Utility\traits.h>
#include <Utility\meta.h>

namespace reactive_framework
{
	template<class T> class accessor
	{
	public:
		typedef T result_type;
		
		virtual result_type operator()() const = 0;
	};

	template<class R> class typed_behaviour : public accessor<R>
	{
	public:

	private:
		bool _recursive_lock;
		
		// e.g.: a -> b -> a [? fallback to 5]
		std::function<result_type()> _recursion_handler;	// it provides a solution to have a fallback value
	};

	template<class T> class value_holder : public typed_behaviour<T>
	{
	public:
		value_holder() = default;
		value_holder(const value_holder&) = default;

		value_holder& operator=(const value_holder&) = default;
		value_holder& operator=(result_type t_)
		{
			std::swap(_value, t_);

			return *this;
		}

		result_type& operator()()
		{
			return _value;
		}

		result_type operator()() const override
		{
			return _value;
		}
	private:
		result_type _value;
	};

	template<class T> class accessor_proxy : public accessor<T>
	{
	public:
		result_type operator()() const override
		{
			auto& src_behaviour = *_src_behaviour;

			if (!_src_behaviour)
			{
				throw std::runtime_error{"source is undefined"};
			}

			return src_behaviour();
		}

		void bind(std::shared_ptr<typed_behaviour<T>> src_behaviour_)
		{
			std::swap(_src_behaviour, src_behaviour_);
		}
	private:
		std::shared_ptr<typed_behaviour<T>> _src_behaviour;
	};

	// forward declaration
	template<class T> class rv_builder;

	template<class T, class I = std::string> class rv
	{
	public:
		typedef T value_type;
		typedef I id_type;

		rv()
		{
			_accessor->bind(_behaviour);
		}

		rv(const rv&) = default;

		rv(rv&& other_)
		{
			std::swap(_behaviour, other_._behaviour);
			std::swap(_accessor, other_._accessor);
			std::swap(_name, other_._name);
		}

		rv(id_type name_) : _name{name_}
		{
			_accessor->bind(_behaviour);
		}

		
		
		// force the compiler to make this class polymorphic
		virtual ~rv() = default;



		// safe convertion

		value_type operator()() const
		{
			return _accessor->operator()();
		}

		operator value_type () const
		{
			return _accessor->operator()();
		}
		

		// it's not safe here

		rv& operator=(const value_type& value_) = delete;

		rv& operator=(const rv& other_)
		{
			_name = other_._name;
			_behaviour = other_._behaviour;
			_accessor = other_._accessor;

			return *this;
		}



		// member functions to handle name/id

		void set_name(std::string name_)
		{
			std::swap(_name, name_);
		}

		std::string& name()
		{
			return _name;
		}

		const std::string& name() const
		{
			return _name;
		}



		// to handle internal accessor

		std::shared_ptr<accessor<T>> internal_accessor() const
		{
			return _accessor;
		}

	protected:
		id_type _name = id_type{};

		std::shared_ptr<typed_behaviour<T>> _behaviour = std::make_shared<value_holder<T>>();
		std::shared_ptr<accessor_proxy<T>> _accessor = std::make_shared<accessor_proxy<T>>();
	};

	/*
		+ it could have source rvs
		- value of it is not modifiable directly, e.g.:
			 a -> b
			 a = 5
			 b = 7 // does it have a valid value? this value may be not consistent with the value of the source
				so it should be updated, but this overwrite the value what it has
	*/
	template<class T, class I = std::string> class rv_readonly : public rv<T, I>
	{
	public:
		void rebind(std::shared_ptr<typed_behaviour<T>> behaviour_)
		{
			std::swap(_behaviour, behaviour_);

			_accessor->bind(_behaviour);
		}
	};

	/*
		+ internal value is modifiable directly
		- it can't be a target for chaining
	*/
	template<class T, class I = std::string> class rv_leaf : public rv<T, I>
	{
	public:
		rv_leaf()
		{
		}

		rv_leaf(value_type value_)
		{
			auto& behaviour = *static_pointer_cast<value_holder<T>>(_behaviour);

			behaviour = (std::move(value_));
		}

		rv_leaf(value_type value_, id_type name_) : rv{std::move(name_)}
		{
			auto& behaviour = *static_pointer_cast<value_holder<T>>(_behaviour);
			behaviour = std::move(value_);
		}

		rv_leaf& operator=(value_type value_)
		{
			auto& behaviour = *static_pointer_cast<value_holder<T>>(_behaviour);
			behaviour = std::move(value_);

			return *this;
		}

		value_type& operator()()
		{
			auto& behaviour = *static_pointer_cast<value_holder<T>>(_behaviour);

			return behaviour();
		}

		value_type& ref()
		{
			auto& behaviour = *static_pointer_cast<value_holder<T>>(_behaviour);

			return behaviour();
		}


	private:
	};




	static_assert(std::is_default_constructible<rv<int>>::value, "rv<?> must be default constructible");

	// rv basic trait
	template<class T> struct is_rv : std::false_type{};
	template<class T, class I> struct is_rv<rv<T, I>> : std::true_type{};


	namespace detail
	{
		template<class R, class... Ts> class join_behaviour : public typed_behaviour<R>
		{
		public:
			join_behaviour(std::tuple<std::shared_ptr<accessor<Ts>>...> rvs_)
				: _rvs{rvs_}
			{
			}

			result_type operator()() const override
			{
				typedef Utility::seq_builder<sizeof...(Ts)>::type seq_type;

				return _apply_impl(seq_type{});
			}

		private:
			std::tuple<std::shared_ptr<accessor<Ts>>...> _rvs;

			template<int... Ns> result_type _apply_impl(Utility::seq<Ns...>) const
			{
				return result_type{std::get<Ns>(_rvs)->operator()()...};
			}
		};

		template<class OUT_TYPE, class IN_TYPE> class map_behaviour : public typed_behaviour<OUT_TYPE>
		{
		public:
			typedef IN_TYPE input_type;

			map_behaviour(std::shared_ptr<accessor<input_type>> src_behaviour_, std::function<result_type(input_type)> func_)
			{
				std::swap(_func, func_);
				std::swap(_src_behaviour, src_behaviour_);
			}

			result_type operator()() const override
			{
				return _func(_src_behaviour->operator()());
			}

		private:
			std::function<result_type(input_type)> _func;
			std::shared_ptr<accessor<input_type>> _src_behaviour;
		};

		template<class T> class merge_behaviour : public typed_behaviour<std::vector<T>>
		{
		public:
			merge_behaviour(std::vector<std::shared_ptr<accessor<T>>> rvs_)
			{
				std::swap(_rvs, rvs_);
			}

			result_type operator()() const override
			{
				std::vector<T> result;
				result.reserve(_rvs.size());

				for (auto& ptr : _rvs)
				{
					result.push_back(ptr->operator()());
				}

				return result;
			}

		private:
			std::vector<std::shared_ptr<accessor<T>>> _rvs;
		};

		//
		// helper for flatmap_behaviour
		//
		template<class U> struct flatten_type_of
		{
			typedef std::vector<U> type;
		};
		template<class U> struct flatten_type_of<std::vector<U>>
		{
			typedef typename flatten_type_of<U>::type type;
		};

		template<class T, class S> class flatmap_behaviour : public typed_behaviour<T>
		{
		public:
			template<class T> struct extract_from_vec
			{
				T _t;

				extract_from_vec(T t_) : _t{std::move(t_)} { }
				void push(std::vector<T>& vec_)
				{
					vec_.push_back(_t);
				}
			};

			template<class T> struct extract_from_vec<std::vector<T>>
			{
				std::vector<T> _t;
				extract_from_vec(std::vector<T> t_) : _t{std::move(t_)} { }
				template<class U> void push(std::vector<U>& vec_) const
				{
					for (auto& v : _t)
					{
						extract_from_vec<T> extractor {v};

						extractor.push(vec_);
					}
				}
			};

			static_assert(Utility::is_vector<T>::value, "Type T must be some kind of vector");
			static_assert(Utility::is_vector<S>::value, "Type S must be some kind of vector");

			//typedef ? source_type;	// it's like vector<vector<int>>

			flatmap_behaviour(std::shared_ptr<accessor<S>> rv_)
			{
				std::swap(_rv, rv_);
			}

			result_type operator()() const override
			{
				result_type result;

				extract_from_vec<S> extractor { (*_rv)() };
				extractor.push(result);

				return result;
			}

		private:
			std::shared_ptr<accessor<S>> _rv;
		};
	}

	//
	// builder for rvs
	//	template parameters:
	//		S: status
	template<class T> class rv_builder
	{
	public:
		rv_builder(std::shared_ptr<accessor<T>> accessor_)
		{
			std::swap(_rv_core, accessor_);
		}

		template<class I> rv_builder(rv<T, I>& rv_)
		{
			_rv_core = rv_.internal_accessor();
		}

		template<class T> rv_builder<T> flatten(rv<T>&)
		{
			return rv_builder<T> {_rv_core};
		}

		template<class I> void to(rv_readonly<T, I>& rv_) const
		{
			auto behaviour = dynamic_pointer_cast<typed_behaviour<T>>(_rv_core);

			if (!behaviour)
			{
				throw runtime_error{ "Bind function is missing. Direct join of rvs are not supported." };
			}

			rv_.rebind(std::move(behaviour));
		}

		template<class I = std::string> rv_readonly<T, I> build(I id_ = I{}) const
		{
			rv_readonly<T, I> result;

			to(result);

			return result;
		}
	
		template<class F> auto map(F f_) const
		{
			typedef Utility::function_traits<F>::result_type result_type;

			auto new_behaviour = std::make_shared<detail::map_behaviour<result_type, T>>(_rv_core, f_);

			rv_builder<result_type> builder{new_behaviour};

			return builder;
		}

		template<class... Us, class... Js>
			rv_builder<std::tuple<T, Us...>> join_with(rv<Us, Js>&... rvs_) const
		{
			typedef std::tuple<T, Us...> result_type;

			// shared_ptr<typed_behaviour<result_type, T, Us...>>
			auto new_behaviour = std::make_shared<detail::join_behaviour<result_type, T, Us...>>(
				make_tuple(_rv_core, rvs_.internal_accessor()...));

			// ctor: shared_ptr<typed_behaviour<result_type>>
			return rv_builder<result_type>{std::move(new_behaviour)};
		}

		template<class... Us, class... Js> auto merge_with(rv<Us, Js>&... rvs_)
		{
			return merge_with({std::ref(rvs_)...});
		}

		template<class U, class J> auto merge_with(std::initializer_list<std::reference_wrapper<rv<U, J>>> rvs_)
		{
			return merge_with(rvs_.begin(), rvs_.end());
		}

		template<class I> auto merge_with(I it_beg_, I it_end_)
		{
			std::vector<std::shared_ptr<accessor<T>>> src_rvs { _rv_core };
			
			for (auto it = it_beg_; it != it_end_; ++it)
			{
				src_rvs.push_back(it->get().internal_accessor());
			}

			auto new_behaviour = std::make_shared<detail::merge_behaviour<T>>(std::move(src_rvs));

			return rv_builder<std::vector<T>> { new_behaviour };
		}

		auto flat_map()
			-> rv_builder<typename detail::flatten_type_of<T>::type>
		{
			typedef typename detail::flatten_type_of<T>::type flatten_type;

			static_assert(Utility::is_vector<flatten_type>::value, "Traget type must be some kind of std::vector<?>");

			auto new_behaviour = std::make_shared<detail::flatmap_behaviour<flatten_type, T>>(_rv_core);

			return rv_builder<flatten_type> {std::move(new_behaviour)};
		}

	private:
		std::shared_ptr<accessor<T>> _rv_core;
	};

	//
	//	map
	//
	template<class FUNC, class T, class I> auto map(rv<T, I>& rv_, FUNC func_)
	{
		rv_builder<T> builder{rv_};

		return builder.map(func_);
	}

	//
	//	flatmap
	//
	//	[ [a,b], c, [[d,e]] ] = 
	//
	//rv_flatmap_builder<> flatmap(std::vector<>)
	template<class T, class I> auto flatmap(rv<T, I>& rv_)
	{
		// T is like vector<vector<T>>
		rv_builder<T> behaviour {rv_};

		return behaviour.flat_map();
	}

	//
	//	join
	//
	template<class T1, class I1, class... Ts, class... Is>
		rv_builder<std::tuple<T1, Ts...>> join(rv<T1, I1>& rv1_, rv<Ts, Is>&... rvs_)
	{
		rv_builder<T1> builder {rv1_};

		return builder.join_with(rvs_...);
	}

	//
	//	merge
	//
	template<class T, class I> auto merge(std::initializer_list<std::reference_wrapper<rv<T, I>>> rv_list_)
	{
		auto it_first = rv_list_.begin();

		rv<T, I>& first = *it_first;

		rv_builder<T> builder {first};

		++it_first;

		return builder.merge_with(it_first, rv_list_.end());
	}

	template<class... Ts, class... Is> auto merge(rv<Ts, Is>&... rvs_)
	{
		typedef Utility::first_of<Ts...>::type T1;
		typedef Utility::first_of<Is...>::type I1;

		static_assert(Utility::is_same_as_all_of<T1, Ts...>::value, "All Ts... must be the same");

		return merge<T1, I1>({std::ref(rvs_)...});
	}

}
