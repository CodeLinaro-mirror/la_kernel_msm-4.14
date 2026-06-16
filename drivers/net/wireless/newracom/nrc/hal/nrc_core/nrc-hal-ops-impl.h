/*
 *
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * NRC HAL Operations Implementation Header
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

#ifndef _NRC_HAL_OPS_IMPL_H_
#define _NRC_HAL_OPS_IMPL_H_

#include "nrc.h"

/* ===========================================================================
 * HAL Operations Structure Access
 * =========================================================================== */

/**
 * nrc_hal_get_default_ops - Get default HAL operations structure
 *
 * Returns: Pointer to default HAL operations
 */
struct nrc_hal_ops *nrc_hal_get_default_ops(void);

#endif /* _NRC_HAL_OPS_IMPL_H_ */
