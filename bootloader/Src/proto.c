/*
 * proto.c
 *
 *  Created on: Sep 13, 2026
 *      Author: utkarsh
 */


#include "stm32f446xx.h"
#include "proto.h"
#include "flash.h"
#include "uart.h"
#include "crc.h"

static uint8_t frame_buf[PROTO_MAX_PAYLOAD + 8U];
#define APP_SLOT_BASE    0x08020000UL
#define APP_SLOT_SECTOR  5U
#define APP_SLOT_SIZE    0x00020000UL

static void handle_erase(const uint8_t *payload, uint16_t plen)
{
    uint8_t st;

    if (plen != 1U) {
        proto_send_nak(NAK_BAD_LEN);
        return;
    }

    if (payload[0] != APP_SLOT_SECTOR) {
        proto_send_nak(NAK_FLASH_ERR);
        return;
    }

    flash_unlock();
    st = flash_erase_sector(APP_SLOT_SECTOR);
    flash_lock();

    if (st != FLASH_OK) {
        proto_send_nak(NAK_FLASH_ERR);
        return;
    }

    proto_send_ack(0, 0U);
}

static void handle_write(const uint8_t *payload, uint16_t plen)
{
    uint32_t offset;
    uint16_t dlen;
    uint8_t  st;

    if (plen < 5U || ((plen - 4U) & 3U) != 0U) {
        proto_send_nak(NAK_BAD_LEN);
        return;
    }

    offset = (uint32_t)payload[0]
           | ((uint32_t)payload[1] << 8)
           | ((uint32_t)payload[2] << 16)
           | ((uint32_t)payload[3] << 24);

    dlen = plen - 4U;

    if ((offset + dlen) > APP_SLOT_SIZE) {
        proto_send_nak(NAK_BAD_LEN);
        return;
    }

    flash_unlock();
    st = flash_write(APP_SLOT_BASE + offset, &payload[4], dlen);
    flash_lock();

    if (st != FLASH_OK) {
        proto_send_nak(NAK_FLASH_ERR);
        return;
    }

    proto_send_ack(0, 0U);
}

static void handle_verify(const uint8_t *payload, uint16_t plen)
{
    uint32_t len;
    uint32_t want;
    uint32_t got;

    if (plen != 8U) {
        proto_send_nak(NAK_BAD_LEN);
        return;
    }

    len = (uint32_t)payload[0]
        | ((uint32_t)payload[1] << 8)
        | ((uint32_t)payload[2] << 16)
        | ((uint32_t)payload[3] << 24);

    want = (uint32_t)payload[4]
         | ((uint32_t)payload[5] << 8)
         | ((uint32_t)payload[6] << 16)
         | ((uint32_t)payload[7] << 24);

    if (len == 0U || len > APP_SLOT_SIZE) {
        proto_send_nak(NAK_BAD_LEN);
        return;
    }

    got = crc32_compute((const uint8_t *)APP_SLOT_BASE, len);

    if (got != want) {
        proto_send_nak(NAK_VERIFY_FAIL);
        return;
    }

    proto_send_ack(0, 0U);
}

uint8_t proto_recv_frame(uint8_t *cmd, uint8_t *payload, uint16_t *plen)
{
    uint16_t len;
    uint32_t i;
    uint32_t rx_crc;
    uint32_t calc_crc;

    /* hunt for start of frame, discard anything else */
    while (uart_recv_byte() != PROTO_SOF) { }

    frame_buf[0] = PROTO_SOF;
    frame_buf[1] = uart_recv_byte();       /* len low  */
    frame_buf[2] = uart_recv_byte();       /* len high */

    len = (uint16_t)frame_buf[1] | ((uint16_t)frame_buf[2] << 8);

    if (len == 0U || len > (PROTO_MAX_PAYLOAD + 1U)) {
        return NAK_BAD_LEN;
    }

    /* len covers cmd + payload */
    for (i = 0U; i < len; i++) {
        frame_buf[3U + i] = uart_recv_byte();
    }

    rx_crc  = (uint32_t)uart_recv_byte();
    rx_crc |= (uint32_t)uart_recv_byte() << 8;
    rx_crc |= (uint32_t)uart_recv_byte() << 16;
    rx_crc |= (uint32_t)uart_recv_byte() << 24;

    calc_crc = crc32_compute(frame_buf, 3U + len);

    if (calc_crc != rx_crc) {
        return NAK_BAD_CRC;
    }

    *cmd  = frame_buf[3];
    *plen = len - 1U;

    for (i = 0U; i < *plen; i++) {
        payload[i] = frame_buf[4U + i];
    }

    return 0U;
}

void proto_send_frame(uint8_t cmd, const uint8_t *payload, uint16_t plen)
{
    uint16_t len = plen + 1U;
    uint32_t crc;
    uint8_t  hdr[4];

    hdr[0] = PROTO_SOF;
    hdr[1] = (uint8_t)(len & 0xFFU);
    hdr[2] = (uint8_t)((len >> 8) & 0xFFU);
    hdr[3] = cmd;

    crc32_reset();
    crc32_update(hdr, 4U);
    if (plen > 0U) {
        crc32_update(payload, plen);
    }
    crc = crc32_value();

    uart_send(hdr, 4U);
    if (plen > 0U) {
        uart_send(payload, plen);
    }

    uart_send_byte((uint8_t)(crc & 0xFFU));
    uart_send_byte((uint8_t)((crc >> 8)  & 0xFFU));
    uart_send_byte((uint8_t)((crc >> 16) & 0xFFU));
    uart_send_byte((uint8_t)((crc >> 24) & 0xFFU));
}

void proto_send_ack(const uint8_t *payload, uint16_t plen)
{
    proto_send_frame(RESP_ACK, payload, plen);
}

void proto_send_nak(uint8_t reason)
{
    proto_send_frame(RESP_NAK, &reason, 1U);
}

static void handle_get_info(void)
{
    uint8_t info[4];

    info[0] = BL_VERSION_MAJOR;
    info[1] = BL_VERSION_MINOR;
    info[2] = 0U;                  /* active slot, hardcoded for now */
    info[3] = 0U;                  /* reserved */

    proto_send_ack(info, 4U);
}

void proto_handle(void)
{
    uint8_t  payload[PROTO_MAX_PAYLOAD];
    uint8_t  cmd;
    uint16_t plen;
    uint8_t  err;

    err = proto_recv_frame(&cmd, payload, &plen);

    if (err != 0U) {
        proto_send_nak(err);
        return;
    }

    switch (cmd) {
    case CMD_GET_INFO:
        handle_get_info();
        break;

    case CMD_ERASE:
        handle_erase(payload, plen);
        break;

    case CMD_WRITE:
        handle_write(payload, plen);
        break;

    case CMD_VERIFY:
        handle_verify(payload, plen);
        break;

    default:
        proto_send_nak(NAK_UNKNOWN_CMD);
        break;
    }
}
