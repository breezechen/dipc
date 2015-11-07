#include <Windows.h>
#include "ipcpp.h"
#include "rtest.h"

#define  cmd_getstr 123
#define  cmd_getint 145
#define  cmd_inc    146

int on_cmd_getstr(unsigned char* data)
{
    int len = strlen("hello, world!");
    memcpy(data, "hello, world!", len + 1);
    return len + 1;
}

int on_cmd_getint(unsigned char* data)
{
    int len = 123456;
    memcpy(data, &len, sizeof(int));
    return sizeof(int);
}

int on_cmd_inc(unsigned char* data)
{
    int i = *(int *)data;
    i++;
    memcpy(data, &i, sizeof(int));
    return sizeof(int);
}

int on_cmd_getstr2(unsigned char* data)
{
    int len = strlen("wahahaha!");
    memcpy(data, "wahahaha!", len + 1);
    return len + 1;
}

int on_cmd_getint2(unsigned char* data)
{
    int len = 445566;
    memcpy(data, &len, sizeof(int));
    return sizeof(int);
}

int on_cmd_inc2(unsigned char* data)
{
    int i = *(int *)data;
    i += 2;
    memcpy(data, &i, sizeof(int));
    return sizeof(int);
}


DWORD WINAPI ServerThread(LPVOID lp)
{
    dipc::server* svr = (dipc::server*)lp;
    svr->run();
    return 0;
}

dipc::mutex mu;
int ms;
bool flags[10];

DWORD WINAPI ClientThread(LPVOID lp)
{
    int no = (int)lp;
    flags[no] = false;
    //int ms;
    dipc::client* clt;
    if (no % 2) {
        clt = new dipc::client("named");
    } else {
        clt = new dipc::client();
    }

    {
        dipc::locker lo(mu);
        srand(ms);
        ms = rand() % 1000;
        //printf("thread %d sleep for %d ms\n", no, ms);
        Sleep(ms);
        //printf("thread %d sleeped %d ms\n", no, ms);
    }
    
    //Sleep(ms);
    {
        std::vector<unsigned char> ret = clt->request(cmd_getstr, NULL, 0);
//         CommPacket* packet = (CommPacket*)&ret[0];
//         CHECK(packet->cmd == cmd_hello);
        std::string str((char*)&ret[0]);
        //CHECK_EQUAL(std::string("hello, world!"), str);
        printf("thread %d recived: \t\t%s\n", no, str.c_str());
    }
    {
        dipc::locker lo(mu);
        srand(ms);
        ms = rand() % 1000;
        //printf("thread %d sleep for %d ms\n", no, ms);
        Sleep(ms);
        //printf("thread %d sleeped %d ms\n", no, ms);
    }
    {
        std::vector<unsigned char> ret = clt->request(cmd_getint, NULL, 0);
        //CommPacket* packet = (CommPacket*)&ret[0];
        int len = *(int*)(&ret[0]);
        //CHECK_EQUAL(len, 123456);
        printf("thread %d recived: \t\t%d\n", no, len);
    }
    {
        dipc::locker lo(mu);
        srand(ms);
        ms = rand() % 1000;
        //printf("thread %d sleep for %d ms\n", no, ms);
        Sleep(ms);
        //printf("thread %d sleeped %d ms\n", no, ms);
    }
    {
        int data = rand() % 1000;
        std::vector<unsigned char> ret = clt->request(cmd_inc, (unsigned char*)&data, sizeof(int));
        //CommPacket* packet = (CommPacket*)&ret[0];
        int bak = *(int*)(&ret[0]);
        //CHECK_EQUAL(bak, data + 1);
        printf("thread %d sent: \t\t%d --> recived: \t\t%d\n", no, data, bak);
    }
    flags[no] = true;

    delete clt;
    return 0;
}


int main()
{
    srand(GetTickCount());

    dipc::server s;
    s.route(cmd_getstr, on_cmd_getstr);
    s.route(cmd_getint, on_cmd_getint);
    s.route(cmd_inc, on_cmd_inc);
    
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerThread, (LPVOID)&s, 0, NULL);

    dipc::server s2("named");
    s2.route(cmd_getstr, on_cmd_getstr2);
    s2.route(cmd_getint, on_cmd_getint2);
    s2.route(cmd_inc, on_cmd_inc2);

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerThread, (LPVOID)&s2, 0, NULL);

    for (int i = 0; i < 10; i++)
    {
        CreateThread(NULL, 0, ClientThread, (LPVOID)i, 0, NULL);
    }

    bool loop;
    do 
    {
        Sleep(1000);
        loop = false;
        for (int i = 0; i < 10; i++)
        {
            if (!flags[i]) {
                loop = true;
            }
        }
    } while (loop);

    return 0;
}