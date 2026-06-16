/*
 *
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC TX/RX Handler Definitions
 *
 * This header defines structures and macros for TX/RX frame handlers
 * using explicit array-based registration.
 */

#ifndef _NRC_TRX_HANDLER_H_
#define _NRC_TRX_HANDLER_H_

#include <linux/types.h>
#include <linux/list.h>
#include <net/mac80211.h>

/* Forward declarations */
struct nrc;
struct sk_buff;

/**
 * struct nrc_trx_data - TX/RX handler context data
 * @nw: NRC device structure
 * @vif: Virtual interface
 * @sta: Station (may be NULL)
 * @skb: Socket buffer being processed
 * @result: Handler result code
 */
struct nrc_trx_data {
	struct nrc *nw;
	struct ieee80211_vif *vif;
	struct ieee80211_sta *sta;
	struct sk_buff *skb;
	int result;
};

/**
 * struct nrc_trx_handler - TX/RX handler registration structure
 * @handler: Handler function pointer
 * @vif_types: Bitmask of supported VIF types (NL80211_IFTYPE_*)
 */
struct nrc_trx_handler {
	int (*handler)(struct nrc_trx_data *data);
	u32 vif_types;
};

/* Match all VIF types */
#define NL80211_IFTYPE_ALL (BIT(NUM_NL80211_IFTYPES) - 1)

/* Handler arrays - defined in nrc-trx.c */
extern const struct nrc_trx_handler nrc_tx_handlers[];
extern const struct nrc_trx_handler nrc_rx_handlers[];
extern const int nrc_tx_handlers_count;
extern const int nrc_rx_handlers_count;

#endif /* _NRC_TRX_HANDLER_H_ */
