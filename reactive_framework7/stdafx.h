// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers



// TODO: reference additional headers your program requires here
#include <algorithm>
#include <array>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <typeindex>
#include <tuple>
#include <unordered_set>
#include <vector>
#include <chrono>

#include <boost\any.hpp>
#include <boost\fusion\algorithm\iteration\for_each.hpp>
//#include <Utility\exception.hpp>
#include <Utility\event_handler>
#include <Utility\traits.h>
#include <utility\graph2.h>

#include <blocking_queue\blocking_queue>
#include <thread_pool\thread_pool>
