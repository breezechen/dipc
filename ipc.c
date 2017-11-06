#include <windows.h>
#include "ipc.h"

BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	HANDLE hToken;
	BOOL ret = FALSE;

	do
	{
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
			break;
		}

		if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
			break;
		}

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		if (bEnablePrivilege)
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		else
			tp.Privileges[0].Attributes = 0;

		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
			break;
		}

		if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
			break;
		}

		ret = TRUE;
	} while (0);

	return ret;
}

IpcServer* ServerCreate(LPCTSTR serverName)
{
    IpcServer *ret = NULL;
    HANDLE reqEvent = NULL;
    HANDLE repEvent = NULL;
	HANDLE readyEvent = NULL;
	HANDLE doneEvent = NULL;
	HANDLE mapFile = NULL; 
	BYTE *buf = NULL;
	TCHAR memName[MAX_NAME_LEN + 5];
	TCHAR reqeName[MAX_NAME_LEN + 5]; 
	TCHAR repeName[MAX_NAME_LEN + 5];
	TCHAR readyeName[MAX_NAME_LEN + 5];
	TCHAR doneeName[MAX_NAME_LEN + 5];
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

    do 
    {
		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

		sa.bInheritHandle = FALSE;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = &sd;

		if (serverName == NULL || serverName[0] == '\0')
			serverName = TEXT("simple_icp_default");

		if (lstrlen(serverName) > MAX_NAME_LEN)
			break;

		if (_strnicmp(serverName, "Global\\", strlen("Global\\")) == 0 && 
			!SetPrivilege("SeCreateGlobalPrivilege", TRUE)) {
			break;
		}

		wsprintf(memName, TEXT("%s_mem"), serverName);
		wsprintf(reqeName, TEXT("%s_req"), serverName);
		wsprintf(repeName, TEXT("%s_rep"), serverName);
		wsprintf(readyeName, TEXT("%s_red"), serverName);
		wsprintf(doneeName, TEXT("%s_don"), serverName);

		doneEvent = CreateEvent(&sa, FALSE, FALSE, doneeName);
		if (NULL == doneEvent)
			break;
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			break;

        repEvent = CreateEvent(&sa, FALSE, FALSE, repeName);
        if (NULL == repEvent)
            break;
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            break;

        reqEvent = CreateEvent(&sa, FALSE, FALSE, reqeName);
        if (NULL == reqEvent)
            break;
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            break;

		readyEvent = CreateEvent(&sa, FALSE, FALSE, readyeName);
		if (NULL == readyEvent)
			break;
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			break;

        mapFile = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, MEMMAP_SIZE, memName);
        if (NULL == mapFile) 
            break;
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            break;

        buf = (BYTE *)MapViewOfFile(mapFile, FILE_MAP_ALL_ACCESS, 0, 0, MEMMAP_SIZE);
        if (NULL == buf)
            break;

        ret = (IpcServer *)malloc(sizeof(IpcServer));
        if (NULL == ret)
            break;

        ret->mapFile = mapFile;
        ret->buf = buf;
        ret->repEvent = repEvent;
        ret->reqEvent = reqEvent;
		ret->readyEvent = readyEvent;
		ret->doneEvent = doneEvent;
		ret->timeout = 1000;
    } while (0);

    if (NULL == ret) {
        if (NULL != buf)
            UnmapViewOfFile(buf);

        if (NULL != mapFile)
            CloseHandle(mapFile);

		if (NULL != readyEvent)
			CloseHandle(readyEvent);

        if (NULL != reqEvent)
            CloseHandle(reqEvent);

        if (NULL != repEvent)
            CloseHandle(repEvent);

		if (NULL != doneEvent)
			CloseHandle(doneEvent);
    }

    return ret;
}

BOOL ServerWaitClientDone(IpcServer* server)
{
	return WAIT_OBJECT_0 == WaitForSingleObject(server->doneEvent, server->timeout);
}

BOOL ServerWaitForRequst(IpcServer* server)
{
    return WAIT_OBJECT_0 == WaitForSingleObject(server->reqEvent, server->timeout);
}

VOID ServerReplied(IpcServer* server)
{
    SetEvent(server->repEvent);
}

VOID ServerReady(IpcServer* server)
{
	SetEvent(server->readyEvent);
}

VOID ServerClose(IpcServer* server)
{
	CloseHandle(server->readyEvent);
    CloseHandle(server->repEvent);
    CloseHandle(server->reqEvent);
	CloseHandle(server->doneEvent);
    UnmapViewOfFile(server->buf);
    CloseHandle(server->mapFile);
    free(server);
}

CommPacket* ClientRequest(ULONG cmd, const BYTE* data, SIZE_T size, LPCTSTR serverName, DWORD timeout)
{
    HANDLE mapFile = NULL; 
    BYTE *buf = NULL;
    HANDLE reqEvent = NULL;
    HANDLE repEvent = NULL;
	HANDLE readyEvent = NULL;
	HANDLE doneEvent = NULL;
    CommPacket* packet;
    BYTE* ret = NULL;
	TCHAR memName[MAX_NAME_LEN + 5];
	TCHAR reqeName[MAX_NAME_LEN + 5]; 
	TCHAR repeName[MAX_NAME_LEN + 5];
	TCHAR readyeName[MAX_NAME_LEN + 5];
	TCHAR doneeName[MAX_NAME_LEN + 5];

    do 
    {
		if (serverName == NULL || serverName[0] == '\0')
			serverName = TEXT("simple_icp_default");

		if (lstrlen(serverName) > MAX_NAME_LEN)
			break;

		wsprintf(memName, TEXT("%s_mem"), serverName);
		wsprintf(reqeName, TEXT("%s_req"), serverName);
		wsprintf(repeName, TEXT("%s_rep"), serverName);
		wsprintf(readyeName, TEXT("%s_red"), serverName);
		wsprintf(doneeName, TEXT("%s_don"), serverName);

        mapFile = OpenFileMapping(FILE_MAP_WRITE | FILE_MAP_READ, FALSE, memName);
		if (NULL == mapFile) {
			break;
		}
            
        buf = (BYTE *)MapViewOfFile(mapFile, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, MEMMAP_SIZE);
        if (NULL == buf)
            break;

		doneEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, doneeName);
		if (NULL == doneEvent)
			break;

		readyEvent = OpenEvent(SYNCHRONIZE, FALSE, readyeName);
		if (NULL == readyEvent)
			break;

        reqEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, reqeName);
        if (NULL == reqEvent)
            break;

        repEvent = OpenEvent(SYNCHRONIZE, FALSE, repeName);
        if (NULL == repEvent)
            break;

		if (WAIT_OBJECT_0 != WaitForSingleObject(readyEvent, timeout))
			break;

		packet = (CommPacket*)buf;
		packet->size = size + sizeof(CommPacket);
		packet->cmd = cmd;
		if (data && size)
			memcpy(packet->data, data, size);
        SetEvent(reqEvent);
        if (WAIT_OBJECT_0 != WaitForSingleObject(repEvent, timeout))
            break;

        if (packet->size > MEMMAP_SIZE || packet->size == 0)
            break;

        ret = (BYTE*)malloc(packet->size);
        if (NULL == ret)
            break;

        memcpy(ret, buf, packet->size);

		SetEvent(doneEvent);
    } while (0);

	if (doneEvent != NULL)
		CloseHandle(doneEvent);
	if (readyEvent != NULL)
		CloseHandle(readyEvent);
    if (repEvent != NULL)
        CloseHandle(repEvent);
    if (reqEvent != NULL)
        CloseHandle(reqEvent);
    if (buf != NULL)
        UnmapViewOfFile(buf);
    if (mapFile != NULL)
        CloseHandle(mapFile);

    return (CommPacket* )ret;
}

VOID FreePacket(CommPacket *packet)
{
    free(packet);
}

