/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * NRC WLAN Module Parameters Header
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

#ifndef _NRC_WLAN_PARAMS_H_
#define _NRC_WLAN_PARAMS_H_

/* ===========================================================================
 * Parameter Synchronization Functions
 * =========================================================================== */

/**
 * nrc_wlan_sync_params - Synchronize WLAN parameters to nrc structure
 * @nw: NRC device structure
 *
 * This function copies WLAN module parameters to the nrc device structure
 * so that the HAL layer can access them without direct parameter references.
 */
void nrc_wlan_sync_params(struct nrc *nw);

/* CQM initialization and cleanup handled in MAC80211 layer */

#endif /* _NRC_WLAN_PARAMS_H_ */