// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#ifdef UTILITY_EXPORTS
//#define LUABIND_API __declspec(dllexport)
#else
//#define LUABIND_API __declspec(dllimport)
#endif

// TODO: reference additional headers your program requires here
#include <algorithm>
#include <codecvt>
#include <functional>
#include <sstream>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/edge_list.hpp>
#include <boost/graph/labeled_graph.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/transform_value_property_map.hpp>