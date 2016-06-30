#pragma once
#include "stdafx.h"
#include "CppUnitTest.h"
#include <utility\exception_utils.hpp>


namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			void execute_with_exception_dump(std::function<void()> lambda_);
		}
	}
}
