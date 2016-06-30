#pragma once
#include <iterator>
#include <functional>
#include <vector>
#include <boost/optional.hpp>
#include "traits.hpp"


define_has_operator(index, []);


namespace utility
{


	template<class T> class node
	{
	public:
		typedef T value_type;

		node(std::vector<value_type> vec_)
			: _vec { std::move(vec_) }	// it calls vector<object_ref> { initializer_list<object_ref> } instead of vector<object_ref> { vector<object_ref>&& }
		{
			//_vec.swap(vec_);
		}

		template<class IndexableType> auto map(IndexableType type_, std::enable_if_t<has_operator_index<IndexableType>::value>* = nullptr)
		{
			typedef std::remove_reference_t<decltype(type_[std::declval<value_type>()])> result_type;

			return map<result_type>([&](value_type v_)
			{
				return type_[v_];
			});
		}

		template<class U> node<U> map(std::function<U(value_type)> func_)
		{
			typedef U new_value_type;

			std::vector<new_value_type> ans;
			ans.reserve(_vec.size());

			for(auto& item : _vec)
			{
				ans.push_back(func_(item));
			}

			return ans;
		}

		template<class S> node<T> merge(S source_) const &
		{
			node<T> src = from(std::move(source_));

			return _merge(_vec, std::move(src._vec));
		}

		template<class S> node<T> merge(S source_) &&
		{
			node<T> src = from(source_);

			return _merge(std::move(_vec), std::move(src._vec));
		}

		value_type fold_left(std::function<value_type(value_type, value_type)> func_, value_type init_ = value_type{})
		{
			value_type acc { std::move(init_) };

			for(auto& item : _vec)
			{
				acc = func_(acc, item);
			}

			return acc;
		}

		node filter(std::function<bool(value_type)> pred_)
		{
			std::vector<value_type> ans;

			std::copy_if(_vec.begin(), _vec.end(), back_inserter(ans), pred_);

			return ans;
		}

		boost::optional<value_type> unique() const
		{
			bool all_the_same = all_of(_vec.begin(), _vec.end(), [this](auto e_)
			{
				return e_ == _vec.front();
			});

			if(all_the_same && !_vec.empty())
			{
				return _vec.front();					
			}
			
			return boost::none;
		}

		void for_each(std::function<void(value_type)> lambda_)
		{
			for(auto& e : _vec)
			{
				lambda_(e);
			}
		}

		operator std::vector<value_type>() &&
		{
			return std::move(_vec);
		}

		operator std::vector<value_type>() const &
		{
			return _vec;
		}

		template<template<class...> typename M = std::unordered_map> auto as_map() &&
		{
			typedef value_type::first_type key_type;
			typedef value_type::second_type mapped_type;

			M<key_type, mapped_type> ans;

			std::move(_vec.begin(), _vec.end(), std::inserter(ans, ans.end()));

			return ans;
		}

		auto as_map() const &
		{
			typedef value_type::key_type key_type;
			typedef value_type::mapped_type mapped_type;

			std::unordered_map<key_type, mapped_type> ans;

			std::copy(_vec.begin(), _vec.end(), ans.end());

			return ans;
		}

		auto as_vector() const & { return _vec; }
		auto as_vector() const && { return std::move(_vec); }

	private:
		std::vector<value_type> _vec;

		static std::vector<T> _merge(std::vector<T> v1_, std::vector<T> v2_)
		{
			v1_.insert(v1_.end(),
				std::make_move_iterator(v2_.begin()),
				std::make_move_iterator(v2_.end()));

			return v1_;
		}
	};



	//
	#define collection typename
	template<collection C> auto from(C collection_, std::enable_if_t<is_iterable<C>::value>* = nullptr)
	{
		return from(std::make_pair(
			std::make_move_iterator(collection_.begin()),
			std::make_move_iterator(collection_.end())
		));
	}

	template<class T> auto from(std::vector<T> vec_)
	{
		return node<T> { std::move(vec_) };
	}

	//
	//
	//
	template<class T> struct make_moveable
	{
		typedef T type;
	};

	template<class A, class B> struct make_moveable<std::pair<const A, B>>
	{
		typedef std::pair<A, B> type;
	};

	template<class T> using make_moveable_t = typename make_moveable<T>::type;


	template<class I> auto from(std::pair<I, I> its_)
	{
		typedef typename I::value_type value_type;
		typedef make_moveable_t<value_type> moveable_value_type;

		return node<moveable_value_type> { std::vector<moveable_value_type> { its_.first, its_.second } };
	}
}
