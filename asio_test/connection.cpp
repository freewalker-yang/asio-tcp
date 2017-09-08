#include "stdafx.h"
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/locks.hpp>
#include <string>
#include "connection.h"


boost::mutex g_mutex_IO;


void std_string_format(std::string & _str, const char * _Format, ...) 
{
	std::string tmp;

	va_list marker = NULL;
	va_start(marker, _Format);

	size_t num_of_chars = _vscprintf(_Format, marker);

	if (num_of_chars > tmp.capacity()) {
		tmp.resize(num_of_chars + 1);
	}

	vsprintf_s((char *)tmp.data(), tmp.capacity(), _Format, marker);

	va_end(marker);

	_str = tmp.c_str();
	
}

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
		header_.len = nSize; //set current size
		return;
	}

	//not allocated
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

	//{
	//	//boost::unique_lock<boost::mutex> lock(g_mutex_IO);
	//	boost::mutex::scoped_lock lock(g_mutex_IO);
	//	std::cout << "new client accepted:id=" << client_id_ << std::endl;
	//}
	output_console("new client accepted");
	

	//initiate the header read
	do_read();
}

bool session_tcp::output_console(char* msg)
{
	boost::mutex::scoped_lock lock(g_mutex_IO);
	unsigned long thread_id = ::GetCurrentThreadId();

	std::string strPrefix;
	std_string_format(strPrefix, "thread[%5d] - id[%2d] : ", thread_id, client_id_);

	std::cout << strPrefix.c_str();
	std::cout << msg << std::endl;

	return true;
}

bool session_tcp::output_console(const std::string& str)
{
	
	boost::mutex::scoped_lock lock(g_mutex_IO);
	unsigned long thread_id = ::GetCurrentThreadId();

	std::string strPrefix;
	std_string_format(strPrefix, "thread[%5d] - id[%2d] : ", thread_id, client_id_);

	std::cout << strPrefix.c_str();
	std::cout << str.c_str() << std::endl;
	

	return true;
}

void session_tcp::write(conn_msg* msg)
{
	size_t nSize = 0;
	std::string str;
	{
		WriteLock lock(mutex_);

		if (msg_queue_.empty())
		{
			//no write task in process, directly call do_write
			msg_queue_.push_back(msg);
			lock.unlock();
			std_string_format(str, "new msg to write : msgsize = %d", msg->body_length());
			output_console(str);
			do_write();
			return;
		}
	

		//only add to the queue 
		msg_queue_.push_back(msg);
		nSize = msg_queue_.size();
	}


	std_string_format(str, "new msg to write:msgsize=%d,queue size=%d", 
		msg->body_length(),nSize);
	output_console(str);
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
		//std::cout << "read header bytes" << header_.len << std::endl;
		
		
		std::string str;
		std_string_format(str, "handler_read_header:len = %d", header_.len);
		output_console(str);
		
		buf_.prepare(header_.len);

		boost::asio::async_read(socket_, buf_, boost::asio::transfer_exactly(header_.len),
			boost::bind(&session_tcp::handler_read_body, shared_from_this(),
			boost::asio::placeholders::error));
	}
	else
	{
		//delete this;
		//std::cout << "error occured:leave" << std::endl;
		
		std::string error_str = "error occured in handler_read_header:leave()-";
		error_str.append(error.message());
		output_console(error_str);
		conn_mgr_.leave(shared_from_this());
	}
}

void session_tcp::handler_read_body(boost::system::error_code error)
{
	//after read body process, handle the msg
	if (!error)
	{
	
		std::string str;
		std_string_format(str, "new msg received : size = %d", header_.len);
		output_console(str);

		//then process the msg
		ProcessMsg();

		//issue another read
		do_read();
	}
	else
	{
		//std::cout << "error occured" << std::endl;
		//output_console("error occured in handler_read_body:leave");
		std::string error_str = "error occured in handler_read_body:leave()-";
		error_str.append(error.message());
		output_console(error_str);
		conn_mgr_.leave(shared_from_this());

	}
}

//pick up front item(if have) from queue and copy data to 
//then call async_write to send 
void session_tcp::do_write()
{
	
	ReadLock lock(mutex_);
	if (msg_queue_.empty())
	{
		//no need to write
		return;
	}
	
	//get the front msg
	conn_msg* msg = msg_queue_.front();

	std::string str;
	std_string_format(str, "asyc_write : msgsize = %d, queue size=%d", msg->body_length(), msg_queue_.size());
	output_console(str);
	
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
			output_console("failed:there should be any item in queue");

			//__debugbreak();
			//assert(false);
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
		//std::cout << "error occured" << std::endl;
		//output_console("error occured in handler_write_header:leave");
		std::string error_str = "error occured in handler_write_header:leave()-";
		error_str.append(error.message());
		output_console(error_str);
		conn_mgr_.leave(shared_from_this());
	}
}

void session_tcp::handler_write_body(boost::system::error_code error)
{
	if (!error)
	{
		size_t nSize = 0, nSize_msg = 0;
		bool bError = false;
		{
			WriteLock lock(mutex_);
			if (msg_queue_.size() > 0)
			{
				conn_msg* msg = msg_queue_.front();
				nSize_msg = msg->body_length();
				//delete msg;
				conn_mgr_.dellocate_msg_buffer(msg);
				msg_queue_.pop_front(); //pop the front item
			}
			else
			{
				bError = true;
			}

			nSize = msg_queue_.size();
			
		}

		std::string str;
		if (bError)
			output_console("write complete(error occured):queue is empty");
		else
		{
			std_string_format(str, "write complete : sizeMsg = %d,queue size = %d", nSize_msg, nSize);
			output_console(str);
		}
		
		

		//try to do another write if possible
		do_write();

	}
	else
	{
		//std::cout << "error occured" << std::endl;
		//output_console("error occured in handler_write_body:leave");
		std::string error_str = "error occured in handler_write_body:leave()-";
		error_str.append(error.message());
		output_console(error_str);
		conn_mgr_.leave(shared_from_this());
	}
}

int session_tcp::ProcessMsg()
{

	////header is header_ and body is buf_
	int nRet = 0;

	//here we only echo back the msg
	//conn_msg* msg = new conn_msg(header_);
	conn_msg* msg = conn_mgr_.allocate_msg_buffer(header_);
	msg->set_body(buf_);
	write(msg);

	buf_.consume(buf_.size());

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
connection_mgr::connection_mgr()
{
	conn_msg_buffer_.set_capacity(50);
}

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

conn_msg* connection_mgr::allocate_msg_buffer(const HDR& header)
{
	conn_msg* ret = NULL;

#if MSG_BUFFER_ALLOCATOR == 1
	//pick up any item can meet the requirements
	if (header.len <= conn_msg::max_body_length)
	{
		ReadLock lock_read(buffer_mutex_);
		if (conn_msg_buffer_.empty())
		{
			ret = new conn_msg(header);
			return ret;
		}
		else //not empty,  the last item will be used
		{
			lock_read.unlock();

			{
				WriteLock lock(buffer_mutex_);

				if (conn_msg_buffer_.empty())
				{
					lock.unlock();
					ret = new conn_msg(header);
					return ret;
				}
				else
				{
					ret = conn_msg_buffer_.front();
					conn_msg_buffer_.pop_front();
					return ret;
				}
			}
		}
	}
	else //customized size
	{
		ret = new conn_msg(header);
		return ret;
	}
#else
	ret = new conn_msg(header);
	return ret;
#endif //MSG_BUFFER_ALLOCATOR
}

void connection_mgr::dellocate_msg_buffer(conn_msg* msg)
{
	if (!msg)
	{
		assert(false);
		return;
	}

#if MSG_BUFFER_ALLOCATOR == 1
	if (msg->capacity() == conn_msg::max_body_length)
	{
		ReadLock lock_read(buffer_mutex_);
		if (conn_msg_buffer_.full())
		{
			delete msg;
			return;
		}
		else
		{
			lock_read.unlock();

			{
				WriteLock lock(buffer_mutex_);

				if (conn_msg_buffer_.full())
				{
					lock.unlock();
					delete msg;
				}
				else
				{
					conn_msg_buffer_.push_back(msg);
				}
			}
		}
	}
	else  //customize size, free it
	{
		delete msg;
	}
#else
	delete msg;
#endif //#if MSG_BUFFER_ALLOCATOR == 1
}

void connection_mgr::write_to_all(conn_msg* msg)
{
	ReadLock lock(mutex_);

	HDR header;
	msg->header(header);

	std::set<connection_ptr>::iterator it = connection_list_.begin();
	for (; it != connection_list_.end(); ++it)
	{
		session_tcp* session = it->get();

		if (!session)
			continue;

		//session->write(msg->clone());
		conn_msg* msg_new = allocate_msg_buffer(header);
		msg_new->set_body(msg->body(), msg->body_length());
		session->write(msg_new);
	}
}