using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace reactive_framework8_debugger
{
	class RvGraphHistory
	{
#region Event types
		abstract class Event
		{
			private readonly HistoryItem _item; // only for history list in GUI

			protected Event(HistoryItem item_)
			{
				_item = item_;
			}

			public HistoryItem Desc { get { return _item; } }

			public virtual void Apply(RvGraph graph_)
			{
				//_item.Selected = true;
			}

			public virtual void Revert(RvGraph graph_)
			{
				//_item.Selected = false;
			}
		}

		class ChangeVertexValueEvent : Event
		{
			private readonly RvNodeVertex _targetVertex;
			private readonly string _oldVal, _newVal;

			public ChangeVertexValueEvent(HistoryItem item_, RvNodeVertex targetVertex_, string oldVal_, string newVal_)
				: base(item_)
			{
				_targetVertex = targetVertex_;
				_oldVal = oldVal_;
				_newVal = newVal_;
			}

			public override void Apply(RvGraph graph_)
			{
				base.Apply(graph_);

				_targetVertex.Value = _newVal;
			}

			public override void Revert(RvGraph graph_)
			{
				base.Revert(graph_);

				_targetVertex.Value = _oldVal;
			}
		}

		class AddEdgeEvent : Event
		{
			private readonly RvEdge _newEdge;

			public AddEdgeEvent(HistoryItem item_, RvEdge newEdge_) : base(item_)
			{
				_newEdge = newEdge_;
			}

			public override void Apply(RvGraph graph_)
			{
				base.Apply(graph_);

				graph_.AddVerticesAndEdge(_newEdge);
			}

			public override void Revert(RvGraph graph_)
			{
				base.Revert(graph_);

				graph_.RemoveEdge(_newEdge);
			}
		}

		class NewNodeEvent : Event
		{
			private RvNodeVertex _vertex;
			private RvNodeVertex.NodeType _vertexType;

			public NewNodeEvent(HistoryItem item_, RvNodeVertex vertex_, RvNodeVertex.NodeType vertexType_)
				: base(item_)
			{
				_vertex = vertex_;
				_vertexType = vertexType_;
			}

			public override void Apply(RvGraph graph_)
			{
				base.Apply(graph_);

				graph_.AddVertex(_vertex);
			}

			public override void Revert(RvGraph graph_)
			{
				base.Revert(graph_);

				graph_.RemoveVertex(_vertex);
			}
		}
		#endregion

		private List<Event> _history = new List<Event>();

		private Dictionary<string, RvNodeVertex> _latestVertices = new Dictionary<string, RvNodeVertex>();

		private int _currentEvent = -1;

		private readonly RvGraph _graph = new RvGraph();

		public event EventHandler OnGraphChanged;
		public event EventHandler<HistoryItem> OnHistoryChanged;

		public RvGraph Graph
		{
			get
			{
				return _graph;
			}
		}

		public void ChangeVertexValue(string vertexName_, string newValue_)
		{
			var text = $"{vertexName_} has been changed to {newValue_}";

			RvNodeVertex vertex = null;
			if (!_latestVertices.TryGetValue(vertexName_, out vertex))
			{
				_latestVertices.Add(vertexName_, new RvNodeVertex(RvNodeVertex.NodeType.Value, vertexName_));
			}

			//
			var ev = new HistoryItem(_history.Count, text);
			_history.Add(new ChangeVertexValueEvent(ev, vertex, vertex.Value, newValue_));

			//
			OnHistoryChanged?.Invoke(this, ev);
		}

		internal void AddNode(string vertexName_, RvNodeVertex.NodeType vertexType_)
		{
			var text = $"{vertexName_} has been registered";

			RvNodeVertex vertex = null;
			if (!_latestVertices.TryGetValue(vertexName_, out vertex))
			{
				_latestVertices.Add(vertexName_, vertex = new RvNodeVertex(vertexType_, vertexName_));
			}

			//
			var ev = new HistoryItem(_history.Count, text);
			_history.Add(new NewNodeEvent(ev, vertex, vertexType_));

			//
			OnHistoryChanged?.Invoke(this, ev);
		}

		public void AddEdge(string edgeName_, string srcVertexName_, string dstVertexName_, RvNodeVertex.NodeType srcType_, RvNodeVertex.NodeType dstType_)
		{
			RvNodeVertex v1, v2;
			if (!_latestVertices.TryGetValue(srcVertexName_, out v1))
			{
				v1 = new RvNodeVertex(srcType_, srcVertexName_);
				_latestVertices[srcVertexName_] = v1;
			}

			if (!_latestVertices.TryGetValue(dstVertexName_, out v2))
			{
				v2 = new RvNodeVertex(dstType_, dstVertexName_);
				_latestVertices[dstVertexName_] = v2;
			}

			var text = $"{edgeName_} has been added";
			var ev = new HistoryItem(_history.Count, text);
			var edge = new RvEdge(edgeName_, v1, v2);

			_history.Add(new AddEdgeEvent(ev, edge));

			OnHistoryChanged?.Invoke(this, ev);
		}

		public int CurrentEventIndex
		{
			get
			{
				return _currentEvent;
			}
			set
			{
				if (value >= _history.Count)
				{
					throw new ArgumentOutOfRangeException();
				}

				if (_currentEvent >= 0)
				{
					_history[_currentEvent].Desc.Selected = false;
				}

				ActivateEvents(value);
				_currentEvent = value;

				if (value >= 0)
				{
					_history[_currentEvent].Desc.Selected = true;
				}
			}
		}

		public void ActivateTheLatest()
		{
			CurrentEventIndex = _history.Count - 1;
		}

		private void ActivateEvents(int newActiveEvent_)
		{
			bool graphChanged = false;

			// [_currentEvent] must be active

			// activate events which were not (move forward)
			//for (int i = newActiveEvent_; i > _currentEvent; --i)
			for (int i = _currentEvent+1; i <= newActiveEvent_; ++i)
			{
				graphChanged |= true;

				_history[i].Apply(_graph);
			}

			// deactivate events which were active (move backward)
			for (int i = _currentEvent; i > newActiveEvent_; --i)
			{
				graphChanged |= true;

				_history[i].Revert(_graph);
			}

			if (graphChanged)
			{
				OnGraphChanged?.Invoke(this, new EventArgs());
			}
		}
	}
}
