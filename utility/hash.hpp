#pragma once
#include <tuple>
#include <algorithm>
#include <memory>
#include <typeindex>
#include "types.hpp"

namespace std
{
	template<> struct hash<tuple<char, utility::byte>>
		: unary_function<tuple<char, utility::byte>, size_t>
	{
		size_t operator()(const tuple<char, utility::byte>& tpl_) const
		{
			size_t hash_code = get<0>(tpl_) << 8 | get<1>(tpl_);
			return hash_code;
		}
	};

	template<class A, class B> struct hash<pair<A, B>>
	{
		size_t operator()(const pair<A, B>& pair_) const
		{
			return hash<A>{}(pair_.first) ^ hash<B>{}(pair_.second);
		}
	};

	template<class T> struct hash<weak_ptr<T>>
	{
		size_t operator()(const weak_ptr<T>& ptr_) const
		{
			return reinterpret_cast<size_t>(ptr_._Get());
		}
	};

	template<class T> struct equal_to<weak_ptr<T>>
	{
		bool operator()(const weak_ptr<T>& a_, const weak_ptr<T>& b_) const
		{
			return a_._Get() == b_._Get();
		}
	};

	template<class H, class... Ts> struct hash<tuple<H, Ts...>> 
		: unary_function<tuple<H, Ts...>, size_t>
	{
			size_t operator()(const tuple<H, Ts...>& tpl_) const
			{
				typedef std::index_sequence_for<Ts...>::type seq_t;

				return hash<H>{}(get<0>(tpl_)) ^ _apply_tail(tpl_, seq_t{});
			}

			template<size_t... Ns> size_t _apply_tail(const tuple<H, Ts...>& tpl_, std::index_sequence<Ns...>) const
			{
				return hash<tuple<Ts...>>{}(make_tuple(get<1 + Ns>(tpl_)...));
			}
	};

	//
	template<class T> struct hash<tuple<T>> : unary_function<tuple<T>, size_t>
	{
		size_t operator()(const tuple<T>& tpl_) const
		{
			return hash<T>{}(get<0>(tpl_));
		}
	};


	// hash for std::vector
	// 
	template<class T> struct hash<vector<T>> : unary_function<vector<T>, size_t>
	{
		size_t operator()(const vector<T>& vec_) const
		{
			size_t h = 0;

			hash<T> hasher;

			for(auto& e : vec_)
			{
				h ^= hasher(e);
			}

			return h;
		}
	};

	//
	//
	/*
	template<> struct hash<boost::any> : unary_function<boost::any, size_t>
	{
		size_t operator()(const boost::any& any_) const
		{
			return any_.type().hash_code();
		}
	};
	*/
}
