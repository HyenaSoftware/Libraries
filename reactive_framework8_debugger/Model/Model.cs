using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using System.IO;
using System.Threading;

namespace reactive_framework8_debugger
{
	class Model : IDisposable
	{
		public event EventHandler<Message> OnHaveNewMessage;
		public event EventHandler OnConnected;

		private BackgroundWorker _connectionWorker = null;

		private readonly JsonSerializer _jsonSerializer = new JsonSerializer();

		private string _host;
		private int _port;

//		private volatile bool _doDeserialize = false;

		public Model()
		{
		}

		public void TryConnectTo(string host_, int port_)
		{
			_host = host_;
			_port = port_;

			Disconnect();

			_connectionWorker = new BackgroundWorker();
			_connectionWorker.WorkerSupportsCancellation = true;
			_connectionWorker.DoWork += MessageProcessor;
			_connectionWorker.RunWorkerAsync();
		}

		public void Disconnect()
		{
			if(_connectionWorker != null)
			{
				_connectionWorker.CancelAsync();
				_connectionWorker.Dispose();
				_connectionWorker = null;
			}
		}

		private void MessageProcessor(object sender, DoWorkEventArgs e)
		{
			Network network = new Network();
			network.TryToConnect(_host, _port);
			
			OnConnected?.Invoke(this, new EventArgs());

			TextReader textReader = new StreamReader(network.DataStream, Encoding.ASCII);
			JsonTextReader jsonTextReader = new JsonTextReader(textReader);
			jsonTextReader.SupportMultipleContent = true;

			while(!_connectionWorker.CancellationPending)
			{
				try
				{
					bool hasNext = jsonTextReader.Read(); // probably ignores the next "end token"
					if (!hasNext)
					{
						continue;
					}

					var msg = _jsonSerializer.Deserialize<Message>(jsonTextReader);

					//
					OnHaveNewMessage?.Invoke(this, msg);
				}
				catch(Exception e_)
				{
					var text = e_.ToString();
				}
			}

			network.Dispose();
			network = null;

			textReader.Close();
			jsonTextReader.Close();

			textReader = null;
			jsonTextReader = null;

			e.Cancel = true;
		}

		public void Dispose()
		{
			Disconnect();
		}
	}
}
