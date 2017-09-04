#include "stdafx.h"
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "connection.h"

//////////////////////////////////////////////////////////////////////////////////
void session_tcp::start(UINT clientid)
{
	client_id_ = clientid;

	conn_mgr_.join(shared_from_this());

	//initiate the header read
	do_read();
}

void session_tcp::do_read()

{
	boost::asio::async_read(socket_, boost::asio::buffer(&header_, HEADER_SIZE),
		boost::bind(&session_tcp::handler_read_header, shared_from_this(),
		boost::asio::placeholders::error));
}

void session_tcp::handler_read_header(boost::system::error_code error)
{
	if (!error)
	{
		std::cout << "read header bytes" << header_.len << std::endl;
		//assert(bytes_transferred == max_length);
		buf_.prepare(header_.len);

		boost::asio::async_read(socket_, buf_, boost::asio::transfer_exactly(header_.len),
			boost::bind(&session_tcp::handler_read_body, shared_from_this(),
			boost::asio::placeholders::error));
	}
	else
	{
		//delete this;
		std::cout << "error occured" << std::endl;
		conn_mgr_.leave(shared_from_this());
	}
}

void session_tcp::handler_read_body(boost::system::error_code error)
{
	//after read body process, handle the msg
	if (!error)
	{
		ProcessMsg(error);
	}
	else
	{
		std::cout << "error occured" << std::endl;
		conn_mgr_.leave(shared_from_this());

	}
}

void session_tcp::do_write()
{

	header_.packNum = curr_pack_num_++;

	{
		boost::mutex::scoped_lock lock1(cond_mutex_);
		sending_ = true;
	}
	

	//write header first
	boost::asio::async_write(socket_, boost::asio::buffer(&header_, HEADER_SIZE),
		boost::bind(&session_tcp::handler_write_header, shared_from_this(),
		boost::asio::placeholders::error));

}

void session_tcp::handler_write_header(boost::system::error_code error)
{
	if (!error)
	{
		//here should log the success
		//...


		//and then we write body
		boost::asio::async_write(socket_, buf_, boost::asio::transfer_exactly(header_.len),
			boost::bind(&session_tcp::handler_write_msg, shared_from_this(),
			boost::asio::placeholders::error));

		buf_.consume(buf_.size());
	}
	else
	{
		std::cout << "error occured" << std::endl;
		conn_mgr_.leave(shared_from_this());
	}
}

void session_tcp::handler_write_msg(boost::system::error_code error)
{
	if (!error)
	{
		assert(sending_ == true);

		{
			boost::mutex::scoped_lock lock1(cond_mutex_);
			std::cout << "write complete" << std::endl;
			sending_ = false;
			cond_.notify_one();
		}

		

		do_read();

	}
	else
	{
		std::cout << "error occured" << std::endl;
		conn_mgr_.leave(shared_from_this());
	}
} 

bool session_tcp::wait_for_write(short millseconds)
{
	boost::mutex::scoped_lock lock1(cond_mutex_);

	if (!sending_)
	{
		return true;
	}
	else
	{
		
		while (sending_)
		{
			cond_.wait(lock1);
		}

		return true;
	}
		
}

int session_tcp::ProcessMsg(boost::system::error_code error)
{
	if (error)
	{
		delete this;
		return 1;
	}

	////header is header_ and body is buf_
	int nRet = 0;

	//here we only echo back the msg
	do_write();

	////dispacth to message handler assording to msg type
	//switch (header_.type)
	//{
	//case 0:
	//	nRet = ProcessConn();
	//	break;
	//case 1:
	//	nRet = ProcessRequest();
	//	break;
	//case 2:
	//	nRet = ProcessCommand();
	//	break;
	//case 3:
	//	nRet = ProcessPulse();
	//	break;
	//case 4:
	//	nRet = ProcessData();
	//	break;
	//default:
	//	nRet = ProcessConn();
	//	break;
	//}

	return nRet;
}

int session_tcp::ProcessConn()
{
	return 0;
}

int session_tcp::ProcessData()
{
	return 0;
}

int session_tcp::ProcessCommand()
{
	return 0;
}

int session_tcp::ProcessRequest()
{
	return 0;
}

int session_tcp::ProcessPulse()
{
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
connection_ptr connection_mgr::find(UINT clientid)
{
	if (clientid == 0)
	{
		return NULL;
	}

	ReadLock lock(mutex_);

	std::set<connection_ptr>::iterator it = connection_list_.begin();
	for (; it != connection_list_.end(); ++it)
	{
		if (it->get() == NULL)
		{
			//ERROR
			continue;
		}

		if (it->get()->client_id() == clientid)
		{
			std::cout << "before connection_mgr::find return: count=" << it->use_count() << std::endl;
			return *it;
		}
	}

	return NULL;
}