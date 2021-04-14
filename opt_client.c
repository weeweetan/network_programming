#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_LEN 1000
//#define LOC_PORT 9090    //本地监听端口
//#define REM_PORT 8080    //远程发送端口

//#define SER_IP "10.0.0.1"
#define MAXSIZE 40

#define IPOPT_TAG 0x21        //IP选项标志字段
#define IPOPT_LEN 8            //IP选项长度字段


int str_to_number(const char* str);

int main(int argc, char** argv)
{
    char rec_buf[MAX_LEN];                                        //接受信息
    char snd_buf[MAX_LEN];                                        //发送信息

    int sk;                                                        //socket句柄
    struct sockaddr_in loc_addr;                                //用于指定本地监听信息
    struct sockaddr_in rem_addr;                                //获取远程地址信息
    int loc_addr_len,rem_addr_len;

    int count,ret;
    struct in_addr addr;                                        //用于获取地址信息
    int optval = 1;
    unsigned int SeqID=0;

    int LOC_PORT,REM_PORT;                                        //指定本地端口，和服务器端口
    char *SER_IP;                                                //用于获取服务器IP
    //构造自定义的TCP选项
    //unsigned char opt[MAXSIZE];
    //opt[0] = IPOPT_TAG;
    //opt[1] = IPOPT_LEN;
    unsigned char opt[MAXSIZE];
    opt[0] = 0x21;
    opt[1] = IPOPT_LEN;

    //检查传参
    if(argc != 4)                                                //第一个是程序名，第二个是本地端口，第三个是服务器ip,第四个是服务器端口
    {
        printf("Error: Number of Input Argv must be 4!\n");
        return -1;
    }
    LOC_PORT = str_to_number(argv[1]);
    REM_PORT = str_to_number(argv[3]);
    SER_IP = argv[2];

    bzero(&loc_addr, sizeof(loc_addr));
    loc_addr.sin_family = AF_INET;
    loc_addr.sin_addr.s_addr = htonl(INADDR_ANY);                //作为服务器，可能有多块网卡，设置INADDR_ANY，表示绑定一个默认网卡进行监听
    loc_addr.sin_port = htons(LOC_PORT);
    loc_addr_len = sizeof(loc_addr);

    bzero(&rem_addr, sizeof(rem_addr));
    rem_addr.sin_family = AF_INET;
    rem_addr.sin_addr.s_addr = inet_addr(SER_IP);                //作为服务器，可能有多块网卡，设置INADDR_ANY，表示绑定一个默认网卡进行监听
    rem_addr.sin_port = htons(REM_PORT);
    rem_addr_len = sizeof(loc_addr);

    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if(sk<0)
    {
        printf("socket create failure\n");
        return -1;
    }
    setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));    //设置地址复用

    ret = bind(sk, (struct sockaddr*)&loc_addr, loc_addr_len);
    if(ret < 0)
    {
        printf("socket bind failure\n");
        return -1;
    }

    while (1)
    {
        printf("Input info:>>>");
        scanf("%s", snd_buf);
        if (!strcmp(snd_buf, "quit"))
            break;
        //写入选项数据
        *(int *)(opt + 2) = htonl(++SeqID);
        setsockopt(sk,IPPROTO_IP,IP_OPTIONS,(void *)opt,IPOPT_LEN);
        sendto(sk, snd_buf, strlen(snd_buf)+1, 0, (struct sockaddr*)&rem_addr, rem_addr_len);
    }

    close(sk);

    return 0;
}

int str_to_number(const char* str)
{
    int i,len, num = 0;
    len= strlen(str);

    for (i = 0; i < len;i++)
        num = num * 10 + str[i] - '0';

    return num;
}
