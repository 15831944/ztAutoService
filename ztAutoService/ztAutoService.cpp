#include "Log.h"
#include "ProcessMgr.h"
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
using namespace std;


//定义全局函数变量
void Init();
BOOL IsInstalled();
BOOL Install();
BOOL Uninstall();
void Task();
void WINAPI ServiceMain();
void WINAPI ServiceStrl(DWORD dwOpcode);

TCHAR szServiceName[] = _T("ztAutoService");
BOOL bInstall;
SERVICE_STATUS_HANDLE hServiceStatus;
SERVICE_STATUS status;
DWORD dwThreadID;
CProcessMgr pm;

/**
 * 处理控制台消息的回调函数
 * @param type  系统消息
 */
BOOL CtrlCHandler(DWORD type)
{
    if (type == CTRL_C_EVENT            //用户按下Ctrl+C,关闭程序。
        || CTRL_BREAK_EVENT == type     //用户按下CTRL+BREAK
        || CTRL_LOGOFF_EVENT == type    //用户退出时(注销)
        || CTRL_SHUTDOWN_EVENT == type) //当系统被关闭时
    {
        status.dwCurrentState = SERVICE_STOPPED;
        LogInfo(_T("stop all child process and exit"));
        return TRUE;
    } else if (CTRL_CLOSE_EVENT == type)//当试图关闭控制台程序
    {
        status.dwCurrentState = SERVICE_STOPPED;
        LogInfo(_T("stop all child process and exit"));
        Sleep(INFINITE);
        return TRUE;
    }
    return FALSE;
}

/**
 *  创建控制台
 */
void Control()
{
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "conin$", "r", stdin);//重定向输入流
    freopen_s(&stream, "conout$", "w", stdout);//重定向输出流
    //freopen_s(&stream, "conerr$", "w", stderr);//重定向输入流
}

int _tmain(int argc, _TCHAR* argv[])
{
    Init();

    dwThreadID = ::GetCurrentThreadId();

    if(argc == 1) {
        SERVICE_TABLE_ENTRY st[] =
        {
            { szServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
            { NULL, NULL }
        };
        if (!::StartServiceCtrlDispatcher(st))
        {
            LogError(_T("Register Service Main Function Error!"));
        }
    } else if (_tcsicmp(argv[1], _T("-i")) == 0) {
        Install();
    }
    else if (_tcsicmp(argv[1], _T("-u")) == 0)
    {
        Uninstall();
    }
    else if (_tcsicmp(argv[1], _T("-s")) == 0)
    {
        status.dwCurrentState = SERVICE_RUNNING;
        /** 设置控制台消息回调 */
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlCHandler, TRUE); 
        printf("use ctrl+c to exit");
        Task();
        PostThreadMessage(dwThreadID, WM_CLOSE, 0, 0);
    }

    return 0;
}


void Init()
{
	hServiceStatus = NULL;
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState = SERVICE_STOPPED;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	status.dwWin32ExitCode = 0;
	status.dwServiceSpecificExitCode = 0;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 0;

    LogInit(szServiceName);
}

void Task()
{
    pm.Start();

    //服务执行内容
    while(status.dwCurrentState == SERVICE_RUNNING)
    {
        pm.ProtectRun();
        Sleep(10000);
    }

    //程序结束
    pm.Stop();
}

void WINAPI ServiceMain()
{
	// Register the control request handler
	status.dwCurrentState = SERVICE_START_PENDING;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	//注册服务控制
	hServiceStatus = RegisterServiceCtrlHandler(szServiceName, ServiceStrl);
	if (hServiceStatus == NULL)
	{
		LogWarning(_T("Handler not installed"));
		return;
	}
	SetServiceStatus(hServiceStatus, &status);

	status.dwWin32ExitCode = S_OK;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 0;
	status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hServiceStatus, &status);

	//服务执行内容
	Task();


    //执行结束
	status.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(hServiceStatus, &status);
	LogInfo(_T("Service stopped"));
}

/**
 * 服务控制主函数，这里实现对服务的控制，当在服务管理器上停止或其它操作时，将会运行此处代码
 */
void WINAPI ServiceStrl(DWORD dwOpcode)
{
	switch (dwOpcode)
	{
	case SERVICE_CONTROL_STOP:
		status.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(hServiceStatus, &status);
		PostThreadMessage(dwThreadID, WM_CLOSE, 0, 0);
		break;
	case SERVICE_CONTROL_PAUSE:
		break;
	case SERVICE_CONTROL_CONTINUE:
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		break;
	default:
		LogError(_T("Bad service request"));
	}
}


BOOL IsInstalled()
{
	BOOL bResult = FALSE;

	//打开服务控制管理器
	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hSCM != NULL)
	{
		//打开服务
		SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_QUERY_CONFIG);
		if (hService != NULL)
		{
			bResult = TRUE;
			::CloseServiceHandle(hService);
		}
		::CloseServiceHandle(hSCM);
	}
	return bResult;
}

BOOL Install()
{
	if (IsInstalled())
		return TRUE;

	//打开服务控制管理器
	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCM == NULL)
	{
		LogError(_T("Couldn't open service manager"));
		return FALSE;
	}

	// Get the executable file path
	TCHAR szFilePath[MAX_PATH];
	::GetModuleFileName(NULL, szFilePath, MAX_PATH);

	//创建服务
	SC_HANDLE hService = ::CreateService(
		hSCM, szServiceName, szServiceName,
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
		szFilePath, NULL, NULL, _T(""), NULL, NULL);

	if (hService == NULL)
	{
		::CloseServiceHandle(hSCM);
		LogError(_T("Couldn't create service"));
		return FALSE;
	}

	//启动系统服务
	if (StartService(hService, 0, NULL) == false)
	{
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hSCM);
		return FALSE;
	}

	::CloseServiceHandle(hService);
	::CloseServiceHandle(hSCM);
	return TRUE;
}

BOOL Uninstall()
{
	if (!IsInstalled())
		return TRUE;

	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hSCM == NULL)
	{
		LogError(_T("Couldn't open service manager"));
		return FALSE;
	}

	SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_STOP | DELETE);

	if (hService == NULL)
	{
		::CloseServiceHandle(hSCM);
		LogError(_T("Couldn't open service"));
		return FALSE;
	}
	SERVICE_STATUS status;
	::ControlService(hService, SERVICE_CONTROL_STOP, &status);

	//删除服务
	BOOL bDelete = ::DeleteService(hService);
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hSCM);

	if (bDelete)
		return TRUE;

	LogError(_T("Service could not be deleted"));
	return FALSE;
}