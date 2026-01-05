/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC Backend Callback Interface - SPI to HAL event notification
 */

#ifndef _NRC_BACKEND_HIF_CALLBACK_H
#define _NRC_BACKEND_HIF_CALLBACK_H

#include <linux/types.h>
#include "nrc-hif-defs.h"
#include "nrc-ps-common.h"

/* Forward declarations */
struct nrc;
struct nrc_hif_device;

/*
 * Target Notification Codes (see system_common.h in firmware)
 * These codes are sent by the target device to notify the host
 * of various events and state changes.
 */
#define TARGET_NOTI_WDT_EXPIRED		(0x7D)	/* Watchdog timer expired */
#define TARGET_NOTI_FW_READY_FROM_WDT	(0x9D)	/* FW ready after WDT reset */
#define TARGET_NOTI_W_DISABLE_ASSERTED	(0xAB)	/* W_DISABLE signal asserted */
#define TARGET_NOTI_REQUEST_FW_DOWNLOAD	(0xDC)	/* Request firmware download */
#define TARGET_NOTI_FW_READY_FROM_PS	(0xEC)	/* FW ready after power save */
#define TARGET_NOTI_FAILED_TO_ENTER_PS	(0xED)	/* Failed to enter power save */
#define TARGET_NOTI_TWT_SERVICE		(0xEA)	/* TWT service notification */
#define TARGET_NOTI_TWT_QUIET		(0xEB)	/* TWT quiet notification */
#define TARGET_NOTI_BEACON_UPDATED	(0xBE)	/* Beacon updated notification */
#define TARGET_NOTI_FW_ENTER_TO_PS	(0xEF)	/* Firmware entering power save */
#define TARGET_NOTI_PS_READY		(0x11)	/* Power save ready */

/* Backend Event Types */
enum nrc_backend_event_type {
	NRC_BACKEND_EVT_IRQ = 0, /* Hardware interrupt occurred */
	NRC_BACKEND_EVT_RX_READY, /* RX data ready */
	NRC_BACKEND_EVT_TX_COMPLETE, /* TX complete */
	NRC_BACKEND_EVT_ERROR, /* Error occurred */
	NRC_BACKEND_EVT_RESET_TX, /* TX reset required */
	NRC_BACKEND_EVT_RESET_RX, /* RX reset required */

	/* Backend-specific device notification events */
	NRC_BACKEND_EVT_TARGET_NOTI_WDT_EXPIRED, /* Watchdog timer expired */
	NRC_BACKEND_EVT_TARGET_NOTI_FW_READY_FROM_WDT, /* FW ready after WDT reset */
	NRC_BACKEND_EVT_TARGET_NOTI_W_DISABLE_ASSERTED, /* W_DISABLE signal asserted */
	NRC_BACKEND_EVT_TARGET_NOTI_REQUEST_FW_DOWNLOAD, /* Request firmware download */
	NRC_BACKEND_EVT_TARGET_NOTI_FW_READY_FROM_PS, /* FW ready after power save */
	NRC_BACKEND_EVT_TARGET_NOTI_FAILED_TO_ENTER_PS, /* Failed to enter power save */
	NRC_BACKEND_EVT_TARGET_NOTI_TWT_SERVICE, /* TWT service notification */
	NRC_BACKEND_EVT_TARGET_NOTI_TWT_QUIET, /* TWT quiet notification */
	NRC_BACKEND_EVT_TARGET_NOTI_BEACON_UPDATED, /* Beacon updated notification */
	NRC_BACKEND_EVT_TARGET_NOTI_FW_ENTER_TO_PS, /* Firmware entering power save */
	NRC_BACKEND_EVT_TARGET_NOTI_PS_READY, /* Power save ready notification */

	NRC_BACKEND_EVT_MAX
};

/* Backend Event Data Structure */
struct nrc_spi_event_data {
	enum nrc_backend_event_type type;
	void *data;
	size_t data_len;
};

/* Backend Callback Function Type */
typedef int (*nrc_spi_callback_fn)(struct nrc_spi_event_data *event);

/* Backend Callback Registration - exported via EXPORT_SYMBOL */
int nrc_spi_register_callback(nrc_spi_callback_fn callback);
int nrc_spi_unregister_callback(void);

/* Backend Event Trigger (internal use) */
int nrc_spi_trigger_event(struct nrc_spi_event_data *event);

/* Backend-HAL Interface Functions */
void nrc_backend_set_hal_core_refs(struct nrc_hif_device *hdev);

/**
 * nrc_spi_target_noti_to_event - Convert TARGET_NOTI to backend event type
 * @target_noti: Target notification code from msg[3]
 * @event_type: Output parameter for event type (only set if valid)
 *
 * Returns: true if valid event, false if unknown (caller should not trigger event)
 */
static inline bool
nrc_spi_target_noti_to_event(u32 target_noti, enum nrc_backend_event_type *event_type)
{
	switch (target_noti) {
	case 0x7D: /* TARGET_NOTI_WDT_EXPIRED */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_WDT_EXPIRED;
		return true;
	case 0x9D: /* TARGET_NOTI_FW_READY_FROM_WDT */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_FW_READY_FROM_WDT;
		return true;
	case 0xAB: /* TARGET_NOTI_W_DISABLE_ASSERTED */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_W_DISABLE_ASSERTED;
		return true;
	case 0xDC: /* TARGET_NOTI_REQUEST_FW_DOWNLOAD */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_REQUEST_FW_DOWNLOAD;
		return true;
	case 0xEC: /* TARGET_NOTI_FW_READY_FROM_PS */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_FW_READY_FROM_PS;
		return true;
	case 0xED: /* TARGET_NOTI_FAILED_TO_ENTER_PS */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_FAILED_TO_ENTER_PS;
		return true;
	case 0xEA: /* TARGET_NOTI_TWT_SERVICE */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_TWT_SERVICE;
		return true;
	case 0xEB: /* TARGET_NOTI_TWT_QUIET */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_TWT_QUIET;
		return true;
	case 0xBE: /* TARGET_NOTI_BEACON_UPDATED */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_BEACON_UPDATED;
		return true;
	case 0xEF: /* TARGET_NOTI_FW_ENTER_TO_PS */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_FW_ENTER_TO_PS;
		return true;
	case 0x11: /* TARGET_NOTI_PS_READY */
		*event_type = NRC_BACKEND_EVT_TARGET_NOTI_PS_READY;
		return true;
	default:
		return false; /* Unknown event - caller should log error and skip */
	}
}

#endif /* _NRC_BACKEND_SPI_CALLBACK_H */
