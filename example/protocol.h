#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdlib.h>
#include "channel_io.h"

typedef struct msg_packet 
{
	channel_io_t * channel;
	uint8_t buf;
	int32_t length;
}msg_packet_t;

/**
*	Abstract protocol struct, define the common interface of protocol.
*/
typedef struct protocol
{
	/**
	*	Initial the interface.
	*/
	int32_t (* init)(void * arg);

	/**
	*	Parsing one completly packet from buffer,
	*	@return：The retrieved packet size. = 0 retrieved no packet, < 0 some error occur.
	*/
	int32_t (* parse_packet_stream)(struct protocol * protocol, msg_packet_t * packet);

	/**
	*	Parsing one completly packet from buffer,
	*	@return：The retrieved packet size. = 0 retrieved no packet, < 0 some error occur.
	*/
	int32_t (* parse_packet_datagram)(struct protocol * protocol, msg_packet_t * packet);

	/**
	*	Postting the packet to corresponding channel， Like as USB(MFI/AOA),WIFI, Zigbee.
	*	@return  the integer value of msg_channel, < 0 some error occur.
	*/
	int32_t (* disptach_packet)(struct protocol * protocol, msg_packet_t * packet);

	/**
	*	Handling the packet which is addressed to rcdaemon.
	*/
	void (* handle_rc_packet)(struct protocol * protocol, msg_packet_t * packet);

	/**
	*	Re-initial the interface, release resource.
	*/
	void (* deinit)();
} protocol_t;

extern protocol_t mavlink_protocol;
#endif
