#pragma once

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#include <iostream>
#include <vector>


namespace utility
{
	class nistream : std::istream
	{
	public:
	private:
	};

	class nostream : std::ostream
	{
	public:
	private:
	};

	class niostream : std::iostream
	{
	public:
	private:
	};



	struct desc
	{
		size_t buffer_size { 512 };
		std::string port;

		desc(std::string);
	};

	class server
	{
	public:
		server(desc);
		~server();

		std::vector<char> read() const;
		void write(std::vector<char>) const;

	private:
		SOCKET _client_socket { INVALID_SOCKET };

		size_t _buffer_size;
		std::string _port;

		SOCKET _create_listen_socket() const;
	};

	class client
	{
	public:
		client(std::string, desc);
		~client();

		std::vector<char> read() const;
		void write(std::vector<char>) const;

	private:
		SOCKET _socket{ INVALID_SOCKET };

		size_t _buffer_size;
		std::string _port;

		SOCKET _create_socket(std::string) const;
	};
}
