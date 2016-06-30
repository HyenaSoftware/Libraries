#include "stdafx.h"
#include "CppUnitTest.h"
#include "unittest_helpers.hpp"
#include "unittest_converters.h"

#include <utility\stream_collections.hpp>
#include <utility\mirror.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace utility;


namespace utility_unittest
{
	enum E_POLICY
	{
		NON_COPY_ABLE
	};

	template<E_POLICY X, E_POLICY... Vs>				struct has_policy_of;
	template<E_POLICY X, E_POLICY Y, E_POLICY... Vs>	struct has_policy_of<X, Y, Vs...>	: has_policy_of<X, Vs...> { };
	template<E_POLICY X, E_POLICY... Vs>				struct has_policy_of<X, X, Vs...>	: true_type	{ };
	template<E_POLICY X>								struct has_policy_of<X>				: false_type { };


	template<E_POLICY... Ps> struct test_struct_generic
	{
		int f1 = 0;
		static int f2;

		test_struct_generic() = default;

		test_struct_generic(const test_struct_generic& other_)
		{
			static_assert(!has_policy_of<NON_COPY_ABLE, Ps...>::value, "The use of copy constructor is not allowed in this unittest");
		}

		test_struct_generic(int n_) : f1 { n_ }
		{
		}

		int add(int a_, int b_)
		{
			return a_ + b_;
		}

		static int sub(int a_, int b_)
		{
			return a_ - b_;
		}

		void inc_by(int n_)
		{
			f1 += n_;
		}
	};

	typedef test_struct_generic<> test_struct;

	static const string TEST_STRUCT_NAME { "test_struct" };
	//int test_struct::f2 = 0;
	template<E_POLICY... Ps> int test_struct_generic<Ps...>::f2 = 0;

	struct my_class_a
	{
		int f(int a_, int b_)
		{
			return a_ + b_;
		}

		virtual int g(int a_)
		{
			return a_;
		}
	};

	struct my_class_b : my_class_a
	{
		virtual int g(int a_) override
		{
			return -a_;
		}
	};


	TEST_CLASS(mirror_runtime_database_unittest)
	{
		TEST_METHOD(test_add_class)
		{
			runtime_database rt;

			// 1
			Assert::ExpectException<class_not_found_exception>([&rt]
			{
				rt.get_class("test_struct");
			}, L"it must throw class_not_found_exception");

			// 2
			rt.add(cpp_class_t
			{
				"test_struct",
				type_of<test_struct>()
			});

			auto& c1 = rt.get_class("test_struct");
			auto c2 = c1;
		}

		TEST_METHOD(test_runtime_database_static_init)
		{
			//execute_with_exception_dump([]
			//{
				runtime_database rt;

				rt.add(cpp_class_t
				{
					TEST_STRUCT_NAME,
					typeid(test_struct),
					make_constructor<test_struct>(),
					make_constructor<test_struct, int>(),
					method_t	{ "add", &test_struct::add },
					field_t		{ "f1", &test_struct::f1 }
 				});

				auto obj = rt.get_class(TEST_STRUCT_NAME).ctor(detail::cpp_ctor_key_t { { type_of<int>() } }).create({ object_ref { 1 } });
				
				Assert::IsTrue(type_index { typeid(test_struct) } == obj.type(), L"Created object must be an instance of test_struct");

				auto rv = rt.get_class(TEST_STRUCT_NAME)
					.method(detail::cpp_method_key_t { "add", { type_of<int>(), type_of<int>() }})
					.invoke(obj, { object_ref {2}, object_ref {3} });

				Assert::IsTrue(type_index{ typeid(int) } == rv.type(), L"return type must be int");

				auto val = rv.as<int>();

				Assert::AreEqual(5, val, L"return value must be 5");
			//});
		} 

		TEST_METHOD(test_runtime_database_dynamic_init)
		{
			runtime_database rt;

			/*rt.add(make_class<test_struct>(TEST_STRUCT_NAME)
				.set_ctor(make_constructor<test_struct>())
				.set_method({"add", &test_struct::add})
			);*/
			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				type_of<test_struct>(),
				constructor_t { type_tag<test_struct>{} },
				method_t { "add", &test_struct::add }
			});

			auto obj = rt.get_class(TEST_STRUCT_NAME).ctor(detail::cpp_ctor_key_t { }).create();

			auto rv = rt.get_class(TEST_STRUCT_NAME)
				.method(detail::cpp_method_key_t{ "add", { type_of<int>(), type_of<int>() } })
				.invoke(obj, { object_ref{ 2 }, object_ref{ 3 } });

			auto val = static_cast<int>(rv);

			Assert::AreEqual(5, val);
		}

		//
		//	tests for methods
		//

		TEST_METHOD(test_runtime_database_invoke)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				method_t { "add", &test_struct::add }
			});

			test_struct s;
			shared_ptr<test_struct> ptr_obj { detail::eternal_shared_ptr, &s };

			int a1 = 3;
			int a2 = 2;

			auto a = rt.get_class(TEST_STRUCT_NAME)
				.method(detail::cpp_method_key_t{ "add", { type_of<int>(), type_of<int>() }})
				.invoke(ptr_obj, { object_ref { 3 }, object_ref { 2 } });
			auto val = a.as<int>();

			Assert::AreEqual(5, val, L"invoked method must return with 5");
		}

		TEST_METHOD(test_runtime_database_invalid_invoke)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				method_t{ "add", &test_struct::add }
			});

			auto m = rt.get_class(TEST_STRUCT_NAME)
				.method(detail::cpp_method_key_t{ "add",{ type_of<int>(), type_of<int>() } });

			Assert::ExpectException<logic_error>([=]
			{
				m.invoke_static<int>(3, 2);
			});
		}

		TEST_METHOD(test_runtime_database_static_invoke)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				method_t { "sub", &test_struct::sub }
			});

			auto val = rt.get_class(TEST_STRUCT_NAME)
				//.method<int, int>("sub")
				.method(detail::cpp_method_key_t{ "sub",{ type_of<int>(), type_of<int>() } })
				.invoke_static<int>(3, 2);

			Assert::AreEqual(1, val, L"invoked method must return with 1");
		}

		TEST_METHOD(test_runtime_database_invalid_static_invoke)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				method_t{ "sub", &test_struct::sub }
			});


			auto m = rt.get_class(TEST_STRUCT_NAME)
				//.method<int, int>("sub");
				.method(detail::cpp_method_key_t{ "sub",{ type_of<int>(), type_of<int>() } });

			Assert::ExpectException<logic_error>([=]
			{
				test_struct obj;
				shared_ptr<test_struct> ptr_obj{ detail::eternal_shared_ptr, &obj };

				m.invoke(ptr_obj, { object_ref { 3 }, object_ref { 2 } });
			});
		}

		TEST_METHOD(test_runtime_database_self_lvalue)
		{
			runtime_database rt;

			typedef test_struct_generic<NON_COPY_ABLE> test_struct;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				method_t{ "inc_by", &test_struct::inc_by }
			});

			test_struct obj;
			shared_ptr<test_struct> ptr_obj{ detail::eternal_shared_ptr, &obj };

			obj.f1 = 5;

			rt.get_class(TEST_STRUCT_NAME)
				//.method<int>("inc_by")
				.method(detail::cpp_method_key_t{ "inc_by",{ type_of<int>() } })
				.invoke(ptr_obj, { object_ref { 3 } });

			Assert::AreEqual(8, obj.f1);
		}

		//
		// tests for fields
		//

		TEST_METHOD(test_runtime_database_field_set)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				field_t{ "f1", &test_struct::f1 }
			});

			test_struct obj { 42 };
			std::shared_ptr<test_struct> ptr_obj { detail::eternal_shared_ptr, &obj };

			auto f1_field = rt.get_class(TEST_STRUCT_NAME).field(detail::cpp_field_key_t{ "f1" });
			
			auto f1_val1 = static_cast<int>(f1_field.value(ptr_obj));
			Assert::AreEqual(42, f1_val1, L"value of f1 field must be 42");

			//
			f1_field.set_value(ptr_obj, object_ref { 8 });
			Assert::AreEqual(8, obj.f1, L"value of obj.f1 must be 8");

			auto f1_val2 = f1_field.value(ptr_obj).as<int>();
			Assert::AreEqual(8, f1_val2, L"value of f1 field must be 8");
		}

		TEST_METHOD(test_runtime_database_field_set_by_any_pointer)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				field_t{ "f1", &test_struct::f1 }
			});

			test_struct obj{ 42 };
			std::shared_ptr<test_struct> ptr_obj{ detail::eternal_shared_ptr, &obj };

			auto f1_field = rt.get_class(TEST_STRUCT_NAME).field(detail::cpp_field_key_t{ "f1" });

			//
			f1_field.set_value(ptr_obj, object_ref { 8 });
			Assert::AreEqual(8, obj.f1, L"value of obj.f1 must be 8");

			auto f1_val2 = static_cast<int>(f1_field.value(ptr_obj));
			Assert::AreEqual(8, f1_val2, L"value of f1 field must be 8");
		}

		TEST_METHOD(test_runtime_database_field_set_by_any_reference)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				field_t { "f1", &test_struct::f1 }
			});

			object_ref ptr_obj { test_struct{ 42 } };
			test_struct& obj = ptr_obj.as<test_struct&>();

			auto f1_field = rt.get_class(TEST_STRUCT_NAME).field(detail::cpp_field_key_t{ "f1" });

			//
			f1_field.set_value(ptr_obj, object_ref{ 8 });
			Assert::AreEqual(8, obj.f1, L"value of obj.f1 must be 8");

			auto f1_val2 = static_cast<int>(f1_field.value(ptr_obj));
			Assert::AreEqual(8, f1_val2, L"value of f1 field must be 8");
		}

		TEST_METHOD(test_runtime_database_invalid_field_access)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				field_t{ "f1", &test_struct::f1 }
			});

			auto f2_field = rt.get_class(TEST_STRUCT_NAME).field(detail::cpp_field_key_t{ "f1" });

			Assert::ExpectException<logic_error>([=]
			{
				f2_field.value();
			});
		}

		TEST_METHOD(test_runtime_database_static_field_set)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				field_t{ "f2", &test_struct::f2 }
			});

			auto f2_field = rt.get_class(TEST_STRUCT_NAME).field(detail::cpp_field_key_t{ "f2" });

			test_struct::f2 = 42;

			auto f2_val1 = static_cast<int>(f2_field.value());
			Assert::AreEqual(42, f2_val1, L"value of f1 field must be 42");

			//
			f2_field.set_value(object_ref { 8 });
			Assert::AreEqual(8, test_struct::f2, L"value of obj.f1 must be 8");

			auto f2_val2 = static_cast<int>(f2_field.value());
			Assert::AreEqual(8, f2_val2, L"value of f1 field must be 8");
		}

		TEST_METHOD(test_runtime_database_invalid_static_field_access)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				TEST_STRUCT_NAME,
				typeid(test_struct),
				field_t{ "f2", &test_struct::f2 }
			});

			auto f2_field = rt.get_class(TEST_STRUCT_NAME).field(detail::cpp_field_key_t{ "f2" });

			Assert::ExpectException<logic_error>([=]
			{
				test_struct a;
				
				f2_field.value(object_ref{ a });
			});
		}

		TEST_METHOD(test_runtime_database_inheritance_call_parent)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				"my_class_a",
				typeid(my_class_a),
				method_t { "f", &my_class_a::f }
			})
			.add(cpp_class_t
			{
				"my_class_b",
				typeid(my_class_b),
				
			})
			.add(class_converter_t
			{
				type_tag<my_class_a> { }, type_tag<my_class_b> { }
			});

			auto method = rt.get_class("my_class_b").method(detail::cpp_method_key_t{ "f", type_tag<int> {}, type_tag<int> {} });

			auto ptr_obj = make_shared<my_class_b>();

			auto ans = method.invoke(ptr_obj, { object_ref { 11 }, object_ref{ 22 } });

			int val = ans.as<int>();

			Assert::AreEqual(33, val);
		}

		TEST_METHOD(test_runtime_database_inheritance_call_derived)
		{
			runtime_database rt;

			rt.add(cpp_class_t
			{
				"my_class_a",
				typeid(my_class_a),
				method_t{ "g", &my_class_a::g }
			})
				.add(cpp_class_t
			{
				"my_class_b",
				typeid(my_class_b),
				method_t{ "g", &my_class_b::g }
			})
			.add(class_converter_t
			{
				type_tag<my_class_a> { }, type_tag<my_class_b> { }
			});

			shared_ptr<my_class_a> ptr_obj { make_shared<my_class_b>() };


			//
			auto method_a = rt.get_class("my_class_a").method(detail::cpp_method_key_t{ "g", type_tag<int> {} });

			auto ans = method_a.invoke(ptr_obj, { object_ref{ 11 } });

			int val = ans.as<int>();

			Assert::AreEqual(-11, val);

			//
			auto method_b = rt.get_class("my_class_b").method(detail::cpp_method_key_t{ "g", type_tag<int> {} });

			auto ans2 = method_b.invoke(ptr_obj, { object_ref{ 11 } });

			int val2 = ans2.as<int>();

			Assert::AreEqual(-11, val2);
		}
	};
}
