// LibAsioTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "LibAsioTest.h"
#include "IAsioFacade.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <unordered_map>
//#include <Winsock2.h>
#include "log.h"

#ifdef _DEBUG
#pragma  comment(lib,"LibAsio_D.lib")
#else
#pragma  comment(lib,"LibAsio_R.lib")
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

PANTHEIOS_EXTERN_C PAN_CHAR_T const PANTHEIOS_FE_PROCESS_IDENTITY[] = PANTHEIOS_LITERAL_STRING("Lib.Asio.Test");

using namespace std;
// 唯一的应用程序对象
class SockHandler:public ISockHandler
{
private:
	boost::detail::lightweight_mutex m_read_lock;
	std::tr1::unordered_map<long,long>	 m_seq_read_validor;

	boost::detail::lightweight_mutex m_write_lock;
	std::tr1::unordered_map<long,long>	 m_seq_write_validor;

public:
	


	virtual void on_connect(long connID,const char* errMsg)
	{
		LOG(informational,"Connected %ld",connID);
		//char buffer[512];
		//int nRecvSize = Recv(connID,512,buffer);

	};

	virtual void on_accept(long connID, const char* errMsg)
	{
		LOG(informational,"Accepted %ld",connID);

	};


	void on_read(long connID,const char* errMsg, std::size_t bytes_transferred, buffer_ptr buf,long read_seq)
	{
		LOG3(10*1000*1000,informational,"Connection %ld received %d seq %ld",connID, buf->size(), read_seq);
		{
			boost::detail::lightweight_mutex::scoped_lock guard(m_read_lock);
			if(m_seq_read_validor.count(connID))
			{
				long prev_seq = m_seq_read_validor[connID];
				ASSERT(prev_seq < read_seq);
				//ASSERT(prev_seq+1 == read_seq);
			}
			m_seq_read_validor[connID] = read_seq;

		}
		if(read_seq < 40000)
		{
			buffer_ptr tmpBuffer = boost::make_shared<buffer>(boost::cref(*buf));
			buf->clear();
			buf.reset();
			Send(connID,tmpBuffer);
		}
		else
			Close(connID);
	};

	void on_write(long connID, const char* errMsg, std::size_t bytes_transferred,	buffer_ptr bufferLeft,long write_seq)
	{
		{
			boost::detail::lightweight_mutex::scoped_lock guard(m_write_lock);
			if(m_seq_write_validor.count(connID))
			{
				long prev_seq = m_seq_write_validor[connID];
				ASSERT(prev_seq < write_seq);
				//ASSERT(prev_seq+1 == write_seq);
			}
			m_seq_write_validor[connID] = write_seq;

		}

	};



	void on_warning(long connID,const char *warningMsg)
	{
		LOG3(10*1000*1000,warning,"Connection %ld warning %s",connID, warningMsg);
	}

	void on_error(long connID, const char* errMsg)
	{
		//std::string errMsg = e.message();
		LOG(error,"Connection %ld Error %s",connID, errMsg);
	};

	void on_close(long connID, const char* errMsg,const char* msgReason)
	{
		//std::string errMsg = e.message();

		LOG(informational,"Connection %ld Close %s Reason %s",connID, errMsg,msgReason);

	};

	void on_cancel(long connID, buffer_ptr cancel_buffer,long write_seq)
	{
		cout<<"Socket "<<connID<<" Cancle Seq "<<write_seq<<endl;
		LOG(informational,"Connection %ld Cancle Seq %ld",connID, write_seq);

	};

	void on_timer(long timerID,long connID)
	{
		Send(connID,"Test Hello World  111111111111111111111111111111111111111111111111111111111111111111111111111\n"
			"2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222"
			"333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"
			"444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444");
	
		Send(connID,"Test Hello World  111111111111111111111111111111111111111111111111111111111111111111111111111\n"
			"2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222"
			"333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"
			"444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444");

		Send(connID,"Test Hello World  111111111111111111111111111111111111111111111111111111111111111111111111111\n"
			"2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222"
			"333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"
			"444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444");
	}

	void on_timer2(long timerID,long connID)
	{
		boost::system::error_code ec;
		char buffer[4096];
		long read_seq;
		int nRecvSize = Recv(connID,sizeof(buffer),buffer,read_seq,ec);


		if(nRecvSize > 0)
		{
			{
				boost::detail::lightweight_mutex::scoped_lock guard(m_read_lock);
				if(m_seq_read_validor.count(connID))
				{
					long prev_seq = m_seq_read_validor[connID];
					ASSERT(prev_seq < read_seq);
					//ASSERT(prev_seq+1 == read_seq);
				}
				m_seq_read_validor[connID] = read_seq;

			}


			LOG2(50,notice,">>>>>>>>>>Connection %ld 同步读 %d<<<<<<<<<<<",connID, nRecvSize);
			Send(connID,buffer,nRecvSize);
		}
		else
		{
			Schedule_stop(timerID);
			LOG(error,"同步读错误");
		}
	}


	void on_timer3(long timerID,long connID)
	{

		static bool bSync = false;
		bSync ^= true;
		SetSyncRead(connID,bSync);
	}



};

CWinApp theApp;



int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// 初始化 MFC 并在失败时显示错误
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: 更改错误代码以符合您的需要
		_tprintf(_T("错误: MFC 初始化失败\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: 在此处为应用程序的行为编写代码。


		const int MaxSockets = 2;//10000;

		cout<<"Start Service"<<endl;

		std::map<std::string,std::map<std::string,std::string> > cmdParam;
		cmdParam["listen"]["io_service_pool_size"] = "1";
		cmdParam["listen"]["speed_threads_for_a_io_service"] = "6";
		cmdParam["network"]["io_service_pool_size"] = "128";
		cmdParam["network"]["speed_threads_for_a_io_service"] = "6";
		cmdParam["timer"]["io_service_pool_size"] = "64";
		cmdParam["timer"]["speed_threads_for_a_io_service"] = "4";
		cmdParam["host"]["address"] = "0.0.0.0";
		cmdParam["host"]["port"] = "10005";
		cmdParam["host"]["min_read_buffer_size"] = "4096";
		cmdParam["logfile"]["filename"] = ".\\LibAsio-%D-%T.log";

		AsioFacadeGetInstance(ServerFacade).Configure(cmdParam);
		AsioFacadeGetInstance(ServerFacade).Run();

		AsioFacadeGetInstance(ClientFacade).Configure(cmdParam);
		AsioFacadeGetInstance(ClientFacade).Run();


		cmdParam["logfile"]["filename"] = ".\\LibAsioTest-%D-%T.log";
		Singleton2<logger,3000>::instance().Configure(cmdParam,logger::E_LOG_FUNC);

		boost::shared_ptr<SockHandler> clientHandler(new SockHandler());
		AsioFacadeGetInstance(ClientFacade).AttachHandler(clientHandler.get());
		boost::shared_ptr<SockHandler> serverHandler(new SockHandler());
		AsioFacadeGetInstance(ServerFacade).AttachHandler(serverHandler.get());

		std::vector<long> socketIDS,timerIDS;
		for(int i=0;i<MaxSockets;i++)
		{

			long connID = AsioFacadeGetInstance(ClientFacade).Connect("127.0.0.1","10005");
			socketIDS.push_back(connID);

			long timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer2,clientHandler,_1,connID),boost::posix_time::seconds(15),-1);
			timerIDS.push_back(timerID);

			timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer3,clientHandler,_1,connID),boost::posix_time::seconds(45),-1);
			timerIDS.push_back(timerID);

			timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer2,clientHandler,_1,connID),boost::posix_time::seconds(25),-1);
			timerIDS.push_back(timerID);


			//connID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer,clientHandler,connID),boost::posix_time::seconds(1),-1);
			timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer,clientHandler,_1,connID),boost::posix_time::seconds(1),2);
			timerIDS.push_back(timerID);

			timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer,clientHandler,_1,connID),boost::posix_time::seconds(1),2);
			timerIDS.push_back(timerID);

			timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer,clientHandler,_1,connID),boost::posix_time::seconds(1),2);
			timerIDS.push_back(timerID);




		}

		getchar();

		for(int i=0;i<MaxSockets;i++)
		{

			AsioFacadeGetInstance(ClientFacade).Close(socketIDS[i]);

		}

		getchar();

		for(int i=0;i<timerIDS.size();i++)
		{

			AsioFacadeGetInstance(ClientFacade).Schedule_stop(timerIDS[i]);

		}


		getchar();





		AsioFacadeGetInstance(ServerFacade).DetachHandler(serverHandler.get());


		AsioFacadeGetInstance(ClientFacade).DetachHandler(clientHandler.get());

		//SingletonBase::SetStopAll();
		AsioFacadeCleanUp();
		

	}

	return nRetCode;
}
