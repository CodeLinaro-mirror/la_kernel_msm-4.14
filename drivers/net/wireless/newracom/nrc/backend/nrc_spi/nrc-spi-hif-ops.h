/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC SPI HIF Operations Header
 * Hardware Interface Function Prototypes and Operations Structure for SPI Backend
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

#ifndef _NRC_SPI_HIF_OPS_H_
#define _NRC_SPI_HIF_OPS_H_

/* Common directory headers */
#include "nrc-hif.h"

/* ===========================================================================
 * Internal SPI Module Function Prototypes
 * ===========================================================================
 * These functions are used across SPI module files (nrc-spi-hif-ops.c and
 * nrc-hif-cspi.c) but are NOT part of the HIF ops structure.
 * =========================================================================== */

/* Device reset and cleanup functions */
void spi_hif_reset_rx(struct nrc_hif_device *hdev);
void spi_hif_reset_tx(struct nrc_hif_device *hdev);
void spi_hif_close(struct nrc_hif_device *hdev);

/* IRQ management functions */
int spi_hif_status_irq(struct nrc_hif_device *hdev);
void spi_hif_clear_irq(struct nrc_hif_device *hdev);
/* Thread control function */
int spi_hif_rx_thread_resume(struct nrc_hif_device *hdev);

/* ===========================================================================
 * HIF Operations Structure
 * ===========================================================================
 *
 * HIF operation functions are implemented as static functions in
 * nrc-spi-hif-ops.c and accessed only through the spi_ops structure.
 * =========================================================================== */

/**
 * spi_ops - SPI Hardware Interface Operations Structure
 *
 * Complete HIF operations structure that maps SPI hardware interface
 * functions to the common HIF abstraction layer used by HAL.
 */
extern struct nrc_hif_ops spi_ops;

/**
 * nrc_spi_get_hif_ops - Get SPI HIF operations structure
 *
 * Returns: Pointer to SPI HIF operations structure for HAL registration
 */
struct nrc_hif_ops *nrc_spi_get_hif_ops(void);

#endif /* _NRC_SPI_HIF_OPS_H_ */
