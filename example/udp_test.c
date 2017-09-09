#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../src/ae.h"
#include "../src/anet.h"
#include "../src/zmalloc.h"

#define CLIENT_PORT 11567
#define SERVER_PORT 11568
#define SERVER_ADDRESS "0.0.0.0"
#define UNIX_SOCK_UDP_OUT "/Users/liangkui/Projects/c/liae-test/usb_proxy_svr_udp_out"
#define UNIX_SOCK_UDP_IN "/Users/liangkui/Projects/c/liae-test/usb_proxy_svr_udp_in"

void readFromHost(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int buffer_size = 1024;
    char *buffer = (char *)zmalloc(sizeof(char) * buffer_size);
    int size = read(fd, buffer, buffer_size);
    printf("Receive: %d\n", size);
    printf("%s\n", buffer);
    anetWrite(fd, buffer, strlen(buffer));
    zfree(buffer);
}

void readFromClient(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int buffer_size = 1024;
    char *buffer = (char *)zmalloc(sizeof(char) * buffer_size);
    uint32_t client_len;
    struct sockaddr_in client_addr;
    bzero((char *)&client_addr, sizeof(client_addr));
    int size = recvfrom(fd, buffer, buffer_size, 0, (struct sockaddr *)&client_addr, &client_len);
    printf("Receive: %d\n", size);
    printf("%s\n", buffer);
    aeDeleteFileEvent(loop, fd, AE_READABLE);
    close(fd);

    int s = anetUdpConnectSock(NULL, SERVER_PORT, (struct sockaddr *)&client_addr);
    assert(s != ANET_ERR);
    aeCreateFileEvent(loop, s, AE_READABLE, readFromHost, NULL);
    anetWrite(fd, buffer, strlen(buffer));
}

void runServer()
{
    int ipfd;
    char error[256];
    // create server socket
    ipfd = anetUdpCreate(error, SERVER_PORT);
    assert(ipfd != ANET_ERR);

    // create main event loop
    aeEventLoop *loop;
    loop = aeCreateEventLoop(10);

    // regist socket connect callback
    int ret;
    ret = aeCreateFileEvent(loop, ipfd, AE_READABLE, readFromClient, NULL);

    assert(ret != AE_ERR);

    // start main loop
    aeMain(loop);

    // stop loop
    aeDeleteEventLoop(loop);
}

void runClient(int times)
{
    char error[256];

    int ipfd = anetUdpConnect(error, CLIENT_PORT, SERVER_ADDRESS, SERVER_PORT);

    assert(ipfd != ANET_ERR);

    // create main event loop
    aeEventLoop *loop;
    loop = aeCreateEventLoop(10);

    // regist socket connect callback
    int ret;
    ret = aeCreateFileEvent(loop, ipfd, AE_READABLE, readFromHost, NULL);
    assert(ret != AE_ERR);

    char *hello = "hello";
    write(ipfd, hello, strlen(hello));

    // start main loop
    aeMain(loop);

    // stop loop
    aeDeleteEventLoop(loop);
}

void runUnixTest()
{
    char error[256];

    unlink(UNIX_SOCK_UDP_OUT);
    int out = anetUnixUdpCreate(error, UNIX_SOCK_UDP_OUT);
    assert(out != ANET_ERR);

    unlink(UNIX_SOCK_UDP_IN);
    int in = anetUnixUdpCreate(error, UNIX_SOCK_UDP_IN);
    assert(in != ANET_ERR);

    out = anetUnixUdpConnect(error, out, UNIX_SOCK_UDP_IN);
    assert(out != ANET_ERR);

    in = anetUnixUdpConnect(error, in, UNIX_SOCK_UDP_OUT);
    assert(out != ANET_ERR);

    // create main event loop
    aeEventLoop *loop;
    loop = aeCreateEventLoop(10);

    // regist socket connect callback
    int ret;
    ret = aeCreateFileEvent(loop, in, AE_READABLE, readFromHost, NULL);
    assert(ret != AE_ERR);

    ret = aeCreateFileEvent(loop, out, AE_READABLE, readFromHost, NULL);
    assert(ret != AE_ERR);

    char *hello = "hello";
    write(out, hello, strlen(hello));

    // start main loop
    aeMain(loop);

    // stop loop
    aeDeleteEventLoop(loop);
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        printf("Usage : ae_test [server | client] $loop_times");
    }

    if (memcmp("server", argv[1], strlen(argv[1])) == 0)
    {
        runServer();
    }
    else if (memcmp("client", argv[1], strlen(argv[1])) == 0)
    {
        int times = atoi(argv[2]);
        runClient(times);
    }
    else if (memcmp("local", argv[1], strlen(argv[1])) == 0)
    {
        runUnixTest();
    }
    else
    {
        printf("Arguments invalid, %s", argv[1]);
    }

    return 0;
}
