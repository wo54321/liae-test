#include "../src/anet.h"
#include "channel_io.h"

/**
* 	Rcdaemon interact to other through the below path,
*/
#define UNIX_SOCK_AGENT_LCM_PATH "/var/run/usb_proxy_svr_1108"
#define UNIX_SOCK_AGENT_USB_PATH "/var/run/usb_proxy_svr_1109"
#define UNIX_SOCK_AGENT_INTERNAL_PATH "/var/run/usb_proxy_svr_1110"
#define WIFI_UDP_SERVER_ADDR "192.168.42.1"
#define WIFI_UDP_SERVER_PORT 14540
#define WIFI_UDP_CLIENT_PORT 11178

static int32_t unix_sock_open(channel_io_t *channel, char *path)
{
    char error[256];

    channel->fd = -1;
    for (int i = 0; i < MAX_CHANNELS_NO; ++i)
    {
        channel->post_chanel[i] = NULL;
    }

    unlink(path);
    channel->listen_fd = anetUnixServer(error, path, 0, 5);
    if (channel->listen_fd < 0)
    {
        printf("error %s\n", error);
    }
    return channel->listen_fd;
}

static int32_t udp_open(channel_io_t *channel, int32_t client_port, char *server_addr, int32_t server_port)
{
    char error[256];

    channel->fd = -1;
    channel->listen_fd = -1;
    for (int i = 0; i < MAX_CHANNELS_NO; ++i)
    {
        channel->post_chanel[i] = NULL;
    }

    channel->fd = anetUdpConnect(error, client_port, server_addr, server_port);

    if (channel->fd < 0)
    {
        printf("error %s\n", error);
    }
    return channel->fd;
}

static int32_t usb_open()
{
    return unix_sock_open(&usb_channel, UNIX_SOCK_AGENT_USB_PATH);
}

static int32_t wifi_open()
{
    return udp_open(&wifi_channel, WIFI_UDP_CLIENT_PORT, WIFI_UDP_SERVER_ADDR, WIFI_UDP_SERVER_PORT);
}

static int32_t internal_open()
{
    return unix_sock_open(&internal_channel, UNIX_SOCK_AGENT_INTERNAL_PATH);
}

static int32_t probe(struct channel_io *self, int32_t channel_num, struct channel_io *channels[])
{
    msg_channel forward_channel = MSG_CHANNEL_NONE;
    switch (self->channel)
    {
    case MSG_CHANNEL_WIFI:
        forward_channel = MSG_CHANNEL_USB;
        break;
    case MSG_CHANNEL_INTERNAL:
        forward_channel = MSG_CHANNEL_NONE;
        break;
    case MSG_CHANNEL_USB:
        forward_channel = MSG_CHANNEL_WIFI;
        break;
    default:
        return -1;
    }

    for (int32_t i = 0; i < channel_num; ++i)
    {
        int32_t posting_channel_index = 0;
        if (channels[i] == NULL)
        {
            continue;
        }

        if (channels[i]->channel == self->channel)
        {
            continue;
        }

        if (channels[i]->channel == forward_channel)
        {
            self->forwad = channels[i]->send;
        }

        if (posting_channel_index++ < MAX_CHANNELS_NO)
        {
            self->post_chanel[posting_channel_index] = channels[i]->send;
        }
    }

    return 0;
}

static int32_t usb_write(uint8_t *buf, int32_t length)
{
    return anetWrite(usb_channel.fd, buf, length);
}

static int32_t wifi_write(uint8_t *buf, int32_t length)
{
    return anetWrite(wifi_channel.fd, buf, length);
}

static int32_t internal_write(uint8_t *buf, int32_t length)
{
    return anetWrite(internal_channel.fd, buf, length);
}

static void common_broadcast(channel_io_t *channel, uint8_t *buf, int32_t length)
{
    for (int i = 0; i < MAX_CHANNELS_NO; ++i)
    {
        if (channel->post_chanel[i] != NULL)
        {
            channel->post_chanel[i](buf, length);
        }
    }
}

static void usb_broadcast(uint8_t *buf, int32_t length)
{
    common_broadcast(&usb_channel, buf, length);
}

static void wifi_broadcast(uint8_t *buf, int32_t length)
{
    common_broadcast(&wifi_channel, buf, length);
}

static void internal_broadcast(uint8_t *buf, int32_t length)
{
    common_broadcast(&internal_channel, buf, length);
}

static void common_close(channel_io_t *channel)
{
    if (channel->listen_fd > 0)
    {
        close(channel->listen_fd);
        channel->listen_fd = -1;
    }
    if (channel->fd > 0)
    {
        close(channel->fd);
        channel->fd = -1;
    }
}

static void usb_close()
{
    common_close(&usb_channel);
}

static void wifi_close()
{
    common_close(&wifi_channel);
}

static void internal_close()
{
    common_close(&internal_channel);
}

channel_io_t usb_channel = {
    .fd = -1,
    .listen_fd = -1,
    .isDatagram = false,
    .post_chanel = {
        NULL,
    },
    .channel = MSG_CHANNEL_USB,

    .open = usb_open,
    .probe = probe,
    .send = usb_write,
    .forwad = NULL,
    .broadcast = usb_broadcast,
    .close = usb_close};

channel_io_t wifi_channel = {
    .fd = -1,
    .listen_fd = -1,
    .isDatagram = true,
    .post_chanel = {
        NULL,
    },
    .channel = MSG_CHANNEL_WIFI,

    .open = wifi_open,
    .probe = probe,
    .send = wifi_write,
    .forwad = NULL,
    .broadcast = wifi_broadcast,
    .close = wifi_close};

channel_io_t internal_channel = {
    .fd = -1,
    .listen_fd = -1,
    .isDatagram = false,
    .post_chanel = {
        NULL,
    },
    .channel = MSG_CHANNEL_INTERNAL,

    .open = internal_open,
    .probe = probe,
    .send = internal_write,
    .forwad = NULL,
    .broadcast = internal_broadcast,
    .close = internal_close};
