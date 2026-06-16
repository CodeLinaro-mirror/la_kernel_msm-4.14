/* SPDX-License-Identifier: ISC */

/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC Recovery - Error counting + FW watchdog → netlink → recoveryd
 */

#ifndef _NRC_RECOVERY_H_
#define _NRC_RECOVERY_H_

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

struct nrc_hif_device;

/* Thresholds */
#define NRC_RECOVERY_ERR_THRESHOLD	3	/* 4th consecutive → trigger */
#define NRC_RECOVERY_TOTAL_THRESHOLD	6	/* 7th aggregate → trigger */
#define NRC_RECOVERY_TIME_WINDOW_MS	60000	/* 60s no clear → trigger */
#define NRC_RECOVERY_WDT_PERIOD_MS	2000	/* FW alive check: fail if no RX within 2s */

enum nrc_recovery_err_type {
	NRC_RECOVERY_WIM_ERR,
	NRC_RECOVERY_TX_ERR,
	NRC_RECOVERY_WAKEUP_ERR,
	NRC_RECOVERY_ERR_MAX,
};

struct nrc_recovery_wdt {
	bool enable;
	bool suspended;
	int period;
	struct timer_list wdt;
	struct timer_list poll_timer;
	struct work_struct work;
	struct work_struct poll_work;
};

struct nrc_recovery {
	struct nrc_hif_device *hdev;	/* back-pointer (set at init) */
	bool enabled;
	bool in_recovery;
	u8 err[NRC_RECOVERY_ERR_MAX];
	u8 total_err;
	struct mutex lock;
	struct delayed_work time_check_work;
	u32 recovery_count;
	unsigned long last_recovery_jiffies;
};

#ifdef CONFIG_SUPPORT_RECOVERY

extern const char *const nrc_recovery_err_names[];

/* Error counting */
void nrc_recovery_init(struct nrc_hif_device *hdev);
void nrc_recovery_reset(struct nrc_hif_device *hdev);
void nrc_recovery_deinit(struct nrc_hif_device *hdev);
void nrc_recovery_start(struct nrc_hif_device *hdev, const char *reason);
void nrc_recovery_inc(struct nrc_hif_device *hdev,
		      enum nrc_recovery_err_type type);
void nrc_recovery_zero(struct nrc_hif_device *hdev,
		       enum nrc_recovery_err_type type);
void nrc_recovery_show(struct nrc_hif_device *hdev);

/* FW Watchdog */
void nrc_recovery_wdt_init(struct nrc_hif_device *hdev, int period_ms);
void nrc_recovery_wdt_kick(struct nrc_hif_device *hdev);
void nrc_recovery_wdt_clear(struct nrc_hif_device *hdev);
void nrc_recovery_wdt_set_suspended(struct nrc_hif_device *hdev, bool suspend);

#else /* !CONFIG_SUPPORT_RECOVERY */

static inline void nrc_recovery_init(struct nrc_hif_device *h) { }
static inline void nrc_recovery_reset(struct nrc_hif_device *h) { }
static inline void nrc_recovery_deinit(struct nrc_hif_device *h) { }
static inline void nrc_recovery_start(struct nrc_hif_device *h,
				      const char *r) { }
static inline void nrc_recovery_inc(struct nrc_hif_device *h,
				    enum nrc_recovery_err_type t) { }
static inline void nrc_recovery_zero(struct nrc_hif_device *h,
				     enum nrc_recovery_err_type t) { }
static inline void nrc_recovery_show(struct nrc_hif_device *h) { }
static inline void nrc_recovery_wdt_init(struct nrc_hif_device *h,
					 int p) { }
static inline void nrc_recovery_wdt_kick(struct nrc_hif_device *h) { }
static inline void nrc_recovery_wdt_clear(struct nrc_hif_device *h) { }
static inline void nrc_recovery_wdt_set_suspended(struct nrc_hif_device *h,
						  bool s) { }

#endif /* CONFIG_SUPPORT_RECOVERY */

#endif /* _NRC_RECOVERY_H_ */
