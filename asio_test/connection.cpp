#include "stdafx.h"
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "connection.h"

//////////////////////////////////////////////////////////////////////////////////
conn_msg::conn_msg(const HDR& header)
	: buff_(NULL)
	, size_allocated_(0)
{
	header_ = header;
	
	
}

conn_msg::~conn_msg()
{
	if (buff_ != buff_fix)
	{
		delete[] buff_;
		buff_ = NULL;
	}
}

void conn_msg::body_length(size_t nSize)
{

	if (size_allocated_) //already allocated
	{
		assert(buff_);

		if (nSize <= size_allocated_)
			NULL;
		else //not enough size, should reallocate later
		{
			delete[] buff_;
			buff_ = NULL;
			size_allocated_ = 0;
		}
	}

	//already allocated
	if (size_allocated_)
	{
		assert(buff_);
		return;
	}

	header_.len = nSize;
	if (header_.len > max_body_length)
	{
		buff_ = new char[header_.len];
		size_allocated_ = header_.len;
	}
	else
	{
		buff_ = buff_fix;

	}

	memset(buff_, 0, header_.len);
}

bool conn_msg::set_body(const boost::asio::streambuf& buf)
{
	body_length(buf.size());

	boost::asio::const_buffers_1 src_data = buf.data();
	const char* src = boost::asio::buffer_cast<const char*>(src_data);

	memcpy(body(), src, body_length());
	return true;
}

bool conn_msg::set_body(void* pData, size_t nSize)
{
	body_length(nSize);

	memcpy(buff_, pData, nSize);
	return true;
}
//////////////////////////////////////////////////////////////////////////////////



void session_tcp::start(UINT clientid)
{
	client_id_ = clientid;

	conn_mgr_.join(shared_from_this());

	//initiate the header read
	do_read();
}

void session_tcp::write(conn_msg* msg)
{
	
	WriteLock lock(mutex_);

	if (msg_queue_.empty())
	{
		//no write task in process, directly call do_write
		msg_queue_.push_back(msg);
		lock.unlock();
		do_write();
		return;
	}
	

	//only add to the queue 
	msg_queue_.push_back(msg);
	
	
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
		//first issue another read
		do_read();

		//then process the msg
		ProcessMsg();
	}
	else
	{
		std::cout << "error occured" << std::endl;
		conn_mgr_.leave(shared_from_this());

	}
}

//pick up front item(if have) from queue and copy data to 
//then call async_write to send 
void session_tcp::do_write()
{
	/*{
		WriteLock lock(mutex_);
		header_.packNum = curr_pack_num_++;
	}*/
	
	ReadLock lock(mutex_);
	if (msg_queue_.empty())
	{
		//no need to write
		return;
	}
	
	//get the front msg
	conn_msg* msg = msg_queue_.front();
	
	//write header first
	boost::asio::async_write(socket_, boost::asio::buffer(msg->header(), msg->header_length()),
		boost::bind(&session_tcp::handler_write_header, shared_from_this(),
		boost::asio::placeholders::error));

}

void session_tcp::handler_write_header(boost::system::error_code error)
{
	if (!error)
	{
		//here should log the success
		//...

		ReadLock lock(mutex_);
		if (msg_queue_.empty())
		{
			//there should be any msg to write in queue
			assert(false);
			return;
		}

		//get the front msg
		conn_msg* msg = msg_queue_.front();

		//and then we write body
		boost::asio::async_write(socket_, boost::asio::buffer(msg->body(), msg->body_length()),
			boost::bind(&session_tcp::handler_write_body, shared_from_this(),
			boost::asio::placeholders::error));

		
	}
	else
	{
		std::cout << "error occured" << std::endl;
		conn_mgr_.leave(shared_from_this());
	}
}

void session_tcp::handler_write_body(boost::system::error_code error)
{
	if (!error)
	{
		{
			WriteLock lock(mutex_);
			conn_msg* msg = msg_queue_.front();
			delete msg;
			msg_queue_.pop_front(); //pop the front item

			std::cout << "write complete" << std::endl;
			
		}

		//try to do another write if possible
		do_write();

	}
	else
	{
		std::cout << "error occured" << std::endl;
		conn_mgr_.leave(shared_from_this());
	}
}

int session_tcp::ProcessMsg()
{

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

void connection_mgr::write_to_all(conn_msg* msg)
{
	ReadLock lock(mutex_);

	std::set<connection_ptr>::iterator it = connection_list_.begin();
	for (; it != connection_list_.end(); ++it)
	{
		session_tcp* session = it->get();

		if (!session)
			continue;

		session->write(msg->clone());
	}
}