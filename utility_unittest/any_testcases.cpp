#include "stdafx.h"
#include "CppUnitTest.h"
#include "unittest_converters.h"

#include <utility\any.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace utility;

namespace utility_unittest
{
	using namespace utility;

	TEST_CLASS(any_unittest)
	{
	public:
		TEST_METHOD(test_any_val_access)
		{
			int n = 5;

			any a { n };

			//
			int v1 = any_cast<int>(a);

			Assert::AreEqual(5, v1);

			//
			n = 8;
			int v2 = any_cast<int>(a);

			Assert::AreEqual(5, v2);
		}

		TEST_METHOD(test_any_ref_access)
		{
			int n = 5;

			any a = n;

			//
			int& r = any_cast<int&>(a);

			Assert::AreEqual(5, n);
			Assert::AreEqual(5, r);

			//
			r = 8;

			Assert::AreEqual(5, n);

			int v1 = any_cast<int>(a);

			Assert::AreEqual(8, v1);
		}
	};

	TEST_CLASS(any_ref_unittest)
	{
	public:


		TEST_METHOD(test_const_any_ref)
		{
			int n = 5;
			any_ptr a { &n };

			const int& r = *static_cast<int*>(a);

			Assert::AreEqual(5, r);

			n = 42;

			Assert::AreEqual(42, r);
		}

		TEST_METHOD(test_const_any_ref2)
		{
			const int n = 5;
			any a { n };

			int m = any_cast<int>(a);

			Assert::AreEqual(5, m);
		}

		TEST_METHOD(test_const_any_ref3)
		{
			int n = 5;
			any_ptr a { &n };

			//
			const int& r = *static_cast<int*>(a);

			Assert::AreEqual(5, r);
			Assert::AreEqual(5, n);

			n = 8;

			Assert::AreEqual(8, r);
		}

		TEST_METHOD(test_any_ref_as_const)
		{
			int n = 7;
			any_ptr a { &n };

			any_ptr b = a;

			const int& r = *static_cast<int*>(b);

			Assert::AreEqual(7, r);

			n = 42;

			Assert::AreEqual(42, r);
		}

		TEST_METHOD(test_any_ptr_convert)
		{
			//any
		}

	};
}
