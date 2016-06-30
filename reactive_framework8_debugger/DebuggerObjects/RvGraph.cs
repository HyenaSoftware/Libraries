using QuickGraph;

namespace reactive_framework8_debugger
{
	public class RvGraph : BidirectionalGraph<RvNodeVertex, RvEdge>
	{
		public RvGraph() { }

		public RvGraph(bool allowParallelEdges)
            : base(allowParallelEdges)
		{
		}

		public RvGraph(bool allowParallelEdges, int vertexCapacity)
            : base(allowParallelEdges, vertexCapacity)
		{
		}
	}
}