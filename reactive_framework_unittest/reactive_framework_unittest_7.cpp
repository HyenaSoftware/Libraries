
#include "stdafx.h"

#pragma warning(push)
#pragma warning(disable: 4996)

#include "CppUnitTest.h"
#include "unittest_converters.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;


#define ENABLE_REACTIVE_FRAMEWORK_7_TEST true
#define ENABLE_REACTIVE_FRAMEWORK_7_GRAPH_TEST true



#if ENABLE_REACTIVE_FRAMEWORK_7_TEST
#include <reactive_framework7\reactive_framework7.h>


namespace reactive_framework7_unittest
{
	using namespace reactive_framework7;

	//TEST_CLASS(reactove_framework7_test_DSL)
	//{
	//public:

	//};

	TEST_CLASS(reactive_framework7_high_level_unittest)
	{
	public:
		typedef int id_type;

		TEST_METHOD(TestMap)
		{
			using namespace utility;
			default_reactive_context rc;

			// + traits
			rv<int, id_type> a{ rc };
			rv<float, id_type> b{ rc };

			auto MAP_FUNC = [](int n_) {return static_cast<float>(n_); };

			//b.on_changed[0] = bind_promise(pr_result);

			a.map(MAP_FUNC).into(b);

			// int -> float
			a = 5;

			const float expected_value = MAP_FUNC(5);
			float given_value = b.value();

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestMapTo)
		{
			using namespace utility;

			// + traits
			default_reactive_context rc;

			rv<int, id_type> a{ rc };

			auto MAP_FUNC = [](int n_) {return static_cast<float>(n_); };

			rv<float, id_type> b{ a.map(MAP_FUNC) };

			// int -> float

			a = 5;

			const float expected_value = MAP_FUNC(5);
			const float given_value = b;

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestMap2)
		{
			using namespace utility;

			default_reactive_context rc;

			auto MAP_FUNC = [](int n_) {return static_cast<float>(n_); };

			rv<int> a{ rc };
			rv<float> b{ rc };

			a.map(MAP_FUNC).into(b);

			float given_value;
			b.on_changed.insert(make_pair(0, [&given_value](float v_)
			{
				given_value = v_;
			}));


			// int -> float

			a = 5;

			const float expected_value = MAP_FUNC(5);

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestSplit)
		{
			using namespace std;
			using namespace std::placeholders;

			default_reactive_context rc;

			rv<vector<int>> a{ rc };
			rv<int> b{ rc }, c{ rc };

			auto selector = [](vector<int> vec_, size_t i_) { return (vec_.size() > i_) ? vec_[i_] : 0; };

			a.map<int>(bind(selector, _1, 0)).into(b);
			a.map<int>(bind(selector, _1, 1)).into(c);

			vector<int> v{ 3, 5 };

			a = v;

			int val_of_b = b;
			int val_of_c = c;

			Assert::AreEqual(3, val_of_b);
			Assert::AreEqual(5, val_of_c);

			v[0] = 7;
			a = v;

			int val_of_b2 = b;
			int val_of_c2 = c;

			Assert::AreEqual(7, val_of_b2);
			Assert::AreEqual(5, val_of_c2);
		}

		TEST_METHOD(TestSplit2)
		{
			using namespace std;
			using namespace std::placeholders;

			auto selector = [](vector<int> vec_, size_t i_) { return (vec_.size() > i_) ? vec_[i_] : 0; };

			default_reactive_context rc;
			rv<vector<int>> a{ rc };

			auto b = a.map<int>(bind(selector, _1, 0));
			auto c = a.map<int>(bind(selector, _1, 1));

			vector<int> v{ 3, 5 };
			a = v;

			int val_of_b = b;
			int val_of_c = c;

			Assert::AreEqual(3, val_of_b);
			Assert::AreEqual(5, val_of_c);


			vector<int> v2{ 11, 17, 101, 49 };

			auto d = a.map<int>(bind(selector, _1, 2));
			auto e = a.map<int>(bind(selector, _1, 3));

			a = v2;

			int val_of_b2 = b;
			int val_of_c2 = c;
			int val_of_d2 = d;
			int val_of_e2 = e;

			Assert::AreEqual(11, val_of_b2);
			Assert::AreEqual(17, val_of_c2);
			Assert::AreEqual(101, val_of_d2);
			Assert::AreEqual(49, val_of_e2);
		}

		TEST_METHOD(TestMerge)
		{
			using namespace std;

			default_reactive_context rc;
			rv<int> a{ rc }, b{ rc };

			rv<vector<int>, id_type> c
			{
				zip([](int a_, int b_)
			{
				return vector<int> { a_, b_ };
			}, a, b)
			};

			a = 3;
			b = 7;

			std::vector<int> val_of_c = c;
			const std::vector<int> expected_c_value{ 3, 7 };
			Assert::AreEqual(expected_c_value, val_of_c);

			a = 5;
			b = 7;

			val_of_c = c;
			const std::vector<int> expected_c_value2{ 5, 7 };
			Assert::AreEqual(expected_c_value2, val_of_c);
		}

		TEST_METHOD(TestMerge2)
		{
			using namespace std;

			default_reactive_context rc;
			rv<int> a{ rc }, b{ rc }, c{ rc }, d{ rc };

			auto e = zip([](int a_, int b_) { return vector<int> { a_, b_ }; }, a, b);

			a = 808;
			b = 1919;

			std::vector<int> value = e;
			const std::vector<int> expected_value{ 808, 1919 };
			Assert::AreEqual(expected_value, value);

			e = std::vector<int>{ 11 };

			auto f = zip([](vector<int> e_, int a_, int b_)
			{
				vector<int> res{ a_, b_ };
				res.insert(res.begin(), e_.begin(), e_.end());
				return res;
			}, e, c, d);

			a = 51;
			b = 7;
			c = 64;
			d = 1337;

			value = f;
			const std::vector<int> expected_value2{ 51, 7, 64, 1337 };
			Assert::AreEqual(expected_value2, value);
		}

		TEST_METHOD(TestJoinHeterogen)
		{
			default_reactive_context rc;

			rv<float, id_type> a{ rc };
			rv<char, id_type> b{ rc };

			auto c = zip(make_tuple<float, char>, a, b);

			a = 4.f;
			b = 'b';

			tuple<float, char> val_of_c = c;
			const tuple<float, char> expected_c_value{ 4.f, 'b' };
			Assert::AreEqual(expected_c_value, val_of_c);
		}

		TEST_METHOD(TestJoinHomogen)
		{
			default_reactive_context rc;

			rv<int> a{ rc };
			rv<int> b{ rc };
			rv<tuple<int, int>> c{ rc };

			zip(make_tuple<int, int>, a, b).into(c);

			a = 4;
			b = 5;

			tuple<int, int> val_of_c = c;
			const tuple<int, int> expected_c_value{ 4, 5 };
			Assert::AreEqual(expected_c_value, val_of_c);
		}

		TEST_METHOD(TestChaining)
		{
			default_reactive_context rc;
			rv<int> a{ rc };

			auto inc = [](int n_)
			{
				return n_ + 1;
			};

			auto b = a.map(inc);
			auto c = b.map(inc);

			int cb_value_of_b = -1;
			int cb_value_of_c = -1;

			bool b_was_updated = false, c_was_updated = false;
			b.on_changed[0] = [&](int n_) { b_was_updated = true; cb_value_of_b = n_; };
			c.on_changed[0] = [&](int n_) { c_was_updated = true; cb_value_of_c = n_; };

			a = 1;

			Assert::IsTrue(b_was_updated);
			Assert::IsTrue(c_was_updated);

			Assert::AreEqual(2, cb_value_of_b);
			Assert::AreEqual(3, cb_value_of_c);

			int value_of_b = b;
			int value_of_c = c;

			Assert::AreEqual(2, value_of_b);
			Assert::AreEqual(3, value_of_c);
		}

		TEST_METHOD(TestLoop)
		{
			default_reactive_context rc;
			//
			//	set max count of loop for the RC
			//
			rc.policy().set_max_single_chain_recompute(9);

			rv<int, id_type> a{ rc };

			auto inc = [](int n_)
			{
				return n_ + 1;
			};

			auto pass_throught = [](int n_) { return n_; };

			auto b = a.map(inc);
			b.into(a);

			//
			std::vector<int> values_of_a, values_of_b;

			a.on_changed[0] = [&](int n_)
			{
				values_of_a.push_back(n_);
			};

			b.on_changed[0] = [&](int n_)
			{
				values_of_b.push_back(n_);
			};

			//
			a = 1;

			//
			//	it depends on the policy settings of the RC
			//
			const std::vector<int> expected_values_of_a{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
			const std::vector<int> expected_values_of_b{ 2, 3, 4, 5, 6, 7, 8, 9, 10 };

			Assert::AreEqual(expected_values_of_a, values_of_a);
			Assert::AreEqual(expected_values_of_b, values_of_b);
		}

		TEST_METHOD(TestBridgeNodes)
		{
			auto MAP_FUNC = [](int n_)
			{
				return n_ + 1;
			};

			default_reactive_context rc;
			rv<int> a{ rc }, c{ rc };

			// sub scope
			{
				rv<int> b = a.map(MAP_FUNC);

				c = b.map(MAP_FUNC);
			}

			a = 1;

			const int expected_c_value = 3;
			const int c_value = c;

			Assert::AreEqual(expected_c_value, c_value);
		}

		TEST_METHOD(TestChainPreSet)
		{
			auto MAP_FUNC = [](int n_)
			{
				return n_ + 1;
			};

			default_reactive_context rc;

			rv<int> a{ rc };

			a = 1;

			rv<int> b = a.map(MAP_FUNC);
			rv<int>	c = b.map(MAP_FUNC);

			const int expected_b_value = 2;
			const int b_value = b;

			Assert::AreEqual(expected_b_value, b_value);

			const int expected_c_value = 3;
			const int c_value = c;

			Assert::AreEqual(expected_c_value, c_value);
		}

		TEST_METHOD(test_multiset)
		{
			default_reactive_context rc;

			rv<int> a{ rc, 10 };
			rv<int> b{ rc, 2 };

			auto c = zip(make_tuple<int, int>, a, b);

			tuple<int, int> EXPECTED_C{ 10, 2 };
			auto VALUE_OF_C = c.value();
			Assert::AreEqual(EXPECTED_C, VALUE_OF_C);

			//
			a = 20;

			tuple<int, int> EXPECTED_C2{ 20, 2 };
			auto VALUE_OF_C2 = c.value();
			Assert::AreEqual(EXPECTED_C2, VALUE_OF_C2);

			//
			b = 5;

			tuple<int, int> EXPECTED_C3{ 20, 5 };
			auto VALUE_OF_C3 = c.value();
			Assert::AreEqual(EXPECTED_C3, VALUE_OF_C3);
		}

		TEST_METHOD(test_filtering)
		{
			default_reactive_context rc;

			rv<char> a{ rc, 'a' };
			rv<char> b{ rc, 'a' };

			auto c = zip(make_tuple<char, char>,
				a.filter([](char c_)
			{
				return c_ != 'b';
			}), b);

			//
			Assert::AreEqual(make_tuple('a', 'a'), c.value());

			//
			a = 'b';
			Assert::AreEqual(make_tuple('a', 'a'), c.value());

			//
			b = 'b';
			Assert::AreEqual(make_tuple('a', 'b'), c.value());

			//
			a = 'c';
			Assert::AreEqual(make_tuple('c', 'b'), c.value());
		}

		TEST_METHOD(test_pointer_filering)
		{
			typedef int pointer;

			default_reactive_context rc;

			rv<pointer> a{ rc, 0 };

			Assert::AreEqual(0, a.value());

			rv<int> b = a.filter([](pointer ptr_) { return ptr_ != 0; });

			Assert::AreEqual(0, b.value());	// what should it be ?
		}
	};
}

#endif

#pragma warning(pop)
