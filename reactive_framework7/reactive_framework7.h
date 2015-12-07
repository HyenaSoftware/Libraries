#pragma once
#include "stdafx.h"
#include "value_holders"
#include "operators"

template<class T, class... Ts, class... Us, class I> auto unpack(I beg_, I end_, std::tuple<T, Ts...>, Us&&... us_)
{
	auto next = beg_;

	return unpack<>(++next, end_, std::tuple<Ts...> { }, std::forward<Us>(us_)..., static_cast<T>(*beg_));
}

template<class... Us, class I> auto unpack(I beg_, I end_, std::tuple<>, Us&&... us_)
{
	return std::make_tuple(std::forward<Us>(us_)...);
}


template<class... Ts, class I> std::tuple<Ts...> to_tuple(I beg_, I end_)
{
	return unpack(beg_, end_, std::tuple<Ts...>{});
}




namespace reactive_framework7
{
	//
	struct undefined_type {};
	

	class unnassigned_reactive_variable_exception : public std::runtime_error
	{
	public:
		unnassigned_reactive_variable_exception(const char* str_)
			: std::runtime_error { str_ }
		{
		}
	};


	template<class I> class rv_builder;
	template<class T, class I> class rv;
	class abstract_reactive_context;

	typedef size_t rv_internal_id_t;
	typedef utility::graph computation_graph_t;

	struct irv
	{
		virtual std::shared_ptr<detail::inode> value_holder() const = 0;
	};


	template<class T, class  I = int> class rv : private irv
	{
		template<class U, class I> friend class rv;
	public:
		typedef T value_type;
		typedef I id_t;

		utility::event<value_type> on_changed;

		//rv()
		//	: _ptr_value_holder{ std::make_shared<detail::value_holder<value_type>>(reactive_context::instance()) }
		//{
		//	_reset_underlying_callback();
		//}
		rv(abstract_reactive_context& rc_, value_type val_)
			: _ptr_value_holder{ std::make_shared<detail::value_holder<value_type>>(rc_) }
			, _rc{ rc_ }
		{
			_reset_underlying_callback();

			_ptr_value_holder->set_value(val_);
			_rc.on_value_holder_changed(*_ptr_value_holder);
		}

		//
		//	shallow copy?
		//
		rv(const rv& other_)
			: _rc{ other_._rc }
			//, _ptr_value_holder{ std::make_shared<detail::value_holder<value_type>>(*other_._ptr_value_holder) }
			, _ptr_value_holder{ other_._ptr_value_holder }
		{
			_reset_underlying_callback();
		}

		rv(rv&& other_)
			: _rc{ other_._rc }
			, _ptr_value_holder{ std::make_shared<detail::value_holder<value_type>>(other_._rc) }
		{
			swap(other_);
		}
		
		rv(abstract_reactive_context& rc_)
			: _ptr_value_holder{ std::make_shared<detail::value_holder<value_type>>(rc_) }
			, _rc { rc_ }
		{
			_reset_underlying_callback();
		}

		~rv()
		{
			_ptr_value_holder->on_changed.erase(FUNC_ID_UNDERLYING_VALUE_UPDATED);

			//_rc.release_rv(*this);
		}

		rv& operator=(rv rv_)
		{
			swap(rv_);

			return *this;
		}

		rv& operator=(value_type value_)
		{
			_ptr_value_holder->set_value(value_);
			_rc.on_value_holder_changed(*_ptr_value_holder);

			return *this;
		}

		void set_value(std::function<value_type(value_type)> lambda_)
		{
			_ptr_value_holder->set_value(lambda_);
			_rc.on_value_holder_changed(*_ptr_value_holder);
		}

		//
		// it overwrites EVERYTHING in the target object (event handlers, etc...)
		//
		void swap(rv& other_)
		{
			std::swap(_ptr_value_holder, other_._ptr_value_holder);
			std::swap(on_changed, other_.on_changed);

			// the underlying value still tries to update the old rv object...
			_reset_underlying_callback();
			other_._reset_underlying_callback();

			// underlying value has been changed so let's update who subscribed to the events of this object
			_fire();
			other_._fire();
		}

		operator value_type() const
		{
			return value();
		}

		value_type value() const
		{
 			return _ptr_value_holder->value();
		}

		//

		void assign_to(abstract_reactive_context& rc_)
		{
			_ptr_rc = &rc_;
		}

		abstract_reactive_context* rc() const
		{
			return _ptr_rc;
		}

		//
		template<class F> auto map(F mapper_)
		{
			using namespace Utility;

			return map(as_std_function(mapper_));
		}

		template<class U> rv<U, id_t> map(std::function<U(value_type)> mapper_)
		{
			using namespace detail;

			rv<U, id_t> new_rv { _rc };

			auto ptr_op = std::make_shared<rv_map_operator<U, value_type>>(_rc, new_rv._ptr_value_holder, _ptr_value_holder.get(), mapper_);

			_rc.regist_rv_edge(ptr_op, { value_holder() });
			_rc.regist_rv_edge(new_rv.value_holder(), { ptr_op });

			return new_rv;
		}

		template<class U, class... As> rv<U, id_t> zip_with(std::function<U(value_type, As...)> mapper_, rv<As, id_t>&... rvs_)
		{
			using namespace detail;

			rv<U, id_t> new_rv { _rc };

			auto ptr = std::make_shared<rv_zip_operator<U, value_type, As...>>(_rc, new_rv._ptr_value_holder, mapper_, _ptr_value_holder.get(), rvs_._ptr_value_holder.get()...);

			_rc.regist_rv_edge(ptr, { value_holder(), rvs_.value_holder()... });
			_rc.regist_rv_edge(new_rv.value_holder(), { ptr });
			
			return new_rv;
		}


		// TODO:
		// what's is the semantic?
		//
		//
		void into(rv<value_type, id_t>& other_rv_) &
		{
			_rc.regist_rv_edge(other_rv_.value_holder(), { value_holder() });

			// other_rv <- other_rv.value_holder
			other_rv_._reset_underlying_callback();

			// underlying value has been changed so let's update who subscribed to the events of this object
			other_rv_._fire();
		}

		void into(rv<value_type, id_t>& other_rv_) && { share(other_rv_); }

		void share(rv<value_type, id_t>& other_rv_)
		{
			using namespace detail;

			_rc.copy_setup(value_holder(), other_rv_.value_holder());

			//
			_reset_underlying_callback();
			other_rv_._reset_underlying_callback();

			// underlying value has been changed so let's update who subscribed to the events of this object
			other_rv_._fire();
		}

	private:
		enum FUNC_ID
		{
			FUNC_ID_UNDERLYING_VALUE_UPDATED = 0
		};

		abstract_reactive_context& _rc;

		//
		//	* theoretically it never ever can be null for an rv
		//	* in practice move-ctor can have it as null
		//	so if the move-ctor calls, swap or any other methods which work with this field
		//	must do a null-check to avoid memory access violation 
		//
		std::shared_ptr<detail::value_holder<value_type>> _ptr_value_holder;

		std::shared_ptr<detail::inode> value_holder() const override
		{
			return _ptr_value_holder;
		}

		void _reset_underlying_callback()
		{
			if(_ptr_value_holder)
			{
				_ptr_value_holder->on_changed[FUNC_ID_UNDERLYING_VALUE_UPDATED] = [this]
				{
					on_changed(value());
				};
			}
		}

		void _fire()
		{
			on_changed(value());
		}
	};

	
	struct reactive_context_exec_policy_tag { };
	

	class limit_single_chain_execution_policy : reactive_context_exec_policy_tag
	{
	public:
		limit_single_chain_execution_policy(detail::ireactive_context& rc_);

		bool should_propagate(detail::inode& source_);
		void update_node_from(detail::inode& target_, detail::inode& source_);

		int max_single_chain_recompute() const;
		void set_max_single_chain_recompute(int);

	private:
		std::unordered_map<int, int> _node_id_to_limit;
		int _single_chain_recompute_limit = 1;

		detail::ireactive_context& _rc;
	};


	class multi_threaded_exec_policy : reactive_context_exec_policy_tag
	{
	public:
		bool should_propagate(detail::inode& source_);
		void update_node_from(detail::inode& target_, detail::inode& source_);

	private:
		utility::thread_pool _thread_pool{ 10 };
	};


	class interruptable_chain_execution_policy : reactive_context_exec_policy_tag
	{
	public:
		bool should_propagate(detail::inode& source_);
		void update_node_from(detail::inode& target_, detail::inode& source_);
	};



	class abstract_reactive_context : public detail::ireactive_context
	{
	public:
		abstract_reactive_context();

		//
		//	notification about rv_ has a new value no
		//
		void update(const irv& rv_);

		void regist_rv_edge(std::shared_ptr<detail::inode> dst_, std::initializer_list<std::shared_ptr<detail::inode>> srcs_);

		void copy_setup(std::shared_ptr<detail::inode> rv_from_, std::shared_ptr<detail::inode> rv_to_);

		void on_value_holder_changed(detail::inode& src_node_);

		computation_graph_t::vertex_id_t vertex_id_of(const detail::inode& holder_);

		void release_rv(const detail::inode&) override;

	protected:
		virtual void _update_node_from(detail::inode& target_, detail::inode& source_) = 0;
		virtual bool _should_propagate(detail::inode& source_) = 0;

	private:
		abstract_reactive_context(const abstract_reactive_context&) = delete;
		abstract_reactive_context(abstract_reactive_context&&) = delete;

		computation_graph_t _graph;

		std::unordered_map<computation_graph_t::vertex_id_t,	std::shared_ptr<detail::inode>> _vertex_data;
	};

	template<class P> class reactive_context : public abstract_reactive_context
	{
		static_assert(std::is_base_of<reactive_context_exec_policy_tag, P>::value, HERE"it's not a policy");

	public:
		typedef P policy_t;

		policy_t& policy()
		{
			return _policy;
		}

	private:
		policy_t _policy { *this };

		void _update_node_from(detail::inode& target_, detail::inode& source_) override
		{
			_policy.update_node_from(target_, source_);
		}

		bool _should_propagate(detail::inode& source_) override
		{
			return _policy.should_propagate(source_);
		}
	};

	typedef reactive_context<limit_single_chain_execution_policy> default_reactive_context;



	template<class F, class I, class... As> auto zip(F mapper_, rv<As, I>&... rvs_)
	{
		using namespace Utility;

		typedef typename function_traits<F>::result_type result_type;

		static_assert(!std::is_same<result_type, void>::value, "mapper return type can't be void");

		return zip<result_type>(std::function<result_type(As...)>{ mapper_ }, rvs_...);
	}

	template<class R, class I, class A1, class... As> rv<R, I> zip(std::function<R(A1, As...)> mapper_, rv<A1, I>& rv_, rv<As, I>&... rvs_)
	{
		return rv_.zip_with(mapper_, rvs_...);
	}

	template<class T, class I> rv_builder<I> from(rv<T, I>& src_)
	{
		rv_builder<I> builder;

		builder.from(src_);

		return builder;
	}

	template<class T, class I> rv_builder<I> merge(std::initializer_list<std::reference_wrapper<rv<T, I>>> src_, int from_ = 0)
	{
		rv_builder<I> builder;

		return builder;
	}

	template<class... Ts, class I> rv_builder<I> join(rv<Ts, I>&... src_)
	{
		rv_builder<I> builder;

		return builder;
	}
}
