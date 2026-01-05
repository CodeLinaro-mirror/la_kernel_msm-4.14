/* SPDX-License-Identifier: ISC */
/*
 * NRC MCP Transmit Implementation
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

/* Linux kernel headers */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/genetlink.h>
#include <linux/ieee80211.h>

/* Common headers */
#include "nrc-wim-types.h"
#include "nrc-debug-common.h"
#include "nrc-hal-core-interface.h"
#include "nrc-hif.h"

/* Local headers */
#include "nrc-log.h"
#include "nrc-netlink-driver.h"
#include "nrc-mcp-transmit.h"
#include "nrc-mcp-init.h"
#include "mcp.h"

const char *channel_id_to_str(int channel)
{
	static const char *table[CHAN_ID_MAX + 1] = {
		"CHAN_ID_CONTROL_H2F",	  "CHAN_ID_CONTROL_F2H",
		"CHAN_ID_DATA",		  "CHAN_ID_EVENT",
		"CHAN_ID_PROTOCOL_H2F",	  "CHAN_ID_PROTOCOL_F2H",
		"CHAN_ID_PROTOCOL_EVENT", "CHAN_ID_DRIVER_H2D",
		"CHAN_ID_DRIVER_D2H",	  "CHAN_ID_UNDEFINED"};

	if (channel < CHAN_ID_CONTROL_H2F && channel >= CHAN_ID_MAX) {
		return table[CHAN_ID_MAX];
	}

	return table[channel];
}

int nrc_mcp_process_wim_request_wait(int request_id, int resp_id, int cmd,
				     struct wim_tlv *tlv)
{
	struct sk_buff *wim_resp = NULL;
	struct mcp_priv *mcp = nrc_mcp_get_device();
	struct nrc_hif_device *hdev = mcp ? mcp->hdev : NULL;
	int ret = 0;

	/* Create WIM SKB from netlink attribute data */
	struct sk_buff *wim_skb =
		nrc_hal_ops_wim_alloc_skb(cmd, tlv_len(tlv->l));
	if (!wim_skb) {
		LOG_ERR("MCP: Failed to allocate WIM SKB\n");
		return -ENOMEM;
	}

	nrc_hal_ops_wim_skb_add_tlv(wim_skb, tlv->t, tlv->l, tlv->v);

	LOG_WIM("Sending WIM request for channel %s, len=%d",
		channel_id_to_str(request_id), wim_skb->len);

	/* Send WIM request via MCP TX path and wait for response */
	if (nrc_hal_ops_wim_request(wim_skb, 0, WIM_RESP_TIMEOUT, true,
				    &wim_resp))
		wim_resp = NULL;

	if (wim_resp) {
		struct wim_hdr *wim;
		struct wim_tlv *tlv_resp;

		wim = (struct wim_hdr *)wim_resp->data;
		LOG_WIM("Received WIM response for channel %s, cmd=%d, len=%d",
			channel_id_to_str(resp_id), wim->resp, wim_resp->len);

		skb_pull(wim_resp, sizeof(struct wim_hdr));

		tlv_resp = (struct wim_tlv *)wim_resp->data;
		// LOG_WIM("MCP: WIM response TLV - type=%d, len=%d, data_len=%d",
		// 	tlv_resp->t, tlv_resp->l, wim_resp->len);

		/* send_to_netlink will free the SKB */
		ret = send_to_netlink(resp_id, wim_resp, hdev, HIF_TYPE_WIM, true);
		if (ret < 0) {
			LOG_ERR("Failed to send response to netlink channel %s err %d",
				channel_id_to_str(resp_id), ret);
		}

		/* Return success if we got a response, even if send_to_netlink failed */
		return 0;
	} else {
		LOG_ERR("WIM request timeout or failed for channel %s",
			channel_id_to_str(request_id));

		return -ETIMEDOUT;
	}
}

/**
 * nrc_mcp_xmit_hif_frame - Transmit MCP frame via HIF
 * @subtype: HIF frame subtype
 * @data: Frame data payload
 * @len: Length of frame data
 * @hif_header_included: true if HIF header is already included in data, false otherwise
 */
int nrc_mcp_xmit_hif_frame(int subtype, const u8 *data, u32 len,
			   bool hif_header_included)
{
	struct sk_buff *skb;
	struct nrc_hif_device *hdev = nrc_hal_core_get_hdev();
	int ret;

	if (!hdev) {
		LOG_ERR("MCP: HIF device not available\n");
		return -ENODEV;
	}

	/* Optimized path: Allocate SKB for data only */
	skb = dev_alloc_skb(len);
	if (!skb) {
		LOG_ERR("MCP: Failed to allocate SKB for HIF frame\n");
		return -ENOMEM;
	}

	/* Track FRAME SKB allocation (TX path) */
	NRC_SKB_TRACK_ALLOC(hdev, skb, HIF_TYPE_FRAME, false, false);

	/* Copy frame data (HIF header added by HAL) */
	skb_put_data(skb, data, len);

	LOG_INFO(
		"MCP: TX via MCP queue, subtype=%d, len=%u, hif_header_included=%d",
		subtype, len, hif_header_included);

	/* Transmit via MCP-specific HAL ops */
	ret = nrc_hal_ops_xmit_mcp_frame(subtype, skb, hif_header_included);
	if (ret < 0) {
		LOG_ERR("MCP: Failed to transmit via MCP queue: %d\n", ret);
		/* SKB already freed by HAL on error */
		return ret;
	}

	return 0;
}
