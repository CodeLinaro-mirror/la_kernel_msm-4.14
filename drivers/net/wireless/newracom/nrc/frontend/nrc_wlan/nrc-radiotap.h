/*
 *
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC Radiotap Header Definitions
 *
 * This header defines radiotap structures used for monitor mode
 * and packet capture functionality.
 */

#ifndef _NRC_RADIOTAP_H_
#define _NRC_RADIOTAP_H_

#include <linux/types.h>
#include <net/ieee80211_radiotap.h>

/**
 * struct nrc_radiotap_hdr - Standard radiotap header for S1G frames
 */
struct nrc_radiotap_hdr {
	struct ieee80211_radiotap_header hdr;
	uint64_t rt_tsft;		/* IEEE80211_RADIOTAP_TSFT */
	uint8_t rt_flags;		/* IEEE80211_RADIOTAP_FLAGS */
	uint8_t rt_pad;			/* pad for IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_frequency;	/* IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_flags;
	uint16_t rt_pad2;		/* pad for IEEE80211_RADIOTAP_TLV:S1G */
	uint16_t rt_tlv_type;		/* IEEE80211_RADIOTAP_TLV */
	uint16_t rt_tlv_length;
	uint16_t rt_s1g_known;
	uint16_t rt_s1g_data1;
	uint16_t rt_s1g_data2;
} __packed;

/**
 * struct nrc_radiotap_hdr_agg - Radiotap header for aggregated frames
 */
struct nrc_radiotap_hdr_agg {
	struct ieee80211_radiotap_header hdr;
	uint64_t rt_tsft;		/* IEEE80211_RADIOTAP_TSFT */
	uint8_t rt_flags;		/* IEEE80211_RADIOTAP_FLAGS */
	uint8_t rt_pad;			/* pad for IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_frequency;	/* IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_flags;
	uint16_t rt_pad2;		/* pad for IEEE80211_RADIOTAP_AMPDU_STATUS */
	uint32_t rt_ampdu_ref;		/* IEEE80211_RADIOTAP_AMPDU_STATUS */
	uint16_t rt_ampdu_flags;
	uint8_t rt_ampdu_crc;
	uint8_t rt_ampdu_reserved;
	uint16_t rt_tlv_type;		/* IEEE80211_RADIOTAP_TLV */
	uint16_t rt_tlv_length;
	uint16_t rt_s1g_known;
	uint16_t rt_s1g_data1;
	uint16_t rt_s1g_data2;
} __packed;

/**
 * struct nrc_radiotap_hdr_ndp - Radiotap header for NDP frames
 */
struct nrc_radiotap_hdr_ndp {
	struct ieee80211_radiotap_header hdr;
	uint64_t rt_tsft;		/* IEEE80211_RADIOTAP_TSFT */
	uint8_t rt_flags;		/* IEEE80211_RADIOTAP_FLAGS */
	uint8_t rt_pad;			/* pad for IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_frequency;	/* IEEE80211_RADIOTAP_CHANNEL */
	uint16_t rt_ch_flags;
	uint8_t rt_rssi;		/* IEEE80211_RADIOTAP_DBM_ANTSIGNAL */
	uint8_t rt_zero_length_psdu;	/* IEEE80211_RADIOTAP_0_LENGTH_PSDU */
} __packed;

/**
 * struct nrc_local_radiotap_hdr - Local radiotap header for TX injection
 */
struct nrc_local_radiotap_hdr {
	struct ieee80211_radiotap_header hdr;
	__le64 rt_tsft;
	u8 rt_flags;
	u8 rt_rate;
	__le16 rt_channel;
	__le16 rt_chbitmask;
} __packed;

/**
 * struct nrc_tx_stats - TX statistics structure
 */
struct nrc_tx_stats {
	uint32_t mcs;
	uint32_t bw;
	uint32_t gi;
};

#endif /* _NRC_RADIOTAP_H_ */
