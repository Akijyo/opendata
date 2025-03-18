//这是测试TcpClient类的客户端文件
#include "include/TcpClient.h"
using namespace std;

//接收消息，解析消息类型，在接收心跳包的时候重置计数
void parseRecvMsg(TcpClient& client)
{
    while(1)
    {
        //1.接收消息
        MessageType type;
        string msg;
        client.recvMsgWithType(msg,type);
        if(msg.empty())
        {
            break;//防止死循环
        }
        //2.解析消息类型
        if(type==MessageType::Heart)
        {
            //2-1.重置计数
            cout<<"心跳包:"<<msg<<endl;
            client.resetCount();
        }
        else
        {
            cout<<"数据包:"<<msg<<endl;
        }
    }
}

void parseSendHeart(TcpClient& client)
{
    //int heartCount=0;
    while(1)
    {
        cout<<"心跳包计数:"<<client.getCount()<<endl;
        //心跳包计数+1
        client.addCount();
        //如果心跳包计数超过3次，断开连接
        if(client.getCount()>3)
        {
            cout<<"心跳包计数超过3次,断开连接"<<endl;
            exit(0);
        }
        // if(heartCount<5)
        // {
        //         client.sendMsgWithType("heart beat",MessageType::Heart);
        //         heartCount++;
        // }
        //发送给服务器心跳包
        client.sendMsgWithType("heart beat",MessageType::Heart);
        sleep(5);
    }
}

int main()
{
    //创建客户端对象
    TcpClient client;
    client.connectServer("127.0.0.1",8888);

    //创建两个线程，一个用于接收消息，一个用于发送心跳包
    thread t(parseRecvMsg,ref(client));
    thread t2(parseSendHeart,ref(client));
    int number=0;
    while(1)
    {
        client.sendMsgWithType("hello world" + to_string(number));
        number++;
        cout<<"发送成功"<<endl;
        sleep(1);
    }
    t.join();
    t2.join();
    return 0;
}