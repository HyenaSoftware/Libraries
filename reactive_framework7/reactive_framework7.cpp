#include "stdafx.h"
#include "reactive_framework7.h"


using namespace std;


namespace reactive_framework7 
{
	abstract_reactive_context::abstract_reactive_context()
	{
	}


	void abstract_reactive_context::update(const irv& rv_)
	{
		on_value_holder_changed(*rv_.value_holder());
	}

	void abstract_reactive_context::regist_rv_edge(std::shared_ptr<detail::inode> dst_, std::initializer_list<std::shared_ptr<detail::inode>> srcs_)
	{
		auto w_id = vertex_id_of(*dst_);

		auto& ref_dst = *dst_;
		auto& ref_src = **srcs_.begin();

		_vertex_data[w_id].swap(dst_);

		for (auto src : srcs_)
		{
			auto v_id = vertex_id_of(*src);

			_graph.insert({ v_id, w_id });

			_vertex_data[v_id] = std::move(src);
		}

		_update_node_from(ref_dst, ref_src);
	}

	void abstract_reactive_context::copy_setup(std::shared_ptr<detail::inode> node_from_, std::shared_ptr<detail::inode> node_to_)
	{
		auto from_id = vertex_id_of(*node_from_);
		auto to_id = vertex_id_of(*node_to_);

		auto src_nodes = _graph.input_vertices(from_id);

		for (auto src_id : src_nodes)
		{
			_graph.insert({ src_id, to_id });

			_vertex_data[to_id] = node_to_;
		}

		if(src_nodes.empty())
		{
			throw runtime_error
			{
				HERE"From-node doesn't have any input nodes. This operation might not be valid"
			};
		}

		auto& an_src_node = *_vertex_data.at(src_nodes.front());
		_update_node_from(*node_to_, an_src_node);
	}

	computation_graph_t::vertex_id_t abstract_reactive_context::vertex_id_of(const detail::inode& holder_)
	{
		auto v_id = holder_.id();

		return v_id;
	}

	void abstract_reactive_context::release_rv(const detail::inode& node_)
	{
		auto node_id = node_.id();

		// TODO: make sure this isn't called after the context has been destroyed

		//_graph.erase(node_id);
		//_vertex_data.erase(node_id);
	}

	//
	//	notification about src_id_ has a new value now
	//
	void abstract_reactive_context::on_value_holder_changed(detail::inode& src_node_)
	{
		if (_should_propagate(src_node_))
		{
			auto src_id = vertex_id_of(src_node_);
			auto output_vertices = _graph.output_vertices(src_id);

			for (auto dst_id : output_vertices)
			{
				auto& dst_node = _vertex_data.at(dst_id);
			
				_update_node_from(*dst_node, src_node_);
			}
		}
	}

	//
	//
	//
	limit_single_chain_execution_policy::limit_single_chain_execution_policy(detail::ireactive_context& rc_)
		: _rc { rc_ }
	{
	}

	int limit_single_chain_execution_policy::max_single_chain_recompute() const
	{
		return _single_chain_recompute_limit;
	}

	void limit_single_chain_execution_policy::set_max_single_chain_recompute(int single_chain_recompute_limit_)
	{
		_single_chain_recompute_limit = single_chain_recompute_limit_;
	}

	void limit_single_chain_execution_policy::update_node_from(detail::inode& target_, detail::inode& source_)
	{
		//
		++_node_id_to_limit[source_.id()];

		bool updated = target_.set_value_from(source_);

		if (updated)
		{
			_rc.on_value_holder_changed(target_);
		}

		--_node_id_to_limit[source_.id()];
	}

	bool limit_single_chain_execution_policy::should_propagate(detail::inode& node_)
	{
		int& count_of_updates = _node_id_to_limit[node_.id()];

		return count_of_updates < _single_chain_recompute_limit;
	}
}
