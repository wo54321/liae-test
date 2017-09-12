#ifndef __CHANNEL_IO__
#define __CHANNEL_IO__

#include <stdint.h>
#include <stdbool.h>

#ifdef __android__
#define LOG_TAG "channe_io"
#include <cutils/log.h>
#define logi(fmt,  arg  ...) ALOGI(fmt, ##arg)
#define loge(fmt,  arg  ...) ALOGE(fmt, ##arg)
#elif defined(__linux__)
#include <stdio.h>
#define logi(fmt,  arg  ...) printf("Info: "fmt, ##arg)
#define loge(fmt,  arg  ...) printf("Error: "fmt, ##arg)
#endif

#define MAX_CHANNELS_NO 3

typedef enum msg_channel {
    MSG_CHANNEL_NONE = 0,
    MSG_CHANNEL_WIFI,
    MSG_CHANNEL_USB,
    MSG_CHANNEL_INTERNAL,
} msg_channel;

typedef struct channel_io
{
    msg_channel channel;
    int32_t fd;
    int32_t listen_fd;
    bool isDatagram;
    int32_t (*post_chanel[MAX_CHANNELS_NO])(uint8_t *buf, int32_t length);

    int32_t (*open)();
    int32_t (*probe)(struct channel_io *self, int32_t channel_num, struct channel_io *channels[]);
    int32_t (*send)(uint8_t *buf, int32_t length);
    int32_t (*forwad)(uint8_t *buf, int32_t length);
    void (*broadcast)(uint8_t *buf, int32_t length);
    void (*close)();
} channel_io_t;

extern channel_io_t * channels_io[];
extern const uint32_t CHANNELS_NUMBER;
#endif