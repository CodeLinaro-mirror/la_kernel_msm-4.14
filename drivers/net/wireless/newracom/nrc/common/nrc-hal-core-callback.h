/*
 *
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC HAL Callback Interface - HAL to Frontend event notification
 */

#ifndef _NRC_HAL_CORE_CALLBACK_H
#define _NRC_HAL_CORE_CALLBACK_H

#include <linux/types.h>

/* HAL Event Types */
enum nrc_hal_event_type {
	/* HAL to Frontend Events */
	NRC_HAL_EVT_SPI_IRQ = 0, /* SPI interrupt received */
	NRC_HAL_EVT_RX_READY, /* RX data ready */
	NRC_HAL_EVT_TX_COMPLETE, /* TX complete */
	NRC_HAL_EVT_ERROR, /* Error occurred */
	NRC_HAL_EVT_WIM_EVENT, /* WIM event needs processing */
	NRC_HAL_EVT_REG_NOTIFIER, /* Regulatory domain change notification */
	NRC_HAL_EVT_KICK_TXQ, /* Kick TX queue processing */
	NRC_HAL_EVT_CLEANUP_TXQ_ALL, /* Cleanup all TX queues */
	NRC_HAL_EVT_FREE_SKB, /* Free SKB(HIF_TYPE_FRAME) */
	NRC_HAL_EVT_PS_DYN_START_CUSTOM_TIMEOUT, /* Start dynamic PS with custom timeout */
	NRC_HAL_EVT_WAKE_DONE, /* Wakeup sequence complete (HAL → Frontend) */
	NRC_HAL_EVT_PS_ENTER_FAILED, /* PS enter failed, recovery done (HAL → Frontend) */
	NRC_HAL_EVT_TARGET_NOTI_WDT_EXPIRED, /* WDT expired - target rebooting */
	NRC_HAL_EVT_TARGET_NOTI_FW_READY_FROM_WDT, /* Firmware ready from WDT reset */
	NRC_HAL_EVT_TARGET_NOTI_W_DISABLE_ASSERTED, /* W_DISABLE asserted - connection loss */
	NRC_HAL_EVT_TARGET_NOTI_TWT_SERVICE, /* TWT service notification */
	NRC_HAL_EVT_TARGET_NOTI_TWT_QUIET, /* TWT quiet notification */
	NRC_HAL_EVT_RECOVERY_TRIGGER, /* SW error recovery trigger */

	NRC_HAL_EVT_MAX
};

/* Frontend Types */
enum nrc_frontend_type {
	NRC_FRONTEND_WLAN = 0,
	NRC_FRONTEND_MCP,
	NRC_FRONTEND_MAX
};

/* HAL Event Data Structure */
struct nrc_hal_event_data {
	enum nrc_hal_event_type type;
	enum nrc_frontend_type frontend_type;
	void *data;
	size_t data_len;
};

/* HAL Callback Function Type */
typedef int (*nrc_hal_callback_fn)(struct nrc_hal_event_data *event);

/* Multi-Frontend Callback Registration */
int nrc_hal_register_callback(enum nrc_frontend_type type,
			      nrc_hal_callback_fn callback);
int nrc_hal_unregister_callback(enum nrc_frontend_type type);

/* HAL Event Trigger (internal use) */
int nrc_hal_trigger_event(struct nrc_hal_event_data *event);

/* HAL Initialization/Cleanup */
int nrc_hal_callback_init(void);
void nrc_hal_callback_cleanup(void);

#endif /* _NRC_HAL_CORE_CALLBACK_H */
