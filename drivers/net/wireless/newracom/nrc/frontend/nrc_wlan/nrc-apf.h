/*
 *
 * Copyright (c) 2016-2024 Newracom, Inc.
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

#ifndef __NRC_APF_H__
#define __NRC_APF_H__


int nrc_apf_get_enable (struct nrc *nw, int *enable);
int nrc_apf_set_enable (struct nrc *nw, int enable);
int nrc_apf_get_version (struct nrc *nw);
int nrc_apf_get_maxlen (struct nrc *nw);
int nrc_apf_set_packet_filter (struct nrc *nw, u8 *program, size_t len);
int nrc_apf_read_packet_filter (struct nrc *nw, u32 src_offset, u8 *host_dst, u32 len);

void nrc_apf_debugfs_init (struct dentry *root, void *nw);

#endif
