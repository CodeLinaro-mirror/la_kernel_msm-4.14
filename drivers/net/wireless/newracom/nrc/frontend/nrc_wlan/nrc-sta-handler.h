/*
 *
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC Station Handler Definitions
 *
 * This header defines structures and macros for station state handlers
 * using explicit array-based registration.
 */

#ifndef _NRC_STA_HANDLER_H_
#define _NRC_STA_HANDLER_H_

#include <linux/types.h>
#include <net/mac80211.h>

/**
 * struct nrc_sta_handler - Station state change handler
 * @sta_state: Callback for station state transitions
 *
 * Handlers are called when station state changes
 * (e.g., authentication, association, authorization).
 */
struct nrc_sta_handler {
	int (*sta_state)(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			 struct ieee80211_sta *sta,
			 enum ieee80211_sta_state old_state,
			 enum ieee80211_sta_state new_state);
};

/* Handler arrays - defined in nrc-mac80211.c */
extern const struct nrc_sta_handler nrc_sta_handlers[];
extern const int nrc_sta_handlers_count;

#endif /* _NRC_STA_HANDLER_H_ */
