#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "../src/ae.h"
#include "../src/anet.h"
#include "../src/zmalloc.h"
#include "channel_io.h"
#include "protocol.h"

typedef struct channel_context 
{
    channel_io_t * channel;
    protocol_t * protocol;
}channel_context_t;

void readFromHost(aeEventLoop *loop, int fd, void *clientdata, int mask) 
{
    channel_context_t * context = (channel_context_t *) clientdata;
    if (context->channel->isDatagram) {
        context->protocol->parse_packet_datagram();
    }else {
        context->protocol->parse_packet_stream();
    }
}

void acceptTcpHandler(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int client_fd;
    channel_context_t * context = (channel_context_t *) clientdata;
	// create client socket
	client_fd = anetUnixAccept(NULL, fd);
    printf("Accepted !!\n");
    
    context->channel->fd = client_fd;
	// set client socket non-block
	anetNonBlock(NULL, client_fd);

	// regist on message callback
    aeCreateFileEvent(loop, client_fd, AE_READABLE, readFromHost, context);
}

int main(int arg, char *argv[]) 
{
    protocol_t * protocol = &mavlink_protocol;
    protocol->init(NULL);

    // create main event loop
	aeEventLoop *loop;
    loop = aeCreateEventLoop(100);
    uint32_t channel_num =  sizeof (channels_io) / sizeof (channels_io[0]);

    for (int i =0; i < channel_num; ++i) 
    {
        int32_t fd = channels_io[i]->open();
        if (fd < 0) {
            printf("Open fail, msg_channel: %d\n", channels_io[i]->channel);
            goto error;
        }

        channel_context_t * context = (channel_context_t*) zmalloc(sizeof(channel_context_t));
        context->channel = channels_io[i];
        context->protocol = protocol;
        if (channels_io[i]->isDatagram)
        {
            aeCreateFileEvent(loop, channels_io[i]->fd, AE_READABLE, readFromHost, context);
        }else {
            //Tcp connection, need add accept new connect.
            aeCreateFileEvent(loop, channels_io[i]->listen_fd, AE_READABLE, acceptTcpHandler, context);
        }
        channels_io[i]->probe(channels_io[i],channel_num, channels_io);
    }

    // start main loop
	aeMain(loop);

    for (int i = 0; i < channel_num ; ++i) {
        if (!channels_io[i]->isDatagram) {
            aeDeleteFileEvent(loop,  channels_io[i]->listen_fd, AE_READABLE);
        }
        aeDeleteFileEvent(loop,  channels_io[i]->fd, AE_READABLE);
    }
error:
    for (int i =0; i < channel_num; ++i) {
        channels_io[i]->close();
    }
    // stop loop
    aeDeleteEventLoop(loop);
    protocol->deinit();

    return 0;
}