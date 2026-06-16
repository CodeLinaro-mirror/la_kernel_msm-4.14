/*
 *
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC HIF Definitions - Host Interface constants and basic types
 *
 * This header contains HIF-related constants that are used across
 * multiple layers (backend, HAL, frontend). It has no dependencies
 * on other NRC headers to avoid circular includes.
 */

#ifndef _NRC_HIF_DEFS_H_
#define _NRC_HIF_DEFS_H_

#include <linux/types.h>

/*
 * EIRQ Status Register Values
 * These values are read from the target device's EIRQ status register
 * to determine the current device state.
 */
#define EIRQ_STATUS_DEVICE_ROM		(0x00)	/* Device in ROM/bootloader mode */
#define EIRQ_STATUS_TXQUE_EIRQ		(0x01)	/* TX queue interrupt pending */
#define EIRQ_STATUS_RXQUE_EIRQ		(0x02)	/* RX queue interrupt pending */
#define EIRQ_STATUS_DEVICE_READY	(0x04)	/* Device ready/firmware running */
#define EIRQ_STATUS_DEVICE_SLEEP	(0x08)	/* Device in sleep mode */

/*
 * Firmware Software State (driver-side tracking)
 * These values track firmware loading progress in the driver
 */
#define NRC_FW_NONE	(0)	/* No firmware loaded */
#define NRC_FW_LOADING	(1)	/* Firmware download in progress */
#define NRC_FW_ACTIVE	(2)	/* Firmware running */
#define NRC_FW_FAILED	(-1)	/* Firmware load failed */

/**
 * enum NRC_FW_STATE - Firmware hardware state (from EIRQ_STATUS)
 * @NRC_FW_STATE_ROM: Device in ROM/bootloader mode (EIRQ=0x00)
 * @NRC_FW_STATE_READY: Firmware running and operational (EIRQ=0x04)
 * @NRC_FW_STATE_SLEEP: Firmware in deep sleep mode (EIRQ=0x08)
 */
enum NRC_FW_STATE {
	NRC_FW_STATE_ROM = 0,
	NRC_FW_STATE_READY = 1,
	NRC_FW_STATE_SLEEP = 2,
};

/* FW state string for debug */
static inline const char *nrc_fw_state_str(enum NRC_FW_STATE state)
{
	switch (state) {
	case NRC_FW_STATE_ROM:
		return "ROM";
	case NRC_FW_STATE_READY:
		return "READY";
	case NRC_FW_STATE_SLEEP:
		return "SLEEP";
	default:
		return "UNKNOWN";
	}
}

/**
 * enum NRC_DRV_STATE - Driver Operational States (Capability Progression)
 *
 * Ordered by Operational Capability Level:
 * - 0-2: Inactive/Blocking states (No data/WIM traffic)
 * - 3:   Transitional state (Starting up, control traffic only)
 * - 4-5: Active/Operational states (Full capability)
 *
 * @NRC_DRV_INIT:   [Level 0] Initial state, hardware not yet initialized.
 * @NRC_DRV_REBOOT: [Level 1] WDT recovery in progress (blocks new requests).
 * @NRC_DRV_STOP:   [Level 2] Shutdown in progress (blocks all new requests).
 * @NRC_DRV_START:  [Level 3] Startup in progress, loading firmware/config.
 * @NRC_DRV_RUNNING:[Level 4] Normal operation, fully active.
 * @NRC_DRV_PS:     [Level 5] Hardware in Deep Sleep (fully configured).
 */
enum NRC_DRV_STATE {
	NRC_DRV_INIT = 0,
	NRC_DRV_REBOOT = 1,
	NRC_DRV_STOP = 2,
	NRC_DRV_START = 3,
	NRC_DRV_RUNNING = 4,
	NRC_DRV_PS = 5,
};

/* Driver state string conversion */
static inline const char *nrc_drv_state_str(enum NRC_DRV_STATE state)
{
	switch (state) {
	case NRC_DRV_REBOOT:
		return "REBOOT";
	case NRC_DRV_INIT:
		return "INIT";
	case NRC_DRV_STOP:
		return "STOP";
	case NRC_DRV_START:
		return "START";
	case NRC_DRV_RUNNING:
		return "RUNNING";
	case NRC_DRV_PS:
		return "SLEEP";
	default:
		return "UNKNOWN";
	}
}

/*
 * HIF Slot Definitions
 * Used for TX/RX buffer management between host and target
 */
#define TX_SLOT			0
#define RX_SLOT			1
#define TX_SLOT_SIZE		456
#define RX_SLOT_SIZE		492

/* Slot index types */
#define SLOT_HEAD		0
#define SLOT_TAIL		1

/*
 * Credit Queue Management
 * Credits control flow between host and target for each AC queue
 */
#define CREDIT_QUEUE_MAX	(12)

/* Credit calculation constants */
#define TCN			(2 * 1)
#define TCNE			(0)

/* Per-AC credit limits */
#define CREDIT_AC0		(TCN * 2 + TCNE)	/* BK (4) */
#define CREDIT_AC1		(TCN * 20 + TCNE)	/* BE (40) */
#define CREDIT_AC1_20		(TCN * 10 + TCNE)	/* BE (20) - reduced */
#define CREDIT_AC1_80		(TCN * 40 + TCNE)	/* BE (80) - extended */
#define CREDIT_AC2		(TCN * 4 + TCNE)	/* VI (8) */
#define CREDIT_AC3		(TCN * 4 + TCNE)	/* VO (8) */

/*
 * WIM (Wireless Interface Message) Constants
 */
#define WIM_SKB_MAX		(10)
#define WIM_RESP_TIMEOUT	(msecs_to_jiffies(100))

/*
 * VIF (Virtual Interface) Limits
 */
#ifndef NR_NRC_VIF
#define NR_NRC_VIF		(2)
#endif
#define NR_NRC_VIF_HW_QUEUE	(4)
#define NR_NRC_MAX_TXQ		(100)

/* VIF0 AC0~3, BCN, GP, VIF1 AC0~3, BCN */
#define NRC_QUEUE_MAX		(NR_NRC_VIF_HW_QUEUE * NR_NRC_VIF + 3)

/*
 * TID (Traffic Identifier) Limits
 */
#define NRC_MAX_TID		(8)

/*
 * MAC Frame and Protocol Detection Macros
 */
#define WLAN_FC_GET_TYPE(fc)	(((fc) & 0x000c) >> 2)
#define WLAN_FC_GET_STYPE(fc)	(((fc) & 0x00f0) >> 4)
#define IS_ARP(skb)		((skb)->protocol == htons(ETH_P_ARP))

#endif /* _NRC_HIF_DEFS_H_ */
