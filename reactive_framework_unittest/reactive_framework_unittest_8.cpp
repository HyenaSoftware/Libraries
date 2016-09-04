
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
#include <reactive_framework8\rv>
auto a = _1;

namespace reactive_framework8_unittest
{
	using namespace reactive_framework8;
	using namespace utility;

	TEST_CLASS(reactive_framework8_high_level_unittest)
	{
	public:
		typedef int id_type;

		template<class T> static const T& get(const boost::optional<T>& opt_)
		{
			Assert::IsTrue(opt_.is_initialized());

			return *opt_;
		}

		TEST_METHOD(TestMap)
		{
			rv_context rc;

			rv<int> a;

			rc.debugger().set_name(a, "a");

			auto MAP_FUNC = [](int n_) {return static_cast<float>(n_); };

			rv<float> b = rc.map(MAP_FUNC, a);
			rc.debugger().set_name(b, "b");

			// int -> float
			a << 5;

			const float expected_value = MAP_FUNC(5);
			const float given_value = get(b.value());

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestSplit)
		{
			using namespace std::placeholders;

			rv_context rc;

			rv<vector<int>> a;
			rv<int> b, c;

			rc.debugger().set_name(a, "a");
			rc.debugger().set_name(b, "b");
			rc.debugger().set_name(c, "c");

			auto selector = [](vector<int> vec_, size_t i_) { return (vec_.size() > i_) ? vec_[i_] : 0; };

			rc.map([&](vector<int> v_) { return selector(v_, 0); }, a).into(b);
			rc.map([&](vector<int> v_) { return selector(v_, 1); }, a).into(c);

			vector<int> v{ 3, 5 };

			a = v;

			int val_of_b = get(b.value());
			int val_of_c = get(c.value());

			Assert::AreEqual(3, val_of_b);
			Assert::AreEqual(5, val_of_c);

			v[0] = 7;
			a = v;

			int val_of_b2 = get(b.value());
			int val_of_c2 = get(c.value());

			Assert::AreEqual(7, val_of_b2);
			Assert::AreEqual(5, val_of_c2);
		}

		TEST_METHOD(TestSplit2)
		{
			using namespace std;
			using namespace std::placeholders;

			auto selector = [](vector<int> vec_, size_t i_) { return (vec_.size() > i_) ? vec_[i_] : 0; };

			rv_context rc;
			rv<vector<int>> a;

			rv<int> b = rc.map([&](vector<int> v_) { return selector(v_, 0); }, a);
			rv<int> c = rc.map([&](vector<int> v_) { return selector(v_, 1); }, a);

			vector<int> v{ 3, 5 };
			a = v;

			int val_of_b = get(b.value());
			int val_of_c = get(c.value());

			Assert::AreEqual(3, val_of_b);
			Assert::AreEqual(5, val_of_c);


			vector<int> v2{ 11, 17, 101, 49 };

			rv<int> d = rc.map([&](vector<int> v_) { return selector(v_, 2); }, a);
			rv<int> e = rc.map([&](vector<int> v_) { return selector(v_, 3); }, a);

			a = v2;

			int val_of_b2 = get(b.value());
			int val_of_c2 = get(c.value());
			int val_of_d2 = get(d.value());
			int val_of_e2 = get(e.value());

			Assert::AreEqual(11, val_of_b2);
			Assert::AreEqual(17, val_of_c2);
			Assert::AreEqual(101, val_of_d2);
			Assert::AreEqual(49, val_of_e2);
		}

		TEST_METHOD(TestMerge)
		{
			using namespace std;

			rv_context rc;
			rv<int> a, b;

			rv<vector<int>> c
			{
				rc.zip([](int a_, int b_)
				{
					return vector<int> { a_, b_ };
				}, a, b)
			};

			a = 3;
			b = 7;

			std::vector<int> val_of_c = get(c.value());
			const std::vector<int> expected_c_value{ 3, 7 };
			Assert::AreEqual(expected_c_value, val_of_c);

			a = 5;
			b = 7;

			val_of_c = get(c.value());
			const std::vector<int> expected_c_value2{ 5, 7 };
			Assert::AreEqual(expected_c_value2, val_of_c);
		}

		TEST_METHOD(TestMerge2)
		{
			using namespace std;

			rv_context rc;
			rv<int> a, b, c, d;

			rv<vector<int>> e = rc.zip([](int a_, int b_) { return vector<int> { a_, b_ }; }, a, b);

			a = 808;
			b = 1919;

			std::vector<int> value = get(e.value());
			const std::vector<int> expected_value{ 808, 1919 };
			Assert::AreEqual(expected_value, value);

			e = std::vector<int>{ 11 };

			rv<vector<int>> f = rc.zip([](vector<int> e_, int a_, int b_)
			{
				vector<int> res{ a_, b_ };
				res.insert(res.begin(), e_.begin(), e_.end());
				return res;
			}, e, c, d);

			a = 51;
			b = 7;
			c = 64;
			d = 1337;

			value = get(f.value());
			const std::vector<int> expected_value2{ 51, 7, 64, 1337 };
			Assert::AreEqual(expected_value2, value);
		}

		TEST_METHOD(TestJoinHeterogen)
		{
			rv_context rc;

			rv<float> a;
			rv<char> b;

			rv<tuple<float, char>> c = rc.zip(make_tuple<float, char>, a, b);

			a = 4.f;
			b = 'b';

			tuple<float, char> val_of_c = get(c.value());
			const tuple<float, char> expected_c_value{ 4.f, 'b' };
			Assert::AreEqual(expected_c_value, val_of_c);
		}

		TEST_METHOD(TestJoinHomogen)
		{
			rv_context rc;

			rv<int> a;
			rv<int> b;
			rv<tuple<int, int>> c {	rc.zip(make_tuple<int, int>, a, b) };

			a = 4;
			b = 5;

			tuple<int, int> val_of_c = get(c.value());
			const tuple<int, int> expected_c_value{ 4, 5 };
			Assert::AreEqual(expected_c_value, val_of_c);
		}

		TEST_METHOD(TestChaining)
		{
			rv_context rc;
			rv<int> a;

			auto inc = [](int n_)
			{
				return n_ + 1;
			};

			rv<int> b = rc.map(inc, a);
			rv<int> c = rc.map(inc, b);

			int cb_value_of_b = -1;
			int cb_value_of_c = -1;

			bool b_was_updated = false, c_was_updated = false;
			b.subscribe([&](int n_) { b_was_updated = true; cb_value_of_b = n_; });
			c.subscribe([&](int n_) { c_was_updated = true; cb_value_of_c = n_; });

			a = 1;

			Assert::IsTrue(b_was_updated);
			Assert::IsTrue(c_was_updated);

			Assert::AreEqual(2, cb_value_of_b);
			Assert::AreEqual(3, cb_value_of_c);

			int value_of_b = *b.value();
			int value_of_c = *c.value();

			Assert::AreEqual(2, value_of_b);
			Assert::AreEqual(3, value_of_c);
		}

		TEST_METHOD(TestLoop)
		{
			rv_context rc;

			//
			//	set max count of loop for the RC
			//
			constexpr int max_single_chain_recompute = 5;

			auto inc = [](int n_) { return n_ + 1; };
			auto pass_throught = [](int n_) { return n_; };

			int loops = 0;

			rv<int> a;
			rv<int> b = rc.map(inc, a);
			a = rc.filter([&](int) ->bool
			{
				return max_single_chain_recompute > ++loops;
			}, b);

			//
			std::vector<int> values_of_a, values_of_b;

			a.subscribe([&](int n_)
			{
				values_of_a.push_back(n_);
			});

			b.subscribe([&](int n_)
			{
				values_of_b.push_back(n_);
			});

			//
			a = 1;

			//
			//	it depends on the policy settings of the RC
			//
			const std::vector<int> expected_values_of_a{ 1, 2, 3, 4, 5 };
			const std::vector<int> expected_values_of_b{ 2, 3, 4, 5, 6 };

			Assert::AreEqual(expected_values_of_a, values_of_a);
			Assert::AreEqual(expected_values_of_b, values_of_b);
		}

		TEST_METHOD(TestBridgeNodes)
		{
			auto MAP_FUNC = [](int n_)
			{
				return n_ + 1;
			};

			rv_context rc;
			rv<int> a, c;

			// sub scope
			{
				rv<int> b = rc.map(MAP_FUNC, a);

				c = rc.map(MAP_FUNC, b);
			}

			a = 1;

			const int expected_c_value = 3;
			const int c_value = get(c.value());

			Assert::AreEqual(expected_c_value, c_value);
		}

		TEST_METHOD(TestChainPreSet)
		{
			auto MAP_FUNC = [](int n_)
			{
				return n_ + 1;
			};

			rv_context rc;

			rv<int> a = 1;

			rv<int> b = rc.map(MAP_FUNC, a);
			rv<int>	c = rc.map(MAP_FUNC, b);

			const int expected_b_value = 2;
			const int b_value = get(b.value());

			Assert::AreEqual(expected_b_value, b_value);

			const int expected_c_value = 3;
			const int c_value = get(c.value());

			Assert::AreEqual(expected_c_value, c_value);
		}

		TEST_METHOD(test_multiset)
		{
			rv_context rc;

			rv<int> a { 10 };
			rv<int> b { 2 };

			rv<tuple<int, int>> c = rc.zip(make_tuple<int, int>, a, b);

			tuple<int, int> EXPECTED_C{ 10, 2 };
			auto VALUE_OF_C = get(c.value());
			Assert::AreEqual(EXPECTED_C, VALUE_OF_C);

			//
			a = 20;

			tuple<int, int> EXPECTED_C2{ 20, 2 };
			auto VALUE_OF_C2 = get(c.value());
			Assert::AreEqual(EXPECTED_C2, VALUE_OF_C2);

			//
			b = 5;

			tuple<int, int> EXPECTED_C3{ 20, 5 };
			auto VALUE_OF_C3 = get(c.value());
			Assert::AreEqual(EXPECTED_C3, VALUE_OF_C3);
		}

		TEST_METHOD(test_filtering)
		{
			rv_context rc;

			rv<char> a { 'a' };
			rv<char> b { 'a' };

			rv<tuple<char, char>> c = rc.zip(make_tuple<char, char>,
				rc.filter([](char b_)
				{
					return b_ != 'b';
				}, a).as<char>(), b);

			//
			Assert::AreEqual(make_tuple('a', 'a'), get(c.value()));

			//
			a = 'b';
			Assert::AreEqual(make_tuple('a', 'a'), get(c.value()));

			//
			b = 'b';
			Assert::AreEqual(make_tuple('a', 'b'), get(c.value()));

			//
			a = 'c';
			Assert::AreEqual(make_tuple('c', 'b'), get(c.value()));
		}

		TEST_METHOD(test_pointer_filering)
		{
			typedef int pointer;

			rv_context rc;

			rv<pointer> a { 0 };

			Assert::AreEqual(0, get(a.value()));

			rv<int> b = rc.filter([](pointer ptr_) { return ptr_ != 0; }, a);

			Assert::IsFalse(b.value().is_initialized());

			a = 1;

			Assert::AreEqual(1, get(b.value()));

			a = 0;

			Assert::AreEqual(1, get(b.value()));
		}
	};
}

#endif

#pragma warning(pop)
