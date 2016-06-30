#include "stdafx.h"
#include "network.hpp"

using namespace utility;
using namespace std;


desc::desc(std::string port_)
	: port { move(port_) }
{
}


server::server(desc desc_)
	: _port { move(desc_.port) }
	, _buffer_size { desc_.buffer_size }
{
	_client_socket = _create_listen_socket();
}

server::~server()
{
	int i_result = 0;

	// shutdown the connection since we're done
	i_result = shutdown(_client_socket, SD_SEND);
	if (i_result == SOCKET_ERROR)
	{
		stringstream sb;
		sb << "shutdown failed with error: " << WSAGetLastError();
		
		// what to do here?
	}

	// cleanup
	closesocket(_client_socket);
	WSACleanup();
}

SOCKET server::_create_listen_socket() const
{
	addrinfo *result { nullptr };
	addrinfo hints;

	int i_result = 0;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Initialize Winsock
	WSADATA wsaData;
	i_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (i_result != 0)
	{
		stringstream sb;
		sb << "WSAStartup failed with error: " << i_result;
		throw runtime_error { sb.str() };
	}


	// Resolve the server address and port
	i_result = getaddrinfo(NULL, _port.c_str(), &hints, &result);
	if (i_result != 0)
	{
		stringstream sb;
		sb << "getaddrinfo failed with error: " << i_result;

		WSACleanup();

		throw runtime_error{ sb.str() };
	}

	// Create a SOCKET for connecting to server
	auto listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET)
	{
		stringstream sb;
		sb << "socket failed with error: " << WSAGetLastError();

		freeaddrinfo(result);
		WSACleanup();

		throw runtime_error{ sb.str() };
	}

	// Setup the TCP listening socket
	i_result = ::bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
	if (i_result == SOCKET_ERROR)
	{
		stringstream sb;
		sb << "bind failed with error: " << WSAGetLastError();
		freeaddrinfo(result);
		closesocket(listen_socket);
		WSACleanup();
		throw runtime_error{ sb.str() };
	}

	freeaddrinfo(result);

	i_result = listen(listen_socket, SOMAXCONN);
	if (i_result == SOCKET_ERROR)
	{
		stringstream sb;
		sb << "listen failed with error: " << WSAGetLastError();
		closesocket(listen_socket);
		WSACleanup();
		throw runtime_error{ sb.str() };
	}

	// Accept a client socket
	auto client_socket = accept(listen_socket, NULL, NULL);
	if (client_socket == INVALID_SOCKET)
	{
		stringstream sb;
		sb << "accept failed with error: " << WSAGetLastError();
		closesocket(listen_socket);
		WSACleanup();
		throw runtime_error{ sb.str() };
	}

	// No longer need server socket
	closesocket(listen_socket);

	return client_socket;
}

vector<char> server::read() const
{
	std::vector<char> buffer, result;
	buffer.resize(_buffer_size);

	size_t offset = 0;

	while(true)
	{
		auto data_size = recv(_client_socket, &buffer[offset], _buffer_size, 0);

		if(data_size == 0)
			break;

		if (data_size < 0)
		{
			int ec = WSAGetLastError();

			char msg_buffer[1024];
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ec, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg_buffer, 1024, NULL);

			stringstream sb;
			sb << "I/O error: " << msg_buffer << " (" << ec << ")";

			closesocket(_client_socket);
			WSACleanup();
			
			throw runtime_error { sb.str() };
		}

		result.insert(result.end(), buffer.begin(), buffer.begin() + data_size);
	}

	return result;
}

void server::write(vector<char> data_) const
{
	for (auto it = data_.begin(); it != data_.end(); )
	{
		size_t delta_size = min(data_.end() - it, _buffer_size);

		int i_result = send(_client_socket, &*it, delta_size, 0);
		if (i_result == SOCKET_ERROR)
		{
			stringstream sb;
			sb << "send failed with error: " << WSAGetLastError();
			closesocket(_client_socket);
			WSACleanup();
			throw runtime_error	{ sb.str() };
		}

		it += delta_size;
	}
}


client::client(string address_, desc desc_)
	: _port{ move(desc_.port) }
	, _buffer_size{ desc_.buffer_size }
{
	_socket = _create_socket(move(address_));
}

client::~client()
{
	int i_result = 0;

	// shutdown the connection since we're done
	i_result = shutdown(_socket, SD_SEND);
	if (i_result == SOCKET_ERROR)
	{
		stringstream sb;
		sb << "shutdown failed with error: " << WSAGetLastError();

		// what to do here?
	}

	// cleanup
	closesocket(_socket);
	WSACleanup();
}

SOCKET client::_create_socket(string address_) const
{
	WSADATA wsaData;

	// Initialize Winsock
	int i_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (i_result != 0)
	{
		stringstream sb;
		sb << "WSAStartup failed with error: " << i_result;
		throw runtime_error{ sb.str() };
	}

	addrinfo *result = nullptr, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	i_result = getaddrinfo(address_.c_str(), _port.c_str(), &hints, &result);
	if (i_result != 0)
	{
		stringstream sb;
		sb << "getaddrinfo failed with error: " << i_result;

		WSACleanup();

		throw runtime_error{ sb.str() };
	}

	SOCKET new_sckt { INVALID_SOCKET };

	// Attempt to connect to an address until one succeeds
	for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		new_sckt = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (new_sckt == INVALID_SOCKET)
		{
			stringstream sb;
			sb << "socket failed with error: " << WSAGetLastError();

			WSACleanup();

			throw runtime_error{ sb.str() };
		}

		// Connect to server.
		i_result = connect(new_sckt, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (i_result == SOCKET_ERROR)
		{
			closesocket(new_sckt);
			new_sckt = INVALID_SOCKET;
			continue;
		}

		break;
	}
	
	freeaddrinfo(result);

	if (new_sckt == INVALID_SOCKET)
	{
		WSACleanup();
		throw runtime_error{ "Unable to connect to server!" };
	}

	return new_sckt;
}

vector<char> client::read() const
{
	std::vector<char> buffer;
	buffer.resize(_buffer_size);

	size_t offset = 0;

	while (true)
	{
		auto data_size = recv(_socket, &buffer[offset], _buffer_size, 0);

		if (data_size == 0)
			return buffer;

		if (data_size < 0)
		{
			closesocket(_socket);
			WSACleanup();

			throw runtime_error{ "I/O error" };
		}

		offset += data_size;
		buffer.resize(offset + _buffer_size);
	}
}

void client::write(vector<char> data_) const
{
	// Send an initial buffer
	for (auto it = data_.begin(); it != data_.end(); )
	{
		size_t delta_size = min(data_.end() - it, _buffer_size);

		int i_result = send(_socket, &*it, delta_size, 0);
		if (i_result == SOCKET_ERROR)
		{
			stringstream sb;
			sb << "send failed with error: " << WSAGetLastError();
			closesocket(_socket);
			WSACleanup();
			throw runtime_error{ sb.str() };
		}

		it += delta_size;
	}
}

