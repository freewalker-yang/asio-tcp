// asio_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

#include <boost\bind.hpp>
#include <boost\thread.hpp>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#define USE_TCP  0
#define USE_UDP  1
#define USE_TCPDCP  USE_TCP

#define LOG_RECV_ENABLE  1

typedef boost::shared_lock<boost::shared_mutex> ReadLock;
typedef boost::unique_lock<boost::shared_mutex> WriteLock;

boost::shared_mutex mutex;

enum { max_length = 1024*1024 }; //1M

void RunConnection(int num, char* argv[])
{
	
	boost::system::error_code error;
	boost::asio::io_service io_service;

	try
	{

#if USE_TCPDCP == USE_TCP

	using boost::asio::ip::tcp;

	tcp::resolver resolver(io_service);
	using boost::asio::ip::tcp;

	//tcp::resolver::query query(tcp::v4(), argv[1], argv[2]);
	tcp::resolver::query query(tcp::v4(), "localhost", "4567");
	tcp::resolver::iterator iterator = resolver.resolve(query);

	tcp::socket s(io_service);
	boost::asio::connect(s, iterator, error);

	using namespace std; // For strlen.

	if (error)
	{
		cout << "thread " << num << " connection error!" << endl;
		return;
	}

	
	{
		WriteLock lock(mutex);
		cout << "thread " << num << ":connected" << endl;
	}
	

	/*if (error)
	std::cout << boost::asio::;*/

	
	//std::cout << "Enter message: ";
	char* request = new char[max_length];
	//std::cin.getline(request, max_length);
	_stprintf_s(request, max_length, "thread %d:here is the message i deliver to you!", num);
	

	HDR header, header_rcv;
	memset(&header, 0, sizeof(header));
	memset(&header_rcv, 0, sizeof(header_rcv));

	size_t request_length = max_length - sizeof(header); // strlen(request);

	header.len = request_length;
	
	//first is the header
	size_t nSize = boost::asio::write(s, boost::asio::buffer(&header, HEADER_SIZE));
	//then body
	nSize += boost::asio::write(s, boost::asio::buffer(request, request_length));

	//char* reply = new char[max_length];
	int nTotalCnt = 100, nCount = 0;
	DWORD64 tick_1, tick_2;
	boost::asio::streambuf buf;
	size_t nPackCnt = 0;

	do
	{
		header.packNum++;

	


		tick_1 = GetTickCount64();

		size_t reply_length = 0, nLen = 0;

		

		//first read the header
		reply_length = boost::asio::read(s, boost::asio::buffer(&header_rcv, HEADER_SIZE));
		//buf.prepare(header.len);
		assert(reply_length == HEADER_SIZE);
		reply_length = boost::asio::read(s, buf, boost::asio::transfer_exactly(header_rcv.len));
		assert(reply_length == header_rcv.len);

		nPackCnt++;

		nSize = boost::asio::write(s, boost::asio::buffer(&header_rcv, HEADER_SIZE));
		assert(nSize == HEADER_SIZE);
		//then body	
		nSize = boost::asio::write(s, buf, boost::asio::transfer_exactly(header_rcv.len));
		assert(nSize == header_rcv.len);

		buf.consume(buf.size());

		tick_2 = GetTickCount64();

		DWORD64 dwTick = tick_2 - tick_1;

#if LOG_RECV_ENABLE == 1
		{
			WriteLock lock(mutex);
			cout << "id=[" << num << "]package recv[len=" << header_rcv.len;
			cout << ", index=" << nPackCnt << "]" << endl;
		}
#endif //LOG_RECV_ENABLE
		

		boost::this_thread::sleep(boost::posix_time::milliseconds(5));

	} while (nCount++ < nTotalCnt);

	/*std::cout << "Reply is: ";
	std::cout.write(reply, reply_length);
	std::cout << "\n";*/
#else
	using boost::asio::ip::udp;

	udp::socket s(io_service, udp::endpoint(udp::v4(), 0));

	udp::resolver resolver(io_service);
	udp::resolver::query query(udp::v4(), argv[1], argv[2]);
	udp::resolver::iterator iterator = resolver.resolve(query);

	using namespace std; // For strlen.
	std::cout << "Enter message: ";
	char request[max_length];
	std::cin.getline(request, max_length);
	size_t request_length = strlen(request);
	s.send_to(boost::asio::buffer(request, request_length), *iterator);

	char reply[max_length];
	udp::endpoint sender_endpoint;
	size_t reply_length = s.receive_from(
		boost::asio::buffer(reply, max_length), sender_endpoint);
	std::cout << "Reply is: ";
	std::cout.write(reply, reply_length);
	std::cout << "\n";
#endif 

	}
	catch (std::exception& e)
	{
		WriteLock lock(mutex);
		std::cerr << "thread " << num << ":Exception: " << e.what() << "\n";
		return;
	}

	{
		WriteLock lock(mutex);
		std::cout << "thread " << num << ":exit" << std::endl;
	}
}

int _tmain(int argc, char* argv[])
{
	
		
		/*if (argc != 3)
		{
			std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
			return 1;
		}*/

		//using namespace boost;
		
		DWORD tick_1 = GetTickCount();

		boost::thread_group group;
		for (int num = 0; num<20; num++)
			group.create_thread(boost::bind(&RunConnection, num, argv));
		group.join_all();

		DWORD tick = GetTickCount() - tick_1;

		std::cout << "finished in " << tick << " ms" << std::endl;
	
		system("pause");

	return 0;
}


