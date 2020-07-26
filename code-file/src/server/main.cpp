#include "ChatServer.hpp"
#include "ChatService.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

//服务端异常中断处理
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "Invaild input,Example:./ChatServer 127.0.0.1 600X " << endl;
        exit(-1);
    }
    signal(SIGINT, resetHandler);
    EventLoop loop;
    char *ipaddr = argv[1];
    uint16_t port = atoi(argv[2]);
    InetAddress addr(ipaddr, port);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();
}
