/*
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

/* Linux kernel headers */
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

/* Assembly headers */
#include <asm/uaccess.h>

/* Common directory headers - Core */
#include "nrc.h"
#include "nrc-hif.h"

/* Common directory headers - Debug & Trace */
#include "nrc-debug-common.h"

/* Common directory headers - Interfaces */
#include "nrc-backend-hif-callback.h"
#include "nrc-backend-hif-interface.h"
#include "nrc-hal-core-callback.h"
#include "nrc-hal-core-interface.h"

/* Local module headers */
#include "nrc-fw.h"
#include "nrc-init.h"
#include "nrc-vendor.h"
#include "wim.h"
#include "nrc-debug.h"
#include "hif.h"
#include "nrc-ps.h"
#if defined(CONFIG_SUPPORT_BD)
#include "nrc-bd.h"
#endif
#include "nrc-tx.h"
#ifdef CONFIG_SUPPORT_RECOVERY
#include "nrc-recovery.h"
#endif

/**
 * nrc_init_credit_queue - Initialize credit queue management in struct nrc
 * @hdev: pointer to struct nrc_hif_device
 *
 * Initializes the credit queue fields (front, rear, credit_max) based on chip ID.
 * This function replaces the SPI module's spi_set_default_credit function.
 */
void nrc_init_credit_queue(struct nrc_hif_device *hdev)
{
	int i;
	u16 chip_id;

	if (!hdev) {
		ERR_HIF("Invalid hdev pointer");
		return;
	}

	/* Initialize credit spinlock */
	spin_lock_init(&hdev->credit.lock);

	/* Initialize all arrays to zero */
	for (i = 0; i < CREDIT_QUEUE_MAX; i++) {
		hdev->credit.front[i] = 0;
		hdev->credit.rear[i] = 0;
		hdev->credit.credit_max[i] = 0;
	}

	/* Get chip ID from hdev structure if available, otherwise use default */
	if (hdev && hdev->chip_id != 0) {
		chip_id = hdev->chip_id;
	} else {
		ERR_HIF("Warning - chip ID not available");
		return;
	}

	/* Set credit limits based on chip ID */
	switch (chip_id) {
	case 0x7292:
	case 0x7394:
		hdev->credit.credit_max[0] = CREDIT_AC0;
		hdev->credit.credit_max[1] = CREDIT_AC1_80;
		hdev->credit.credit_max[2] = CREDIT_AC2;
		hdev->credit.credit_max[3] = CREDIT_AC3;
		hdev->credit.credit_max[5] = CREDIT_AC1;

		hdev->credit.credit_max[6] = CREDIT_AC0;
		hdev->credit.credit_max[7] = CREDIT_AC1_80;
		hdev->credit.credit_max[8] = CREDIT_AC2;
		hdev->credit.credit_max[9] = CREDIT_AC3;
		break;

	case 0x7391:
	case 0x7392:
	case 0x4791:
	case 0x5291:
		hdev->credit.credit_max[0] = 4;
		hdev->credit.credit_max[1] = CREDIT_AC1_20;
		hdev->credit.credit_max[2] = 4;
		hdev->credit.credit_max[3] = 4;

		hdev->credit.credit_max[6] = 4;
		hdev->credit.credit_max[7] = CREDIT_AC1_20;
		hdev->credit.credit_max[8] = 4;
		hdev->credit.credit_max[9] = 4;
		break;

	default:
		ERR_HIF("Unknown chip ID 0x%04x, using default credit values",
			chip_id);
		break;
	}

	/* Debug output: single line summary */
	{
		char buf[128];
		int len = 0;

		for (i = 0; i < CREDIT_QUEUE_MAX; i++) {
			if (hdev->credit.credit_max[i] > 0)
				len += scnprintf(buf + len, sizeof(buf) - len,
						 "[%d]=%d ", i,
						 hdev->credit.credit_max[i]);
		}
		DBG(CAT(CREDIT), "credit: %s", buf);
	}
}

#define MAX_RETRY_CNT 3
#define MAX_FW_RETRY_CNT 30

int nrc_nw_start(void)
{
	int ret;
	int retry = 0;
	struct nrc_hif_device *hdev = nrc_hal_core_get_hdev();
	enum NRC_DRV_STATE current_state;

	if (!hdev) {
		ERR_HIF("Invalid HIF device or ops");
		return -EINVAL;
	}

	/* Ensure firmware structure is initialized (needed for restarts) */
	if (!hdev->fw.priv) {
		WARN_HIF("FW private structure missing, re-initializing...");
		ret = nrc_hal_fw_init(hdev);
		if (ret) {
			ERR_FW("Failed to re-initialize HAL firmware: %d", ret);
			return ret;
		}
	}

	current_state = NRC_HIF_DRV_STATE(hdev);

	/* Check if HAL is already started by another frontend */
	if (current_state != NRC_DRV_INIT) {
		if (current_state >= NRC_DRV_START) {
			VBS_HIF("HAL already started (state=%s).",
				nrc_drv_state_str(current_state));
			return 0;
		} else {
			ERR_HIF("Invalid HIF state for nw_start: %s (%d)",
				nrc_drv_state_str(current_state),
				current_state);
			return -EINVAL;
		}
	}

	/* Perform full initialization */
	INFO("NRC start");

	/* 1st Phase: Hardware Reset and Probe (Handshake)
	 * Integrated from former hal_probe_hif_device logic.
	 * This ensures hardware is alive and chip_id is updated before use. */
try_probe:
	nrc_hif_ops_reset_device();
	ret = nrc_hif_ops_probe();
	if (ret && retry < MAX_RETRY_CNT) {
		WARN_HIF("Hardware probe failed (ret=%d), retrying... (%d/%d)",
			 ret, retry + 1, MAX_RETRY_CNT);
		retry++;
		goto try_probe;
	}

	if (ret) {
		ERR_HIF("Failed to probe hardware after %d retries: %d",
			MAX_RETRY_CNT, ret);
		return ret;
	}

	/* Initialize/Refresh credit queue based on probed chip_id */
	nrc_init_credit_queue(hdev);

	/* Check if firmware is already loaded (module reload case) */
	if (hdev->fw.loaded) {
		INFO("Firmware already loaded (%s), skipping download",
		     hdev->params->fw_name);
		NRC_HIF_SET_DRV_STATE(hdev, NRC_DRV_START);
		goto skip_fw_download;
	}

	/* Check if HW is in bootloader mode */
	if (hdev->params->fw_name && !nrc_hif_ops_fw_is_boot()) {
		ERR_HIF("Target not in bootloader mode");
		return -EINVAL;
	}

#if defined(CONFIG_SUPPORT_BD)
	ret = nrc_check_bd(hdev);
	if (ret) {
		ERR_HIF("Failed to load Board Data: %d", ret);
		return -EINVAL;
	}
#endif

	ret = nrc_fw_load(hdev);
	if (ret != 0) {
		ERR_HIF("Firmware loading failed: %d", ret);
		return ret;
	}

	hdev->fw.loaded = true;
	INFO("Firmware loaded: %s", hdev->params->fw_name);

skip_fw_download:
	NRC_HIF_SET_DRV_STATE(hdev, NRC_DRV_START);
	ret = nrc_hal_start();
	if (ret) {
		ERR_HIF("Failed to start HAL threads/IRQs: %d", ret);
		goto err_return;
	}

	ret = nrc_fw_start(hdev);
	if (ret) {
		ERR_HIF("Failed to send WIM_CMD_START: %d", ret);
		nrc_hif_dump_slot_credit("NW_START_FAIL");
		goto err_return;
	}

	/* Initialization complete - transition to operational state */
	NRC_HIF_SET_DRV_STATE(hdev, NRC_DRV_RUNNING);

#ifdef CONFIG_SUPPORT_RECOVERY
	/* Initialize or reset recovery engine based on start type.
	 * First start: full init. Restart after recovery: reset counters only.
	 * Error counting is always active for monitoring via debugfs. */
	if (!hdev->recovery)
		nrc_recovery_init(hdev);
	else
		nrc_recovery_reset(hdev);

	/* Start FW watchdog only when recovery=1 (daemon will handle restart).
	 * With recovery=0, WDT bark would send netlink with no listener. */
	if (hdev->params && hdev->params->recovery > 0) {
		nrc_recovery_wdt_init(hdev, NRC_RECOVERY_WDT_PERIOD_MS);
		nrc_recovery_wdt_kick(hdev);
	}
#endif

	INFO("HAL started successfully (DRV_RUNNING)");

	return 0;

err_return:
	ERR_HIF("HAL start sequence failed, rolling back...");
	hdev->fw.loaded = false;
	nrc_hal_stop(hdev);
	nrc_hif_ops_reset_device();
	NRC_HIF_SET_DRV_STATE(hdev, NRC_DRV_INIT);
	return ret;
}

int nrc_nw_start_fusing(void)
{
	int ret;
	struct nrc_hif_device *hdev = nrc_hal_core_get_hdev();
	if (!hdev) {
		ERR_HIF("Invalid HIF device or ops");
		return -EINVAL;
	}

	INFO("NRC start fusing flash");

	if (!NRC_DRV_IS_INIT(hdev)) {
		ERR_HIF("Invalid DRV state (%s)", NRC_DRV_STATE_STR(hdev));
		return -EINVAL;
	}

	if (hdev->params->dl_name && !nrc_hif_ops_fw_is_boot()) {
		ERR_HIF("Target not in bootloader mode");
		return -EINVAL;
	}

	ret = nrc_fw_fusing(hdev);

	return ret;
}

/**
 * nrc_nw_stop - Stop network operation
 *
 * Performs full HAL and hardware shutdown. Integrated from former cleanup logic.
 * Returns 0 on success.
 */
int nrc_nw_stop(void)
{
	struct nrc_hif_device *hdev = nrc_hal_core_get_hdev();

	if (!hdev) {
		ERR_HIF("Invalid HIF device");
		return -EINVAL;
	}

	/* Prevent redundant stop */
	if (NRC_HIF_DRV_STATE(hdev) == NRC_DRV_INIT ||
	    NRC_HIF_DRV_STATE(hdev) == NRC_DRV_STOP) {
		return 0;
	}

	INFO("Stopping HAL");

#ifdef CONFIG_SUPPORT_RECOVERY
	/* Stop FW watchdog before shutdown */
	nrc_recovery_wdt_clear(hdev);
	/* Disable recovery before shutdown to prevent new triggers */
	nrc_recovery_deinit(hdev);
#endif

	/* 1. Send WIM_CMD_STOP while state is still RUNNING
	 * This ensures the command is accepted and processed by FW */
	if (NRC_FW_IS_STARTED(hdev)) {
		DBG_HIF("Sending WIM_CMD_STOP before HAL stop");
		nrc_wim_request(NULL, WIM_CMD_STOP, 0, false, NULL);
		NRC_FW_CLEAR_STARTED(hdev);
	}

	/* 2. Transition to STOP state - prevents further WIM/Data requests */
	NRC_HIF_SET_DRV_STATE(hdev, NRC_DRV_STOP);

	/* 3. Stop HAL (Rx thread, IRQ handling, GPIOs) */
	nrc_hal_stop(hdev);

	/* 4. Reset PS state machine (preserve supports_dynamic_ps/timeout
	 *    which are set once at hw registration and restored via association) */
	hdev->ps.state = NRC_PS_STATE_WAKE;
	hdev->ps.mode = NRC_PS_NONE;
	hdev->ps.modem_enabled = false;
	hdev->ps.wake_pending = false;
	complete_all(&hdev->wake_done);

	/* 5. Cleanup TX queues and other core resources */
	nrc_tx_cleanup_queues();

	/* 6. Physical device reset */
	nrc_hif_ops_reset_device();

	/* 7. Firmware structure cleanup and clear loaded flag */
	nrc_hal_fw_cleanup(hdev);
	hdev->fw.loaded = false;

	/* 8. Final state transition */
	NRC_HIF_SET_DRV_STATE(hdev, NRC_DRV_INIT);

	return 0;
}

int nrc_hal_fw_init(struct nrc_hif_device *hdev)
{
	bool fw_was_loaded;

	/* Check if fw_priv is already allocated to prevent double initialization */
	if (hdev->fw.priv) {
		dev_warn(hdev->dev,
			 "FW already initialized, skipping re-initialization");
		return 0;
	}

	NRC_HIF_SET_DRV_STATE(hdev, NRC_DRV_INIT);

	/* Save fw.loaded flag before clearing structure (needed for WLAN module reload) */
	fw_was_loaded = hdev->fw.loaded;

	/* Initialize firmware structure */
	memset(&hdev->fw, 0, sizeof(hdev->fw));

	hdev->fw.priv = nrc_fw_alloc();
	if (!hdev->fw.priv) {
		ERR_HIF("Failed to allocate FW private structure");
		return -ENOMEM;
	}

	/* If firmware was already loaded (WLAN module reload case), restore flag */
	if (fw_was_loaded) {
		hdev->fw.loaded = true;
	}

	/* Initialize firmware state atomics */
	atomic_set(&hdev->fw.state, NRC_FW_NONE);
	atomic_set(&hdev->fw.started, 0);
	atomic_set(&hdev->fw.tx, 0);
	atomic_set(&hdev->fw.rx, 0);

	/* Initialize firmware capabilities */
	hdev->fw.use_ext_lna = false;
	hdev->fw.recovery_wdt = NULL;

#ifdef CONFIG_SUPPORT_RECOVERY
	/* Initialize restart mutual exclusion */
	mutex_init(&hdev->restart_mtx);
	hdev->restarting = false;
	init_completion(&hdev->restart_done);
#endif

	return 0;
}

void nrc_hal_fw_cleanup(struct nrc_hif_device *hdev)
{
	if (hdev && hdev->fw.priv) {
		/* Release firmware if loaded */
		if (hdev->fw.fw) {
			release_firmware(hdev->fw.fw);
			hdev->fw.fw = NULL;
		}

		/* Cleanup recovery watchdog if allocated */
		if (hdev->fw.recovery_wdt) {
			hdev->fw.recovery_wdt = NULL;
		}

		/* Cleanup firmware private data */
		nrc_fw_cleanup(hdev->fw.priv);

		/* Clear entire firmware structure except loaded flag (if needed to persist)
		 * Note: nrc_nw_stop explicitly clears loaded flag when full reset is intended. */
		memset(&hdev->fw, 0, sizeof(hdev->fw));
	}
}

/**
 * nrc_params_alloc - Allocate and initialize nrc_params structure
 *
 * Returns: Pointer to allocated nrc_params structure, or NULL on failure
 */
struct nrc_params *nrc_params_alloc(void)
{
	struct nrc_params *params;

	params = kzalloc(sizeof(struct nrc_params), GFP_KERNEL);
	if (!params) {
		ERR_HIF("Failed to allocate nrc_params structure");
		return NULL;
	}

	/* Initialize default values if needed */
	// params->power_save = 0;
	// params->sw_enc = 0;
	// params->ampdu_mode = 1;
	// params->support_ch_width = 0;

	return params;
}

/**
 * nrc_params_free - Free nrc_params structure
 * @params: Pointer to nrc_params structure to free
 *
 * Frees all dynamically allocated strings within the structure,
 * then frees the structure itself.
 */
void nrc_params_free(struct nrc_params *params)
{
	if (params) {
		/* Free all dynamically allocated string parameters */
		if (params->fw_name) {
			kfree(params->fw_name);
			params->fw_name = NULL;
		}
		if (params->bd_name) {
			kfree(params->bd_name);
			params->bd_name = NULL;
		}
		if (params->macaddr) {
			kfree(params->macaddr);
			params->macaddr = NULL;
		}
		if (params->fw_update_name) {
			kfree(params->fw_update_name);
			params->fw_update_name = NULL;
		}
		if (params->dl_name) {
			kfree(params->dl_name);
			params->dl_name = NULL;
		}
		if (params->bl_name) {
			kfree(params->bl_name);
			params->bl_name = NULL;
		}

		/* Free the structure itself */
		kfree(params);
	}
}

/**
 * nrc_debug_alloc - Allocate and initialize nrc_debug structure
 *
 * Returns: Pointer to allocated nrc_debug structure, or NULL on failure
 */
struct nrc_debug *nrc_debug_alloc(void)
{
	struct nrc_debug *debug;

	debug = kzalloc(sizeof(struct nrc_debug), GFP_KERNEL);
	if (!debug) {
		ERR_HIF("Failed to allocate nrc_debug structure");
		return NULL;
	}

	return debug;
}

/**
 * nrc_debug_free - Free nrc_debug structure
 * @debug: Pointer to nrc_debug structure to free
 */
void nrc_debug_free(struct nrc_debug *debug)
{
	if (debug) {
		kfree(debug);
	}
}
