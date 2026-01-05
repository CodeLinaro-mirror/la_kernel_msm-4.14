/*
 * NRC Power Save Module Header
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

#ifndef _NRC_PS_H_
#define _NRC_PS_H_

#include "nrc-ps-common.h"

/* Target sleep/wake timing constants */
#define TARGET_MAX_TIME_TO_FALL_ASLEEP 550 /* ms */

/* Forward declarations */
struct nrc_hif_device;
struct work_struct;

/* State Machine API - Single entry point for all PS state transitions */

/**
 * nrc_ps_handle_event - Handle PS state machine event
 * @hdev: HIF device structure
 * @event_data: Event data with type, mode, timeout, reason
 *
 * This is the SINGLE entry point for all PS state transitions.
 * All state changes must go through this function to maintain consistency.
 *
 * Returns: 0 on success, negative error code on failure
 */
int nrc_ps_handle_event(struct nrc_hif_device *hdev,
			struct nrc_ps_event_data *event_data);

/**
 * nrc_ps_request_wake - Request wake from atomic context (TX tasklet)
 * @hdev: HIF device structure
 * @reason: Reason for wake request
 *
 * Safe to call from atomic context. Sets wake_pending flag and
 * triggers GPIO immediately without workqueue.
 *
 * Returns: 0 if wake initiated, 1 if already awake, negative on error
 */
int nrc_ps_request_wake(struct nrc_hif_device *hdev, enum NRC_PS_REASON reason);

/**
 * nrc_ps_request_wake_sync - Synchronous wake with timeout
 * @hdev: HIF device structure
 * @timeout_ms: Timeout in milliseconds (0 for async)
 * @reason: Reason for wake request
 *
 * Must be called from sleepable context. Waits for FW_READY.
 *
 * Returns: 0 on success, negative on timeout/error
 */
int nrc_ps_request_wake_sync(struct nrc_hif_device *hdev, int timeout_ms,
			     enum NRC_PS_REASON reason);

/* PS Interrupt Handler */
void nrc_ps_handle_fw_ready(void);

/* HAL Master PS Operations */

/**
 * nrc_hal_ps_request_sleep - Request sleep mode (HAL Master)
 * @mode: Power save mode
 * @timeout: Sleep duration in ms
 * @wowlan: WoWLAN configuration (optional)
 * @reason: Reason for PS mode change
 *
 * Handles complete sleep sequence including state machine and HW operations.
 * Returns: 0 on success, negative on failure
 */
int nrc_hal_ps_request_sleep(enum NRC_PS_MODE mode, u64 timeout,
			     struct cfg80211_wowlan *wowlan,
			     enum NRC_PS_REASON reason);

/**
 * nrc_hal_ps_request_wake - Request wake from sleep (HAL Master)
 * @timeout_ms: Timeout in ms (0 for async)
 * @reason: Reason for wake
 *
 * Handles complete wake sequence including state machine and HW operations.
 * Returns: 0 on success, negative on failure
 */
int nrc_hal_ps_request_wake(int timeout_ms, enum NRC_PS_REASON reason);

#endif /* _NRC_PS_H_ */
