/*
 * Copyright (c) 2016-2019 Newracom, Inc.
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
#ifndef _NRC_INIT_H_
#define _NRC_INIT_H_

#include "nrc-hif.h"

struct nrc_test_ops;
struct nrc_uart_priv;

/* HIF Device Management Functions */
int nrc_hal_fw_init(struct nrc_hif_device *hdev);
void nrc_hal_fw_cleanup(struct nrc_hif_device *hdev);
int nrc_nw_start(void);
int nrc_nw_start_fusing(void);
int nrc_nw_stop(void);

/* Parameters Management Functions */
struct nrc_params *nrc_params_alloc(void);
void nrc_params_free(struct nrc_params *params);

/* Hardware Configuration Functions */
void nrc_init_credit_queue(struct nrc_hif_device *hdev);

#endif
