using GraphSharp.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;

namespace reactive_framework8_debugger
{
	class ViewModel : INotifyPropertyChanged, IDisposable
	{
		
		private readonly List<string> layoutAlgorithmTypes = new List<string>
		#region List of graph layout algorithms
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
		#endregion

		private string layoutAlgorithmType;

		private ObservableCollection<HistoryItem> _history = new ObservableCollection<HistoryItem>();

		// move to model layer
		
		private string _serverStatus = "Client status is unknown";

		private RvGraphHistory _graphHist = null;
		private readonly Model _model = new Model();


		public ViewModel()
		{
			ConnectionInProgress = false;
			ConnectBtnText = "Connect";
			LayoutAlgorithmType = "CompoundFDP";

			ResetGraphHistory();

			_model.OnHaveNewMessage += OnNewMessageHandler;
			_model.OnConnected += (object sender, EventArgs e_) =>
			{
				ConnectionInProgress = false;
				ConnectBtnText = "Connected";
				NotifyPropertyChanged("ConnectBtnText");
			};
		}

		private void ResetEventHistory()
		{
			_history.Clear();
		}

		private void ResetGraphHistory()
		{
			_graphHist = new RvGraphHistory();
			_graphHist.OnGraphChanged += (object sender, EventArgs e) =>
			{
				NotifyPropertyChanged("Graph");
			};

			_graphHist.OnHistoryChanged += (object sender, HistoryItem e) =>
			{
				_history.Add(e);
				NotifyPropertyChanged("History");
			};

			NotifyPropertyChanged("Graph");
		}

		private void OnNewMessageHandler(object sender, Message msg_)
		{
			Action<Message> processEvent = this.ProcessEvent;

			Application.Current.Dispatcher.BeginInvoke(processEvent, msg_);
		}

		private void ProcessEvent(Message msg_)
		{
			switch (msg_.Event)
			{
				case "value.change":
					_graphHist.ChangeVertexValue(msg_.Vertex, msg_.Value);					
					break;

				case "value.add":
					_graphHist.AddNode(msg_.Vertex, NodeTypeFrom(msg_.VertexType));
					break;

				case "edge.add":
					var edgeName = $"{msg_.Vertex} -> {msg_.Covertex}";
					_graphHist.AddEdge(edgeName, msg_.Vertex, msg_.Covertex, NodeTypeFrom(msg_.VertexType), NodeTypeFrom(msg_.CoVertexType));
					break;

				case "operator.add":
					_graphHist.AddNode(msg_.Vertex, NodeTypeFrom(msg_.VertexType));
					break;

				default:
					throw new Exception($"Don't know how to process this message: '{msg_.Event}'.");
			}

			_graphHist.ActivateTheLatest();
		}

		private static RvNodeVertex.NodeType NodeTypeFrom(string value_)
		{
			switch (value_.ToLower())
			{
				case "operator":	return RvNodeVertex.NodeType.Operator;
				case "value":		return RvNodeVertex.NodeType.Value;
				default:			throw new Exception($"Don't know what kind of node {value_} is.");
			}
		}

		private void _server_OnClientFound(object sender, EventArgs e)
		{
			ServerStatusLbl = "Connected to client...";
        }

		public event PropertyChangedEventHandler PropertyChanged;

		public RvGraph Graph
		{
			get { return _graphHist.Graph; }
		}

		public String ServerStatusLbl
		{
			get { return _serverStatus; }
			set
			{
				_serverStatus = value;
				NotifyPropertyChanged("ServerStatusLbl");
			}
		}

		public string HostAndPort
		{
			get;
			set;
		}

		public List<String> LayoutAlgorithmTypes
		{
			get { return layoutAlgorithmTypes; }
		}

		public ObservableCollection<HistoryItem> History
		{
			get { return _history; }
		}

		private class ConnectCommand : ICommand
		{
			public event EventHandler CanExecuteChanged;

			private readonly ViewModel _viewModel;

			public ConnectCommand(ViewModel model_)
			{
				_viewModel = model_;
			}

			public bool CanExecute(object parameter_)
			{
				return !_viewModel.ConnectionInProgress;
			}

			public void Execute(object parameter_)
			{
				_viewModel.ConnectionInProgress = true;

				string[] args = _viewModel.HostAndPort.Split(':');

				_viewModel.ConnectBtnText = "Connecting...";
				_viewModel.NotifyPropertyChanged("ConnectBtnText");

				_viewModel.ResetGraphHistory();
				_viewModel.ResetEventHistory();
				_viewModel._model.TryConnectTo(args[0], int.Parse(args[1]));
			}
		}

		public string ConnectBtnText { get; set; }

		public ICommand TryConnectToCommand
		{
			get
			{
				return new ConnectCommand(this);
			}
		}

		public int CurrentHistIndex
		{
			set
			{
				_graphHist.CurrentEventIndex = value;
            }
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

		public bool ConnectionInProgress { get; private set; }

		private void NotifyPropertyChanged(String info)
		{
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(info));
		}

		public void Dispose()
		{
			_model.Dispose();
		}
	}
}
