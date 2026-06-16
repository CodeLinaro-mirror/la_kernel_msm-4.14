/*
 *
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * NRC MCP Debug Header - MCP Frontend debug interface
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

#ifndef _NRC_MCP_DEBUG_H_
#define _NRC_MCP_DEBUG_H_

#include <linux/device.h>

/* Include common debug interface */
#include "nrc-debug-common.h"

/* MCP layer aliases */
#ifndef ERR_MCP
#define ERR_MCP(fmt, ...) ERR(fmt, ##__VA_ARGS__)
#endif
#ifndef WARN_MCP
#define WARN_MCP(fmt, ...) WRN(fmt, ##__VA_ARGS__)
#endif
#ifndef INFO_MCP
#define INFO_MCP(fmt, ...) INFO(fmt, ##__VA_ARGS__)
#endif
#ifndef DBG_MCP
#define DBG_MCP(fmt, ...) DBG(CAT(BASIC), fmt, ##__VA_ARGS__)
#endif
#ifndef VBS_MCP
#define VBS_MCP(fmt, ...) VBS(CAT(BASIC), fmt, ##__VA_ARGS__)
#endif


/* Forward declarations */
struct mcp_priv;

/* Global variables */
extern unsigned long nrc_debug_mask;
extern struct device *g_dev;

/* MCP debug macros:
 * Module identity is provided by the kernel device prefix (e.g., "nrc-mcp:")
 * Use generic INFO/WARN/ERR from nrc-debug-common.h directly.
 */

/* MCP Debug Functions */
void nrc_init_debugfs(struct mcp_priv *mcp);
void nrc_exit_debugfs(struct mcp_priv *mcp);

#endif /* _NRC_MCP_DEBUG_H_ */
