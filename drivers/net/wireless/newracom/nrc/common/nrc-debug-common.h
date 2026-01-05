/*
 * Copyright (c) 2016-2024 Newracom, Inc.
 *
 * NRC Debug Interface - Unified debug macros for all modules
 */

#ifndef _NRC_DEBUG_COMMON_H_
#define _NRC_DEBUG_COMMON_H_

/* Linux kernel headers */
#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/kernel.h>

/* Forward declarations */
struct hif;

/* Debug level enum - priority order from highest to lowest */
enum NRC_DEBUG_LEVEL {
	NRC_DBG_LEVEL_ERR = 0, /* Error messages - always shown */
	NRC_DBG_LEVEL_WARN = 1, /* Warning messages */
	NRC_DBG_LEVEL_INFO = 2, /* Information messages */
	NRC_DBG_LEVEL_DBG = 3, /* Debug messages - only in DEBUG builds */
	NRC_DBG_LEVEL_MAX
};

/* Default debug level: INFO in production, DBG when DEBUG is defined */
#if defined(DEBUG)
#define DEFAULT_NRC_DBG_LEVEL NRC_DBG_LEVEL_DBG
#else
#define DEFAULT_NRC_DBG_LEVEL NRC_DBG_LEVEL_INFO
#endif

/* Common debug masks for all modules - category filtering */
enum NRC_DEBUG_MASK {
	NRC_DBG_BASIC = 0,
	NRC_DBG_HIF = 1,
	NRC_DBG_WIM = 2,
	NRC_DBG_TX = 3,
	NRC_DBG_RX = 4,
	NRC_DBG_MAC = 5,
	NRC_DBG_CAPI = 6,
	NRC_DBG_PS = 7,
	NRC_DBG_STATS = 8,
	NRC_DBG_STATE = 9,
	NRC_DBG_FW = 10,
	NRC_DBG_AMPDU = 11,
	NRC_DBG_CREDIT = 12,
	NRC_DBG_SLOT = 13,
	NRC_DBG_BUS = 14,
};

#define NRC_DBG_MASK_ANY (0xFFFFFFFF)
#define DEFAULT_NRC_DBG_MASK_ALL (NRC_DBG_MASK_ANY)
#define DEFAULT_NRC_DBG_MASK (BIT(NRC_DBG_BASIC) | BIT(NRC_DBG_STATE))

/* Debug print flags - can be overridden per module */
#ifndef NRC_DBG_PRINT_FRAME_TX
#define NRC_DBG_PRINT_FRAME_TX 0
#endif

#ifndef NRC_DBG_PRINT_FRAME_RX
#define NRC_DBG_PRINT_FRAME_RX 0
#endif

#ifndef NRC_DBG_PRINT_ARP_FRAME
#define NRC_DBG_PRINT_ARP_FRAME 0
#endif

/* Level-based debug macros with category prefix */
/* DBG level macros - detailed debug information (only in DEBUG builds) */
#define DBG_HIF(fmt, ...) \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_HIF, "Hif " fmt, ##__VA_ARGS__)
#define DBG_WIM(fmt, ...) \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_WIM, "Wim " fmt, ##__VA_ARGS__)
#define DBG_TX(fmt, ...) \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_TX, "Tx " fmt, ##__VA_ARGS__)
#define DBG_RX(fmt, ...) \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_RX, "Rx " fmt, ##__VA_ARGS__)
#define DBG_MAC(fmt, ...) \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_MAC, "Mac " fmt, ##__VA_ARGS__)
#define DBG_CAPI(fmt, ...)                                          \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_CAPI, "Capi " fmt, \
		      ##__VA_ARGS__)
#define DBG_PS(fmt, ...) \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_PS, "Ps " fmt, ##__VA_ARGS__)
#define DBG_STS(fmt, ...)                                           \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_STATS, "Sts " fmt, \
		      ##__VA_ARGS__)
#define DBG_ST(fmt, ...)                                           \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_STATE, "St " fmt, \
		      ##__VA_ARGS__)
#define DBG_FW(fmt, ...) \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_FW, "Fw " fmt, ##__VA_ARGS__)
#define DBG_AMPDU(fmt, ...)                                           \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_AMPDU, "Ampdu " fmt, \
		      ##__VA_ARGS__)
#define DBG_CREDIT(fmt, ...)                                            \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_CREDIT, "Credit " fmt, \
		      ##__VA_ARGS__)
#define DBG_SLOT(fmt, ...)                                          \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_SLOT, "Slot " fmt, \
		      ##__VA_ARGS__)
#define DBG_BUS(fmt, ...) \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_BUS, "Bus " fmt, ##__VA_ARGS__)

/* Multi-mask debug macros - output if ANY of the masks are enabled */
#define DBG_MULTI(masks, fmt, ...) \
	nrc_dbg_level_multi(NRC_DBG_LEVEL_DBG, masks, fmt, ##__VA_ARGS__)

/* Convenience macros for common combinations */
#define DBG_TX_CREDIT(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_TX) | BIT(NRC_DBG_CREDIT), fmt, ##__VA_ARGS__)

#define DBG_RX_CREDIT(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_RX) | BIT(NRC_DBG_CREDIT), fmt, ##__VA_ARGS__)

#define DBG_TX_SLOT(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_TX) | BIT(NRC_DBG_SLOT), fmt, ##__VA_ARGS__)

#define DBG_RX_SLOT(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_RX) | BIT(NRC_DBG_SLOT), fmt, ##__VA_ARGS__)

#define DBG_TX_MAC(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_TX) | BIT(NRC_DBG_MAC), fmt, ##__VA_ARGS__)

#define DBG_RX_MAC(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_RX) | BIT(NRC_DBG_MAC), fmt, ##__VA_ARGS__)

#define DBG_HIF_TX(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_HIF) | BIT(NRC_DBG_TX), fmt, ##__VA_ARGS__)

#define DBG_HIF_RX(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_HIF) | BIT(NRC_DBG_RX), fmt, ##__VA_ARGS__)

#define DBG_HIF_FW(fmt, ...) \
	DBG_MULTI(BIT(NRC_DBG_HIF) | BIT(NRC_DBG_FW), fmt, ##__VA_ARGS__)

/* General debug macro - basic category */
#define DBG(fmt, ...)                                               \
	nrc_dbg_level(NRC_DBG_LEVEL_DBG, NRC_DBG_BASIC, "Dbg " fmt, \
		      ##__VA_ARGS__)

/* INFO level macros - informational messages (shown by default) */
#define INFO(fmt, ...)                                                \
	nrc_dbg_level(NRC_DBG_LEVEL_INFO, NRC_DBG_BASIC, "Info " fmt, \
		      ##__VA_ARGS__)

/* WARN level macros - warning messages with function name and line number */
#define WARn(category, fmt, ...)                                       \
	nrc_dbg_warn("Warning [" category "] %s:%d " fmt "", __func__, \
		     __LINE__, ##__VA_ARGS__)

/* ERR level macros - error messages with function name and line number (always shown) */
#define ERR(category, fmt, ...)                                               \
	nrc_dbg_err("Error [" category "] %s:%d " fmt "", __func__, __LINE__, \
		    ##__VA_ARGS__)

/* Category-specific warning macros */
#define WARN_WIM(fmt, ...) WARn("Wim", fmt, ##__VA_ARGS__)
#define WARN_HIF(fmt, ...) WARn("Hif", fmt, ##__VA_ARGS__)
#define WARN_FW(fmt, ...) WARn("Fw", fmt, ##__VA_ARGS__)
#define WARN_INIT(fmt, ...) WARn("Init", fmt, ##__VA_ARGS__)
#define WARN_BD(fmt, ...) WARn("Bd", fmt, ##__VA_ARGS__)
#define WARN_PS(fmt, ...) WARn("Ps", fmt, ##__VA_ARGS__)
#define WARN_TX(fmt, ...) WARn("Tx", fmt, ##__VA_ARGS__)
#define WARN_RX(fmt, ...) WARn("Rx", fmt, ##__VA_ARGS__)
#define WARN_SPI(fmt, ...) WARn("Spi", fmt, ##__VA_ARGS__)
#define WARN_CB(fmt, ...) WARn("Cb", fmt, ##__VA_ARGS__)
#define WARN_HAL(fmt, ...) WARn("Hal", fmt, ##__VA_ARGS__)
#define WARN_WLAN(fmt, ...) WARn("Wlan", fmt, ##__VA_ARGS__)
#define WARN_MCP(fmt, ...) WARn("Mcp", fmt, ##__VA_ARGS__)

/* Category-specific error macros */
#define ERR_WIM(fmt, ...) ERR("Wim", fmt, ##__VA_ARGS__)
#define ERR_HIF(fmt, ...) ERR("Hif", fmt, ##__VA_ARGS__)
#define ERR_FW(fmt, ...) ERR("Fw", fmt, ##__VA_ARGS__)
#define ERR_INIT(fmt, ...) ERR("Init", fmt, ##__VA_ARGS__)
#define ERR_BD(fmt, ...) ERR("Bd", fmt, ##__VA_ARGS__)
#define ERR_PS(fmt, ...) ERR("Ps", fmt, ##__VA_ARGS__)
#define ERR_TX(fmt, ...) ERR("Tx", fmt, ##__VA_ARGS__)
#define ERR_RX(fmt, ...) ERR("Rx", fmt, ##__VA_ARGS__)
#define ERR_SPI(fmt, ...) ERR("Spi", fmt, ##__VA_ARGS__)
#define ERR_CB(fmt, ...) ERR("Cb", fmt, ##__VA_ARGS__)
#define ERR_HAL(fmt, ...) ERR("Hal", fmt, ##__VA_ARGS__)
#define ERR_WLAN(fmt, ...) ERR("Wlan", fmt, ##__VA_ARGS__)
#define ERR_MCP(fmt, ...) ERR("Mcp", fmt, ##__VA_ARGS__)

/* MAC address formatting macros */
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

/* Global debug variables - each module should define these */
extern unsigned long nrc_debug_mask;
extern enum NRC_DEBUG_LEVEL nrc_debug_level;
extern struct device *g_dev;

/* Core debug functions - inline implementations for common use */
static inline void nrc_dbg_init(struct device *dev)
{
	nrc_debug_mask = DEFAULT_NRC_DBG_MASK;
	nrc_debug_level = DEFAULT_NRC_DBG_LEVEL;
	g_dev = dev;
}

static inline void nrc_dbg_enable(enum NRC_DEBUG_MASK mk)
{
	set_bit(mk, &nrc_debug_mask);
}

static inline void nrc_dbg_disable(enum NRC_DEBUG_MASK mk)
{
	clear_bit(mk, &nrc_debug_mask);
}

static inline void nrc_dbg_set_level(enum NRC_DEBUG_LEVEL level)
{
	if (level < NRC_DBG_LEVEL_MAX)
		nrc_debug_level = level;
}

static inline enum NRC_DEBUG_LEVEL nrc_dbg_get_level(void)
{
	return nrc_debug_level;
}

static inline void nrc_hal_set_debug_mask(unsigned long mask)
{
	nrc_debug_mask = mask;
}

static inline void nrc_set_debug_mask(unsigned long mask)
{
	nrc_debug_mask = mask;
}

/* Warning function - shown based on level, no mask check */
static inline void nrc_dbg_warn(const char *fmt, ...)
{
	va_list args;
	int i;
	static char buf[512] = {
		0,
	};

	/* WARN level messages: only check if level allows WARN */
	if (NRC_DBG_LEVEL_WARN > nrc_debug_level)
		return;

	/* No category mask check for warnings - they should be shown based on level only */

	va_start(args, fmt);
	if (fmt != NULL) {
		i = vsnprintf(buf, sizeof(buf), fmt, args);
	} else {
		strcpy(buf, "Format string is NULL !!!");
		i = strlen(buf);
	}
	va_end(args);

	if (g_dev == NULL)
		pr_warn("%s\n", buf); /* Use pr_warn for warnings */
	else
		dev_warn(g_dev, "%s\n", buf); /* Use dev_warn for warnings */
}

/* Error function - always shown, no mask check (only level check) */
static inline void nrc_dbg_err(const char *fmt, ...)
{
	va_list args;
	int i;
	static char buf[512] = {
		0,
	};

	/* ERR level messages: only check if level allows ERR (should always pass) */
	if (NRC_DBG_LEVEL_ERR > nrc_debug_level)
		return;

	/* No category mask check for errors - they should always be shown */

	va_start(args, fmt);
	if (fmt != NULL) {
		i = vsnprintf(buf, sizeof(buf), fmt, args);
	} else {
		strcpy(buf, "Format string is NULL !!!");
		i = strlen(buf);
	}
	va_end(args);

	if (g_dev == NULL)
		pr_err("%s\n", buf); /* Use pr_err for errors */
	else
		dev_err(g_dev, "%s\n", buf); /* Use dev_err for errors */
}

/* Main nrc_dbg_level function - with level and category filtering */
static inline void nrc_dbg_level(enum NRC_DEBUG_LEVEL level,
				 enum NRC_DEBUG_MASK mk, const char *fmt, ...)
{
	va_list args;
	int i;
	static char buf[512] = {
		0,
	};

	/* Check debug level first - skip if message level is higher than current level */
	if (level > nrc_debug_level)
		return;

	/* Then check category mask */
	if (!test_bit(mk, &nrc_debug_mask))
		return;

	va_start(args, fmt);
	if (fmt != NULL) {
		i = vsnprintf(buf, sizeof(buf), fmt, args);
	} else {
		strcpy(buf, "Format string is NULL !!!");
		i = strlen(buf);
	}
	va_end(args);

	if (g_dev == NULL)
		pr_info("%s\n", buf);
	else
		dev_info(g_dev, "%s\n", buf);
}

/* Multi-mask debug function - allows multiple category masks */
static inline void nrc_dbg_level_multi(enum NRC_DEBUG_LEVEL level,
				       unsigned long masks, const char *fmt,
				       ...)
{
	va_list args;
	int i, pos = 0;
	static char buf[512] = {
		0,
	};
	char prefix[64] = {0};
	bool matched = false;
	int count = 0;

	/* Check debug level first */
	if (level > nrc_debug_level)
		return;

	/* Check if ANY of the provided masks are enabled */
	for (i = 0; i < 32; i++) {
		if ((masks & BIT(i)) && test_bit(i, &nrc_debug_mask)) {
			matched = true;
			break;
		}
	}

	if (!matched)
		return;

	/* Build prefix from all masks in the combination */
	if (masks & BIT(NRC_DBG_HIF)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sHif",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_WIM)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sWim",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_TX)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sTx",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_RX)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sRx",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_MAC)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sMac",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_PS)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sPs",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_AMPDU)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sAmpdu",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_CREDIT)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sCredit",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_SLOT)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sSlot",
				count ? "/" : "");
		count++;
	}
	if (masks & BIT(NRC_DBG_BUS)) {
		pos += snprintf(prefix + pos, sizeof(prefix) - pos, "%sBus",
				count ? "/" : "");
		count++;
	}

	/* Add trailing space */
	if (count > 0)
		snprintf(prefix + pos, sizeof(prefix) - pos, " ");

	/* Format message with prefix */
	va_start(args, fmt);
	if (fmt != NULL) {
		i = snprintf(buf, sizeof(buf), "%s", prefix);
		vsnprintf(buf + i, sizeof(buf) - i, fmt, args);
	} else {
		strcpy(buf, "Format string is NULL !!!");
	}
	va_end(args);

	if (g_dev == NULL)
		pr_info("%s\n", buf);
	else
		dev_info(g_dev, "%s\n", buf);
}

/* Legacy nrc_dbg function - compatibility wrapper (defaults to DBG level) */
static inline void nrc_dbg(enum NRC_DEBUG_MASK mk, const char *fmt, ...)
{
	va_list args;
	int i;
	static char buf[512] = {
		0,
	};

	/* Default to DBG level for legacy compatibility */
	if (NRC_DBG_LEVEL_DBG > nrc_debug_level)
		return;

	if (!test_bit(mk, &nrc_debug_mask))
		return;

	va_start(args, fmt);
	if (fmt != NULL) {
		i = vsnprintf(buf, sizeof(buf), fmt, args);
	} else {
		strcpy(buf, "Format string is NULL !!!");
		i = strlen(buf);
	}
	va_end(args);

	if (g_dev == NULL)
		pr_info("%s\n", buf);
	else
		dev_info(g_dev, "%s\n", buf);
}

/* Loopback debug */
struct lb_time_info {
	int _i;
	s64 _txt;
	s64 _rxt;
};

/* All debug variables are now in struct nrc_debug - access via nw->debug->variable */

enum LOOPBACK_MODE {
	LOOPBACK_MODE_ROUNDTRIP,
	LOOPBACK_MODE_TX_ONLY,
	LOOPBACK_MODE_RX_ONLY,
	LOOPBACK_MODE_MAX
};

/* VALIDATE_HIF_HEADER macro is defined in nrc-hif.h (requires struct hif, HIF_TYPE_MAX) */

#endif /* _NRC_DEBUG_COMMON_H_ */
