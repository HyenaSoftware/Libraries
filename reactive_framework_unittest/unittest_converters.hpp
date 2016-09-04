#include "stdafx.h"
#include "CppUnitTest.h"

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template<class T> std::wstring ToString(const std::vector<T>& vec_)
			{
				std::wstringstream ws;

				ws << "[";

				for (auto& e : vec_)
				{
					ws << e << ", ";
				}

				if (!vec_.empty())
				{
					ws.seekp(-2, std::ios_base::end);
				}

				ws << "]";

				return ws.str();
			}

			std::wstring ToString(const std::type_index& ti_);
			std::wstring ToString(const std::type_info& ti_);

			template<class A, class B> std::wstring ToString(const std::pair<A, B>& pair_)
			{
				wstringstream ws;

				ws << "[" << pair_.first << "; " << pair_.second << "]";

				return ws.str();
			}

			template<class... Ts> std::wstring ToString(const std::tuple<Ts...>& ts_)
			{
				wstringstream ws;

				ws << "[";

				utility::for_each(ts_, [&ws](auto t_) { ws << t_ << ", "; });

				if (std::tuple_size<std::tuple<Ts...>>::value > 0)
				{
					ws.seekp(-2, ios_base::end);
				}

				ws << "]";

				return ws.str();
			}

			template<class T> std::wstring ToString(const std::unordered_set<T>& set_)
			{
				using namespace std;

				wstringstream ws;

				ws << "set [";
				for (auto& e : set_)
				{
					ws << ToString(e) << "; ";
				}

				ws.seekp(-2, ios::cur);
				ws << "]";

				return ws.str();
			}
		}
	}
}

