#ifndef _SERVER_H
#define _SERVER_H

class server_tcp
{
public:
	server_tcp(boost::asio::io_service& ios, int port)
		:ios_(ios)
		, acceptor_(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
		, curr_client_id_(1)
	{
		std::cout << "start listen:" << std::endl;
		start_accept();
	}

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
			std::cout << "new client accepted:" << std::endl;
			new_session->start(curr_client_id_++);
		}
		else
		{
			std::cout << "new client accepted error" << std::endl;
			delete new_session.get();
		}

		start_accept();
	}

	void write_to_all(conn_msg* msg)
	{
		conn_mgr_.write_to_all(msg);
	}

private:
	boost::asio::io_service& ios_;

	boost::asio::ip::tcp::acceptor acceptor_;

	connection_mgr conn_mgr_;

	UINT curr_client_id_;
};

#endif //_SERVER_H