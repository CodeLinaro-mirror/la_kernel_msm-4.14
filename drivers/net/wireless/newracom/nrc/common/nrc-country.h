#ifndef _NRC_COUNTRY_H_
#define _NRC_COUNTRY_H_

#include <linux/types.h>

/*
 * NRC7394 country/regulatory-domain definitions — single source of truth.
 *
 * All country-code knowledge for the entire driver lives here:
 *   - enum nrc_country_id   sequential index used as array subscript
 *   - nrc_cc_alpha2[]       ISO 3166-1 alpha-2 string per country
 *   - nrc_cc_bd_idx[]       BD binary index per country (HAL TX-power lookup)
 *   - eu_countries_cc[]     EU regulatory group member list
 *   - country_match()       NULL-terminated list search
 *   - nrc_cc_from_alpha2()  alpha-2 → enum resolver
 *
 * Adding a new country:
 *   1. Append a new NRC_CC_XX entry before NRC_CC_MAX in the enum.
 *   2. Add the alpha-2 string to nrc_cc_alpha2[].
 *   3. Add the BD binary index to nrc_cc_bd_idx[] (0 = no entry → US fallback).
 *   4. Add the S1G channel/proxy tables in frontend/nrc_wlan/nrc-s1g.c.
 *
 * Virtual country codes (tw_band module parameter selects among TW variants):
 *   TW  (tw_band=-1/1): TW 840 MHz band (T8)
 *   T2  (tw_band=2):    TW NCC 920 MHz band — 920.5–924.5 MHz
 *   T9  (tw_band=9):    TW 920 MHz band    — 921.0–924.0 MHz
 */

/**
 * enum nrc_country_id - NRC7394 regulatory domain identifiers
 *
 * Values are sequential from 0 and used as array indices in the Frontend
 * S1G channel and proxy-map tables.  K0 is a deprecated placeholder kept
 * for historical continuity; it has no channel table or BD entry.
 */
enum nrc_country_id {
	NRC_CC_US = 0,
	NRC_CC_JP,
	NRC_CC_K0,  /* KR: deprecated placeholder, no active channels */
	NRC_CC_K1,  /* KR USN1 921-923 MHz (LBT required) */
	NRC_CC_TW,  /* TW 840 MHz band (T8) */
	NRC_CC_EU,
	NRC_CC_CN,
	NRC_CC_NZ,
	NRC_CC_AU,
	NRC_CC_K2,  /* KR USN5 925-931 MHz (MIC required) */
	NRC_CC_SG,
	NRC_CC_T2,  /* TW NCC 920 MHz — 920.5–924.5 MHz (tw_band=2) */
	NRC_CC_T9,  /* TW 920 MHz     — 921.0–924.0 MHz (tw_band=9) */
	NRC_CC_MAX,
};

/**
 * nrc_cc_alpha2 - canonical alpha-2 string for each country ID
 * Indexed by enum nrc_country_id.
 */
static const char *const nrc_cc_alpha2[NRC_CC_MAX] = {
	[NRC_CC_US] = "US", [NRC_CC_JP] = "JP",
	[NRC_CC_K0] = "K0", [NRC_CC_K1] = "K1",
	[NRC_CC_TW] = "TW", [NRC_CC_EU] = "EU",
	[NRC_CC_CN] = "CN", [NRC_CC_NZ] = "NZ",
	[NRC_CC_AU] = "AU", [NRC_CC_K2] = "K2",
	[NRC_CC_SG] = "SG", [NRC_CC_T2] = "T2",
	[NRC_CC_T9] = "T9",
};

/**
 * nrc_cc_bd_idx - BD binary index for each country ID
 *
 * Values are fixed by the firmware binary format — do NOT change.
 * 0 means no dedicated BD entry; the driver falls back to US TX power.
 */
static const u8 nrc_cc_bd_idx[NRC_CC_MAX] = {
	[NRC_CC_US] = 1,  [NRC_CC_JP] = 2,
	[NRC_CC_K0] = 0,  [NRC_CC_K1] = 3,
	[NRC_CC_TW] = 13, [NRC_CC_EU] = 5,
	[NRC_CC_CN] = 0,  [NRC_CC_NZ] = 7,
	[NRC_CC_AU] = 8,  [NRC_CC_K2] = 9,
	[NRC_CC_SG] = 14, [NRC_CC_T2] = 13, /* TW NCC: reuse TW BD */
	[NRC_CC_T9] = 13,                   /* TW 920MHz: reuse TW BD */
};

/**
 * eu_countries_cc - EU 27 member states plus GB and SA
 *
 * Any alpha-2 in this list shares the EU S1G channel plan and is
 * resolved to NRC_CC_EU by nrc_cc_from_alpha2().
 */
static const char *const eu_countries_cc[] = {
	"AT", "BE", "BG", "CY", "CZ", "DE", "DK", "EE", "ES", "FI",
	"FR", "GR", "HR", "HU", "IE", "IT", "LT", "LU", "LV", "MT",
	"NL", "PL", "PT", "RO", "SE", "SI", "SK", "GB", "SA", NULL
};

/**
 * country_match - test whether a country code appears in a NULL-terminated list
 * @cc:      NULL-terminated array of two-character country code strings
 * @country: two-character country code to search for
 *
 * Returns 1 if found, 0 otherwise.
 */
static inline int country_match(const char *const cc[],
				const char *const country)
{
	int i;

	if (!country)
		return 0;
	for (i = 0; cc[i]; i++) {
		if (cc[i][0] == country[0] && cc[i][1] == country[1])
			return 1;
	}
	return 0;
}

/**
 * nrc_cc_from_alpha2 - resolve an alpha-2 string to enum nrc_country_id
 * @cc: two-character country code (need not be NUL-terminated)
 *
 * EU member states (eu_countries_cc[]) map to NRC_CC_EU.
 * Unknown codes fall back to NRC_CC_US.
 *
 * Note: "KR" is NOT handled here — the caller must resolve it to "K1"
 * or "K2" first (requires access to the kr_band module parameter).
 * "TW" virtual band codes (T2, T9) are also NOT handled here —
 * the caller must apply the tw_band module parameter override first.
 */
static inline enum nrc_country_id nrc_cc_from_alpha2(const char *cc)
{
	int i;

	if (!cc)
		return NRC_CC_US;
	for (i = 0; i < NRC_CC_MAX; i++) {
		if (nrc_cc_alpha2[i][0] == cc[0] &&
		    nrc_cc_alpha2[i][1] == cc[1])
			return (enum nrc_country_id)i;
	}
	/* EU member codes are not in nrc_cc_alpha2[] individually */
	if (country_match(eu_countries_cc, cc))
		return NRC_CC_EU;
	return NRC_CC_US; /* unknown → US fallback */
}

#endif /* _NRC_COUNTRY_H_ */
