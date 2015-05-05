#include "stdafx.h"

#pragma warning(push)
#pragma warning(disable: 4996)

#include "CppUnitTest.h"
#include "..\reactive_framework\reactive_framework.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace reactive_framework;
using namespace std;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			std::wstring ToString(const std::vector<int>& vec_)
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

			struct _ToStringTupleImp
			{
				wstringstream& _ws;
				_ToStringTupleImp(wstringstream& ws_) : _ws{ ws_ } {}
				template<class T> void operator()(T& t_) const
				{
					_ws << t_ << ", ";
				}
			};

			template<class... Ts> std::wstring ToString(const std::tuple<Ts...>& ts_)
			{
				wstringstream ws;

				ws << "[";

				Utility::for_each(ts_, _ToStringTupleImp{ws});

				if (std::tuple_size<std::tuple<Ts...>>::value > 0)
				{
					ws.seekp(-2, ios_base::end);
				}

				ws << "]";

				return ws.str();
			}

			template<class A, class B> std::wstring ToString(const std::pair<A, B>& pair_)
			{
				wstringstream ws;

				ws << "[" << pair_.first << "; " << pair_.second << "]";

				return ws.str();
			}
		}
	}
}

namespace reactive_framework_unittest
{
	TEST_CLASS(reactive_framework_unittest)
	{
	public:
		
		TEST_METHOD(TestMerge)
		{
			// + traits
			rv_leaf<int> a, b, c, d;
			rv_readonly<vector<int>> e;

			// T, T, T -> vector<T>
			merge( a, b, c ).to(e);

			const vector<int> expected_value { 0, 0, 0 };
			vector<int> given_value = e;

			Assert::AreEqual(expected_value, given_value, L"Value mismatch");

			a = 1;
			b = 2;
			c = 3;
			d = 4;

			//e.as_merger().push_back(d);

			const vector<int> expected_value2 { 1,2,3 };
			given_value = e;

			Assert::AreEqual(expected_value2, given_value, L"Value mismatch");
		}

		TEST_METHOD(TestMap)
		{
			// + traits

			rv_leaf<int> a;
			rv_readonly<float> b;

			map(a, MAP_FUNC).to(b);

			// int -> float

			a = 5;
			const float expected_value = MAP_FUNC(5);
			const float given_value = b;

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestFlatten)
		{
			// +traits

			rv_leaf<vector<vector<int>>> a;
			rv_readonly<vector<int>> b;

			const vector<vector<int>> test_value
			{
				{ 1, 2, 3 },
				{ 4, 5},
				{ 6 }
			};

			const vector<int> expected_value{ 1, 2, 3, 4, 5, 6 };

			// vector<vector<T>> -> vector<T>
			flatmap(a).to(b);

			a = test_value;

			const vector<int> given_value = b;

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestChain)
		{
			auto PASS_THROUGH = [](int n_) { return n_; };

			rv_leaf<int> a;
			rv_readonly<int> c;

			{
				rv<int> b = map(a, PASS_THROUGH).build();

				map(b, PASS_THROUGH).to(c);
			}

			const int expected_value = 5;

			a = expected_value;

			const int given_value = c;

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestAutoJoin)
		{
			rv_leaf<int> a;
			rv_leaf<float> b;
			rv<tuple<int, float>> c;

			// int, int -> tuple<int, int>
			c = join(a, b).build();

			a = 3;
			b = 5.f;

			const tuple<int, float> expected_value { 3, 5.f };
			const tuple<int, float> value_of_c = c;

			Assert::AreEqual(expected_value, value_of_c);
		}

		/*
			a = 4 -> b [=a] -> c [=b]

			map(a, _ * 2).to(b)

			a=5 -> b [= a*2]	  c [=<core>]
				\				 /
				 \				/
				   <core> [=a]
		*/
		TEST_METHOD(TestChaining)
		{
			rv_leaf<int> a;

			rv_readonly<int> b = map(a, PASS_THROUGH).build();

			rv<int> c = map(b, PASS_THROUGH).build();

			const int expected_value_of_a1 = 4;

			a = expected_value_of_a1;

			const int value_of_c1 = a();
			Assert::AreEqual(expected_value_of_a1, value_of_c1);

			// rebind
			map(a, DOUBLE_IT).to(b);

			const int value_of_a2 = 5;
			a = value_of_a2;

			const int expected_value_of_c = value_of_a2 * 2;
			const int value_of_c2 = c;
			Assert::AreEqual(expected_value_of_c, value_of_c2);
		}

		TEST_METHOD(TestBasic)
		{
			const int expected_value = 3;

			rv_leaf<int> a = expected_value;

			const int value_of_a = a;

			Assert::AreEqual(expected_value, value_of_a);
		}

		TEST_METHOD(TestLoop)
		{
			rv_leaf<int> a;

			rv<int> b = map(a, PASS_THROUGH).build();

			//map(b, PASS_THROUGH).to(a);

			a = 3;

			//Assert::Fail(L"TODO: Complete it.");
		}

	private:

		// simple
		static const function<double(int, float)> _JOIN_FUNC;
		static const function<float(int)> MAP_FUNC;
		static const function<int(int)> PASS_THROUGH;
		static const function<int(int)> DOUBLE_IT;
	};


	const function<float(int)> reactive_framework_unittest::MAP_FUNC { [](int n_) {return static_cast<float>(n_); } };
	const function<int(int)> reactive_framework_unittest::PASS_THROUGH { [](int n_) {return n_; } };
	const function<int(int)> reactive_framework_unittest::DOUBLE_IT { [](int n_) {return 2 * n_; } };
	const function<double(int, float)> reactive_framework_unittest::_JOIN_FUNC { [](int n_, float m_) {return n_ * m_; } };
}


#pragma warning(pop)
