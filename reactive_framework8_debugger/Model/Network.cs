using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace reactive_framework8_debugger
{
	class Network : IDisposable
	{
		public Stream DataStream { get { return _networkStream; } }

		private Socket _serverSocket;

		private NetworkStream _networkStream;

		public Network()
		{
		}

		public bool IsConnected
		{
			get
			{
				return _serverSocket != null && _networkStream != null;
			}
		}

		public void Disconnect()
		{
			if (_serverSocket != null)
			{
				_serverSocket.Shutdown(SocketShutdown.Both);
				_serverSocket.Close();
				_serverSocket = null;
			}

			if (_networkStream != null)
			{
				_networkStream.Close();
				_networkStream = null;
			}
		}

		public void TryToConnect(string host_, int port_)
		{
			// Establish the local endpoint for the socket.
			// Dns.GetHostName returns the name of the 
			// host running the application.
			IPHostEntry ipHostInfo = Dns.GetHostEntry(host_);

			foreach (var ipAddress in ipHostInfo.AddressList)
			{
				IPEndPoint localEndPoint = new IPEndPoint(ipAddress, port_);

				// Create a TCP/IP socket.
				Socket listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

				// Bind the socket to the local endpoint and 
				// listen for incoming connections.
				try
				{
					listener.Bind(localEndPoint);
					listener.Listen(10);

					// Start listening for connections.
					// Program is suspended while waiting for an incoming connection.
					_serverSocket = listener.Accept();

					//_reciever.Start();
					break;
				}
				catch (Exception e)
				{
					continue;
				}
			}

			if (_serverSocket == null)
			{
				throw new Exception("Unable to connect any client");
			}

			_networkStream = new NetworkStream(_serverSocket);
		}

		public void Dispose()
		{
			Disconnect();
		}
	}
}
