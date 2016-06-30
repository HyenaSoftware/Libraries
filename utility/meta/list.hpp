#pragma once
#include <tuple>

namespace utility
{
	template<class...> struct list;

	template<class T1, class... Ts> struct list<T1, Ts...>
	{
		T1 head;
		list<Ts...> tail;
	};

	template<class T> struct list<T> { T head; };

	/*
		Map format:
			std::tuple<std::pair<key_type1, value_type1>, std::pair<key_type2, value_type2>, ...>
	*/

	namespace collections
	{
		template<class, class> struct get;

		template<class E1, class... Es, class K> struct get<std::tuple<E1, Es...>, K>
		{
			typedef typename get<std::tuple<Es...>, K>::type type;
		};

		template<class K, class V, class... Es> struct get<std::tuple<std::pair<K, V>, Es...>, K>
		{
			typedef V type;
		};

		template<class K> struct get<std::tuple<>, K>
		{
			typedef void type;
		};
	
		namespace
		{
			template<class E, class K, class V> struct set_if
			{
				typedef E type;
				typedef std::false_type succeeded;
			};
			template<class K, class V, class W> struct set_if<std::pair<K, V>, K, W>
			{
				typedef std::pair<K, W> type;
				typedef std::true_type succeeded;
			};

			template<class M, class M2, class K, class V, bool> struct set_iterator;
			template<class A1, class... As, class... Bs, class K, class V, bool B>
				struct set_iterator<std::tuple<A1, As...>, std::tuple<Bs...>, K, V, B>
			{
				typedef set_if<A1, K, V> return_value;

				typedef typename set_iterator<
					std::tuple<As...>,
					std::tuple<typename return_value::type, Bs...>,
					K,
					V,
					B || return_value::succeeded::value
				>::type type;
			};

			template<class... Bs, class K, class V> struct set_iterator<std::tuple<>, std::tuple<Bs...>, K, V, true>
			{
				typedef std::tuple<Bs...> type;
			};
			template<class... Bs, class K, class V> struct set_iterator<std::tuple<>, std::tuple<Bs...>, K, V, false>
			{
				typedef std::tuple<Bs..., std::pair<K, V>> type;
			};
		}

		template<class M, class K, class V> struct set
		{
			typedef typename set_iterator<M, std::tuple<>, K, V, false>::type type;
		};
	}
}
