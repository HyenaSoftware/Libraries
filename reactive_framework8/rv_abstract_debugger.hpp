#pragma once
#include "stdafx.h"

namespace reactive_framework8
{
	namespace mixins
	{
		template<class R> struct output;
	}

	template<class T> class rv;
	//template<class T> class graph::node;
	namespace graph
	{
		template<class T> class node;
	}

	//class rv_abstract_operator;
	//template<class R> class rv_abstract_operator_with_return_type;


	class rv_abstract_debugger
	{
	public:
		rv_abstract_debugger() = default;
		virtual ~rv_abstract_debugger() = default;

		//	Node operations
		//
		template<class T> void notify_value_change(graph::node<T>& urv_, boost::optional<T> value_)
		{
			using namespace utility;
			std::stringstream sb;

			if (value_)
			{
				sb << *value_;
			}
			else
			{
				sb << "[null]";
			}

			notify_value_change(name_of(urv_), sb.str());
		}

		virtual void notify_value_change(std::string rv_name_, std::string value_) = 0;

		template<class T> void notify_rv_assigned_to(graph::node<T>& urv_)
		{
			notify_rv_assigned_to(name_of(urv_));
		}

		virtual void notify_rv_assigned_to(std::string rv_name_) = 0;

		// operator operations
		//
		template<class T> void notify_new_operator(graph::node<T>& operator_)
		{
			notify_new_operator(name_of(&operator_, typeid(T)));
		}

		virtual void notify_new_operator(std::string) = 0;

		//	Edge registration
		//
		template<class T, class R> void add_edge_from_value(graph::node<T>& value_node_, graph::node<R>& op_node_)
		{
			add_edge_from(&value_node_, typeid(T), &op_node_, typeid(R));
		}

		template<class T, class R> void add_edge_to_value(graph::node<R>& op_node_, graph::node<T>& value_node_)
		{
			add_edge_to(&op_node_, typeid(R), &value_node_, typeid(T));
		}

		virtual void add_edge_from(void* node_ptr_, std::type_index node_type_, void* operator_ptr_, std::type_index operator_type_) = 0;

		virtual void add_edge_to(void* operator_ptr_, std::type_index operator_type_, void* node_ptr_, std::type_index node_type_) = 0;

		//	basic stuffs
		//
		template<class T> void set_name(rv<T>& rv_, std::string name_)
		{
			set_name(rv_._ptr_impl.get(), std::move(name_));
		}

		void set_name(void*, std::string name_);

		//	Auxiliary functions
		//
		template<class T> std::string name_of(rv<T>& rv_)
		{
			return name_of(rv_._ptr_impl.get(), typeid(T));
		}

		template<class T> std::string name_of(graph::node<T>& urv_)
		{
			return name_of(&urv_, typeid(T));
		}

	protected:
		std::string name_of(void* ptr_, std::type_index ti_);
		
	private:
		std::unordered_map<void*, std::string> _urv_to_name;

		std::string generate_name(void* ptr_, std::type_index ti_);
	};


}
