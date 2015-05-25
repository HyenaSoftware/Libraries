#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <typeindex>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <Utility\traits.h>
#include <Utility\meta.h>
#include <Utility\exception.hpp>

namespace std
{
	template<class T> std::weak_ptr<T> as_weak_ptr(std::shared_ptr<T> ptr_)
	{
		return std::weak_ptr<T>{std::move(ptr_)};
	}
}

namespace Utility
{
	template<class A> void swap_if_possible(A& a1_, A& a2_)
	{
		std::swap(a1_, a2_);
	}

	template<class A, class B> typename std::enable_if<!std::is_same<A, B>::value>::type swap_if_possible(A&, B&)
	{
		// no op
	}

	template<class F> void exec_for_each(F f_) { }

	template<class F, class T, class... Ts> void exec_for_each(F f_, T& t_, Ts&... ts_)
	{
		f_(t_);
		exec_for_each(f_, ts_...);
	}


	template<class T> using static_not = std::integral_constant<bool, !T::value>;
}

namespace reactive_framework5
{
	class edge;	// forward declaration

	class node
	{
	public:
		typedef int node_id;

		node() : _id{ reinterpret_cast<node_id>(this) } {}

		node_id id() const
		{
			return _id;
		}

		void set_id(node_id id_)
		{
			std::swap(_id, id_);
		}

		//virtual void add_output_edge(std::weak_ptr<edge> edge_) = 0;
		//virtual void add_input_edge(std::weak_ptr<edge> edge_) = 0;


	private:

		node_id _id;
	};

	define_has_method(add_output_edge);
	define_has_method(add_input_edge);

	class edge
	{
	public:
		typedef int edge_id;

		edge() : _id{ reinterpret_cast<edge_id>(this) } {}

		edge_id id() const
		{
			return _id;
		}

	private:
		edge_id _id;
	};

	class graph
	{
	public:
		typedef node::node_id node_id;
		typedef edge::edge_id edge_id;

		template<class SN, class E, class DN> void connect(std::shared_ptr<SN> src_n_, std::shared_ptr<E> edge_, std::shared_ptr<DN> dst_n_)
		{
			static_assert(std::is_base_of<node, SN>::value, "Type SN must be subtype of node");
			static_assert(std::is_base_of<node, DN>::value, "Type DN must be subtype of node");
			static_assert(std::is_base_of<edge, E>::value, "Type E must be subtype of edge");

			static_assert(has_method_add_input_edge<SN>::value, "");
			static_assert(has_method_add_output_edge<DN>::value, "");

			src_n_->add_output_edge(edge_);
			dst_n_->add_input_edge(edge_);

			edge_->set_input_node(src_n_);
			edge_->set_output_node(dst_n_);

			_nodes.insert(std::make_pair(src_n_->id(), src_n_));
			_nodes.insert(std::make_pair(dst_n_->id(), dst_n_));
			_edges.insert(std::make_pair(edge_->id(), edge_));
		}

	private:

		std::unordered_map<edge_id, std::weak_ptr<edge>> _edges;
		std::unordered_map<node_id, std::weak_ptr<node>> _nodes;

	};


	template<class R, class A> class typed_edge;

	template<class T> class typed_node : public node
	{
	public:
		typedef T value_type;

		std::vector<std::function<void(value_type)>> on_changed;

		template<class EDGE_OUT, class EDGE_IN> void add_output_edge(std::shared_ptr<typed_edge<EDGE_OUT, EDGE_IN>> edge_)
		{
			_output_slots[edge_] = [edge_](value_type& v_)
			{
				edge_->set(v_);
			};
		}

		template<class EDGE_OUT, class EDGE_IN> void add_input_edge(std::shared_ptr<typed_edge<EDGE_OUT, EDGE_IN>> edge_)
		{
			// unnecessary for us
		}

		/*
			it supports 'overwrite' kind of operations
		*/
		void set(value_type value_)
		{
			std::swap(_value, value_);

			_fire_on_changed();
		}

		/*
			this version of set supports 'modification' kind of operations
			which is allows to use e.g. containers (when only a few elements should be updated rather
			than the whole container)
		*/
		void set(std::function<void(value_type&)> lambda_)
		{
			lambda_(_value);

			_fire_on_changed();
		}

		value_type get() const
		{
			return _value;
		}
	private:
		void _fire_on_changed()
		{
			for (auto& func : on_changed)
			{
				func(_value);
			}

			for (auto& output_node : _output_slots)
			{
				output_node.second(_value);
			}
		}

		value_type _value;

		// store a 'strong' reference to the edge to keep it alive
		// add reference support
		std::unordered_map<std::shared_ptr<edge>, std::function<void(value_type&)>> _output_slots;
	};

	template<class R, class A> class typed_edge : public edge
	{
	public:
		typedef A input_value_type;
		typedef R output_value_type;

		template<class F> typed_edge(F f_) : _lambda{f_}
		{
		}

		template<class NODE_INPUT_TYPE> void set_input_node(std::shared_ptr<typed_node<NODE_INPUT_TYPE>> node_)
		{
			// no operation
		}

		template<class NODE_OUTPUT_TYPE> void set_output_node(std::shared_ptr<typed_node<NODE_OUTPUT_TYPE>> node_)
		{
			_target = node_;
		}

		void set(input_value_type value_)
		{
			Utility::throw_if(_target.expired(), "underlying node is not exists");
			Utility::throw_if(!static_cast<bool>(_lambda), "there is no lambda function");

			auto ptr = _target.lock();

			ptr->set(
				std::forward<output_value_type>(
					_lambda(
						std::forward<input_value_type>(value_))));
		}

	private:
		std::weak_ptr<typed_node<output_value_type>> _target;

		std::function<output_value_type(input_value_type)> _lambda;
	};


	template<class T, class I = std::string> class rv
	{
	public:
		typedef T value_type;

		std::vector<std::function<void(value_type)>> on_changed;

		rv& operator=(value_type value_)
		{
			std::swap(_value, value_);

			Utility::throw_if(!_underlying_node, "there is no underlying node");

			// it updates the underlying nodes
			//	the underlying node calls back the eventhandler of on_changed
			//	so it's not required to be called
			_underlying_node->set(_value);

			return *this;
		}

		operator value_type () const
		{
			return _value;
		}

		void bind_underlying_node(std::shared_ptr<typed_node<T>> underlying_node_)
		{
			std::swap(_underlying_node, underlying_node_);

			// it handles the data updating, which comes from the underlying node
			_underlying_node->on_changed.push_back([this](value_type value_)
			{
				std::swap(_value, value_);
				_fire_on_changed();
			});

		}
	private:
		value_type _value;

		std::shared_ptr<typed_node<T>> _underlying_node;

		void _fire_on_changed()
		{
			for (auto& func : on_changed)
			{
				func(_value);
			}
		}
	};


	struct undefined_type {};

	template<class SRC, class DST> class rv_builder;

	/*
		node	edge		node
		T		T& -> U		U

		template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE>
	*/
	template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE>
		class rv_context
	{
		template<class, class, class, class> friend class rv_context;
	public:
		typedef NODE_SRC_TYPE node_src_type;
		typedef NODE_DST_TYPE node_dst_type;

		typedef EDGE_SRC_TYPE edge_src_type;
		typedef EDGE_DST_TYPE edge_dst_type;

		rv_context(const rv_context&) = default;

		template<class NS, class ES, class ED, class ND> rv_context(rv_context<NS, ES, ED, ND> other_)
			: _graph{ other_._graph }
		{
			Utility::swap_if_possible(_src_node, other_._src_node);
			Utility::swap_if_possible(_dst_node, other_._dst_node);
			Utility::swap_if_possible(_edge, other_._edge);
			Utility::swap_if_possible(_ptr_src_rv, other_._ptr_src_rv);
			Utility::swap_if_possible(_ptr_dst_rv, other_._ptr_dst_rv);
		}

		rv_context(graph& graph_) : _graph{ graph_ }
		{
		}

		void set_src_node(std::shared_ptr<typed_node<node_src_type>> src_node_)
		{
			std::swap(_src_node, src_node_);
		}

		void set_dst_node(std::shared_ptr<typed_node<node_dst_type>> dst_node_)
		{
			std::swap(_src_node, src_node_);
		}

		void set_edge(std::shared_ptr<typed_edge<edge_dst_type, edge_src_type>> edge_)
		{
			std::swap(_edge, edge_);
		}

		void set_ptr_src_rc(rv<node_src_type>& rv_)
		{
			_ptr_src_rv = &rv_;
		}

		void set_ptr_dst_rc(rv<node_dst_type>& rv_)
		{
			_ptr_dst_rv = &rv_;
		}

		bool is_complete() const
		{
			return static_cast<bool>(_src_node)
				&& static_cast<bool>(_dst_node)
				&& static_cast<bool>(_edge)
				&& _ptr_src_rv != nullptr
				&& _ptr_dst_rv != nullptr;
		}

		bool try_apply()
		{
			bool completed = is_complete();
			if (completed)
			{
				_apply_impl(is_complete_type{});
			}

			return completed;
		}

		// shared_ptr is used here instead of weak_ptr to keep them alive during the
		//  graph path building to avoid a lot of boring reference checking
		std::shared_ptr<typed_node<node_src_type>> _src_node;
		std::shared_ptr<typed_node<node_dst_type>> _dst_node;
		std::shared_ptr<typed_edge<edge_dst_type, edge_src_type>> _edge;

		rv<node_src_type>* _ptr_src_rv = nullptr;
		rv<node_dst_type>* _ptr_dst_rv = nullptr;

		graph& get_graph()
		{
			return _graph;
		}

	protected:
		graph& _graph;

		typedef Utility::static_not<
			Utility::is_any_of<undefined_type, node_src_type, node_dst_type, edge_src_type, edge_dst_type>
		> is_complete_type;

		void _apply_impl(std::true_type)
		{
			// connect the nodes
			_graph.connect(_src_node, std::move(_edge), _dst_node);

			_ptr_src_rv->bind_underlying_node(std::move(_src_node));
			_ptr_dst_rv->bind_underlying_node(std::move(_dst_node));

			_ptr_src_rv = nullptr;
			_ptr_dst_rv = nullptr;
		}

		void _apply_impl(std::false_type)
		{
		}
	};

	template<class SRC, class DST> class builder_context
	{
	public:
		std::function<void()> src_node_binder;
		std::function<void()> dst_node_binder;

		bool is_complete() const
		{
			return src_node_binder && dst_node_binder;
		}

		void apply()
		{
			Utility::throw_if(!is_complete(), "incomplete");

			src_node_binder();
			dst_node_binder();
		}
	};

	template<class, class, class, class> class rv_node_builder;
	template<class, class, class, class> class rv_edge_builder;

	/*
	*/
	template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE>
		class rv_abstract_builder
	{
		//static_assert( ! std::is_reference<DST>::value, "reference type");
		//static_assert( ! std::is_reference<SRC>::value, "reference type");

		template<class, class, class, class> friend class rv_node_builder;
	public:
		typedef NODE_SRC_TYPE node_src_type;
		typedef NODE_DST_TYPE node_dst_type;

		typedef EDGE_SRC_TYPE edge_src_type;
		typedef EDGE_DST_TYPE edge_dst_type;

		rv_abstract_builder(rv_context<node_src_type, edge_src_type, edge_dst_type, node_dst_type> context_)
			: _context{ std::move(context_) }
		{
		}

		friend const rv_context<node_src_type, edge_src_type, edge_dst_type, node_dst_type>& context_of(rv_abstract_builder& rvb_)
		{
			return rvb_._context;
		}

	protected:
		rv_context<node_src_type, edge_src_type, edge_dst_type, node_dst_type> _context;
	};

	template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE>
		class rv_node_builder : public rv_abstract_builder<NODE_SRC_TYPE, EDGE_SRC_TYPE, EDGE_DST_TYPE, NODE_DST_TYPE>
	{
	public:
		using rv_abstract_builder::rv_abstract_builder;

		/*
			rv_node_builder<A, B>

			A -> B

			F : A -> B
		*/
		template<class F> auto map(F f_)
		{
			using namespace Utility;

			typedef typename function_traits<F>::result_type result_type;
			typedef typename function_traits<F>::arg<0>::type arg0_type;

			static_assert(!std::is_reference<arg0_type>::value, "reference type");

			static_assert(function_traits<F>::args_arity == 1, "");
			static_assert(std::is_convertible<node_src_type, arg0_type>::value, "Node source type must be convertible to edge source type.");
			static_assert(std::is_same<node_dst_type, undefined_type>::value, "Destionation type must be undefined yet.");

			throw_if(!_context._src_node, "source node is not specified");

			rv_context<node_src_type, arg0_type, result_type, undefined_type> context { _context };

			context._edge = std::make_shared<typed_edge<result_type, arg0_type>>(f_);

			rv_edge_builder<node_src_type, arg0_type, result_type, undefined_type> builder{ context };

			return builder;
		}

		template<class... NSs, class... ESs, class... EDs, class... NDs> void split(rv_abstract_builder<NSs, ESs, EDs, NDs>... abs_)
		{
			using namespace Utility;

			static_assert(is_all_of<undefined_type, NSs...>::value, "abstract builders must have source types");
			static_assert(is_all_of<undefined_type, ESs...>::value, "abstract builders must have source types");
			static_assert(is_all_of<undefined_type, EDs...>::value, "abstract builders must have source types");
			static_assert(!is_any_of<undefined_type, NDs...>::value, "abstract builders must have source types");

			int i = 0;

			exec_for_each([=, &i](auto& ra_)
			{
				typedef decltype(ra_._context)::node_dst_type sub_node_dst_type;

				rv_context<node_src_type, node_src_type, sub_node_dst_type, sub_node_dst_type> sub_context {ra_._context};

				sub_context._ptr_src_rv = _context._ptr_src_rv;
				sub_context._src_node = _context._src_node;

				int j = i;
				++i;

				sub_context._edge = std::make_shared<typed_edge<sub_node_dst_type, node_src_type>>(
					[j](node_src_type& vec_) -> sub_node_dst_type
					{
						return vec_.at(j);
					});

				sub_context.try_apply();

			}, abs_...);
		}
	};


	template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE>
		class rv_edge_builder : public rv_abstract_builder<NODE_SRC_TYPE, EDGE_SRC_TYPE, EDGE_DST_TYPE, NODE_DST_TYPE>
	{
		template<class, class, class, class> friend class rv_node_builder;
		template<class, class, class, class> friend class rv_edge_builder;
	public:
		template<class NS, class ES, class ED, class ND> rv_edge_builder(rv_context<NS, ES, ED, ND> context_)
			: rv_abstract_builder{ std::move(context_) }
		{
		}

		template<class new_node_dst_type> auto into(rv<new_node_dst_type>& rv_)
		{
			using namespace Utility;

			static_assert(std::is_same<node_dst_type, undefined_type>::value, "Previous destination node type must be undefined.");

			rv_context<node_src_type, edge_src_type, edge_dst_type, new_node_dst_type> context { _context };

			context._ptr_dst_rv = &rv_;
			context._dst_node = std::make_shared<typed_node<new_node_dst_type>>();

			context.try_apply();

			rv_abstract_builder<node_src_type, edge_src_type, edge_dst_type, new_node_dst_type> builder{ context };

			return builder;
		}

		auto from(rv<node_src_type>& rv_)
		{
			_context._src_node = std::make_shared<typed_node<source_type>>();
			_context._ptr_src_rv = &rv_;

			_context.try_apply();

			rv_node_builder<source_type, dest_type> builder{ _context };

			return builder;
		}

		template<class... NSs, class... ESs, class... EDs, class... NDs> void split(rv_abstract_builder<NSs, ESs, EDs, NDs>... abs_)
		{
			using namespace Utility;

			static_assert(is_all_of<undefined_type, NSs...>::value, "abstract builders must have undefined source types");
		}

	protected:
	};



	class reactive_context
	{
	public:
		template<class NS> auto from(rv<NS>& rv_)
		{
			rv_context<NS, undefined_type, undefined_type, undefined_type> context { _graph };

			context._ptr_src_rv = &rv_;
			context._src_node = std::make_shared<typed_node<NS>>();

			typedef rv_node_builder<NS, undefined_type, undefined_type, undefined_type> rv_node_builder_t;
			rv_node_builder_t builder { std::move(context) };

			static_assert(std::is_same<rv_node_builder_t::node_src_type, NS>::value, "");
			static_assert(std::is_same<rv_node_builder_t::node_dst_type, undefined_type>::value, "");

			return builder;
		}

		template<class T> auto into(rv<T>& rv_)
		{
			rv_context<undefined_type, undefined_type, undefined_type, T> context{ _graph };

			context._ptr_dst_rv = &rv_;
			context._dst_node = std::make_shared<typed_node<T>>();

			typedef rv_node_builder<undefined_type, undefined_type, undefined_type, T> rv_node_builder_t;

			rv_node_builder_t builder{ std::move(context) };

			static_assert(std::is_same<rv_node_builder_t::node_src_type, undefined_type>::value, "");
			static_assert(std::is_same<rv_node_builder_t::edge_src_type, undefined_type>::value, "");
			static_assert(std::is_same<rv_node_builder_t::edge_dst_type, undefined_type>::value, "");
			static_assert(std::is_same<rv_node_builder_t::node_dst_type, T>::value, "");

			return builder;
		}

		template<class F> auto map(F f_)
		{
			typedef typename Utility::function_traits<F>::result_type result_type;
			typedef typename Utility::function_traits<F>::arg<0>::type arg0_type;

			static_assert(Utility::function_traits<F>::args_arity == 1, "");

			//static_assert(!std::is_reference<arg0_type>::value, "reference type");

			rv_context<undefined_type, arg0_type, result_type, undefined_type> context { _graph };
				
			context._edge = std::make_shared<typed_edge<result_type, arg0_type>>(f_);

			rv_edge_builder<undefined_type, arg0_type, result_type, undefined_type> builder { std::move(context) };

			return builder;
		}

		/*
			make a wrapper:

			- edge_builder
				- edge_builder
				- edge_builder

			NSs : int
			ESs : int
			EDs : float
			NDs : vector<float>
		*/
		template<class... NSs, class... ESs, class... EDs, class... NDs> auto merge(rv_node_builder<NSs, ESs, EDs, NDs>... ebs_)
		{
			typedef typename Utility::first_of<NSs...>::type first_of_node_src_type;

			static_assert(Utility::is_all_of<undefined_type, ESs...>::value, "Destination node type must be undefined.");
			static_assert(Utility::is_all_of<undefined_type, EDs...>::value, "Destination node type must be undefined.");
			static_assert(Utility::is_all_of<undefined_type, NDs...>::value, "Destination node type must be undefined.");

			typedef std::vector<first_of_node_src_type> vec_of_first_of_node_dst_type;

			static_assert(Utility::is_same_as_all_of<first_of_node_src_type, NSs...>::value, "");

			//builder_context builder;
			rv_context<
				first_of_node_src_type,
				first_of_node_src_type&,
				std::vector<first_of_node_src_type>,
				std::vector<first_of_node_src_type>
			> context {_graph};

			Utility::exec_for_each([](auto& node_builder_)
			{
				context._ptr_src_rv = &

			}, ebs_...);

			rv_edge_builder<
				first_of_node_src_type,
				first_of_node_src_type&,
				std::vector<first_of_node_src_type>,
				std::vector<first_of_node_src_type>
			> builder{ std::move(context) };

			return builder;
		}
	private:
		graph _graph;
	};
}
