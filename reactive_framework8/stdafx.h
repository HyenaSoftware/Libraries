// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include <memory>
#include <sstream>
#include <vector>
#include <iostream>
#include <unordered_map>


#include <boost/optional.hpp>
#include <boost/property_tree/json_parser.hpp>	// it has a global using namespace for boost

#include <thread_pool\thread_pool>
#include <utility\traits.hpp>
#include <utility\tuple_utils.hpp>
#include <utility\network.hpp>
#include <utility\string_utils.hpp>
#include <utility\boolean_operators.hpp>