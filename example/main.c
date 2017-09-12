#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "../src/ae.h"
#include "../src/anet.h"
#include "../src/zmalloc.h"
#include "channel_io.h"
#include "protocol.h"

static uint8_t rx_buff[MAX_PROTO_BUF_SIZE];
static msg_packet_t rx_msg_packet;
static bool is_running = true;

typedef struct channel_context 
{
    channel_io_t * channel;
    protocol_t * protocol;
    struct channel_context * next;
}channel_context_t;

static int32_t setup_channel(channel_context_t * context, aeEventLoop * loop, aeFileProc * proc) 
{
    channel_io_t * channel = context->channel;

    int32_t fd = channel->open();
    if (fd < 0) {
        loge("Open fail, msg_channel: %d\n", channel->channel);
        return fd;
    }

    if (channel->isDatagram)
    {
        aeCreateFileEvent(loop, channel->fd, AE_READABLE, proc, context); 
    }else {
        aeCreateFileEvent(loop, channel->listen_fd, AE_READABLE, proc, context);
    }
    channel->probe(channel, CHANNELS_NUMBER, channels_io);
    return fd;
}

void readFromHost(aeEventLoop *loop, int fd, void *clientdata, int mask) 
{
    channel_context_t * context = (channel_context_t *) clientdata;
    int32_t ret = read(context->channel->fd, rx_buff, MAX_PROTO_BUF_SIZE);

    if (ret <= 0) {
        if (errno != EAGAIN && errno != EINTR)
        {
            //Some error happen, need re-setup the channel io.
            aeDeleteFileEvent(loop, context->channel->fd, AE_READABLE);
            close(context->channel->fd);
            
            if (is_running && context->channel->isDatagram) 
            {
                setup_channel(context, loop, readFromHost);
            }
        }
        loge("Error: %d\n", errno);
        return;
    }
    rx_msg_packet.buf = rx_buff;
    rx_msg_packet.length = ret;
    rx_msg_packet.channel = context->channel;

    if (context->channel->isDatagram) {
        context->protocol->parse_packet_datagram(context->protocol, &rx_msg_packet);
    }else {
        context->protocol->parse_packet_stream(context->protocol, &rx_msg_packet);
    }
}

void acceptTcpHandler(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int client_fd;
    channel_context_t * context = (channel_context_t *) clientdata;

    // remove the exist tcp client.
    if (context->channel->fd > 0) 
    {
        aeDeleteFileEvent(loop, context->channel->fd, AE_READABLE);
        close(context->channel->fd);
    }
	// create client socket
	client_fd = anetUnixAccept(NULL, fd);
    logi("Accepted !!\n");
    
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
    channel_context_t * context_head = NULL, * context_cursor = NULL;

    // create main event loop
	aeEventLoop *loop;
    loop = aeCreateEventLoop(100);
    
    for (int i =0; i < CHANNELS_NUMBER; ++i) 
    {
        channel_context_t * context = (channel_context_t*) zmalloc(sizeof(channel_context_t));
        context->channel = channels_io[i];
        context->protocol = protocol;
        if (context_cursor != NULL) {
            context_cursor->next = context;
        } else {
            context_head = context;
        }
        context_cursor = context;
        

        aeFileProc * proc;
        if (channels_io[i]->isDatagram) {
            proc = readFromHost;
        }else {
            proc = acceptTcpHandler;
        }
        if (setup_channel(context, loop, proc) < 0) 
        {
            loge("Setup channel fail, channel index: %d\n", i);
            goto error;
        }
    }

    // start main loop
	aeMain(loop);

    for (int i = 0; i < CHANNELS_NUMBER ; ++i) {
        if (!channels_io[i]->isDatagram) {
            aeDeleteFileEvent(loop,  channels_io[i]->listen_fd, AE_READABLE);
        }
        aeDeleteFileEvent(loop,  channels_io[i]->fd, AE_READABLE);
    }
    
error:
    for (int i =0; i < CHANNELS_NUMBER; ++i) {
        channels_io[i]->close();
    }

    for (context_cursor = context_head; context_cursor != NULL; ) 
    {
        channel_context_t * temp = context_cursor;
        context_cursor = context_cursor->next;
        zfree(temp);
    }
    // stop loop
    aeDeleteEventLoop(loop);
    protocol->deinit();

    return 0;
}