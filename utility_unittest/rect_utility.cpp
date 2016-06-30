#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace utility;



namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
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

			template<class T> std::wstring ToString(const rect<T>& t_)
			{
				wstringstream ws;

				ws << "{ [" << t_.left << "; " << t_.top << "] -> ["
					<< t_.right << "; " << t_.bottom << "] }";

				return ws.str();
			}
		}
	}
}


namespace utility_unittest
{
	TEST_CLASS(rect_unittest)
	{
	public:
		TEST_METHOD(TestRect_Diagonal)
		{
			rect<int> r
			{
				{ 0, 0 },
				{ 2, 2 }
			};

			auto p = r.diagonal_vec();

			const math::vector<int> expected_center{ 2, 2 };

			Assert::AreEqual(expected_center, p);
		}

		TEST_METHOD(TestRect_Center)
		{
			rect<int> r
			{
				{ 0, 0 },
				{ 10, 5 }
			};

			auto p = static_cast<math::point<int>>(r.center());

			const math::point<int> expected_center { 5, 3 };

			Assert::AreEqual(expected_center, p);
		}

		TEST_METHOD(test_rect_intersect)
		{
			rect<int> p
			{
				{ 0, 0 },
				{ 2, 2 }
			};

			rect<int> q
			{
				{ 1, 1 },
				{ 3, 3 }
			};

			auto r = intersect(p, q);

			rect<int> expected_r
			{
				{ 1, 1},
				{ 2, 2}
			};

			Assert::AreEqual(expected_r, r);
		}

		TEST_METHOD(test_rect_intersect_check_area)
		{
			rect<int> p
			{
				{ 0, 0 },
				{ 2, 2 }
			};

			rect<int> q
			{
				{ 4, 4 },
				{ 6, 6 }
			};

			auto r = intersect(p, q);

			Assert::AreEqual(4, r.area());
		}

		TEST_METHOD(test_ordering)
		{
			math::point<int> p { 0, 0 }, q { 5, 5 };

			auto r = make_rect(p, q);

			Assert::IsTrue(r.left < r.right);
			Assert::IsTrue(r.top < r.bottom);
		}

		TEST_METHOD(test_ordering2)
		{
			math::point<int> p{ 0, 0 }, q{ 5, 5 };

			auto r = make_rect(p.x, q.x, p.y, q.y);

			Assert::IsTrue(r.left < r.right);
			Assert::IsTrue(r.top < r.bottom);
		}

		TEST_METHOD(test_rect_zero_intersect)
		{
			rect<int> p
			{
				{ 0, 0 },
				{ 2, 2 }
			};

			rect<int> q
			{
				{ 2, 2 },
				{ 4, 4 }
			};

			auto r = intersect(p, q);

			rect<int> expected_r
			{
				{ 2, 2 },
				{ 2, 2 }
			};

			const int expected_area { 0 };

			Assert::AreEqual(expected_r, r);
			Assert::AreEqual(expected_area, r.area());
		}
	};
}
