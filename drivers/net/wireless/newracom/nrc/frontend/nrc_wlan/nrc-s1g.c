/* Common directory headers - Debug & Trace */
#include "nrc-debug-common.h"
#include "nrc-debug.h"

/* Local module headers */
#include "nrc-s1g.h"
/* Always-available: country string and empty proxy sentinel */
static char s1g_alpha2[3] = "US"; /* default country */
static const struct s1g_proxy_map s1g_proxy_map_empty[] = {{}};

#if defined(CONFIG_S1G_CHANNEL)
static const struct s1g_channel_table s1g_ch_table_empty[] = {{}};

// Japan
static const struct s1g_channel_table s1g_ch_table_jp[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"JP", 9170, 1, BW_1M, 1, 8, 5, 0},
	{"JP", 9180, 3, BW_1M, 1, 8, -5, 1},
	{"JP", 9190, 5, BW_1M, 1, 8, 5, 0},
	{"JP", 9200, 7, BW_1M, 1, 8, -5, 1},
	{"JP", 9210, 9, BW_1M, 1, 8, 5, 0},
	{"JP", 9220, 11, BW_1M, 1, 8, -5, 1},
	{"JP", 9230, 13, BW_1M, 1, 8, 5, 0},
	{"JP", 9240, 15, BW_1M, 1, 8, -5, 1},
	{"JP", 9250, 17, BW_1M, 1, 8, 5, 0},
	{"JP", 9260, 19, BW_1M, 1, 8, -5, 1},
	{"JP", 9270, 21, BW_1M, 1, 8, 5, 0},
	{}};

// Korea (K1) USN band (921MH~923MH)
static const struct s1g_channel_table s1g_ch_table_k1_usn[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"KR", 9215, 1, BW_1M, 1, 14, -5, 1},
	{"KR", 9225, 3, BW_1M, 1, 14, -5, 1},
	{}};

// Korea (K2) MIC Band (925MH~931MHz)
static const struct s1g_channel_table s1g_ch_table_k2_mic[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"KR", 9255, 1, BW_1M, 1, 14, 5, 0},
	{"KR", 9265, 3, BW_1M, 1, 14, -5, 1},
	{"KR", 9275, 5, BW_1M, 1, 14, 5, 0},
	{"KR", 9285, 7, BW_1M, 1, 14, -5, 1},
	{"KR", 9295, 9, BW_1M, 1, 14, 5, 0},
	{"KR", 9305, 11, BW_1M, 1, 14, -5, 1},
#if defined(S1G_INCLUDE_4M_OP_2M_TX)
	{"KR", 9280, 4, BW_4M, 1, 15, 0, 1},
	{"KR", 9300, 8, BW_4M, 1, 15, 0, 1},
#else
	{"KR", 9270, 4, BW_4M, 1, 15, 0, 0},
	{"KR", 9290, 8, BW_4M, 1, 15, 0, 0},
#endif
	{}};

// Taiwan
static const struct s1g_channel_table s1g_ch_table_tw[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"TW", 8390, 1, BW_1M, 2, 1, 5, 0},
	{"TW", 8400, 3, BW_1M, 2, 1, -5, 1},
	{"TW", 8410, 5, BW_1M, 2, 1, 5, 0},
	{"TW", 8420, 7, BW_1M, 2, 1, -5, 1},
	{"TW", 8430, 9, BW_1M, 2, 1, 5, 0},
	{"TW", 8440, 11, BW_1M, 2, 1, -5, 1},
	{"TW", 8450, 13, BW_1M, 2, 1, 5, 0},
	{"TW", 8460, 15, BW_1M, 2, 1, -5, 1},
	{"TW", 8470, 17, BW_1M, 2, 1, 5, 0},
	{"TW", 8480, 19, BW_1M, 2, 1, -5, 1},
	{"TW", 8490, 21, BW_1M, 2, 1, 5, 0},
	{"TW", 8500, 23, BW_1M, 2, 1, -5, 1},
	{"TW", 8510, 25, BW_1M, 2, 1, -5, 1},
	{"TW", 8395, 2, BW_2M, 2, 1, 0, 0},
	{"TW", 8415, 6, BW_2M, 2, 1, 0, 0},
	{"TW", 8435, 10, BW_2M, 2, 1, 0, 0},
	{"TW", 8455, 14, BW_2M, 2, 1, 0, 0},
	{"TW", 8475, 18, BW_2M, 2, 1, 0, 0},
	{"TW", 8495, 22, BW_2M, 2, 1, 0, 0},
	{"TW", 8405, 4, BW_4M, 2, 1, 0, 1},
	{"TW", 8445, 12, BW_4M, 2, 1, 0, 1},
	{"TW", 8485, 20, BW_4M, 2, 1, 0, 1},
	{}};

// US
static const struct s1g_channel_table s1g_ch_table_us[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"US", 9025, 1, BW_1M, 1, 1, 5, 0},
	{"US", 9035, 3, BW_1M, 1, 1, -5, 1},
	{"US", 9045, 5, BW_1M, 2, 1, 5, 0},
	{"US", 9055, 7, BW_1M, 2, 1, -5, 1},
	{"US", 9065, 9, BW_1M, 2, 1, 5, 0},
	{"US", 9075, 11, BW_1M, 2, 1, -5, 1},
	{"US", 9085, 13, BW_1M, 2, 1, 5, 0},
	{"US", 9095, 15, BW_1M, 2, 1, -5, 1},
	{"US", 9105, 17, BW_1M, 2, 1, 5, 0},
	{"US", 9115, 19, BW_1M, 2, 1, -5, 1},
	{"US", 9125, 21, BW_1M, 2, 1, 5, 0},
	{"US", 9135, 23, BW_1M, 2, 1, -5, 1},
	{"US", 9145, 25, BW_1M, 2, 1, 5, 0},
	{"US", 9155, 27, BW_1M, 2, 1, -5, 1},
	{"US", 9165, 29, BW_1M, 2, 1, 5, 0},
	{"US", 9175, 31, BW_1M, 2, 1, -5, 1},
	{"US", 9185, 33, BW_1M, 2, 1, 5, 0},
	{"US", 9195, 35, BW_1M, 2, 1, -5, 1},
	{"US", 9205, 37, BW_1M, 1, 1, 5, 0},
	{"US", 9215, 39, BW_1M, 1, 1, -5, 1},
	{"US", 9225, 41, BW_1M, 1, 1, 5, 0},
	{"US", 9235, 43, BW_1M, 1, 1, -5, 1},
	{"US", 9245, 45, BW_1M, 1, 1, 5, 0},
	{"US", 9255, 47, BW_1M, 1, 1, -5, 1},
	{"US", 9265, 49, BW_1M, 1, 1, 5, 0},
	{"US", 9275, 51, BW_1M, 1, 1, -5, 1},
	{"US", 9030, 2, BW_2M, 1, 2, 0, 0},
	{"US", 9050, 6, BW_2M, 2, 2, 0, 0},
	{"US", 9070, 10, BW_2M, 2, 2, 0, 0},
	{"US", 9090, 14, BW_2M, 2, 2, 0, 0},
	{"US", 9110, 18, BW_2M, 2, 2, 0, 0},
	{"US", 9130, 22, BW_2M, 2, 2, 0, 0},
	{"US", 9150, 26, BW_2M, 2, 2, 0, 0},
	{"US", 9170, 30, BW_2M, 2, 2, 0, 0},
	{"US", 9190, 34, BW_2M, 2, 2, 0, 0},
	{"US", 9210, 38, BW_2M, 1, 2, 0, 0},
	{"US", 9230, 42, BW_2M, 1, 2, 0, 0},
	{"US", 9250, 46, BW_2M, 1, 2, 0, 0},
	{"US", 9270, 50, BW_2M, 1, 2, 0, 0},
	{"US", 9060, 8, BW_4M, 2, 2, 0, 1},
	{"US", 9100, 16, BW_4M, 2, 3, 0, 1},
	{"US", 9140, 24, BW_4M, 2, 3, 0, 1},
	{"US", 9180, 32, BW_4M, 2, 3, 0, 1},
	{"US", 9220, 40, BW_4M, 1, 3, 0, 1},
	{"US", 9260, 48, BW_4M, 1, 3, 0, 1},
	/* Op35 — 2 MHz, S1G ch 128-172, 904-926 MHz (2 MHz step) */
	{"US", 9040, 128, BW_2M, 2, 35, 0, 0},
	{"US", 9060, 132, BW_2M, 2, 35, 0, 0},
	{"US", 9080, 136, BW_2M, 2, 35, 0, 0},
	{"US", 9100, 140, BW_2M, 2, 35, 0, 0},
	{"US", 9120, 144, BW_2M, 2, 35, 0, 0},
	{"US", 9140, 148, BW_2M, 2, 35, 0, 0},
	{"US", 9160, 152, BW_2M, 2, 35, 0, 0},
	{"US", 9180, 156, BW_2M, 2, 35, 0, 0},
	{"US", 9200, 160, BW_2M, 2, 35, 0, 0},
	{"US", 9220, 164, BW_2M, 2, 35, 0, 0},
	{"US", 9240, 168, BW_2M, 2, 35, 0, 0},
	{"US", 9260, 172, BW_2M, 2, 35, 0, 0},
	/* Op36 — 4 MHz, S1G ch 130-170, 905-925 MHz (4 MHz step) */
	{"US", 9050, 130, BW_4M, 2, 36, 0, 1},
	{"US", 9090, 138, BW_4M, 2, 36, 0, 1},
	{"US", 9130, 146, BW_4M, 2, 36, 0, 1},
	{"US", 9170, 154, BW_4M, 2, 36, 0, 1},
	{"US", 9210, 162, BW_4M, 2, 36, 0, 1},
	{"US", 9250, 170, BW_4M, 2, 36, 0, 1},
	{}};

// EU
static const struct s1g_channel_table s1g_ch_table_eu[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"EU", 8635, 1, BW_1M, 1, 6, 5, 0},
	{"EU", 8645, 3, BW_1M, 1, 6, -5, 1},
	{"EU", 8655, 5, BW_1M, 1, 6, 5, 0},
	{"EU", 8665, 7, BW_1M, 1, 6, -5, 1},
	{"EU", 8675, 9, BW_1M, 1, 6, -5, 1},
	{"EU", 8640, 2, BW_2M, 2, 7, 0, 0},
	{"EU", 8660, 6, BW_2M, 2, 7, 0, 0},
	{}};

// China - test channel
/*
static const struct s1g_channel_table s1g_ch_table_cn[]= {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"CN",	4330,	1,	BW_1M,	1,	20,		5,	0},

	{"CN",	4725,	3,	BW_1M,	1,	20,		5,	0},
	{"CN",	4720,	2,	BW_2M,	2,	21,		0,	0},
	{"CN",	4730,	4,	BW_2M,	2,	22,		0,	1},

	{"CN",	4785,	6,	BW_1M,	1,	20,		5,	0},
	{"CN",	4780,	5,	BW_2M,	2,	21,		0,	0},
	{"CN",	4790,	7,	BW_4M,	2,	22,		0,	1},

	{"CN",	4845,	9,	BW_1M,	1,	20, 	5,	0},
	{"CN",	4840,	8,	BW_2M,	2,	21, 	0,	0},
	{"CN",	4850,	10,	BW_4M,	2,	22, 	0,	1},

	{"CN",	4905,	12,	BW_1M,	1,	20, 	5,	0},
	{"CN",	4900,	11,	BW_2M,	2,	21, 	0,	0},
	{"CN",	4910,	13,	BW_4M,	2,	22, 	0,	1},

	{"CN",	4965,	15,	BW_1M,	1,	20, 	5,	0},
	{"CN",	4960,	14,	BW_2M,	2,	21, 	0,	0},
	{"CN",	4970,	16,	BW_4M,	2,	22, 	0,	1},

	{"CN",	5025,	18,	BW_1M,	1,	20, 	5,	0},
	{"CN",	5020,	17,	BW_2M,	2,	21, 	0,	0},
	{"CN",	5030,	19,	BW_4M,	2,	22, 	0,	1},

	{"CN",	5085,	21,	BW_1M,	1,	20, 	5,	0},
	{"CN",	5080,	20,	BW_2M,	2,	21, 	0,	0},
	{"CN",	5090,	22,	BW_4M,	2,	22, 	0,	1},

	{"CN",	5055,	24,	BW_1M,	1,	20, 	5,	0},
	{"CN",	5065,	23,	BW_1M,	1,	20, 	5,	0},
	{}
};
*/
// China
static const struct s1g_channel_table s1g_ch_table_cn[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"CN", 7555, 1, BW_1M, 1, 9, 5, 0},
	{"CN", 7565, 3, BW_1M, 1, 9, -5, 1},
	{"CN", 7575, 5, BW_1M, 1, 9, 5, 0},
	{"CN", 7585, 7, BW_1M, 1, 9, -5, 1},
	{"CN", 7595, 9, BW_1M, 1, 9, 5, 0},
	{"CN", 7605, 11, BW_1M, 1, 9, -5, 1},
	{"CN", 7615, 13, BW_1M, 1, 9, 5, 0},
	{"CN", 7625, 15, BW_1M, 1, 9, -5, 1},
	{"CN", 7635, 17, BW_1M, 1, 9, 5, 0},
	{"CN", 7645, 19, BW_1M, 1, 9, -5, 1},
	{"CN", 7655, 21, BW_1M, 1, 9, 5, 0},
	{"CN", 7665, 23, BW_1M, 1, 9, -5, 1},
	{"CN", 7675, 25, BW_1M, 1, 9, 5, 0},
	{"CN", 7685, 27, BW_1M, 1, 9, -5, 1},
	{"CN", 7695, 29, BW_1M, 1, 9, 5, 0},
	{"CN", 7705, 31, BW_1M, 1, 9, -5, 1},
	{"CN", 7795, 16, BW_1M, 1, 10, 5, 0},
	{"CN", 7805, 18, BW_1M, 1, 10, -5, 1},
	{"CN", 7815, 20, BW_1M, 1, 10, 5, 0},
	{"CN", 7825, 22, BW_1M, 1, 10, -5, 1},
	{"CN", 7835, 24, BW_1M, 1, 10, 5, 0},
	{"CN", 7845, 26, BW_1M, 1, 10, -5, 1},
	{"CN", 7855, 28, BW_1M, 1, 10, 5, 0},
	{"CN", 7865, 30, BW_1M, 1, 10, -5, 1},
	{"CN", 7800, 2, BW_2M, 2, 11, 0, 0},
	{"CN", 7820, 6, BW_2M, 2, 11, 0, 0},
	{"CN", 7840, 10, BW_2M, 2, 11, 0, 0},
	{"CN", 7860, 14, BW_2M, 2, 11, 0, 0},
	{"CN", 7810, 4, BW_4M, 2, 12, 0, 1},
	{"CN", 7850, 12, BW_4M, 2, 12, 0, 1},
	{}};

// New Zealand
static const struct s1g_channel_table s1g_ch_table_nz[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"NZ", 9155, 27, BW_1M, 1, 26, 5, 0},
	{"NZ", 9165, 29, BW_1M, 1, 26, -5, 1},
	{"NZ", 9175, 31, BW_1M, 1, 26, 5, 0},
	{"NZ", 9185, 33, BW_1M, 1, 26, -5, 1},
	{"NZ", 9195, 35, BW_1M, 1, 26, 5, 0},
	{"NZ", 9205, 37, BW_1M, 1, 26, -5, 1},
	{"NZ", 9215, 39, BW_1M, 1, 26, 5, 0},
	{"NZ", 9225, 41, BW_1M, 1, 26, -5, 1},
	{"NZ", 9235, 43, BW_1M, 1, 26, 5, 0},
	{"NZ", 9245, 45, BW_1M, 2, 26, 5, 0},
	{"NZ", 9255, 47, BW_1M, 2, 26, -5, 1},
	{"NZ", 9265, 49, BW_1M, 2, 26, 5, 0},
	{"NZ", 9275, 51, BW_1M, 2, 26, -5, 1},
	{"NZ", 9170, 30, BW_2M, 1, 27, 0, 0},
	{"NZ", 9190, 34, BW_2M, 1, 27, 0, 0},
	{"NZ", 9210, 38, BW_2M, 1, 27, 0, 0},
	{"NZ", 9230, 42, BW_2M, 1, 27, 0, 0},
	{"NZ", 9250, 46, BW_2M, 2, 27, 0, 0},
	{"NZ", 9270, 50, BW_2M, 2, 27, 0, 0},
	{"NZ", 9180, 32, BW_4M, 1, 28, 0, 1},
	{"NZ", 9220, 40, BW_4M, 1, 28, 0, 1},
	{"NZ", 9260, 48, BW_4M, 2, 28, 0, 1},
	{}};

// Australia
static const struct s1g_channel_table s1g_ch_table_au[] = {
	//cc, freq,channel, bw,	cca, 	oper,offset,pri_loc
	{"AU", 9155, 27, BW_1M, 1, 22, 5, 0},
	{"AU", 9165, 29, BW_1M, 1, 22, -5, 1},
	{"AU", 9175, 31, BW_1M, 1, 22, 5, 0},
	{"AU", 9185, 33, BW_1M, 1, 22, -5, 1},
	{"AU", 9195, 35, BW_1M, 1, 22, 5, 0},
	{"AU", 9205, 37, BW_1M, 2, 22, 5, 0},
	{"AU", 9215, 39, BW_1M, 2, 22, -5, 1},
	{"AU", 9225, 41, BW_1M, 2, 22, 5, 0},
	{"AU", 9235, 43, BW_1M, 2, 22, -5, 1},
	{"AU", 9245, 45, BW_1M, 2, 22, 5, 0},
	{"AU", 9255, 47, BW_1M, 2, 22, -5, 1},
	{"AU", 9265, 49, BW_1M, 2, 22, 5, 0},
	{"AU", 9275, 51, BW_1M, 2, 22, -5, 1},
	{"AU", 9170, 30, BW_2M, 1, 23, 0, 0},
	{"AU", 9190, 34, BW_2M, 1, 23, 0, 0},
	{"AU", 9210, 38, BW_2M, 2, 23, 0, 0},
	{"AU", 9230, 42, BW_2M, 2, 23, 0, 0},
	{"AU", 9250, 46, BW_2M, 2, 23, 0, 0},
	{"AU", 9270, 50, BW_2M, 2, 23, 0, 0},
	{"AU", 9180, 32, BW_4M, 1, 24, 0, 1},
	{"AU", 9220, 40, BW_4M, 2, 24, 0, 1},
	{"AU", 9260, 48, BW_4M, 2, 24, 0, 1},
	{}};

#endif /* CONFIG_S1G_CHANNEL */

/* ---------------------------------------------------------------
 * Proxy-channel tables: always compiled (independent of S1G mode).
 * ---------------------------------------------------------------*/
static const struct s1g_proxy_map s1g_proxy_table_us[] = {
	{9025, 2412, 1, 1},    {9035, 2422, 3, 3},
	{9045, 2432, 5, 5},    {9055, 2442, 7, 7},
	{9065, 2452, 9, 9},    {9075, 2462, 11, 11},
	{9085, 5180, 13, 36},  {9095, 5185, 15, 37},
	{9105, 5190, 17, 38},  {9115, 5195, 19, 39},
	{9125, 5200, 21, 40},  {9135, 5205, 23, 41},
	{9145, 5210, 25, 42},  {9155, 5215, 27, 43},
	{9165, 5220, 29, 44},  {9175, 5225, 31, 45},
	{9185, 5230, 33, 46},  {9195, 5235, 35, 47},
	{9205, 5240, 37, 48},  {9215, 5745, 39, 149},
	{9225, 5750, 41, 150}, {9235, 5755, 43, 151},
	{9245, 5760, 45, 152}, {9255, 5500, 47, 100},
	{9265, 5520, 49, 104}, {9275, 5540, 51, 108},
	{9030, 2417, 2, 2},    {9050, 2437, 6, 6},
	{9070, 2457, 10, 10},  {9090, 5765, 14, 153},
	{9110, 5770, 18, 154}, {9130, 5775, 22, 155},
	{9150, 5780, 26, 156}, {9170, 5785, 30, 157},
	{9190, 5790, 34, 158}, {9210, 5795, 38, 159},
	{9230, 5800, 42, 160}, {9250, 5805, 46, 161},
	{9270, 5560, 50, 112}, {9060, 2447, 8, 8},
	{9100, 5810, 16, 162}, {9140, 5815, 24, 163},
	{9180, 5820, 32, 164}, {9220, 5825, 40, 165},
	{9260, 5580, 48, 116}, {9040, 5250, 128, 50},
	{9060, 5260, 132, 52}, {9080, 5270, 136, 54},
	{9100, 5280, 140, 56}, {9120, 5290, 144, 58},
	{9140, 5300, 148, 60}, {9160, 5310, 152, 62},
	{9180, 5320, 156, 64}, {9200, 5330, 160, 66},
	{9220, 5340, 164, 68}, {9240, 5350, 168, 70},
	{9260, 5360, 172, 72}, {9050, 5380, 130, 76},
	{9090, 5400, 138, 80}, {9130, 5420, 146, 84},
	{9170, 5440, 154, 88}, {9210, 5460, 162, 92},
	{9250, 5480, 170, 96}, {}};

static const struct s1g_proxy_map s1g_proxy_table_jp[] = {{9210, 5200, 9, 40},
							  {9230, 5210, 13, 42},
							  {9240, 5215, 15, 43},
							  {9250, 5220, 17, 44},
							  {9260, 5225, 19, 45},
							  {9270, 5230, 21, 46},
							  {9235, 5180, 2, 36},
							  {9245, 5185, 4, 37},
							  {9255, 5190, 6, 38},
							  {9265, 5195, 8, 39},
							  {9245, 5235, 36, 47},
							  {9255, 5240, 38, 48},
							  {}};

static const struct s1g_proxy_map s1g_proxy_table_k1[] = {
	{9215, 5180, 1, 36}, {9225, 5185, 3, 37}, {}};

static const struct s1g_proxy_map s1g_proxy_table_tw[] = {
	{8390, 5180, 1, 36},   {8400, 5185, 3, 37},   {8410, 5190, 5, 38},
	{8420, 5195, 7, 39},   {8430, 5200, 9, 40},   {8440, 5205, 11, 41},
	{8450, 5210, 13, 42},  {8460, 5215, 15, 43},  {8470, 5220, 17, 44},
	{8480, 5225, 19, 45},  {8490, 5230, 21, 46},  {8500, 5235, 23, 47},
	{8510, 5240, 25, 48},  {8395, 5745, 2, 149},  {8415, 5750, 6, 150},
	{8435, 5755, 10, 151}, {8455, 5760, 14, 152}, {8475, 5765, 18, 153},
	{8495, 5770, 22, 154}, {8405, 5775, 4, 155},  {8445, 5780, 12, 156},
	{8485, 5785, 20, 157}, {}};

/* TW NCC 920 MHz band (T2, tw_band=2): proxy ch36-43 → 920.5-924.5 MHz */
static const struct s1g_proxy_map s1g_proxy_table_t2[] = {
	{9205, 5180, 37, 36},  {9215, 5185, 39, 37},  {9225, 5190, 41, 38},
	{9235, 5195, 43, 39},  {9245, 5200, 45, 40},  {9210, 5205, 38, 41},
	{9230, 5210, 42, 42},  {9220, 5215, 40, 43},  {}};

/* TW 920 MHz band (T9, tw_band=9): proxy ch36-42 → 921.0-924.0 MHz */
static const struct s1g_proxy_map s1g_proxy_table_t9[] = {
	{9210, 5180, 38, 36},  {9220, 5185, 40, 37},  {9230, 5190, 42, 38},
	{9240, 5195, 44, 39},  {9215, 5200, 39, 40},  {9235, 5205, 43, 41},
	{9225, 5210, 41, 42},  {}};

static const struct s1g_proxy_map s1g_proxy_table_eu[] = {{8635, 5180, 1, 36},
							  {8645, 5185, 3, 37},
							  {8655, 5190, 5, 38},
							  {8665, 5195, 7, 39},
							  {8675, 5200, 9, 40},
							  {8685, 5205, 11, 41},
							  {8695, 5210, 13, 42},
							  {8640, 5215, 2, 43},
							  {8660, 5220, 6, 44},
							  {8680, 5225, 10, 45},
							  {8650, 5230, 4, 46},
							  {8670, 5235, 8, 47},
							  {}};

static const struct s1g_proxy_map s1g_proxy_table_cn[] = {
	{7555, 5180, 1, 36},   {7565, 5185, 3, 37},
	{7575, 5190, 5, 38},   {7585, 5195, 7, 39},
	{7595, 5200, 9, 40},   {7605, 5205, 11, 41},
	{7615, 5210, 13, 42},  {7625, 5215, 15, 43},
	{7635, 5220, 17, 44},  {7645, 5225, 19, 45},
	{7655, 5230, 21, 46},  {7665, 5235, 23, 47},
	{7675, 5240, 25, 48},  {7795, 5745, 16, 149},
	{7805, 5750, 18, 150}, {7815, 5755, 20, 151},
	{7825, 5760, 22, 152}, {7835, 5765, 24, 153},
	{7845, 5770, 26, 154}, {7855, 5775, 28, 155},
	{7865, 5780, 30, 156}, {7800, 5185, 2, 37},
	{7820, 5200, 6, 40},   {7840, 5215, 10, 43},
	{7860, 5230, 14, 46},  {7810, 5190, 4, 38},
	{7850, 5215, 12, 43},  {}};

static const struct s1g_proxy_map s1g_proxy_table_nz[] = {{9155, 5180, 27, 36},
							  {9165, 5185, 29, 37},
							  {9175, 5190, 31, 38},
							  {9185, 5195, 33, 39},
							  {9195, 5200, 35, 40},
							  {9205, 5205, 37, 41},
							  {9215, 5210, 39, 42},
							  {9225, 5215, 41, 43},
							  {9235, 5220, 43, 44},
							  {9245, 5225, 45, 45},
							  {9255, 5230, 47, 46},
							  {9265, 5235, 49, 47},
							  {9275, 5240, 51, 48},
							  {9170, 5765, 30, 153},
							  {9190, 5770, 34, 154},
							  {9210, 5775, 38, 155},
							  {9230, 5780, 42, 156},
							  {9250, 5785, 46, 157},
							  {9270, 5790, 50, 158},
							  {9180, 5810, 32, 162},
							  {9220, 5815, 40, 163},
							  {9260, 5820, 48, 164},
							  {}};

static const struct s1g_proxy_map s1g_proxy_table_au[] = {{9155, 5180, 27, 36},
							  {9165, 5185, 29, 37},
							  {9175, 5190, 31, 38},
							  {9185, 5195, 33, 39},
							  {9195, 5200, 35, 40},
							  {9205, 5205, 37, 41},
							  {9215, 5210, 39, 42},
							  {9225, 5215, 41, 43},
							  {9235, 5220, 43, 44},
							  {9245, 5225, 45, 45},
							  {9255, 5230, 47, 46},
							  {9265, 5235, 49, 47},
							  {9275, 5240, 51, 48},
							  {9170, 5765, 30, 153},
							  {9190, 5770, 34, 154},
							  {9210, 5775, 38, 155},
							  {9230, 5780, 42, 156},
							  {9250, 5785, 46, 157},
							  {9270, 5790, 50, 158},
							  {9180, 5810, 32, 162},
							  {9220, 5815, 40, 163},
							  {9260, 5820, 48, 164},
							  {}};

static const struct s1g_proxy_map s1g_proxy_table_k2[] = {
	{9255, 5180, 1, 36}, {9265, 5185, 3, 37}, {9275, 5190, 5, 38},
	{9285, 5195, 7, 39}, {9295, 5200, 9, 40}, {9305, 5205, 11, 41},
	{9280, 5210, 4, 42}, {9300, 5215, 8, 43}, {}};

/* SG dual-band: 866 MHz (S8) + 920 MHz (S9) */
static const struct s1g_proxy_map s1g_proxy_table_sg[] = {
	/* 866 MHz band: proxy ch36-39 */
	{8665, 5180,  7, 36}, {8675, 5185,  9, 37},
	{8685, 5190, 11, 38}, {8680, 5195, 10, 39},
	/* 920 MHz band: proxy ch40-47, ch149-154 */
	{9175, 5200, 31, 40},  {9185, 5205, 33, 41},  {9195, 5210, 35, 42},
	{9205, 5215, 37, 43},  {9215, 5220, 39, 44},  {9225, 5225, 41, 45},
	{9235, 5230, 43, 46},  {9245, 5235, 45, 47},
	{9180, 5745, 32, 149}, {9200, 5750, 36, 150}, {9220, 5755, 40, 151},
	{9240, 5760, 44, 152}, {9190, 5765, 34, 153}, {9230, 5770, 42, 154},
	{}};

#if defined(CONFIG_S1G_CHANNEL)
static const struct s1g_channel_table
	*const s1g_ch_table_set[NRC_CC_MAX] = {
		[NRC_CC_US] = s1g_ch_table_us,    [NRC_CC_JP] = s1g_ch_table_jp,
		[NRC_CC_K0] = s1g_ch_table_empty, [NRC_CC_K1] = s1g_ch_table_k1_usn,
		[NRC_CC_TW] = s1g_ch_table_tw,    [NRC_CC_EU] = s1g_ch_table_eu,
		[NRC_CC_CN] = s1g_ch_table_cn,    [NRC_CC_NZ] = s1g_ch_table_nz,
		[NRC_CC_AU] = s1g_ch_table_au,    [NRC_CC_K2] = s1g_ch_table_k2_mic,
		[NRC_CC_SG] = s1g_ch_table_empty, [NRC_CC_T2] = s1g_ch_table_empty,
		[NRC_CC_T9] = s1g_ch_table_empty,
};
#endif /* CONFIG_S1G_CHANNEL */

static const struct s1g_proxy_map *const s1g_proxy_map_set[NRC_CC_MAX] = {
	[NRC_CC_US] = s1g_proxy_table_us,  [NRC_CC_JP] = s1g_proxy_table_jp,
	[NRC_CC_K0] = s1g_proxy_map_empty, [NRC_CC_K1] = s1g_proxy_table_k1,
	[NRC_CC_TW] = s1g_proxy_table_tw,  [NRC_CC_EU] = s1g_proxy_table_eu,
	[NRC_CC_CN] = s1g_proxy_table_cn,  [NRC_CC_NZ] = s1g_proxy_table_nz,
	[NRC_CC_AU] = s1g_proxy_table_au,  [NRC_CC_K2] = s1g_proxy_table_k2,
	[NRC_CC_SG] = s1g_proxy_table_sg,  [NRC_CC_T2] = s1g_proxy_table_t2,
	[NRC_CC_T9] = s1g_proxy_table_t9,
};

#if defined(CONFIG_S1G_CHANNEL)
static const struct s1g_channel_table *ptr_nrc_s1g_ch_table = s1g_ch_table_us;
#endif /* CONFIG_S1G_CHANNEL */
static const struct s1g_proxy_map *ptr_nrc_s1g_proxy_map = s1g_proxy_table_us;
static struct bd_supp_param g_supp_ch_list;
static bool g_supp_ch_list_ready;

static void nrc_s1g_clear_supp_ch_list(void)
{
	memset(&g_supp_ch_list, 0, sizeof(g_supp_ch_list));
	g_supp_ch_list_ready = false;
}

void nrc_s1g_build_supp_ch_list(void)
{
	int i;

	nrc_s1g_clear_supp_ch_list();

	for (i = 0; i < NRC_BD_MAX_CH_LIST && ptr_nrc_s1g_proxy_map[i].s1g_freq;
	     i++) {
		g_supp_ch_list.s1g_ch_index[i] =
			ptr_nrc_s1g_proxy_map[i].s1g_ch_idx;
		g_supp_ch_list.nons1g_ch_freq[i] =
			ptr_nrc_s1g_proxy_map[i].proxy_freq;
		g_supp_ch_list.s1g_ch_freq[i] =
			ptr_nrc_s1g_proxy_map[i].s1g_freq;
		g_supp_ch_list.num_ch++;
	}

	g_supp_ch_list_ready = g_supp_ch_list.num_ch > 0;
}

const struct bd_supp_param *nrc_s1g_get_supp_ch_list(void)
{
	return g_supp_ch_list_ready ? &g_supp_ch_list : NULL;
}

bool nrc_s1g_is_proxy_freq_supported(u32 proxy_mhz)
{
	const struct bd_supp_param *supp = nrc_s1g_get_supp_ch_list();
	int i;

	if (!supp)
		return false;

	for (i = 0; i < supp->num_ch; i++) {
		if (supp->nons1g_ch_freq[i] == proxy_mhz)
			return true;
	}

	return false;
}

enum nrc_country_id nrc_get_current_ccid_by_country(const char *country_code)
{
	return nrc_cc_from_alpha2(country_code);
}

char *nrc_get_current_s1g_country(void)
{
	return s1g_alpha2;
}

void nrc_set_s1g_country(char *country_code)
{
	enum nrc_country_id cc_id;
	extern int tw_band;

	/* Apply TW band override: tw_band selects T2 or T9 proxy table */
	if (country_code[0] == 'T' && country_code[1] == 'W') {
		if (tw_band == 2)
			cc_id = NRC_CC_T2;
		else if (tw_band == 9)
			cc_id = NRC_CC_T9;
		else
			cc_id = NRC_CC_TW; /* default: TW 840 MHz (T8) */
	} else {
		cc_id = nrc_cc_from_alpha2(country_code);
	}

	if (cc_id == NRC_CC_US &&
	    !(country_code[0] == 'U' && country_code[1] == 'S'))
		WARN_MAC("%s: unknown country '%c%c', using US defaults",
			 __func__, country_code[0], country_code[1]);

	/* Use normalized alpha2 (EU members → "EU", unknown → "US") */
	s1g_alpha2[0] = nrc_cc_alpha2[cc_id][0];
	s1g_alpha2[1] = nrc_cc_alpha2[cc_id][1];
	s1g_alpha2[2] = '\0';
#if defined(CONFIG_S1G_CHANNEL)
	ptr_nrc_s1g_ch_table = s1g_ch_table_set[cc_id];
#endif
	ptr_nrc_s1g_proxy_map = s1g_proxy_map_set[cc_id];
	nrc_s1g_build_supp_ch_list();

	DBG_MAC("%s: Country code %s, proxy channels: %d", __func__,
		country_code, g_supp_ch_list.num_ch);
}

#if defined(CONFIG_S1G_CHANNEL)
const struct s1g_channel_table *nrc_get_current_s1g_cc_table(void)
{
	return ptr_nrc_s1g_ch_table;
}

int nrc_get_num_channels_by_current_country(void)
{
	int i = 0;
	for (i = 0; i < MAX_S1G_CHANNEL_NUM; i++) {
		if (!ptr_nrc_s1g_ch_table[i].s1g_freq)
			return i;
	}
	return MAX_S1G_CHANNEL_NUM;
}

int nrc_get_s1g_freq_by_arr_idx(int arr_index)
{
	return ptr_nrc_s1g_ch_table[arr_index].s1g_freq;
}

int nrc_get_s1g_width_by_freq(int freq)
{
	int i = 0;

	for (i = 0; i < nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].chan_spacing;
	}
	return ptr_nrc_s1g_ch_table[0].chan_spacing;
}

int nrc_get_s1g_width_by_arr_idx(int arr_index)
{
	return ptr_nrc_s1g_ch_table[arr_index].chan_spacing;
}

void nrc_s1g_set_channel_bw(int freq, struct cfg80211_chan_def *chandef)
{
	int w;

	w = nrc_get_s1g_width_by_freq(freq);
	switch (w) {
	default:
	case 1:
		chandef->width = NL80211_CHAN_WIDTH_1;
		break;
	case 2:
		chandef->width = NL80211_CHAN_WIDTH_2;
		break;
	case 4:
		chandef->width = NL80211_CHAN_WIDTH_4;
		break;
	}
}

uint8_t nrc_get_channel_idx_by_freq(int freq)
{
	int i = 0;
	for (i = 0; i < nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].s1g_freq_index;
	}
	return ptr_nrc_s1g_ch_table[0].s1g_freq_index;
}

uint8_t nrc_get_cca_by_freq(int freq)
{
	int i = 0;
	for (i = 0; i < nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].cca_level_type;
	}
	return ptr_nrc_s1g_ch_table[0].cca_level_type;
}

uint8_t nrc_get_oper_class_by_freq(int freq)
{
	int i = 0;
	for (i = 0; i < nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].global_oper_class;
	}
	return ptr_nrc_s1g_ch_table[0].global_oper_class;
}

uint8_t nrc_get_offset_by_freq(int freq)
{
	int i = 0;
	for (i = 0; i < nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].offset;
	}
	return ptr_nrc_s1g_ch_table[0].offset;
}

uint8_t nrc_get_pri_loc_by_freq(int freq)
{
	int i = 0;
	for (i = 0; i < nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq)
			return ptr_nrc_s1g_ch_table[i].primary_loc;
	}
	return ptr_nrc_s1g_ch_table[0].primary_loc;
}

/*
 * BW-aware lookup helpers for US Op35/Op36 support.
 *
 * Op35 (2 MHz) and Op36 (4 MHz) share the same S1G base frequencies as
 * existing channels.  Freq-only lookups return the first table match, which
 * may have the wrong BW/oper_class/chan_index.  These functions match both
 * freq and chan_spacing (BW enum) to select the correct entry.
 *
 * Fallback to freq-only if no exact (freq, bw) pair is found, with a
 * WARN_WLAN so mismatches are visible during development.
 */
int nrc_nl80211_width_to_s1g_bw(enum nl80211_chan_width width)
{
	switch (width) {
	case NL80211_CHAN_WIDTH_2:
		return BW_2M;
	case NL80211_CHAN_WIDTH_4:
		return BW_4M;
	case NL80211_CHAN_WIDTH_1:
	default:
		return BW_1M;
	}
}

static const struct s1g_channel_table *nrc_find_ch_by_freq_bw(int freq, int bw)
{
	int i;

	for (i = 0; i < nrc_get_num_channels_by_current_country(); i++) {
		if (ptr_nrc_s1g_ch_table[i].s1g_freq == freq &&
		    ptr_nrc_s1g_ch_table[i].chan_spacing == bw)
			return &ptr_nrc_s1g_ch_table[i];
	}
	return NULL;
}

uint8_t nrc_get_channel_idx_by_freq_bw(int freq, int bw)
{
	const struct s1g_channel_table *ch = nrc_find_ch_by_freq_bw(freq, bw);

	if (!ch) {
		WARN_MAC(
			"no exact (freq=%d bw=%d) match, fallback to freq-only",
			freq, bw);
		return nrc_get_channel_idx_by_freq(freq);
	}
	return ch->s1g_freq_index;
}

uint8_t nrc_get_cca_by_freq_bw(int freq, int bw)
{
	const struct s1g_channel_table *ch = nrc_find_ch_by_freq_bw(freq, bw);

	if (!ch)
		return nrc_get_cca_by_freq(freq);
	return ch->cca_level_type;
}

uint8_t nrc_get_oper_class_by_freq_bw(int freq, int bw)
{
	const struct s1g_channel_table *ch = nrc_find_ch_by_freq_bw(freq, bw);

	if (!ch)
		return nrc_get_oper_class_by_freq(freq);
	return ch->global_oper_class;
}

uint8_t nrc_get_offset_by_freq_bw(int freq, int bw)
{
	const struct s1g_channel_table *ch = nrc_find_ch_by_freq_bw(freq, bw);

	if (!ch)
		return nrc_get_offset_by_freq(freq);
	return ch->offset;
}

uint8_t nrc_get_pri_loc_by_freq_bw(int freq, int bw)
{
	const struct s1g_channel_table *ch = nrc_find_ch_by_freq_bw(freq, bw);

	if (!ch)
		return nrc_get_pri_loc_by_freq(freq);
	return ch->primary_loc;
}
#endif /* CONFIG_S1G_CHANNEL */
