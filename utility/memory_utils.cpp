#include "stdafx.h"
#include "memory_utils.hpp"



namespace utility
{
	namespace detail
	{
		static void* g_nullptr_variable = nullptr;
		std::shared_ptr<void> eternal_shared_ptr { g_nullptr_variable };
	}
}

