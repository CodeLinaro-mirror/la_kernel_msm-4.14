/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC Recovery - Error counting + FW watchdog → netlink → recoveryd
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "nrc-hif.h"
#include "nrc-hif-defs.h"
#include "nrc-debug-common.h"
#include "nrc-hal-core-callback.h"
#include "nrc-hal-core-interface.h"
#include "nrc-recovery.h"
#include "nrc-debug.h"
#include "wim.h"

const char *const nrc_recovery_err_names[] = {
	[NRC_RECOVERY_WIM_ERR]    = "wim_err",
	[NRC_RECOVERY_TX_ERR]     = "tx_err",
	[NRC_RECOVERY_WAKEUP_ERR] = "wakeup_err",
};

/* ── Error Counting ─────────────────────────────────────────────────── */

static void nrc_recovery_time_check_handler(struct work_struct *work)
{
	struct nrc_recovery *r =
		container_of(work, struct nrc_recovery, time_check_work.work);
	struct nrc_hif_device *hdev = r->hdev;

	if (!r->enabled || !r->total_err)
		return;

	WRN("recovery: time window expired (total_err=%u)", r->total_err);
	nrc_recovery_start(hdev, "time_window");
}

void nrc_recovery_init(struct nrc_hif_device *hdev)
{
	struct nrc_recovery *r;

	r = kzalloc(sizeof(*r), GFP_KERNEL);
	if (!r) {
		ERR("recovery: failed to alloc");
		return;
	}

	r->hdev = hdev;
	mutex_init(&r->lock);
	INIT_DELAYED_WORK(&r->time_check_work,
			  nrc_recovery_time_check_handler);
	r->enabled = true;
	hdev->recovery = r;
	INFO("recovery: init");
}

void nrc_recovery_reset(struct nrc_hif_device *hdev)
{
	struct nrc_recovery *r = hdev->recovery;

	if (!r)
		return;
	mutex_lock(&r->lock);
	r->in_recovery = false;
	memset(r->err, 0, sizeof(r->err));
	r->total_err = 0;
	mutex_unlock(&r->lock);
	INFO("recovery: reset");
}

void nrc_recovery_deinit(struct nrc_hif_device *hdev)
{
	struct nrc_recovery *r = hdev->recovery;

	if (!r)
		return;
	mutex_lock(&r->lock);
	r->enabled = false;
	r->in_recovery = false;
	memset(r->err, 0, sizeof(r->err));
	r->total_err = 0;
	mutex_unlock(&r->lock);
	cancel_delayed_work_sync(&r->time_check_work);
	INFO("recovery: deinit");
	hdev->recovery = NULL;
	kfree(r);
}

/**
 * nrc_recovery_start - Trigger recovery via HAL→Frontend→netlink
 */
void nrc_recovery_start(struct nrc_hif_device *hdev, const char *reason)
{
	struct nrc_recovery *r = hdev->recovery;
	struct nrc_hal_event_data event;
	enum NRC_DRV_STATE state;

	if (!r || !r->enabled)
		return;

	state = (enum NRC_DRV_STATE)atomic_read(&hdev->drv_state);
	if (state != NRC_DRV_RUNNING && state != NRC_DRV_PS) {
		DBG_STATE("recovery: skip (drv_state=%s, reason=%s)",
			  nrc_drv_state_str(state), reason);
		return;
	}

	mutex_lock(&r->lock);
	if (r->in_recovery) {
		mutex_unlock(&r->lock);
		return;
	}
	r->in_recovery = true;
	r->recovery_count++;
	r->last_recovery_jiffies = jiffies;
	mutex_unlock(&r->lock);

	/*
	 * Use non-sync cancel: nrc_recovery_start() may be called from
	 * within nrc_recovery_time_check_handler (a work_struct handler
	 * for time_check_work itself).  cancel_delayed_work_sync() would
	 * try to flush that same work item, causing a recursive lock
	 * warning and potential deadlock.  The in_recovery flag already
	 * prevents the handler from triggering a second recovery if it
	 * somehow fires after this point.
	 */
	cancel_delayed_work(&r->time_check_work);

	ERR("recovery: TRIGGERED #%u (reason=%s) "
		"wim=%u tx=%u wake=%u total=%u",
		r->recovery_count, reason,
		r->err[NRC_RECOVERY_WIM_ERR],
		r->err[NRC_RECOVERY_TX_ERR],
		r->err[NRC_RECOVERY_WAKEUP_ERR],
		r->total_err);

	memset(&event, 0, sizeof(event));
	event.type = NRC_HAL_EVT_RECOVERY_TRIGGER;
	event.frontend_type = NRC_FRONTEND_WLAN;
	event.data = (void *)reason;
	event.data_len = strlen(reason) + 1;
	nrc_hal_trigger_event(&event);

	/* Clear in_recovery after event dispatch (synchronous) so
	 * future triggers can fire if this recovery attempt fails.
	 */
	mutex_lock(&r->lock);
	r->in_recovery = false;
	mutex_unlock(&r->lock);
}

void nrc_recovery_inc(struct nrc_hif_device *hdev,
		      enum nrc_recovery_err_type type)
{
	struct nrc_recovery *r = hdev->recovery;

	if (!r || !r->enabled || type >= NRC_RECOVERY_ERR_MAX)		return;

	mutex_lock(&r->lock);
	r->err[type]++;
	r->total_err++;
	mutex_unlock(&r->lock);

	WRN("recovery: %s++ (%u), total=%u",
		 nrc_recovery_err_names[type], r->err[type], r->total_err);

	if (r->err[type] > NRC_RECOVERY_ERR_THRESHOLD)
		nrc_recovery_start(hdev, nrc_recovery_err_names[type]);
	else if (r->total_err > NRC_RECOVERY_TOTAL_THRESHOLD)
		nrc_recovery_start(hdev, "total_err");
	else if (NRC_RECOVERY_TIME_WINDOW_MS)
		mod_delayed_work(system_wq, &r->time_check_work,
				 msecs_to_jiffies(NRC_RECOVERY_TIME_WINDOW_MS));
}

void nrc_recovery_zero(struct nrc_hif_device *hdev,
		       enum nrc_recovery_err_type type)
{
	struct nrc_recovery *r = hdev->recovery;

	if (!r || !r->enabled || type >= NRC_RECOVERY_ERR_MAX || !r->err[type])
		return;

	mutex_lock(&r->lock);
	r->total_err = (r->total_err >= r->err[type]) ?
		       r->total_err - r->err[type] : 0;
	r->err[type] = 0;
	mutex_unlock(&r->lock);

	if (!r->total_err)
		cancel_delayed_work_sync(&r->time_check_work);
}

void nrc_recovery_show(struct nrc_hif_device *hdev)
{
	struct nrc_recovery *r = hdev->recovery;

	if (!r) {
		INFO("recovery: not initialized");
		return;
	}
	INFO("recovery: enabled=%d in_recovery=%d count=%u "
		 "wim=%u tx=%u wake=%u total=%u",
		 r->enabled, r->in_recovery, r->recovery_count,
		 r->err[NRC_RECOVERY_WIM_ERR],
		 r->err[NRC_RECOVERY_TX_ERR],
		 r->err[NRC_RECOVERY_WAKEUP_ERR],
		 r->total_err);
}

/* ── FW Watchdog (WDT) ──────────────────────────────────────────────
 * Two timers: poll_timer (period/2) sends keep-alive, wdt (period)
 * barks if no RX. Every RX calls wdt_kick() to reset both.
 * ─────────────────────────────────────────────────────────────────── */

static void nrc_recovery_wdt_trigger(struct work_struct *work)
{
	struct nrc_hif_device *hdev = nrc_hal_core_get_hdev();

	if (hdev)
		nrc_recovery_start(hdev, "wdt_bark");
}

static void nrc_recovery_wdt_bark(struct timer_list *t)
{
	struct nrc_recovery_wdt *wdt = from_timer(wdt, t, wdt);

	if (wdt->suspended)
		return;

	ERR("recovery: FW alive check failed! FW unresponsive");
	queue_work(system_wq, &wdt->work);
}

static void nrc_recovery_wdt_poll_work(struct work_struct *work)
{
	struct nrc_recovery_wdt *wdt =
		container_of(work, struct nrc_recovery_wdt, poll_work);

	if (wdt->enable && !wdt->suspended)
		nrc_wim_request(NULL, WIM_CMD_KEEP_ALIVE, 0, false, NULL);
}

static void nrc_recovery_wdt_poll(struct timer_list *t)
{
	struct nrc_recovery_wdt *wdt = from_timer(wdt, t, poll_timer);

	if (wdt->suspended)
		return;

	queue_work(system_wq, &wdt->poll_work);
}

void nrc_recovery_wdt_init(struct nrc_hif_device *hdev, int period_ms)
{
	struct nrc_recovery_wdt *wdt;

	if (hdev->fw.recovery_wdt && hdev->fw.recovery_wdt->enable)
		return;

	wdt = kzalloc(sizeof(*wdt), GFP_KERNEL);
	if (!wdt) {
		ERR("recovery: failed to alloc FW alive check");
		return;
	}

	wdt->enable = true;
	wdt->period = period_ms;
	timer_setup(&wdt->wdt, nrc_recovery_wdt_bark, 0);
	timer_setup(&wdt->poll_timer, nrc_recovery_wdt_poll, 0);
	INIT_WORK(&wdt->work, nrc_recovery_wdt_trigger);
	INIT_WORK(&wdt->poll_work, nrc_recovery_wdt_poll_work);
	hdev->fw.recovery_wdt = wdt;
	INFO("recovery: FW alive check init (period=%dms)", period_ms);
}

void nrc_recovery_wdt_kick(struct nrc_hif_device *hdev)
{
	struct nrc_recovery_wdt *wdt;

	if (!hdev)
		return;
	wdt = hdev->fw.recovery_wdt;
	if (!wdt || !wdt->enable || wdt->suspended)
		return;

	mod_timer(&wdt->wdt, jiffies + msecs_to_jiffies(wdt->period));
	mod_timer(&wdt->poll_timer, jiffies + msecs_to_jiffies(wdt->period / 2));
}

void nrc_recovery_wdt_set_suspended(struct nrc_hif_device *hdev, bool suspend)
{
	struct nrc_recovery_wdt *wdt;

	if (!hdev)
		return;
	wdt = hdev->fw.recovery_wdt;
	if (!wdt || !wdt->enable) {
		if (suspend)
			DBG_PS("recovery: FW alive check not active, skip suspend");
		return;
	}

	wdt->suspended = suspend;
	if (suspend) {
		del_timer(&wdt->wdt);
		del_timer(&wdt->poll_timer);
		DBG_PS("recovery: FW alive check suspended (PS sleep)");
	} else {
		nrc_recovery_wdt_kick(hdev);
		DBG_PS("recovery: FW alive check resumed");
	}
}

void nrc_recovery_wdt_clear(struct nrc_hif_device *hdev)
{
	struct nrc_recovery_wdt *wdt;

	if (!hdev)
		return;
	wdt = hdev->fw.recovery_wdt;
	if (!wdt || !wdt->enable)
		return;

	wdt->enable = false;
	del_timer_sync(&wdt->wdt);
	del_timer_sync(&wdt->poll_timer);
	cancel_work_sync(&wdt->work);
	cancel_work_sync(&wdt->poll_work);
	kfree(wdt);
	hdev->fw.recovery_wdt = NULL;
	INFO("recovery: FW alive check cleared");
}
