using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace reactive_framework8_debugger
{
	public class HistoryItem : INotifyPropertyChanged
	{
		private readonly string _text;

		private bool _selected = false;

		private readonly int _versionId;

		public event PropertyChangedEventHandler PropertyChanged;

		public HistoryItem(int versionId_, string text_)
		{
			_versionId = versionId_;
			_text = text_;
		}

		public Visibility Visibility
		{
			get
			{
				return _selected ? Visibility.Visible : Visibility.Hidden;
			}
		}

		public bool Selected
		{
			get { return _selected; }
			set
			{
				_selected = value;
				PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Visibility"));
			}
		}

		public int VersionId
		{
			get { return _versionId; }
		}

		public string Text
		{
			get { return $"{VersionId} - {_text}"; }
		}
	}


}
