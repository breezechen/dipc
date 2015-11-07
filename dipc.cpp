#include <Windows.h>
#include <string>

#include "ipc.h"
extern "C" {
#include "ipc.c"
};

#include "dipc.h"

namespace dipc
{
  
    mutex::mutex()   {};
    mutex::~mutex()  {};
    void mutex::lock()    {
        while(this->interlock_ == 1 
            || InterlockedCompareExchange(&this->interlock_, 1, 0) == 1) { 
            Sleep(1);
        }
    }
    void mutex::unlock() {
        this->interlock_ = 0;
    }

    locker::locker(mutex& m): m_(m) {
        m_.lock();
    }
    locker::~locker()  {
        m_.unlock();
    }

    server::server(const std::tstring& name, int timeout) :
    data_(NULL), stop_(false), routers_() {
        do  {
            data_ = ServerCreate(name.empty()?(NULL):(name.c_str()));
            if (data_) {
                data_->timeout = timeout;
            }
        } while (false);
    }

    server::~server(){ }

    void server::run() {
        stop_ = false;
        while(!stop_) {
            ServerReady(data_);
            if (ServerWaitForRequst(data_)) {
                CommPacket* packet = (CommPacket*)data_->buf;
                int i = 0;
                for (; i < routers_.size(); i++)
                {
                    if (routers_[i].cmd == packet->cmd) {
                        packet->size = routers_[i].handler((unsigned char*)packet->data) + sizeof(CommPacket);
                        break;
                    }
                }
                if (i == routers_.size()) {
                    packet->cmd = -1;
                    packet->size = sizeof(CommPacket);
                }
                ServerReplied(data_);
                ServerWaitClientDone(data_);
            }
        }
    }

    void server::stop() { stop_ = true; }
    void server::route(int cmd, pf_handler handler) {
        std::vector<router>::iterator it = routers_.begin();
        for (; it != routers_.end(); it++)
        {
            if (it->cmd == cmd) {
                it->handler = handler;
                break;
            }
        }
        if (it == routers_.end()) {
            router r = {cmd, handler};
            routers_.push_back(r);
        }
    }

    client::client(const std::tstring& server_name, int timeout) :
    server_name(server_name), timeout(timeout) { }

    client::~client() { }

    std::vector<unsigned char> client::request(int cmd, unsigned char* data, int data_size) {
        CommPacket* packet = ClientRequest(cmd, data, data_size, server_name.c_str(), timeout);
        std::vector<unsigned char> ret((unsigned char*)packet, (unsigned char*)packet + packet->size);
        FreePacket(packet);
        return ret;
    }
}