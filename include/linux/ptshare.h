/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PTSHARE_H
#define _LINUX_PTSHARE_H

#ifndef __ASSEMBLY__

#include <linux/refcount.h>
#include <linux/mmu_notifier.h>
#include <linux/android_kabi.h>

struct mm_struct;
struct mem_cgroup;

struct ptshare_desc {
	/* The headless shadow MM for this domain */
	struct mm_struct *ptshare_mm;
	struct mmu_notifier mmu_notifier;
	struct mem_cgroup *memcg;
	refcount_t refcount;
	struct rcu_head rcu;

	ANDROID_BACKPORT_RESERVE(1);
	ANDROID_BACKPORT_RESERVE(2);
};

#endif /* !__ASSEMBLY__ */

#endif /* _LINUX_PTSHARE_H */
