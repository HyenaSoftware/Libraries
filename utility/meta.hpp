#pragma once

//#ifndef HERE
//#define HERE "<HERE macro is undefined> "
//#endif

#include <type_traits>
#include <utility>

#include "macros"
#include "traits.hpp"
#include "meta\list.hpp"
#include "meta\value_list.h"

namespace utility
{
	template<int a, int b> struct static_gcd
	{
		static const int value = static_gcd<b, a%b>::value;
	};
	template<int a> struct static_gcd<a,0>
	{
		static const int value = a;
	};

	template<int a> struct static_abs
	{
		static const int value = a < 0 ? -a : a;
	};

	// calculate n^k
	template<int N, int K> struct static_pow
	{
		static const int value = static_pow<N,K-1>::value * N;
	};
	template<int N> struct static_pow<N,0>
	{
		static const int value = 1;
	};

	template<int N, int D = 1> class static_fraction
	{
		static const int _Nabs = static_abs<N>::value;
		static const int _Dabs = static_abs<D>::value;
		static const int _gcd = static_gcd<Nabs,Dabs>::value;
	public:
		static const int numerator = N / _gcd;
		static const int denominator = D / _gcd;
	};

	template<typename T, T MAN, T A> struct static_float
	{
		static_assert( std::is_arithmetic<T>::value, "T type must be an arithmetic type");
		static const float value;
	};
	template<typename T, T MAN, T A> const float static_float<T,MAN,A>::value = MAN * static_pow<10,A>::value;


	template<class A, class B, class C, int N> struct static_switch;
	template<class A, class B, class C> struct static_switch<A,B,C,0> { typedef A type; };
	template<class A, class B, class C> struct static_switch<A,B,C,1> { typedef B type; };
	template<class A, class B, class C> struct static_switch<A,B,C,2> { typedef C type; };

#if _MSC_VER >= 1700
	template<class T, class P = std::less_equal<T>> void assign_if(T& a, const T& b)
	{
		P pred;
		if( pred(a,b) )
			a = b;
	}
#else
	template<class T, class P = std::less_equal<T>> struct assign_if
	{
		P pred;
		void operator()(T& a, const T& b) const
		{
			if( pred(a,b) )
				a = b;
		}
	};
#endif

	// Existance quantor	(E)
	template<class,				class...>		struct is_any_of;		// declaration to allow empty Ts... pack
	template<class V, class T1, class... Ts>	struct is_any_of<V, T1, Ts...>	: is_any_of<V, Ts...> { };
	template<class V,			class... Ts>	struct is_any_of<V, V,	Ts...>	: std::true_type { };
	template<class V>							struct is_any_of<V>				: std::false_type { };

	// Universal quantor	(A)
	template<class, class...>					struct is_all_of;
	template<class V, class T1, class... Ts>	struct is_all_of<V, T1, Ts...>	: std::false_type { };
	template<class V,			class... Ts>	struct is_all_of<V, V,  Ts...>	: is_all_of<V, Ts...> { };
	template<class V>							struct is_all_of<V>				: std::true_type { };

	template<class, class...>				struct is_neither_of;
	template<class V, class H, class... T>	struct is_neither_of<V, H, T...> : is_neither_of<V, T...> { };
	template<class V, class... T>			struct is_neither_of<V, V, T...> : std::false_type { };
	template<class V>						struct is_neither_of<V> : std::true_type { };

	// define shortcuts
	//template<class... Ts> using is_any_of_v = is_any_of<Ts...>::value;
	//template<class... Ts> using is_all_of_v = is_all_of<Ts...>::value;


	static_assert(!is_any_of<char>::value, "is_any_of test #1");
	static_assert(!is_any_of<char, int>::value, "is_any_of test #2");
	static_assert(is_any_of<char, char>::value, "is_any_of test #3");
	static_assert(is_any_of<char, char, int>::value, "is_any_of test #4");
	static_assert(is_any_of<char, int, char>::value, "is_any_of test #5");

	static_assert(!is_neither_of<char, int, char>::value, "E1");
	static_assert(!is_neither_of<char, char, int>::value, "E2");
	static_assert(is_neither_of<char, int, float>::value, "E3");
	static_assert(is_neither_of<char, float>::value, "E4");
	static_assert(is_neither_of<char, float, int>::value, "E5");
	static_assert(is_neither_of<char>::value, "E6");


	template<template<class> class, class...> struct for_any_of;
	template<template<class> class P, class H, class... Ts> struct for_any_of<P, H, Ts...>
		: std::conditional<P<H>::value,
			std::true_type, 
			for_any_of<P, Ts...>
		>::type {};
	template<template<class> class P> struct for_any_of<P> : std::false_type {};


	template<template<class> class, class...> struct for_all_of;
	template<template<class> class P, class H, class... Ts> struct for_all_of<P, H, Ts...>
		: std::conditional<P<H>::value,
			for_all_of<P, Ts...>,
			std::false_type
		>::type {};
	template<template<class> class P> struct for_all_of<P> : std::true_type {};


	//
	//	is_same_as_all_of<T, A, B, C>
	//
	template<class T, class... Us> struct is_same_as_all_of : std::false_type {};
	template<class T, class... Us> struct is_same_as_all_of<T, T, Us...> : is_same_as_all_of<T, Us...>{};
	template<class T> struct is_same_as_all_of<T, T> : std::true_type {};

	//
	//	is_same_as_any_of<T, A, B, C>
	//
	template<class T, class S, class... Us> struct is_same_as_any_of : is_same_as_any_of<T, Us...> {};
	template<class T, class... Us> struct is_same_as_any_of<T, T, Us...> : std::true_type {};
	template<class T, class S> struct is_same_as_any_of<T, S> : std::false_type {};

	//
	// get the first item of a variadic type list
	//
	template<class T1, class...> struct first_of
	{
		typedef T1 type;
	};

	//
	// get the tail of a variadic type list
	//
	template<class, class... Ts> struct tail_of
	{
		typedef std::tuple<Ts...> type;
	};

	//
	//	P : predicate, which takes a type and returns std::true_type or std::false_type
	//	D : default or fallback value, which is used when P predicate is false for all of types
	//	H and Ts... : these are the parts of a variadic type list (head + tail)
	//
	template<template<class> class P, class D, class H, class... Ts> struct select_if
	{
		typedef typename std::conditional<
			P<H>::value,
			H,
			typename select_if<P, D, Ts...>::type
		>::type type;
	};

	template<template<class> class P, class D, class H> struct select_if<P, D, H>
	{
		typedef typename std::conditional<
			P<H>::value,
			H,
			D
		>::type type;
	};


	/**
	 *	it used to create a mutable pair<A, B> from an immutable one
	 *	it's mainly used when we want to get a modifiable copy from a key of a map
	 *	
	 *	map<string, int> my_map;
	 *	
	 *	auto it = my_map.find("key")
	 *	auto entry = *it;
	 *	entry.first = "it fails"; // we would like to change the key
	 *		// but it won't compile, because type of entry is pair<const string, int>
	 *		// so the first part of this pair is immutable
	 *	my_map.insert(entry);
	 *
	 * auto entry = mutable_pair_of(*it);
	 * entry.first = "success!";
	 *
	 */
	template<class> struct mutable_pair_type_of;
	template<class A, class B> struct mutable_pair_type_of<std::pair<const A, B>>
	{
		typedef std::pair<A, B> type;
	};

	template<class A, class B> std::pair<A, B> mutable_pair_of(const std::pair<const A, B>& pair_)
	{
		return std::pair<A, B>(pair_.first, pair_.second);
	}

	template<class A, class B, class T> struct _get_helper_
	{
		static_assert(std::is_same<A,T>::value || std::is_same<B,T>::value, "The pair has no T typed field.");
	};
	template<class A, class B> struct _get_helper_<A, B, A>
	{
		template<class A2, class B2> inline static A2& get(std::pair<A2, B2>& p)
		{
			return p.first;
		}
	};
	template<class A, class B> struct _get_helper_<A, B, B>
	{
		template<class A2, class B2> inline static B2& get(std::pair<A2, B2>& p)
		{
			return p.second;
		}
	};

	template<class T, class A, class B> inline typename _get_helper_<
		typename std::remove_const<A>::type, typename std::remove_const<A>::type, T>::type&
		get(std::pair<A, B>& p)
	{
		typedef typename std::remove_const<A>::type typeofA;
		typedef typename std::remove_const<B>::type typeofB;

		static_assert(std::is_same<typeofA,T>::value || std::is_same<typeofB,T>::value, "The pair has no T typed field.");

		return _get_helper_<T, typeofA, typeofB, T>::get<A, B>(p);
	}

	// std::tuple
	template<class> struct mutable_tuple_type_of;
	template<class... A> struct mutable_tuple_type_of<std::tuple<A...>>
	{
		typedef std::tuple<typename std::remove_const<A>::type...> type;
	};

	template<class... Ts> auto mutable_tuple_of(const std::tuple<Ts...>& tpl_)
	{
		typedef typename mutable_tuple_type_of<std::tuple<Ts...>>::type mutable_tuple_type;

		return mutable_tuple_type{tpl_};
	}

	namespace _helper
	{
		// there is no T in U
		template<int I, int N, class T, class U>				struct _get_tuple_idx
		{
			static_assert(!std::is_same<U, std::tuple<>>::value, "std::tuple<...> has no item type T");
		};
		// if it has a T in the tuple, it's the Nth
		template<int I, class T, class... U>			struct _get_tuple_idx<I, 1, T, std::tuple<T, U...>>
		{
			static const int value = I;
		};
		// if it has a T in the tuple, but it isn't the Nth
		template<int I, int N, class T, class... U>			struct _get_tuple_idx<I, N, T, std::tuple<T, U...>>
		{
			static const int value = _get_tuple_idx<I+1, N-1, T, std::tuple<U...>>::value;
		};
		// if the current head of subtuple isn't matching with T
		template<int I, int N, class T, class S, class... U>	struct _get_tuple_idx<I, N, T, std::tuple<S, U...>>
		{
			static const int value = _get_tuple_idx<I+1, N, T, std::tuple<U...>>::value;
		};
	}

	//
	//	VC++ STL workaround: std::get<type>(tuple) is missing from STL
	//
	template<class, class, int> struct get_tuple_idx;

	template<class T, class... U, int N> struct get_tuple_idx<T, std::tuple<U...>, N>
	{
		static const int value = _helper::_get_tuple_idx<0, N, T, std::tuple<U...>>::value;
	};

	template<class T, int N = 1, class... U> T& get(std::tuple<U...>& tpl)
	{
		static const int index = _helper::_get_tuple_idx<0, N, T, std::tuple<U...>>::value;

		return std::get<index>(tpl);
	}

	template<class T, int N = 1, class... U> const T& get(const std::tuple<U...>& tpl_)
	{
		static const int index = _helper::_get_tuple_idx<0, N, T, std::tuple<U...>>::value;

		return std::get<index>(tpl_);
	}

	template<class T, int N = 1, class... U> T get(std::tuple<U...>&& tpl)
	{
		return std::move(std::get<get_tuple_idx<T, std::tuple<U...>, N>::value>(tpl));
	}

	//
	//	get<type>(tuple) with fallback value
	//
	template<class T, int N = 1, class... Us> typename std::enable_if<is_any_of<T, Us...>::value, T>::type&
		get_or(std::tuple<Us...>& tpl_, T&& = T {})
	{
		return get<T, N>(tpl_);
		//return std::get< get_tuple_idx<T, std::tuple<Us...>, N>::value >(tpl);
	}

	template<class T, int N = 1, class... Us> typename std::enable_if<is_any_of<T, Us...>::value, T>::type
		get_or(std::tuple<Us...>&& tpl, T&& = T {})
	{
		return std::move(get<T, N>(tpl_));
		//return std::move(std::get< get_tuple_idx<T, std::tuple<Us...>, N>::value >(tpl));
	}

	template<class T, int = 1, class... Us> typename std::enable_if<is_neither_of<T, Us...>::value, T>::type&&
		get_or(std::tuple<Us...>&, T&& t_ = T{})
	{
		static_assert(std::is_default_constructible<T>::value, "T must be default constructible.");

		return std::forward<T>(t_);
	}

	//
	//
	//
	template<class A, class B> struct min_size
	{
		typedef typename std::conditional<(sizeof(A) < sizeof(B)), A, B>::type type;
	};

	template<class A, class B> struct max_size
	{
		typedef typename std::conditional<(sizeof(A) > sizeof(B)), A, B>::type type;
	};

	namespace
	{
		template<template<class, class> class F, class S, class T, class... U> struct _helper_select
		{
			typedef typename _helper_select<F, typename F<S, T>::type, U...>::type type;
		};

		template<template<class, class> class F, class S, class T> struct _helper_select<F, S, T>
		{
			typedef typename F<S, T>::type type;
		};
	}

	template<template<class, class> class F, class... T> struct select
	{
		typedef typename _helper_select<F, T...>::type type;
	};

	template<template<class, class> class F, class T> struct select<F, T>
	{
		typedef T type;
	};

	template<bool, class...> struct tuple_cat_if;
	template<class T1, class... Ts> struct tuple_cat_if<false, T1, Ts...>
	{
		typedef std::tuple<Ts...> type;
	};

	template<class... Ts> struct tuple_cat_if<true, Ts...>
	{
		typedef std::tuple<Ts...> type;
	};


	/*
		(tuple<Ts...>, PRED) -> tuple<Us...>

		Ts... >= Us...
	*/
	template<template<class> class F, class TPL, class = std::tuple<>> struct tuple_select;
	template<template<class> class F, class T1, class... Ts, class... Us>
		struct tuple_select<F, std::tuple<T1, Ts...>, std::tuple<Us...>>
	{
		typedef typename tuple_select<
			F,							// predicate
			std::tuple<Ts...>,			// remains, uninvestigated items
			typename std::conditional<
				F<T1>::value,			// if PRED(T1) == true
				std::tuple<Us..., T1>,	// T1 is selected
				std::tuple<Us...>		// drop T1
			>::type
		>::type type;
	};
	template<template<class> class F, class R> struct tuple_select<F, std::tuple<>, R>
	{
		typedef R type;	// return with the selected itemset
	};

	template<template<class, int> class F, class TPL, class RPL, class MAP> struct tuple_select_nth_impl;
	template<template<class, int> class F, class T1, class... Ts, class... Us, class MAP>
		struct tuple_select_nth_impl<F, std::tuple<T1, Ts...>, std::tuple<Us...>, MAP>
	{
		typedef typename collections::get<MAP, T1>::type count_of_T1_type;	// check how many T1 types we have

		// this will be the new value
		typedef value_holder<int, get_value_of<count_of_T1_type>::value + 1> inc_count_of_T1_type;

		typedef typename collections::set<MAP, T1, inc_count_of_T1_type>::type MAP2;	// set it

		typedef typename tuple_select_nth_impl<
			F,							// predicate
			std::tuple<Ts...>,			// remains, uninvestigated items
			typename std::conditional<
				F<T1, get_value_of<inc_count_of_T1_type>::value>::value,			// if PRED(T1) == true
				std::tuple<Us..., T1>,	// T1 is selected
				std::tuple<Us...>		// drop T1
			>::type,
			MAP2
		>::type type;
	};
	template<template<class, int> class F, class R, class MAP> struct tuple_select_nth_impl<F, std::tuple<>, R, MAP>
	{
		typedef R type;	// return with the selected itemset
	};

	template<template<class, int> class F, class TPL> struct tuple_select_nth;
	template<template<class, int> class F, class... Ts> struct tuple_select_nth<F, std::tuple<Ts...>>
		: tuple_select_nth_impl<F, std::tuple<Ts...>, std::tuple<>, std::tuple< std::pair<Ts, value_holder<int, 0>> ... > >
	{
	};



	/*
		get<1, Ts...>::type
	*/

	template<int N, class... Ts> struct get_of;
	template<class H, class... Ts> struct get_of<0, H, Ts...>
	{
		typedef H type;
	};

	template<int N, class H, class... Ts> struct get_of<N, H, Ts...> : get_of<N-1, Ts...>
	{
	};

	


	template<bool> struct bool_to_int;
	template<> struct bool_to_int<false>
	{
		static const int value = 0;
	};
	template<> struct bool_to_int<true>
	{
		static const int value = 1;
	};


	namespace detail
	{
		template<class SEQ, class T> struct tuple_tail_helper;
		template<size_t... Is, class T1, class... Ts> struct tuple_tail_helper<std::index_sequence<Is...>, std::tuple<T1, Ts...>>
		{
			static_assert(sizeof...(Is) == sizeof...(Ts), "tuple_tail");

			std::tuple<Ts...> operator()(std::tuple<T1, Ts...> tpl_) const
			{
				return std::make_tuple(std::forward<Ts>(std::get<Is>(tpl_))...);
			}
		};
	}

	template<class T1, class... Ts> std::tuple<Ts...> tuple_tail(std::tuple<T1, Ts...> tpl_)
	{
		typedef Utility::seq_builder<sizeof...(Ts)>::type seq_type;

		tuple_tail<seq_type> helper;

		return helper(std::move(tpl_));
	}


	//
	//	auxiliaries for std::bind
	//
	//	variadic_bind(f, seq<0,1,2,3>) -> std::bind(f, _1, _2, _3, _4)
	//
	//
	template<int> struct placeholder_template{};

	template<class T, class R, class... As, int... Ns> auto variadic_bind(R(T::*f_)(As...), T* obj_, std::index_sequence<Ns...>)
	{
		return std::bind(f_, obj_, placeholder_template<Ns - 1>{} ...);
	}

	template<class T, class R, class... As, int... Ns> auto variadic_bind(R(T::*f_)(As...) const, const T* obj_, std::index_sequence<Ns...>)
	{
		return std::bind(f_, obj_, placeholder_template<Ns - 1>{} ...);
	}

	template<class R, class... As, int... Ns> auto variadic_bind(R(*f_)(As...), std::index_sequence<Ns...>)
	{
		return std::bind(f_, placeholder_template<Ns - 1>{} ...);
	}

	//
	// call f_ with the unpacked content of a tuple
	//
	template<class F, size_t... Ns, class... Ts> auto extract_n_call(F f_, std::index_sequence<Ns...>, std::tuple<Ts...>& tpl_)
	{
		return f_(std::get<Ns>(tpl_)...);
	}


	template<class F, class... Ts> auto extract_n_call(F f_, std::tuple<Ts...>& tpl_)
		-> typename function_traits<F>::result_type
	{
		return extract_n_call(f_, seq_builder<sizeof...(Ts)>::type{}, tpl_);
	}
}

namespace std
{
	template<int N> struct is_placeholder<utility::placeholder_template<N>> : integral_constant<int, N + 2>{};
}

namespace utility
{
	namespace detail
	{
		//
		// call f_ with the unpacked content of a tuple
		//
		template<class F, size_t... Ns, class... Ts> auto unpack_and_call_impl(F function_, std::index_sequence<Ns...>, std::tuple<Ts...>& tpl_)
		{
			return function_(std::get<Ns>(tpl_)...);
		}

		template<class F, size_t... Ns, class... Ts> auto unpack_and_call_impl(F function_, std::index_sequence<Ns...>, const std::tuple<Ts...>& tpl_)
		{
			return function_(std::get<Ns>(tpl_)...);
		}

		//
		// + argument transformation
		//
		template<class F, class G, size_t... Ns, class... Ts> auto unpack_and_call_impl(F function_, G transformer_, std::index_sequence<Ns...>, std::tuple<Ts...>& tpl_)
		{
			return function_(transformer_(std::get<Ns>(tpl_))...);
		}

		template<class F, class G, size_t... Ns, class... Ts> auto unpack_and_call_impl(F function_, G transformer_, std::index_sequence<Ns...>, const std::tuple<Ts...>& tpl_)
		{
			return function_(transformer_(std::get<Ns>(tpl_))...);
		}
	}

	// F
	template<class F, class... Ts> auto unpack_and_call(F function_, std::tuple<Ts...>& tpl_)
	{
		return detail::unpack_and_call_impl(function_, std::make_index_sequence<sizeof...(Ts)> { }, tpl_);
	}

	template<class F, class... Ts> auto unpack_and_call(F function_, const std::tuple<Ts...>& tpl_)
	{
		return detail::unpack_and_call_impl(function_, std::make_index_sequence<sizeof...(Ts)> { }, tpl_);
	}

	// F + G
	template<class F, class G, class... Ts> auto unpack_and_call(F function_, G transformer_, std::tuple<Ts...>& tpl_)
	{
		return detail::unpack_and_call_impl(function_, transformer_, std::make_index_sequence<sizeof...(Ts)> { }, tpl_);
	}

	template<class F, class G, class... Ts> auto unpack_and_call(F function_, G transformer_, const std::tuple<Ts...>& tpl_)
	{
		return detail::unpack_and_call_impl(function_, transformer_, std::make_index_sequence<sizeof...(Ts)> { }, tpl_);
	}


}