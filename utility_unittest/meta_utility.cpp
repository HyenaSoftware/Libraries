#include "stdafx.h"
#include "CppUnitTest.h"
#include <utility\meta.hpp>
#include <codecvt>
#include <utility\tuple_utils.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace utility;


namespace utility_unittest
{
	struct should_be_identical
	{
		constexpr should_be_identical() = default;

		should_be_identical(const should_be_identical&)
		{
			Assert::Fail();
		}

		should_be_identical(should_be_identical&&)
		{
			Assert::Fail();
		}

		should_be_identical& operator=(const should_be_identical&)
		{
			Assert::Fail();

			return *this;
		}

		should_be_identical& operator=(should_be_identical&&)
		{
			Assert::Fail();

			return *this;
		}
	};

	struct should_not_be_copied
	{
		should_not_be_copied() = default;

		should_not_be_copied(const should_not_be_copied&)
		{
			Assert::Fail();
		}

		should_not_be_copied(should_not_be_copied&&) = default;

		should_not_be_copied& operator=(const should_not_be_copied& other_)
		{
			Assert::Fail();

			return *this;
		}

		should_not_be_copied& operator=(should_not_be_copied&&) = default;

		int value_for_testing = 0;
	};

	struct should_be_copied
	{
		should_be_copied() = default;

		should_be_copied(const should_be_copied& other_)
		{
			_count_of_copies = other_._count_of_copies + 1;
		}

		should_be_copied(should_be_copied&&) = default;

		should_be_copied& operator=(const should_be_copied& other_)
		{
			_count_of_copies = other_._count_of_copies + 1;

			return *this;
		}

		should_be_copied& operator=(should_be_copied&&) = default;

		int value_for_testing = 0;

		int count_of_copies() const
		{
			return _count_of_copies;
		}

	public:
		int _count_of_copies = 0;
	};
}


namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			std::wstring ToString(const utility_unittest::should_not_be_copied& vec_)
			{
				wstringstream ws;

				ws << "[should_not_be_copied]";
				
				return ws.str();
			}

			std::wstring ToString(const utility_unittest::should_be_identical& vec_)
			{
				wstringstream ws;

				ws << "[should_be_identical]";

				return ws.str();
			}
		}
	}
}

namespace utility_unittest
{		
	TEST_CLASS(tuple_unittest)
	{
	public:
		
		TEST_METHOD(TestConstRef)
		{
			const std::tuple<should_be_identical> a;

			for_each(a, [&](auto& v_)
			{
				Assert::AreSame(get<0>(a), v_);
			});
		}

		TEST_METHOD(TestRef)
		{
			std::tuple<should_not_be_copied> a;

			for_each(a, [&](auto& v_)
			{
				v_.value_for_testing = 5;

				Assert::AreSame(get<0>(a), v_);
			});

			Assert::AreEqual(5, get<0>(a).value_for_testing);
		}

		TEST_METHOD(TestValue)
		{
			std::tuple<should_be_copied> a;

			for_each(a, [](auto v_)
			{
				Assert::AreEqual(1, v_.count_of_copies());
			});
		}

	};
}
