// asio_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>


#include "connection.h"
#include "server.h"



int _tmain(int argc, char* argv[])
{
	try
	{

		/*if (argc != 2)
		{
			std::cerr << "Usage: async_tcp_echo_server <port>\n";
			return 1;
		}*/

		boost::asio::io_service io_service;

		using namespace std; // For atoi.
		//server_tcp s(io_service, atoi(argv[1]));
		//server_tcp s(io_service, atoi(argv[1]));
		server_tcp s(io_service, 4567);

		//io_service.run();

		boost::thread t1(boost::bind(&(boost::asio::io_service::run), &io_service));
		//boost::thread t2(boost::bind(&(boost::asio::io_service::run), &io_service));
		//boost::thread t3(boost::bind(&(boost::asio::io_service::run), &io_service));

		/*for (;;)
		{



			boost::this_thread::sleep(boost::posix_time::milliseconds(20));
		}*/

		t1.join();
		//t2.join();
		//t3.join();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

