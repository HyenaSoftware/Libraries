using QuickGraph;
using System.ComponentModel;

namespace reactive_framework8_debugger
{
	public class RvEdge : Edge<RvNodeVertex>, INotifyPropertyChanged
	{
		private string _id;

		public event PropertyChangedEventHandler PropertyChanged;

		public RvEdge(string id, RvNodeVertex source, RvNodeVertex target)
            : base(source, target)
        {
			_id = id;
		}

		public override string ToString()
		{
			return _id;
		}
	}
}