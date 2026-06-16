/*
 *
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * NRC WLAN Netlink Interface
 *
 * This file defines the netlink communication interface for WLAN operations,
 * including WFA CAPI commands, test operations, and APF (Android Packet Filter).
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

#ifndef _NRC_NETLINK_H_
#define _NRC_NETLINK_H_

/* Forward declarations */
struct nrc;

/* ========================================================================
 * Netlink Operation Commands
 * ======================================================================== */

/**
 * enum nrc_nl_op_cmds - Netlink operation command definitions
 *
 * NOTE: These numeric definitions must match nrcnetlink.py
 * Must match nl cmd, attr as defined by the driver.
 */
enum nrc_nl_op_cmds {
	/* WFA CAPI (Wi-Fi Alliance Certification API) Commands */
	NL_WFA_CAPI_STA_GET_INFO = 0, /* Get STA information */
	NL_WFA_CAPI_STA_SET_11N = 1, /* Set 802.11n parameters */
	NL_WFA_CAPI_SEND_ADDBA = 2, /* Send ADDBA request */
	NL_WFA_CAPI_SEND_DELBA = 3, /* Send DELBA request */
	NL_WFA_CAPI_LISTEN_INTERVAL = 11, /* Configure listen interval */
	NL_WFA_CAPI_BSS_MAX_IDLE = 12, /* Configure BSS max idle */
	NL_WFA_CAPI_BSS_MAX_IDLE_OFFSET =
		13, /* Configure BSS max idle offset */

	/* Test and Debug Commands */
	NL_TEST_MMIC_FAILURE = 4, /* Test MIC failure */
	NL_MIC_SCAN = 14, /* MIC scan operation */
	NL_AUTO_BA_TOGGLE = 20, /* Toggle automatic block ACK */

	/* Shell and Management Commands */
	NL_SHELL_RUN = 5, /* Run shell command */
	NL_SHELL_RUN_SIMPLE = 8, /* Run simple shell command */
	NL_SHELL_RUN_RAW = 19, /* Run raw shell command */
	NL_CMD_RECOVERY = 15, /* Trigger recovery */

	/* Frame Injection and IE Management */
	NL_MGMT_FRAME_INJECTION = 6, /* Inject management frame */
	NL_FRAME_INJECTION = 16, /* General frame injection */
	NL_SET_IE = 17, /* Set Information Element */
	NL_SET_SAE = 18, /* Set SAE parameters */

	/* Logging and Application Interface */
	NL_CMD_LOG_EVENT = 7, /* Log event */
	NL_CLI_APP_GET_INFO = 10, /* CLI application get info */
	NL_CLI_APP_DRIVER = 21, /* CLI application driver interface */

	/* HaLow (802.11ah) Specific */
	NL_HALOW_SET_DUT = 9, /* Set HaLow DUT parameters */

	/* APF (Android Packet Filter) Commands */
	NL_APF_SET_ENABLE = 22, /* Enable/disable APF */
	NL_APF_GET_ENABLE = 23, /* Get APF enable status */
	NL_APF_GET_CAPABILITIES = 24, /* Get APF capabilities */
	NL_APF_SET_PACKET_FILTER = 25, /* Set packet filter */
	NL_APF_GET_PACKET_FILTER = 26, /* Get packet filter */
};

/* ========================================================================
 * Netlink Operation Attributes
 * ======================================================================== */

/**
 * enum nrc_nl_op_attrs - Netlink operation attribute definitions
 *
 * These attributes are used to pass parameters and data in netlink messages.
 */
enum nrc_nl_op_attrs {
	/* WFA CAPI Attributes */
	NL_WFA_CAPI_INTF_ID = 0, /* Interface ID */
	NL_WFA_CAPI_PARAM_NAME = 1, /* Parameter name */
	NL_WFA_CAPI_PARAM_STR_VAL = 2, /* String parameter value */
	NL_WFA_CAPI_PARAM_DESTADDR = 3, /* Destination address */
	NL_WFA_CAPI_PARAM_MCS = 4, /* MCS (Modulation Coding Scheme) */
	NL_WFA_CAPI_PARAM_TID = 5, /* Traffic Identifier */
	NL_WFA_CAPI_PARAM_SMPS = 6, /* Spatial Multiplexing Power Save */
	NL_WFA_CAPI_PARAM_STBC = 7, /* Space-Time Block Coding */
	NL_WFA_CAPI_PARAM_VENDOR1 = 8, /* Vendor specific parameter 1 */
	NL_WFA_CAPI_PARAM_VENDOR2 = 9, /* Vendor specific parameter 2 */
	NL_WFA_CAPI_PARAM_VENDOR3 = 10, /* Vendor specific parameter 3 */
	NL_WFA_CAPI_PARAM_RESPONSE = 11, /* Response data */
	NL_WFA_CAPI_PARAM_VIF_ID = 20, /* Virtual Interface ID */
	NL_WFA_CAPI_PARAM_BSS_MAX_IDLE = 21, /* BSS maximum idle period */
	NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET = 22, /* BSS max idle offset */
	NL_WFA_CAPI_PARAM_LISTEN_INTERVAL = 23, /* Listen interval */

	/* Shell Command Attributes */
	NL_SHELL_RUN_CMD = 12, /* Shell command */
	NL_SHELL_RUN_CMD_RESP = 13, /* Shell command response */
	NL_SHELL_RUN_CMD_RAW = 35, /* Raw shell command */
	NL_SHELL_RUN_CMD_RESP_RAW = 36, /* Raw shell command response */

	/* Frame Injection Attributes */
	NL_MGMT_FRAME_INJECTION_STYPE = 14, /* Management frame subtype */
	NL_FRAME_INJECTION_BUFFER = 28, /* Frame injection buffer */

	/* Logging Attributes */
	NL_CMD_LOG_MSG = 15, /* Log message */
	NL_CMD_LOG_TYPE = 16, /* Log type */

	/* HaLow Attributes */
	NL_HALOW_PARAM_NAME = 17, /* HaLow parameter name */
	NL_HALOW_PARAM_STR_VAL = 18, /* HaLow string value */
	NL_HALOW_RESPONSE = 19, /* HaLow response */

	/* MIC Scan Attributes */
	NL_MIC_SCAN_CHANNEL_START = 24, /* MIC scan start channel */
	NL_MIC_SCAN_CHANNEL_END = 25, /* MIC scan end channel */
	NL_MIC_SCAN_CHANNEL_BITMAP = 26, /* MIC scan channel bitmap */

	/* Recovery Attributes */
	NL_CMD_RECOVERY_MSG = 27, /* Recovery message */

	/* IE Management Attributes */
	NL_SET_IE_EID = 29, /* IE Element ID */
	NL_SET_IE_LENGTH = 30, /* IE Length */
	NL_SET_IE_DATA = 31, /* IE Data */

	/* SAE Attributes */
	NL_SET_SAE_EID = 32, /* SAE Element ID */
	NL_SET_SAE_LENGTH = 33, /* SAE Length */
	NL_SET_SAE_DATA = 34, /* SAE Data */

	/* Block ACK Attributes */
	NL_AUTO_BA_ON = 37, /* Auto Block ACK enable */

	/* CLI Application Attributes */
	NL_CLI_APP_DRIVER_CMD = 38, /* CLI driver command */
	NL_CLI_APP_DRIVER_CMD_RESP = 39, /* CLI driver command response */

	/* APF (Android Packet Filter) Attributes */
	NL_APF_PARAM_ENABLE = 40, /* APF enable parameter */
	NL_APF_PARAM_CAP = 41, /* APF capability parameter */
	NL_APF_PARAM_FILTER = 42, /* APF filter parameter */

	/* Padding and Limits */
	NL_NRC_PAD, /* Padding for 64-bit alignment */
	NL_WFA_CAPI_ATTR_LAST, /* Last attribute marker */
	MAX_NL_WFA_CAPI_ATTR =
		NL_WFA_CAPI_ATTR_LAST - 1, /* Maximum attribute */
};

/* ========================================================================
 * Response Constants and Multicast Groups
 * ======================================================================== */

/* Standard response strings for WFA CAPI operations */
#define NL_WFA_CAPI_RESP_OK ("COMPLETE") /* Operation completed successfully */
#define NL_WFA_CAPI_RESP_ERR ("ERROR") /* Operation failed */
#define NL_WFA_CAPI_RESP_NONE ("NONE") /* No response available */

/* Standard response strings for HaLow operations */
#define NL_HALOW_RESP_OK ("OK") /* HaLow operation successful */
#define NL_HALOW_RESP_ERR ("ERROR") /* HaLow operation failed */
#define NL_HALOW_RESP_NOT_SUPP \
	("Not supported") /* HaLow feature not supported */

/**
 * enum nrc_nl_multicast_grp - Netlink multicast group definitions
 *
 * These groups are used for broadcasting events and responses to multiple
 * netlink subscribers.
 */
enum nrc_nl_multicast_grp {
	NL_MCGRP_WFA_CAPI_RESPONSE, /* WFA CAPI response multicast group */
	NL_MCGRP_NRC_LOG, /* NRC logging multicast group */
	NL_MCGRP_LAST, /* Last group marker */
};

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

/**
 * nrc_netlink_init - Initialize NRC netlink interface
 * @nw: NRC wireless device structure
 *
 * Initializes the netlink interface for communication with userspace
 * applications and tools.
 *
 * Return: 0 on success, negative error code on failure
 */
int nrc_netlink_init(struct nrc *nw);

/**
 * nrc_netlink_exit - Cleanup NRC netlink interface
 *
 * Performs cleanup and releases resources allocated by nrc_netlink_init().
 */
void nrc_netlink_exit(void);

/**
 * nrc_netlink_rx - Handle received netlink message
 * @nw: NRC wireless device structure
 * @skb: Socket buffer containing the netlink message
 * @subtype: Message subtype
 *
 * Processes incoming netlink messages and dispatches them to appropriate
 * handlers based on the subtype.
 *
 * Return: 0 on success, negative error code on failure
 */
int nrc_netlink_rx(struct nrc *nw, struct sk_buff *skb, u8 subtype);

/**
 * nrc_netlink_trigger_recovery - Trigger system recovery via netlink
 * @nw: NRC wireless device structure
 *
 * Sends a recovery trigger message through the netlink interface to
 * initiate system recovery procedures.
 *
 * Return: 0 on success, negative error code on failure
 */
int nrc_netlink_trigger_recovery(struct nrc *nw);

#endif /* _NRC_NETLINK_H_ */
