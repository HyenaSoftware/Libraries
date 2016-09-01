


using System.ComponentModel;

namespace reactive_framework8_debugger
{
	public class RvNodeVertex : INotifyPropertyChanged
	{
		public enum NodeType
		{
			Value,
			Operator
		}

		private readonly NodeType _nodeType;
		private readonly string _name;

		public string Name { get { return _name; } }

		private string _value;

		public string Value
		{
			get { return _value; }
			set
			{
				_value = value;
				PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Text"));
			}
		}

		public RvNodeVertex(NodeType nodeType_, string name_, string value_ = null)
		{
			_name = name_;
			_value = value_;
			_nodeType = nodeType_;
		}

		public event PropertyChangedEventHandler PropertyChanged;

		public string Text
		{
			get { return $"{Name} [{Value}]"; }
		}

		public bool IsOperator
		{
			get { return _nodeType == NodeType.Operator;  } 
		}
    }
}