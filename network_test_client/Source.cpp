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
atomic_bool running { true };

void reader(client& c_)
{
	while (running)
	{
		auto data = to_<string>(c_.read());
		if (!data.empty())
		{
			//lock_guard<mutex> l{ mtx_io };
			cout << "[client] message from server: " << data << endl;
		}
	}
}

void writer(client& c_)
{
	while (true)
	{
		cout << "[client] >> ";
		string msg;
		cin >> msg;

		if(msg == "exit")
			return;

		c_.write(to_<vector<char>>(msg));
	}

	running = false;
}


void main()
{
	client c{ "localhost",{ "1234" } };

	//thread th_reader { reader, std::ref(c) };
	thread th_writer { writer, std::ref(c) };

	th_writer.join();
	//th_reader.join();
}