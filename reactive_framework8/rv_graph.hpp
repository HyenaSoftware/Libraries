#pragma once
#include "stdafx.h"

namespace reactive_framework8
{
	template<class T> class rv;
	class rv_context;

	namespace graph
	{
		struct inotifiable
		{
			virtual void invalidate() = 0;
		};

		template<class T> class node : public inotifiable
		{
		public:
			node() = default;

			node(rv<T>& host_rv_)
			{
				add_owner(host_rv_);
			}

			const boost::optional<T>& value() const
			{
				return _value;
			}

			void set_value(T value_)
			{
				_value = value_;

				notify();

				// debugger
				if (_ptr_rc)
				{
					_ptr_rc->debugger().notify_value_change(*this, _value);


					for (auto& trg : _targets)
					{
						auto target = trg.lock();
						if (target)
						{
							_ptr_rc->submit([=]
							{
								target->invalidate();
							});
						}
					}
				}
			}

			void invalidate() override final
			{
				auto new_val = _re_calc();
				if (new_val)
				{
					set_value(*new_val);
				}
			}

			void assign(std::weak_ptr<inotifiable> target_)
			{
				_targets.push_back(std::move(target_));
			}

			void assign(rv_context& rc_)
			{
				_ptr_rc = &rc_;
			}

			void add_owner(rv<T>& new_owner_rv_)
			{
				_host_rvs.insert(&new_owner_rv_);
			}

			void remove_owner(rv<T>& owner_rv_)
			{
				_host_rvs.erase(&owner_rv_);
			}

		protected:
			virtual void notify() { }
			std::unordered_set<rv<T>*> _host_rvs;

		private:
			boost::optional<T> _value;
			std::vector<std::weak_ptr<inotifiable>> _targets;
			rv_context* _ptr_rc = nullptr;

			virtual boost::optional<T> _re_calc() const = 0;
		};

		template<class T> class value_node final : public node<T>
		{
		public:
			value_node(rv<T>& host_rv_)
				: node{ host_rv_ }
			{
			}

			value_node(const value_node&) = default;

			void set_source(node<T>& src_node_)
			{
				_source = &src_node_;
			}

		private:
			node<T>* _source = nullptr;

			boost::optional<T> _re_calc() const override
			{
				if(_source)
				{
					return _source->value();
				}

				return value();
			}

			void notify() override
			{
				for (auto ptr_host : _host_rvs)
				{
					ptr_host->notify();
				}
			}
		};

		template<class F, class T> class operator_node;
		template<class R, class... As, class T> class operator_node<R(As...), T> : public node<T>
		{
			static constexpr std::index_sequence_for<As...> SEQ { };
		
		protected:
			operator_node(std::function<R(As...)> func_, std::tuple<std::weak_ptr<node<As>>...> sources_)
				: _func { std::move(func_) }
				, _sources { std::move(sources_) }
			{
			}

			R call_underlying_func(std::tuple<As...>& args_) const
			{
				return _call_impl(args_, SEQ);
			}

			boost::optional<std::tuple<As...>> get_source_args() const
			{
				return _args(SEQ);
			}

		private:
			std::function<R(As...)> _func;
			std::tuple<std::weak_ptr<node<As>>...> _sources;

			template<size_t... Ns> boost::optional<std::tuple<As...>> _args(std::index_sequence<Ns...>) const
			{
				using namespace utility;

				auto locked_inputs = std::make_tuple(std::get<Ns>(_sources).lock() ...);

				bool any_expired = or(std::get<Ns>(locked_inputs) == nullptr...);

				bool all_valid = !any_expired && and(std::get<Ns>(locked_inputs)->value().is_initialized()...);

				if (!all_valid) return { };

				return std::make_tuple(std::get<Ns>(locked_inputs)->value().get()...);
			}

			template<size_t... Ns> R _call_impl(std::tuple<As...>& args_, std::index_sequence<Ns...>) const
			{
				return _func(std::get<Ns>(args_) ...);
			}
		};

		template<class R, class... As> class map_operator_node : public operator_node<R(As...), R>
		{
		public:
			map_operator_node(std::function<R(As...)> func_, std::tuple<std::weak_ptr<node<As>>...> sources_)
				: operator_node { std::move(func_), std::move(sources_) }
			{
			}
		private:
			boost::optional<R> _re_calc() const override
			{
				auto args = get_source_args();
				if(args)
				{
					return call_underlying_func(*args);
				}

				return { };
			}
		};

		template<class T> class filter_operator_node : public operator_node<bool(T), T>
		{
		public:
			filter_operator_node(std::function<bool(T)> func_, std::weak_ptr<node<T>> source_)
				: operator_node{ std::move(func_), std::make_tuple(source_) }
			{

			}

		private:
			boost::optional<T> _re_calc() const override
			{
				auto opt_args = get_source_args();
				if (opt_args && call_underlying_func(*opt_args))
				{
					return std::get<0>(*opt_args);
				}

				return { };
			}
		};
	}
}
