#ifndef _ECHOSERVER_H
#define _ECHOSERVER_H

#include "../globlItem.h"
#include <functional>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>
#include <set>

typedef boost::shared_lock<boost::shared_mutex> ReadLock;
typedef boost::unique_lock<boost::shared_mutex> WriteLock;

typedef std::function<void(boost::system::error_code ec)> callback_t;
class connection_mgr;

class session_tcp : public boost::enable_shared_from_this<session_tcp>
{
public:

	typedef boost::shared_ptr<session_tcp> pointer;

	session_tcp(boost::asio::io_service& ios, connection_mgr& conn_mgr)
		: socket_(ios)
		, conn_mgr_(conn_mgr)
	{
		curr_pack_num_ = 0;
		client_id_ = 0;
		sending_ = false;
	}

	~session_tcp()
	{
		
	}

	static pointer create(boost::asio::io_service& ios, connection_mgr& conn_mgr)
	{
		return pointer(new session_tcp(ios, conn_mgr));
	}

	boost::asio::ip::tcp::socket& socket()
	{
		return socket_;
	}

public:
	void start(UINT clientid);

	void do_read();
	void handler_read_header(boost::system::error_code error);
	void handler_read_body(boost::system::error_code error);

	void do_write();
	void handler_write_header(boost::system::error_code error);

	void handler_write_msg(boost::system::error_code error);
	void prepare_write();
public:
	UINT client_id() const
	{
		return client_id_;
	}

private:

	bool wait_for_write(short millseconds);

	int ProcessMsg(boost::system::error_code error);

	int ProcessConn();
	int ProcessData();
	int ProcessCommand();
	int ProcessRequest();
	int ProcessPulse();

private:
	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf buf_;

	connection_mgr& conn_mgr_;

	HDR  header_; //message header
	UINT curr_pack_num_; //current 
	UINT client_id_;
	//we can not send multiple data at one time
	//should be one after one
	bool sending_; //is sending or not

	//add the mutex
	boost::shared_mutex mutex_;
	boost::mutex cond_mutex_;
	boost::condition_variable cond_;
};

typedef boost::shared_ptr<session_tcp> connection_ptr;

class connection_mgr : public boost::noncopyable
{
public:
	void join(connection_ptr incomer)
	{
		WriteLock lock(mutex_);
		connection_list_.insert(incomer);
	}

	void leave(connection_ptr outcomer)
	{
		WriteLock lock(mutex_);
		connection_list_.erase(outcomer);
	}

	connection_ptr find(UINT clientid);
	

private:
	std::set<connection_ptr> connection_list_;

	boost::shared_mutex mutex_;
};

using boost::asio::ip::udp;

//server for udp
class server_udp
{
	

public:
	server_udp(boost::asio::io_service& ios, short port)
		:ios_(ios)
		, socket_(ios, udp::endpoint(udp::v4(), port))
	{
		start();
	}

	void start()
	{
		socket_.async_receive_from(boost::asio::buffer(data_buff, max_length),
			sender_endpoint_,
			boost::bind(&server_udp::handler_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}

	void handler_read(const boost::system::error_code error, size_t byte_recv)
	{
		if (!error && byte_recv > 0)
		{
			socket_.async_send_to(boost::asio::buffer(data_buff, byte_recv),
				sender_endpoint_, 
				boost::bind(&server_udp::handler_write, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			socket_.async_receive_from(boost::asio::buffer(data_buff, max_length),
				sender_endpoint_,
				boost::bind(&server_udp::handler_write, 
				this, boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
	}

	void handler_write(const boost::system::error_code error, size_t /*byte_send*/)
	{
		socket_.async_receive_from(boost::asio::buffer(data_buff, max_length),
			sender_endpoint_,
			boost::bind(&server_udp::handler_write,
			this, boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
	

private:
	boost::asio::io_service& ios_;
	udp::endpoint sender_endpoint_;

	udp::socket socket_;
	enum { max_length = 1024 };
	char data_buff[max_length];
};

#endif //_ECHOSERVER_H