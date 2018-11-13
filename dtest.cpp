#include <WinSock2.h>
#include <Windows.h>
#include "ipcpp.h"
#include "rtest.h"
#include <time.h>

#pragma comment(lib, "Ws2_32.lib")

int usleep(long usec)
{
    struct timeval tv;
    fd_set dummy;
    SOCKET s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    FD_ZERO(&dummy);
    FD_SET(s, &dummy);
    tv.tv_sec = usec/1000000L;
    tv.tv_usec = usec%1000000L;
    return select(0, 0, 0, &dummy, &tv);
}

#define  cmd_getstr 123
#define  cmd_getint 145
#define  cmd_inc    146

int on_cmd_getstr(unsigned char* data, int dsize)
{
    int len = strlen("hello, world!");
    memcpy(data, "hello, world!", len + 1);
    return len + 1;
}

int on_cmd_getint(unsigned char* data, int dsize)
{
    int len = 123456;
    memcpy(data, &len, sizeof(int));
    return sizeof(int);
}

int on_cmd_inc(unsigned char* data, int dsize)
{
    int i = *(int *)data;
    i++;
    memcpy(data, &i, sizeof(int));
    return sizeof(int);
}

int on_cmd_getstr2(unsigned char* data, int dsize)
{
    int len = strlen("wahahaha!");
    memcpy(data, "wahahaha!", len + 1);
    return len + 1;
}

int on_cmd_getint2(unsigned char* data, int dsize)
{
    int len = 445566;
    memcpy(data, &len, sizeof(int));
    return sizeof(int);
}

int on_cmd_inc2(unsigned char* data, int dsize)
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

namespace mutex
{
    class singleton
    {
    private:
        singleton()
        {
            InitializeCriticalSection(&cs);
        }
        CRITICAL_SECTION cs;
    public:
        static singleton* instance()
        {
            static volatile LONG _inited = 0;
            static singleton* o_inst = NULL;
            if (InterlockedExchange(&_inited, 1) == 0) 
            {
                o_inst = new singleton();
            }
            else 
            {
                while (!o_inst)
                {
                    Sleep(1);
                }
            }
            return o_inst;
        }
        ~singleton()
        {
            DeleteCriticalSection(&cs);
        }
        CRITICAL_SECTION* critical_section()
        {
            return &cs;
        }
    };

    class lock
    {
    public:
        lock()
        {
            EnterCriticalSection(singleton::instance()->critical_section());
        }
        ~lock()
        {
            LeaveCriticalSection(singleton::instance()->critical_section());
        }
    };
};

#define NUM_CLIENT  40

dipc::mutex mu;
int ms = 0;
bool flags[NUM_CLIENT];

DWORD WINAPI ClientThread(LPVOID lp)
{
    int no = (int)lp;
    flags[no] = false;
    //int ms;
    dipc::client* clt;
    if (no % 1) {
        clt = new dipc::client("named");
    } else {
        clt = new dipc::client();
    }

    for (int c = 0; c < 10000; c++)
    {
        {
            //dipc::locker lo(mu);
            //mutex::lock lo;
            //ms += 1;
            //srand(ms);
            //ms = rand() % 10;
            //printf("thread %d sleep for %d ms\n", no, ms);
            //Sleep(1);
            //printf("thread %d sleeped %d ms\n", no, ms);
        }

        //Sleep(ms);
        {
            std::string ret = clt->request(cmd_getstr);
            //         CommPacket* packet = (CommPacket*)&ret[0];
            //         CHECK(packet->cmd == cmd_hello);
            //std::string str((char*)&ret[0]);
            //CHECK_EQUAL(std::string("hello, world!"), str);
            //printf("thread %d recived: \t\t%s\n", no, str.c_str());
            CHECK(ret.size());
        }
        {
            // dipc::locker lo(mu);
            //srand(ms);
            // ms = rand() % 1000;
            //printf("thread %d sleep for %d ms\n", no, ms);
            // Sleep(ms);
            //printf("thread %d sleeped %d ms\n", no, ms);
        }
        {
			std::string ret = clt->request(cmd_getint);
            //CommPacket* packet = (CommPacket*)&ret[0];
            int len = *(int*)(ret.c_str());
            //CHECK_EQUAL(len, 123456);
            //printf("thread %d recived: \t\t%d\n", no, len);
            CHECK(ret.size());
        }
        {
            //         dipc::locker lo(mu);
            //         srand(ms);
            //         ms = rand() % 1000;
            //         //printf("thread %d sleep for %d ms\n", no, ms);
            //         Sleep(ms);
            //         //printf("thread %d sleeped %d ms\n", no, ms);
        }
        {
            int data = rand() % 1000;
			std::string ret = clt->request(cmd_inc, std::string((char*)&data, sizeof(int)));
            //CommPacket* packet = (CommPacket*)&ret[0];
            int bak = *(int*)(ret.c_str());
            //CHECK_EQUAL(bak, data + 1);
            //printf("thread %d sent: \t\t%d --> recived: \t\t%d\n", no, data, bak);
            CHECK(ret.size());
        }
    }
    flags[no] = true;

    delete clt;
    return 0;
}

DWORD WINAPI MutexThread(LPVOID lp)
{
    int no = (int)lp;
    for (int i = 0; i < 4000000; i++)
    {
        //mutex::lock l;
        dipc::locker lo(mu);
        ms += 1;
    }
    flags[no] = true;
    return 0;
}


int main()
{
    WORD wVersionRequested = MAKEWORD(1,0);
    WSADATA wsaData;
    WSAStartup(wVersionRequested, &wsaData);

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

    for (int i = 0; i < NUM_CLIENT; i++)
    {
        CreateThread(NULL, 0, MutexThread, (LPVOID)i, 0, NULL);
    }

    bool loop;
    //time_t start = time(NULL);
    DWORD start = GetTickCount();
    do 
    {
        Sleep(1);
        loop = false;
        for (int i = 0; i < NUM_CLIENT; i++)
        {
            if (!flags[i]) {
                loop = true;
            }
        }
    } while (loop);
    //int sec = time(NULL) - start;
    int msec = GetTickCount() - start;
    printf("%d\n%d\n", ms, msec);
    return 0;
}