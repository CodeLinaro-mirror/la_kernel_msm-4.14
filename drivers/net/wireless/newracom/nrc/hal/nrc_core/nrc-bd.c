/*
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

/* Linux kernel headers */
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/syscalls.h>

/* Assembly headers */
#include <asm/uaccess.h>

/* Common directory headers - Core */
#include "nrc.h"
#include "nrc-hif.h"

/* Common directory headers - Debug & Trace */
#include "nrc-debug-common.h"

/* Common directory headers - Interfaces */
#include "nrc-wim-types.h"

/* Local module headers */
#include "nrc-bd.h"
#include "nrc-country.h"

#if defined(CONFIG_SUPPORT_BD)
#define NRC_BD_HEADER_LENGTH 16
int g_bd_size = 0;


static uint16_t nrc_checksum_16(uint16_t len, uint8_t *buf)
{
	uint32_t checksum = 0;
	int i = 0;

	/* Process 2 bytes at a time; handle odd trailing byte to avoid
	 * uint16_t underflow (len wraps to 65535) and out-of-bounds read.
	 */
	while (len >= 2) {
		checksum += buf[i] + (buf[i + 1] << 8);
		len -= 2;
		i += 2;
	}
	if (len == 1)
		checksum += buf[i];

	checksum = (checksum >> 16) + checksum;

	return checksum;
}

static void *nrc_dump_load(struct nrc_hif_device *hdev, int len)
{
#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
	mm_segment_t old_fs;
#endif
	struct file *filp;
	loff_t pos = 0;
	char filepath[64];
	char *buf;

#ifdef CONFIG_BD_LOAD_ONCE
	if (hdev->bd)
		return hdev->bd;
#endif

#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
#if KERNEL_VERSION(5, 0, 0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs(get_ds());
#elif KERNEL_VERSION(5, 10, 0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#else
	old_fs = force_uaccess_begin();
#endif
#endif /* if KERNEL_VERSION(5,18,0) < NRC_TARGET_KERNEL_VERSION */

	scnprintf(filepath, sizeof(filepath), "/lib/firmware/%s",
		 hdev->params->bd_name);
	filp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		ERR_BD("Failed to load board data, error:%ld", PTR_ERR(filp));
#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
#if KERNEL_VERSION(5, 10, 0) > NRC_TARGET_KERNEL_VERSION
		set_fs(old_fs);
#else
		force_uaccess_end(old_fs);
#endif
#endif
		return NULL;
	}

	buf = kmalloc(len, GFP_KERNEL);
	if (!buf) {
		ERR_BD("failed to allocate BD buffer");
		filp_close(filp, NULL);
#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
#if KERNEL_VERSION(5, 10, 0) > NRC_TARGET_KERNEL_VERSION
		set_fs(old_fs);
#else
		force_uaccess_end(old_fs);
#endif
#endif
		return NULL;
	}

#if KERNEL_VERSION(4, 14, 0) <= NRC_TARGET_KERNEL_VERSION
	kernel_read(filp, buf, len, &pos);
#else
	kernel_read(filp, pos, buf, len);
#endif

	filp_close(filp, NULL);
#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
#if KERNEL_VERSION(5, 10, 0) > NRC_TARGET_KERNEL_VERSION
	set_fs(old_fs);
#else
	force_uaccess_end(old_fs);
#endif
#endif
	return buf;
}

struct wim_bd_param *nrc_read_bd_tx_pwr(struct nrc_hif_device *hdev,
					uint8_t *country_code)
{
	enum nrc_country_id nrc_cc;
	uint8_t cc_index;
	uint16_t len = 0;
	uint8_t type = 0;
	int i, j;
	struct BDF *bd;
	struct wim_bd_param *bd_sel;
	bool check_bd_flag = false;
	uint16_t target_version;

	if (!hdev || !country_code) {
		ERR_BD("invalid argument: hdev=%p cc=%p", hdev, country_code);
		return NULL;
	}

	if (!g_bd_size)
		return NULL;

	DBG_BD("size of bd file is %d", g_bd_size);

	/*
	 * KR must be resolved to K1/K2 before the alpha-2 table lookup
	 * because that decision requires the kr_band module parameter.
	 */
	if (country_code[0] == 'K' && country_code[1] == 'R')
		country_code[1] = (hdev->params->kr_band == 1) ? '1' : '2';

	nrc_cc = nrc_cc_from_alpha2(country_code);

	/* Normalize EU member codes to "EU" for firmware reporting */
	if (nrc_cc == NRC_CC_EU &&
	    !(country_code[0] == 'E' && country_code[1] == 'U')) {
		country_code[0] = 'E';
		country_code[1] = 'U';
	}

	cc_index = nrc_cc_bd_idx[nrc_cc];
	if (!cc_index) {
		/* No dedicated BD entry; fall back to US TX power */
		DBG_STATE("[BD] Country (%c%c) has no BD entry; using US BD as fallback",
			  country_code[0], country_code[1]);
		cc_index = nrc_cc_bd_idx[NRC_CC_US];
	}

	bd = nrc_dump_load(hdev, g_bd_size);
	if (!bd) {
		ERR_BD("bd is NULL");
		return NULL;
	}

	DBG_BD("Major %02X Minor %02X Total len %04X Num_Data_Groups %04X Checksum %04X",
	       bd->ver_major, bd->ver_minor, bd->total_len, bd->num_data_groups,
	       bd->checksum_data);

	bd_sel = kzalloc(sizeof(*bd_sel), GFP_KERNEL);
	if (!bd_sel) {
		ERR_BD("bd_sel alloc failed");
		kfree(bd);
		return NULL;
	}

	target_version = hdev->fw.info.hw_version;
	if (target_version > 0x7FF)
		target_version = 0;

	for (i = 0; i < bd->num_data_groups; i++) {
		type = bd->data[len + 4 * i];
		if (type == cc_index) {
			bd_sel->type = (uint16_t)type;
			bd_sel->hw_version =
				(uint16_t)(bd->data[6 + len + 4 * i] +
					   (bd->data[7 + len + 4 * i] << 8));

			if (target_version == bd_sel->hw_version) {
				bd_sel->length =
					(uint16_t)(bd->data[2 + len + 4 * i] +
						   (bd->data[3 + len + 4 * i] << 8));
				bd_sel->checksum =
					(uint16_t)(bd->data[4 + len + 4 * i] +
						   (bd->data[5 + len + 4 * i] << 8));

				for (j = 0; j < bd_sel->length - 2 &&
					    j < WIM_MAX_BD_DATA_LEN; j++)
					bd_sel->value[j] =
						bd->data[8 + len + 4 * i + j];

				check_bd_flag = true;
				DBG_BD("type %04X len %04X checksum %04X hw_ver %04X",
				       bd_sel->type, bd_sel->length,
				       bd_sel->checksum, bd_sel->hw_version);
				break;
			}
		}
		len += (uint16_t)(bd->data[2 + len + 4 * i] +
				  (bd->data[3 + len + 4 * i] << 8));
	}

	if (check_bd_flag)
		DBG_BD("[BD] HW version matched (%u)", target_version);
	else
		ERR_BD("[BD] HW version not matched (%u)", target_version);

#ifdef CONFIG_BD_LOAD_ONCE
	hdev->bd = bd;
#else
	kfree(bd);
#endif

	if (check_bd_flag)
		return bd_sel;

	kfree(bd_sel);
	return NULL;
}

int nrc_check_bd(struct nrc_hif_device *hdev)
{
	struct BDF *bd;
	struct file *filp;
	loff_t pos = 0;
	struct kstat *stat;
	char *buf;
	size_t length;
	int ret;
	char filepath[64];
#if KERNEL_VERSION(5, 10, 0) <= NRC_TARGET_KERNEL_VERSION
	int rc;
#endif
#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
	mm_segment_t old_fs;
#endif

	if (!hdev || !hdev->params) {
		ERR_BD("invalid argument: hdev=%p", hdev);
		return -EINVAL;
	}

#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
#if KERNEL_VERSION(5, 0, 0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs(get_ds());
#elif KERNEL_VERSION(5, 10, 0) > NRC_TARGET_KERNEL_VERSION
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#else
	old_fs = force_uaccess_begin();
#endif
#endif /* if KERNEL_VERSION(5,18,0) < NRC_TARGET_KERNEL_VERSION */

	scnprintf(filepath, sizeof(filepath), "/lib/firmware/%s",
		 hdev->params->bd_name);
	filp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		ERR_BD("Failed to load board data (%s): error %ld", filepath,
		       PTR_ERR(filp));
#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
#if KERNEL_VERSION(5, 10, 0) > NRC_TARGET_KERNEL_VERSION
		set_fs(old_fs);
#else
		force_uaccess_end(old_fs);
#endif
#endif
		return -EIO;
	}

	stat = kmalloc(sizeof(*stat), GFP_KERNEL);
	if (!stat) {
		filp_close(filp, NULL);
		return -ENOMEM;
	}

#if KERNEL_VERSION(5, 10, 0) <= NRC_TARGET_KERNEL_VERSION
	rc = vfs_getattr(&filp->f_path, stat, STATX_SIZE, AT_STATX_SYNC_AS_STAT);
	if (rc != 0)
		ERR_BD("vfs_getattr error (%d)", rc);
#else
	vfs_stat(filepath, stat);
#endif
	length = (size_t)stat->size;
	kfree(stat);

	buf = kmalloc((int)length, GFP_KERNEL);
	if (!buf) {
		filp_close(filp, NULL);
		ERR_BD("failed to allocate BD buffer");
		return -ENOMEM;
	}

#if KERNEL_VERSION(4, 14, 0) <= NRC_TARGET_KERNEL_VERSION
	g_bd_size = kernel_read(filp, buf, (int)length, &pos);
#else
	g_bd_size = kernel_read(filp, pos, buf, (int)length);
#endif

	filp_close(filp, NULL);
#if KERNEL_VERSION(5, 18, 0) > NRC_TARGET_KERNEL_VERSION
#if KERNEL_VERSION(5, 10, 0) > NRC_TARGET_KERNEL_VERSION
	set_fs(old_fs);
#else
	force_uaccess_end(old_fs);
#endif
#endif

	if (g_bd_size < NRC_BD_HEADER_LENGTH) {
		ERR_BD("Invalid data size (%d)", g_bd_size);
		kfree(buf);
		return -EINVAL;
	}

	bd = (struct BDF *)buf;
	if ((bd->total_len > g_bd_size - NRC_BD_HEADER_LENGTH) ||
	    (bd->total_len < NRC_BD_HEADER_LENGTH)) {
		ERR_BD("Invalid total length (%d)", bd->total_len);
		kfree(buf);
		return -EINVAL;
	}

	ret = nrc_checksum_16(bd->total_len, (uint8_t *)&bd->data[0]);
	if (bd->checksum_data != ret) {
		ERR_BD("Checksum mismatch (expected %u, got %u)",
		       bd->checksum_data, ret);
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);
	return 0;
}
#endif /* #if defined(CONFIG_SUPPORT_BD) */
