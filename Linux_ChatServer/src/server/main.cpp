#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv){
    if (argc < 2){
        cerr << "command invalid! example: ./ChatServer 8000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port

    char buf[64] = "127.0.0.1";

    char *ip;
    uint16_t port;
    if(argc==3){
        port = atoi(argv[1]);
        ip = argv[2];
    }
    else{
        port = atoi(argv[1]);
        ip = buf;
    }

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(port,ip);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}
