#pragma once

#include "traits.hpp"
#include "hash.hpp"
#include <algorithm>
#include <set>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <queue>
#include <stack>

#pragma push_macro("min")
#pragma push_macro("max")

#undef min
#undef max

/*
instrusive graph implementation
*/

namespace utility
{
	// set like
	class graph
	{
	public:
		typedef int vertex_id_t;
		typedef std::pair<vertex_id_t, vertex_id_t> edge_id_t;

		enum EDGE_DIRECTION : byte
		{
			EDGE_DIRECTION_FORWARD = 0x1,
			EDGE_DIRECTION_BACKWARD = 0x2,
			EDGE_DIRECTION_UNDEFINED = 0,
			EDGE_DIRECTION_DEFAULT = EDGE_DIRECTION_UNDEFINED
		};

		struct edge_property
		{
			byte direction = EDGE_DIRECTION_UNDEFINED;
		};

		typedef std::unordered_map<vertex_id_t, std::unordered_map<vertex_id_t, edge_property>> container_t;

		static const vertex_id_t UNDEFINED_VERTEX = std::numeric_limits<vertex_id_t>::min();


		struct iterator : std::iterator<std::forward_iterator_tag, edge_id_t>
		{
			iterator(container_t& edges_by_vertex_, edge_id_t first_, EDGE_DIRECTION search_direction_ = EDGE_DIRECTION_FORWARD)
				: _edges_by_vertex{ edges_by_vertex_ }
				, _SEARCH_DIRECTION { search_direction_ }
			{
				_trace.push(first_);
				_ignored.insert(first_);
			}

			//
			//	constructs end iterator
			//
			iterator(container_t& edges_by_vertex_)
				: _edges_by_vertex{ edges_by_vertex_ }
				, _SEARCH_DIRECTION { EDGE_DIRECTION_DEFAULT }
			{
			}

			friend bool operator==(const iterator& lhs_, const iterator& rhs_)
			{
				bool both_has_data = !(lhs_._trace.empty() || rhs_._trace.empty()) ;
				
				return both_has_data
					? (lhs_._trace.front() == rhs_._trace.front())
					: (lhs_._trace.empty() && rhs_._trace.empty());
			}

			friend bool operator!=(const iterator& lhs_, const iterator& rhs_)
			{
				return !( lhs_ == rhs_);
			}

			const value_type& operator*() const
			{
				return _trace.front();
			}

			const value_type* operator->() const
			{
				return &_trace.front();
			}

			iterator& operator++()
			{
				auto tos = _trace.front();
				_trace.pop();

				bool reverse = _SEARCH_DIRECTION == EDGE_DIRECTION_BACKWARD;

				auto next_beg = reverse ? tos.first : tos.second;

				for(auto next_end : _edges_by_vertex.at(next_beg))
				{
					if (next_end.second.direction & _SEARCH_DIRECTION)
					{
						auto next_edge = reverse_if({ next_beg, next_end.first }, next_end.second.direction);

						if (_ignored.insert(next_edge).second)
						{
							_trace.push(next_edge);
						}
					}
				}

				return *this;
			}

			const EDGE_DIRECTION _SEARCH_DIRECTION;
			std::queue<edge_id_t> _trace;
			std::unordered_set<edge_id_t> _ignored;
			container_t& _edges_by_vertex;
		};


		graph() = default;

		graph(std::initializer_list<edge_id_t> edges_)
		{
			for(auto e : edges_)
			{
				insert(e);
			}
		}


		iterator insert(edge_id_t edge_)
		{
			_edges_by_vertex[edge_.first][edge_.second].direction |= EDGE_DIRECTION_FORWARD;
			_edges_by_vertex[edge_.second][edge_.first].direction |= EDGE_DIRECTION_BACKWARD;

			return { _edges_by_vertex, edge_ };
		}
		
		void erase(edge_id_t edge_)
		{
			_edges_by_vertex[edge_.first].erase(edge_.second);
			_edges_by_vertex[edge_.second].erase(edge_.first);
		}

		void erase(vertex_id_t vertex_)
		{
			auto it = _edges_by_vertex.find(vertex_);
			
			if (it != _edges_by_vertex.end())
			{
				for(auto edge_desc : it->second)
				{
					_edges_by_vertex[edge_desc.first].erase(vertex_);
				}

				_edges_by_vertex.erase(vertex_);
			}
		}

		iterator find(edge_id_t edge_)
		{
			using namespace std;

			auto& it_end_vertices = _edges_by_vertex.find(edge_.first);

			if (it_end_vertices != _edges_by_vertex.end())
			{
				auto it = it_end_vertices->second.find(edge_.second);
				if(it != it_end_vertices->second.end())
				{
					return { _edges_by_vertex, edge_ };
				}
			}

			return end();
		}

		std::pair<iterator, iterator> equal_range(edge_id_t edge_)
		{
			if(edge_.first == UNDEFINED_VERTEX)
			{
				return _er_undefined_dst(edge_.second, EDGE_DIRECTION_BACKWARD);
			}
			else if (edge_.second == UNDEFINED_VERTEX)
			{
				return _er_undefined_dst(edge_.first, EDGE_DIRECTION_FORWARD);
			}
			else
			{
				auto it = find(edge_);

				return { it, ++it };
			}
		}


		iterator begin()
		{
			auto& it_end_Vertices = _edges_by_vertex.begin();
			if(it_end_Vertices != _edges_by_vertex.end())
			{
				return
				{
					_edges_by_vertex,
					{
						it_end_Vertices->first,
						it_end_Vertices->second.begin()->first,
					}					
				};
			}

			return end();
		}

		iterator end()
		{
			return { _edges_by_vertex };
		}

		size_t count_of_vertices() const
		{
			return _edges_by_vertex.size();
		}

		std::vector<vertex_id_t> output_vertices(vertex_id_t v_) const
		{
			return _near_vertices(v_, EDGE_DIRECTION_FORWARD);
		}

		std::vector<vertex_id_t> input_vertices(vertex_id_t v_) const
		{
			return _near_vertices(v_, EDGE_DIRECTION_BACKWARD);
		}

		void merge(graph& other_)
		{
			for(auto it = other_.begin(); it != other_.end(); ++it)
			{
				insert( *it );
			}
		}

	private:
		enum
		{
			INPUT_EDGES,
			OUTPUT_EDGES
		};

		container_t _edges_by_vertex;

		static edge_id_t reverse_if(edge_id_t e_, EDGE_DIRECTION dir_)
		{
			return dir_ == EDGE_DIRECTION_BACKWARD
				? edge_id_t { e_.second, e_.first }
				: e_;
		}

		static edge_id_t reverse_if(edge_id_t e_, byte dir_)
		{
			return dir_ & EDGE_DIRECTION_FORWARD
				? e_
				: edge_id_t{ e_.second, e_.first };
		}

		std::pair<iterator, iterator> _er_undefined_dst(vertex_id_t src_vertex_, EDGE_DIRECTION dir_)
		{
			using namespace std;

			vector<vertex_id_t> out_vertices;

			auto it_end_vertices = _edges_by_vertex.find(src_vertex_);
			if (it_end_vertices != _edges_by_vertex.end())
			{
				auto it = it_end_vertices->second.begin();

				//
				for(;it != it_end_vertices->second.end() && it->second.direction != dir_; ++it);

				auto first_edge	= reverse_if({ UNDEFINED_VERTEX, src_vertex_	}, dir_);
				auto last_edge	= reverse_if({ UNDEFINED_VERTEX, it->first		}, dir_);

				return
				{
					++iterator { _edges_by_vertex, first_edge,	dir_ },
					++iterator { _edges_by_vertex, last_edge,	dir_ }
				};
			}

			return { end(), end() };
		}

		std::vector<vertex_id_t> _near_vertices(vertex_id_t v_, EDGE_DIRECTION dir_) const
		{
			auto it = _edges_by_vertex.find(v_);

			std::vector<vertex_id_t> vec;

			if(it != _edges_by_vertex.end())
			{
				auto& near_vertices = it->second;

				for(auto& e: near_vertices)
				{
					if (e.second.direction & dir_)
					{
						vec.push_back(e.first);
					}
				}
			}

			return vec;
		}
	};





	// map like
	template<class VERTEX_DATA_TYPE, class EDGE_DATA_TYPE = void> class valued_graph
	{
	public:
		typedef VERTEX_DATA_TYPE vertex_data_t;
		typedef EDGE_DATA_TYPE edge_data_t;

		typedef int vertex_id_t;
		typedef int edge_id_t;

		edge_data_t& operator[](edge_id_t edge_)
		{
			return _edges[edge_];
		}

		vertex_data_t& operator[](vertex_id_t vertex_)
		{
			return _vertices[vertex_];
		}

	private:
		std::unordered_map<vertex_id_t, vertex_data_t> _vertices;
		std::unordered_map<edge_id_t, edge_data_t> _edges;

		graph _graph;
	};
}

#pragma pop_macro("min")
#pragma pop_macro("max")
