// LibAsio.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "LibAsio.h"
#include "Singleton2.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#if 1
extern "C" BOOL WINAPI DllMain(
							    HINSTANCE hInstance, 
								DWORD dwReason, 
								LPVOID lpReserved)
{
	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			break;
		}

	case DLL_THREAD_ATTACH:
		{
			break;
		}

	case DLL_THREAD_DETACH:
		{
			break;
		}

	case DLL_PROCESS_DETACH:
		{
			SingletonBase::SetStopAll();
			SingletonBase::Release();
			break;
		}
	}

	return TRUE;
}
#endif

