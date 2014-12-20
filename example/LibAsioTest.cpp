// LibAsioTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "LibAsioTest.h"
#include "IAsioFacade.h"
#pragma  comment(lib,"LibAsio.lib")
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

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


		std::map<std::string,std::map<std::string,std::string> > cmdParam;
		cmdParam["listen"]["io_service_pool_size"] = "4";
		cmdParam["listen"]["speed_threads_for_a_io_service"] = "4";
		cmdParam["network"]["io_service_pool_size"] = "4";
		cmdParam["network"]["speed_threads_for_a_io_service"] = "4";
		cmdParam["timer"]["io_service_pool_size"] = "4";
		cmdParam["timer"]["speed_threads_for_a_io_service"] = "4";
		cmdParam["host"]["address"] = "127.0.0.1";
		cmdParam["host"]["port"] = "20000";



		GetFacadeInstance(ServerFacade).Configure(cmdParam);


	}

	return nRetCode;
}
