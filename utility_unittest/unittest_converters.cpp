#include "stdafx.h"



using namespace std;
using namespace utility;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{

			wstring ToString(const vector<int>& vec_)
			{
				wstringstream ws;

				ws << "[";

				for (auto& e : vec_)
				{
					ws << e << ", ";
				}

				if (!vec_.empty())
				{
					ws.seekp(-2, ios_base::end);
				}

				ws << "]";

				return ws.str();
			}

			wstring ToString(const type_index& ti_)
			{
				return to_wstring(ti_.name());
			}

			wstring ToString(const type_info& ti_)
			{
				return to_wstring(ti_.name());
			}
		}
	}
}