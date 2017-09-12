#include "protocol.h"
#include "common/mavlink.h"

#define MAVLINKE_CHANNEL_INDEX 0

static mavlink_message_t rx_msg;
static mavlink_status_t rx_status;
static uint8_t tx_buff[MAX_PROTO_BUF_SIZE];

static int32_t init_mav()
{
    int chan = MAVLINKE_CHANNEL_INDEX;
	mavlink_reset_channel_status(chan);
	rx_status.msg_received = MAVLINK_FRAMING_INCOMPLETE;
	rx_status.parse_state = MAVLINK_PARSE_STATE_IDLE;
	memset((void *)&rx_msg, 0, sizeof(rx_msg));
}

static int32_t parse_packet_stream_mav(protocol_t * protocol, msg_packet_t * packet) 
{
	uint32_t i = 0;
	while (packet->length > i)
	{
		uint8_t byte = packet->buf[i];
		if (mavlink_parse_char(MAVLINKE_CHANNEL_INDEX, byte, &rx_msg, &rx_status))
		{
			logi("Received message with ID %d, sequence: %d from component %d of system %d\n",
				   rx_msg.msgid, rx_msg.seq, rx_msg.compid, rx_msg.sysid);
			
			packet->packeted_msg = (void *)&rx_msg;
			if (protocol->handle_rc_packet(protocol, packet) != 0)
			{
				protocol->disptach_packet(protocol, packet);
			} 
			rx_status.msg_received = MAVLINK_FRAMING_INCOMPLETE;
			rx_status.parse_state = MAVLINK_PARSE_STATE_IDLE;
			memset((void *)&rx_msg, 0, sizeof(rx_msg));
		}
		++i;
	}
	return i;
}

static int32_t parse_packet_datagram_mav(protocol_t * protocol, msg_packet_t * packet)
{
	uint32_t i = 0;
	while (packet->length > i)
	{
		uint8_t byte = packet->buf[i];
		if (mavlink_parse_char(MAVLINKE_CHANNEL_INDEX, byte, &rx_msg, &rx_status))
		{
			logi("Received message with ID %d, sequence: %d from component %d of system %d\n",
				   rx_msg.msgid, rx_msg.seq, rx_msg.compid, rx_msg.sysid);
			
			packet->packeted_msg = (void *)&rx_msg;
			if (protocol->handle_rc_packet(protocol, packet) != 0)
			{
				protocol->disptach_packet(protocol, packet);
			} 
			rx_status.msg_received = MAVLINK_FRAMING_INCOMPLETE;
			rx_status.parse_state = MAVLINK_PARSE_STATE_IDLE;
			memset((void *)&rx_msg, 0, sizeof(rx_msg));
		}
		++i;
	}
	return i;
}

static int32_t disptach_packet_mav(protocol_t * protocol, msg_packet_t * packet) 
{
	mavlink_message_t * forwad_msg = (mavlink_message_t *)(packet->packeted_msg);
	uint16_t size = mavlink_msg_to_send_buffer(tx_buff, forwad_msg);
    return packet->channel->forwad(tx_buff, size);
}

static int32_t handle_rc_packet_mav(protocol_t * protocol, msg_packet_t * packet)
{
    return 0;
}

static void deinit_mav() 
{

}

protocol_t mavlink_protocol = {
    .init = init_mav,
    .parse_packet_stream = parse_packet_stream_mav,
    .parse_packet_datagram = parse_packet_datagram_mav,
    .disptach_packet = disptach_packet_mav,
    .handle_rc_packet = handle_rc_packet_mav,
    .deinit = deinit_mav
};