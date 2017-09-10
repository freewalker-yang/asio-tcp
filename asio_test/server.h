#ifndef _SERVER_H
#define _SERVER_H

//#include <thread>
#include <boost/bind.hpp>


class server_tcp
{
public:

	enum { max_io_thread = 10 };

	server_tcp(unsigned int io_thread_num = boost::thread::hardware_concurrency())
		:acceptor_(ios_)
		, curr_client_id_(1)
		, io_thread_num_(io_thread_num)
	{
		
		assert(io_thread_num_ <= max_io_thread);
		
	}

	void start(int port);

	void stop();

	bool stopped() const
	{
		return ios_.stopped();
	}

	void write_to_all(conn_msg* msg)
	{
		conn_mgr_.write_to_all(msg);
	}

	size_t client_num()
	{
		return conn_mgr_.size();
	}

private:
	void start_accept()
	{
		session_tcp::pointer new_session = session_tcp::create(ios_, conn_mgr_);

		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server_tcp::handler_accept, this, new_session,
			boost::asio::placeholders::error));
	}

	void handler_accept(session_tcp::pointer new_session, const boost::system::error_code& error)
	{
		if (!error)
		{
			new_session->start(curr_client_id_++);
		}
		else
		{
			output_console("new client accepted error(%s)", error.message().c_str());
			//delete new_session.get();
			conn_mgr_.leave(new_session);
		}

		start_accept();
	}

private:
	boost::asio::io_service ios_;
	boost::asio::ip::tcp::acceptor acceptor_;
	connection_mgr conn_mgr_;

	std::vector<boost::thread*> io_thread_array_;


	UINT curr_client_id_;
	UINT io_thread_num_;
};

#endif //_SERVER_H