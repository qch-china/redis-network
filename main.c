#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include "ae.h"
#include "anet.h"

#define PORT 4444
#define MAX_LEN 1024

//存放错误信息的字符串
char g_err_string[1024];

//事件循环机制
aeEventLoop *g_event_loop = NULL;

//定时器的入口，输出一句话
int PrintTimer(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
    static int i = 0;
    printf("Test Output: %d\n", i++);

    //10秒后再次执行该函数
    return 10000;
}

//停止事件循环
void StopServer()
{
    aeStop(g_event_loop);
}

void ClientClose(aeEventLoop *el, int fd, int err)
{
    //如果err为0，则说明是正常退出，否则就是异常退出
    if( 0 == err )
        printf("Client quit: %d\n", fd);
    else if( -1 == err )
        fprintf(stderr, "Client Error: %s\n", strerror(errno));

    //删除结点，关闭文件
    aeDeleteFileEvent(el, fd, AE_READABLE);
    close(fd);
}

//有数据传过来了，读取数据
void ReadFromClient(aeEventLoop *el, int fd, void *privdata, int mask)
{
    char buffer[MAX_LEN] = { 0 };
    int res;
    res = read(fd, buffer, MAX_LEN);
    if( res <= 0 )
    {
        ClientClose(el, fd, res);
    }
    else
    {
        res = write(fd, buffer, MAX_LEN);
        if( -1 == res )
            ClientClose(el, fd, res);
    }
}

//接受新连接
void AcceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
    int cfd, cport;
    char ip_addr[128] = { 0 };
    cfd = anetTcpAccept(g_err_string, fd, ip_addr, &cport);
    printf("Connected from %s:%d\n", ip_addr, cport);

    if( aeCreateFileEvent(el, cfd, AE_READABLE,
        ReadFromClient, NULL) == AE_ERR )
    {
        fprintf(stderr, "client connect fail: %d\n", fd);
        close(fd);
    }
}

int main()
{

    printf("Start\n");

    signal(SIGINT, StopServer);

    //初始化网络事件循环
    g_event_loop = aeCreateEventLoop(1024*10);

    //设置监听事件
    int fd = anetTcpServer(g_err_string, PORT, NULL);
    if( ANET_ERR == fd )
        fprintf(stderr, "Open port %d error: %s\n", PORT, g_err_string);
    if( aeCreateFileEvent(g_event_loop, fd, AE_READABLE, 
        AcceptTcpHandler, NULL) == AE_ERR )
        fprintf(stderr, "Unrecoverable error creating server.ipfd file event.");

    //设置定时事件
    aeCreateTimeEvent(g_event_loop, 1, PrintTimer, NULL, NULL);

    //开启事件循环
    aeMain(g_event_loop);

    //删除事件循环
    aeDeleteEventLoop(g_event_loop);

    printf("End\n");

    return 0;
}