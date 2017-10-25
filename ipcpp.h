#ifndef _DIPC_H_
#define _DIPC_H_

#include <vector>

#include "ipc.h"

namespace dipc
{
    class mutex
    {
    private:
        volatile LONG interlock_;
    public:
        mutex();
        ~mutex();
    private:
        void lock();
        void unlock();
        friend class locker;
    };

    class locker
    {
        mutex&    m_;
    public:
        locker(mutex& m);
        ~locker();
    };

    typedef int (*pf_handler)(unsigned char* data);

#ifdef _UNICODE
#define  tstring    wstring
#else
#define  tstring    string
#endif

    class server {
        struct router {
            int cmd;
            pf_handler handler;
        };

    public:
        server(const std::tstring& name = std::tstring(), int timeout = 1000);
        ~server();
        void run();
        void stop();
        void route(int cmd, pf_handler handler);

    private:
        bool stop_;
        mutex mr_;
        std::vector<router> routers_;
		IpcServer* data_;
    };

    class client {
    public:
        client(const std::tstring& server_name = std::tstring(), int timeout = 1000);
        ~client();

		std::string request(int cmd, const std::string& data = std::string());
    private:
        std::tstring server_name;
        int timeout;
    };
}

#endif