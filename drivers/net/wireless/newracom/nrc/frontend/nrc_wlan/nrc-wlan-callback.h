/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * NRC WLAN Callback Interface
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

#ifndef _NRC_WLAN_CALLBACK_H
#define _NRC_WLAN_CALLBACK_H

#include <linux/types.h>
#include "nrc-hal-core-callback.h"

/* WLAN Event Processing Functions */
int nrc_wlan_handle_spi_irq(struct nrc_hal_event_data *event);
int nrc_wlan_handle_rx_ready(struct nrc_hal_event_data *event);
int nrc_wlan_handle_tx_complete(struct nrc_hal_event_data *event);
int nrc_wlan_handle_fw_ready(struct nrc_hal_event_data *event);
int nrc_wlan_handle_error(struct nrc_hal_event_data *event);
int nrc_wlan_handle_reg_notifier(struct nrc_hal_event_data *event);

/* WLAN Callback System */
int nrc_wlan_callback_init(void);
void nrc_wlan_callback_cleanup(void);

#endif /* _NRC_WLAN_CALLBACK_H */