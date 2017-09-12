// asio_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
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

		//boost::asio::io_service io_service;


		using namespace std; // For atoi.

		server_tcp s(18, 4);
		s.start(4567);

		//io_service.run();

		//HDR header;
		//::ZeroMemory(&header, sizeof(header));
		//header.len = strlen("this is command string:")+4;

		//conn_msg msg(header);
		//boost::asio::streambuf buf;
		//size_t nCount = 0;
		//

		//for (;;)
		//{
		//	//first clear the content
		//	buf.consume(buf.size());

		//	boost::archive::binary_oarchive oa(buf);
		//	oa << "this is command string:" << nCount;

		//	msg.set_body(buf);

		//	s.write_to_all(&msg);

		//	//nCount++;
		//	boost::this_thread::sleep(boost::posix_time::milliseconds(100));

		//	/*if (s.client_num() >= 10)
		//	{
		//		nCount++;
		//		if (nCount > 2)
		//			break;
		//	}*/
		//}

		boost::this_thread::sleep(boost::posix_time::seconds(50));


		s.stop();

		s.start(4567);

		for (;;)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(10));
		}


		
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

