#include "stdafx.h"
#include "CppUnitTest.h"
#include "unittest_converters.h"

#include <utility\graph2.h>



using namespace std;
using namespace utility;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace utility_unittest
{
	TEST_CLASS(graph_unittest)
	{
	public:
		TEST_METHOD(TestInsert)
		{
			utility::graph g;

			auto jt = g.insert({ 0, 1 });

			Assert::IsTrue(g.end() != jt);

			Assert::AreEqual(*jt, std::make_pair(0, 1));
		}

		TEST_METHOD(TestFind)
		{
			utility::graph g;

			auto it = g.insert({ 0, 1 });

			auto jt = g.find({ 0, 1 });

			Assert::IsTrue(it == jt);
			Assert::IsTrue(jt != g.end());
			Assert::AreEqual(*jt, std::make_pair(0, 1));
		}

		TEST_METHOD(TestEqualRange_OutEdges)
		{
			utility::graph g;

			g.insert({ 0, 1 });
			g.insert({ 1, 2 });
			g.insert({ 1, 3 });
			g.insert({ 2, 4 });

			auto rng = g.equal_range({ 1, graph::UNDEFINED_VERTEX });

			Assert::IsTrue(rng.first != g.end());
			Assert::AreEqual(std::make_pair(1, 2), *rng.first);

			++rng.first;

			Assert::IsTrue(rng.first != g.end());
			Assert::AreEqual(std::make_pair(1, 3), *rng.first);

			++rng.first;

			Assert::IsTrue(rng.first != g.end());
			Assert::IsTrue(rng.first == rng.second);
		}

		TEST_METHOD(TestEqualRange_InEdges)
		{
			utility::graph g;

			g.insert({ 0, 1 });
			g.insert({ 1, 2 });
			g.insert({ 3, 2 });
			g.insert({ 0, 3 });
			g.insert({ 4, 1 });

			auto rng = g.equal_range({ graph::UNDEFINED_VERTEX, 2 });

			Assert::IsTrue(rng.first != g.end());
			Assert::AreEqual(std::make_pair(1, 2), *rng.first);

			++rng.first;

			Assert::IsTrue(rng.first != g.end());
			Assert::AreEqual(std::make_pair(3, 2), *rng.first);

			++rng.first;

			Assert::IsTrue(rng.first == rng.second);
			Assert::IsTrue(rng.first != g.end());
		}

		TEST_METHOD(TestMerge)
		{
			utility::graph f, g;

			f.insert({ 0, 1 });
			f.insert({ 1, 2 });
			f.insert({ 2, 0 });

			g.insert({ 0, 3 });
			g.insert({ 3, 4 });
			g.insert({ 4, 0 });

			f.merge(g);

			const std::unordered_set<graph::edge_id_t> EXPECTED_EDGES
			{
				{ 0, 1 },
				{ 1, 2 },
				{ 2, 0 },
				{ 0, 3 },
				{ 3, 4 },
				{ 4, 0 }
			};

			std::unordered_set<graph::edge_id_t> found_edges;

			size_t number_of_edges = 0;

			for (auto& e : f)
			{
				found_edges.insert(e);
				++number_of_edges;
			}


			Assert::AreEqual(EXPECTED_EDGES, found_edges);
			Assert::AreEqual(EXPECTED_EDGES.size(), number_of_edges, L"One or more edges retrived back multiple times.");
		}

		TEST_METHOD(test_unguided)
		{
			utility::graph g;

			g.insert({ 8, 42 });
			g.insert({ 42, 8 });

			auto inputs_of_8 = g.input_vertices(8);
			auto outputs_of_8 = g.input_vertices(8);

			auto inputs_of_42 = g.input_vertices(42);
			auto outputs_of_42 = g.input_vertices(42);

			vector<int> expected_inputs_of_8{ 42 };
			vector<int> expected_inputs_of_42{ 8 };
			vector<int> expected_outputs_of_8{ 42 };
			vector<int> expected_outputs_of_42{ 8 };

			Assert::AreEqual(expected_inputs_of_8, inputs_of_8);
			Assert::AreEqual(expected_inputs_of_42, inputs_of_42);
			Assert::AreEqual(expected_outputs_of_8, outputs_of_8);
			Assert::AreEqual(expected_outputs_of_42, outputs_of_42);
		}
	};
}