#pragma once
#include "stdafx.h"

namespace reactive_framework8
{
	enum class E_DEBUGGER_EVENT
	{
		NODE_VALUE_CHANGED
	};

	extern const std::unordered_map<E_DEBUGGER_EVENT, std::string> DEBUGGER_EVENT_TO_STRING;


	template<class T> std::string generate_name(T& t_)
	{
		std::stringstream sb;
		sb << hex << "[0x" << &t_ << "][" << typeid(t_).name() << "]";

		return sb.str();
	}

	template<class T> class rv;
	template<class T> class underlying_rv;
	class rv_abstract_operator;


	class rv_debugger
	{
	public:
		~rv_debugger();

		template<class T> void notify(E_DEBUGGER_EVENT event_, underlying_rv<T>& urv_, boost::optional<T> value_)
		{
			std::stringstream sb;

			if(value_)
			{
				sb << *value_;
			}
			else
			{
				sb << "[null]";
			}

			notify(event_, name_of(urv_), sb.str());
		}

		void notify(E_DEBUGGER_EVENT event_, std::string rv_name_, std::string value_);

		template<class T> void add_edge(underlying_rv<T>& urv_, rv_abstract_operator& op_)
		{
			std::lock_guard<std::mutex> l { _mtx_print };
			std::cout << "new edge: " << name_of(urv_) << " -> " << typeid(op_).name() << endl;
		}

		template<class T> void add_edge(rv_abstract_operator& op_, underlying_rv<T>& urv_)
		{
			std::lock_guard<std::mutex> l{ _mtx_print };
			std::cout << "new edge: " << typeid(op_).name() << " -> " << name_of(urv_) << endl;
		}

		template<class T> void set_name(rv<T>& rv_, std::string name_)
		{
			set_name(rv_._ptr_impl.get(), std::move(name_));
		}

		template<class T> std::string name_of(rv<T>& rv_)
		{
			return name_of(rv_._ptr_impl);
		}

		template<class T> std::string name_of(underlying_rv<T>& urv_)
		{
			auto it = _urv_to_name.find(&urv_);

			if(it == _urv_to_name.end())
			{
				std::tie(it, std::ignore) = _urv_to_name.insert({&urv_, generate_name(urv_)});
			}

			return it->second;
		}

	private:
		std::mutex _mtx_print;
		std::unordered_map<void*, std::string> _urv_to_name;

		void set_name(void*, std::string name_);
	};
}
