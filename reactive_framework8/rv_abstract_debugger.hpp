#pragma once
#include "stdafx.h"

namespace reactive_framework8
{
	enum class E_DEBUGGER_EVENT
	{
		NODE_VALUE_CHANGED
	};

	extern const std::unordered_map<E_DEBUGGER_EVENT, std::string> DEBUGGER_EVENT_TO_STRING;


	template<class T> class rv;
	template<class T> class underlying_rv;
	class rv_abstract_operator;


	class rv_abstract_debugger
	{
	public:
		rv_abstract_debugger() = default;
		virtual ~rv_abstract_debugger() = default;


		template<class T> void notify(E_DEBUGGER_EVENT event_, underlying_rv<T>& urv_, boost::optional<T> value_)
		{
			std::stringstream sb;

			if (value_)
			{
				sb << *value_;
			}
			else
			{
				sb << "[null]";
			}

			notify(event_, name_of(urv_), sb.str());
		}

		virtual void notify(E_DEBUGGER_EVENT event_, std::string rv_name_, std::string value_) = 0;


		template<class T> void add_edge(underlying_rv<T>& urv_, rv_abstract_operator& op_)
		{
			add_edge(&urv_, typeid(urv_), op_);
		}

		template<class T> void add_edge(rv_abstract_operator& op_, underlying_rv<T>& urv_)
		{
			add_edge(op_, &urv_, typeid(urv_));
		}

		virtual void add_edge(void* ptr_, std::type_index, rv_abstract_operator& op_) = 0;

		virtual void add_edge(rv_abstract_operator& op_, void* ptr_, std::type_index) = 0;

		template<class T> void set_name(rv<T>& rv_, std::string name_)
		{
			set_name(rv_._ptr_impl.get(), std::move(name_));
		}

		template<class T> std::string name_of(rv<T>& rv_)
		{
			return name_of(rv_._ptr_impl.get(), typeid(*rv_._ptr_impl));
		}

		template<class T> std::string name_of(underlying_rv<T>& urv_)
		{
			return name_of(&urv_, typeid(urv_));
		}

		void set_name(void*, std::string name_);

	protected:
		std::string name_of(void* ptr_, std::type_index ti_);
		
	private:
		std::unordered_map<void*, std::string> _urv_to_name;

		std::string generate_name(void* ptr_, std::type_index ti_);
	};


}
