//这是测试二进制传输的客户端文件
#include "include/TcpClient.h"
using namespace std;

struct student
{
    int id;
};

//接收消息，解析消息类型，在接收心跳包的时候重置计数
void parseRecvMsg(TcpClient& client)
{
    while(1)
    {
        //1.接收消息
        MessageType type;
        struct student stu;
        if (!client.recvMsgBin(&stu, type))
        {
            break;//防止死循环
        }
        //2.解析消息类型
        if(type==MessageType::Heart)
        {
            //2-1.重置计数
            cout<<"心跳包:"<<stu.id<<endl;
            client.resetCount();
        }
        else
        {
            cout<<"数据包:"<<stu.id<<endl;
        }
    }
}

void parseSendHeart(TcpClient& client)
{
    // int heartCount=0;
    struct student stu;
    stu.id=-1;
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
        client.sendMsgBin(&stu, sizeof(stu), MessageType::Heart);
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

    struct student stu;
    stu.id=0;
    while(1)
    {
        client.sendMsgBin(&stu, sizeof(stu));
        stu.id++;
        cout<<"发送成功"<<endl;
        sleep(1);
    }
    t.join();
    t2.join();
    return 0;
}