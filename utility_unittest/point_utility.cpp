#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace utility;
using namespace utility::math;


namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template<class T> std::wstring ToString(const T t_)
			{
				wstringstream ws;


				return ws.str();
			}

			template<class T> std::wstring ToString(const math::point<T>& t_)
			{
				wstringstream ws;

				ws << "[" << t_.x << "; " << t_.y << "]";

				return ws.str();
			}

			template<class T> std::wstring ToString(const math::vector<T>& t_)
			{
				wstringstream ws;

				ws << "[" << t_.x << "; " << t_.y << "]";

				return ws.str();
			}
		}
	}
}


namespace utility_unittest
{
	TEST_CLASS(point_unittest)
	{
		TEST_METHOD(TestVector_add)
		{
			math::vector<int> a { 2, 2 };
			math::vector<int> b { 10, 10 };

			auto c = a + b;
			math::vector<int> expected_c { 12, 12 };

			Assert::AreEqual(expected_c, c);
		}

		TEST_METHOD(TestVector_sub)
		{
			math::vector<int> a{ 2, 2 };
			math::vector<int> b{ 10, 10 };

			auto c = a - b;
			math::vector<int> expected_c = a + (-1 * b);

			Assert::AreEqual(expected_c, c);
		}

		TEST_METHOD(test_point_and_vector_add)
		{
			math::vector<int> a	{ 2, 2 };
			point<int> b	{ 10, 10 };

			auto c = a + b;
			point<int> expected_c{ 12, 12 };

			Assert::AreEqual(expected_c, c);
		}

		TEST_METHOD(test_point_and_vector_add_symetria)
		{
			math::vector<int> a{ 2, 2 };
			point<int> b{ 10, 10 };

			auto c = a + b;
			auto d = b + a;

			Assert::AreEqual(c, d);
		}

		TEST_METHOD(test_point_and_vector_sub_symetria)
		{
			math::vector<int> a{ 2, 2 };
			math::vector<int> b{ 10, 10 };

			auto c = a - b;
			auto d = b - a;

			Assert::AreEqual(c, -d);
		}
	};
}
