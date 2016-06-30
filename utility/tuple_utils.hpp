#pragma once
#include <tuple>
#include <vector>
#include "any.hpp"
#include "object_ref.hpp"



namespace utility
{
	namespace detail
	{
		template<class T, class... Ts, class... Us, class I> auto _to_tuple_helper(I beg_, I end_, Us&&... us_)
		{
			auto next = beg_;

			return _to_tuple_helper<Ts...>(++next, end_, std::forward<Us>(us_)..., static_cast<T>(*beg_));
		}

		template<class... Us, class I> auto _to_tuple_helper(I beg_, I end_, Us&&... us_)
		{
			return std::make_tuple(std::forward<Us>(us_)...);
		}
	}


	template<class... Ts, class I> std::tuple<Ts...> to_tuple(I beg_, I end_)
	{
		return _to_tuple_helper<Ts...>(beg_, end_);
	}





	//
	//
	//
	template<class R, class... As> class tuple_arged_function
	{
	public:
		typedef R return_t;

		tuple_arged_function() = default;
		tuple_arged_function(std::function<return_t(As...)> functor_)
			: _functor{ std::move(functor_) }
		{
		}

		return_t operator()(std::tuple<As...> args_)
		{
			constexpr size_t ARGS_COUNT = sizeof...(As);

			typedef std::make_index_sequence<ARGS_COUNT> seq_t;

			return operator()(std::move(args_), seq_t{});
		}

	private:
		std::function<return_t(As...)> _functor;

		template<size_t... Is> return_t operator()(std::tuple<As...> args_, std::index_sequence<Is...>)
		{
			return _functor(std::get<Is>(args_)...);
		}
	};

	//
	//
	//
	template<class R, class... As> class any_arged_function
	{
	public:
		typedef R return_t;
		typedef std::tuple<As...> tuple_t;

		enum { arity = sizeof...(As) };

		any_arged_function() = default;
		any_arged_function(std::function<return_t(As...)> functor_)
			: _functor{ std::move(functor_) }
		{
		}

		/*
			T obj;
			T* ptr_obj = &a;
			any_ptr ptr_ptr_obj = &ptr_obj;

			int a1;
			any_ptr ptr_a1 = &a1;

			float* a2;
			any_ptr ptr_a2 = &a2;

			any_ptr -> T*		== object pointer == [this]
			any_ptr -> int		== arg1
			any_ptr -> float*	== arg2

			corresponding method signature:
				f(T*, int, float*)
		*/

		[[deprecated]] return_t operator()(std::vector<any_ptr> args_) const
		{
			typedef std::make_index_sequence<arity> seq_t;

			return operator()(std::move(args_), seq_t { });
		}

		return_t operator()(std::vector<object_ref> args_) const
		{
			typedef std::make_index_sequence<arity> seq_t;

			return operator()(std::move(args_), seq_t{});
		}

	private:
		std::function<return_t(As...)> _functor;

		template<size_t... Is> return_t operator()(std::vector<any_ptr> args_, std::index_sequence<Is...>) const
		{
			return _functor(
				*static_cast<std::remove_reference_t<std::tuple_element_t<Is, tuple_t>>*>(args_.at(Is)) ...
				);
		}

		template<size_t... Is> return_t operator()(std::vector<object_ref> args_, std::index_sequence<Is...>) const
		{
			return _functor(
				args_.at(Is).as<std::tuple_element_t<Is, tuple_t>>()
				...
			);
		}
	};


	//
	//
	//
	template<class> struct is_tuple : std::false_type {};
	template<class... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};

	/*
	for each loop for tuple
	*/
	namespace detail
	{
		template<class> struct _foreach_impl;
		template<size_t N, size_t... Ns> struct _foreach_impl<std::index_sequence<N, Ns...>>
		{
			template<class T, class F> void operator()(T&& tuple_, F&& lambda_) const
			{
				lambda_(std::get<N>(tuple_));
				_foreach_impl<std::index_sequence<Ns...>> { } (std::forward<T>(tuple_), std::forward<F>(lambda_));
			}
		};

		template<size_t N> struct _foreach_impl<std::index_sequence<N>>
		{
			template<class T, class F> void operator()(T&& tuple_, F&& lambda_) const
			{
				lambda_(std::get<N>(tuple_));
			}
		};
	}

	template<class F, class... Ts> void for_each(std::tuple<Ts...> tuple_, F lambda_)
	{
		typedef std::index_sequence_for<Ts...> seq_t;

		detail::_foreach_impl<seq_t> { } (std::move(tuple_), std::move(lambda_));
	}
}
