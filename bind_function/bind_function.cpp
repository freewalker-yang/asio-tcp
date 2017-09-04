// bind_function.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost\bind.hpp>
#include <boost\function.hpp>
#include <iostream>

#include <boost\asio.hpp>
#include <boost\date_time\posix_time\posix_time.hpp>

#include <boost\thread.hpp>

using namespace std;

class button
{
public:
	boost::function<void()> m_handler;
};

class Player
{
public:
	void Play() { cout << "play" << endl; }
	void Stop() { cout << "stop" << endl; }
};

int Add_two(int a, int b)
{
	return a + b;
}

int Add_Three(int a, int b, int c)
{
	return Add_two(Add_two(a, b), c);
}

class foo
{
public:
	int test(int a, int b)
	{
		return a*b;
	}
};

void TestBind()
{
	Player play;
	button play_btn, stop_btn;

	play_btn.m_handler = boost::bind(&Player::Play, &play);
	stop_btn.m_handler = boost::bind(&Player::Stop, &play);

	play_btn.m_handler();
	stop_btn.m_handler();

	int nResult = boost::bind(Add_two, _1, 3)(2);

	nResult = boost::bind(Add_Three, _1, _2, 3)(1, 2);

	foo obj;
	boost::function<void(int, int)> func = boost::bind(&foo::test, obj, _1, _2);

	func(1, nResult);
	boost::bind(&foo::test, obj, _1, 1)(nResult);
}

void Timer_sync()
{
	boost::asio::io_service ios;

	std::cout << "Before Timer!" << std::endl;

	boost::asio::deadline_timer timer(ios, boost::posix_time::seconds(5));

	timer.wait();

	std::cout << "Hello World After Timer(5 seconds)!" << std::endl;
}

void timer_handler_asycwait(boost::system::error_code error)
{
	std::cout << "Hello World After Timer(5 seconds)!" << std::endl;
}

void timer_asyc()
{
	boost::asio::io_service ios;

	std::cout << "Before Timer!" << std::endl;

	boost::asio::deadline_timer timer(ios, boost::posix_time::seconds(5));

	timer.async_wait(&timer_handler_asycwait);

	std::cout << "This is in asyc mode!" << std::endl;

	ios.run();
}


using  namespace boost::asio;

io_service ios_asio;

boost::mutex mutex_;

void function_for_post(int i)
{
	boost::unique_lock<boost::mutex> lock(mutex_);

	std::cout << "thread id(" << ::GetCurrentThreadId() << "):";
	std::cout << "function called " << i << std::endl;
}

void ioservice_post()
{
	boost::asio::strand strand1(ios_asio), strand2(ios_asio);

	for (int i = 0; i < 10; i++)
	{
		if (i < 3)
			ios_asio.post(strand1.wrap(boost::bind(function_for_post, i)));
		else if (i < 6)
			ios_asio.post(strand2.wrap(boost::bind(function_for_post, i)));
		else
			ios_asio.post(boost::bind(function_for_post, i));

	}

	boost::thread_group thread_grp;

	for (int i = 0; i < 3; i++)
	{
		thread_grp.create_thread(boost::bind(&io_service::run, &ios_asio));

		
	}

	boost::this_thread::sleep(boost::posix_time::microseconds(500));

	thread_grp.join_all();


}

void ioservice_dispatch_and_post()
{
	
	for (int i = 0; i < 5; i++)
	{
		
		ios_asio.dispatch(boost::bind(function_for_post, 2*i));
		
		ios_asio.post(boost::bind(function_for_post, 2*i+1));

	}
	
}

//int _tmain(int argc, _TCHAR* argv[])
//{
//	ios_asio.post(ioservice_dispatch_and_post);
//	ios_asio.run();
//
//	system("pause");
//
//	return 0;
//}
//
