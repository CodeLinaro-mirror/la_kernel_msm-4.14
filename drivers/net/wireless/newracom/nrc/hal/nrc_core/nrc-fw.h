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

#ifndef _NRC_FW_H_
#define _NRC_FW_H_

#define DEFAULT_FIRMWARE_NAME "uni.bin"

#define ROM_CHUNK_SIZE (1024) /*(2*1024)*/
#define ROM_FRAG_BYTES (ROM_CHUNK_SIZE - 16)

#define XIP_CHUNK_SIZE (TX_SLOT_SIZE - 16)
#define XIP_FRAG_BYTES (XIP_CHUNK_SIZE - 16)

struct fw_frag_hdr {
	u32 eof;
	u32 address;
	u32 len;
} __packed;

struct fw_frag {
	struct fw_frag_hdr hdr;
	union {
		struct {
			u8 payload[ROM_FRAG_BYTES];
			u32 checksum;
		} rom;
		struct {
			u8 payload[XIP_FRAG_BYTES];
			u32 checksum;
		} xip;
	};
} __packed;

struct nrc_fw_priv {
	struct firmware *fw;
	struct fw_frag_hdr frag_hdr;
	const u8 *fw_data_pos;
	int remain_bytes;
	int num_chunks;
	int cur_chunk; /* index of chunk to be transferred */
	u8 index;
	u32 index_fb;
	uint32_t start_addr;
	bool ack;
	bool csum;
};

/* ========================================
 * Public API - External Interface
 * ======================================== */

/* FW module lifecycle */
struct nrc_fw_priv *nrc_fw_alloc(void);
void nrc_fw_cleanup(struct nrc_fw_priv *priv);

/**
 * FW Loading Operations
 *
 * nrc_fw_load() - Initial firmware load (driver initialization)
 *   - Downloads firmware binary to device RAM
 *   - Performs optional XIP flash fusing if fw_update_name is set
 *   - Called once during driver initialization
 *   - Use case: insmod nrc_wlan.ko
 *
 * nrc_fw_reload() - Firmware reload (power save wake)
 *   - Lightweight RAM download only, no XIP fusing
 *   - Called when device wakes from deep sleep
 *   - Use case: PS wake after TARGET_NOTI_REQUEST_FW_DOWNLOAD
 *
 * nrc_fw_fusing() - Flash fusing mode (manufacturing/recovery)
 *   - Downloads DL (Download Loader) first
 *   - Then writes FW and bootloader to flash
 *   - Called via separate entry point (nrc_nw_start_fusing)
 *   - Use case: Factory flash programming
 */
int nrc_fw_load(struct nrc_hif_device *hdev);
int nrc_fw_reload(struct nrc_hif_device *hdev);
int nrc_fw_fusing(struct nrc_hif_device *hdev);

/* FW runtime operations */
int nrc_fw_start(struct nrc_hif_device *hdev);

#endif
