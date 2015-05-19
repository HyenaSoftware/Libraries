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

		template<class EDGE_OUT> void add_output_edge(std::shared_ptr<typed_edge<EDGE_OUT, value_type>> edge_)
		{
			auto ptr = edge_.get();
			auto f = std::bind(&typed_edge<EDGE_OUT, value_type>::set, ptr, std::placeholders::_1);
			_output_slots[ptr] = f;
		}

		template<class EDGE_IN> void add_input_edge(std::shared_ptr<typed_edge<value_type, EDGE_IN>> edge_)
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

		std::unordered_map<const edge*, std::function<void(value_type)>> _output_slots;
	};

	template<class R, class A> class typed_edge : public edge
	{
	public:
		typedef A input_value_type;
		typedef R output_value_type;

		void set_input_node(std::shared_ptr<typed_node<input_value_type>> node_)
		{
			// no operation
		}

		void set_output_node(std::shared_ptr<typed_node<output_value_type>> node_)
		{
			_target = node_;
		}

		void set(input_value_type value_)
		{
			_target.lock()->set(std::move(value_));
		}

	private:
		std::weak_ptr<typed_node<output_value_type>> _target;
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

	template<class T> class rv_binding
	{
	public:
		typedef T value_type;

		rv_binding() = default;

		rv_binding& operator << (std::shared_ptr<typed_node<value_type>> node_ptr_)
		{
			std::swap(_node_ptr, node_ptr_);

			return *this;
		}

		rv_binding& operator << (rv<value_type>& rv_)
		{
			_rv = &rv_;

			return *this;
		}

		void apply_if_complete()
		{
			if (_rv && _node_ptr)
			{
				apply_binding();
			}
		}

		void apply_binding()
		{
			_rv->bind_underlying_node(std::move(_node_ptr));
			_rv = nullptr;
		}

	private:
		std::shared_ptr<typed_node<value_type>> _node_ptr;
		rv<value_type>* _rv = nullptr;
	};

	template<class SRC, class DST> class rv_builder
	{
		template<class U1, class U3> friend class rv_builder;
	public:
		typedef SRC source_type;
		typedef DST dest_type;

		rv_builder(graph& graph_) : _graph{ graph_ }
		{
		}

		template<class I> void to(rv<DST, I>& rv_)
		{
			auto dst_node = std::make_shared<typed_node<DST>>();

			_proxy_of_src_rv.set_node(_src_node);

			_graph.connect(
				std::as_weak_ptr(std::move(_src_node)),
				std::as_weak_ptr(std::move(_edge)),
				std::as_weak_ptr(std::move(dst_node))
				);
		}

		template<class F> auto map(F f_)
		{
			typedef Utility::function_traits<F>::result_type result_type;
			typedef Utility::function_traits<F>::arg<0>::type arg_type;

			static_assert(std::is_same<arg_type, SRC>::value, "arg of F and SRC must be the same");

			rv_builder<SRC, result_type> builder{ _graph };

			auto new_edge = std::make_shared<typed_edge<result_type, SRC>>();

			std::swap(builder._proxy_of_src_rv, _proxy_of_src_rv);
			std::swap(builder._src_node, _src_node);
			std::swap(builder._edge, new_edge);

			return builder;
		}

	protected:
		graph& _graph;

		// shared_ptr is used here instead of weak_ptr to keep them alive during the
		//  graph path building to avoid a lot of boring reference checking
		std::shared_ptr<typed_node<SRC>> _src_node;
		std::shared_ptr<typed_edge<DST, SRC>> _edge;

		rv_binding<SRC> _src_binder;
		rv_binding<DST> _dst_binder;
	};

	template<class, class> class rv_node_builder;
	template<class, class> class rv_edge_builder;

	template<class SRC, class DST> class rv_node_builder : public rv_builder<SRC, undefined_type>
	{
	public:
		rv_node_builder(graph& graph_, rv<SRC>& rv_) : rv_builder{ graph_ }
		{
			_src_node = std::make_shared<typed_node<SRC>>();
			_src_binder << _src_node << rv_;
		}

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

			static_assert(function_traits<F>::args_arity == 1, "");
			static_assert(std::is_same<arg0_type, source_type>::value, "");

			rv_edge_builder<arg0_type, result_type> builder{ _graph };
			std::swap(builder._dst_binder, _dst_binder);
			std::swap(builder._src_binder, _src_binder);
			std::swap(builder._src_node, _src_node);
			std::swap(builder._edge, _edge);

			return builder;
		}
	};

	template<class SRC, class DST> class rv_edge_builder : public rv_builder<SRC, DST>
	{
		template<class, class> friend class rv_node_builder;
		template<class, class> friend class rv_edge_builder;
	public:
		typedef SRC source_type;
		typedef DST dest_type;

		rv_edge_builder(graph& graph_) : rv_builder{ graph_ }
		{
		}

		rv_edge_builder(graph& graph_, std::shared_ptr<typed_node<source_type>> node_) : rv_builder{ graph_ }
		{
			std::swap(_src_node, node_);

			_src_binder << node_;
		}

		template<class F> rv_edge_builder(F f_)
		{
			typedef typename Utility::function_traits<F>::result_type result_type;
			typedef typename Utility::function_traits<F>::arg<0>::type arg0_type;

			static_assert(Utility::function_traits<F>::arity == 1, "");

			_edge = std::make_shared<typed_edge<source_type, dest_type>>(f_);
		}

		auto into(rv<dest_type>& rv_)
		{
			auto dst_node = std::make_shared<typed_node<dest_type>>();

			_dst_binder << dst_node << rv_;

			_src_binder.apply_if_complete();
			_dst_binder.apply_if_complete();

			rv_edge_builder<dest_type, undefined_type> builder{ _graph };

			std::swap(builder._src_binder, _dst_binder);
			std::swap(builder._src_node, dst_node);

			return builder;
		}

		auto from(rv<source_type>& rv_)
		{
			_src_binder << rv_;

			_src_binder.apply_if_complete();
			_dst_binder.apply_if_complete();

			rv_node_builder<source_type, dest_type> builder{ _graph };
			std::swap(builder._dst_binder, _dst_binder);
			std::swap(builder._src_binder, _src_binder);
			std::swap(builder._src_node, _src_node);
			std::swap(builder._edge, _edge);

			return builder;
		}

		template<class... Ts, class... Us> void split(rv_edge_builder<Ts, Us>... ebs_)
		{

		}
	};




	class reactive_context
	{
	public:
		template<class T> auto from(rv<T>& rv_)
		{
			rv_node_builder<T, undefined_type> builder{ _graph, rv_ };

			return builder;
		}

		template<class F> auto map(F f_)
		{
			typedef typename Utility::function_traits<F>::result_type result_type;
			typedef typename Utility::function_traits<F>::arg<0>::type arg0_type;

			static_assert(Utility::function_traits<F>::args_arity == 1, "");

			rv_edge_builder<arg0_type, result_type> builder{ _graph };

			return builder;
		}

		template<class... Ts, class... Us> auto merge(rv_edge_builder<Ts, Us>... ebs_)
		{
			typedef typename Utility::first_of<Us...>::type first_type;
			static_assert(Utility::is_same_as_all_of<first_type, Us...>::value, "");

			rv_edge_builder<std::vector<first_type>, std::vector<first_type>> builder{ _graph };

			return builder;
		}
	private:
		graph _graph;
	};
}
