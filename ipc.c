#include <windows.h>
#include "ipc.h"

IpcServer* ServerCreate(LPCTSTR serverName)
{
    IpcServer *ret = NULL;
    HANDLE reqEvent = NULL;
    HANDLE repEvent = NULL;
	HANDLE mapFile = NULL; 
	BYTE *buf = NULL;
	TCHAR memName[MAX_NAME_LEN + 5];
	TCHAR reqeName[MAX_NAME_LEN + 5]; 
	TCHAR repeName[MAX_NAME_LEN + 5];

    do 
    {
		if (serverName == NULL)
			serverName = TEXT("simple_icp_default");

		if (lstrlen(serverName) > MAX_NAME_LEN)
			break;

		wsprintf(memName, TEXT("%s_mem"), serverName);
		wsprintf(reqeName, TEXT("%s_req"), serverName);
		wsprintf(repeName, TEXT("%s_rep"), serverName);

        repEvent = CreateEvent(NULL, FALSE, FALSE, repeName);
        if (NULL == repEvent)
            break;
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            break;

        reqEvent = CreateEvent(NULL, FALSE, FALSE, reqeName);
        if (NULL == reqEvent)
            break;
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            break;

        mapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MEMMAP_SIZE, memName);
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
		ret->timeout = 1000;
    } while (0);

    if (NULL == ret) {
        if (NULL != buf)
            UnmapViewOfFile(buf);

        if (NULL != mapFile)
            CloseHandle(mapFile);

        if (NULL != reqEvent)
            CloseHandle(reqEvent);

        if (NULL != repEvent)
            CloseHandle(repEvent);
    }

    return ret;
}

BOOL ServerWaitForRequst(IpcServer* server)
{
    return WAIT_OBJECT_0 == WaitForSingleObject(server->reqEvent, server->timeout);
}

VOID ServerReplied(IpcServer* server)
{
    SetEvent(server->repEvent);
}

VOID ServerClose(IpcServer* server)
{
    CloseHandle(server->repEvent);
    CloseHandle(server->reqEvent);
    UnmapViewOfFile(server->buf);
    CloseHandle(server->mapFile);
    free(server);
}

BYTE* ClientRequest(ULONG cmd, const BYTE* data, SIZE_T size, LPCTSTR serverName, DWORD timeout)
{
    HANDLE mapFile = NULL; 
    BYTE *buf = NULL;
    HANDLE reqEvent = NULL;
    HANDLE repEvent = NULL;
    CommPacket* packet;
    BYTE* ret = NULL;
	TCHAR memName[MAX_NAME_LEN + 5];
	TCHAR reqeName[MAX_NAME_LEN + 5]; 
	TCHAR repeName[MAX_NAME_LEN + 5];
	DWORD dw;

    do 
    {
		if (serverName == NULL)
			serverName = TEXT("simple_icp_default");

		if (lstrlen(serverName) > MAX_NAME_LEN)
			break;

		wsprintf(memName, TEXT("%s_mem"), serverName);
		wsprintf(reqeName, TEXT("%s_req"), serverName);
		wsprintf(repeName, TEXT("%s_rep"), serverName);

        mapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memName);
        if (NULL == mapFile)
            break;

        buf = (BYTE *)MapViewOfFile(mapFile, FILE_MAP_ALL_ACCESS, 0, 0, MEMMAP_SIZE);
        if (NULL == buf)
            break;

        reqEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, reqeName);
        if (NULL == reqEvent)
            break;

        repEvent = OpenEvent(SYNCHRONIZE, FALSE, repeName);
        if (NULL == repEvent)
            break;

		packet = (CommPacket*)buf;
		packet->size = size + sizeof(CommPacket);
		packet->cmd = cmd;
		if (data && size)
			memcpy(packet->data, data, size);

        SetEvent(reqEvent);
        if (WAIT_OBJECT_0 != WaitForSingleObject(repEvent, timeout))
            break;

        if (packet->size == 0)
            break;

        ret = (BYTE*)malloc(packet->size);
        if (NULL == ret)
            break;

        memcpy(ret, buf, packet->size);

    } while (0);

    if (repEvent != NULL)
        CloseHandle(repEvent);
    if (reqEvent != NULL)
        CloseHandle(reqEvent);
    if (buf != NULL)
        UnmapViewOfFile(buf);
    if (mapFile != NULL)
        CloseHandle(mapFile);

    return ret;
}



