#include <utility\network.hpp>
#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

#pragma comment(lib, "utility.lib")

using namespace std;
using namespace utility;


template<class D, class S> D to_(S str_)
{
	return
	{
		begin(str_),
		end(str_)
	};
}

mutex mtx_io;
atomic_bool running{ true };

void reader(server& s_)
{
	try
	{
		while (running)
		{
			auto data = to_<string>(s_.read());
			if (!data.empty())
			{
				//lock_guard<mutex> l{ mtx_io };
				cout << "[server] message from server: " << data << endl;
			}
		}
	}
	catch(exception& e_)
	{
		cerr << e_.what() << endl;
	}
}

void writer(server& s_)
{
	while (true)
	{
		//lock_guard<mutex> l{ mtx_io };

		cout << "[server] >> ";

		string msg;
		cin >> msg;

		if (msg == "exit")
			return;

		s_.write(to_<vector<char>>(msg));
	}

	running = false;
}

void main()
{
	server s { desc { "1234" } };

	thread th_reader{ reader, std::ref(s) };
	thread th_writer{ writer, std::ref(s) };

	th_writer.join();
	th_reader.join();
}
