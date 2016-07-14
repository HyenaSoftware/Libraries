#pragma once
#include "stdafx.h"

#undef UNICODE

namespace utility
{
	struct desc
	{
		size_t buffer_size;
		std::string port;

		desc(std::string, size_t = 512);
	};

	class end_point
	{
	public:
		end_point(SOCKET, size_t buffer_size_);
		~end_point();

		size_t read(char* ptr_date_, size_t size_) const;
		void write(const char* ptr_data_, size_t size_) const;

	protected:
		SOCKET _socket{ INVALID_SOCKET };
		size_t _buffer_size;

	private:
	};

	class server
	{
	public:
		server(desc);
		~server();

		/*
			blocks and try to 
		*/
		end_point listening();

	private:
		SOCKET _listen_socket { INVALID_SOCKET };

		addrinfo* _ptr_server_address;

		std::string _port;
		size_t _buffer_size;

		std::tuple<SOCKET, addrinfo*> _create_listen_socket() const;
		SOCKET _listen();
	};

	class client : public end_point
	{
	public:
		client(std::string address_, desc);
		~client();

	private:
		std::string _port;

		static SOCKET _create_socket(std::string address_, std::string port_);
	};

}
