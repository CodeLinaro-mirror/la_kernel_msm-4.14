/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * NRC HAL Operations Implementation
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <net/mac80211.h>

#include "nrc-hal-ops-impl.h"
#include "nrc-hal-core-interface.h"
#include "nrc-backend-hif-interface.h"
#include "nrc-init.h"
#include "wim.h"
#include "nrc-bd.h"
#include "nrc-debug.h"
#include "hif.h"
#include "nrc-ps.h"
#include "nrc-tx.h"

/* ===========================================================================
 * Network Device Operations Implementation
 * =========================================================================== */

/**
 * nrc_hal_nw_start_impl - Start network device implementation
 */
static int nrc_hal_nw_start_impl(void)
{
	struct nrc_hif_device *hdev = nrc_hal_core_get_hdev();
	if (!hdev) {
		ERR_HIF("Invalid HIF device or ops");
		return -EINVAL;
	}
	if (NRC_PARAM_FLASH_FW(hdev)) {
		return nrc_nw_start_fusing();
	} else {
		return nrc_nw_start();
	}
}

static struct wim_bd_param *nrc_hal_bd_get_tx_pwr_impl(u8 *cc)
{
#if defined(CONFIG_SUPPORT_BD)
	struct nrc_hif_device *hdev = nrc_hal_core_get_hdev();
	return nrc_read_bd_tx_pwr(hdev, cc);
#else
	return NULL;
#endif
}

static struct bd_supp_param *nrc_hal_bd_get_supp_ch_list_impl(void)
{
#if defined(CONFIG_SUPPORT_BD)
	return &g_supp_ch_list;
#else
	return NULL;
#endif
}

/* ===========================================================================
 * HAL Operations Structure
 *
 * Note: WLAN-specific WIM functions (sta_type, scan, key, ampdu, tsf, apf)
 * have been moved to frontend/nrc_wlan/nrc-wim-wlan.c.
 * They use core WIM primitives directly (wim_alloc_skb, wim_skb_add_tlv, wim_request).
 * =========================================================================== */

static struct nrc_hal_ops default_hal_ops = {
	.nw_start = nrc_hal_nw_start_impl,
	.nw_stop = nrc_nw_stop,
	.xmit_wlan_frame = nrc_xmit_wlan_frame,
	.xmit_mcp_frame = nrc_xmit_mcp_frame,
	.xmit_injected_frame = nrc_xmit_injected_frame,
	/* Core WIM utility functions */
	.wim_request = nrc_wim_request,
	.wim_alloc_skb = nrc_wim_alloc_skb,
	.wim_alloc_skb_vif = nrc_wim_alloc_skb_vif,
	.wim_skb_add_tlv = nrc_wim_skb_add_tlv,
	/* BD (Board Data) operations */
	.bd_get_tx_pwr = nrc_hal_bd_get_tx_pwr_impl,
	.bd_get_supp_ch_list = nrc_hal_bd_get_supp_ch_list_impl,
	/* TX operations */
	.tx_cleanup_queues = nrc_tx_cleanup_queues,
	/* Power Save Operations - HAL Master */
	.ps_request_sleep = nrc_hal_ps_request_sleep,
	.ps_request_wake = nrc_hal_ps_request_wake,
};

/**
 * nrc_hal_get_default_ops - Get default HAL operations structure
 */
struct nrc_hal_ops *nrc_hal_get_default_ops(void)
{
	return &default_hal_ops;
}
