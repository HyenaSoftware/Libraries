#include "stdafx.h"
#include "network.hpp"
#include "module_cross_singleton.hpp"

using namespace utility;
using namespace std;

#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")


class WSAInit : public singleton
{
public:
	WSAInit()
	{
		// Initialize Winsock
		WSADATA wsaData;
		int	i_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (i_result != 0)
		{
			stringstream sb;
			sb << "WSAStartup failed with error: " << i_result;
			throw runtime_error{ sb.str() };
		}
	}

	~WSAInit()
	{
		WSACleanup();
	}
};


desc::desc(std::string port_, size_t buffer_size_)
	: port { move(port_) }
	, buffer_size { buffer_size_ }
{
}


server::server(desc desc_)
	: _port { move(desc_.port) }
	, _buffer_size { desc_.buffer_size }
{
	// ensure that WSA is initialized
	// WSACleanup() is called by the dtor of WSAInit at the termiantion of this process
	gl_storage().get_singleton_of<WSAInit>();

	std::tie(_listen_socket, _ptr_server_address) = _create_listen_socket();
}

server::~server()
{
	//
	if (_ptr_server_address)
	{
		freeaddrinfo(_ptr_server_address);
		_ptr_server_address = nullptr;
	}

	// No longer need server socket
	if(_listen_socket != INVALID_SOCKET)
	{
		closesocket(_listen_socket);
		_listen_socket = INVALID_SOCKET;
	}
}

std::tuple<SOCKET, addrinfo*> server::_create_listen_socket() const
{
	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	addrinfo *ptr_addrinfo { nullptr };

	// Resolve the server address and port
	int i_result = getaddrinfo(NULL, _port.c_str(), &hints, &ptr_addrinfo);
	if (i_result != 0)
	{
		stringstream sb;
		sb << "getaddrinfo failed with error: " << i_result;

		throw runtime_error { sb.str() };
	}

	// Create a SOCKET for connecting to server
	auto listen_socket = socket(ptr_addrinfo->ai_family, ptr_addrinfo->ai_socktype, ptr_addrinfo->ai_protocol);
	if (listen_socket == INVALID_SOCKET)
	{
		stringstream sb;
		sb << "socket failed with error: " << WSAGetLastError();

		freeaddrinfo(ptr_addrinfo);

		throw runtime_error { sb.str() };
	}

	return make_tuple(listen_socket, ptr_addrinfo);
}

SOCKET server::_listen()
{
	// Setup the TCP listening socket
	auto i_result = ::bind(_listen_socket, _ptr_server_address->ai_addr, (int)_ptr_server_address->ai_addrlen);
	if (i_result == SOCKET_ERROR)
	{
		stringstream sb;
		sb << "bind failed with error: " << WSAGetLastError();
		throw runtime_error{ sb.str() };
	}


	i_result = listen(_listen_socket, SOMAXCONN);
	if (i_result == SOCKET_ERROR)
	{
		stringstream sb;
		sb << "listen failed with error: " << WSAGetLastError();
		throw runtime_error{ sb.str() };
	}

	// Accept a client socket
	auto client_socket = accept(_listen_socket, NULL, NULL);
	if (client_socket == INVALID_SOCKET)
	{
		stringstream sb;
		sb << "accept failed with error: " << WSAGetLastError();
		throw runtime_error{ sb.str() };
	}

	return client_socket;
}

end_point server::listening()
{
	return { _listen(), _buffer_size };
}


end_point::end_point(SOCKET socket_, size_t buffer_size_)
	: _socket { socket_ }
	, _buffer_size { buffer_size_ }
{
}

end_point::~end_point()
{
	// shutdown the connection since we're done
	int i_result = shutdown(_socket, SD_SEND);
	if (i_result == SOCKET_ERROR)
	{
		stringstream sb;
		sb << "shutdown failed with error: " << WSAGetLastError();

		// what to do here?
	}

	closesocket(_socket);
	_socket = INVALID_SOCKET;
}

size_t end_point::read(char* ptr_data_, size_t size_) const
{
	// recv won't return with zero (*), it rather waits
	// if data.size == buffer.size then on the next call it blocks
	//
	//	so it's not possible to make difference whenever no more data will be sent or
	//	the size of the current batch is exactly the size of the buffer
	//
	// (*) only when it disconnected
	auto status_or_size = recv(_socket, ptr_data_, size_, 0);

	if (status_or_size == SOCKET_ERROR)
	{
		int ec = WSAGetLastError();

		char msg_buffer[1024];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ec, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg_buffer, 1024, NULL);

		stringstream sb;
		sb << "I/O error: " << msg_buffer << " (" << ec << ")";
			
		throw runtime_error { sb.str() };
	}

	return status_or_size;
}

void end_point::write(const char* ptr_data_, size_t size_) const
{
	int i_result = send(_socket, ptr_data_, size_, 0);
	if (i_result == SOCKET_ERROR)
	{
		stringstream sb;
		sb << "send failed with error: " << WSAGetLastError();

		throw runtime_error	{ sb.str() };
	}
}


client::client(string address_, desc desc_)
	: end_point { _create_socket(move(address_), desc_.port), desc_.buffer_size }
	, _port { move(desc_.port) }
{
}

client::~client()
{
}

SOCKET client::_create_socket(string address_, string port_)
{
	// ensure that WSA is initialized
	// WSACleanup() is called by the dtor of WSAInit at the termiantion of this process
	gl_storage().get_singleton_of<WSAInit>();

	addrinfo *result = nullptr, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	int i_result = getaddrinfo(address_.c_str(), port_.c_str(), &hints, &result);
	if (i_result != 0)
	{
		stringstream sb;
		sb << "getaddrinfo failed with error: " << i_result;

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
		throw runtime_error{ "Unable to connect to server!" };
	}

	return new_sckt;
}


