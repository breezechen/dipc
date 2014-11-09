#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include "ipc.h"

BOOL gServerExit = FALSE;

#define cmd_hello	101

#define cmd_set_str	201
#define cmd_get_str	202

#define cmd_set_server_timeout	301

#define cmd_rep_ok	1
#define cmd_rep_not_support	2


DWORD WINAPI ServerThread(LPVOID param)
{
	IpcServer* server = ServerCreate();
	if (!server)
		return -1;

	time_t start = time(NULL);

	while(!gServerExit)
	{
		ServerReady(server);
		if (ServerWaitForRequst(server))
		{
			int sec = time(NULL) - start;
			CommPacket* packet = (CommPacket *)server->buf;
			switch (packet->cmd)
			{
			case cmd_hello:
			printf("%04d Server: get hello\n", sec);
				packet->cmd = cmd_rep_ok;
				packet->size = sizeof(CommPacket);
				break;
			case cmd_set_str:
				{
					char *str = (char *)packet->data;
				printf("%d Server: request str->%s\n", sec, str);
					packet->cmd = cmd_rep_ok;
					packet->size = sizeof(CommPacket);
				}
				break;
			case cmd_get_str:
				strcpy((char *)packet->data, "world");
				packet->cmd = cmd_rep_ok;
				packet->size = sizeof(CommPacket) + strlen("world") + 1;
			printf("%04d Server: ask for reply str->hello\n", sec);
				break;
			case cmd_set_server_timeout:
				{
					ULONG* timeout = (ULONG *)packet->data;
					server->timeout = *timeout;
				printf("%04d Server: set server wait timeout to %d\n", sec, *timeout);
					packet->cmd = cmd_rep_ok;
					packet->size = sizeof(CommPacket);
				}
				break;
			default:
			printf("%04d Server: cmd not supported\n", sec);
				packet->cmd = cmd_rep_not_support;
				packet->size = sizeof(CommPacket);
				break;
			}
			ServerReplied(server);
			ServerWaitClientDone(server);
		}
		else
		{
			int sec = time(NULL) - start;
		printf("%04d Server: no request\n", sec);
		}
	}
	ServerClose(server);
}

DWORD WINAPI ClientThread(LPVOID param)
{
	int no = (int)param;
	bool ok = false;
	while(1)
	{
		srand(GetTickCount());

		{
			do 
			{
				Sleep(rand() % 5);
				CommPacket *ret = (CommPacket *)ClientRequest(cmd_hello, NULL, 0);
				if (ret)
				{
					if (ret->cmd == cmd_rep_ok) {
					printf("Client %d: server replied ok\n", no);
					}
					free(ret);
					ok = true;
				}
				else 
				{
				printf("Client %d: oh! timeout~\n", no);
					ok = false;
				}
			} while (!ok);
		}

		
		{
			do 
			{
				Sleep(rand() % 5);
				char buf[32];
				sprintf(buf, "I am client %d", no);
				CommPacket *ret = (CommPacket *)ClientRequest(cmd_set_str, (BYTE *)buf, strlen(buf) + 1);
				if (ret)
				{
					if (ret->cmd == cmd_rep_ok) {
					printf("Client %d: server replied ok\n", no);
					}
					free(ret);
					ok = true;
				}
				else 
				{
				printf("Client %d: oh! timeout~\n", no);
					ok = false;
				}
			} while (!ok);
		}

		{
			do 
			{
				Sleep(rand() % 5);
				CommPacket *ret = (CommPacket *)ClientRequest(cmd_get_str, NULL, 0);
				if (ret)
				{
					if (ret->cmd == cmd_rep_ok) {
						char *str = (char *)ret->data;
					printf("Client %d: server replied->%s\n", no, str);
					}
					free(ret);
					ok = true;
				}
				else
				{
				printf("Client %d: oh! timeout~\n", no);
					ok = false;
				}
			} while (!ok);
			
		}

		
		{
			do 
			{
				Sleep(rand() % 5);
				ULONG timeout = 2000;
				CommPacket *ret = (CommPacket *)ClientRequest(cmd_set_server_timeout, (BYTE *)&timeout, sizeof(ULONG));
				if (ret)
				{
					if (ret->cmd == cmd_rep_ok) {
					printf("Client %d: server replied ok\n", no);
					}
					free(ret);
					ok = true;
				}
				else
				{
				printf("Client %d: oh! timeout~\n", no);
					ok = false;
				}
			} while (!ok);
			
		}

		
		{
			do 
			{
				Sleep(rand() % 5);
				CommPacket *ret = (CommPacket *)ClientRequest(44444, NULL, 0);
				if (ret)
				{
					if (ret->cmd == cmd_rep_ok) {
					printf("Client %d: server replied ok\n", no);
					}
					else if (ret->cmd == cmd_rep_not_support) {
					printf("Client %d: server replied not support\n", no);
					}
					free(ret);
					ok = true;
				}
				else
				{
				printf("Client %d: oh! timeout~\n", no);
					ok = false;
				}
			} while (!ok);
		}

	}
}
int main(int argc, char **argv)
{
	CreateThread(NULL, 0, ServerThread, NULL, 0, NULL);
	//Sleep(2000);
	for (int i = 0; i < 5; i++)
	{
		CreateThread(NULL, 0, ClientThread, (LPVOID)i, 0, NULL);
		//Sleep(2000);
	}
	Sleep(2000);
	gServerExit = TRUE;
	Sleep(5000);
	gServerExit = FALSE;
	CreateThread(NULL, 0, ServerThread, NULL, 0, NULL);
	Sleep(20000);
}