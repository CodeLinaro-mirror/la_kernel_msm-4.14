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

#ifdef CONFIG_PTSHARE
/* dup_mm() copies the parent's mm_struct: start outside any domain. */
static inline void ptshare_mm_init(struct mm_struct *mm)
{
	RCU_INIT_POINTER(mm->ptshare_desc, NULL);
	refcount_set(&mm->ptshare_vmas, 0);
}
#else /* !CONFIG_PTSHARE */
static inline void ptshare_mm_init(struct mm_struct *mm)
{
}
#endif /* CONFIG_PTSHARE */

#endif /* !__ASSEMBLY__ */

#endif /* _LINUX_PTSHARE_H */
