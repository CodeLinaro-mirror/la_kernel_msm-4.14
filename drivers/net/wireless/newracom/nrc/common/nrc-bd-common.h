/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC Board Data Definitions - BD structures for TX power and channels
 */

#ifndef _NRC_BD_COMMON_H_
#define _NRC_BD_COMMON_H_

#include <linux/types.h>

/* BD Constants */
#define NRC_BD_MAX_CH_LIST 45

/* BD Structure Definition */
struct bd_supp_param {
	uint8_t num_ch;
	uint8_t s1g_ch_index[NRC_BD_MAX_CH_LIST];
	uint16_t nons1g_ch_freq[NRC_BD_MAX_CH_LIST];
};

#endif /* _NRC_BD_COMMON_H_ */
