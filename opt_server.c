#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_LEN 1000
#define MAX_SIZE 40
#define IPOPT_LEN 8
//#define LOC_PORT 8080                                                        //本地监听端口

int str_to_number(const char* str);

int main(int argc, char** argv)
{
    char rec_buf[MAX_LEN];                                                    //接受信息
    char snd_buf[MAX_LEN];                                                    //发送信息

    int sk;                                                                    //socket句柄
    struct sockaddr_in loc_addr;                                            //用于指定本地监听信息
    struct sockaddr_in rem_addr;                                            //获取远程地址信息
    int loc_addr_len,rem_addr_len;

    int count,ret,i,opt_len=MAX_SIZE;
    struct in_addr addr;                                                    //用于获取地址信息
    int optval = 1;

    int LOC_PORT;

    unsigned char opt[MAX_SIZE];
    //opt[0] = 0x21;
    //opt[1] = IPOPT_LEN;

    //检查传参
    if(argc != 2)                                                            //第一个是程序名，第二个是本地端口
    {
        printf("Error: Number of Input Argv must be 2!\n");
        return -1;
    }

    LOC_PORT = str_to_number(argv[1]);

    bzero(&loc_addr, sizeof(loc_addr));
    loc_addr.sin_family = AF_INET;
    loc_addr.sin_addr.s_addr = htonl(INADDR_ANY);                            //作为服务器，可能有多块网卡，设置INADDR_ANY，表示绑定一个默认网卡进行监听
    loc_addr.sin_port = htons(LOC_PORT);
    loc_addr_len = sizeof(loc_addr);

    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if(sk<0)
    {
        printf("socket create failure\n");
        return -1;
    }
    setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));            //设置地址复用

    ret = bind(sk, (struct sockaddr*)&loc_addr, loc_addr_len);
    if(ret < 0)
    {
        printf("socket bind failure\n");
        return -1;
    }


    while (1)
    {
        printf("Waiting for data from sender \n");
        count = recvfrom(sk, rec_buf, MAX_LEN, 0, (struct sockaddr*)&rem_addr, &rem_addr_len);
        //printf("%s %d\n", rec_buf,count);
        if(count==-1)
        {
            printf("receive data failure\n");
            return -1;
        }

        addr.s_addr = rem_addr.sin_addr.s_addr;
        printf("Receive info: %s from %s %d\n", rec_buf,inet_ntoa(addr),rem_addr.sin_port);
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

