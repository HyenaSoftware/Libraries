#include "stdafx.h"
#include "CppUnitTest.h"
#include "unittest_converters.h"

#include <utility\rtti_aux.hpp>
#include <utility\object_ref.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace utility;

namespace utility_unittest
{
	using namespace utility;

	class polymorphic_base
	{
	public:
		virtual ~polymorphic_base() = default;
	};

	class polymorphic_derived : public polymorphic_base
	{
	};


	TEST_CLASS(object_ref_unittest)
	{
	public:
		TEST_METHOD(test_primitive_boxing)
		{
			object_ref number { 5 };

			int num = (int)number;

			Assert::AreEqual(5, num);
		}

		TEST_METHOD(test_polymorphic_boxing)
		{
			const type_index BASE_TYPE { typeid(polymorphic_base) };
			const type_index DERIVED_TYPE { typeid(polymorphic_derived) };

			shared_ptr<polymorphic_base> base_ptr { make_shared<polymorphic_derived>() };

			object_ref ptr { base_ptr };

			const auto boxed_type = ptr.type();

			Assert::AreEqual(BASE_TYPE, boxed_type);
			Assert::AreEqual(DERIVED_TYPE, type_index { typeid (ptr.as<polymorphic_base>()) });
			Assert::AreEqual(DERIVED_TYPE, type_index{ typeid (*ptr.as_pointer<polymorphic_base>()) });
		}
	};
}
