using GraphSharp.Controls;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace reactive_framework8_debugger
{
	public class DebuggerGraphLayout : GraphLayout<RvNodeVertex, RvEdge, RvGraph> { }

	class ViewModel : INotifyPropertyChanged
	{
		private RvGraph graph;
		private List<string> layoutAlgorithmTypes = new List<string>
		{
			"BoundedFR",
			"Circular",
            "CompoundFDP",
            "EfficientSugiyama",
            "FR",
            "ISOM",
            "KK",
            "LinLog",
            "Tree",
		};

		private string layoutAlgorithmType;

		private char _nextLetter = 'd';
		private RvNodeVertex _lastVertex;

		public ViewModel()
		{
			Graph = new RvGraph(true);

			RvNodeVertex[] vertices =
			{
				new RvNodeVertex("a"),
				new RvNodeVertex("b"),
				new RvNodeVertex("c"),
			};

			_lastVertex = vertices.Last();

			foreach (var v in vertices)
			{
				Graph.AddVertex(v);
			}

			Graph.AddEdge(new RvEdge("a->b", vertices[0], vertices[1]));
			Graph.AddEdge(new RvEdge("b->c", vertices[1], vertices[2]));
			Graph.AddEdge(new RvEdge("c->a", vertices[2], vertices[0]));

			LayoutAlgorithmType = "CompoundFDP";
		}

		internal void AddVertex()
		{
			var v = new RvNodeVertex(_nextLetter.ToString());

			_nextLetter = (char)(_nextLetter + 1);

			Graph.AddVertex(v);
			Graph.AddEdge(new RvEdge(String.Format("{0} -> {1}", _lastVertex.ID, v.ID), _lastVertex, v));

			_lastVertex = v;

			NotifyPropertyChanged("Graph");
		}

		public event PropertyChangedEventHandler PropertyChanged;

		public RvGraph Graph
		{
			get { return graph; }
			set
			{
				graph = value;
				NotifyPropertyChanged("Graph");
			}
		}

		public List<String> LayoutAlgorithmTypes
		{
			get { return layoutAlgorithmTypes; }
		}


		public string LayoutAlgorithmType
		{
			get { return layoutAlgorithmType; }
			set
			{
				layoutAlgorithmType = value;
				NotifyPropertyChanged("LayoutAlgorithmType");
			}
		}

		private void NotifyPropertyChanged(String info)
		{
			if (PropertyChanged != null)
			{
				PropertyChanged(this, new PropertyChangedEventArgs(info));
			}
		}
	}
}
