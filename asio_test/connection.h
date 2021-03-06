#ifndef _CONNECTION_H
#define _CONNECTION_H


#include <functional>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <deque>
#include <boost/circular_buffer.hpp>

typedef boost::shared_lock<boost::shared_mutex> ReadLock;
typedef boost::unique_lock<boost::shared_mutex> WriteLock;

typedef std::function<void(boost::system::error_code ec)> callback_t;
class connection_mgr;

extern boost::mutex g_mutex_IO;

class conn_msg : public boost::noncopyable
{
public:
	
	enum { max_body_length = 256 };

	//the size speficied in header.len can changed in set_body function 
	//later(the header only speficy the package type)
	conn_msg(const HDR& header);
	~conn_msg();

	/*conn_msg* clone()
	{
		conn_msg* clone_obj = new conn_msg(header_);
		clone_obj->set_body(body(), body_length());

		return clone_obj;
	}*/

	char* body()
	{
		return buff_;
	}

	size_t body_length() const
	{
		return header_.len;
	}

	size_t capacity() const
	{
		return size_allocated_ > 0 ? size_allocated_ : max_body_length;
	}

	void body_length(size_t nSize);

	char* header()
	{
		return (char*)(&header_);
	}

	void header(HDR& header)
	{
		header = header_;
	}

	size_t header_length() const
	{
		return HEADER_SIZE;
	}

	bool set_body(const boost::asio::streambuf& buf);
	bool set_body(void* pData, size_t nSize);

private:
	char  buff_fix[max_body_length];
	char* buff_;
	size_t size_allocated_; //size allocated

	HDR   header_; //the header info

};

typedef std::deque<conn_msg*> conn_message_queue;

///////////////////////////////////////////////////////////////////////////////////////////////////

class session_tcp : public boost::enable_shared_from_this<session_tcp>
{
public:

	typedef boost::shared_ptr<session_tcp> pointer;

	session_tcp(boost::asio::io_service& ios, connection_mgr& conn_mgr);

	virtual ~session_tcp();

	static pointer create(boost::asio::io_service& ios, connection_mgr& conn_mgr)
	{
		return pointer(new session_tcp(ios, conn_mgr));
	}

	boost::asio::ip::tcp::socket& socket()
	{
		ReadLock lock(mutex_);
		return socket_;
	}

	UINT client_id()
	{
		ReadLock lock(mutex_);
		return client_id_;
	}

	void start(UINT clientid);

	//call Write to send a msg
	void write(conn_msg* msg);

	bool output_console(const char * _Format, ...);
	bool output_console(const std::string& str);

public:

	//2017/9/12 yang add get remote address
	std::string remote_address()
	{
		ReadLock lock(mutex_);

		boost::asio::ip::tcp::endpoint ep = socket_.remote_endpoint();
		
		//UINT port = ep.port();
		
		return ep.address().to_v4().to_string();
	}
	
	//2017/9/12 yang add get local address
	std::string local_address()
	{
		ReadLock lock(mutex_);
		boost::asio::ip::tcp::endpoint ep = socket_.local_endpoint();
		
		//UINT port = ep.port();
		return ep.address().to_v4().to_string();
	}

private:
	
	void do_read();
	void handler_read_header(boost::system::error_code error);
	void handler_read_body(boost::system::error_code error);

	void do_write();
	void handler_write_header(boost::system::error_code error);
	void handler_write_body(boost::system::error_code error);
	
	int ProcessMsg();

private: 

	//can override the function to handle own business
	virtual int ProcessMsg_business(conn_msg* msg);

private:
	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf buf_; //buf to read, only in reading

	connection_mgr& conn_mgr_;

	HDR  header_; //message header to read
	UINT curr_pack_num_; //current 
	UINT client_id_;

	//we can not send multiple data at one time
	//should be one after one
	conn_message_queue msg_queue_;

	//add the mutex
	boost::shared_mutex mutex_;
};

typedef boost::shared_ptr<session_tcp> connection_ptr;

class connection_mgr : public boost::noncopyable
{
public:
	connection_mgr(UINT max_conn);

	bool can_join()
	{
		ReadLock lock(mutex_);

		if (connection_list_.size() > max_conn_)
			return false;
		else
			return true;
	}

	bool join(connection_ptr incomer)
	{
		WriteLock lock(mutex_);
		if (connection_list_.size() > max_conn_)
			return false;

		connection_list_.insert(incomer);
		
		return true;
	}

	void leave(connection_ptr outcomer)
	{
		WriteLock lock(mutex_);
		connection_list_.erase(outcomer);
	}

	//stop : to disconnect all the clients
	void stop();

	//clear the msg buffer in circular buffer
	void clear_buffer();

	connection_ptr find(UINT clientid);

	//allocate the msg buffer 
	conn_msg* allocate_msg_buffer(const HDR& header);
	void dellocate_msg_buffer(conn_msg* msg);

	size_t size()
	{
		ReadLock lock(mutex_);
		return connection_list_.size();
	}
	
	void write_to_all(conn_msg* msg);

private:
	std::set<connection_ptr> connection_list_;

	UINT  max_conn_;  //maximum connections

	boost::shared_mutex mutex_;
	boost::shared_mutex buffer_mutex_;

	boost::circular_buffer<conn_msg*> conn_msg_buffer_;
};

//using boost::asio::ip::udp;

////server for udp
//class server_udp
//{
//	
//
//public:
//	server_udp(boost::asio::io_service& ios, short port)
//		:ios_(ios)
//		, socket_(ios, udp::endpoint(udp::v4(), port))
//	{
//		start();
//	}
//
//	void start()
//	{
//		socket_.async_receive_from(boost::asio::buffer(data_buff, max_length),
//			sender_endpoint_,
//			boost::bind(&server_udp::handler_read, this,
//			boost::asio::placeholders::error,
//			boost::asio::placeholders::bytes_transferred));
//	}
//
//	void handler_read(const boost::system::error_code error, size_t byte_recv)
//	{
//		if (!error && byte_recv > 0)
//		{
//			socket_.async_send_to(boost::asio::buffer(data_buff, byte_recv),
//				sender_endpoint_, 
//				boost::bind(&server_udp::handler_write, this,
//				boost::asio::placeholders::error,
//				boost::asio::placeholders::bytes_transferred));
//		}
//		else
//		{
//			socket_.async_receive_from(boost::asio::buffer(data_buff, max_length),
//				sender_endpoint_,
//				boost::bind(&server_udp::handler_write, 
//				this, boost::asio::placeholders::error,
//				boost::asio::placeholders::bytes_transferred));
//		}
//	}
//
//	void handler_write(const boost::system::error_code error, size_t /*byte_send*/)
//	{
//		socket_.async_receive_from(boost::asio::buffer(data_buff, max_length),
//			sender_endpoint_,
//			boost::bind(&server_udp::handler_write,
//			this, boost::asio::placeholders::error,
//			boost::asio::placeholders::bytes_transferred));
//	}
//	
//
//private:
//	boost::asio::io_service& ios_;
//	udp::endpoint sender_endpoint_;
//
//	udp::socket socket_;
//	enum { max_length = 1024 };
//	char data_buff[max_length];
//};

#endif //_CONNECTION_H