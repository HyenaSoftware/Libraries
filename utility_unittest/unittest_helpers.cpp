#include "stdafx.h"
#include "unittest_helpers.hpp"

using namespace std;
using namespace utility;



namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			void execute_with_exception_dump(function<void()> lambda_)
			{
				try
				{
					lambda_();
				}
				catch(std::exception& e_)
				{
					auto e_ptrs = unwrap_exceptions(e_);

					stringstream ss;
					for(auto e_ptr : e_ptrs)
					{
						rethrow_and_handle(e_ptr, [&ss](auto& e_)
						{
							ss << e_.what() << endl;
						});
					}

					auto wstr = to_wstring(ss.str());

					Assert::Fail(wstr.c_str());
				}
			}
		}
	}
}

