/* SPDX-License-Identifier: ISC */

/*
 * NRC MCP Transmit Interface
 * Provides transmission functions for MCP WIM and HIF frames
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

#ifndef _NRC_MCP_TRANSMIT_H_
#define _NRC_MCP_TRANSMIT_H_

#include <linux/types.h>
#include "nrc-wim-types.h"

/* Utility functions */
const char *channel_id_to_str(int channel);

/* WIM-based transmission */
/**
 * nrc_mcp_process_wim_request_wait - Process MCP WIM request and wait for response
 * @request_id: Netlink channel ID for request
 * @resp_id: Netlink channel ID for response
 * @cmd: WIM command code
 * @tlv: TLV structure containing the WIM command payload
 *
 * Processes MCP-specific WIM requests by:
 * 1. Creating a WIM SKB from the TLV data
 * 2. Sending it to firmware via HAL layer
 * 3. Waiting for firmware response with timeout
 * 4. Forwarding response to userspace via netlink
 *
 * The function handles both successful responses and timeout/error cases,
 * sending appropriate messages back to userspace via the response channel.
 *
 * Return: 0 on success (response received and forwarded), -ETIMEDOUT on timeout,
 *         -ENOMEM on allocation failure
 */
int nrc_mcp_process_wim_request_wait(int request_id, int resp_id, int cmd,
				     struct wim_tlv *tlv);

/* HIF frame-based transmission */
/**
 * nrc_mcp_xmit_hif_frame - Transmit MCP frame via HIF
 * @subtype: HIF frame subtype (HIF_FRAME_SUB_MCP_PROTOCOL or HIF_FRAME_SUB_MCP_DATA)
 * @data: Protocol/Data to transmit
 * @len: Length of data
 * @hif_header_included: true if HIF header is already included in data, false otherwise
 *
 * Creates and transmits a HIF frame with type HIF_TYPE_FRAME and specified subtype.
 * This function allocates an SKB, optionally adds the HIF header, and transmits via HAL ops.
 *
 * Supported subtypes:
 * - HIF_FRAME_SUB_MCP_PROTOCOL: For MCP protocol messages
 * - HIF_FRAME_SUB_MCP_DATA: For MCP data packets
 *
 * Returns: 0 on success, negative error code on failure
 */
int nrc_mcp_xmit_hif_frame(int subtype, const u8 *data, u32 len,
			   bool hif_header_included);

#endif /* _NRC_MCP_TRANSMIT_H_ */
