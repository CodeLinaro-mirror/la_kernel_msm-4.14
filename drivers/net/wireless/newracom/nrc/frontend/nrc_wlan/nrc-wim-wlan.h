/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * WLAN-specific WIM (Wireless Interface Message) Functions
 *
 * This file contains WLAN-specific WIM command builders and handlers.
 * These functions use the core WIM infrastructure (nrc_wim_alloc_skb,
 * nrc_wim_skb_add_tlv, nrc_wim_request) to construct and send
 * WLAN-specific commands to firmware.
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

#ifndef _NRC_WIM_WLAN_H_
#define _NRC_WIM_WLAN_H_

#include <linux/types.h>
#include <net/mac80211.h>

/* Forward declarations */
struct nrc_hif_device;
struct ieee80211_vif;
struct ieee80211_sta;
struct ieee80211_key_conf;
struct cfg80211_scan_request;
struct ieee80211_scan_ies;
struct cfg80211_sched_scan_request;

/* WIM AMPDU action enum - matches firmware definition */
enum WIM_AMPDU_ACTION;

/**
 * Station management
 */
int nrc_wim_wlan_change_sta(struct ieee80211_vif *vif, struct ieee80211_sta *sta,
			    u8 cmd, bool sleep);

int nrc_wim_wlan_set_sta_type(struct ieee80211_vif *vif);
int nrc_wim_wlan_unset_sta_type(struct ieee80211_vif *vif);

/**
 * MAC address management
 */
int nrc_wim_wlan_set_mac_addr(struct ieee80211_vif *vif);
int nrc_wim_wlan_set_p2p_addr(struct ieee80211_vif *vif);

/**
 * Scan operations
 */
int nrc_wim_wlan_hw_scan(struct ieee80211_vif *vif,
			 struct cfg80211_scan_request *req,
			 struct ieee80211_scan_ies *ies);

int nrc_wim_wlan_sched_scan_start(struct ieee80211_vif *vif,
				  struct cfg80211_sched_scan_request *req,
				  struct ieee80211_scan_ies *ies);

int nrc_wim_wlan_sched_scan_stop(struct ieee80211_vif *vif);

/**
 * Key management
 */
enum wim_cipher_type nrc_wim_wlan_to_wim_cipher_type(u32 cipher);
u32 nrc_wim_wlan_to_ieee80211_cipher(enum wim_cipher_type cipher);

int nrc_wim_wlan_install_key(enum set_key_cmd cmd, struct ieee80211_vif *vif,
			     struct ieee80211_sta *sta,
			     struct ieee80211_key_conf *key);

/**
 * AMPDU operations
 */
int nrc_wim_wlan_ampdu_action(struct ieee80211_vif *vif,
			      enum WIM_AMPDU_ACTION action,
			      struct ieee80211_sta *sta, u16 tid);

/**
 * TSF operations
 */
u64 nrc_wim_wlan_get_tsf(struct ieee80211_vif *vif);

/**
 * APF (Android Packet Filter) operations
 */
int nrc_wim_wlan_apf_get_enable(struct nrc_hif_device *hdev, int *enable);
int nrc_wim_wlan_apf_set_enable(struct nrc_hif_device *hdev, int enable);
u32 nrc_wim_wlan_apf_get_version(struct nrc_hif_device *hdev);
u32 nrc_wim_wlan_apf_get_maxlen(struct nrc_hif_device *hdev);
int nrc_wim_wlan_apf_set_packet_filter(struct nrc_hif_device *hdev, u8 *program,
				       size_t len);
int nrc_wim_wlan_apf_get_packet_filter(struct nrc_hif_device *hdev,
				       u32 src_offset, u8 *host_dst, u32 len);

#endif /* _NRC_WIM_WLAN_H_ */
