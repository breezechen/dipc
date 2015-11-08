dipc
====

A damn lightweight IPC implementation worked on windows. It's using shared memory and windows event, yes, it's rude and stupid, but it works.  

There is also a c++ wrapper provided.

How to use?
-----------
* Using in c  
    Just include `ipc.c` and `ipc.h` into your project. See `test.cpp`.

* Using in c++  
    Download `ipc.c` `ipc.h` `ipcpp.cpp` `ipcpp.h` to your project directory, and using it by including `ipcpp.cpp` and `ipcpp.h`. See `dtest.cpp` for more information.

```c++
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
    int len = 123456;
    memcpy(data, &len, sizeof(int));
    return sizeof(int);
}

int main(int argc, char** argv)
{
    dipc::server s;
    s.route(cmd_getstr, on_cmd_getstr);
    s.route(cmd_getint, on_cmd_getint);

    {
        dipc::client clt;
        std::vector<unsigned char> ret = clt.request(cmd_getstr, NULL, 0);
        std::string str((char*)&ret[0]);
        CHECK_EQUAL(std::string("hello, world!"), str);
    }
    {
        dipc::client clt;
        std::vector<unsigned char> ret = clt.request(cmd_getint, NULL, 0);
        int len = *(int*)(&ret[0]);
        CHECK_EQUAL(len, 123456);
    }

    return 0;
}


```

License?
--------
It's under MIT.

The unit test lib from <https://github.com/rioki/rtest.git>. Check license <https://github.com/rioki/rtest#license>.

