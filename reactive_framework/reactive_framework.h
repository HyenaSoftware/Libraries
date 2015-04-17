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

#include <Utility\meta.h>
//#include <Utility\exception.hpp>
#include <Utility\traits.h>
//#include <Utility\hash.h>

namespace reactive_framework
{
	template<class R> class typed_behaviour
	{
	public:
		typedef R result_type;

		virtual result_type operator()() const = 0;
		virtual typed_behaviour& operator=(result_type)
		{
			// No operation
			return *this;
		}
	};

	template<class T> class value_holder : public typed_behaviour<T>
	{
	public:
		value_holder() = default;
		value_holder(const value_holder&) = default;

		value_holder& operator=(const value_holder&) = default;
		value_holder& operator=(T t_) override
		{
			std::swap(_value, t_);

			return *this;
		}

		result_type operator()() const override
		{
			return _value;
		}

	private:
		result_type _value;
	};

	// forward declaration
	template<class T, class TRAIT = void> class rv_builder;


	template<class T, class I = std::string> class rv
	{
		template<class, class> friend class rv_builder;
	public:
		typedef T value_type;
		typedef I id_type;

		rv() = default;

		rv(const rv&) = default;

		rv(rv&& other_) : _name{std::move(other_._name)}
		{
			std::swap(_behaviour, other_._behaviour);
		}

		rv(value_type value_) : _value{value_}
		{
		}

		rv(id_type name_) : _name{name_}
		{
		}

		rv(value_type value_, id_type id_) : _value{value_}, _name{id_} {}

		operator value_type () const
		{
			return (*_behaviour)();
		}

		rv& operator=(const value_type& value_)
		{
			*_behaviour = value_;

			return *this;
		}

		rv& operator=(const rv& other_)
		{
			_name = other_._name;
			_behaviour = other_._behaviour;

			return *this;
		}

		void rebind(std::shared_ptr<typed_behaviour<T>> behaviour_)
		{
			std::swap(_behaviour, behaviour_);
		}

		void set_name(std::string name_)
		{
			std::swap(_name, name_);
		}

		const std::string& name() const
		{
			return _name;
		}

	private:
		id_type _name = id_type{};

		std::shared_ptr<typed_behaviour<T>> _behaviour = make_default_behaviour();
			
		static std::shared_ptr<typed_behaviour<T>> make_default_behaviour()
		{
			return std::make_shared<value_holder<T>>();
		}
	};

	template<class I> class rv<void, I>
	{
	public:
		static_assert(std::is_same<void, void>::value, "Type T can not be void.");

		rv(...){}
	};


	template<class T> struct is_trait : std::false_type{};

	// rv basic trait
	template<class T> struct is_rv : std::false_type{};
	template<class T, class I> struct is_rv<rv<T, I>> : std::true_type{};

	// rv factory trait
	struct rv_factory_trait
	{
	};

	// traits for merge
	template<class T, class I> struct merge_trait
	{
		//static_assert(is_rv<RV>::value, "Type RV must be like rv<?>.");

		typedef rv<T, I>	rv_type;

		typedef rv_type		rv_input_type;

		typedef typename rv_type::value_type	output_arg_type;
		typedef std::vector<output_arg_type>	output_container_type;
		typedef rv<output_container_type>		rv_output_type;
	};

	// traits for join
	template<class... Ts> struct join_trait
	{
		typedef std::tuple<Ts...> input_type;

		//typedef R output_type;
		typedef input_type output_type;

		template<class... Ts> struct type_of_output_value
		{
			typedef	std::tuple<Ts...> type;
		};

		// move to rv_join_builder
		struct joiner_functor
		{
			output_type operator()(Ts... ts_) const
			{
				return output_type{std::forward<Ts>(ts_)...};
			}
		};
	};

	template<class... Ts> struct is_trait<join_trait<Ts...>> : std::true_type{};


	// trait for flatten
	template<class T> struct flatten_trait;
	template<class T> struct flatten_trait<std::vector<T>>
	{
		typedef std::vector<T> source_type;
		typedef T component_type;
	};


	namespace detail
	{
		template<class R, class... Ts> class join_behaviour : public typed_behaviour<R>
		{
		public:
			join_behaviour(std::tuple<std::shared_ptr<typed_behaviour<Ts>>...> rvs_)
				: _rvs{rvs_}
			{
			}

			result_type operator()() const override
			{
				typedef Utility::seq_builder<sizeof...(Ts)>::type seq_type;

				return _apply_impl(seq_type{});
			}

		private:
			std::tuple<std::shared_ptr<typed_behaviour<Ts>>...> _rvs;

			template<int... Ns> result_type _apply_impl(Utility::seq<Ns...>) const
			{
				return result_type{std::get<Ns>(_rvs)->operator()()...};
			}
		};

		template<class OUT_TYPE, class IN_TYPE> class map_behaviour : public typed_behaviour<OUT_TYPE>
		{
		public:
			typedef IN_TYPE input_type;

			map_behaviour(std::shared_ptr<typed_behaviour<input_type>> src_behaviour_, std::function<result_type(input_type)> func_)
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
			std::shared_ptr<typed_behaviour<input_type>> _src_behaviour;
		};

		template<class T> class merge_behaviour : public typed_behaviour<std::vector<T>>
		{
		public:
			merge_behaviour(std::vector<std::shared_ptr<typed_behaviour<T>>> rvs_)
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
			std::vector<std::shared_ptr<typed_behaviour<T>>> _rvs;
		};
	}

	//
	// builder for rvs
	//	template parameters:
	//		S: status
	template<class T, class TRAIT > class rv_builder
	{
	public:
		typedef TRAIT trait_type;

		rv_builder(std::shared_ptr<typed_behaviour<T>> behaviour_)
		{
			std::swap(_rv_core, behaviour_);
		}

		template<class I> rv_builder(rv<T, I>& rv_)
		{
			_rv_core = rv_._behaviour;
		}

		template<class T, class TRAIT = flatten_trait<T>> rv_builder<TRAIT> flatten(rv<T>&)
		{
			return rv_builder<T> {_rv_core};
		}

		template<class I> void to(rv<T, I>& rv_) const
		{
			rv_.rebind(std::move(_rv_core));
		}

		template<class I = std::string> rv<T, I> build(I id_ = I{}) const
		{
			rv<T, I> result;

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
				make_tuple(_rv_core, rvs_._behaviour...));

			// ctor: shared_ptr<typed_behaviour<result_type>>
			return rv_builder<result_type>{std::move(new_behaviour)};
		}

		template<class... Us, class... Js>
			rv_builder<std::tuple<T, Us...>> join_with(std::tuple<std::reference_wrapper<rv<Us, Js>>...> rvs_tpl_) const
		{
			typedef Utility::seq_builder<sizeof...(Us)>::type seq_type;

			typedef rv_builder<T, TRAIT> this_class;
			typedef rv_builder<std::tuple<T, Us...>>(this_class::*PtrJoinWith)(rv<Us, Js>&...) const;

			PtrJoinWith ptr = &this_class::join_with;

			return Utility::extract_n_call(
				Utility::variadic_bind(ptr, this, seq_type{}),
				seq_type{},
				rvs_tpl_);
		}

		//template<class TRAIT, class... Us, class... Js>
		//	rv_builder<typename TRAIT:: std::tuple<T, Us...>> join_with(TRAIT, std::tuple<std::reference_wrapper<rv<Us, Js>>...> rvs_) const
		//{
		//		return rv_builder<std::tuple<T, Us...>>{};
		//}

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
			std::vector<std::shared_ptr<typed_behaviour<T>>> src_rvs { _rv_core };
			
			for (auto it = it_beg_; it != it_end_; ++it)
			{
				src_rvs.push_back(it->get()._behaviour);
			}

			auto new_behaviour = std::make_shared<detail::merge_behaviour<T>>(std::move(src_rvs));

			return rv_builder<std::vector<T>> { new_behaviour };
		}

		void flat_map()
		{
		}

	private:
		std::shared_ptr<typed_behaviour<T>> _rv_core;
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

	//
	//	join
	//
	template<class T1, class I1, class... Ts, class... Is>
		rv_builder<std::tuple<T1, Ts...>> join(rv<T1, I1>& rv1_, rv<Ts, Is>&... rvs_)
	{
		rv_builder<T1> builder {rv1_};

		tuple<std::reference_wrapper<rv<Ts, Is>>...> tail {std::ref(rvs_)...};
		
		return builder.join_with(std::move(tail));
	}

	//template<class TRAIT, class T1, class I1, class... Ts, class... Is>
	//	auto join(TRAIT t_, rv<T1, I1>& rv1_, rv<Ts, Is>&... rvs_)
	//{
	//	rv_builder<T1, TRAIT> builder {rv1_};

	//	tuple<std::reference_wrapper<rv<Ts, Is>>...> tail
	//		= make_tuple<std::reference_wrapper<rv<Ts, Is>>...>(std::ref(rvs_)...);

	//	return builder.join_with(t_, std::move(tail));
	//}

	//
	//	merge
	//
	template<class T, class I, class TRAIT = merge_trait<rv<T, I>>> auto merge(std::initializer_list<std::reference_wrapper<rv<T, I>>> rv_list_)
	{
		auto it_first = rv_list_.begin();

		rv<T, I>& first = *it_first;

		rv_builder<T> builder {first};

		++it_first;

		return builder.merge_with(it_first, rv_list_.end());
	}

	template<
		class... Ts, class... Is,
		class TRAIT = merge_trait<typename Utility::first_of<Ts...>::type, typename Utility::first_of<Is...>::type
	>>
		auto merge(rv<Ts, Is>&... rvs_)
	{
		typedef Utility::first_of<Ts...>::type T1;
		typedef Utility::first_of<Is...>::type I1;

		// rv_builder{rv1}.merge_with(rvs_...)
		return merge<T1, I1, TRAIT>({std::ref(rvs_)...});
	}

}
