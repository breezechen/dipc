#include <Windows.h>
#include "dipc.h"
#include "rtest.h"

#define  cmd_hello  123

int on_cmd_getstr(unsigned char* data)
{
    int len = strlen("hello, world!");
    memcpy(data, "hello, world!", len + 1);
    return len + 1;
}

DWORD WINAPI ServerThread(LPVOID lp)
{
    dipc::server* svr = (dipc::server*)lp;
    svr->run();
    return 0;
}

DWORD WINAPI ClientThread(LPVOID lp)
{
    dipc::client clt;
    {
        std::vector<unsigned char> ret = clt.request(cmd_hello, NULL, 0);
        CommPacket* packet = (CommPacket*)&ret[0];
        CHECK(packet->cmd == cmd_hello);
        CHECK_EQUAL(std::string("hello, world!"), std::string((char*)packet->data));
    }

    Sleep((rand() % 5) * 200);

    {
        std::vector<unsigned char> ret = clt.request(356, NULL, 0);
        CommPacket* packet = (CommPacket*)&ret[0];
        CHECK(packet->cmd == -1);
    }

    return 0;
}


int main()
{
    srand(GetTickCount());

    dipc::server s;
    s.route(cmd_hello, on_cmd_getstr);
    
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerThread, (LPVOID)&s, 0, NULL);
    //Sleep(2000);
    for (int i = 0; i < 5; i++)
    {
        CreateThread(NULL, 0, ClientThread, (LPVOID)i, 0, NULL);
        //Sleep(2000);
    }

    Sleep(5000);

    return 0;
}