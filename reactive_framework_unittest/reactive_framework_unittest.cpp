#include "stdafx.h"

#pragma warning(push)
#pragma warning(disable: 4996)

#include "CppUnitTest.h"
#include "..\reactive_framework\reactive_framework.h"
#include "..\reactive_framework\reactive_framework5.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
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
	using namespace reactive_framework;

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

namespace reactive_framework5_unittest
{
	using namespace reactive_framework5;

	TEST_CLASS(reactive_framework5_graph_unittest)
	{
	public:
		TEST_METHOD(TestGraphNodeConnection)
		{
			int value_in_lambda = -1;

			constexpr int DELTA = 1;

			auto PASS_THROUGH = [&](int n_)
			{
				value_in_lambda = n_;
				return n_ + DELTA;
			};

			auto a = std::make_shared<typed_node<int>>();
			auto b = std::make_shared<typed_node<int>>();
			auto e = std::make_shared<typed_edge<int, int>>(PASS_THROUGH);

			int result = -1;

			b->on_changed.push_back([&](int v_)
			{
				result = v_;
			});

			graph g;

			g.connect(a, e, b);

			constexpr int TEST_VALUE = 567;

			a->set(TEST_VALUE);

			Assert::AreEqual(TEST_VALUE, value_in_lambda);

			Assert::AreEqual(TEST_VALUE + DELTA, b->get());

			Assert::AreEqual(TEST_VALUE + DELTA, result);
		}

		TEST_METHOD(TestGraphCallback)
		{
			typed_node<int> a;

			int result = -1;

			a.on_changed.push_back([&](int v_)
			{
				result = v_;
			});

			a.set(42);

			Assert::AreEqual(42, result);

			Assert::AreEqual(42, a.get());
		}

		TEST_METHOD(TestMultipleOut)
		{
			constexpr int TEST_VALUE = 567;
			constexpr int DELTA = 1;
			int value_in_lambda = -1;

			auto PASS_THROUGH = [&](int n_)
			{
				value_in_lambda = n_;
				return n_ + DELTA;
			};

			auto a = std::make_shared<typed_node<int>>();
			auto b = std::make_shared<typed_node<int>>();
			auto c = std::make_shared<typed_node<int>>();
			auto d = std::make_shared<typed_node<int>>();

			a->set_id(101);
			b->set_id(101);
			c->set_id(102);
			d->set_id(103);

			graph g;
			g.connect(a, std::make_shared<typed_edge<int, int>>(PASS_THROUGH), b);
			g.connect(a, std::make_shared<typed_edge<int, int>>(PASS_THROUGH), c);
			g.connect(a, std::make_shared<typed_edge<int, int>>(PASS_THROUGH), d);


			int results[] = { -1, -1, -1};
			auto writer = [&](int value_, int i_)
			{
				results[i_] = value_;
			};

			b->on_changed.push_back(std::bind(writer, std::placeholders::_1, 0));
			c->on_changed.push_back(std::bind(writer, std::placeholders::_1, 1));
			d->on_changed.push_back(std::bind(writer, std::placeholders::_1, 2));

			a->set(TEST_VALUE);

			Assert::AreEqual(TEST_VALUE, value_in_lambda);

			Assert::AreEqual(TEST_VALUE + DELTA, b->get());
			Assert::AreEqual(TEST_VALUE + DELTA, c->get());
			Assert::AreEqual(TEST_VALUE + DELTA, d->get());

			Assert::AreEqual(TEST_VALUE + DELTA, results[0]);
			Assert::AreEqual(TEST_VALUE + DELTA, results[1]);
			Assert::AreEqual(TEST_VALUE + DELTA, results[2]);
		}
	};

	TEST_CLASS(reactove_framework5_test_DSL)
	{
	public:
		TEST_METHOD(TestFrom)
		{
			reactive_context rvc;

			rv<int> a;

			auto builder = rvc.from(a);

			Assert::IsNotNull(context_of(builder)._ptr_src_rv);
			Assert::IsNull(context_of(builder)._ptr_dst_rv);

			Assert::IsNotNull(context_of(builder)._src_node.get());
			Assert::IsNull(context_of(builder)._dst_node.get());

			Assert::IsNull(context_of(builder)._edge.get());

			Assert::IsFalse(context_of(builder).is_complete());
		}

		TEST_METHOD(TestMap)
		{
			reactive_context rvc;

			rv<int> a;

			auto PASS_THROUGH = [](int n_){return n_;};

			auto builder = rvc.map(PASS_THROUGH);

			auto& ctx = context_of(builder);

			Assert::IsNull(ctx._dst_node.get());
			Assert::IsNull(ctx._src_node.get());

			Assert::IsNull(ctx._ptr_dst_rv);
			Assert::IsNull(ctx._ptr_src_rv);

			Assert::IsNotNull(ctx._edge.get());

			Assert::IsFalse(context_of(builder).is_complete());
		}

		TEST_METHOD(TestFromMap)
		{
			auto PASS_THROUGH = [](int n_) {return n_; };

			reactive_context rvc;

			rv<int> a;

			auto builder = rvc.from(a).map(PASS_THROUGH);

			Assert::IsNotNull(context_of(builder)._ptr_src_rv);
			Assert::IsNull(context_of(builder)._ptr_dst_rv);

			Assert::IsNotNull(context_of(builder)._src_node.get());
			Assert::IsNull(context_of(builder)._dst_node.get());

			Assert::IsNotNull(context_of(builder)._edge.get());

			Assert::IsFalse(context_of(builder).is_complete());
		}

		TEST_METHOD(TestMapInto)
		{
			auto PASS_THROUGH = [](int n_) {return n_; };

			reactive_context rvc;

			rv<int> a;

			auto builder = rvc.map(PASS_THROUGH).into(a);

			Assert::IsNull(context_of(builder)._ptr_src_rv);
			Assert::IsNotNull(context_of(builder)._ptr_dst_rv);

			Assert::IsNull(context_of(builder)._src_node.get());
			Assert::IsNotNull(context_of(builder)._dst_node.get());

			Assert::IsNotNull(context_of(builder)._edge.get());

			Assert::IsFalse(context_of(builder).is_complete());
		}

		TEST_METHOD(TestFromMapInto)
		{
			auto PASS_THROUGH = [](int n_) {return n_; };

			reactive_context rvc;

			rv<int> a, b;

			auto builder = rvc.from(a).map(PASS_THROUGH).into(b);

			Assert::IsNull(context_of(builder)._ptr_src_rv);
			Assert::IsNull(context_of(builder)._ptr_dst_rv);

			Assert::IsNull(context_of(builder)._src_node.get());
			Assert::IsNull(context_of(builder)._dst_node.get());

			Assert::IsNull(context_of(builder)._edge.get());

			Assert::IsFalse(context_of(builder).is_complete());
		}

		TEST_METHOD(TestIncompleteBuilder)
		{
			auto PASS_THROUGH = [](int n_) {return n_; };

			reactive_context rvc;

			rv<int> a;

			rv_abstract_builder<undefined_type, int, int, int> builder = rvc.map(PASS_THROUGH).into(a);
			auto& ctx = context_of(builder);

			Assert::IsNull(context_of(builder)._ptr_src_rv);
			Assert::IsNotNull(context_of(builder)._ptr_dst_rv);

			Assert::IsNull(context_of(builder)._src_node.get());
			Assert::IsNotNull(context_of(builder)._dst_node.get());

			Assert::IsNotNull(context_of(builder)._edge.get());

			Assert::IsFalse(context_of(builder).is_complete());
		}

	};

	template<class R, class A> class selector
	{
	public:
		typedef R nested_result_type;
		typedef A nested_arg0_type;

		selector(const selector&) = default;

		template<class F> selector(F f_, int i_) : _i{i_}, _f{f_} {}

		nested_result_type operator()(std::vector<nested_arg0_type>& v_)
		{
			return _f(v_[_i]);
		}

	private:
		int _i;
		std::function<nested_result_type (nested_arg0_type)> _f;
	};

	template<class F> auto make_selector(F f_, int i_)
	{
		typedef typename Utility::function_traits<F>::template arg<0>::type nested_arg0_type;
		typedef typename Utility::function_traits<F>::result_type nested_result_type;

		static_assert(Utility::has_operator_call<F>::value, "");

		return selector<nested_result_type, nested_arg0_type>{ f_, i_};
	}


	/*
		- event handling
		- rv
		- mapper functions which has different input/output types
		- DSL test in large scale
		- DSL test case: 
			rvc.from(a).map(PASS_THROUGH).into(b).into(c);
			a ----> b
				\-> c
	*/
	TEST_CLASS(reactive_framework5_high_level_unittest)
	{
	public:
		TEST_METHOD(TestMap)
		{
			// + traits
			reactive_context rvc;

			rv<int> a;
			rv<float> b;

			auto MAP_FUNC = [](int n_) {return static_cast<float>(n_); };

			rvc
				.from(a)
				.map(MAP_FUNC)
				.into(b)
				;

			// int -> float

			a = 5;
			const float expected_value = MAP_FUNC(5);
			const float given_value = b;

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestMap2)
		{
			auto MAP_FUNC = [](int n_) {return static_cast<float>(n_); };
			
			
			reactive_context rvc;

			rv<int> a;
			rv<float> b;

			float given_value;
			
			b.on_changed.push_back([&given_value](float v_)
			{
				given_value = v_;
			});

			rvc.from(a).map(MAP_FUNC).into(b);

			// int -> float

			a = 5;
			const float expected_value = MAP_FUNC(5);

			Assert::AreEqual(expected_value, given_value);
		}

		TEST_METHOD(TestSplit)
		{
			reactive_context rvc;

			rv<vector<int>> a;
			rv<int> b, c;

			rvc.from(a).split
			(
				rvc.into(b),
				rvc.into(c)
			);

			vector<int> v { 3, 5 };
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

		TEST_METHOD(TestMerge)
		{
			rv<int> a, b;
			rv<vector<int>> c;

			reactive_context rvc;
			rvc.merge
			(
				rvc.from(a),
				rvc.from(b)
			).into(c);

			a = 3;
			b = 7;

			std::vector<int> val_of_c = c;
			const std::vector<int> expected_c_value {3, 7};
			Assert::AreEqual(expected_c_value, val_of_c);

			a = 5;
			b = 7;

			val_of_c = c;
			const std::vector<int> expected_c_value2 { 5, 7 };
			Assert::AreEqual(expected_c_value2, val_of_c);
		}

		TEST_METHOD(TestCross)
		{
			auto to_float = [](int v_){return static_cast<float>(v_);};
			auto to_int = [](float v_){return static_cast<int>(v_);};

			make_selector(to_int, 0);

			reactive_context rc;

			rv<int> a;
			rv<int> b;

			rv<int> d;
			rv<int> e;

			rv<std::vector<float>> c;

			std::function<int(std::vector<float>&)> func;

			rc
				.merge
				(
					rc.from(a),
					rc.from(b)
				)
				.into(c);

			rc.from(c)
				.split
				(
					//rc.map(func).into(d)
					//rc.map(make_selector(to_int, 0)).into(d)
					//rc.map(make_selector(to_int, 0)).into(d),
					//rc.map(make_selector(to_int, 1)).into(e)
				);

			a = 3;

			Assert::AreEqual(3, static_cast<int>(d));
			Assert::AreEqual(0, static_cast<int>(e));

			b = 5;

			Assert::AreEqual(3, static_cast<int>(d));
			Assert::AreEqual(5, static_cast<int>(e));

			DBGBREAK
		}
	};
}

#pragma warning(pop)
