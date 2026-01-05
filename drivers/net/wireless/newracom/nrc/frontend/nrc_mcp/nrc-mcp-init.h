/* SPDX-License-Identifier: ISC */
/*
 * NRC MCP Initialization Interface
 *
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

#ifndef _NRC_MCP_INIT_H_
#define _NRC_MCP_INIT_H_

#include <linux/types.h>

/* Forward declarations */
struct mcp_priv;

/*
 * MCP Module Initialization Functions
 */

/**
 * nrc_mcp_get_device - Get MCP device instance
 *
 * Returns: MCP device instance or NULL if not initialized
 */
struct mcp_priv *nrc_mcp_get_device(void);

/**
 * nrc_mcp_is_initialized - Check if MCP module is initialized
 *
 * Returns: true if initialized, false otherwise
 */
bool nrc_mcp_is_initialized(void);

/* ===========================================================================
 * MCP Callback Functions
 * =========================================================================== */

/**
 * nrc_mcp_callback_init - Initialize MCP callback system
 *
 * Returns: 0 on success, negative error code on failure
 */
int nrc_mcp_callback_init(void);

/**
 * nrc_mcp_callback_cleanup - Cleanup MCP callback system
 */
void nrc_mcp_callback_cleanup(void);

/* ===========================================================================
 * MCP Parameter Functions
 * =========================================================================== */

/**
 * nrc_mcp_sync_params - Synchronize MCP parameters
 * @mcp: MCP device instance
 */
void nrc_mcp_sync_params(struct mcp_priv *mcp);

#endif /* _NRC_MCP_INIT_H_ */