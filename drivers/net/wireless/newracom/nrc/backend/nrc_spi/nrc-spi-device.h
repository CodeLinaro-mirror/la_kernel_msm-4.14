/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * NRC SPI Device Registration Interface
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

#ifndef _NRC_SPI_DEVICE_H
#define _NRC_SPI_DEVICE_H

/* Linux kernel headers */
#include <linux/spi/spi.h>

/* Forward declarations */
struct nrc_spi_device_info;
struct nrc_hif_ops;

/* SPI device registration functions - exported via EXPORT_SYMBOL */
struct nrc_spi_device_info *nrc_spi_get_device_info(void);
bool nrc_spi_is_device_available(void);

/* Register/Unregister SPI device for HAL layer */
int nrc_spi_register_device(struct spi_device *spi, void *priv, struct nrc_hif_ops *ops);
void nrc_spi_unregister_device(struct spi_device *spi);

#endif /* _NRC_SPI_DEVICE_H */