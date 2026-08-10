/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PTSHARE_H
#define _LINUX_PTSHARE_H

#ifndef __ASSEMBLY__

#include <linux/mm.h>
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
/**
 * vma_is_ptshare_client - Check whether a VMA is a page table sharing client
 * @vma: The VMA to check.
 *
 * Use this to skip or refuse operations that must not modify the page
 * tables of @vma, which are shared with the other members of its domain.
 * It stays true until @vma is completely unshared, so it also covers
 * callers that do not hold the mmap lock, such as rmap walks.
 *
 * Return: true if @vma is a client VMA.
 */
static inline bool vma_is_ptshare_client(const struct vm_area_struct *vma)
{
	return vma->vm_flags & VM_PT_SHARED;
}

/**
 * vma_shares_pagetables - Check whether a client VMA is attached to its domain
 * @vma: The VMA to check.
 *
 * Use this before acting on the domain of @vma, for example to unshare
 * its page tables. Unlike vma_is_ptshare_client(), it becomes false as
 * soon as @vma drops its reference to the shared page tables, so that
 * the reference is never dropped twice.
 *
 * Return: true if @vma is a client VMA attached to its domain.
 */
static inline bool vma_shares_pagetables(const struct vm_area_struct *vma)
{
	return vma_is_ptshare_client(vma) && READ_ONCE(vma->__vm_ptshare_desc);
}

/**
 * mm_is_ptshare_shadow - Check whether an MM is the shadow MM of a domain
 * @mm: The MM to check.
 *
 * Return: true if @mm owns the shared page tables of a sharing domain.
 */
static inline bool mm_is_ptshare_shadow(const struct mm_struct *mm)
{
	return mm_flags_test(MMF_PTSHARE_SHADOW, mm);
}

/**
 * vma_is_ptshare_shadow - Check whether a VMA belongs to a shadow MM
 * @vma: The VMA to check.
 *
 * A shadow VMA maps the shared page tables directly and keeps them alive
 * for as long as client VMAs use them. Use this to keep operations meant
 * for processes, such as uprobes or THP collapse, away from shadow VMAs.
 *
 * Return: true if @vma is a shadow VMA.
 */
static inline bool vma_is_ptshare_shadow(const struct vm_area_struct *vma)
{
	return vma->vm_mm && mm_is_ptshare_shadow(vma->vm_mm);
}

/**
 * vma_ptshare_desc - Get the domain descriptor of a client VMA
 * @vma: A client VMA.
 *
 * Return: the descriptor, or NULL if @vma is not attached to its domain.
 */
static inline struct ptshare_desc *vma_ptshare_desc(const struct vm_area_struct *vma)
{
	if (WARN_ON_ONCE(!vma_is_ptshare_client(vma)))
		return NULL;

	return READ_ONCE(vma->__vm_ptshare_desc);
}

/**
 * vma_set_ptshare_desc - Attach a client VMA to its domain, or detach it
 * @vma: A client VMA.
 * @desc: The descriptor of the domain, or NULL to detach @vma.
 */
static inline void vma_set_ptshare_desc(struct vm_area_struct *vma,
					struct ptshare_desc *desc)
{
	if (WARN_ON_ONCE(!vma_is_ptshare_client(vma)))
		return;

	WRITE_ONCE(vma->__vm_ptshare_desc, desc);
}

/**
 * vma_ptshare_refcount - Get the client count of a shadow VMA
 * @vma: A shadow VMA.
 *
 * Return: a pointer to the count of client VMAs that use the page tables
 * of @vma, or NULL if @vma is not a shadow VMA.
 */
static inline refcount_t *vma_ptshare_refcount(struct vm_area_struct *vma)
{
	if (WARN_ON_ONCE(!vma_is_ptshare_shadow(vma)))
		return NULL;

	return &vma->__vm_ptshare_refcount;
}

/*
 * vm_area_dup() does not zero the new VMA: a copy of a client VMA keeps
 * its descriptor, every other copy gets a cleared union.
 */
static inline void ptshare_vma_init_from(struct vm_area_struct *dest,
					 const struct vm_area_struct *src)
{
	dest->__vm_ptshare_desc = vma_is_ptshare_client(src) ?
				  READ_ONCE(src->__vm_ptshare_desc) : NULL;
}

/* dup_mm() copies the parent's mm_struct: start outside any domain. */
static inline void ptshare_mm_init(struct mm_struct *mm)
{
	RCU_INIT_POINTER(mm->ptshare_desc, NULL);
	refcount_set(&mm->ptshare_vmas, 0);
}

#else /* !CONFIG_PTSHARE */
static inline bool vma_is_ptshare_client(const struct vm_area_struct *vma)
{
	return false;
}

static inline bool vma_shares_pagetables(const struct vm_area_struct *vma)
{
	return false;
}

static inline bool mm_is_ptshare_shadow(const struct mm_struct *mm)
{
	return false;
}

static inline bool vma_is_ptshare_shadow(const struct vm_area_struct *vma)
{
	return false;
}

static inline struct ptshare_desc *vma_ptshare_desc(const struct vm_area_struct *vma)
{
	return NULL;
}

static inline void vma_set_ptshare_desc(struct vm_area_struct *vma,
					struct ptshare_desc *desc)
{
}

static inline refcount_t *vma_ptshare_refcount(struct vm_area_struct *vma)
{
	return NULL;
}

static inline void ptshare_vma_init_from(struct vm_area_struct *dest,
					 const struct vm_area_struct *src)
{
}

static inline void ptshare_mm_init(struct mm_struct *mm)
{
}

#endif /* CONFIG_PTSHARE */

#endif /* !__ASSEMBLY__ */

#endif /* _LINUX_PTSHARE_H */
