#include "stdafx.h"
#include <boost/thread.hpp>
#include "connection.h"
#include "server.h"

///////////////////////////////////////////////////////////////////////////////////////////
void server_tcp::start(int port)
{

	ios_.reset();

	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), port);

	acceptor_.open(ep.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(ep);
	acceptor_.listen();

	output_console("the server start to listen.");

	start_accept();

#if IO_THREAD_IN_CLASS == 1
	io_thread_array_.resize(io_thread_num_);

	for (UINT i = 0; i < io_thread_num_; i++)
	{
		io_thread_array_[i] = new boost::thread(boost::bind(&(boost::asio::io_service::run), &ios_));
	}

#endif //IO_THREAD_IN_CLASS


}

void server_tcp::stop()
{
	output_console("prepare to stop the server...");

	acceptor_.cancel();
	acceptor_.close();

	//disconnect the clients
	conn_mgr_.stop();

	ios_.stop();

#if IO_THREAD_IN_CLASS == 1
	for (UINT i = 0; i < io_thread_num_; i++)
	{
		io_thread_array_[i]->join();
		delete io_thread_array_[i];
		io_thread_array_[i] = NULL;
	}

	io_thread_array_.clear();

	output_console("the server is stopped.");

	conn_mgr_.clear_buffer();


#endif //IO_THREAD_IN_CLASS
}