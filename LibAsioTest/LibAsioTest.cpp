// LibAsioTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "LibAsioTest.h"
#include "IAsioFacade.h"
#include <boost/bind.hpp>
#include <boost/local_function.hpp>
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

#pragma pack(push,1) //请务必一字节对齐

class SockHandler:public ISockHandler
{
private:
	boost::detail::lightweight_mutex m_read_lock;
	std::tr1::unordered_map<long,long>	 m_seq_read_validor;

	boost::detail::lightweight_mutex m_write_lock;
	std::tr1::unordered_map<long,long>	 m_seq_write_validor;

public:
	
	virtual ~SockHandler()
	{};

	virtual void on_connect(long connID,const char* errMsg)
	{
		SocketState sockState;

		if(State(connID,sockState))
		{
			if(sockState.currentConnectTimeWithms > 30*1000)
				LOG2(100,warning,"连接花了太多时间 [%ld] milliseconds", sockState.currentConnectTimeWithms);

			//long timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer,this,_1,connID),boost::posix_time::seconds(10),2);

			//timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer,this,_1,connID),boost::posix_time::seconds(15),2);

			long timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_timer,this,_1,connID),boost::posix_time::seconds(20),6);

		}


	};

	virtual void on_accept(long connID, const char* errMsg)
	{
		LOG2(100,informational,"Accepted %ld",connID);

	};

	void on_defer_send( long timerID,long connID,buffer_ptr buf) 
	{

		Send(connID,buf);

	};



	void on_read(long connID,const char* errMsg, std::size_t bytes_transferred, buffer_ptr buf,long read_seq)
	{
		LOG3(60*1000*1000,informational,"Connection %ld received %d seq %ld",connID, buf->size(), read_seq);
		{
			SocketState sockState;

			if(State(connID,sockState))
			{
				if(sockState.currentReadTimeWithms > 30*1000)
					LOG2(100,warning,"异步读数据花了太多时间 [%ld] milliseconds", sockState.currentReadTimeWithms);
			}

			boost::detail::lightweight_mutex::scoped_lock guard(m_read_lock);
			if(m_seq_read_validor.count(connID))
			{
				long prev_seq = m_seq_read_validor[connID];
				ASSERT(prev_seq < read_seq);
				//ASSERT(prev_seq+1 == read_seq);
			}
			m_seq_read_validor[connID] = read_seq;
		}

		if(read_seq < 5000)
		{
			buffer_ptr tmpBuffer = boost::make_shared<buffer>(boost::cref(*buf));
			buf->clear();
			buf.reset();
			//SwitchToThread();

			long timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_defer_send,this,_1,connID,tmpBuffer),
																				 boost::posix_time::milliseconds((read_seq*20)%2000+100),1);
			//Send(connID,tmpBuffer);
		}
		else
		{
			Close(connID);
		}
	};

	void on_write(long connID, const char* errMsg, std::size_t bytes_transferred,	buffer_ptr bufferLeft,long write_seq)
	{
		SocketState sockState;

		if(State(connID,sockState))
		{
			if(sockState.currentWriteTimeWithms > 30*1000)
				LOG2(100,warning,"写数据花了太多时间 [%ld] milliseconds", sockState.currentWriteTimeWithms);
		}

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

		{
			boost::detail::lightweight_mutex::scoped_lock guard(m_read_lock);
			m_seq_read_validor.erase(connID);
		}
		{
			boost::detail::lightweight_mutex::scoped_lock guard(m_write_lock);
			m_seq_write_validor.erase(connID);
		}

	};

	void on_cancel(long connID, buffer_ptr cancel_buffer,long write_seq)
	{
		cout<<"Socket "<<connID<<" Cancle Seq "<<write_seq<<endl;
		LOG(informational,"Connection %ld Cancle Seq %ld",connID, write_seq);

	};

	void on_timer(long timerID,long connID)
	{
		for(int i = 0;i<3;i++)
		{
			Send(connID,"Test Hello World  111111111111111111111111111111111111111111111111111111111111111111111111111\n"
				"2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222"
				"333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"
				"444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444");
		}
	
	}

	void on_SyncRead(long timerID,long connID)
	{

		bool BOOST_LOCAL_FUNCTION( long connID, bind this_) 
		{
			boost::detail::lightweight_mutex::scoped_lock guard(this_->m_read_lock);
			return this_->m_seq_read_validor.count(connID);
		} BOOST_LOCAL_FUNCTION_NAME(LocalValidConnID)

		if(LocalValidConnID(connID))
		{
			char buffer[4096];
			long read_seq;
			boost::system::error_code ec;
			int nRecvSize = Recv(connID,sizeof(buffer),buffer,read_seq,ec);


			if(nRecvSize >= 0)
			{

				if(nRecvSize > 0)
				{
					LOG2(100,notice,">>>>>>>>>>Connection %ld 同步读 %d<<<<<<<<<<<",connID, nRecvSize);

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

					SocketState sockState;

					if(State(connID,sockState))
					{
						if(sockState.currentReadTimeWithms > 30*1000)
							LOG2(100,warning,"同步读数据花了太多时间 [%ld] milliseconds", sockState.currentReadTimeWithms);
					}



					Send(connID,buffer,nRecvSize);
				}
			}
			else
			{
				//Schedule_stop(timerID);
				LOG2(100,error,"同步读错误[%s]",ec.message().c_str());
			}

		}
		else
			Schedule_stop(timerID);

	}


	void on_SetSyncRead(long timerID,long connID)
	{

		bool BOOST_LOCAL_FUNCTION( long connID, bind this_) 
		{
			boost::detail::lightweight_mutex::scoped_lock guard(this_->m_read_lock);
			return this_->m_seq_read_validor.count(connID);
		} BOOST_LOCAL_FUNCTION_NAME(LocalValidConnID)

		if(LocalValidConnID(connID))
		{
			bool bSync = false;

			SocketState sockState;

			if(State(connID,sockState))
			{
				if(sockState.pendingSyncReadNum)
					bSync = false;
				else
					bSync = true;
			}

			SetSyncRead(connID,bSync);
		}
		else
			Schedule_stop(timerID);

	}

	void on_monitor_module(long timerID)
	{
		ModState modState;
		if(State(modState))
		{
			LOG(notice,"TotalSocket[%ld] TotalTimer[%ld] TotalSocketRead [%lld] kb TotalSocketWrite [%lld] kb",
						modState.totalSockNum,modState.totalTimerNum,
						modState.totalFinishedReadBytes/1024,
						modState.totalFinishedWriteBytes/1024);
		}

	}


	void on_monitor_sock(long timerID,long connID)
	{
		bool BOOST_LOCAL_FUNCTION( long connID, bind this_) 
		{
			boost::detail::lightweight_mutex::scoped_lock guard(this_->m_read_lock);
			return this_->m_seq_read_validor.count(connID);
		} BOOST_LOCAL_FUNCTION_NAME(LocalValidConnID)

		if(LocalValidConnID(connID))
		{
			SocketState sockState;

			if(State(connID,sockState))
			{

			}
		}
		else
			Schedule_stop(timerID);

	}


};

typedef boost::shared_ptr<SockHandler> SockHandlerPtr;



#pragma pack(pop)

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


		const int MaxSockets = 10000;

		cout<<"Start Service"<<endl;

		std::map<std::string,std::map<std::string,std::string> > cmdParam;
		cmdParam["listen"]["io_service_pool_size"] = "1";
		cmdParam["listen"]["speed_threads_for_a_io_service"] = "16";
		cmdParam["network"]["io_service_pool_size"] = "128";
		cmdParam["network"]["speed_threads_for_a_io_service"] = "8";
		cmdParam["timer"]["io_service_pool_size"] = "128";
		cmdParam["timer"]["speed_threads_for_a_io_service"] = "4";
		cmdParam["host"]["address"] = "0.0.0.0";
		cmdParam["host"]["port"] = "10005";
		cmdParam["host"]["min_read_buffer_size"] = "1024";
		cmdParam["logfile"]["filename"] = ".\\LibAsio-%D-%T.log";

		AsioFacadeGetInstance(ServerFacade).Configure(cmdParam);
		AsioFacadeGetInstance(ServerFacade).Run();

		AsioFacadeGetInstance(ClientFacade).Configure(cmdParam);
		AsioFacadeGetInstance(ClientFacade).Run();


		cmdParam["logfile"]["filename"] = ".\\LibAsioTest-%D-%T.log";
		Singleton2<logger,3000>::instance().Configure(cmdParam,logger::E_LOG_FUNC);

		SockHandlerPtr serverHandler(new SockHandler());
		AsioFacadeGetInstance(ServerFacade).AttachHandler(serverHandler);

		SockHandlerPtr clientHandler(new SockHandler());
		AsioFacadeGetInstance(ClientFacade).AttachHandler(clientHandler);


		std::vector<long> socketIDS,timerIDS;
		
		{
			int len = sizeof(SockHandler);
			timer_callback_func func = boost::bind(&SockHandler::on_monitor_module,clientHandler,_1);
			//long timerID = AsioFacadeGetInstance(ClientFacade).Schedule_at_once(func);
			long timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(func,boost::posix_time::seconds(60),-1);

			
			//long timerID = AsioFacadeGetInstance(ClientFacade).Schedule_at_once(boost::bind(&SockHandler::on_monitor_module,clientHandler,_1));
			timerIDS.push_back(timerID);

		}

		
		long timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_monitor_module,clientHandler,_1),boost::posix_time::seconds(15),-1);
		timerIDS.push_back(timerID);

		for(int i=0;i<MaxSockets;i++)
		{

			long connID = AsioFacadeGetInstance(ClientFacade).Connect("127.0.0.1","10005");
			socketIDS.push_back(connID);

			//timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_SyncRead,clientHandler,_1,connID),boost::posix_time::seconds(240),-1);
			//timerIDS.push_back(timerID);

			timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_SyncRead,clientHandler,_1,connID),boost::posix_time::seconds(480),-1);
			timerIDS.push_back(timerID);

			//timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_SetSyncRead,clientHandler,_1,connID),boost::posix_time::seconds(600),-1);
			//timerIDS.push_back(timerID);

			timerID = AsioFacadeGetInstance(ClientFacade).Schedule_from_now(boost::bind(&SockHandler::on_monitor_sock,clientHandler,_1,connID),boost::posix_time::seconds(120),-1);
			timerIDS.push_back(timerID);

			if((i+1) % 25 == 0)
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
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





		AsioFacadeGetInstance(ServerFacade).DetachHandler(serverHandler);


		AsioFacadeGetInstance(ClientFacade).DetachHandler(clientHandler);

		//SingletonBase::SetStopAll();
		AsioFacadeCleanUp();
		

	}

	return nRetCode;
}
