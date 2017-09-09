#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "../src/ae.h"
#include "../src/anet.h"
#include "../src/zmalloc.h"

#include "common/mavlink.h"

#define UNIX_SOCK_TCP_TEST "/Users/liangkui/Projects/c/liae-test/usb_proxy_svr_tcp_test"
#define RECEIVE_BUFFER_SIZE 4096
#define MAVLINKE_CHANNEL_INDEX 0

mavlink_message_t rx_msg;
mavlink_status_t rx_status;

typedef struct buffer_point
{
	uint8_t *buffer;
	uint32_t length;
} buffer_point_t;

int32_t counter = -1;

void init_mavlink()
{
	int chan = MAVLINKE_CHANNEL_INDEX;
	mavlink_reset_channel_status(chan);
	rx_status.msg_received = MAVLINK_FRAMING_INCOMPLETE;
	rx_status.parse_state = MAVLINK_PARSE_STATE_IDLE;
	memset((void *)&rx_msg, 0, sizeof(rx_msg));
}

uint16_t encode_rc_channel(uint8_t chan, mavlink_rc_channels_t *rc_channels, uint8_t *sendbuffer)
{
	mavlink_message_t msg;
	memset((void *)&msg, 0, sizeof(msg));
	mavlink_msg_rc_channels_encode_chan(1, 1, chan, &msg, rc_channels);
	return mavlink_msg_to_send_buffer(sendbuffer, &msg);
}

void receive_rc_channel(buffer_point_t *buffer_p, aeEventLoop *loop, int fd)
{
	uint32_t i = 0;
	while (buffer_p->length > i)
	{
		uint8_t byte = buffer_p->buffer[i];
		if (mavlink_parse_char(MAVLINKE_CHANNEL_INDEX, byte, &rx_msg, &rx_status))
		{
			// printf("Received message with ID %d, sequence: %d from component %d of system %d\n",
			//        rx_msg.msgid, rx_msg.seq, rx_msg.compid, rx_msg.sysid);
			if (rx_msg.msgid == MAVLINK_MSG_ID_RC_CHANNELS)
			{
				mavlink_rc_channels_t rc_channels;
				mavlink_msg_rc_channels_decode(&rx_msg, &rc_channels);

				if (counter-- == 0)
				{
					aeDeleteFileEvent(loop, fd, AE_READABLE);
					aeStop(loop);
					printf("Finished.\n");
				}
				int buffer_size = 1024;
				uint8_t *buffer = (uint8_t *)zmalloc(sizeof(uint8_t) * buffer_size);
				buffer_point_t *tx_buffer_p = (buffer_point_t *)zmalloc(sizeof(buffer_point_t));
				tx_buffer_p->buffer = buffer;
				tx_buffer_p->length = encode_rc_channel(MAVLINKE_CHANNEL_INDEX, &rc_channels, buffer);
				write(fd, buffer_p->buffer, buffer_p->length);
				zfree(tx_buffer_p->buffer);
				zfree(tx_buffer_p);
			}
			rx_status.msg_received = MAVLINK_FRAMING_INCOMPLETE;
			rx_status.parse_state = MAVLINK_PARSE_STATE_IDLE;
			memset((void *)&rx_msg, 0, sizeof(rx_msg));
		}
		++i;
	}
	zfree(buffer_p->buffer);
	zfree(buffer_p);
}

void readFromHost(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
	int buffer_size = 1024;
	uint8_t *buffer = (uint8_t *)zmalloc(sizeof(uint8_t) * buffer_size);
	int size = read(fd, buffer, buffer_size);
	buffer_point_t *buffer_p = (buffer_point_t *)zmalloc(sizeof(buffer_point_t));
	buffer_p->buffer = buffer;
	buffer_p->length = size;
	receive_rc_channel(buffer_p, loop, fd);
}

void acceptUnixHandler(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
	int client_fd;
	// create client socket
	client_fd = anetUnixAccept(NULL, fd);
	printf("Accepted !!\n");

	// set client socket non-block
	anetNonBlock(NULL, client_fd);

	// regist on message callback
	int ret = aeCreateFileEvent(loop, client_fd, AE_READABLE, readFromHost, NULL);
	assert(ret != AE_ERR);

	mavlink_rc_channels_t rc_channels;
	int buffer_size = 1024;
	uint8_t *buffer = (uint8_t *)zmalloc(sizeof(uint8_t) * buffer_size);
	buffer_point_t *tx_buffer_p = (buffer_point_t *)zmalloc(sizeof(buffer_point_t));
	tx_buffer_p->buffer = buffer;
	tx_buffer_p->length = encode_rc_channel(MAVLINKE_CHANNEL_INDEX, &rc_channels, buffer);
	printf("Accept Send size: %d\n", tx_buffer_p->length);
	ret = write(client_fd, tx_buffer_p->buffer, tx_buffer_p->length);
	zfree(tx_buffer_p->buffer);
	zfree(tx_buffer_p);
}

void runServer()
{
	int ipfd;
	char error[256];
	// create server socket
	unlink(UNIX_SOCK_TCP_TEST);
	ipfd = anetUnixServer(error, UNIX_SOCK_TCP_TEST, 0, 5);

	assert(ipfd != ANET_ERR);

	// create main event loop
	aeEventLoop *loop;
	loop = aeCreateEventLoop(10);

	// regist socket connect callback
	int ret;
	ret = aeCreateFileEvent(loop, ipfd, AE_READABLE, acceptUnixHandler, NULL);

	printf("ipfd : %d\n", ipfd);
	assert(ret != AE_ERR);

	// start main loop
	aeMain(loop);

	// stop loop
	aeDeleteEventLoop(loop);
}

void runClient(int times)
{
	counter = times;
	int ipfd = anetUnixNonBlockConnect(NULL, UNIX_SOCK_TCP_TEST);
	assert(ipfd != ANET_ERR);

	// create main event loop
	aeEventLoop *loop;
	loop = aeCreateEventLoop(10);

	printf("ipfd : %d\n", ipfd);
	// regist socket connect callback
	int ret;
	ret = aeCreateFileEvent(loop, ipfd, AE_READABLE, readFromHost, NULL);
	assert(ret != AE_ERR);

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

	init_mavlink();
	if (memcmp("server", argv[1], strlen(argv[1])) == 0)
	{
		runServer();
	}
	else if (memcmp("client", argv[1], strlen(argv[1])) == 0)
	{
		int times = atoi(argv[2]);
		runClient(times);
	}
	else
	{
		printf("Arguments invalid, %s", argv[1]);
	}

	return 0;
}
