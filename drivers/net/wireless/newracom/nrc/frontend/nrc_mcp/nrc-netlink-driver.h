/* SPDX-License-Identifier: ISC */

/*
 * NRC Netlink Driver Interface
 *
 * Copyright (c) 2016-2025 Newracom, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _NRC_NETLINK_DRIVER_H_
#define _NRC_NETLINK_DRIVER_H_

#include <linux/types.h>

/* Packed attribute for structures */
#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

/* ========================================================================
 * Netlink Channel Definitions
 * ======================================================================== */

/* Channel ID Definitions - Communication paths between host and firmware */
#define CHAN_ID_CONTROL_H2F (0) /* Control: Host → Firmware */
#define CHAN_ID_CONTROL_F2H (1) /* Control: Host ← Firmware */
#define CHAN_ID_DATA (2) /* Data: Host → Firmware */
#define CHAN_ID_EVENT (3) /* Event: Host ← Firmware */
#define CHAN_ID_PROTOCOL_H2F (4) /* Protocol: Host → Firmware */
#define CHAN_ID_PROTOCOL_F2H (5) /* Protocol: Host ← Firmware */
#define CHAN_ID_PROTOCOL_EVENT (6) /* Protocol Event: Host ← Firmware */
#define CHAN_ID_DRIVER_H2D (7) /* Driver: Host → Driver */
#define CHAN_ID_DRIVER_D2H (8) /* Driver: Host ← Driver */
#define CHAN_ID_MAX (9) /* Maximum number of channels */

/* ========================================================================
 * Netlink Attribute and Operation Definitions
 * ======================================================================== */

/* Netlink Attributes */
#define ATTR_VERSION (1) /* Version attribute */
#define ATTR_REQUEST (1) /* Request attribute */
#define ATTR_RESPONSE (2) /* Response attribute */
#define ATTR_MAX (3) /* Maximum attribute value */

/* Netlink Operations */
#define OPS_INIT (0) /* Initialization operation */
#define OPS_REQUEST (1) /* Request operation */
#define OPS_MAX (2) /* Maximum operation value */

/* ========================================================================
 * TLV (Type-Length-Value) Definitions
 * ======================================================================== */

/* Control Channel TLVs - Configuration and Management */
#define TLV_TYPE_REQ_CONFIG_FREQ_BW \
	(0x1001) /* Request: Configure frequency/bandwidth */
#define TLV_TYPE_RSP_CONFIG_FREQ_BW \
	(0x1002) /* Response: Configure frequency/bandwidth */
#define TLV_TYPE_REQ_CONFIG_AP_MAC_ADDR \
	(0x1003) /* Request: Configure AP MAC address */
#define TLV_TYPE_RSP_CONFIG_AP_MAC_ADDR \
	(0x1004) /* Response: Configure AP MAC address */
#define TLV_TYPE_REQ_CONFIG_AP (0x1005) /* Request: Configure AP settings */
#define TLV_TYPE_RSP_CONFIG_AP (0x1006) /* Response: Configure AP settings */
#define TLV_TYPE_REQ_CONFIG_BEACON (0x1007) /* Request: Configure beacon */
#define TLV_TYPE_RSP_CONFIG_BEACON (0x1008) /* Response: Configure beacon */
#define TLV_TYPE_REQ_UPDATE_UTC (0x1009) /* Request: Update UTC time */
#define TLV_TYPE_RSP_UPDATE_UTC (0x100A) /* Response: Update UTC time */
#define TLV_TYPE_REQ_CONFIG_AP_DOWN (0x100B) /* Request: AP shutdown */
#define TLV_TYPE_RSP_CONFIG_AP_DOWN (0x100C) /* Response: AP shutdown */
#define TLV_TYPE_REQ_CONFIG_FOTA (0x100D) /* Request: Configure FOTA */
#define TLV_TYPE_RSP_CONFIG_FOTA (0x100E) /* Response: Configure FOTA */
#define TLV_TYPE_REQ_ENABLE_FOTA (0x100E) /* Request: Enable FOTA */
#define TLV_TYPE_RSP_ENABLE_FOTA (0x100F) /* Response: Enable FOTA */
#define TLV_TYPE_REQ_CONFIG_SBR_HW_VERSION \
	(0x1010) /* Request: Configure SBR HW version */
#define TLV_TYPE_RSP_CONFIG_SBR_HW_VERSION \
	(0x1011) /* Response: Configure SBR HW version */
#define TLV_TYPE_REQ_CONFIG_GROUP_WAKEUP \
	(0x1012) /* Request: Configure group wakeup */
#define TLV_TYPE_RSP_CONFIG_GROUP_WAKEUP \
	(0x1013) /* Response: Configure group wakeup */
#define TLV_TYPE_REQ_CONFIG_GROUP_TOP \
	(0x1014) /* Request: Configure group topology */
#define TLV_TYPE_RSP_CONFIG_GROUP_TOP \
	(0x1015) /* Response: Configure group topology */

#define TLV_TYPE_REQ_CONFIG_RESET_OPERATION (0x1038)
#define TLV_TYPE_RSP_CONFIG_RESET_OPERATION (0x1039)
#define TLV_TYPE_REQ_ENABLE_RESET_OPERATION (0x103A)
#define TLV_TYPE_RSP_ENABLE_RESET_OPERATION (0x103B)

#define TLV_TYPE_REQ_CONFIG_SET_DEBUG_MODE (0x103C)
#define TLV_TYPE_RSP_CONFIG_SET_DEBUG_MODE (0x103D)
#define TLV_TYPE_REQ_ENABLE_SET_DEBUG_MODE (0x103E)
#define TLV_TYPE_RSP_ENABLE_SET_DEBUG_MODE (0x103F)

/* Data Channel TLVs - Scheduling and Power Management */
#define TLV_TYPE_REQ_SCHEDULE_WAKEUP (0x2001) /* Request: Schedule wakeup */
#define TLV_TYPE_REQ_SCHEDULE_SUBGROUP_WAKEUP \
	(0x2002) /* Request: Schedule subgroup wakeup */
#define TLV_TYPE_REQ_SCHEDULE_DATA_WAKEUP \
	(0x2003) /* Request: Schedule data wakeup */

/* Event Channel TLVs - Status and Notifications */
#define TLV_TYPE_EVENT_NEXT_GROUP (0x4000) /* Event: Next group notification */
#define TLV_TYPE_EVENT_SCHEDULE_REPORT (0x4001) /* Event: Schedule report */
#define TLV_TYPE_EVENT_IMMEDIATE_REPORT (0x4002) /* Event: Immediate report */
#define TLV_TYPE_EVENT_KEEP_ALIVE (0x4003) /* Event: Keep alive */

/* Driver Channel TLVs - Driver Management */
#define TLV_TYPE_DRIVER_FIRMWARE (0x5001) /* Driver: Firmware management */
#define TLV_TYPE_DRIVER_SET_LOG (0x5003) /* Driver: Set logging */
#define TLV_TYPE_DRIVER_REQ_CREDIT (0x5005) /* Driver: Request credit */
#define TLV_TYPE_DRIVER_RSP_CREDIT (0x5006) /* Driver: Response credit */
#define TLV_TYPE_DRIVER_RAW_PACKET \
	(0x5007) /* Raw HIF packet with pre-built header */

typedef struct {
	uint16_t length; /* Total packet length including HIF header */
	uint8_t data[0]; /* Raw packet data (HIF header + payload) */
} PACKED driver_raw_packet_t;

/* Driver Channel TLVs - Health Check */
#define TLV_TYPE_REQ_DRIVER_PING (0x5008)
#define TLV_TYPE_RSP_DRIVER_PING (0x5009)

/* ========================================================================
 * Function Prototypes and External Interface
 * ======================================================================== */

/* Forward declarations */
struct nrc_hif_device; /* Host Interface Device structure */
struct sk_buff; /* Socket buffer structure */

/**
 * netlink_driver_init - Initialize netlink driver interface
 * @hdev: Host interface device pointer
 *
 * Initializes the netlink driver communication interface for MCP
 * (Multi-Channel Protocol) operations.
 *
 * Return: 0 on success, negative error code on failure
 */
int netlink_driver_init(struct nrc_hif_device *hdev);

/**
 * netlink_driver_exit - Cleanup netlink driver interface
 *
 * Performs cleanup and releases resources allocated by netlink_driver_init().
 */
void netlink_driver_exit(void);

/**
 * send_to_netlink - Send data to netlink socket
 * @id: Channel ID for the message
 * @skb: Socket buffer containing the data to send
 * @hdev: NRC HIF device for tracking SKB statistics
 * @hif_type: HIF type for SKB statistics tracking
 * @is_rx_path: true if SKB was allocated in RX path, false for TX path
 *
 * Transmits data through the specified netlink channel.
 * The SKB will be freed by this function.
 *
 * Return: 0 on success, negative error code on failure
 */
int send_to_netlink(int id, struct sk_buff *skb, struct nrc_hif_device *hdev,
		    u8 hif_type, bool is_rx_path);

#endif /* _NRC_NETLINK_DRIVER_H_ */
