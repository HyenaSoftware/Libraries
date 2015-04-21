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
#include <Utility\traits.h>

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
		value_holder& operator=(result_type t_) override
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
	template<class T> class rv_builder;


	template<class T, class I = std::string> class rv
	{
		template<class> friend class rv_builder;
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

		rv(value_type value_, id_type id_) : _name{id_}
		{
			(*_behaviour) = std::move(value_);
		}

		operator value_type () const
		{
			return (*_behaviour)();
		}

		value_type operator()() const
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

	static_assert(std::is_default_constructible<rv<int>>::value, "rv<?> must be default constructible");

	// rv basic trait
	template<class T> struct is_rv : std::false_type{};
	template<class T, class I> struct is_rv<rv<T, I>> : std::true_type{};


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

			flatmap_behaviour(std::shared_ptr<typed_behaviour<S>> rv_)
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
			std::shared_ptr<typed_behaviour<S>> _rv;
		};
	}

	//
	// builder for rvs
	//	template parameters:
	//		S: status
	template<class T> class rv_builder
	{
	public:
		rv_builder(std::shared_ptr<typed_behaviour<T>> behaviour_)
		{
			std::swap(_rv_core, behaviour_);
		}

		template<class I> rv_builder(rv<T, I>& rv_)
		{
			_rv_core = rv_._behaviour;
		}

		template<class T> rv_builder<T> flatten(rv<T>&)
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

		auto flat_map()
			-> rv_builder<typename detail::flatten_type_of<T>::type>
		{
			typedef typename detail::flatten_type_of<T>::type flatten_type;

			static_assert(Utility::is_vector<flatten_type>::value, "Traget type must be some kind of std::vector<?>");

			auto new_behaviour = std::make_shared<detail::flatmap_behaviour<flatten_type, T>>(_rv_core);

			return rv_builder<flatten_type> {std::move(new_behaviour)};
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
