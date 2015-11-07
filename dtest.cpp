#include <Windows.h>
#include "dipc.h"
#include "rtest.h"

#define  cmd_getstr 123
#define  cmd_getint 145

int on_cmd_getstr(unsigned char* data)
{
    int len = strlen("hello, world!");
    memcpy(data, "hello, world!", len + 1);
    return len + 1;
}

int on_cmd_getint(unsigned char* data)
{
    int len = 9527;
    memcpy(data, &len, sizeof(int));
    return sizeof(int);
}

DWORD WINAPI ServerThread(LPVOID lp)
{
    dipc::server* svr = (dipc::server*)lp;
    svr->run();
    return 0;
}

DWORD WINAPI ClientThread(LPVOID lp)
{
    int no = (int)lp;
    dipc::client clt;
    srand(GetTickCount());
    int ms = (rand() % 5) * 1000;
    printf("thread %d sleep: %d\n", no, ms);
    Sleep(ms);
    {
        std::vector<unsigned char> ret = clt.request(cmd_getstr, NULL, 0);
//         CommPacket* packet = (CommPacket*)&ret[0];
//         CHECK(packet->cmd == cmd_hello);
        std::string str((char*)&ret[0]);
        CHECK_EQUAL(std::string("hello, world!"), str);
        printf("thread %d recived: %s\n", no, str.c_str());
    }

    {
        std::vector<unsigned char> ret = clt.request(cmd_getint, NULL, 0);
        //CommPacket* packet = (CommPacket*)&ret[0];
        int len = *(int*)(&ret[0]);
        CHECK_EQUAL(len, 9527);
        printf("thread %d recived: %d\n", no, len);
    }

    return 0;
}


int main()
{
    srand(GetTickCount());

    dipc::server s;
    s.route(cmd_getstr, on_cmd_getstr);
    s.route(cmd_getint, on_cmd_getint);
    
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerThread, (LPVOID)&s, 0, NULL);
    //Sleep(2000);
    for (int i = 0; i < 10; i++)
    {
        CreateThread(NULL, 0, ClientThread, (LPVOID)i, 0, NULL);
        //Sleep(2000);
    }

    Sleep(5000);

    return 0;
}