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

	constexpr std::size_t operator "" _sz(unsigned long long n_)
	{
		return static_cast<size_t>(n_);
	}

	template<class T> using static_not = std::integral_constant<bool, !T::value>;
}

namespace reactive_framework5
{
	template<class I> class node
	{
	public:
		typedef I id_type;

		node() : _id{ reinterpret_cast<id_type>(this) } {}

		id_type id() const
		{
			return _id;
		}

		void set_id(id_type id_)
		{
			std::swap(_id, id_);
		}

	private:

		id_type _id;
	};

	define_has_method(add_output_edge);
	define_has_method(add_input_edge);

	template<class I> class edge
	{
	public:
		typedef I id_type;

		edge() : _id{ reinterpret_cast<id_type>(this) } {}

		id_type id() const
		{
			return _id;
		}

		void set_id(id_type id_)
		{
			std::swap(_id, id_);
		}

	private:
		id_type _id;
	};
	
	using default_node = node<int>;
	using default_edge = edge<int>;

	template<class I> class graph
	{
	public:
		typedef typename node<I>::id_type id_type;
		static_assert(std::is_same<typename node<I>::id_type, typename edge<I>::id_type>::value, "id type of node and edge must be the same.");

		typedef node<I> node_t;
		typedef edge<I> edge_t;

		template<class SN, class E, class DN> void connect(std::shared_ptr<SN> src_n_, std::shared_ptr<E> edge_, std::shared_ptr<DN> dst_n_)
		{
			static_assert(std::is_base_of<node<id_type>, SN>::value, "Type SN must be subtype of node");
			static_assert(std::is_base_of<node<id_type>, DN>::value, "Type DN must be subtype of node");
			static_assert(std::is_base_of<edge<id_type>, E>::value, "Type E must be subtype of edge");

			static_assert(has_method_add_input_edge<SN>::value, "");
			static_assert(has_method_add_output_edge<DN>::value, "");

			Utility::throw_if(src_n_ == nullptr, "source node is not exists.");
			Utility::throw_if(dst_n_ == nullptr, "source node is not exists.");
			Utility::throw_if(edge_ == nullptr, "edge is not exists.");

			src_n_->add_output_edge(edge_);
			dst_n_->add_input_edge(edge_);

			edge_->set_input_node(src_n_);
			edge_->set_output_node(dst_n_);

			_nodes.insert(std::make_pair(src_n_->id(), src_n_));
			_nodes.insert(std::make_pair(dst_n_->id(), dst_n_));
			_edges.insert(std::make_pair(edge_->id(), edge_));
		}

	private:

		std::unordered_map<id_type, std::weak_ptr<edge_t>> _edges;
		std::unordered_map<id_type, std::weak_ptr<node_t>> _nodes;
	};

	using default_graph = graph<int>;


	template<class R, class A, class I> class typed_edge;

	template<class T, class I> class typed_node : public node<I>
	{
	public:
		typedef T value_type;

		std::vector<std::function<void(value_type)>> on_changed;

		template<class EDGE_OUT, class EDGE_IN> void add_output_edge(std::shared_ptr<typed_edge<EDGE_OUT, EDGE_IN, id_type>> edge_)
		{
			_output_slots[edge_] = [edge_](value_type& v_)
			{
				edge_->set(v_);
			};
		}

		template<class EDGE_OUT, class EDGE_IN> void add_input_edge(std::shared_ptr<typed_edge<EDGE_OUT, EDGE_IN, id_type>> edge_)
		{
			// unnecessary for us
			_input_edges.insert(edge_.get());
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
		void set(std::function<value_type(value_type)> lambda_)
		{
			_value = lambda_(std::move(_value));

			_fire_on_changed();
		}

		size_t count_of_output_edges() const
		{
			return _output_slots.size();
		}

		size_t count_of_input_edges() const
		{
			return _input_edges.size();
		}

		value_type get() const
		{
			return _value;
		}

		void swap_content(value_type& value_)
		{
			std::swap(_value, value_);
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
		std::unordered_map<std::shared_ptr<edge<id_type>>, std::function<void(value_type&)>> _output_slots;
		std::unordered_set<edge<id_type>*> _input_edges;
	};

	template<class R, class A, class I> class typed_edge : public edge<I>
	{
	public:
		typedef A input_value_type;
		typedef R output_value_type;

		template<class F> typed_edge(F f_) //: _lambda{f_}
		{
			_func  = make_func(f_);
		}

		template<class NODE_INPUT_TYPE> void set_input_node(std::shared_ptr<typed_node<NODE_INPUT_TYPE, id_type>> node_)
		{
			// no operation
		}

		//template<class NODE_OUTPUT_TYPE> void set_output_node(std::shared_ptr<typed_node<NODE_OUTPUT_TYPE>> node_)
		void set_output_node(std::shared_ptr<typed_node<output_value_type, id_type>> node_)
		{
			Utility::throw_if(node_ == nullptr, "underlying node is not exists");

			_target = node_;
		}

		void set(input_value_type value_)
		{
			Utility::throw_if(_target == nullptr, "underlying node is not exists");

			_func->apply(std::move(value_));
		}

	private:
		std::shared_ptr<typed_node<output_value_type, id_type>> _target;
		//std::function<output_value_type(input_value_type)> _lambda;

		struct func_holder
		{
			std::shared_ptr<typed_node<output_value_type, id_type>>& _target;

			func_holder(std::shared_ptr<typed_node<output_value_type, id_type>>& target_) : _target{target_}
			{
			}

			virtual void apply(input_value_type) = 0;
		};

		struct func_holder_unary : func_holder
		{
			typedef std::function<output_value_type(input_value_type)> lambda_t;
			lambda_t _lambda;

			func_holder_unary(std::shared_ptr<typed_node<output_value_type, id_type>>& target_, lambda_t lambda_)
				: func_holder{std::move(target_) }
			{
				std::swap(_lambda, lambda_);
			}

			void apply(input_value_type value_) override
			{
				_target->set(_lambda(std::move(value_)));
			}
		};

		struct func_holder_binary : func_holder
		{
			typedef std::function<output_value_type(output_value_type, input_value_type)> lambda_t;
			lambda_t _lambda;

			func_holder_binary(std::shared_ptr<typed_node<output_value_type, id_type>>& target_, lambda_t lambda_)
				: func_holder{ std::move(target_) }
			{
				std::swap(_lambda, lambda_);
			}

			void apply(input_value_type value_) override
			{
				_target->set(std::bind(_lambda, std::placeholders::_1, std::move(value_)));
			}
		};

		std::shared_ptr<func_holder> _func;

		template<class F> 
			std::enable_if_t<Utility::function_traits<F>::args_arity == 1, std::shared_ptr<func_holder>>
			make_func(F f_)
		{
			return std::make_shared<func_holder_unary>(_target, f_);
		}

		template<class F>
			std::enable_if_t<Utility::function_traits<F>::args_arity == 2, std::shared_ptr<func_holder>>
			make_func(F f_)
		{
			typedef Utility::function_traits<F> func_type;

			static_assert(std::is_same<func_type::arg<0>::type, func_type::result_type>::value,
				"First argument of the function must be the same as the return type.");

			return std::make_shared<func_holder_binary>(_target, f_);
		}
	};


	template<class T, class I = std::string> class rv
	{
	public:
		typedef T value_type;
		typedef I id_type;

		rv() = default;

		rv(id_type id_)
		{
			std::swap(_id, id_);
		}

		rv(value_type value_, id_type id_ = id_type{})
		{
			std::swap(_value, value_);
			std::swap(_id, id_);
		}

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

		value_type get() const
		{
			return _value;
		}

		id_type id() const
		{
			return _id;
		}

		void set_id(id_type id_)
		{
			std::swap(_id, id_);

			if (_underlying_node)
			{
				_underlying_node->set_id(_id);
			}
		}

		void bind_underlying_node(std::shared_ptr<typed_node<value_type, id_type>> underlying_node_)
		{
			std::swap(_underlying_node, underlying_node_);

			// it handles the data updating, which comes from the underlying node
			auto lthis = this;

			_underlying_node->on_changed.push_back([lthis](value_type value_)
			{
				std::swap(lthis->_value, value_);
				lthis->_fire_on_changed();
			});

			_underlying_node->set(_value);
			_underlying_node->set_id(_id);
		}

		std::shared_ptr<typed_node<value_type, id_type>> underlying_node() const
		{
			return _underlying_node;
		}
	private:
		value_type _value;
		id_type _id;

		std::shared_ptr<typed_node<value_type, id_type>> _underlying_node;

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

	template<class = std::string> class reactive_context;
	template<class, class, class, class, class> class rv_context;
	template<class, class, class, class, class> class rv_context_one_to_multiple;
	template<class, class, class, class, class> class rv_context_multiple_to_one;

	template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE, class I>
		class rv_abstract_context
	{
	public:
		typedef NODE_SRC_TYPE node_src_type;
		typedef NODE_DST_TYPE node_dst_type;

		typedef EDGE_SRC_TYPE edge_src_type;
		typedef EDGE_DST_TYPE edge_dst_type;

		typedef I id_type;

		graph<id_type>& get_graph()
		{
			return _graph;
		}

	protected:
		rv_abstract_context(graph<id_type>& graph_) : _graph {graph_}{}

		graph<id_type>& _graph;
	};

	template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE, class I>
		class rv_context : public rv_abstract_context<NODE_SRC_TYPE, EDGE_SRC_TYPE, EDGE_DST_TYPE, NODE_DST_TYPE, I>
	{
		template<class, class, class, class, class> friend class rv_context;
		template<class, class, class, class, class> friend class rv_context_one_to_multiple;
	public:

		template<class NS, class ES, class ED, class ND> struct rebind
		{
			typedef rv_context<NS, ES, ED, ND, id_type> type;
		};

		template<class NS, class ES, class ED, class ND> using rebind_t = typename rebind<NS, ES, ED, ND>::type;

		rv_context(const rv_context& other_) = default;

		template<class NS, class ES, class ED, class ND> rv_context(rv_context_multiple_to_one<NS, ES, ED, ND, id_type> other_)
			: rv_abstract_context{ other_._graph }
		{
			Utility::swap_if_possible(_dst_node, other_._dst_node);
			Utility::swap_if_possible(_ptr_dst_rv, other_._ptr_dst_rv);
		}

		template<class NS, class ES, class ED, class ND> rv_context(rv_context<NS, ES, ED, ND, id_type> other_)
			: rv_abstract_context{ other_._graph }
		{
			Utility::swap_if_possible(_src_node, other_._src_node);
			Utility::swap_if_possible(_dst_node, other_._dst_node);
			Utility::swap_if_possible(_edge, other_._edge);
			Utility::swap_if_possible(_ptr_src_rv, other_._ptr_src_rv);
			Utility::swap_if_possible(_ptr_dst_rv, other_._ptr_dst_rv);
		}

		rv_context(graph<id_type>& graph_)
			: rv_abstract_context{ graph_ }
		{
		}

		bool try_apply()
		{
			return _try_apply_impl(is_complete_type{});
		}

		bool is_complete() const
		{
			return static_cast<bool>(_src_node)
				&& static_cast<bool>(_dst_node)
				&& static_cast<bool>(_edge)
				&& _ptr_src_rv != nullptr
				&& _ptr_dst_rv != nullptr;
		}

		std::shared_ptr<typed_node<node_src_type, id_type>> src_node() const
		{
			return _src_node;
		}

		std::shared_ptr<typed_node<node_dst_type, id_type>> dst_node() const
		{
			return _dst_node;
		}

		std::shared_ptr<typed_edge<edge_dst_type, edge_src_type, id_type>> edge() const
		{
			return _edge;
		}

		rv<node_src_type, id_type>* ptr_src_rv() const
		{
			return _ptr_src_rv;
		}

		rv<node_dst_type, id_type>* ptr_dst_rv() const
		{
			return _ptr_dst_rv;
		}

		void set_src_node(std::shared_ptr<typed_node<node_src_type, id_type>> src_node_)
		{
			std::swap(_src_node, src_node_);
		}

		void set_dst_node(std::shared_ptr<typed_node<node_dst_type, id_type>> dst_node_)
		{
			std::swap(_dst_node, dst_node_);
		}

		void set_edge(std::shared_ptr<typed_edge<edge_dst_type, edge_src_type, id_type>> edge_)
		{
			std::swap(_edge, edge_);
		}

		void set_ptr_src_rv(rv<node_src_type, id_type>& rv_)
		{
			_ptr_src_rv = &rv_;
		}

		void set_ptr_dst_rv(rv<node_dst_type, id_type>& rv_)
		{
			_ptr_dst_rv = &rv_;
		}

	private:
		typedef Utility::static_not<
			Utility::is_any_of<undefined_type, node_src_type, node_dst_type, edge_src_type, edge_dst_type>
		> is_complete_type;

		bool _try_apply_impl(std::true_type)
		{
			bool completed = is_complete();
			if (completed)
			{
				// connect the nodes
				_graph.connect(_src_node, std::move(_edge), _dst_node);

				_ptr_src_rv->bind_underlying_node(std::move(_src_node));
				_ptr_dst_rv->bind_underlying_node(std::move(_dst_node));

				_ptr_src_rv = nullptr;
				_ptr_dst_rv = nullptr;
			}

			return completed;
		}

		bool _try_apply_impl(std::false_type)
		{
			return false;
		}

		// shared_ptr is used here instead of weak_ptr to keep them alive during the
		//  graph path building to avoid a lot of boring reference checking
		std::shared_ptr<typed_node<node_src_type, id_type>> _src_node;
		std::shared_ptr<typed_node<node_dst_type, id_type>> _dst_node;
		std::shared_ptr<typed_edge<edge_dst_type, edge_src_type, id_type>> _edge;

		rv<node_src_type, id_type>* _ptr_src_rv = nullptr;
		rv<node_dst_type, id_type>* _ptr_dst_rv = nullptr;
	};

	template<class N, class I> struct aggregated_nodes
	{
		typedef N node_type;
		typedef I id_type;

		std::shared_ptr<typed_node<node_type, id_type>> node;
		rv<node_type, id_type>* ptr_rv = nullptr;

		aggregated_nodes() = default;

		aggregated_nodes(std::shared_ptr<typed_node<node_type, id_type>> node_, rv<node_type, id_type>* ptr_rv_)
			: ptr_rv{ ptr_rv_ }
		{
			std::swap(node, node_);
		}

		bool is_empty() const
		{
			return node == nullptr
				&& ptr_rv == nullptr;
		}
	};


	template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE, class I>
		class rv_context_one_to_multiple : public rv_abstract_context<NODE_SRC_TYPE, EDGE_SRC_TYPE, EDGE_DST_TYPE, NODE_DST_TYPE, I>
	{
	public:
		typedef aggregated_nodes<node_dst_type, id_type> target;

		rv_context_one_to_multiple(const rv_context_one_to_multiple&) = default;

		template<class NS, class ES, class ED, class ND>
			rv_context_one_to_multiple(rv_context_one_to_multiple<NS, ES, ED, ND, id_type> other_)
			: rv_abstract_context{ other_.get_graph() }
		{
			Utility::swap_if_possible(_src_node, other_._src_node);
			Utility::swap_if_possible(_ptr_src_rv, other_._ptr_src_rv);
			Utility::swap_if_possible(_outputs, other_._outputs);
		}

		template<class NS, class ES, class ED, class ND>
			rv_context_one_to_multiple(rv_context<NS, ES, ED, ND, id_type> other_)
			: rv_abstract_context{ other_.get_graph() }
		{
			Utility::swap_if_possible(_src_node, other_._src_node);
			Utility::swap_if_possible(_ptr_src_rv, other_._ptr_src_rv);
		}

		void set_src_node(std::shared_ptr<typed_node<node_src_type, id_type>> src_node_)
		{
			std::swap(_src_node, src_node_);
		}

		void set_ptr_src_rv(rv<node_src_type, id_type>& rv_)
		{
			_ptr_src_rv = &rv_;
		}

		void add_dst_node(target target_)
		{
			_outputs.push_back(std::move(target_));
		}

		bool is_complete() const
		{
			return
				_src_node &&
				_ptr_src_rv != nullptr &&
				!_outputs.empty();
		}

		bool try_apply()
		{
			return _try_apply_impl(is_complete_type{});
		}

	private:
		typedef Utility::static_not<
			Utility::is_any_of<undefined_type, node_src_type, node_dst_type, edge_src_type, edge_dst_type>
		> is_complete_type;

		bool _try_apply_impl(std::true_type)
		{
			bool completed = is_complete();
			if (completed)
			{
				size_t wr_index = _src_node->count_of_output_edges();

				for (auto& out : _outputs)
				{
					auto split_func = [wr_index](const edge_src_type& vec_) -> edge_dst_type
					{
						if(vec_.size() <= wr_index)
							return edge_dst_type{};

						return vec_[wr_index];
					};

					++wr_index;
					auto edge = std::make_shared<typed_edge<edge_dst_type, const edge_src_type&, id_type>>(split_func);

					// connect the nodes
					_graph.connect(_src_node, std::move(edge), out.node);

					out.ptr_rv->bind_underlying_node(std::move(out.node));
				}

				_ptr_src_rv->bind_underlying_node(_src_node);
				_ptr_src_rv = nullptr;
			}

			return completed;
		}

		bool _try_apply_impl(std::false_type)
		{
			return false;
		}

		// shared_ptr is used here instead of weak_ptr to keep them alive during the
		//  graph path building to avoid a lot of boring reference checking
		std::shared_ptr<typed_node<node_src_type, id_type>> _src_node;
		rv<node_src_type, id_type>* _ptr_src_rv = nullptr;

		std::vector<target> _outputs;
	};

	template<class NODE_SRC_TYPE, class EDGE_SRC_TYPE, class EDGE_DST_TYPE, class NODE_DST_TYPE, class I>
		class rv_context_multiple_to_one : public rv_abstract_context<NODE_SRC_TYPE, EDGE_SRC_TYPE, EDGE_DST_TYPE, NODE_DST_TYPE, I>
	{
		template<class, class, class, class, class> friend class rv_context;
		template<class, class, class, class, class> friend class rv_context_multiple_to_one;
	public:
		typedef aggregated_nodes<node_src_type, id_type> source;

		template<class NS, class ES, class ED, class ND> struct rebind
		{
			typedef rv_context_multiple_to_one<NS, ES, ED, ND, id_type> type;
		};

		template<class NS, class ES, class ED, class ND> using rebind_t = typename rebind<NS, ES, ED, ND>::type;

		template<class NS, class ES, class ED, class ND>
			rv_context_multiple_to_one(rv_context_multiple_to_one<NS, ES, ED, ND, id_type> other_)
				: rv_abstract_context{ other_.get_graph() }
		{
			using namespace Utility;
			
			swap_if_possible(_dst_node, other_._dst_node);
			swap_if_possible(_ptr_dst_rv, other_._ptr_dst_rv);
			swap_if_possible(_sources, other_._sources);
		}

		rv_context_multiple_to_one(graph<id_type>& graph_)
			: rv_abstract_context{ graph_ }
		{
		}

		void set_dst_node(std::shared_ptr<typed_node<node_dst_type, id_type>> dst_node_)
		{
			std::swap(_dst_node, dst_node_);
		}

		void set_ptr_dst_rv(rv<node_dst_type, id_type>& rv_)
		{
			_ptr_dst_rv = &rv_;
		}

		void add_src_node(source source_)
		{
			_sources.push_back(std::move(source_));
		}
		
		bool is_complete() const
		{
			return
				_dst_node &&
				_ptr_dst_rv != nullptr &&
				!_sources.empty();
		}

		bool try_apply()
		{
			return _try_apply_impl(is_complete_type{});
		}

	private:
		typedef Utility::static_not<
			Utility::is_any_of<undefined_type, node_src_type, node_dst_type, edge_src_type, edge_dst_type>
		> is_complete_type;

		bool _try_apply_impl(std::true_type)
		{
			using namespace Utility;

			bool completed = is_complete();
			if (completed)
			{
				// take the last index of vector in target node
				
				size_t wr_index = _dst_node->count_of_input_edges();

				for (auto& in : _sources)
				{
					auto merger_func = [wr_index](edge_dst_type vec_, edge_src_type value_) -> edge_dst_type
					{
						if(vec_.size() <= wr_index)
						{
							vec_.resize(wr_index + 1);
						}

						std::swap(vec_[wr_index], value_);

						return std::move(vec_);
					};

					++wr_index;

					auto edge = std::make_shared<typed_edge<edge_dst_type, edge_src_type, id_type>>(merger_func);

					// connect the nodes
					_graph.connect(in.node, std::move(edge), _dst_node);

					in.ptr_rv->bind_underlying_node(std::move(in.node));
				}

				_ptr_dst_rv->bind_underlying_node(_dst_node);
				_ptr_dst_rv = nullptr;

				node_dst_type vec;
				_dst_node->swap_content(vec);
				vec.insert(vec.end(), std::max(wr_index - vec.size(), 0_sz), node_src_type{});
				_dst_node->swap_content(vec);
			}

			return completed;
		}

		bool _try_apply_impl(std::false_type)
		{
			return false;
		}

		// shared_ptr is used here instead of weak_ptr to keep them alive during the
		//  graph path building to avoid a lot of boring reference checking
		std::shared_ptr<typed_node<node_dst_type, id_type>> _dst_node;
		rv<node_dst_type, id_type>* _ptr_dst_rv = nullptr;

		std::vector<source> _sources;
	};

	template<class... NSs, class... ESs, class EDGE_DST_TYPE, class NODE_DST_TYPE, class I>
	class rv_context_multiple_to_one<std::tuple<NSs...>, std::tuple<ESs...>, EDGE_DST_TYPE, NODE_DST_TYPE, I>
		: public rv_abstract_context<std::tuple<NSs...>, std::tuple<ESs...>, EDGE_DST_TYPE, NODE_DST_TYPE, I>
	{
		template<class, class, class, class, class> friend class rv_context;
		template<class, class, class, class, class> friend class rv_context_multiple_to_one;
	public:
		template<class NS, class ES, class ED, class ND> struct rebind
		{
			typedef rv_context_multiple_to_one<NS, ES, ED, ND, id_type> type;
		};

		template<class NS, class ES, class ED, class ND> using rebind_t = typename rebind<NS, ES, ED, ND>::type;

		template<class... NSs2, class... ESs2, class ED, class ND>
		rv_context_multiple_to_one(rv_context_multiple_to_one<std::tuple<NSs2...>, std::tuple<ESs2...>, ED, ND, id_type> other_)
			: rv_abstract_context{ other_.get_graph() }
		{
			using namespace Utility;

			swap_if_possible(_dst_node, other_._dst_node);
			swap_if_possible(_ptr_dst_rv, other_._ptr_dst_rv);
			swap_if_possible(_sources, other_._sources);
		}

		rv_context_multiple_to_one(graph<id_type>& graph_)
			: rv_abstract_context{ graph_ }
		{
		}

		void set_dst_node(std::shared_ptr<typed_node<node_dst_type, id_type>> dst_node_)
		{
			std::swap(_dst_node, dst_node_);
		}

		void set_ptr_dst_rv(rv<node_dst_type, id_type>& rv_)
		{
			_ptr_dst_rv = &rv_;
		}

		template<class NS> void add_src_node(aggregated_nodes<NS, id_type> source_)
		{
			static_assert(Utility::is_any_of<NS, NSs...>::value,
				"node source type of this input source must be either of the acceptable node src types");

			std::swap(std::get<aggregated_nodes<NS, id_type>>(_sources), source_);
		}

		bool is_complete() const
		{
			bool srcs_is_non_empties = true;
			Utility::for_each(_sources, [&](auto& e_)
			{
				srcs_is_non_empties &= !e_.is_empty();
			});

			return
				_dst_node &&
				_ptr_dst_rv != nullptr &&
				srcs_is_non_empties;
		}

		bool try_apply()
		{
			return _try_apply_impl(is_complete_type{});
		}

	private:
		typedef Utility::static_not<
			Utility::is_any_of<undefined_type, node_src_type, node_dst_type, edge_src_type, edge_dst_type>
		> is_complete_type;

		bool _try_apply_impl(std::true_type)
		{
			bool completed = is_complete();
			if (completed)
			{
				using namespace Utility;

				for_each(_sources, [this](auto& agg_node_)
				{
					typedef std::remove_reference_t<decltype(agg_node_)> agg_node_t;
					typedef typename agg_node_t::node_type sub_node_src_type;

					auto join_func = [](std::tuple<ESs...> tpl_, sub_node_src_type value_)
					{
						std::swap(std::get<sub_node_src_type>(tpl_), value_);
						return std::move(tpl_);
					};

					auto edge = std::make_shared<typed_edge<std::tuple<NSs...>, sub_node_src_type, id_type>>(join_func);

					// connect the nodes
					_graph.connect(agg_node_.node, std::move(edge), _dst_node);

					agg_node_.ptr_rv->bind_underlying_node(std::move(agg_node_.node));
				});

				_ptr_dst_rv->bind_underlying_node(_dst_node);
				_ptr_dst_rv = nullptr;
			}

			return completed;
		}

		bool _try_apply_impl(std::false_type)
		{
			return false;
		}

		// shared_ptr is used here instead of weak_ptr to keep them alive during the
		//  graph path building to avoid a lot of boring reference checking
		std::shared_ptr<typed_node<node_dst_type, id_type>> _dst_node;
		rv<node_dst_type, id_type>* _ptr_dst_rv = nullptr;

		std::tuple<aggregated_nodes<NSs, id_type>...> _sources;
	};

	template<class> class rv_node_builder;
	template<class> class rv_edge_builder;

	/*
	*/
	template<class CONTEXT> class rv_abstract_builder
	{
		template<class> friend class reactive_context;
		template<class> friend class rv_node_builder;
	public:
		typedef CONTEXT context_type;
		typedef typename CONTEXT::id_type id_type;

		typedef typename CONTEXT::node_src_type node_src_type;
		typedef typename CONTEXT::node_dst_type node_dst_type;

		typedef typename CONTEXT::edge_src_type edge_src_type;
		typedef typename CONTEXT::edge_dst_type edge_dst_type;

		rv_abstract_builder(context_type context_)
			: _context{ std::move(context_) }
		{
		}

		friend const rv_context<node_src_type, edge_src_type, edge_dst_type, node_dst_type, id_type>& context_of(rv_abstract_builder& rvb_)
		{
			return rvb_._context;
		}

	protected:
		context_type _context;
	};

	template<class CONTEXT>	class rv_node_builder : public rv_abstract_builder<CONTEXT>
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

			throw_if(!_context.src_node(), "source node is not specified");

			typedef context_type::rebind_t<node_src_type, arg0_type, result_type, undefined_type> new_context_type;

			new_context_type context { _context };

			context.set_edge(std::make_shared<typed_edge<result_type, arg0_type, id_type>>(f_));

			return rv_edge_builder<new_context_type> { context };
		}

		template<template<class, class, class, class, class> class C, class... NDs>
		void split(rv_abstract_builder<C<undefined_type, undefined_type, undefined_type, NDs, id_type>>... abs_)
		{
			using namespace Utility;

			/*
				this.NS = vector<int>
				this.ES = undefined
				this.ED = undefined
				this.ND = undefined

				sub.NSs = undefined
				sub.ESs = undefined
				sub.EDs = undefined
				sub.NDs = int
			*/
			static_assert(!is_any_of<undefined_type, NDs...>::value, "abstract builders must have source types");

			typedef typename first_of<NDs...>::type first_node_dst_t;
			typedef first_node_dst_t first_edge_dst_t;

			typedef rv_context_one_to_multiple<
				node_src_type,
#pragma message("Check the 2nd argument as it should be edge_src_type instead of node_src_type, but it makes the unittests failed.")
				node_src_type,
				first_edge_dst_t,
				first_node_dst_t,
				id_type
			> rv_context_t;

			rv_context_t context{ _context };

			context.set_src_node(_context.src_node());
			context.set_ptr_src_rv(*_context.ptr_src_rv());

			for_each(std::make_tuple(std::move(abs_)...), [=, &context](auto& ra_)
			{
				context.add_dst_node(rv_context_t::target
				{
					ra_._context.dst_node(), ra_._context.ptr_dst_rv()
				});
			});

			context.try_apply();
		}
	};


	template<class CONTEXT>
		class rv_edge_builder : public rv_abstract_builder<CONTEXT>
	{
		template<class> friend class rv_node_builder;
		template<class> friend class rv_edge_builder;
	public:
		template<class C2> rv_edge_builder(C2 context_)
			: rv_abstract_builder{ std::move(context_) }
		{
			static_assert(!std::is_same<graph<id_type>, std::decay_t<C2>>::value, "?");
		}

		template<class new_node_dst_type> auto into(rv<new_node_dst_type, id_type>& rv_)
		{
			using namespace Utility;

			static_assert(std::is_same<node_dst_type, undefined_type>::value, "Previous destination node type must be undefined.");

			typedef
				typename context_type::rebind_t<node_src_type, edge_src_type, edge_dst_type, new_node_dst_type>
				rv_context_type;

			// FIXME: _context has the right context type,
			//	but context has the right arguments
			rv_context_type context { _context }; 

			auto dst_underlying_node = rv_.underlying_node();

			//throw_if(rv_.underlying_node() != nullptr, "underlying node is being overwrited");
			if (dst_underlying_node == nullptr)
			{
				dst_underlying_node = std::make_shared<typed_node<new_node_dst_type, id_type>>();
			}

			context.set_ptr_dst_rv(rv_);
			context.set_dst_node(std::move(dst_underlying_node));
			context.try_apply();

			return rv_abstract_builder<rv_context_type> { context };
		}

		auto from(rv<node_src_type>& rv_)
		{
			_context._src_node = std::make_shared<typed_node<source_type>>();
			_context._ptr_src_rv = &rv_;

			_context.try_apply();

			rv_node_builder<source_type, dest_type> builder{ _context };

			return builder;
		}

		template<template<class, class, class, class> class C, 
			class... NSs, class... ESs, class... EDs, class... NDs>
			void split(rv_abstract_builder<C<NSs, ESs, EDs, NDs>>... abs_)
		{
			using namespace Utility;

			static_assert(is_all_of<undefined_type, NSs...>::value, "abstract builders must have undefined source types");

			throw std::runtime_error{"rv_edge_builder::split() is not implemented"};
		}

	protected:
	};


	template<class I> class reactive_context
	{
	public:
		typedef I id_type;

		template<class NS> auto from(rv<NS, id_type>& rv_)
		{
			rv_context<NS, undefined_type, undefined_type, undefined_type, id_type> context { _graph };

			auto underlying_ptr = rv_.underlying_node();

			context.set_ptr_src_rv(rv_);
			context.set_src_node(underlying_ptr ? underlying_ptr : std::make_shared<typed_node<NS, id_type>>());

			typedef rv_node_builder<rv_context<NS, undefined_type, undefined_type, undefined_type, id_type>> rv_node_builder_t;
			rv_node_builder_t builder { std::move(context) };

			static_assert(std::is_same<rv_node_builder_t::node_src_type, NS>::value, "");
			static_assert(std::is_same<rv_node_builder_t::node_dst_type, undefined_type>::value, "");

			return builder;
		}

		template<class T> auto into(rv<T, id_type>& rv_)
		{
			typedef rv_context<undefined_type, undefined_type, undefined_type, T, id_type> rv_context_t;
			rv_context_t context{ _graph };

			context.set_ptr_dst_rv(rv_);
			context.set_dst_node(std::make_shared<typed_node<T, id_type>>());

			typedef rv_node_builder<rv_context_t> rv_node_builder_t;

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

			typedef rv_context<undefined_type, arg0_type, result_type, undefined_type, id_type> rv_context_t;
			rv_context_t context { _graph };
				
			context.set_edge(std::make_shared<typed_edge<result_type, arg0_type, id_type>>(f_));

			rv_edge_builder<rv_context_t> builder { std::move(context) };

			return builder;
		}

		/*
			make a wrapper:

			- edge_builder
				- edge_builder
				- edge_builder

			this.NSs : int
			this.ESs : undefined_type
			this.EDs : undefined_type
			this.NDs : undefined_type

			sub.NSs : undefined_type
			sub.ESs : undefined_type
			sub.EDs : undefined_type
			sub.NDs : vector<int>
		*/
		template<template<class, class, class, class, class> class C, class... NSs>
			auto merge(rv_node_builder<C<NSs, undefined_type, undefined_type, undefined_type, id_type>>... ebs_)
		{
			using namespace Utility;

			typedef typename first_of<NSs...>::type first_of_node_src_type;
			typedef std::vector<first_of_node_src_type> vec_first_of_node_src_type;

			static_assert(is_all_of<NSs...>::value, "Every node src type must be the same.");

			typedef rv_context_multiple_to_one<
				first_of_node_src_type,
				first_of_node_src_type,
				vec_first_of_node_src_type,	// edge dst type
				undefined_type,
				id_type
			> rv_context_t;
			

			rv_context_t context {_graph};

			for_each(std::make_tuple(std::move(ebs_)...), [&](auto& builder_)
			{
				context.add_src_node(rv_context_t::source
				{
					builder_._context.src_node(), builder_._context.ptr_src_rv()
				});
			});

			return rv_edge_builder<rv_context_t> { std::move(context) };
		}

		template<template<class, class, class, class, class> class C, class... NSs>
			auto join(rv_node_builder<C<NSs, undefined_type, undefined_type, undefined_type, id_type>>... builders_)
		{
			using namespace Utility;

			typedef rv_context_multiple_to_one<
				std::tuple<NSs...>,
				std::tuple<NSs...>,
				std::tuple<NSs...>,
				undefined_type,
				id_type
			> rv_context_t;

			rv_context_t context { _graph };

			for_each(std::make_tuple(std::move(builders_)...), [&context](auto& builder_)
			{
				typedef std::remove_reference_t<decltype(builder_)> builder_t;
				typedef typename builder_t::node_src_type sub_node_src_type;

				context.add_src_node(aggregated_nodes<sub_node_src_type, id_type>
				{
					builder_._context.src_node(), builder_._context.ptr_src_rv()
				});

			});
			
			return rv_edge_builder<rv_context_t> { std::move(context) };
		}
	private:
		graph<id_type> _graph;
	};
}
