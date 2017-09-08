#ifndef _SERVER_H
#define _SERVER_H

//#include <thread>
#include <boost/bind.hpp>


class server_tcp
{
public:

	enum { max_io_thread = 10 };

	server_tcp(boost::asio::io_service& ios, unsigned int io_thread_num = boost::thread::hardware_concurrency())
		:ios_(ios)
		, acceptor_(ios)
		, curr_client_id_(1)
		, io_thread_num_(io_thread_num)
	{
		
		assert(io_thread_num_ <= max_io_thread);

		

		
	}

	void start(int port)
	{
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), port);

		acceptor_.open(ep.protocol());
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor_.bind(ep);
		acceptor_.listen();

		std::cout << "start listen:" << std::endl;

		start_accept();

#if IO_THREAD_IN_CLASS == 1
		/*io_thread_array_.resize(io_thread_num_);

		for (UINT i = 0; i < io_thread_num_; i++)
		{
			io_thread_array_[i] = new boost::thread(boost::bind(&(boost::asio::io_service::run), &ios_));
		}*/

#endif //IO_THREAD_IN_CLASS
		
		
	}

	void stop()
	{
		acceptor_.cancel();

		ios_.stop();

#if IO_THREAD_IN_CLASS == 1
		/*for (UINT i = 0; i < io_thread_num_; i++)
		{
			io_thread_array_[i]->join();
			delete io_thread_array_[i];
			io_thread_array_[i] = NULL;
		}

		io_thread_array_.clear();*/
		

#else

#endif //IO_THREAD_IN_CLASS
	}

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
			std::cout << "new client accepted error" << std::endl;
			//delete new_session.get();
			conn_mgr_.leave(new_session);
		}

		start_accept();
	}

	void bind_io_thread_func()
	{

	}

private:
	boost::asio::io_service& ios_;
	boost::asio::ip::tcp::acceptor acceptor_;
	connection_mgr conn_mgr_;

	std::vector<boost::thread*> io_thread_array_;


	UINT curr_client_id_;
	UINT io_thread_num_;
};

#endif //_SERVER_H