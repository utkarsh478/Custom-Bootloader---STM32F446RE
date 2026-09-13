/*
 * proto.h
 *
 *  Created on: Sep 13, 2026
 *      Author: utkarsh
 */
#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

#define PROTO_SOF        0xA5U
#define PROTO_MAX_PAYLOAD 260U

#define CMD_GET_INFO     0x01U
#define CMD_ERASE        0x02U
#define CMD_WRITE        0x03U
#define CMD_VERIFY       0x04U
#define CMD_ACTIVATE     0x05U
#define CMD_BOOT         0x06U
#define CMD_CONFIRM      0x07U

#define RESP_ACK         0x79U
#define RESP_NAK         0x1FU

#define NAK_BAD_CRC      0x01U
#define NAK_BAD_LEN      0x02U
#define NAK_UNKNOWN_CMD  0x03U
#define NAK_FLASH_ERR    0x04U
#define NAK_VERIFY_FAIL  0x05U

#define BL_VERSION_MAJOR 1U
#define BL_VERSION_MINOR 0U

uint8_t proto_recv_frame(uint8_t *cmd, uint8_t *payload, uint16_t *plen);
void    proto_send_frame(uint8_t cmd, const uint8_t *payload, uint16_t plen);
void    proto_send_ack(const uint8_t *payload, uint16_t plen);
void    proto_send_nak(uint8_t reason);
void    proto_handle(void);

#endif
