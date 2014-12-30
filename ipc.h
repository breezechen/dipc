#ifndef _BREEZE_SIMPLE_IPC_H_
#define _BREEZE_SIMPLE_IPC_H_


#define MAX_NAME_LEN	64

// 4M
#define MEMMAP_SIZE   4 * 1024 *1024

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct _IpcServer{
    HANDLE mapFile;
    HANDLE reqEvent;
    HANDLE repEvent;
    BYTE * buf;
	DWORD timeout;
}IpcServer, *PIpcServer;

typedef struct _CommPacket{
    ULONG   size;
    ULONG   cmd;
    BYTE data[0];
}CommPacket, *PCommPacket;

#pragma pack(pop)

#ifdef __cplusplus
IpcServer* ServerCreate(LPCTSTR serverName = NULL);
#else
IpcServer* ServerCreate(LPCTSTR serverName);
#endif

BOOL ServerWaitForRequst(IpcServer* server);
VOID ServerReplied(IpcServer* server);
VOID ServerClose(IpcServer* server);

#ifdef __cplusplus
CommPacket* ClientRequest(ULONG cmd, const BYTE* data, SIZE_T size, LPCTSTR serverName = NULL, DWORD timeout = 1000);
#else
CommPacket* ClientRequest(ULONG cmd, const BYTE* data, SIZE_T size, LPCTSTR serverName, DWORD timeout);
#endif
VOID FreePacket(CommPacket *packet);


#ifdef __cplusplus
}
#endif
#endif