#include "redis.hpp"
#include <iostream>
#include <unistd.h>
using namespace std;

Redis::Redis()
    : publish_context_(nullptr), subcribe_context_(nullptr)
{
}

Redis::~Redis(){
    if (publish_context_ != nullptr){
        redisFree(publish_context_);
    }

    if (subcribe_context_ != nullptr){
        redisFree(subcribe_context_);
    }
}

bool Redis::connect(){
    // 负责publish发布消息的上下文连接
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if (nullptr == publish_context_){
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    subcribe_context_ = redisConnect("127.0.0.1", 6379);
    if (nullptr == subcribe_context_){
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;

    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message){
    redisReply *reply = (redisReply *)redisCommand(publish_context_, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply){
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel){
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->subcribe_context_, "SUBSCRIBE %d", channel)){
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done){
        if (REDIS_ERR == redisBufferWrite(this->subcribe_context_, &done)){
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetReply

    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel){
    if (REDIS_ERR == redisAppendCommand(this->subcribe_context_, "UNSUBSCRIBE %d", channel)){
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done){
        if (REDIS_ERR == redisBufferWrite(this->subcribe_context_, &done)){
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message(){
    sleep(1);
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->subcribe_context_, (void **)&reply)){
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
            // 给业务层上报通道上发生的消息
            notify_message_handler_(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}


void Redis::init_notify_handler(function<void(int,string)> fn){
    this->notify_message_handler_ = fn;
}
