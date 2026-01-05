/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC MCP Definitions - Modular Control Protocol structures
 */

#ifndef _MCP_H_
#define _MCP_H_

#include <linux/device.h>

/* Forward declarations */
struct nrc_hif_device;
struct nrc_params;
struct nrc_debug;

/**
 * struct mcp_priv - MCP (Modular Control Protocol) device structure
 * @dev: Device pointer
 * @hdev: HIF device pointer for hardware interface
 * @params: MCP-specific parameters
 * @debug: Debug configuration
 */
struct mcp_priv {
	struct device *dev;
	struct nrc_hif_device *hdev;

	/* nrc_hif_device-specific shared parameters */
	struct nrc_params *params;
	struct nrc_debug *debug;
};

#endif /* _MCP_H_ */
